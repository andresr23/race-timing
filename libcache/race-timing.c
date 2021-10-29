#define _GNU_SOURCE

#include "race-timing.h"

#include <pthread.h>

/* 'nop' Instruction Sequences. */
#define NOP_1 asm volatile("nop")
#define NOP_2   NOP_1  ; NOP_1
#define NOP_4   NOP_2  ; NOP_2
#define NOP_8   NOP_4  ; NOP_4
#define NOP_16  NOP_8  ; NOP_8
#define NOP_32  NOP_16 ; NOP_16
#define NOP_64  NOP_32 ; NOP_32
#define NOP_128 NOP_64 ; NOP_64
#define NOP_256 NOP_128; NOP_128
#define NOP_512 NOP_256; NOP_256

#define MFENCE asm volatile("mfence")
#define LFENCE asm volatile("lfence")
#define SFENCE asm volatile("sfence")

#define RDTSCP asm volatile("rdtscp" ::: "rax", "rcx", "rdx")

#define CPUID  asm volatile("cpuid" ::"a" (0) : "rbx", "rcx", "rdx")

static struct {
  int core;
  int mode;
  int ready;
} rt_server_config = {0};

static pthread_t rt_server_thread;

// Race-Timing cache lines ---------------------------------------
__attribute__((aligned (4096))) static uint32_t race_cl[16] = {0};
__attribute__((aligned (4096))) static uint32_t flag_cl[16] = {0};

#if defined(ZEN) /*------------------------------------------------------------ AMD's Zen */
__attribute__((aligned(64))) void * rt_server_l1d(void *info)
{
  __attribute__((aligned (4096))) int stack_align[16] = {0};
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(rt_server_config.core, &cpuset);
  pthread_setaffinity_np(rt_server_thread, sizeof(cpu_set_t), &cpuset);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  rt_server_config.ready = 1;
  // Single-Core (SMT)
  if (rt_server_config.mode == RT_MODE_SC) {
    while (1) {
      if (flag_cl[0]) {
#if defined(LFENCE_ALLOWS_SPECULATION)
        asm volatile("lfence\n\trdtscp" ::: "rax", "rcx", "rdx");
        race_cl[0] = 1; // r <- true
        SFENCE;
        flag_cl[0] = 0; // f <- false
        MFENCE;
#else /* 'lfence' is an speculation barrier. */
        race_cl[0] = 1; // r <- true
        SFENCE;
        flag_cl[0] = 0; // f <- false
#endif /* ?LFENCE_ALLOWS_SPECULATION, single-core mode. */
      }
    }
  // Multi-Core
  } else {
    while (1) {
      if (flag_cl[0]) {
#if defined(LFENCE_ALLOWS_SPECULATION)
        RDTSCP;
        asm volatile(
          "LOCK btsl $0, 0(%%rax) \n\t" // r <- true
          "sfence                 \n\t"
          "LOCK btrl $0, 0(%%rbx) \n\t" // f <- false
          "mfence                     "
          :: "a" (&race_cl[0]), "b" (&flag_cl[0]) : "memory"
        );
#else /* 'lfence' is an speculation barrier. */
        NOP_128;
        asm volatile(
          "LOCK btsl $0, 0(%%rax) \n\t" // r <- true
          "sfence                 \n\t"
          "LOCK btrl $0, 0(%%rbx) \n\t" // f <- false
          "mfence                     "
          :: "a" (&race_cl[0]), "b" (&flag_cl[0]) : "memory"
        );
#endif /* ?LFENCE_ALLOWS_SPECULATION, multi-core mode. */
      }
    }
  }
}

#if defined(LFENCE_ALLOWS_SPECULATION)

/* race_interim = 20 for 100% if 'lfence' is NOT configured as a spec barrier. */
__attribute__((aligned(64), noinline)) uint32_t rt_load_sc(void *address, int ri, int od)
{
  // (Optimal) Detachment
  asm volatile(
    "1:      \n\t"
    "loop 1b \n\t"
    "rdtscp      "
    :
    : "c" (od)
    : "rax", "rdx"
  );
  // (Optional) Replace OD with memory barriers and 'rdtscp' for less accuracy.
  //SFENCE; LFENCE; RDTSCP;
  // (Optimal) Replace OD with an 'mfence-rdtscp' instruction sequence.
  //MFENCE; RDTSCP;
  // (Optional) Replace OD with a 'cpuid-rdtscp' instruction sequence.
  //CPUID; RDTSCP;
  race_cl[0] = 0; // r <- false
  flag_cl[0] = 1; // f <- true
  asm volatile(
    "mfence               \n\t"
    "movq 0(%%rax), %%rax \n\t" // Memory load
    "lfence               \n\t" // Memory fence
    "rdtscp               \n\t" // Serialization
    "movl %%ebx, %%ecx    \n  "
    "1:                   \n\t"
    "loop 1b              \n\t" // Race Interim
    :
    : "a" (address), "b" (ri)
    : "memory", "rcx", "rdx"
  );
  return race_cl[0]; // Return r
}

__attribute__((aligned(64), noinline)) uint32_t rt_load_mc(void *address, int ri, int od)
{
  while (flag_cl[0]);
  asm volatile(
    "1:      \n\t"
    "loop 1b \n\t"
    "rdtscp      "
    :
    : "c" (od)
    : "rax", "rdx"
  );
  // (Optimal) Replace OD with an 'mfence-rdtscp' instruction sequence.
  //MFENCE; RDTSCP;
  // (Optional) Replace OD with a 'cpuid-rdtscp' instruction sequence.
  //CPUID; RDTSCP;
  asm volatile(
    "LOCK btrl $0, 0(%%rcx) \n\t" // r <- false
    "LOCK btsl $0, 0(%%rdx) \n\t" // f <- true
    "mfence                 \n\t"
    "movq 0(%%rax), %%rax   \n\t" // Memory load
    "lfence                 \n\t" // Memory fence
    "rdtscp                 \n\t" // Serialization
    "movl %%ebx, %%ecx      \n  "
    "1:                     \n\t"
    "loop 1b                    " // Race Interim
    :
    : "a" (address), "b" (ri), "c" (&race_cl[0]), "d" (&flag_cl[0])
    : "memory"
  );
  return race_cl[0]; // Return r
}

#else /* 'lfence' is an speculation barrier. */

/* race_interim = 20 for 100%. */
__attribute__((aligned(64), noinline)) uint32_t rt_load_sc(void *address, int ri, int od)
{
  // Out-of-Order Detachment (OD)
  asm volatile(
    "1:      \n\t"
    "loop 1b \n\t"
    "lfence      "
    :
    : "c" (od)
    :
  );
  race_cl[0] = 0; // r <- false
  flag_cl[0] = 1; // f <- true
  asm volatile(
    "mfence               \n\t"
    "movq 0(%%rax), %%rax \n\t" // Memory load
    "lfence               \n  " // Memory fence and serialization
    "1:                   \n\t"
    "loop 1b                  " // Race interim
    :
    : "a" (address), "c" (ri)
    : "memory"
  );
  return race_cl[0]; // Return r
}

/* race_interim = 2 for 100%. */
__attribute__((aligned(64), noinline)) uint32_t rt_load_mc(void *address, int ri, int od)
{
  while (flag_cl[0]);
  // Out-of-Order Detachment (OD)
  asm volatile(
    "1:      \n\t"
    "loop 1b \n\t"
    "rdtscp      "
    :
    : "c" (od)
    : "rax", "rdx"
  );
  asm volatile(
    "LOCK btrl $0, 0(%%rcx) \n\t" // r <- false
    "LOCK btsl $0, 0(%%rdx) \n\t" // f <- true
    "mfence                 \n\t"
    "movq 0(%%rax), %%rax   \n\t" // Memory load
    "lfence                 \n\t" // Memory fence and serialization
    "rdtscp                 \n\t"
    "movl %%ebx, %%ecx      \n  "
    "1:                     \n\t"
    "loop 1b                    " // Race interim
    :
    : "a" (address), "b" (ri), "c" (&race_cl[0]), "d" (&flag_cl[0])
    : "memory"
  );
  return race_cl[0]; // Return r
}
#endif /* ?LFENCE_ALLOWS_SPECULATION */

#else /*----------------------------------------------------------------------- Intel's Skylake */

__attribute__((aligned(64))) static void * rt_server_l1d(void *info)
{
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(rt_server_config.core, &cpuset);
  pthread_setaffinity_np(rt_server_thread, sizeof(cpu_set_t), &cpuset);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
  rt_server_config.ready = 1;
  // Single-Core (SMT).
  if (rt_server_config.mode == RT_MODE_SC) {
    while (1) {
      if (race_cl[1]) {
        NOP_128;
        race_cl[0] = 1; // r <- true
        MFENCE;
        race_cl[1] = 0; // f <- false
      }
    }
  // Multi-Core.
  } else {
    while (1) {
      if (flag_cl[0]) {
        asm volatile(
          "LOCK btsl $0, 0(%%rax) \n\t" // r <- true
          "mfence                 \n\t"
          "LOCK btrl $0, 0(%%rbx) \n\t" // f <- false
          "mfence                 \n\t"
          "lfence                     "
          :: "a" (&race_cl[0]), "b" (&flag_cl[0])
          : "memory"
        );
      }
    }
  }
}

/* Race Interim = 12 for near 100% accuracy */
__attribute__((aligned(64), noinline)) uint32_t rt_load_sc(void *address, int ri, int od)
{
  // Out-of-Order Detachment (OD)
  asm volatile(
    "1:      \n\t"
    "loop 1b \n\t"
    "lfence      "
    :
    : "c" (od)
    :
  );
  // (Optional) replace OD with 'cpuid' here.
  //asm volatile("cpuid"::: "rax", "rbx", "rcx", "rdx");
  race_cl[0] = 0; // r <- false
  race_cl[1] = 1; // f <- true
  asm volatile(
    "mfence              \n\t"
    "movq (%%rax), %%rax \n\t" // Memory load
    "lfence              \n  " // Memory fence and serialization
    "1:                  \n\t"
    "loop 1b                 " // Race Interim
    :
    : "a" (address), "c" (ri)
    : "memory"
  );
  return race_cl[0]; // Return r
}

/* Race Interim = 14-15, od unused. */
__attribute__((aligned(64), noinline)) uint32_t rt_load_mc(void *address, int ri, int od)
{
  while (flag_cl[0]);
  asm volatile(
    "LOCK btrl $0, 0(%%rdx) \n\t" // r <- false
    "mfence                 \n\t"
    "LOCK btsl $0, 0(%%rbx) \n\t" // f <- true
    "mfence                 \n\t"
    "movq 0(%%rax), %%rax   \n\t" // Memory load
    "lfence                 \n  " // Memory fence and serialization
    "1:                     \n\t"
    "fnop                   \n\t"
    "fnop                   \n\t"
    "wait                   \n\t"
    "loop 1b                    " // Race interim
    :
    : "a" (address), "b" (&flag_cl[0]), "c" (ri), "d" (&race_cl[0])
    : "memory"
  );
  return race_cl[0]; // Return r
}

#endif /* Intel's Skylake or AMD's Zen. */

void rt_server_start(int core, int mode)
{
  if (rt_server_config.ready == 1)
    return;
  rt_server_config.core = core;
  rt_server_config.mode = mode;
  rt_server_config.ready = 0;
  pthread_create(&rt_server_thread, NULL, rt_server_l1d, NULL);
  while (rt_server_config.ready == 0);
}

void rt_server_stop(void)
{
  pthread_cancel(rt_server_thread);
  pthread_join(rt_server_thread, NULL);
  rt_server_config.ready = 0;
}

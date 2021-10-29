#include "cache.h"

#include "race-timing.h"

void load_address(register void *address)
{
  asm volatile(
    "movq 0(%%rax), %%rax \n\t"
    "lfence                   "
    :
    : "a" (address)
    : "memory"
  );
}

/*
 * load_address_set_tsc
 * --------------------
 * Access the provided memory address and time the memory access via the TSC.
 *
 * @return: Cycle latency from accessing 'address'.
 */
uint32_t load_address_tsc(register void *address)
{
  register uint32_t tsc_delta;
  asm volatile(
    "mfence               \n\t"
    "cpuid                \n\t"
    "rdtsc                \n\t"
    "movl %%eax, %%edi    \n\t"
    "movq 0(%%rsi), %%rsi \n\t" // Load
    "lfence               \n\t"
    "rdtscp               \n\t"
    "subl %%edi, %%eax        "
    : "=a" (tsc_delta)
    : "S" (address)
    : "%rbx", "%rcx", "%rdx", "memory"
  );
  return tsc_delta;
}

/*
 * load_address_set_pmc
 * --------------------
 * Access the provided memory address and time the memory access via PMCs.
 *
 * @return: PMC event increase from accessing 'address'.
 */
uint32_t load_address_pmc(register void *address)
{
  register uint32_t pmc_delta;
  asm volatile(
    "mfence               \n\t"
    "cpuid                \n\t"
    "movl $0, %%ecx       \n\t"
    "rdpmc                \n\t"
    "movl %%eax, %%edi    \n\t"
    "movq 0(%%rsi), %%rsi \n\t" // Load
    "lfence               \n\t"
    "rdpmc                \n\t"
    "subl %%edi, %%eax        "
    : "=a" (pmc_delta)
    : "S" (address)
    : "%rbx", "%rcx", "%rdx", "memory"
  );
  return pmc_delta;
}

/*
 * load_address_rt_sc_l1d
 * ----------------------
 * Access the provided memory address and identify if an L1D cache miss occurs.
 *
 * @return: L1D cache miss ? 1 : 0
 */
uint32_t load_address_rt_sc_l1d(void *address)
{
  return rt_load_sc(address, RI_SC_L1D_MISS, SCHEDULER_SIZE);
}

/*
 * load_address_rt_mc_l1d
 * ----------------------
 * Access the provided memory address and identify if an L1D cache miss occurs.
 *
 * @return: L1D cache miss ? 1 : 0
 */
uint32_t load_address_rt_mc_l1d(void *address)
{
  return rt_load_mc(address, RI_MC_L1D_MISS, SCHEDULER_SIZE);
}

void load_virtual_page_set(virtual_page_t *virtual_page, int set)
{
  load_address(VIRTUAL_PAGE_ADDRESS((*virtual_page), set));
}

uint32_t load_virtual_page_set_tsc(virtual_page_t *virtual_page, int set)
{
  return load_address_tsc(VIRTUAL_PAGE_ADDRESS((*virtual_page), set));
}

uint32_t load_virtual_page_set_pmc(virtual_page_t *virtual_page, int set)
{
  return load_address_pmc(VIRTUAL_PAGE_ADDRESS((*virtual_page), set));
}

uint32_t load_virtual_page_set_rt_sc_l1d(virtual_page_t *virtual_page, int set)
{
  return load_address_rt_sc_l1d(VIRTUAL_PAGE_ADDRESS((*virtual_page), set));
}

uint32_t load_virtual_page_set_rt_mc_l1d(virtual_page_t *virtual_page, int set)
{
  return load_address_rt_mc_l1d(VIRTUAL_PAGE_ADDRESS((*virtual_page), set));
}

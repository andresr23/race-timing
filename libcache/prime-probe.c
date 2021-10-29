#include "cache.h"

#include "race-timing.h"

void prime_l1d_set(register eset_l1d_t *eset_l1d, register int set)
{
  asm volatile(
    "movq 0(%%rax), %%rax \n\t" // 1
    "movq 0(%%rax), %%rax \n\t" // 2
    "movq 0(%%rax), %%rax \n\t" // 3
    "movq 0(%%rax), %%rax \n\t" // 4
    "movq 0(%%rax), %%rax \n\t" // 5
    "movq 0(%%rax), %%rax \n\t" // 6
    "movq 0(%%rax), %%rax \n\t" // 7
    "movq 0(%%rax), %%rax \n\t" // 8
    "movq 0(%%rax), %%rax \n\t" // 9
    "movq 0(%%rax), %%rax \n\t" // 10
    "movq 0(%%rax), %%rax \n\t" // 11
    "movq 0(%%rax), %%rax \n\t" // 12
    "movq 0(%%rax), %%rax \n\t" // 13
    "movq 0(%%rax), %%rax \n\t" // 14
    "movq 0(%%rax), %%rax \n\t" // 15
    "movq 0(%%rax), %%rax \n\t" // 16
    "lfence                   "
    :
    : "a" (ESET_L1D_PRIME_ADDRESS((*eset_l1d), set))
    : "memory"
  );
}

/*
 * probe_l1d_set_tsc
 * -----------------
 * Measure the latency, via the TSC, of 8 consecutive memory accesses
 * to probe an L1D cache set.
 *
 * @return: Cycle latency from accessing the specified cache 'set'.
 */
uint32_t probe_l1d_set_tsc(register eset_l1d_t *eset_l1d, register int set)
{
  register uint32_t tsc_delta;
  asm volatile(
    "mfence               \n\t"
    "cpuid                \n\t"
    "rdtsc                \n\t"
    "movl %%eax, %%edi    \n\t"
    "movq 0(%%rsi), %%rsi \n\t" // 1
    "movq 0(%%rsi), %%rsi \n\t" // 2
    "movq 0(%%rsi), %%rsi \n\t" // 3
    "movq 0(%%rsi), %%rsi \n\t" // 4
    "movq 0(%%rsi), %%rsi \n\t" // 5
    "movq 0(%%rsi), %%rsi \n\t" // 6
    "movq 0(%%rsi), %%rsi \n\t" // 7
    "movq 0(%%rsi), %%rsi \n\t" // 8
    "lfence               \n\t"
    "rdtscp               \n\t"
    "subl %%edi, %%eax        "
    : "=a" (tsc_delta)
    : "S" (ESET_L1D_PROBE_ADDRESS((*eset_l1d), set))
    : "%rbx", "%rcx", "%rdx", "memory"
  );
  return tsc_delta;
}

/*
 * probe_l1d_set_pmc
 * -----------------
 * Count events, via PMC 0, when performing 8 consecutive memory accesses
 * to probe an L1D cache set.
 *
 * @return: Events from accessing the specified cache 'set'.
 */
uint32_t probe_l1d_set_pmc(register eset_l1d_t *eset_l1d, register int set)
{
  register uint32_t pmc_delta;
  asm volatile(
    "mfence               \n\t"
    "cpuid                \n\t"
    "movl $0, %%ecx       \n\t"
    "rdpmc                \n\t"
    "movl %%eax, %%edi    \n\t"
    "movq 0(%%rsi), %%rsi \n\t" // 1
    "movq 0(%%rsi), %%rsi \n\t" // 2
    "movq 0(%%rsi), %%rsi \n\t" // 3
    "movq 0(%%rsi), %%rsi \n\t" // 4
    "movq 0(%%rsi), %%rsi \n\t" // 5
    "movq 0(%%rsi), %%rsi \n\t" // 6
    "movq 0(%%rsi), %%rsi \n\t" // 7
    "movq 0(%%rsi), %%rsi \n\t" // 8
    "lfence               \n\t"
    "rdpmc                \n\t" // %ecx still has a 0 value.
    "subl %%edi, %%eax        "
    : "=a" (pmc_delta)
    : "S" (ESET_L1D_PROBE_ADDRESS((*eset_l1d), set))
    : "%rbx", "%rcx", "%rdx", "memory"
  );
  return pmc_delta;
}

/*
 * probe_l1d_set_rt_sc (Race-Timing)
 * ---------------------------------
 * Identify L1D cache misses when accessing all the cache lines of an specified
 * L1D cache set.
 *
 * @return: L1D misses from accessing the specified cache 'set'.
 */
uint32_t probe_l1d_set_rt_sc(eset_l1d_t *eset_l1d, int set)
{
  register uint32_t miss_count = 0;
  for (register int way = ESET_L1D_IDX_E; way >= ESET_L1D_IDX_S; way--) {
    miss_count += rt_load_sc(ESET_L1D_BYWAY_ADDRESS((*eset_l1d),set, way),
                             RI_SC_L1D_MISS,
                             SCHEDULER_SIZE);
  }
  return miss_count;
}

/*
 * probe_l1d_set_rt_mc (Race-Timing)
 * ---------------------------------
 * Identify L1D cache misses when accessing all the cache lines of an specified
 * L1D cache set.
 *
 * @return: L1D misses from accessing the specified cache 'set'.
 */
uint32_t probe_l1d_set_rt_mc(eset_l1d_t *eset_l1d, int set)
{
  register uint32_t miss_count = 0;
  for (register int way = ESET_L1D_IDX_E; way >= ESET_L1D_IDX_S; way--) {
    miss_count += rt_load_mc(ESET_L1D_BYWAY_ADDRESS((*eset_l1d),set, way),
                             RI_SC_L1D_MISS,
                             SCHEDULER_SIZE);
  }
  return miss_count;
}

void prime_l1d_cache(eset_l1d_t *eset_l1d)
{
  for (int set = 0; set < CACHE_L1D_SETS; set++)
    prime_l1d_set(eset_l1d, set);
}

/*
 * probe_l1d_cache_tsc
 * -------------------
 * Probe each set of the cache individually by using the TSC.
 * Store the measurement in the 'delta' fields of the eviction set.
 */
void probe_l1d_cache_tsc(eset_l1d_t *eset_l1d)
{
  for (int set = 0; set < CACHE_L1D_SETS; set++)
    ESET_L1D_DELTA((*eset_l1d), set) = probe_l1d_set_tsc(eset_l1d, set);
}

/*
 * probe_l1d_cache_pmc
 * -------------------
 * Probe each set of the cache individually by using PMCs.
 * Store the measurement in the 'delta' fields of the eviction set.
 */
void probe_l1d_cache_pmc(eset_l1d_t *eset_l1d)
{
  for (int set = 0; set < CACHE_L1D_SETS; set++)
    ESET_L1D_DELTA((*eset_l1d), set) = probe_l1d_set_pmc(eset_l1d, set);
}

/*
 * probe_l1d_cache_rt_sc
 * ---------------------
 * Probe each set of the cache individually by using Race-Timing in single-core mode.
 * Store the measurement in the 'delta' fields of the eviction set.
 */
void probe_l1d_cache_rt_sc(eset_l1d_t *eset_l1d)
{
  for (int set = 0; set < CACHE_L1D_SETS; set++)
    ESET_L1D_DELTA((*eset_l1d), set) = probe_l1d_set_rt_sc(eset_l1d, set);
}

/*
 * probe_l1d_cache_rt_mc
 * ---------------------
 * Probe each set of the cache individually by using Race-Timing in multi-core mode.
 * Store the measurement in the 'delta' fields of the eviction set.
 */
void probe_l1d_cache_rt_mc(eset_l1d_t *eset_l1d)
{
  for (int set = 0; set < CACHE_L1D_SETS; set++)
    ESET_L1D_DELTA((*eset_l1d), set) = probe_l1d_set_rt_mc(eset_l1d, set);
}

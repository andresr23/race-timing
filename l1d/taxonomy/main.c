#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "libcache/cache.h"
#include "libcache/race-timing.h"

/* ----------------------------- Experiment ----------------------------------*/
#define EXPERIMENT "taxonomy"
/*----------------------------------------------------------------------------*/

/* Repetitions to calculate averages and rates. */
#define REPETITIONS 1000000

#if defined(ZEN)
  #define TEST_CORE 2
  #define TEST_MODE RT_MODE_MC

uint32_t (*probe_l1d_set_rt_function)(eset_l1d_t *, int) = &probe_l1d_set_rt_mc;
#else /* Skylake */
  #define TEST_CORE 4
  #define TEST_MODE RT_MODE_SC

uint32_t (*probe_l1d_set_rt_function)(eset_l1d_t *, int) = &probe_l1d_set_rt_sc;
#endif /* Micro-architecture. */

void evict_l1d_set_test(eset_l1d_t *eset_l1d, int set, int ways)
{
  if (ways == 0)
    return;
  asm volatile(
    "1:                   \n\t"
    "movq 0(%%rax), %%rax \n\t"
    "loop 1b              \n\t"
    "lfence                   "
    :
    : "a" (ESET_L1D_PRIME_ADDRESS((*eset_l1d), set)), "c" (ways)
    : "memory"
  );
}

int main()
{
  int test_set;

  uint32_t m_avg_pmc[CACHE_L1D_WAYS + 1] = {0};
  uint32_t m_avg_tsc[CACHE_L1D_WAYS + 1] = {0};
  uint32_t m_avg_rt[CACHE_L1D_WAYS + 1] = {0};

  eset_l1d_t eset_l1d_evict;
  eset_l1d_t eset_l1d_prime_probe;

  build_eset_l1d(&eset_l1d_evict);
  build_eset_l1d(&eset_l1d_prime_probe);

  srand(time(0));

  // PMC.
  for (int r = 0; r < REPETITIONS; r++) {
    test_set = rand() % CACHE_L1D_SETS;

    for (int ways = 0; ways <= CACHE_L1D_WAYS; ways++) {
      // Prime - Evict 'w' ways - Probe.
      prime_l1d_set(&eset_l1d_prime_probe, test_set);
      evict_l1d_set_test(&eset_l1d_evict, test_set, ways);
      m_avg_pmc[ways] += probe_l1d_set_pmc(&eset_l1d_prime_probe, test_set);
    }
  }

  // TSC.
  for (int r = 0; r < REPETITIONS; r++) {
    test_set = rand() % CACHE_L1D_SETS;

    for (int ways = 0; ways <= CACHE_L1D_WAYS; ways++) {
      // Prime - Evict 'w' ways - Probe.
      prime_l1d_set(&eset_l1d_prime_probe, test_set);
      evict_l1d_set_test(&eset_l1d_evict, test_set, ways);
      m_avg_tsc[ways] += probe_l1d_set_tsc(&eset_l1d_prime_probe, test_set);
    }
  }

  // Race-Timing.
  rt_server_start(TEST_CORE, TEST_MODE);
  for (int r = 0; r < REPETITIONS; r++) {
    test_set = rand() % CACHE_L1D_SETS;

    for (int ways = 0; ways <= CACHE_L1D_WAYS; ways++) {
      // Prime - Evict 'w' ways - Probe.
      prime_l1d_set(&eset_l1d_prime_probe, test_set);
      evict_l1d_set_test(&eset_l1d_evict, test_set, ways);
      m_avg_rt[ways]  += probe_l1d_set_rt_function(&eset_l1d_prime_probe, test_set);
    }
  }
  rt_server_stop();

  allocate_free_all();

  // Print.
  printf("n | PMC   | TSC     | R-T\n");
  printf("---------------------------\n");
  for (int ways = 0; ways <= CACHE_L1D_WAYS; ways++) {
    printf("%01d | ",  ways);
    printf("%5.3f | ", (float) m_avg_pmc[ways] / REPETITIONS);
    printf("%7.3f | ", (float) m_avg_tsc[ways] / REPETITIONS);
    printf("%5.3f\n",  (float) m_avg_rt[ways]  / REPETITIONS);
  }
  printf("---------------------------\n");
  printf("(%d Repetitions)\n", REPETITIONS);

}

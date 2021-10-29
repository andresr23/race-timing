#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "libcache/cache.h"
#include "libcache/race-timing.h"

/* ----------------------------- Experiment ----------------------------------*/
#define EXPERIMENT "metrics"
/*----------------------------------------------------------------------------*/

/* Repetitions to calculate averages and rates. */
#define REPETITIONS 1000000

#if defined(ZEN)
  #define TEST_CORE 2
  #define TEST_MODE RT_MODE_MC

uint32_t (*load_address_rt_function)(void *) = &load_address_rt_mc_l1d;
#else /* Skylake */
  #define TEST_CORE 4
  #define TEST_MODE RT_MODE_SC

uint32_t (*load_address_rt_function)(void *) = &load_address_rt_sc_l1d;
#endif /* Micro-architecture. */

int main()
{
  int test_set;
  uint32_t h_tmp;
  uint32_t m_tmp;

  uint32_t h_avg_pmc;
  uint32_t m_avg_pmc;
  uint32_t error_pmc;
  uint32_t equal_pmc;

  uint32_t h_avg_tsc;
  uint32_t m_avg_tsc;
  uint32_t error_tsc;
  uint32_t equal_tsc;

  uint32_t h_avg_rt;
  uint32_t m_avg_rt;
  uint32_t error_rt;
  uint32_t equal_rt;

  void *data_address;
  eset_l1d_t eset_l1d;
  virtual_page_t virtual_page;

  build_eset_l1d(&eset_l1d);
  build_virtual_page(&virtual_page);

  srand(time(0));

  // PMC.
  h_avg_pmc = 0;
  m_avg_pmc = 0;
  error_pmc = 0;
  equal_pmc = 0;
  for (int r = 0; r < REPETITIONS; r++) {
    test_set = rand() % CACHE_L1D_SETS;
    data_address = VIRTUAL_PAGE_ADDRESS(virtual_page, test_set);

    // L1D Hit (Load - Reload).
    load_address(data_address);
    h_avg_pmc += h_tmp = load_address_pmc(data_address);

    // L1D Miss (Load - Evict - Reload).
    load_address(data_address);
    evict_l1d_set(&eset_l1d, test_set);
    m_avg_pmc += m_tmp = load_address_pmc(data_address);

    // Examine.
    if (h_tmp  > m_tmp)
      error_pmc++;
    if (h_tmp == m_tmp)
      equal_pmc++;
  }

  // TSC.
  h_avg_tsc = 0;
  m_avg_tsc = 0;
  error_tsc = 0;
  equal_tsc = 0;
  for (int r = 0; r < REPETITIONS; r++) {
    test_set = rand() % CACHE_L1D_SETS;
    data_address = VIRTUAL_PAGE_ADDRESS(virtual_page, test_set);

    // L1D Hit (Load - Reload).
    load_address(data_address);
    h_avg_tsc += h_tmp = load_address_tsc(data_address);

    // L1D Miss (Load - Evict - Reload).
    load_address(data_address);
    evict_l1d_set(&eset_l1d, test_set);
    m_avg_tsc += m_tmp = load_address_tsc(data_address);

    // Examine.
    if (h_tmp  > m_tmp)
      error_tsc++;
    if (h_tmp == m_tmp)
      equal_tsc++;
  }

  // Race-Timing.
  h_avg_rt = 0;
  m_avg_rt = 0;
  error_rt = 0;
  equal_rt = 0;
  rt_server_start(TEST_CORE, TEST_MODE);
  for (int r = 0; r < REPETITIONS; r++) {
    test_set = rand() % CACHE_L1D_SETS;
    data_address = VIRTUAL_PAGE_ADDRESS(virtual_page, test_set);

    // L1D Hit (Load - Reload).
    load_address(data_address);
    h_avg_rt  += h_tmp = load_address_rt_function(data_address);

    // L1D Miss (Load - Evict - Reload).
    load_address(data_address);
    evict_l1d_set(&eset_l1d, test_set);
    m_avg_rt  += m_tmp = load_address_rt_function(data_address);

    // Examine.
    if (h_tmp  > m_tmp)
      error_rt++;
    if (h_tmp == m_tmp)
      equal_rt++;
  }
  rt_server_stop();

  allocate_free_all();

  // Print
  printf("Measurement        | PMC      | TSC       | R-T\n");
  printf("----------------------------------------------------\n");
  printf("Avg. Hit           | %1.6f | %2.6f | %1.6f\n", (float) h_avg_pmc / REPETITIONS, (float) h_avg_tsc / REPETITIONS, (float) h_avg_rt / REPETITIONS);
  printf("Avg. Miss          | %1.6f | %2.6f | %1.6f\n", (float) m_avg_pmc / REPETITIONS, (float) m_avg_tsc / REPETITIONS, (float) m_avg_rt / REPETITIONS);
  printf("----------------------------------------------------\n");
  printf("Error Rate (H > M) | %1.6f | %1.6f  | %1.6f\n", (float) error_pmc / REPETITIONS, (float) error_tsc / REPETITIONS, (float) error_rt / REPETITIONS);
  printf("Equal Rate (H = M) | %1.6f | %1.6f  | %1.6f\n", (float) equal_pmc / REPETITIONS, (float) equal_tsc / REPETITIONS, (float) equal_rt / REPETITIONS);
  printf("----------------------------------------------------\n");
  printf("(%d Repetitions)\n", REPETITIONS);
}

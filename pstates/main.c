#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "libcache/cache.h"

/* ----------------------------- Exeriment -----------------------------------*/
#define EXPERIMENT "pstates"
/* ---------------------------------------------------------------------------*/

/* Fixed L1D cache set for testing. */
#define TEST_CACHE_SET 32

/* Number of measurements. */
#define REPETITIONS 100000

unsigned int results[REPETITIONS] = {0};

int main()
{
  void *data_address;
  uint32_t *delta_tsc;
  virtual_page_t virtual_page;
  build_virtual_page(&virtual_page);

  delta_tsc = (uint32_t *) malloc(sizeof(uint32_t) * REPETITIONS);
  memset(delta_tsc, 0, REPETITIONS * sizeof(uint32_t));

  data_address = VIRTUAL_PAGE_ADDRESS(virtual_page, TEST_CACHE_SET);

  // Experiment.
  for (int r = 0; r < REPETITIONS; r++){
    // Load, then Probe.
    load_address(data_address);
    delta_tsc[r] = load_address_tsc(data_address);
  }

  // Print.
  for (int r = 0; r < REPETITIONS; r++)
    printf("%d\n", delta_tsc[r]);

  free(delta_tsc);

  allocate_free_all();
}

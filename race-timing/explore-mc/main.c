#define _GNU_SOURCE

#include <time.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include "libcache/cache.h"
#include "libcache/race-timing.h"

/* ----------------------------- Exeriment -----------------------------------*/
#define EXPERIMENT "explore-mc"
/* ---------------------------- Output files ---------------------------------*/
#define OUTPUT_FILE_0 EXPERIMENT "-acc.dat"
#define OUTPUT_FILE_1 EXPERIMENT "-pos.dat"
/* ---------------------------------------------------------------------------*/

/* Race-Timing server runs on a different execution core. */
#define TEST_SERVER_CORE 2

/* Fixed L1D cache set for testing. */
#define TEST_CACHE_SET 32

/* Repetitions per combination to calculate accuracy rate. */
#define REPETITIONS 1000

/* Range for the explore parameters. */
#define OD_SPACE 400
#define RI_SPACE 30

#define REPORT_SIZE 20

__attribute__((aligned(64))) int main_global_align[16] = {0};

/* Heatmaps. */
float    accuracy_matrix[RI_SPACE][OD_SPACE] = {0};
uint32_t positive_matrix[RI_SPACE][OD_SPACE] = {0};

/* Report data. */
float best_accuracy[REPORT_SIZE] = {0};
uint32_t best_ri[REPORT_SIZE] = {0};
uint32_t best_od[REPORT_SIZE] = {0};

void print_matrix()
{
  FILE *f0 = fopen(OUTPUT_FILE_0, "w");
  FILE *f1 = fopen(OUTPUT_FILE_1, "w");
  for (int r = 0; r < RI_SPACE; r++) {
    for (int n = 0; n < OD_SPACE; n++) {
      fprintf(f0, "%f ", accuracy_matrix[r][n]);
      fprintf(f1, "%u ", positive_matrix[r][n]);
    }
    fprintf(f0, "\n");
    fprintf(f1, "\n");
  }
  fclose(f0);
  fclose(f1);
}

void print_report()
{
  printf("Accuracy | Race Interim | od\n");
  printf("--------------------------------------\n");
  for (int i = 0; i < REPORT_SIZE; i++)
    printf("%1.3f    | %2u           | %3u\n", best_accuracy[i], best_ri[i], best_od[i]);
}

void update_best_accuracy(float acc, int ri, int od)
{
  for (int i = 0; i < REPORT_SIZE; i++){
    if (acc > best_accuracy[i]) {
      for (int j = (REPORT_SIZE - 1); j > i; j--) {
        best_accuracy[j] = best_accuracy[j - 1];
        best_ri[j] = best_ri[j - 1];
        best_od[j] = best_od[j -1];
      }
      best_accuracy[i] = acc;
      best_ri[i] = ri;
      best_od[i] = od;
      return;
    }
  }
}

int main()
{
  __attribute__((aligned(4096))) int align[16] = {0};
  int evict;
  int noise_array[16];

  uint32_t score;
  uint32_t result;
  uint32_t temp_positive;
  float temp_accuracy;

  void *data_address;
  eset_l1d_t eset_l1d;
  virtual_page_t virtual_page;

  // Build data structures.
  build_eset_l1d(&eset_l1d);
  build_virtual_page(&virtual_page);

  // Start the Race-Timing server
  rt_server_start(TEST_SERVER_CORE, RT_MODE_MC);

  data_address = VIRTUAL_PAGE_ADDRESS(virtual_page, TEST_CACHE_SET);

  srand(time(0));
  for (int ri = 1; ri <= RI_SPACE; ri++) {

    for (int od = 1; od <= OD_SPACE; od++) {
      score = 0;
      temp_positive = 0;

      for(int r = 0; r < REPETITIONS; r++){
        // Randomly choose if evict or not.
        evict = rand() & 0x1;

        // Load reference data.
        load_address(data_address);

        // Evict the reference data if 'evict' is not 0.
        if (evict)
          evict_l1d_set(&eset_l1d, TEST_CACHE_SET);

        // Add core noise.
        for (int i = 0; i < (rand() % 16); i++)
          noise_array[i] = rand() % 10000;

        // Race-Timing.
        result = rt_load_mc(data_address, ri, od);

        // (Optional) Place additional memory fencing here.
        //asm volatile("mfence");
        //asm volatile("lfence");
        //asm volatile("sfence");

        // Verify.
        if (result == evict)
          score++;
        temp_positive += result;
      }
      temp_accuracy = ((float) score) / ((float) REPETITIONS);
      positive_matrix[ri - 1][od - 1] = temp_positive;
      accuracy_matrix[ri - 1][od - 1] = temp_accuracy;
      update_best_accuracy(temp_accuracy, ri, od);
    }
  }
  rt_server_stop();

  print_matrix();
  print_report();

  allocate_free_all();
}

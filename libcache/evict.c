#include "cache.h"

void evict_l1d_set(register eset_l1d_t *eset_l1d, register int set)
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

void evict_l1d_cache(eset_l1d_t *eset_l1d)
{
  for (int set = 0; set < CACHE_L1D_SETS; set++)
    evict_l1d_set(eset_l1d, set);
}

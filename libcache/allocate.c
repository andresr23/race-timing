#define _GNU_SOURCE

#include "cache.h"

#include <stdio.h>
#include <stdlib.h>

/* Define an upper bound on how much memory can be allocated (in pages). */
#define ALLOCATE_MAX (1024 * 64)

// Track all the memory that has been allocated.
static struct {
  cache_line_t *pages[ALLOCATE_MAX];
  int count;
} allocate_registry = {0};

/*
 * allocate_new_page
 * -----------------
 * Allocate a new virtual page by allocating memory for 64 'alloc_cl_t'
 * structures, which together comprise 4-kB of data.
 *
 * posix_memalign(void **memptr,    -> Address of the pointer to be configured.
 *                size_t alignment,
                  size_t size);
 *
 * @return: A pointer to an array of 64 'eset_cache_line_t' structures (4-kB).
 *          NULL if something failed.
 */
cache_line_t * allocate_new_page(void)
{
  cache_line_t *page = NULL;

  // Check how much memory has been allocated.
  if (allocate_registry.count >= (ALLOCATE_MAX)) {
    printf("[%s] Reached memory allocation limit.\n", __FILE__);
    allocate_free_all();
    abort();
  }

  // Check 'posix_memalign'.
  if (posix_memalign((void *) &page, VIRTUAL_PAGE_ALIGNMENT, VIRTUAL_PAGE_SIZE)) {
    printf("[%s] 'posix_memalign' failed.\n", __FILE__);
    allocate_free_all();
    abort();
  }

  // Deal with Linux's lazy paging.
  page[0].delta = 0xFF;

  // Bookeeping.
  allocate_registry.pages[allocate_registry.count++] = page;
  return page;
}

void allocate_free_all(void)
{
  int idx = allocate_registry.count - 1;

  while (idx >= 0) {
    if (allocate_registry.pages[idx] != NULL) {
      free(allocate_registry.pages[idx]);
      allocate_registry.pages[idx] = NULL;
    }
    idx--;
  }

  allocate_registry.count = 0;
}

void allocate_report(void)
{
  printf("[%s] Pages currently allocated: %04d.\n", __FILE__, allocate_registry.count);
}

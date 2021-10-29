#include "cache.h"

/*
 * build_virtual_page
 * ------------------
 * Allocate and configure a single virtual page.
 */
void build_virtual_page(virtual_page_t *virtual_page)
{
  // Allocate.
  virtual_page->lines = allocate_new_page();

  // Configure pointers.
  for (int set = 0; set < VIRTUAL_PAGE_SETS; set++) {
    (virtual_page->lines[set]).next = (void *) &((virtual_page->lines[set]).next);
    (virtual_page->lines[set]).prev = (void *) &((virtual_page->lines[set]).prev);
  }
}

/*
 * build_eset_l1d
 * --------------
 * L1D eviction sets can be easily created in x86-64, just allocate 8 pages.
 */
void build_eset_l1d(eset_l1d_t *eset_l1d)
{
  // Allocate.
  for (int way = 0; way < CACHE_L1D_WAYS; way++)
    eset_l1d->lines[way] = allocate_new_page();

  // Configure pointers.
  for (int set = 0; set < CACHE_L1D_SETS; set++) {

    // Next.
    for (int way = 0; way < ESET_L1D_IDX_E; way++)
      (eset_l1d->lines[way][set]).next = (void *) &((eset_l1d->lines[way + 1][set]).next);
    (eset_l1d->lines[ESET_L1D_IDX_E][set]).next = (void *) &((eset_l1d->lines[ESET_L1D_IDX_S][set]).next);

    // Prev.
    for (int way = ESET_L1D_IDX_E; way > ESET_L1D_IDX_S; way--)
      (eset_l1d->lines[way][set]).prev = (void *) &((eset_l1d->lines[way - 1][set]).prev);
    (eset_l1d->lines[ESET_L1D_IDX_S][set]).prev = (void *) &((eset_l1d->lines[ESET_L1D_IDX_E][set]).prev);

  }
}

#ifndef _CACHE_H_
#define _CACHE_H_

#include <stdint.h>

/* Virtual Pages (4-kB). */
#define VIRTUAL_PAGE_SIZE      4096
#define VIRTUAL_PAGE_ALIGNMENT 4096
#define VIRTUAL_PAGE_SETS      64

/* L1D Parameters for most x86-64 processors. */
#define CACHE_L1D_SETS 64
#define CACHE_L1D_WAYS  8

/*
 * Cache line representation as a 64-byte structure.
 * -------------------------------------------------
 * next : Pointer to the next congruent 'eset_cache_line_t' struct.
 * prev : Pointer to the previous congruent 'eset_cache_line_t' struct.
 * delta: Unsigned integer to save timing measurements.
 * fill : Memory to completely fill a cache line.
 */
typedef struct {
	void *next;
	void *prev;
	uint64_t delta;
  uint64_t fill[5];
} __attribute__((packed)) cache_line_t;
// Size.
#define CACHE_LINE_SIZE 64

//------------------------------------------------------------------------------

/* Virtual page. */
typedef struct {
	cache_line_t *lines;
} virtual_page_t;
// Compute an address of data in cache set '_s'.
#define VIRTUAL_PAGE_ADDRESS(_p, _s) ((void *) &((_p.lines[_s]).next))

/* L1D Eviction Set. */
typedef struct {
  cache_line_t *lines[CACHE_L1D_WAYS];
} eset_l1d_t;
// L1D indexes.
#define ESET_L1D_IDX_S 0
#define ESET_L1D_IDX_E (CACHE_L1D_WAYS - 1)
// Compute addresses of data in cache set '_s', to Prime+Probe.
#define ESET_L1D_PRIME_ADDRESS(_e, _s) ((void *) &((_e.lines[ESET_L1D_IDX_S][_s]).next))
#define ESET_L1D_PROBE_ADDRESS(_e, _s) ((void *) &((_e.lines[ESET_L1D_IDX_E][_s]).prev))
// Compute the address of a cache line in a specific way '_w'.
#define ESET_L1D_BYWAY_ADDRESS(_e, _s, _w) ((void *) &((_e.lines[_w][_s]).next))
// Auxiliary macro to store data in the delta fields of an 'eset_l1d_t'.
#define ESET_L1D_DELTA(_e, _s) ((_e.lines[ESET_L1D_IDX_S][_s]).delta)

#define ESET_L1D_EVICT_ADDRESS ESET_L1D_PRIME_ADDRESS

#define ESET_MEMFENCE asm volatile("lfence")

/*-------------------------------- API ---------------------------------------*/

/* Allocate virtual pages. */
cache_line_t * allocate_new_page(void);
void allocate_free_all(void);
void allocate_report(void);

/* Build eviction sets. */
void build_virtual_page(virtual_page_t *virtual_page);
void build_eset_l1d(eset_l1d_t *eset_l1d);

/* Load. */
// Address.
void load_address(void *address);
uint32_t load_address_tsc(void *address);
uint32_t load_address_pmc(void *address);
uint32_t load_address_rt_sc_l1d(void *address);
uint32_t load_address_rt_mc_l1d(void *address);
// Virtual Page set.
void load_virtual_page_set(virtual_page_t *virtual_page, int set);
uint32_t load_virtual_page_set_tsc(virtual_page_t *virtual_page, int set);
uint32_t load_virtual_page_set_pmc(virtual_page_t *virtual_page, int set);
uint32_t load_virtual_page_set_rt_sc_l1d(virtual_page_t *virtual_page, int set);
uint32_t load_virtual_page_set_rt_mc_l1d(virtual_page_t *virtual_page, int set);

/* Evict. */
void evict_l1d_set(eset_l1d_t *eset_l1d, int set);
void evict_l1d_cache(eset_l1d_t *eset_l1d);

/* L1D Prime+Probe. */
// Set.
void prime_l1d_set(eset_l1d_t *eset_l1d, int set);
uint32_t probe_l1d_set_tsc(eset_l1d_t *eset_l1d, int set);
uint32_t probe_l1d_set_pmc(eset_l1d_t *eset_l1d, int set);
uint32_t probe_l1d_set_rt_sc(eset_l1d_t *eset_l1d, int set); // Race-Timing (SC)
uint32_t probe_l1d_set_rt_mc(eset_l1d_t *eset_l1d, int set); // Race-Timing (MC)
// Cache.
void prime_l1d_cache(eset_l1d_t *eset_l1d);
void probe_l1d_cache_tsc(eset_l1d_t *eset_l1d);
void probe_l1d_cache_pmc(eset_l1d_t *eset_l1d);
void probe_l1d_cache_rt_sc(eset_l1d_t *eset_l1d); // Race-Timing (SC)
void probe_l1d_cache_rt_mc(eset_l1d_t *eset_l1d); // Race-Timing (MC)

/*----------------------------------------------------------------------------*/

#endif /* _CACHE_H_ */

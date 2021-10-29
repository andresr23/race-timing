#ifndef _PMC_MODULE_H_
#define _PMC_MODULE_H_

/* MSR functions and macros. */
#include <asm/msr.h>

/* Default module headers. */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

/* Integer types. */
#include <asm-generic/int-ll64.h>

MODULE_LICENSE("GPL");

#if defined(AMD)
/*
 * Performance Event Select.
 * -------------------------
 * Architectural Performance Monitoring facilities provide a number of
 * MSRs to invididually configure PMCs.
 *
 * References:
 * Open Source Register Reference for AMD Family 17h Processors Models 00h-2Fh
 * section 2.1.10.3 MSRs, page 136.
 */
#define PERF_CTL_0 0xC0010200U
#define PERF_CTL_1 0xC0010202U
#define PERF_CTL_2 0xC0010204U
#define PERF_CTL_3 0xC0010206U

 /*
  * Performance Monitoring Counters are described in:
  *
  * Processor Programming Reference (PPR) for AMD Family 17h Models 00h-0Fh
  * Processors, Section 2.1.15.
  */

/*
 * Requests to L2 Group1.
 * ----------------------
 * 80H : Data Cache Reads (including prefetch)
 * 40H : Data Cache Stores
 * 20H : Data Cache Shared Reads
 * 10H : Instruction Cache Reads
 * 08H : Data Cache State Change Requests
 * 04H : Software Prefetch
 * 01H : L2 Prefetcher
 * 00H : Group2
 */
#define L2_REQUEST_G1 0x060UL
// Umasks
#define RD_BLK_L     0x80UL
#define RD_BLK_X     0x40UL
#define LS_RD_BLK_CS 0x20UL
#define I_CACHE_RD   0x10UL
#define CHANGE_TO_X  0x08UL
#define PREFETCH_L2  0x04UL
#define L2_HW_PF     0x02UL
#define L2_G2        0x01UL

/*
 * LS: Cycles not in halt.
 * -----------------------
 * 00H : Reserved
 */
#define CYCLES_NOT_IN_HALT 0x076UL
// Umasks
#define RESERVED 0x00UL

#else /* Intel */
/*
 * Performance Event Select Register MSRs.
 * ---------------------------------------
 * Architectural Performance Monitoring facilities provide a number of
 * MSRs to invididually configure PMCs.
 *
 * References:
 * Intel® 64 and IA-32 architectures software developer's manual combined
 * volumes 3A, 3B, 3C, and 3D: System programming guide,
 * Section 18.2.1.1 Architectural Performance Monitoring Facilities, Page 18-3.
 *
 * Intel® 64 and IA-32 architectures software developer's manual volume 4:
 * Model-specific registers.
 */
#define PERFEVTSEL0 0x186U
#define PERFEVTSEL1 0x187U
#define PERFEVTSEL2 0x188U
#define PERFEVTSEL3 0x189U

/*
 * Pre-defined Architectural Performance Events are described in:
 *
 * Intel® 64 and IA-32 architectures software developer's manual combined
 * volumes 3A, 3B, 3C, and 3D: System programming guide, Table 18-1.
 */

/*
 * Architectural Event: Memory Load Retired.
 * -----------------------------------------
 * 01H : Retired load instructions with L1 cache hits as data sources.
 * 02H : Retired load instructions with L2 cache hits as data sources.
 * 04H : Retired load instructions with L3 cache hits as data sources.
 * 08H : Retired load instructions missed L1 cache as data sources.
 * 10H : Retired load instructions missed L2 cache as data sources.
 * 20H : Retired load instructions missed L3 cache as data sources.
 * 40H : L1 Miss but FB hit.
 */
#define MEM_LOAD_RETIRED 0xD1UL
// Umasks
#define L1_HIT  0x01UL
#define L2_HIT  0x02UL
#define L3_HIT  0x04UL
#define L1_MISS 0x08UL
#define L2_MISS 0x10UL
#define L3_MISS 0x20UL
#define FB_HIT  0x40UL

#endif /* AMD or Intel */

#endif /* _PMC_MODULE_H_ */

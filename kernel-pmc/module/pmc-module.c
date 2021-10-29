#include "pmc-module.h"

/*
 * enable_rdpmc
 * ------------
 * To enable the 'rdpmc' instruction, the module needs to set the
 * 9th bit of the CR4 register.
 */
static void enable_rdpmc(void)
{
  register u64 cr4;
  asm volatile("mov %%cr4, %0" : "=r" (cr4)::);
  cr4 |= 0x100UL;
  asm volatile("mov %0, %%cr4" :: "r" (cr4));
}

/*
 * disable_rdpmc
 * -------------
 * Reset the 9th bit of the cr4 register to disable the 'rdpmc' instruction.
 *
 * Reference: Architecture Programmer's Manual, Volume 2: System Programming,
 *            section 3.1.3
 */
static void disable_rdpmc(void)
{
  register u64 cr4;
  asm volatile("mov %%cr4, %0" : "=r" (cr4)::);
  cr4 &= ~0x100UL;
  asm volatile("mov %0, %%cr4" :: "r" (cr4));
}

#if defined(AMD)

/*
 * config_pmc
 * ----------
 * Configure a PMC counter by writing into the required MSR.
 *
 * Configuration layout for AMD's MSRs in accordance to the
 * Performance Event Select [5:0] table in:
 * Processor Programming Reference (PPR) for AMD Family 17h Models
 * 00h-0Fh Processors, Page 146
 *
 * [63:42] - Reserved
 * [41:40] - Host/Guest, 0h for all events
 * [39:36] - Reserved
 * [35:32] - Event Select[11:08]
 * [31:24] - Count Mask, 00h for all events in a cc
 * [23]		 - Invert Counter mask
 * [22]		 - Enable Performance Counter
 * [21]		 - Reserved
 * [20]		 - APIC interrupt when overflow
 * [19]		 - Reserved
 * [18]		 - Edge detect
 * [17:16] - OS/Usermode
 * [16:08] - UnitMask
 * [07:00] - Event Select
 */
static void config_pmc(u32 pmc, u64 event, u64 mask)
{
	u64 _v0 = (event & 0xf00UL) << 32; /* Upper bits of event select */
	u64 _v1 = (event & 0x0ffUL);       /* Lower bits of event select */
	u64 _m0 = (mask  & 0xffUL ) <<  8; /* Mask                       */
	u64 _ou = 0x3 << 16;               /* Count OS/User events - 11b */
	u64 _e0 = 0x1 << 22;               /* Enable                     */
	_e0 |= (_v0 | _v1 | _m0 | _ou);
	wrmsrl(pmc, _e0);
}

#else /* Intel */

/*
 * config_pmc
 * ----------
 * Configure a PMC counter by writing into the required MSR.
 *
 * Configuration layout for Intel's IA32 Architectural MSRs in accordance
 * to Table 2-2 in:
 * Intel 64 and IA-32 architectures software developer's
 * manual volume 4: Model-specific registers, Section 2.1 ARCHITECTURAL MSRS,
 * Page 2-3
 *
 * [32:24] - CounterMask = 0x00 -> Detect multiple events on each clock cycle
 * [23]    - CounterMask Invert
 * [22]    - Enable
 * [21]    - Reserved
 * [20]    - APIC interrupt Enable = 0
 * [19]    - Pin Control = 0
 * [18]    - Edge detection = 0
 * [17]    - OS Mode (Ring 0) = 1
 * [16]    - User Mode (1,2,3)= 1
 * [15:08] - Event Mask (UMask)
 * [07:00] - Event Select
 */
static void config_pmc(u32 pmc, u64 event, u64 mask)
{
  u64 _v0 = (event & 0xffUL);       /* Event                      */
  u64 _m0 = (mask  & 0xffUL) << 8;  /* UMask                      */
  u64 _ou = 0x3 << 16;              /* Count OS/User events - 11b */
  u64 _e0 = 0x1 << 22;              /* Enable                     */
  _e0 |= (_v0 | _m0 | _ou);
  wrmsrl(pmc, _e0);
}

#endif /* config_pmc */

/*
 * reset_pmc
 * ---------
 * Discard a PMC configuration.
 */
static void reset_pmc(u32 pmc)
{
  wrmsrl(pmc, 0x0UL);
}

/*----------------------- AMD configuration goes here ------------------------*/
#if defined(AMD)

/*
 * init_pmc_func
 * -------------
 * Configure PMCs as required. The 'pmc_init' function calls this function on
 * each logical core.
 */
static void init_pmc_func(void *info)
{
  // L2 Block reads
  config_pmc(PERF_CTL_0, L2_REQUEST_G1, RD_BLK_L);
  // LS: Cycles not in halt
  //config_pmc(PERF_CTL_0, CYCLES_NOT_IN_HALT, RESERVED);
  // Enable the 'rdpmc' instruction
  enable_rdpmc();
}

/*
 * exit_pmc_func
 * -------------
 * Revert all the configuration made by 'init_pmc_func'. The 'pmc_exit'
 * function calls this function on each logical core.
 */
static void exit_pmc_func(void *info)
{
  reset_pmc(PERF_CTL_0);
  disable_rdpmc();
}

/*----------------------------------------------------------------------------*/

/*--------------------- Intel configuration goes here ------------------------*/
#else /* Intel */

/*
 * init_pmc_func
 * -------------
 * Configure PMCs as required. The 'pmc_init' function calls this function on
 * each logical core.
 */
static void init_pmc_func(void *info)
{
  // L1D misses
  config_pmc(PERFEVTSEL0, MEM_LOAD_RETIRED, L1_MISS);
  // Enable the 'rdpmc' instruction
  enable_rdpmc();
}

/*
 * exit_pmc_func
 * -------------
 * Revert all the configuration made by 'init_pmc_func'. The 'pmc_exit'
 * function calls this function on each logical core.
 */
static void exit_pmc_func(void *info)
{
  reset_pmc(PERFEVTSEL0);
  disable_rdpmc();
}

#endif /* Configuration for AMD or Intel */
/*----------------------------------------------------------------------------*/

/*
 * pmc_init
 * --------
 * Module's entry point. On installation the module writes to the corresponding
 * MSRs to configure the PMCs required by the user. The function additionally
 * enables the 'rdpmc' instruction to the userspace.
 */
static int __init pmc_init(void)
{
  on_each_cpu(init_pmc_func, NULL, 1);
  return 0;
}

/*
 * pmc_exit
 * --------
 * Module's exit point. On removal the module discards all the changes made
 * by the 'pmc_init' function.
 */
static void __exit pmc_exit(void)
{
  on_each_cpu(exit_pmc_func, NULL, 1);
}

module_init(pmc_init);
module_exit(pmc_exit);

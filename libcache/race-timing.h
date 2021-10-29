#ifndef _RACE_TIMING_H_
#define _RACE_TIMING_H_

#include <stdint.h>

#define RT_MODE_SC 0
#define RT_MODE_MC 1

#if defined(ZEN)
  #define SCHEDULER_SIZE 70
  // Events.
  #define RI_SC_L1D_MISS 12
  #define RI_MC_L1D_MISS 2
#else /* Intel Skylake i7-6700. */
  #define SCHEDULER_SIZE 97
  // Events.
  #define RI_SC_L1D_MISS 12
  #define RI_MC_L1D_MISS 14
#endif /* Micro-architecture */

/*-------------------------------- API ---------------------------------------*/

uint32_t rt_load_sc(void *address, int ri, int od);
uint32_t rt_load_mc(void *address, int ri, int od);

void rt_server_start(int core, int config);
void rt_server_stop(void);

/*----------------------------------------------------------------------------*/

#endif /* _RACE_TIMING_H_ */

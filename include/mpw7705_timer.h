/*
 *  Created on: Oct 26, 2015
 *      Author: a.korablinov
 */

#ifndef MPW7705_TIMER_H_
#define MPW7705_TIMER_H_

#include <asm/ppc470s_reg_access.h>

#define platform_tcurrent       read_SPR_TBL_R

#define PPC_FREQ__CPM_C470SCLOCK    (800*1000*1000)
#define PPC_TIMER_FREQ              PPC_FREQ__CPM_C470SCLOCK
#define PLATFORM_TIMER_TICKS_PER_US    (PPC_TIMER_FREQ/(1000*1000))
#define TIMER_TICKS_PER_US          PLATFORM_TIMER_TICKS_PER_US

#define tcurrent()                  platform_tcurrent()
#define ucurrent()                  (tcurrent()/TIMER_TICKS_PER_US)
#define mcurrent()                  (tcurrent()/(1000*TIMER_TICKS_PER_US))

#endif /* MPW7705_TIMER_H_ */

/*
 *  Created on: Jul 8, 2015
 *      Author: a.korablinov
 */

#ifndef __PPC_476FP_ITRPT_H__
#define __PPC_476FP_ITRPT_H__

#include "ppc470s_regs.h"

#define ITRPT_TUPLE_SIZE    2
#define ITRPT_TUPLE_IVO_i(X, Y) X
#define ITRPT_TUPLE_RET_i(X, Y) Y
#define ITRPT_CRITICAL_INPUT        ( SPR_IVOR0,    rfci )
#define ITRPT_MACHINE_CHECK         ( SPR_IVOR1,    rfmci )
#define ITRPT_DATA_STORAGE          ( SPR_IVOR2,    rfi )
#define ITRPT_INSTRUCTION_STORAGE   ( SPR_IVOR3,    rfi )
#define ITRPT_EXTERNAL_INPUT        ( SPR_IVOR4,    rfi )
#define ITRPT_ALIGNMENT             ( SPR_IVOR5,    rfi )
#define ITRPT_PROGRAM               ( SPR_IVOR6,    rfi )
#define ITRPT_FP_UNAVAILABLE        ( SPR_IVOR7,    rfi )
#define ITRPT_SYSTEM_CALL           ( SPR_IVOR8,    rfi )
#define ITRPT_AP_UNAVAILABLE        ( SPR_IVOR9,    rfi )
#define ITRPT_DECREMENTER           ( SPR_IVOR10,   rfi )
#define ITRPT_FIXED_INTERVAL_TIMER  ( SPR_IVOR11,   rfi )
#define ITRPT_WATCHDOG_TIMER        ( SPR_IVOR12,   rfci )
#define ITRPT_DATA_TLB_ERROR        ( SPR_IVOR13,   rfi )
#define ITRPT_INSTRUCTION_TLB_ERROR ( SPR_IVOR14,   rfi )
#define ITRPT_DEBUG                 ( SPR_IVOR15,   rfci )

#define STRINGIZE_(X) #X
#define STRINGIZE(X) STRINGIZE_(X)

#define SET_INTERRUPT_HANDLER( interrupt, handler )\
asm volatile (\
    "b      2f\n\t"\
    ".align 4\n\t"\
    "1:\n\t"\
    "stwu   r1,-160(r1)\n\t"\
    "stw    r0, 156(r1)\n\t"\
    "stmw   r2, 36(r1)\n\t"\
    \
    "mflr   r0\n\t"\
    "stw    r0, 32(r1)\n\t"\
    \
    "mfcr   r0\n\t"\
    "stw    r0, 28(r1)\n\t"\
    \
    "mfctr  r0\n\t"\
    "stw    r0, 24(r1)\n\t"\
    \
    "mfxer  r0\n\t"\
    "stw    r0, 20(r1)\n\t"\
    \
    "lis    r0, %0@h\n\t"\
    "ori    r0, r0, %0@l\n\t"\
    "mtctr  r0\n\t"\
    "bctrl\n\t"\
    \
    "lwz    r0, 20(r1)\n\t"\
    "mtxer  r0\n\t"\
    \
    "lwz    r0, 24(r1)\n\t"\
    "mtctr  r0\n\t"\
    \
    "lwz    r0, 28(r1)\n\t"\
    "mtcr   r0\n\t"\
    \
    "lwz    r0, 32(r1)\n\t"\
    "mtlr   r0\n\t"\
    \
    "lmw    r2, 36(r1)\n\t"\
    "lwz    r0, 156(r1)\n\t"\
    "addi   r1,r1,160\n\t"\
    \
    STRINGIZE(ITRPT_TUPLE_RET_i interrupt) "\n\t"\
    "2:\n\t"\
    "lis    r0, 1b@h\n\t"\
    "mtspr  63, r0\n\t"\
    "li     r0, 1b@l\n\t"\
    "mtspr  " STRINGIZE(ITRPT_TUPLE_IVO_i interrupt) ", r0\n\t"\
    ::"i"(handler)\
    :"r0"\
)

#endif //__PPC_476FP_ITRPT_H__

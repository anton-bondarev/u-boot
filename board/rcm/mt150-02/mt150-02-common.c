/*
 * RCM 1888BM18 common code for SPL and main bootloader board support
 *
 * Copyright (C) 2020 MIR
 *	Mikhail.Petrov@mir.dev
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

#define SPRN_DBCR0_44X 0x134
#define DBCR0_RST_SYSTEM 0x30000000

#define SPR_TBL_R 0x10C
#define SPR_TBU_R 0x10D

static uint64_t tcurrent(void)
{
	uint32_t valuel = 0;
	uint32_t valueh = 0;

	__asm volatile(
		"mfspr %0, %1 \n\t"
		: "=r"(valuel)
		: "i"(SPR_TBL_R)
		:);

	__asm volatile(
		"mfspr %0, %1 \n\t"
		: "=r"(valueh)
		: "i"(SPR_TBU_R)
		:);


	return (((uint64_t)valueh) << 32) + valuel;
}

static uint32_t mcurrent(void)
{
	return (tcurrent() / (1000 * TIMER_TICKS_PER_US));
}

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long tmp;

	asm volatile(
		"mfspr	%0,%1\n"
		"oris	%0,%0,%2@h\n"
		"mtspr	%1,%0"
		: "=&r"(tmp)
		: "i"(SPRN_DBCR0_44X), "i"(DBCR0_RST_SYSTEM));

	return 0;
}

// returns time in milliseconds
ulong get_timer(ulong base)
{
	return mcurrent() - base;
}

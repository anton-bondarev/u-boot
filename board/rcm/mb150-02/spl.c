/*
 * RCM 1888BM18 SPL startup code
 *
 * Copyright (C) 2020 MIR
 *	Mikhail.Petrov@mir.dev
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <spl.h>
#include <asm/tlb47x.h>
#include <rcm-emi.h>
#include "boot.h"
#include "emi-temp.h" // ???

DECLARE_GLOBAL_DATA_PTR;

extern void _start(void);

static const __attribute__((used)) __attribute__((section(".header")))
struct rumboot_bootheader hdr = {
	.magic = RUMBOOT_HEADER_MAGIC,
	.version = RUMBOOT_HEADER_VERSION,
	.chip_id = RUMBOOT_PLATFORM_CHIPID,
	.chip_rev = RUMBOOT_PLATFORM_CHIPREV,
	.entry_point = {
		(uint32_t)&_start,
	}
};

static void init_periph_byte_order(void)
{
	tlb47x_inval(0xD0020000, TLBSID_64K); 
	tlb47x_map(0x20C0020000, 0xD0020000, TLBSID_64K, TLB_MODE_RW);
	tlb47x_inval(0xD0030000, TLBSID_64K); 
	tlb47x_map(0x20C0030000, 0xD0030000, TLBSID_64K, TLB_MODE_RW);
}

void board_init_f(ulong dummy)
{
	init_periph_byte_order();
	spl_early_init();
	preloader_console_init();
	spl_set_bd();
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_UART;
	// return BOOT_DEVICE_EDCL;
}

void spl_board_init(void)
{
	// ???
	if (!emi_init()) {
		printf("Error in the EMI initialization - resetting...\n");
		udelay(5 * 1000 * 1000);
		do_reset(0, 0, 0, 0);
	}
#ifdef CONFIG_SPL_RCM_EMI_CORE
	// ??? rcm_emi_init();
#endif
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_BOOTROM;
	spl_boot_list[1] = spl_boot_device();
}

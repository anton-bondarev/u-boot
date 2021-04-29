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
#include "plb6mcif2-bridge.h"
#include "boot.h"

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
	tlb47x_map_nocache(0x20C0020000, 0xD0020000, TLBSID_64K, TLB_MODE_RW);
	tlb47x_inval(0xD0030000, TLBSID_64K); 
	tlb47x_map_nocache(0x20C0030000, 0xD0030000, TLBSID_64K, TLB_MODE_RW);
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
	return BOOT_DEVICE_XMODEM_EDCL;
}

#ifdef CONFIG_SPL_RCM_EMI_CORE
static bool test_mem(uint32_t base, uint32_t size)
{
	volatile uint32_t *start = (volatile uint32_t*)base;
	uint32_t count = size / 4;
	volatile uint32_t *p;
	uint32_t i;

	p = start;
	for (i = 0; i < count; ++i) {
		*p = (uint32_t)p;
		++p;
	}

	p = start;
	for (i = 0; i < count; ++i) {
		if (*p != (uint32_t)p)
			return false;
		++p;
	}

	return true;
}
#endif // CONFIG_SPL_RCM_EMI_CORE

#ifdef CONFIG_SPL_RCM_EMI_CORE
static void map_memory_range(const struct rcm_emi_memory_range *range, const char *mem_name)
{
	tlb47x_inval(range->base, TLBSID_256M);
	tlb47x_map_nocache(range->base, range->base, TLBSID_256M, TLB_MODE_RWX);
	printf("Testing %s...\n", mem_name);
	if (!test_mem(range->base, 0x10000)) {
		printf("The test has been failed. Resetting...");
		udelay(5 * 1000 * 1000);
		do_reset(NULL, 0, 0, NULL);
	}
}
#endif // CONFIG_SPL_RCM_EMI_CORE


void spl_board_init(void) {
#ifdef CONFIG_SPL_RCM_EMI_CORE
	const struct rcm_emi_memory_type_config *memory_type_config;
	plb6mcif2_init();
	rcm_emi_init();

	// map first 256 MB of SRAM
	memory_type_config = &rmc_emi_get_memory_config()->memory_type_config[RCM_EMI_MEMORY_TYPE_ID_SRAM];
	if (memory_type_config->bank_number != 0)
		map_memory_range(&memory_type_config->ranges[0], "SRAM");

	// map first 256 MB of SDRAM
	memory_type_config = &rmc_emi_get_memory_config()->memory_type_config[RCM_EMI_MEMORY_TYPE_ID_SDRAM];
	if (memory_type_config->bank_number != 0)
		map_memory_range(&memory_type_config->ranges[0], "SDRAM");
#endif // CONFIG_SPL_RCM_EMI_CORE
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = BOOT_DEVICE_BOOTROM;
	spl_boot_list[1] = spl_boot_device();
}

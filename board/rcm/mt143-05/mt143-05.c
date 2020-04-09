/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 */
#include <version.h>
#include <common.h>
#include <config.h>
#include <spl.h>
#include <dm.h>
#include <fdt_support.h>
#include <hang.h>
#include <linux/io.h>
#include "boot.h"
#include <debug_uart.h>

#ifdef CONFIG_SPL_BUILD

extern void reset(void);

static const __attribute__((used)) __attribute__((section(".header")))
struct rumboot_bootheader hdr = {
    .magic      = RUMBOOT_HEADER_MAGIC,
    .version    = RUMBOOT_HEADER_VERSION,
    .chip_id    = RUMBOOT_PLATFORM_CHIPID,
    .chip_rev    = RUMBOOT_PLATFORM_CHIPREV,
    .entry_point    = {
        (uint32_t)&reset,
    }
};


u32 spl_boot_device(void)
{
	return BOOT_DEVICE_MMC1;
}


void board_init_f(ulong dummy)
{
	timer_init();

    int ret = spl_early_init();
    if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();

}

volatile unsigned int* _test_addr;

bool is_ddr_ok(void)
{
	int i;
	_test_addr = (unsigned int*)CONFIG_SYS_TEXT_BASE; 

	for(i = 0; i < 256; i++)
		_test_addr[i] = (unsigned int) &_test_addr[i];
	printf("DDR test...");

	for(i = 0; i < 256; i++)
	{
		if(_test_addr[i] != (unsigned int) &_test_addr[i])
		{
			printf(" Error at %d [%08X != %08X]\n", i, _test_addr[i], (unsigned int)&_test_addr[i]);
			break;
		}
	}
	if(i == 256)
	{
		printf(" OK.\n");
		return 1;
	}
	else
		return 0;
}

void spl_board_init(void)
{
	if(!is_ddr_ok())
	{
		printf("Error in DDR resetting...\n");
		udelay(1000000);
		do_reset(0,0,0,0);
	}

	gd->ram_size = CONFIG_SYS_DDR_SIZE;

	u32 boot_device = spl_boot_device();

	if (boot_device == BOOT_DEVICE_SPI)
		puts("Booting from SPI flash\n");
	else if (boot_device == BOOT_DEVICE_MMC1)
		puts("Booting from SD card\n");
	else
		puts("Unknown boot device\n");
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = spl_boot_device();
	switch (spl_boot_list[0]) {
	case BOOT_DEVICE_SPI:
		spl_boot_list[1] = BOOT_DEVICE_MMC1;
		break;
	case BOOT_DEVICE_MMC1:
		spl_boot_list[1] = BOOT_DEVICE_SPI;
		break;
	}
    spl_boot_list[2] = BOOT_DEVICE_UART; // Y-Modem
    spl_boot_list[3] = BOOT_DEVICE_NONE;
}


#endif

int dram_init(void)
{
    gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

    return 0;
}

int board_init(void)
{
    return 0; 
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("SoC: 1888bc048, ver Basis\n");
	return 0;
}
#endif

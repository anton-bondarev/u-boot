/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 */

#ifndef __1888BC048_H
#define __1888BC048_H

#include <linux/sizes.h>

#ifdef CONFIG_SPL_BUILD

#define CONFIG_SPL_STACK		(0x00060000 - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SPL_MAX_FOOTPRINT	0x00040000

#define CONFIG_SYS_SPL_MALLOC_SIZE	SZ_128K
#define CONFIG_SYS_SPL_MALLOC_START	0x00060000
#endif

#define CONFIG_SYS_MALLOC_LEN		SZ_4M

#define CONFIG_SYS_LOAD_ADDR            0x40200000

/* to check following */
#define CONFIG_SYS_SDRAM_BASE		0x81000000
#define CONFIG_SYS_SDRAM_SIZE		SZ_512M - SZ_16M
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_SDRAM_BASE + \
					 CONFIG_SYS_SDRAM_SIZE - \
					 GENERATED_GBL_DATA_SIZE)

#define CONFIG_SYS_DDR_SIZE     SZ_2G

#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SYS_SPI_U_BOOT_OFFS      0x40000

#endif

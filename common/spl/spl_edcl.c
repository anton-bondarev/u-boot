/*
 * EDCL SPL loader
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define DEBUG
#include <common.h>
#include <spl.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_TARGET_1888TX018


static void (*bootrom_enter_host_mode)(void) = (void (*)(void)) BOOT_ROM_HOST_MODE;
#endif

static int spl_edcl_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
#ifdef CONFIG_TARGET_1888TX018
	debug("Enter HOST mode for EDCL loading\n");
	bootrom_enter_host_mode();
#elif CONFIG_TARGET_1879VM8YA
	{
		volatile unsigned long *edcl_sync_cell_ptr = (unsigned long *) EDCL_SYNC_ADDR; 

		// wait for u-boot-dtb.img loading
		*edcl_sync_cell_ptr = 1;
		printf("Enter HOST mode for EDCL loading. waiting for marker at %08X\n", EDCL_SYNC_ADDR);

		// wait finish of loading
		while (*edcl_sync_cell_ptr != 0) {};

		printf("Image found - jumping to %08X\n", CONFIG_SYS_TEXT_BASE);
		return -1;
	}
#else
	debug("EDCL loading not supported. Halt\n");
	hang();
#endif
	return 0; /* never happens */
}

#ifdef CONFIG_TARGET_1888TX018
SPL_LOAD_IMAGE_METHOD("EDCL", 0, BOOT_DEVICE_EDCL, spl_edcl_load_image);
#else
SPL_LOAD_IMAGE_METHOD("EDCL", 0, BOOT_DEVICE_XIP, spl_edcl_load_image);
#endif
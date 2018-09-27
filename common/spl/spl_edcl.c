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

#ifdef CONFIG_MPW7705


static void (*bootrom_enter_host_mode)(void) = (void (*)(void)) BOOT_ROM_HOST_MODE;
#endif

static int spl_edcl_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
#ifdef CONFIG_MPW7705
	debug("Enter HOST mode for EDCL loading\n");
	bootrom_enter_host_mode();
#else
	debug("EDCL loading not supported. Halt\n");
	while(1);
#endif
	return 0; /* never happens */
}

SPL_LOAD_IMAGE_METHOD("EDCL", 0, BOOT_DEVICE_EDCL, spl_edcl_load_image);

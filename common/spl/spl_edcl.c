/*
 * EDCL SPL loader
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spl.h>

DECLARE_GLOBAL_DATA_PTR;

void (*bootrom_enter_host_mode)() = BOOT_ROM_HOST_MODE;

static int spl_edcl_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	debug("Enter HOST mode for EDCL loading\n");
	bootrom_enter_host_mode();
}

SPL_LOAD_IMAGE_METHOD("EDCL", 0, BOOT_DEVICE_EDCL, spl_edcl_load_image);

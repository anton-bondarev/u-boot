/*
 * EDCL SPL loader
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

// #define DEBUG

#include <common.h>
#include <spl.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

static int spl_edcl_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
#if defined(CONFIG_TARGET_1888TX018) || defined(CONFIG_TARGET_1888BM18)
	volatile struct image_header *hdr = (volatile struct image_header*)(CONFIG_SYS_TEXT_BASE - sizeof(struct image_header));
	hdr->ih_magic = 0;
	printf("Enter HOST mode for EDCL loading\n");
	while (true) {
		if (hdr->ih_magic != IH_MAGIC)
			continue;
		if (!image_check_hcrc((struct image_header*)hdr))
			continue;
		if (!image_check_dcrc((struct image_header*)hdr))
			continue;
		if (spl_parse_image_header(spl_image, (struct image_header*)hdr) == 0)
			break;
	}
	printf("The image has been loaded\n");
	udelay(1000000);
#elif CONFIG_TARGET_1879VM8YA
	{
		volatile unsigned long *edcl_sync_cell_ptr = (unsigned long *) EDCL_SYNC_ADDR; 

		spl_edcl_enable();

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
	return 0;
}

#if defined(CONFIG_TARGET_1888TX018) || defined(CONFIG_TARGET_1888BM18)
SPL_LOAD_IMAGE_METHOD("EDCL", 0, BOOT_DEVICE_EDCL, spl_edcl_load_image);
#else
SPL_LOAD_IMAGE_METHOD("EDCL", 0, BOOT_DEVICE_XIP, spl_edcl_load_image);
#endif

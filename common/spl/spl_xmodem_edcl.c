/*
 * Unified XMODEM/EDCL SPL loader
 *
 * Copyright (C) 2020 MIR
 *	Mikhail.Petrov@mir.dev
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

// #define DEBUG

#include <common.h>
#include <spl.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

static struct spl_image_loader *find_xmodem_loader(void)
{
	struct spl_image_loader *drv = ll_entry_start(struct spl_image_loader, spl_image_loader);
	const int n_ents = ll_entry_count(struct spl_image_loader, spl_image_loader);
	struct spl_image_loader *entry;

	for (entry = drv; entry != drv + n_ents; entry++) {
		if (entry->boot_device == BOOT_DEVICE_UART)
			return entry;
	}

	return NULL;
}


static int test_edcl_image(struct spl_image_info *spl_image, struct image_header *hdr)
{
	if (hdr->ih_magic != IH_MAGIC)
		return -EFAULT;
	if (!image_check_hcrc(hdr))
		return -EFAULT;
	if (!image_check_dcrc(hdr))
		return -EFAULT;
	if (!spl_parse_image_header(spl_image, hdr) == 0)
		return -EFAULT;

	return 0;
}

static int spl_xmodem_edcl_load_image(struct spl_image_info *spl_image, struct spl_boot_device *bootdev)
{
	uint32_t base_addr = CONFIG_SYS_TEXT_BASE - sizeof(struct image_header); // ??? no edcl_addr
	uint32_t edcl_addr = base_addr;
	struct spl_image_loader *xmodem_loader = find_xmodem_loader();
	int chr;
	int ret;

	if (xmodem_loader == NULL)
		printf("Warning: XMODEM loader has not been found\n");

	while (true) {
		*((unsigned*)base_addr) = 0; // clear the image magic word
		printf("UPLOAD to 0x%08x. 'X' for X-modem, 'E' for EDCL\n", edcl_addr);
		do {
			chr = getc();
		} while ((chr != 'X') && (chr != 'E'));

		if ((chr == 'X') && (xmodem_loader != NULL)) {
#ifdef CONFIG_TARGET_1888BM18
		printf("CC"); // ????
#endif
			ret = xmodem_loader->load_image(spl_image, NULL);
			if (ret == 0)
				break;
		} else if (chr == 'E') {
			ret = test_edcl_image(spl_image, (struct image_header*)(base_addr));
			if (ret == 0)
				break;
		}

		printf("Image checksum error\n");
	}

	printf("The image has been loaded\n");
	udelay(1000000);

	return 0;
}

#if defined(CONFIG_TARGET_1888TX018) || defined(CONFIG_TARGET_1888BM18)
SPL_LOAD_IMAGE_METHOD("X-MODEM/EDCL", 0, BOOT_DEVICE_XMODEM_EDCL, spl_xmodem_edcl_load_image);
#else
#error Unsupported platform
#endif

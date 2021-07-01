/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 */
//#define DEBUG

#include <version.h>
#include <common.h>
#include <config.h>
#include <spl.h>
#include <dm.h>
#include <fdt_support.h>
#include <hang.h>
#include <linux/io.h>
#include "boot.h"

#define BUF_SIZE 1024
extern char spl_header;

static size_t round_up_to_align(size_t value, size_t align)
{
	if (!align) {
		return value;
	}
	size_t misalign = value % align;
	if (misalign) {
		value += align - misalign;
	}
	return value;
}

static int rumboot_load_image(struct spl_image_info *spl_image,
			      struct spl_boot_device *bootdev)
{
	char pdata[128];
	struct rumboot_bootheader *hdr =
		(struct rumboot_bootheader *)&(spl_header);
	const struct rumboot_bootsource *src = hdr->device;
	int ret = -EIO;

	debug("rb: Trying to chain boot with rumboot...\n");
	if (!src || !src->plugin) {
		printf("Skip rumboot chain - host mode\n");
		return -ENOMEDIUM;
	}

	debug("rb: src: %p, plugin: %p, init:%p\n", src, src->plugin,
	      src->plugin->init);
	debug("rb: original offset: %x\n", (unsigned int)(src->offset));

	if (!src->plugin->init(src, pdata))
		return -EIO;

	bool once_again;
	unsigned int add_offset = src->offset;
	do {
		char buf[BUF_SIZE];
		once_again = false;
		debug("rb: Using %x offset for chain boot\n", add_offset);
		if (BUF_SIZE ==
		    src->plugin->read(src, pdata, buf, add_offset, BUF_SIZE)) {
			struct rumboot_bootheader *hdr0 =
				(struct rumboot_bootheader *)buf;
			struct image_header *ih = NULL;
			ih = (struct image_header *)buf;

			if (hdr0->magic == RUMBOOT_HEADER_MAGIC) {
				once_again = true;
				add_offset += round_up_to_align(
					hdr0->datalen + sizeof(struct rumboot_bootheader),
					src->plugin->align);
				debug("Found rumboot image - try next one\n");
				continue;
			} else if (!spl_parse_image_header(spl_image, ih)) {
				debug("rb: U-boot header detected...\n");
				void *addr = 0;
#ifdef CONFIG_SPL_GZIP
				if (ih->ih_comp == IH_COMP_GZIP)
					addr = CONFIG_SYS_LOAD_ADDR;
				else
#endif
					addr = (void *)spl_image->load_addr;

				debug("rb: Load image to %p, len %d\n", addr,
				      spl_image->size);
				size_t readed =
					src->plugin->read(src, pdata, addr,
							  add_offset,
							  spl_image->size);
				debug("rb: readed %d\n", readed);
				if (round_up_to_align(spl_image->size,
						      src->plugin->align) ==
				    readed)
					ret = 0;
			}
		}
	} while (once_again);

	src->plugin->deinit(src, pdata);

	debug("rb: finished with %d code", ret);

	return ret;
}

SPL_LOAD_IMAGE_METHOD("RUMBOOT", 0, BOOT_DEVICE_BOOTROM, rumboot_load_image);

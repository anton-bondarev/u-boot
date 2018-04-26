/*
 * PL061  pinctrl driver
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define DEBUG 1
#include <common.h>
#include <dm.h>
#include <dm/pinctrl.h>
#include <linux/io.h>
#include <linux/err.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <pl061.h>

DECLARE_GLOBAL_DATA_PTR;

struct pinctrl_pl061_platdata {
	struct pl061_regs *regs;
};

#define MAX_PINMUX_ENTRIES	40

static int pl061_set_state(struct udevice *dev, struct udevice *config)
{
	struct pinctrl_pl061_platdata *plat = dev_get_platdata(dev);
	struct pl061_regs *const regs = plat->regs;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(config);
	unsigned int cells[MAX_PINMUX_ENTRIES];
	int count, i;

	debug("pl061_set_state\n");

	count = fdtdec_get_int_array_count(blob, node, "pinmux",
					   cells, ARRAY_SIZE(cells));
	if (count < 0) {
		printf("%s: bad pinmux array %d\n", __func__, count);
		return -EINVAL;
	}

	if (count > MAX_PINMUX_ENTRIES) {
		printf("%s: unsupported pinmux array count %d\n",
		       __func__, count);
		return -EINVAL;
	}

	for (i = 0 ; i < count; i++) {
		int pinno = cells[i] & 0x7;
		int func = (cells[i] >> 4) & 1;
		int inout = (cells[i] >> 5) & 1;
		int lsif = (cells[i] >> 6) & 1;
		int mgpio = (cells[i] >> 7) & 0x0f;
		int val = (cells[i] >> 11) & 1;

		if(func == 0) // SW mode
		{
			// set sw function
			clrbits(8, &regs->afsel, 1 << pinno);
			
			if(inout == 0)
			   clrbits(8, &regs->dir, 1 << pinno);
			else
			{
				setbits(8, &regs->dir, 1 << pinno);			
				if(val)
			   		setbits(8, &regs->data, 1 << pinno);							
				else
			   		clrbits(8, &regs->data, 1 << pinno);							
			}
		}
		else          // HW mode
		{
			// set hw function
			setbits(8, &regs->afsel, 1 << pinno);
		}

		debug("pinctrl: 0x%x(%x):0x%x(%x) lsif%d, mgpio%d[%d] to %d, %d\n", &regs->dir, readb(&regs->dir) ,&regs->afsel, readb(&regs->afsel), lsif, mgpio, pinno, func, inout);		
	}

	return 0;
}

const struct pinctrl_ops pl061_pinctrl_ops  = {
	.set_state = pl061_set_state,
};

static int pl061_pinctrl_probe(struct udevice *dev)
{
	struct pinctrl_pl061_platdata *plat = dev_get_platdata(dev);
	fdt_addr_t addr_base;

	debug("pl061_pinctrl_probe\n");

	dev = dev_get_parent(dev);
	addr_base = devfdt_get_addr(dev);
	if (addr_base == FDT_ADDR_T_NONE)
		return -EINVAL;

	plat->regs = (struct pl061_regs *)addr_base;

	struct pl061_regs *const regs = plat->regs;
	if( (readb(&regs->pid0) == 0x61) && 
		(readb(&regs->pid1) == 0x10) && 
		(readb(&regs->pid2) == 0x04) && 
		(readb(&regs->pid3) == 0x00))
		{
			debug("pl061_pinctrl_probe ok\n");
			return 0;
		}

	debug("pl061_pinctrl_probe fail\n");
	debug("GPIO probe id's: %02X %02X %02X %02X %02X %02X %02X %02X \n",
		readb(&regs->pid0),
		readb(&regs->pid1),
		readb(&regs->pid2),
		readb(&regs->pid3),
		readb(&regs->cid0),
		readb(&regs->cid1),
		readb(&regs->cid2),
		readb(&regs->cid3)			
	);	
	return 0;
}

static const struct udevice_id pl061_pinctrl_match[] = {
	{ .compatible = "rc-module,pl061-pinctrl" },
	{}
};

U_BOOT_DRIVER(pl061_pinctrl) = {
	.name = "pl061-pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = pl061_pinctrl_match,
	.probe = pl061_pinctrl_probe,
	.platdata_auto_alloc_size = sizeof(struct pinctrl_pl061_platdata),
	.ops = &pl061_pinctrl_ops
};

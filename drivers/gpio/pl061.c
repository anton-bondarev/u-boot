/*
 * PL061 GPIO driver
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define DEBUG 1
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <pl061.h>

DECLARE_GLOBAL_DATA_PTR;


struct pl061_platdata {
	struct pl061_regs *regs;
	int gpio_count;
	const char *bank_name;
};

static int pl061_direction_input(struct udevice *dev, unsigned pin)
{
	struct pl061_platdata *plat = dev_get_platdata(dev);
	struct pl061_regs *const regs = plat->regs;

	clrbits(8, &regs->dir, 1 << pin);

	return 0;
}

static int pl061_direction_output(struct udevice *dev, unsigned pin,
				     int val)
{
	struct pl061_platdata *plat = dev_get_platdata(dev);
	struct pl061_regs *const regs = plat->regs;

	if (val)
		setbits(8, &regs->data, 1 << pin);
	else
		clrbits(8, &regs->data, 1 << pin);
	/* change the data first, then the direction. to avoid glitch */
	setbits(8, &regs->dir, 1 << pin);

	return 0;
}

static int pl061_get_value(struct udevice *dev, unsigned pin)
{
	struct pl061_platdata *plat = dev_get_platdata(dev);
	struct pl061_regs *const regs = plat->regs;

	return readb(&regs->data) & (1 << pin);
}


static int pl061_get_function(struct udevice *dev, unsigned pin)
{
	struct pl061_platdata *plat = dev_get_platdata(dev);
	struct pl061_regs *const regs = plat->regs;

	if(readb(&regs->afsel) & (1 << pin))	// if software mode
	{
		if(readb(&regs->dir) & (1 << pin))
			return GPIOF_OUTPUT;
		else
			return GPIOF_INPUT;

	}
	return GPIOF_FUNC;
}


static int pl061_set_value(struct udevice *dev, unsigned pin, int val)
{
	struct pl061_platdata *plat = dev_get_platdata(dev);
	struct pl061_regs *const regs = plat->regs;

	if (val)
		setbits(8, &regs->data, 1 << pin);
	else
		clrbits(8, &regs->data, 1 << pin);

	return 0;
}

static int pl061_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct pl061_platdata *plat = dev_get_platdata(dev);

	struct pl061_regs *const regs = plat->regs;
	if( (readb(&regs->pid0) == 0x61) && 
		(readb(&regs->pid1) == 0x10) && 
		(readb(&regs->pid2) == 0x04) && 
		(readb(&regs->pid3) == 0x00))
		return 0;

	debug("pl061_probe fail\n");
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

	return -EINVAL;
}

static int pl061_ofdata_to_platdata(struct udevice *dev)
{
	struct pl061_platdata *plat = dev_get_platdata(dev);

	plat->regs = map_physmem(devfdt_get_addr(dev),
				 sizeof(struct pl061_regs),
				 MAP_NOCACHE);
//	plat->gpio_count = fdtdec_get_int(gd->fdt_blob, dev_of_offset(dev),
//		"altr,gpio-bank-width", 32);
//	plat->bank_name = fdt_getprop(gd->fdt_blob, dev_of_offset(dev),
//		"gpio-bank-name", NULL);

	return 0;
}

static const struct dm_gpio_ops pl061_ops = {
	.direction_input	= pl061_direction_input,
	.direction_output	= pl061_direction_output,
	.get_value		= pl061_get_value,
	.set_value		= pl061_set_value,
	.get_function	= pl061_get_function,
};

static const struct udevice_id pl061_ids[] = {
	{ .compatible = "rc-module,pl061" },
	{ }
};

U_BOOT_DRIVER(pl061) = {
	.name		= "pl061",
	.id		= UCLASS_GPIO,
	.of_match	= pl061_ids,
	.ops		= &pl061_ops,
	.ofdata_to_platdata = pl061_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct pl061_platdata),
	.probe		= pl061_probe,
	.bind = dm_scan_fdt_dev,
};

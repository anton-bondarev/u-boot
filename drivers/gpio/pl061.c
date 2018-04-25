/*
 * Copyright (C) 2018  Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#define DEBUG 1
#include <common.h>
#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <asm/gpio.h>

DECLARE_GLOBAL_DATA_PTR;

struct pl061_regs {
	u8	data; u8 skip[0x3ff];	/* Data registers */
	u8  dir; u8  skip0[3];		/* +0x400 - direction register */	
	u8  is;  u8  skip1[3];		/* +0x404 - interrupt detection register */
	u8  ibe; u8  skip2[3];		/* +0x408 - dual front interrupt register */
	u8  iev; u8  skip3[3];		/* +0x40C - interrupt event register */
	u8  ie;  u8  skip4[3];		/* +0x410 - interrupt mask register */
	const u8  ris; u8  skip5[3];/* +0x414 - interrupt state register w/o mask */
	const u8  mis; u8  skip6[3];/* +0x418 - interrupt state register with mask */
	u8  ic;  u8  skip7[3];		/* +0x41C - interrupt clear register */
	u8  afsel; u8  skip8[3];	/* +0x420 - mode setup register */
	u8 reserved[0xbbc];
	const u8 pid0; u8  skip9[3];/* +0xfe0 - peripheral identifier 0x61 */
	const u8 pid1; u8  skipa[3];/* +0xfe4 - peripheral identifier 0x10 */
	const u8 pid2; u8  skipb[3];/* +0xfe8 - peripheral identifier 0x04 */
	const u8 pid3; u8  skipc[3];/* +0xfec - peripheral identifier 0x00 */	
	const u8 cid0; u8  skipd[3];/* +0xff0 - cell identifier 0x0d */
	const u8 cid1; u8  skipe[3];/* +0xff4 - cell identifier 0xf0 */
	const u8 cid2; u8  skipf[3];/* +0xff8 - cell identifier 0x05 */
	const u8 cid3; u8  skipg[3];/* +0xffc - cell identifier 0xb1 */	
};

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

	//uc_priv->gpio_count = plat->gpio_count;
	//uc_priv->bank_name = plat->bank_name;

	return 0;
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
};

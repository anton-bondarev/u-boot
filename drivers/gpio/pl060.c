/*
 * PL060 + RCM Hacks GPIO driver
 *
 * Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
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

DECLARE_GLOBAL_DATA_PTR;

struct pl060_regs
{
    u32 pdr0;     /* +0x00 Data register 0 */
    u32 skip0[3]; /*  */
    u32 ddr0;     /* +0x10 Direction register 0 */
    u32 skip1[3]; /*  */
    u32 eienb;    /* +0x20 Enable interrupt register */
    u32 eireq;    /* +0x24 Interrupts request register */
    u32 eilvl;    /* +0x28 Interrupts level register */
    u32 skip2[1]; /*  */
    u32 ra_contr; /* +0x30 GERA pair assigment */
    u32 ra_val;   /* +0x34 GERA set/req */
    u32 regime;   /* +0x38 pins mode */
    u32 skip3[0x3e9]; /*  */
    u32 pid0;
    u32 pid1;
    u32 pid2;
    u32 pid3;
    u32 cid0;
    u32 cid1;
    u32 cid2;
    u32 cid3;
};

struct pl060_platdata
{
    struct pl060_regs *regs;
    int gpio_count;
};

static int pl060_direction_input(struct udevice *dev, unsigned pin)
{
    struct pl060_platdata *plat = dev_get_platdata(dev);
    struct pl060_regs *const regs = plat->regs;

    debug("pl060_direction_input set for %d\n", pin);

    clrbits(le32, &regs->ddr0, 1 << pin);

    return 0;
}

static int pl060_direction_output(struct udevice *dev, unsigned pin,
                                  int val)
{
    struct pl060_platdata *plat = dev_get_platdata(dev);
    struct pl060_regs *const regs = plat->regs;
    debug("pl060_direction_output set for %d\n", pin);

    if (val)
        setbits(le32, &regs->pdr0, 1 << pin);
    else
        clrbits(le32, &regs->pdr0, 1 << pin);
    /* change the data first, then the direction. to avoid glitch */
    setbits(le32, &regs->ddr0, 1 << pin);

    return 0;
}

static int pl060_get_value(struct udevice *dev, unsigned pin)
{
    struct pl060_platdata *plat = dev_get_platdata(dev);
    struct pl060_regs *const regs = plat->regs;

    unsigned int val = readl(&regs->pdr0) & (1 << pin);

    debug("pl060_get_value for %d == %d\n", pin, val);

    return val == 0?0:1;
}

static int pl060_get_function(struct udevice *dev, unsigned pin)
{
    struct pl060_platdata *plat = dev_get_platdata(dev);
    struct pl060_regs *const regs = plat->regs;

    unsigned int val = readl(&regs->regime);
    if(((val >> (pin&~1)) & 0x3) == 0x3)
    {
        if (readl(&regs->ddr0) & (1 << pin))
            return GPIOF_OUTPUT;
        else
            return GPIOF_INPUT;
    }
    return GPIOF_FUNC;
}

static int pl060_set_value(struct udevice *dev, unsigned pin, int val)
{
    struct pl060_platdata *plat = dev_get_platdata(dev);
    struct pl060_regs *const regs = plat->regs;

    debug("pl060_set_value set value for %d == %d\n", pin, val);

    if (val)
        setbits(le32, &regs->pdr0, 1 << pin);
    else
        clrbits(le32, &regs->pdr0, 1 << pin);

    return 0;
}

static int pl060_probe(struct udevice *dev)
{
    struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
    struct pl060_platdata *plat = dev_get_platdata(dev);

    struct pl060_regs *const regs = plat->regs;
    uc_priv->gpio_count = plat->gpio_count;
    debug("pl060_probe\n");


 //   if ((readl(&regs->pid0) == 0x61) &&
//        (readl(&regs->pid1) == 0x10) &&
//        (readl(&regs->pid2) == 0x04) &&
//        (readl(&regs->pid3) == 0x00))
    {
        writel(0xff, &regs->regime); // switch everything to gpio mode
        return 0;
    }

    debug("pl060_probe fail\n");
    debug("GPIO probe id's: %02X %02X %02X %02X %02X %02X %02X %02X \n",
          readl(&regs->pid0),
          readl(&regs->pid1),
          readl(&regs->pid2),
          readl(&regs->pid3),
          readl(&regs->cid0),
          readl(&regs->cid1),
          readl(&regs->cid2),
          readl(&regs->cid3));

    return -EINVAL;
}

static int pl060_ofdata_to_platdata(struct udevice *dev)
{
    struct pl060_platdata *plat = dev_get_platdata(dev);

    plat->regs = map_physmem(devfdt_get_addr(dev),
                             sizeof(struct pl060_regs),
                             MAP_NOCACHE);
    plat->gpio_count = 8;

    return 0;
}

static const struct dm_gpio_ops pl060_ops = {
    .direction_input = pl060_direction_input,
    .direction_output = pl060_direction_output,
    .get_value = pl060_get_value,
    .set_value = pl060_set_value,
    .get_function = pl060_get_function,
};

static const struct udevice_id pl060_ids[] = {
    {.compatible = "rcm,pl060"},
    {}};

U_BOOT_DRIVER(pl060) = {
    .name = "pl060",
    .id = UCLASS_GPIO,
    .of_match = pl060_ids,
    .ops = &pl060_ops,
    .ofdata_to_platdata = pl060_ofdata_to_platdata,
    .platdata_auto_alloc_size = sizeof(struct pl060_platdata),
    .probe = pl060_probe,
    .bind = dm_scan_fdt_dev,
};

/*
 * RCM GPIO driver
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
#include <linux/io.h>

DECLARE_GLOBAL_DATA_PTR;

#define RCM_GPIO_ID 0x4f495047
#define RCM_GPIO_VERSION 0x01

struct rcm_gpio_regs
{
    u32 id;             /* 0x000 */
    u32 version;        /* 0x004 */
    u32 pad_dir;        /* 0x008 */
    u32 write_to_pad;   /* 0x00c */
    u32 read_from_pad;  /* 0x010 */
    u32 int_status;     /* 0x014 */
    u32 int_mask;       /* 0x018 */
    u32 set0;           /* 0x01c */
    u32 set1;           /* 0x020 */
    u32 switch_source;  /* 0x024 */
};

struct rcm_gpio_platdata
{
    struct rcm_gpio_regs *regs;
    int gpio_count;
};

static int rcm_gpio_direction_input(struct udevice *dev, unsigned pin)
{
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);
    struct rcm_gpio_regs *const regs = plat->regs;

    debug("rcm_gpio_direction_input set for %d\n", pin);

    clrbits(le32, &regs->pad_dir, 1 << pin);

    return 0;
}

static int rcm_gpio_direction_output(struct udevice *dev, unsigned pin,
                                  int val)
{
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);
    struct rcm_gpio_regs *const regs = plat->regs;
    debug("rcm_gpio_direction_output set for %d\n", pin);

    if (val)
        setbits(le32, &regs->write_to_pad, 1 << pin);
    else
        clrbits(le32, &regs->write_to_pad, 1 << pin);
    /* change the data first, then the direction. to avoid glitch */
    setbits(le32, &regs->pad_dir, 1 << pin);

    return 0;
}

static int rcm_gpio_get_value(struct udevice *dev, unsigned pin)
{
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);
    struct rcm_gpio_regs *const regs = plat->regs;

    unsigned int val = ioread32(&regs->read_from_pad) & (1 << pin);

    debug("rcm_gpio_get_value for %d == %d\n", pin, val);

    return val == 0?0:1;
}

static int rcm_gpio_get_function(struct udevice *dev, unsigned pin)
{
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);
    struct rcm_gpio_regs *const regs = plat->regs;

    unsigned int val = ioread32(&regs->switch_source);
    if(val & (1<<pin))
    {
        if (ioread32(&regs->pad_dir) & (1 << pin))
            return GPIOF_OUTPUT;
        else
            return GPIOF_INPUT;
    }
    return GPIOF_FUNC;
}

static int rcm_gpio_set_value(struct udevice *dev, unsigned pin, int val)
{
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);
    struct rcm_gpio_regs *const regs = plat->regs;

    debug("rcm_gpio_set_value set value for %d == %d\n", pin, val);

    if (val)
        setbits(le32, &regs->write_to_pad, 1 << pin);
    else
        clrbits(le32, &regs->write_to_pad, 1 << pin);

    return 0;
}

static int rcm_gpio_probe(struct udevice *dev)
{
    struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);

    struct rcm_gpio_regs *const regs = plat->regs;
    uc_priv->gpio_count = plat->gpio_count;
    printf("rcm_gpio_probe\n");

    if(ioread32(&regs->id) != RCM_GPIO_ID || ioread32(&regs->version) != RCM_GPIO_VERSION)
    {
        printf("%s: error: Detected illegal version of gpio core: 0x%08x 0x%08x\n", dev->name,
            ioread32(&regs->id), 
            ioread32(&regs->version));
        return -EINVAL;
    }

    iowrite32(0xff, &regs->switch_source); // switch everything to gpio mode
    return 0;
}

static int rcm_gpio_ofdata_to_platdata(struct udevice *dev)
{
    struct rcm_gpio_platdata *plat = dev_get_platdata(dev);

    plat->regs = map_physmem(devfdt_get_addr(dev),
                             sizeof(struct rcm_gpio_regs),
                             MAP_NOCACHE);
    plat->gpio_count = 8;

    return 0;
}

static const struct dm_gpio_ops rcm_gpio_ops = {
    .direction_input = rcm_gpio_direction_input,
    .direction_output = rcm_gpio_direction_output,
    .get_value = rcm_gpio_get_value,
    .set_value = rcm_gpio_set_value,
    .get_function = rcm_gpio_get_function,
};

static const struct udevice_id rcm_gpio_ids[] = {
    {.compatible = "rcm,gpio"},
    {}};

U_BOOT_DRIVER(rcm_gpio) = {
    .name = "rcm_gpio",
    .id = UCLASS_GPIO,
    .of_match = rcm_gpio_ids,
    .ops = &rcm_gpio_ops,
    .ofdata_to_platdata = rcm_gpio_ofdata_to_platdata,
    .platdata_auto_alloc_size = sizeof(struct rcm_gpio_platdata),
    .probe = rcm_gpio_probe,
    .bind = dm_scan_fdt_dev,
};

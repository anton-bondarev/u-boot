/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 *
 */
#define DEBUG
#define DBGPREFIX "[rcm-sgmii-phy]: "

#include <version.h>
#include <common.h>
#include <config.h>
#include <dm.h>
#include <generic-phy.h>
#include <syscon.h>
#include <regmap.h>
#include <linux/io.h>

struct sgmii_phy_priv {
    struct regmap* sctl;
    unsigned int sctl_offset;
    void* regs;
    bool auto_negotiation;
	bool initialized;
};

static int sgmii_phy_power_on(struct phy *phy)
{
	struct sgmii_phy_priv *priv = dev_get_priv(phy->dev);
    int ret;
    unsigned int val = 0;
    unsigned long start = get_timer(0);

    dev_dbg(phy->dev, DBGPREFIX "poweron called\n");

	if (!priv->initialized)
		return -EIO;

    ret = regmap_write(priv->sctl, priv->sctl_offset, 1);
    if(ret)
        return ret;
    
    // wait for stabilization
    do
    {
        ret = regmap_read(priv->sctl, priv->sctl_offset, &val);
        if(ret)
            return ret;
        if (get_timer(start) > CONFIG_SYS_HZ)
            break;
    } while (val != 0x000001F1);

    if (val == 0x000001F1)
    {
        dev_dbg(phy->dev, DBGPREFIX "poweron succeed\n");
        return 0;
    }

    return -EIO;
}

static int sgmii_phy_power_off(struct phy *phy)
{
	struct sgmii_phy_priv *priv = dev_get_priv(phy->dev);

    dev_dbg(phy->dev, DBGPREFIX "poweroff called\n");

	if (!priv->initialized)
		return -EIO;

    return regmap_write(priv->sctl, priv->sctl_offset, 0);
}

static int sgmii_phy_init(struct phy *phy)
{
	struct sgmii_phy_priv *priv = dev_get_priv(phy->dev);
    void* regs = priv->regs;

    dev_dbg(phy->dev, DBGPREFIX "init called\n");

    // sgmii magic from manual
    iowrite32(0x40803004, regs + 0x0000);
    iowrite32(0x40803004, regs + 0x0400);
    iowrite32(0x40803004, regs + 0x0800);
    iowrite32(0x40803004, regs + 0x0C00);

	iowrite32(0x00130000, regs + 0x1004);
	iowrite32(0x710001F0, regs + 0x1008);
	iowrite32(0x00000002, regs + 0x100C);
	iowrite32(0x07000000, regs + 0x1020);

	iowrite32(0x0000CEA6, regs + 0x0108);
	iowrite32(0x0000CEA6, regs + 0x0508);
	iowrite32(0x0000CEA6, regs + 0x0908);
	iowrite32(0x0000CEA6, regs + 0x0D08);

    if(priv->auto_negotiation)
    {
        iowrite32(0x00001140, regs + 0x200);
        iowrite32(0x00001140, regs + 0x600);
        iowrite32(0x00001140, regs + 0xA00);
        iowrite32(0x00001140, regs + 0xE00);
    }
    else
    {
        iowrite32(0x00001140, regs + 0x200);
        iowrite32(0x00001140, regs + 0x600);
        iowrite32(0x00001140, regs + 0xA00);
        iowrite32(0x00001140, regs + 0xE00);
    }

	priv->initialized = true;

    return sgmii_phy_power_on(phy);
}

static int sgmii_phy_exit(struct phy *phy)
{
	struct sgmii_phy_priv *priv = dev_get_priv(phy->dev);

    dev_dbg(phy->dev, DBGPREFIX "exit called\n");

	priv->initialized = false;
	return 0;
}

static int sgmii_phy_probe(struct udevice *dev)
{
	struct sgmii_phy_priv *priv = dev_get_priv(dev);

    dev_dbg(dev, DBGPREFIX "probe called\n");

	priv->regs = (void*) devfdt_get_addr(dev);

    priv->auto_negotiation = dev_read_bool(dev, "auto-negotiation");

    priv->sctl = syscon_regmap_lookup_by_phandle(dev, "sctl");
	if (!IS_ERR(priv->sctl)) {
        int rc;
    	unsigned int tmp[2];

		rc =  dev_read_u32_array(dev, "sctl", tmp, 2);
		if (rc) {
			dev_err(dev, DBGPREFIX "couldn't get sgmii control reg offset (err %d)\n", rc);
			return rc;
		}
		priv->sctl_offset = tmp[1];
	}
    else
    {
        dev_err(dev, DBGPREFIX "couldn't get sgmii control syscon (sctl)\n");
        return -EINVAL;
    }

	priv->initialized = false;
	return 0;
}

static struct phy_ops sgmii_phy_ops = {
	.power_on = sgmii_phy_power_on,
	.power_off = sgmii_phy_power_off,
	.init = sgmii_phy_init,
	.exit = sgmii_phy_exit,
};

static const struct udevice_id sgmii_phy_ids[] = {
	{ .compatible = "rcm,sgmii-phy" },
	{ }
};

U_BOOT_DRIVER(rcm_sgmii_phy) = {
	.name		= "rcm_sgmii_phy",
	.id		= UCLASS_PHY,
	.of_match	= sgmii_phy_ids,
	.ops		= &sgmii_phy_ops,
	.probe		= sgmii_phy_probe,
	.priv_auto_alloc_size = sizeof(struct sgmii_phy_priv),
};

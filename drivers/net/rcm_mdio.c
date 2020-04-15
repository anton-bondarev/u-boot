/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 *
 */
#define DEBUG
#define DBGPREFIX "[rcm-mdio]: "

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <miiphy.h>
#include <phy.h>
#include <asm/io.h>
#include <linux/io.h>
#include <wait_bit.h>

#define MDIO_ID 0x4F49444D
#define MDIO_VER 0x00640101

typedef const volatile unsigned int roreg32;
typedef volatile unsigned int rwreg32;

#define ETH_RST_N                 0x00
#define MDC_EN                    0x00

#define START_WR                  0x00
#define START_RD                  0x01
#define BUSY                      0x02
#define ADDR_PHY                  0x03
#define ADDR_REG                  0x08
#define CTRL_DATA                 0x10 


typedef struct _rcm_mdio_regs
{
    roreg32 id;               /* 0x000 - device id         */
    roreg32 version;          /* 0x004 - device version    */
    rwreg32 status;           /* 0x008 - status            */
    rwreg32 irq_mask;         /* 0x00C - irq mask          */
    roreg32 phy_irq_state;    /* 0x010 - irq mask          */
    rwreg32 mg_control;       /* 0x014 - control reg       */
    rwreg32 eth_rst_n;        /* 0x018 - reset reg         */
    rwreg32 freq_divider;     /* 0x01C - frequency divider */
    rwreg32 mdio_en;          /* 0x020 - enable register   */
} rcm_mdio_regs;


typedef struct _rcm_mdio_priv
{
	rcm_mdio_regs *regs;

} rcm_mdio_priv;

static int rcm_mdio_reset(struct udevice *mdio_dev)
{
    rcm_mdio_priv *priv = dev_get_priv(mdio_dev);

	dev_dbg(mdio_dev, DBGPREFIX "reset called\n");

	/* put to reset state */
	iowrite32(0 << ETH_RST_N, &priv->regs->eth_rst_n);

	/* remove reset state */
	iowrite32(1 << ETH_RST_N, &priv->regs->eth_rst_n);

	/* enable MDIO */
	iowrite32(1 << MDC_EN, &priv->regs->mdio_en);

    return 0;
}

static int rcm_mdio_probe(struct udevice *dev)
{
	rcm_mdio_priv *priv = dev_get_priv(dev);
	rcm_mdio_regs* regs = (rcm_mdio_regs*) dev_read_addr(dev);

	dev_dbg(dev, DBGPREFIX "probe called\n");

	if(!regs)
	{
		dev_err(dev, DBGPREFIX "unable to get address of mdio core\n");
        return -EINVAL;
	}

    if(ioread32(&regs->id) != MDIO_ID || ioread32(&regs->version) != MDIO_VER)
    {
        dev_err(dev, DBGPREFIX "detected illegal version of MDIO core: 0x%08x 0x%08x\n",
            ioread32(&regs->id), 
            ioread32(&regs->version));
        return -ENOTSUPP;
    }

	priv->regs = regs;

	/* disable all interrupts */
	iowrite32(0, &regs->irq_mask); 	

	return rcm_mdio_reset(dev);
}

static int rcm_mdio_bind(struct udevice *dev)
{
	dev_dbg(dev, DBGPREFIX "bind called\n");

	if (ofnode_valid(dev->node))
		device_set_name(dev, ofnode_get_name(dev->node));

	return 0;
}

static int rcm_mdio_read(struct udevice *mdio_dev, int addr, int devad, int reg)
{
    rcm_mdio_priv *priv = dev_get_priv(mdio_dev);

	dev_dbg(mdio_dev, DBGPREFIX "read called for %d:%d:%d\n", addr, devad, reg);

	if (devad != MDIO_DEVAD_NONE)
		return -EOPNOTSUPP;

    iowrite32(reg << ADDR_REG | addr << ADDR_PHY | 1 << START_RD, &priv->regs->mg_control);

	int ret = wait_for_bit_le32((void*)&priv->regs->mg_control, BIT(BUSY), true, CONFIG_SYS_HZ, false);
	if (ret < 0)
		return ret;

    return (ioread32(&priv->regs->mg_control) >> CTRL_DATA)&0x0000FFFF;;
}

static int rcm_mdio_write(struct udevice *mdio_dev, int addr, int devad, int reg, u16 val)
{
    rcm_mdio_priv *priv = dev_get_priv(mdio_dev);

	dev_dbg(mdio_dev, DBGPREFIX "write called for %d:%d:%d <= %d\n", addr, devad, reg, (int) val);

	if (devad != MDIO_DEVAD_NONE)
		return -EOPNOTSUPP;

	// wait while busy
	int ret = wait_for_bit_le32((void*)&priv->regs->mg_control, BIT(BUSY), false, CONFIG_SYS_HZ, false);
	if (ret < 0)
		return ret;

    iowrite32(reg << ADDR_REG | addr << ADDR_PHY | 1 << START_WR | val << CTRL_DATA, &priv->regs->mg_control);

    return 0;
}

static const struct mdio_ops rcm_mdio_ops = {
	.read = rcm_mdio_read,
	.write = rcm_mdio_write,
    .reset = rcm_mdio_reset
};

static const struct udevice_id rcm_mdio_ids[] = {
	{ .compatible = "rcm,mdio" },
	{ }
};

U_BOOT_DRIVER(rcm_mdio) = {
	.name			= "rcm-mdio",
	.id			    = UCLASS_MDIO,
	.of_match		= rcm_mdio_ids,
	.bind			= rcm_mdio_bind,
	.probe			= rcm_mdio_probe,
	.ops			= &rcm_mdio_ops,
	.priv_auto_alloc_size	= sizeof(rcm_mdio_priv),
};

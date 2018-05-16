/*
 * MPW7705 MUSB "glue layer"
 * 
 * Copyright (C) 2018, AstroSoft.
 * Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * Based on the PIC32 "glue layer" code.
 */

#define DEBUG
#include <common.h>
#include <linux/usb/musb.h>
#include <linux/io.h>
#include "linux-compat.h"
#include "musb_core.h"
#include "musb_uboot.h"
#include "musb_regs.h"
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;


/* RC-Module controller data */
struct module_musb_data {
	struct musb_host_data mdata;
	struct device dev;
	void __iomem *musb_ctrl;
	void __iomem *musb_parent_ctrl;
};

#define to_module_musb_data(d)	\
	container_of(d, struct module_musb_data, dev)

static void module_musb_disable(struct musb *musb)
{
	/* no way to shut the controller */
}

#define PIC32_TX_EP_MASK	0x0f		/* EP0 + 7 Tx EPs */
#define PIC32_RX_EP_MASK	0x0e		/* 7 Rx EPs */

#define MUSB_SOFTRST		0x7f
#define  MUSB_SOFTRST_NRST	BIT(0)
#define  MUSB_SOFTRST_NRSTX	BIT(1)

#define USBCRCON		0
#define  USBCRCON_USBWKUPEN	BIT(0)  /* Enable Wakeup Interrupt */
#define  USBCRCON_USBRIE	BIT(1)  /* Enable Remote resume Interrupt */
#define  USBCRCON_USBIE		BIT(2)  /* Enable USB General interrupt */
#define  USBCRCON_SENDMONEN	BIT(3)  /* Enable Session End VBUS monitoring */
#define  USBCRCON_BSVALMONEN	BIT(4)  /* Enable B-Device VBUS monitoring */
#define  USBCRCON_ASVALMONEN	BIT(5)  /* Enable A-Device VBUS monitoring */
#define  USBCRCON_VBUSMONEN	BIT(6)  /* Enable VBUS monitoring */
#define  USBCRCON_PHYIDEN	BIT(7)  /* PHY ID monitoring enable */
#define  USBCRCON_USBIDVAL	BIT(8)  /* USB ID value */
#define  USBCRCON_USBIDOVEN	BIT(9)  /* USB ID override enable */
#define  USBCRCON_USBWK		BIT(24) /* USB Wakeup Status */
#define  USBCRCON_USBRF		BIT(25) /* USB Resume Status */
#define  USBCRCON_USBIF		BIT(26) /* USB General Interrupt Status */


static irqreturn_t module_interrupt(int irq, void *hci)
{
	debug("Interrupt %d\n", irq);
	struct musb  *musb = hci;
	irqreturn_t ret = IRQ_NONE;
	u32 epintr, usbintr;

	/* ack usb core interrupts */
	musb->int_usb = musb_readb(musb->mregs, MUSB_INTRUSB);
	if (musb->int_usb)
		musb_writeb(musb->mregs, MUSB_INTRUSB, musb->int_usb);

	/* ack endpoint interrupts */
	musb->int_rx = musb_readw(musb->mregs, MUSB_INTRRX) & PIC32_RX_EP_MASK;
	if (musb->int_rx)
		musb_writew(musb->mregs, MUSB_INTRRX, musb->int_rx);

	musb->int_tx = musb_readw(musb->mregs, MUSB_INTRTX) & PIC32_TX_EP_MASK;
	if (musb->int_tx)
		musb_writew(musb->mregs, MUSB_INTRTX, musb->int_tx);

	/* drop spurious RX and TX if device is disconnected */
	if (musb->int_usb & MUSB_INTR_DISCONNECT) {
		musb->int_tx = 0;
		musb->int_rx = 0;
	}

	if (musb->int_tx || musb->int_rx || musb->int_usb)
		ret = musb_interrupt(musb);

	return ret;
}


#define MODULE_REGLOAD(addr )readl(addr)
#define MODULE_REGSAVE(addr,data) writel(data,addr)
#define MODULE_REGORIN(addr,data) MODULE_REGSAVE(addr,MODULE_REGLOAD(addr)|data)
#define MODULE_REGANDIN(addr,data) MODULE_REGSAVE(addr,MODULE_REGLOAD(addr)&data)


#define MODULE_CONTROL_SCTL_OFFSET 		0
#define MODULE_CONTROL_SYS_CONF0_OFFSET 8
#define MODULE_CONTROL_USBPHY_CFG_OFFSET 0xa000

#define MODULE_USB0_RESET_REG_OFFSET 0x2c



static int module_musb_set_mode(struct musb *musb, u8 mode)
{
	struct device *dev = musb->controller;
	struct module_musb_data *pdata = to_module_musb_data(dev);

	debug("module_musb_set_mode: %d\n", (int) mode);

	return 0;

	switch (mode) {
	case MUSB_HOST:
//		clrsetbits_le32(pdata->musb_ctrl + MODULE_CONTROL_USBPHY_CFG_OFFSET,
//				USBCRCON_USBIDVAL, USBCRCON_USBIDOVEN);
		break;
	case MUSB_PERIPHERAL:
//		setbits_le32(pdata->musb_ctrl + USBCRCON,
//			     USBCRCON_USBIDVAL | USBCRCON_USBIDOVEN);
		break;
	case MUSB_OTG:
		dev_err(dev, "support for OTG is unimplemented\n");
		break;
	default:
		dev_err(dev, "unsupported mode %d\n", mode);
		return -EINVAL;
	}

	return 0;
}


typedef enum
{
    POR_reset_bit_num = 0,
    utmi_reset_phy_bit_num = 1,
    utmi_reset_musb_bit_num = 2,
    utmi_suspendm_en_bit_num = 3
}usb0_reset_reg_bit_numbers;


static int module_musb_reset(struct musb *musb)
{
	struct module_musb_data *pdata = to_module_musb_data(musb->controller);

	void __iomem *control = pdata->musb_parent_ctrl;

    debug("USB reset sequence begin\n");
	writeb((unsigned char) (1 << POR_reset_bit_num) | (1 << utmi_reset_phy_bit_num) | (0 << utmi_suspendm_en_bit_num) | (0 << utmi_reset_musb_bit_num), 
				control+MODULE_USB0_RESET_REG_OFFSET);

    //T1 - POR LOW
    writeb((unsigned char) (0 << POR_reset_bit_num) | (1 << utmi_reset_phy_bit_num) | (0 << utmi_suspendm_en_bit_num) | (0 << utmi_reset_musb_bit_num),
				control+MODULE_USB0_RESET_REG_OFFSET);
    usleep(10);

    //T2 - SUSPENDM HIGH
    writeb((unsigned char) (0 << POR_reset_bit_num) | (1 << utmi_reset_phy_bit_num) | (1 << utmi_suspendm_en_bit_num) | (0 << utmi_reset_musb_bit_num),
				control+MODULE_USB0_RESET_REG_OFFSET);
    usleep(47);

    //T3 T4 UTMI_RESET LOW
    writeb((unsigned char) (0 << POR_reset_bit_num) | (0 << utmi_reset_phy_bit_num) | (1 << utmi_suspendm_en_bit_num) | (0 << utmi_reset_musb_bit_num),
				control+MODULE_USB0_RESET_REG_OFFSET);

    //T5 - Release reset USB controller
    writeb((unsigned char) (0 << POR_reset_bit_num) | (0 << utmi_reset_phy_bit_num) | (1 << utmi_suspendm_en_bit_num) | (1 << utmi_reset_musb_bit_num),
				control+MODULE_USB0_RESET_REG_OFFSET);
    debug("USB reset sequence done\n");

	return 0;
}


static int module_musb_enable(struct musb *musb)
{
	/* soft reset by NRSTx */
	musb_writeb(musb->mregs, MUSB_SOFTRST, MUSB_SOFTRST_NRSTX);

//	musb_ep_select(musb->mregs, 0);
//	musb_writeb(musb->mregs, MUSB_FADDR, 0);

	/* set mode */
	musb_platform_set_mode(musb, musb->board_mode);

	return 0;
}

static int module_musb_init(struct musb *musb)
{
	struct module_musb_data *pdata = to_module_musb_data(musb->controller);
	u32 ctrl, hwvers;
	u8 power;

	void __iomem *control = pdata->musb_ctrl;

    //Initialize the PHY
	
	// select clock to 24mhz
	// set bit 10 - USBPHY_REFCLK_SEL
	MODULE_REGORIN(control+MODULE_CONTROL_SCTL_OFFSET+MODULE_CONTROL_SYS_CONF0_OFFSET, 1 << 10);

	module_musb_reset(musb);

	/* Returns zero if not clocked */
	hwvers = musb_read_hwvers(musb->mregs);
	if (!hwvers)
		return -ENODEV;

	debug("module_musb_init: hwvers: %x\n", hwvers);

			
	/* Reset the musb */
	power = musb_readb(musb->mregs, MUSB_POWER);
	power = power | MUSB_POWER_RESET;
	musb_writeb(musb->mregs, MUSB_POWER, power);
	mdelay(100);

	/* Start the on-chip PHY and its PLL. */

	power = power & ~MUSB_POWER_RESET;
	musb_writeb(musb->mregs, MUSB_POWER, power);

	musb->isr = module_interrupt;

//	ctrl =  USBCRCON_USBIF | USBCRCON_USBRF |
//		USBCRCON_USBWK | USBCRCON_USBIDOVEN |
//		USBCRCON_PHYIDEN | USBCRCON_USBIE |
//		USBCRCON_USBRIE | USBCRCON_USBWKUPEN |
//		USBCRCON_VBUSMONEN;
//	writel(ctrl, pdata->musb_ctrl + USBCRCON);

	return 0;
}

const struct musb_platform_ops module_musb_ops = {
	.init		= module_musb_init,
	.set_mode	= module_musb_set_mode,
	.disable	= module_musb_disable,
	.enable		= module_musb_enable,
};

/* Module default FIFO config - fits in 8KB */
static struct musb_fifo_cfg module_musb_fifo_config[] = {
	{ .hw_ep_num = 1, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 1, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 2, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 2, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 3, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 3, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 4, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 4, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 5, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 5, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 6, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 6, .style = FIFO_RX, .maxpacket = 512, },
	{ .hw_ep_num = 7, .style = FIFO_TX, .maxpacket = 512, },
	{ .hw_ep_num = 7, .style = FIFO_RX, .maxpacket = 512, },
};

static struct musb_hdrc_config module_musb_config = {
	.fifo_cfg	= module_musb_fifo_config,
	.fifo_cfg_size	= ARRAY_SIZE(module_musb_fifo_config),
	.multipoint     = 1,
	.dyn_fifo       = 1,
	.num_eps        = 16,
	.ram_bits       = 12,
};

/* MPW7705 has one MUSB controller which can be host or gadget */
static struct musb_hdrc_platform_data module_musb_plat = {
	.mode           = MUSB_HOST,
	.config         = &module_musb_config,
	.power          = 250,		/* 500mA */
	.platform_ops	= &module_musb_ops,
};

static int musb_usb_probe(struct udevice *dev)
{
	struct usb_bus_priv *priv = dev_get_uclass_priv(dev);
	struct module_musb_data *pdata = dev_get_priv(dev);
	struct musb_host_data *mdata = &pdata->mdata;
	struct fdt_resource mc, ctrl, parentctrl;
	void *fdt = (void *)gd->fdt_blob;
	int node = dev_of_offset(dev);
	void __iomem *mregs;
	int ret;

	priv->desc_before_addr = true;

	ret = fdt_get_named_resource(fdt, node, "reg", "reg-names",
				     "mc", &mc);
	if (ret < 0) {
		printf("module-musb: resource \"mc\" not found\n");
		return ret;
	}

	ret = fdt_get_named_resource(fdt, node, "reg", "reg-names",
				     "control", &ctrl);
	if (ret < 0) {
		printf("module-musb: resource \"control\" not found\n");
		return ret;
	}

	mregs = devm_ioremap(dev, mc.start, fdt_resource_size(&mc));
	pdata->musb_ctrl = devm_ioremap(dev, ctrl.start, fdt_resource_size(&ctrl));

	// get bus device and its conrol reg
	struct udevice *parent = dev_get_parent(dev);
	node = dev_of_offset(parent);

	ret = fdt_get_named_resource(fdt, node, "reg", "reg-names",
				     "ctrlreg", &parentctrl);
	if (ret < 0) {
		printf("module-musb: unable to find parent resource \"ctrlreg\" %d\n", ret);
		return ret;
	}
	pdata->musb_parent_ctrl = devm_ioremap(dev, parentctrl.start, fdt_resource_size(&parentctrl));
	debug("musb_usb_probe: mc = 0x%x, ctrl = 0x%x, parentctrl = 0x%x\n", (unsigned int)mregs, (unsigned int)pdata->musb_ctrl, (unsigned int)pdata->musb_parent_ctrl);

	/* init controller */
#ifdef CONFIG_USB_MUSB_HOST
	mdata->host = musb_init_controller(&module_musb_plat,
					   &pdata->dev, mregs);
	if (!mdata->host)
		return -EIO;

	ret = musb_lowlevel_init(mdata);
#else
	module_musb_plat.mode = MUSB_PERIPHERAL;
	ret = musb_register(&module_musb_plat, &pdata->dev, mregs);
#endif
	if (ret == 0)
		printf("MODULE MUSB OTG\n");

	return ret;
}

static int musb_usb_remove(struct udevice *dev)
{
	struct module_musb_data *pdata = dev_get_priv(dev);

	debug("musb_usb_remove\n");

	musb_stop(pdata->mdata.host);

	return 0;
}

static const struct udevice_id module_musb_ids[] = {
	{ .compatible = "rc-module,musbmhdrc" },
	{ }
};

U_BOOT_DRIVER(usb_musb) = {
	.name		= "module-musb",
	.id		= UCLASS_USB,
	.of_match	= module_musb_ids,
	.probe		= musb_usb_probe,
	.remove		= musb_usb_remove,
#ifdef CONFIG_USB_MUSB_HOST
	.ops		= &musb_usb_ops,
#endif
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct module_musb_data),
};

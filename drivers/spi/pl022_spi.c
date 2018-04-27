/*
 * PL022  SPI driver
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define DEBUG 1

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <spi.h>
#include <asm/io.h>
#include <asm/gpio.h>
//#include <asm/hardware.h>

#define MAX_CS_COUNT    4


/* SSP registers mapping */
#define SSP_CR0		0x000
#define SSP_CR1		0x004
#define SSP_DR		0x008
#define SSP_SR		0x00C
#define SSP_CPSR	0x010
#define SSP_IMSC	0x014
#define SSP_RIS		0x018
#define SSP_MIS		0x01C
#define SSP_ICR		0x020
#define SSP_DMACR	0x024
#define SSP_ITCR	0x080
#define SSP_ITIP	0x084
#define SSP_ITOP	0x088
#define SSP_TDR		0x08C

#define SSP_PID0	0xFE0
#define SSP_PID1	0xFE4
#define SSP_PID2	0xFE8
#define SSP_PID3	0xFEC

#define SSP_CID0	0xFF0
#define SSP_CID1	0xFF4
#define SSP_CID2	0xFF8
#define SSP_CID3	0xFFC

/* SSP Control Register 0  - SSP_CR0 */
#define SSP_CR0_SPO		(0x1 << 6)
#define SSP_CR0_SPH		(0x1 << 7)
#define SSP_CR0_8BIT_MODE	(0x07)
#define SSP_SCR_MAX		(0xFF)
#define SSP_SCR_SHFT		8

/* SSP Control Register 0  - SSP_CR1 */
#define SSP_CR1_MASK_SSE	(0x1 << 1)

#define SSP_CPSR_MAX		(0xFE)

/* SSP Status Register - SSP_SR */
#define SSP_SR_MASK_TFE		(0x1 << 0) /* Transmit FIFO empty */
#define SSP_SR_MASK_TNF		(0x1 << 1) /* Transmit FIFO not full */
#define SSP_SR_MASK_RNE		(0x1 << 2) /* Receive FIFO not empty */
#define SSP_SR_MASK_RFF		(0x1 << 3) /* Receive FIFO full */
#define SSP_SR_MASK_BSY		(0x1 << 4) /* Busy Flag */

/* SSP Interrupt mask register */
#define SSP_IMSC_TXIM_n      3
#define SSP_IMSC_RXIM_n      2
#define SSP_IMSC_RTIM_n      1
#define SSP_IMSC_RORIM_n     0

/* SSP interrupt reset register */
#define SSP_ICR_RTIC_n       1
#define SSP_ICR_RORIC_n      0

/* SSP DMA control register */
#define SSP_DMACR_TXDMAE_n   1
#define SSP_DMACR_RXDMAE_n   0

/* SSP CR1 control register */
#define SSP_CR1_SOD_n        3
#define SSP_CR1_MS_n         2
#define SSP_CR1_SSE_n        1
#define SSP_CR1_LBM_n        0


struct pl022_spi_platdata
{
	void *regs;
	unsigned int freq;
};

struct pl022_spi_priv
{
	void *regs;
	unsigned int mode;
	unsigned int speed;
	unsigned int freq;
	struct gpio_desc cs_gpios[MAX_CS_COUNT];
};

static int pl022_spi_ofdata_to_platdata(struct udevice *bus)
{
	struct pl022_spi_platdata *plat = bus->platdata;
	const void *blob = gd->fdt_blob;
	int node = dev_of_offset(bus);

	plat->regs = (void*)devfdt_get_addr(bus);

	/* FIXME: Use 250MHz as a suitable default */
	plat->freq = fdtdec_get_int(blob, node, "spi-max-frequency",
					100000000);

	debug("%s: regs=%p max-frequency=%d\n", __func__,
	      plat->regs, plat->freq);

	return 0;
}

static int pl022_spi_claim_bus(struct udevice *dev)
{
	debug("pl022_spi_claim_bus\n");

	struct udevice *bus = dev->parent;
	struct pl022_spi_priv *priv = dev_get_priv(bus);
	void *regs = priv->regs;

	/* Enable the SPI hardware */
	writew(readw(regs + SSP_CR1) | SSP_CR1_MASK_SSE,
			regs + SSP_CR1);

	return 0;
}

static int pl022_spi_release_bus(struct udevice *dev)
{
	debug("pl022_spi_release_bus\n");
	struct udevice *bus = dev->parent;
	struct pl022_spi_priv *priv = dev_get_priv(bus);
	void *regs = priv->regs;

	/* Disable the SPI hardware */
	writew(0x0, regs + SSP_CR1);

	return 0;
}

// ASTRO TODO: move to .h file
//void spi_cs_activate(struct spi_slave *slave);
//void spi_cs_deactivate(struct spi_slave *slave);

static void spi_cs_activate(struct udevice *dev, uint cs)
{
	struct udevice *bus = dev_get_parent(dev);
	struct pl022_spi_priv *priv = dev_get_priv(bus);

	if (!dm_gpio_is_valid(&priv->cs_gpios[cs]))
			return;

	dm_gpio_set_value(&priv->cs_gpios[cs], 0);

}

static void spi_cs_deactivate(struct udevice *dev, uint cs)
{
	struct udevice *bus = dev_get_parent(dev);
	struct pl022_spi_priv *priv = dev_get_priv(bus);

	if (!dm_gpio_is_valid(&priv->cs_gpios[cs]))
			return;

	dm_gpio_set_value(&priv->cs_gpios[cs], 1);
}


static int pl022_spi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	debug("pl022_spi_xfer %d, %d, 0x%x\n", bitlen, (int)flags, (int)dout);

	struct udevice *bus = dev->parent;
	struct pl022_spi_priv *priv = dev_get_priv(bus);

	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);	

	u8 *regs = priv->regs;

	//struct spi_slave slave;

	// ASTRO TODO: choose cs
	// slave.cs = 0;

	u32		len_tx = 0, len_rx = 0, len;
	u32		ret = 0;
	const u8	*txp = dout;
	u8		*rxp = din, value;

	if (bitlen == 0)
		goto out;

	if (bitlen % 8) {
		ret = -1;
		flags |= SPI_XFER_END;
		goto out;
	}

	len = bitlen / 8;

	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(dev, slave_plat->cs);

	debug("alive. len_tx = %d, len = %d\n", len_tx, len);

	u16 val = -1;

	while (len_tx < len) {
		//debug("alive1. len_tx = %d, len = %d\n", len_tx, len);
		if (readw(regs + SSP_SR) & SSP_SR_MASK_TNF) {
			value = (txp != NULL) ? *txp++ : 0;
			// debug("SPI write: %x\n", value);
			writew(value, regs + SSP_DR);
			len_tx++;
		} else {
			u16 oldval = val;
			val = readw(regs + SSP_SR);
			if(val != oldval)
				debug("Wait with SR: %x\n", (int)val);
		}

		if (readw(regs + SSP_SR) & SSP_SR_MASK_RNE) {
			value = readw(regs + SSP_DR);
			// debug("SPI read: %x\n", value);
			if (rxp)
				*rxp++ = value;
			len_rx++;
		}
	}

	// finish reading
	while (len_rx < len_tx) {
		if (readw(regs + SSP_SR) & SSP_SR_MASK_RNE) {
			value = readw(regs + SSP_DR);
			if (rxp)
			{
				*rxp++ = value;
				//debug("SPI read: %x\n", value);
			}
			len_rx++;
		}
	}

	debug("alive\n");

out:
	if (flags & SPI_XFER_END)
		spi_cs_deactivate(dev, slave_plat->cs);

	debug("Exit from pl022_spi_xfer!\n");

	return ret;
}

static int pl022_spi_set_speed(struct udevice *bus, uint speed)
{
 
	u16 scr = 1, prescaler, cr0 = 0, cpsr = 0;

	struct pl022_spi_priv *priv = dev_get_priv(bus);
	struct pl022_spi_platdata *plat = dev_get_platdata(bus);
	void *regs = priv->regs;

	prescaler = CONFIG_SYS_SPI_CLK / plat->freq;

   debug("gonna pl022_spi_set_speed, prescaler=%d %d\n", (int) prescaler, plat->freq);

	if (prescaler <= 0xFF)
		cpsr = prescaler;
	else {
		for (scr = 1; scr <= SSP_SCR_MAX; scr++) {
			if (!(prescaler % scr)) {
				cpsr = prescaler / scr;
				if (cpsr <= SSP_CPSR_MAX)
					break;
			}
		}

		if (scr > SSP_SCR_MAX) {
			scr = SSP_SCR_MAX;
			cpsr = prescaler / scr;
			cpsr &= SSP_CPSR_MAX;
		}
	}

	if (cpsr & 0x1)
		cpsr++;

	writew(cpsr, regs + SSP_CPSR);
	cr0 = readw(regs + SSP_CR0);
	writew(cr0 | (scr - 1) << SSP_SCR_SHFT, regs + SSP_CR0);

	return 0;
}

static int pl022_spi_set_mode(struct udevice *bus, uint mode)
{
    debug("gonna pl022_spi_set_mode %02X\n", mode);

	struct pl022_spi_priv *priv = dev_get_priv(bus);
	void *regs = priv->regs;

	/* Set requested polarity and 8bit mode */
	u16 cr0 = SSP_CR0_8BIT_MODE;
	cr0 |= (mode & SPI_CPHA) ? SSP_CR0_SPH : 0;
	cr0 |= (mode & SPI_CPOL) ? SSP_CR0_SPO : 0;

	writew(cr0, regs + SSP_CR0);
	return 0;
}

// ASTRO TODO: move to .h file
int pl022_spi_init(void);

static int pl022_spi_probe(struct udevice *bus)
{
	debug("pl022_spi_probe\n");
    struct pl022_spi_platdata *plat = dev_get_platdata(bus);
	struct pl022_spi_priv *priv = dev_get_priv(bus);
	int i;

	priv->regs = plat->regs;

	int ret = gpio_request_list_by_name(bus, "cs-gpios", priv->cs_gpios,
									ARRAY_SIZE(priv->cs_gpios), 0);
	if (ret < 0) {
			pr_err("Can't get %s gpios! Error: %d\n", bus->name, ret);
			return ret;
	}

	for(i = 0; i < ARRAY_SIZE(priv->cs_gpios); i++) {
			if (!dm_gpio_is_valid(&priv->cs_gpios[i]))
					continue;

			dm_gpio_set_dir_flags(&priv->cs_gpios[i],
									GPIOD_IS_OUT | GPIOD_IS_OUT_ACTIVE);

			// and disable
			dm_gpio_set_value(&priv->cs_gpios[i], 1);
	}


	debug("Priv regs: %08x\n", (int) priv->regs);
	debug("SPI ver: %02x %02x %02x %02x\n", readb(priv->regs + SSP_PID0), readb(priv->regs + SSP_PID1), readb(priv->regs + SSP_PID2), readb(priv->regs + SSP_PID3));
	/* PL022 version is 0x00041022 */
	if (! ((readb(priv->regs + SSP_PID0) == 0x22) &&
		(readb(priv->regs + SSP_PID1) == 0x10) &&
		((readb(priv->regs + SSP_PID2) & 0xf) == 0x04) &&
		(readb(priv->regs + SSP_PID3) == 0x00)))
	{
		debug("SPI ver is NOT correct\n");
		return -EINVAL;
	}

	debug("SPI ver is correct\n");

	// disable interrupts
	u16 imsc = (0b0 << SSP_IMSC_TXIM_n)
            | (0b0 << SSP_IMSC_RXIM_n)
            | (0b0 << SSP_IMSC_RTIM_n)
            | (0b0 << SSP_IMSC_RORIM_n);
	
	writew(imsc, priv->regs + SSP_IMSC);

	// reset interrupts state
	u16 icr = (0b1 << SSP_ICR_RTIC_n)
            | (0b1 << SSP_ICR_RORIC_n);

	writew(icr, priv->regs + SSP_ICR);

	// disable DMA
	u16 dmacr = (0b0 << SSP_DMACR_TXDMAE_n)
            | (0b0 << SSP_DMACR_RXDMAE_n);

	writew(dmacr, priv->regs + SSP_DMACR);

	// enable SSP
	u16 cr1 = (0b1 << SSP_CR1_SOD_n)
            | (0b0 << SSP_CR1_MS_n)
            | (0b1 << SSP_CR1_SSE_n) 
            | (0b0 << SSP_CR1_LBM_n);

	writew(cr1, priv->regs + SSP_CR1);            

	return 0;
}

static const struct dm_spi_ops pl022_spi_ops = {
	.claim_bus   = pl022_spi_claim_bus,
	.release_bus = pl022_spi_release_bus,
	.xfer        = pl022_spi_xfer,
	.set_speed   = pl022_spi_set_speed,
	.set_mode    = pl022_spi_set_mode,
};

static const struct udevice_id pl022_spi_ids[] = {
	{ .compatible = "arm,pl022-spi" },
	{ .compatible = "rc-module,pl022-spi" },
	{}
};

U_BOOT_DRIVER(pl022_spi) = {
	.name                     = "pl022_spi",
	.id                       = UCLASS_SPI,
	.of_match                 = pl022_spi_ids,
	.ops                      = &pl022_spi_ops,
	.ofdata_to_platdata       = pl022_spi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct pl022_spi_platdata),
	.priv_auto_alloc_size     = sizeof(struct pl022_spi_priv),
	.probe                    = pl022_spi_probe,
};
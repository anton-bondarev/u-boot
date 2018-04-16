/*
 * (C) Copyright 2012
 * Armando Visconti, ST Microelectronics, armando.visconti@st.com.
 *
 * Driver for ARM PL022 SPI Controller. Based on atmel_spi.c
 * by Atmel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define DEBUG 1

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <spi.h>
#include <asm/io.h>
//#include <asm/hardware.h>

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
	u8 cs;
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
void spi_cs_activate(struct spi_slave *slave);
void spi_cs_deactivate(struct spi_slave *slave);

static int pl022_spi_xfer(struct udevice *dev, unsigned int bitlen,
		const void *dout, void *din, unsigned long flags)
{
	debug("pl022_spi_xfer %d, %u\n", bitlen, flags);

	struct udevice *bus = dev->parent;
	struct pl022_spi_priv *priv = dev_get_priv(bus);
	void *regs = priv->regs;

	struct spi_slave slave;

	// ASTRO TODO: choose cs
	// slave.cs = 0;

	u32		len_tx = 0, len_rx = 0, len;
	u32		ret = 0;
	const u8	*txp = dout;
	u8		*rxp = din, value;

	if (bitlen == 0)
		/* Finish any previously submitted transfers */
		goto out;

	/*
	 * TODO: The controller can do non-multiple-of-8 bit
	 * transfers, but this driver currently doesn't support it.
	 *
	 * It's also not clear how such transfers are supposed to be
	 * represented as a stream of bytes...this is a limitation of
	 * the current SPI interface.
	 */
	if (bitlen % 8) {
		ret = -1;

		/* Errors always terminate an ongoing transfer */
		flags |= SPI_XFER_END;
		goto out;
	}

	len = bitlen / 8;

	if (flags & SPI_XFER_BEGIN)
		spi_cs_activate(&slave);

	debug("alive. len_tx = %d, len = %d\n", len_tx, len);

	while (len_tx < len) {
		//debug("alive1. len_tx = %d, len = %d\n", len_tx, len);
		if (readw(regs + SSP_SR) & SSP_SR_MASK_TNF) {
			value = (txp != NULL) ? *txp++ : 0;
			debug("SPI write: %02x\n", value);
			writew(value, regs + SSP_DR);
			len_tx++;
		} /*else {
			debug("SR: %04x\n", readw(regs + SSP_SR) & SSP_SR_MASK_TNF);
		}*/

		//debug("alive2. len_tx = %d, len = %d\n", len_tx, len);

		if (readw(regs + SSP_SR) & SSP_SR_MASK_RNE) {
			value = readw(regs + SSP_DR);
			debug("SPI read: %02x\n", value);
			if (rxp)
				*rxp++ = value;
			len_rx++;
		}

		//debug("alive3. len_tx = %d, len = %d\n", len_tx, len);
	}

	while (len_rx < len_tx) {
		if (readw(regs + SSP_SR) & SSP_SR_MASK_RNE) {
			value = readw(regs + SSP_DR);
			if (rxp)
				*rxp++ = value;
			len_rx++;
		}
	}

	debug("alive\n");

out:
	if (flags & SPI_XFER_END)
		spi_cs_deactivate(&slave);

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

	priv->regs = plat->regs;

	// ASTRO TODO: choose cs
	//struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);
	if(!spi_cs_is_valid(0, 0)) {
		debug("SPI params are not correct\n");
		return 1;
	}

	pl022_spi_init();

	debug("Priv regs: %08x\n", priv->regs);
	debug("SPI ver: %02x %02x %02x %02x\n", readb(priv->regs + SSP_PID0), readb(priv->regs + SSP_PID1), readb(priv->regs + SSP_PID2), readb(priv->regs + SSP_PID3));
	/* PL022 version is 0x00041022 */
	if ((readb(priv->regs + SSP_PID0) == 0x61) &&
		(readb(priv->regs + SSP_PID1) == 0x10) &&
		((readb(priv->regs + SSP_PID2) & 0xf) == 0x04) &&
		(readb(priv->regs + SSP_PID3) == 0x00))
	{
		debug("SPI ver is correct\n");
		return 0;
	} else {
		debug("SPI ver is NOT correct\n");
	}

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
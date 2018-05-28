/*
 * generic mmc spi driver adapted to mpw7705
 *
 * Copyright (C) 2010 Thomas Chou <thomas@wytron.com.tw>
 * Licensed under the GPL-2 or later.
 */

#define DEBUG

#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <part.h>
#include <mmc.h>
#include <spi.h>
#include <crc.h>
#include <linux/crc7.h>
#include <asm/byteorder.h>
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <linux/libfdt.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/gpio.h>

struct mpw7705_mmc_platdata {
	struct mmc_config cfg;
	struct mmc mmc;
	uint32_t regbase;
	struct spi_slave *slave;
	struct gpio_desc card_detect;
};


#define MODULE_SPISDIO_ENABLE_OFFSET           0x300

/* MMC/SD in SPI mode reports R1 status always */
#define R1_SPI_IDLE		(1 << 0)
#define R1_SPI_ERASE_RESET	(1 << 1)
#define R1_SPI_ILLEGAL_COMMAND	(1 << 2)
#define R1_SPI_COM_CRC		(1 << 3)
#define R1_SPI_ERASE_SEQ	(1 << 4)
#define R1_SPI_ADDRESS		(1 << 5)
#define R1_SPI_PARAMETER	(1 << 6)
/* R1 bit 7 is always zero, reuse this bit for error */
#define R1_SPI_ERROR		(1 << 7)

/* Response tokens used to ack each block written: */
#define SPI_MMC_RESPONSE_CODE(x)	((x) & 0x1f)
#define SPI_RESPONSE_ACCEPTED		((2 << 1)|1)
#define SPI_RESPONSE_CRC_ERR		((5 << 1)|1)
#define SPI_RESPONSE_WRITE_ERR		((6 << 1)|1)

/* Read and write blocks start with these tokens and end with crc;
 * on error, read tokens act like a subset of R2_SPI_* values.
 */
#define SPI_TOKEN_SINGLE	0xfe	/* single block r/w, multiblock read */
#define SPI_TOKEN_MULTI_WRITE	0xfc	/* multiblock write */
#define SPI_TOKEN_STOP_TRAN	0xfd	/* terminate multiblock write */

/* MMC SPI commands start with a start bit "0" and a transmit bit "1" */
#define MMC_SPI_CMD(x) (0x40 | (x & 0x3f))

/* bus capability */
#define MMC_SPI_VOLTAGE (MMC_VDD_32_33 | MMC_VDD_33_34)
#define MMC_SPI_MIN_CLOCK 400000 /* 400KHz to meet MMC spec */

/* timeout value */
#define CTOUT 8
#define RTOUT 3000000 /* 1 sec */
#define WTOUT 3000000 /* 1 sec */

static uint mmc_spi_sendcmd(struct udevice* dev, ushort cmdidx, u32 cmdarg)
{
	//struct spi_slave *spi = mmc->priv;
	u8 cmdo[7];
	u8 r1;
	int i;
	cmdo[0] = 0xff;
	cmdo[1] = MMC_SPI_CMD(cmdidx);
	cmdo[2] = cmdarg >> 24;
	cmdo[3] = cmdarg >> 16;
	cmdo[4] = cmdarg >> 8;
	cmdo[5] = cmdarg;
	cmdo[6] = (crc7(0, &cmdo[1], 5) << 1) | 0x01;
	dm_spi_xfer(dev, sizeof(cmdo) * 8, cmdo, NULL, SPI_XFER_BEGIN);
	for (i = 0; i < CTOUT; i++) {
		dm_spi_xfer(dev, 1 * 8, NULL, &r1, 0);
		if (i && (r1 & 0x80) == 0) /* r1 response */
			break;
	}
	debug("%s:cmd%d resp%d %x\n", __func__, cmdidx, i, r1);
	return r1;
}

static uint mmc_spi_readdata(struct udevice* dev, void *xbuf,
				u32 bcnt, u32 bsize)
{
	//struct spi_slave *spi = mmc->priv;
	u8 *buf = xbuf;
	u8 r1;
	u16 crc;
	int i;
	while (bcnt--) {
		for (i = 0; i < RTOUT; i++) {
			dm_spi_xfer(dev, 1 * 8, NULL, &r1, 0);
			if (r1 != 0xff) /* data token */
				break;
		}
		debug("%s:tok%d %x\n", __func__, i, r1);
		if (r1 == SPI_TOKEN_SINGLE) {
			dm_spi_xfer(dev, bsize * 8, NULL, buf, 0);
			dm_spi_xfer(dev, 2 * 8, NULL, &crc, 0);
#ifdef CONFIG_MMC_SPI_CRC_ON
			if (be_to_cpu16(crc16_ccitt(0, buf, bsize)) != crc) {
				debug("%s: CRC error\n", mmc->cfg->name);
				r1 = R1_SPI_COM_CRC;
				break;
			}
#endif
			r1 = 0;
		} else {
			r1 = R1_SPI_ERROR;
			break;
		}
		buf += bsize;
	}
	return r1;
}

static uint mmc_spi_writedata(struct udevice* dev, const void *xbuf,
			      u32 bcnt, u32 bsize, int multi)
{
	//struct spi_slave *spi = mmc->priv;
	const u8 *buf = xbuf;
	u8 r1;
	u16 crc;
	u8 tok[2];
	int i;
	tok[0] = 0xff;
	tok[1] = multi ? SPI_TOKEN_MULTI_WRITE : SPI_TOKEN_SINGLE;
	while (bcnt--) {
#ifdef CONFIG_MMC_SPI_CRC_ON
		crc = cpu_to_be16(crc16_ccitt(0, (u8 *)buf, bsize));
#endif
		dm_spi_xfer(dev, 2 * 8, tok, NULL, 0);
		dm_spi_xfer(dev, bsize * 8, buf, NULL, 0);
		dm_spi_xfer(dev, 2 * 8, &crc, NULL, 0);
		for (i = 0; i < CTOUT; i++) {
			dm_spi_xfer(dev, 1 * 8, NULL, &r1, 0);
			if ((r1 & 0x10) == 0) /* response token */
				break;
		}
		debug("%s:tok%d %x\n", __func__, i, r1);
		if (SPI_MMC_RESPONSE_CODE(r1) == SPI_RESPONSE_ACCEPTED) {
			for (i = 0; i < WTOUT; i++) { /* wait busy */
				dm_spi_xfer(dev, 1 * 8, NULL, &r1, 0);
				if (i && r1 == 0xff) {
					r1 = 0;
					break;
				}
			}
			if (i == WTOUT) {
				debug("%s:wtout %x\n", __func__, r1);
				r1 = R1_SPI_ERROR;
				break;
			}
		} else {
			debug("%s: err %x\n", __func__, r1);
			r1 = R1_SPI_COM_CRC;
			break;
		}
		buf += bsize;
	}
	if (multi && bcnt == -1) { /* stop multi write */
		tok[1] = SPI_TOKEN_STOP_TRAN;
		dm_spi_xfer(dev, 2 * 8, tok, NULL, 0);
		for (i = 0; i < WTOUT; i++) { /* wait busy */
			dm_spi_xfer(dev, 1 * 8, NULL, &r1, 0);
			if (i && r1 == 0xff) {
				r1 = 0;
				break;
			}
		}
		if (i == WTOUT) {
			debug("%s:wstop %x\n", __func__, r1);
			r1 = R1_SPI_ERROR;
		}
	}
	return r1;
}

static int mmc_spi_request(struct udevice* dev, struct mmc_cmd *cmd,
		struct mmc_data *data)
{
	//struct spi_slave *spi = mmc->priv;
	u8 r1;
	int i;
	int ret = 0;
	debug("%s:cmd%d %x %x\n", __func__,
	      cmd->cmdidx, cmd->resp_type, cmd->cmdarg);
	dm_spi_claim_bus(dev);
	//spi_cs_activate(spi);
	r1 = mmc_spi_sendcmd(dev, cmd->cmdidx, cmd->cmdarg);
	if (r1 == 0xff) { /* no response */
		ret = -ENOMEDIUM;
		goto done;
	} else if (r1 & R1_SPI_COM_CRC) {
		ret = -ECOMM;
		goto done;
	} else if (r1 & ~R1_SPI_IDLE) { /* other errors */
		ret = -ETIMEDOUT;
		goto done;
	} else if (cmd->resp_type == MMC_RSP_R2) {
		r1 = mmc_spi_readdata(dev, cmd->response, 1, 16);
		for (i = 0; i < 4; i++)
			cmd->response[i] = be32_to_cpu(cmd->response[i]);
		debug("r128 %x %x %x %x\n", cmd->response[0], cmd->response[1],
		      cmd->response[2], cmd->response[3]);
	} else if (!data) {
		switch (cmd->cmdidx) {
		case SD_CMD_APP_SEND_OP_COND:
		case MMC_CMD_SEND_OP_COND:
			cmd->response[0] = (r1 & R1_SPI_IDLE) ? 0 : OCR_BUSY;
			break;
		case SD_CMD_SEND_IF_COND:
		case MMC_CMD_SPI_READ_OCR:
			dm_spi_xfer(dev, 4 * 8, NULL, cmd->response, 0);
			cmd->response[0] = be32_to_cpu(cmd->response[0]);
			debug("r32 %x\n", cmd->response[0]);
			break;
		case MMC_CMD_SEND_STATUS:
			dm_spi_xfer(dev, 1 * 8, NULL, cmd->response, 0);
			cmd->response[0] = (cmd->response[0] & 0xff) ?
				MMC_STATUS_ERROR : MMC_STATUS_RDY_FOR_DATA;
			break;
		}
	} else {
		debug("%s:data %x %x %x\n", __func__,
		      data->flags, data->blocks, data->blocksize);
		if (data->flags == MMC_DATA_READ)
			r1 = mmc_spi_readdata(dev, data->dest,
				data->blocks, data->blocksize);
		else if  (data->flags == MMC_DATA_WRITE)
			r1 = mmc_spi_writedata(dev, data->src,
				data->blocks, data->blocksize,
				(cmd->cmdidx == MMC_CMD_WRITE_MULTIPLE_BLOCK));
		if (r1 & R1_SPI_COM_CRC)
			ret = -ECOMM;
		else if (r1) /* other errors */
			ret = -ETIMEDOUT;
	}
done:
	dm_spi_xfer(dev, 0, 0, 0, 0);
	//spi_cs_deactivate(spi);
	dm_spi_release_bus(dev);
	return ret;
}

static int mmc_spi_set_ios(struct mmc *mmc)
{
//	struct spi_slave *spi = mmc->priv;

//	debug("%s: clock %u\n", __func__, mmc->clock);
//	if (mmc->clock)
//		spi_set_speed(spi, mmc->clock);
	return 0;
}

static int mpw7705_dm_mmc_set_ios(struct udevice *dev)
{
//	debug(">mpw7705_dm_mmc_set_ios\n");
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	return mmc_spi_set_ios(&pdata->mmc);
}

static bool check_sd_card_present(struct udevice *dev)
{
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);

	bool carddetected = dm_gpio_get_value(&pdata->card_detect) == 0;
	debug("check_sd_card_present: %d\n", carddetected);

	return carddetected;
}

static int mpw7705_mmc_probe(struct udevice *dev)
{
	debug(">mpw7705_mmc_probe\n");
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	struct mmc_uclass_priv * upriv = dev_get_uclass_priv(dev);
	struct mmc * mmc = & pdata->mmc;
	struct mmc_config * cfg = & pdata->cfg;

	cfg->voltages = MMC_VDD_33_34 | MMC_VDD_32_33 | MMC_VDD_31_32 | MMC_VDD_165_195;	
	cfg->host_caps = MMC_MODE_8BIT | MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	cfg->f_min = DIV_ROUND_UP(400000, 63);
	cfg->f_max = 52000000; 
	cfg->b_max = 511; /* max 512 - 1 blocks */
	cfg->name = dev->name;

	upriv->mmc = mmc;

	writel(0, pdata->regbase + MODULE_SPISDIO_ENABLE_OFFSET); // switch SDIO to GSPI

	pdata->slave = dev_get_parent_priv(dev);
	mmc->priv = pdata->slave;

	if(!pdata->slave)
	{
		debug("Unable to setup spi slave %x\n", (int) pdata->slave);
		return -EINVAL;
	}

	if (gpio_request_by_name(dev, "carddetect-gpio", 0, &pdata->card_detect,
				 GPIOD_IS_IN)) {
		dev_err(bus, "No cs-gpios property\n");
		return -EINVAL;
	}

	mmc_set_clock(mmc, cfg->f_min, false);
	
	if ( ! check_sd_card_present(dev) )
		return -EINVAL;

	//
	const unsigned char buf[16] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	dm_spi_claim_bus(dev);
	dm_spi_xfer(dev, 16 * 8, buf, NULL, SPI_XFER_BEGIN);
	dm_spi_release_bus(dev);

	return 0;
}

static int mpw7705_mmc_bind(struct udevice *dev)
{
	debug(">mpw7705_mmc_bind\n");
	
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);
	return mmc_bind(dev, & pdata->mmc, & pdata->cfg);
}


static int mpw7705_mmc_ofdata_to_platdata(struct udevice *dev)
{
	debug(">mpw7705_mmc_ofdata_to_platdata\n");
	struct mpw7705_mmc_platdata * pdata = dev_get_platdata(dev);

	fdt_addr_t sdio_addr = devfdt_get_addr(dev);

	if (sdio_addr == FDT_ADDR_T_NONE)
		return -EINVAL;

	pdata->regbase = sdio_addr;
	return 0;
}

static const struct udevice_id mpw7705_mmc_match[] = {
	{ .compatible = "rc-module,mmc-spi-0.2" },
	{ }
};

static const struct dm_mmc_ops mpw7705_dm_mmc_ops = {
	.send_cmd = mmc_spi_request,
	.set_ios = mpw7705_dm_mmc_set_ios,
};

U_BOOT_DRIVER(mpw7705_mmc_drv_spi) = {
	.name = "mpw7705_mmc_spi_drv",
	.id = UCLASS_MMC,
	.of_match = mpw7705_mmc_match,
	.ops = & mpw7705_dm_mmc_ops,
	.probe = mpw7705_mmc_probe,
	.bind = mpw7705_mmc_bind,
	.ofdata_to_platdata = mpw7705_mmc_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct mpw7705_mmc_platdata),
	.priv_auto_alloc_size = 1000
};



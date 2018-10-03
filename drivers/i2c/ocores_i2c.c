/*
 * OpenCores I2C driver
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * Based on Linux Kernel driver by 
 *   			 Peter Korsgaard <jacmet@sunsite.dk>
 * 
 * SPDX-License-Identifier:	GPL-2.0+
 */
#undef DEBUG
#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <i2c.h>

struct ocores_i2c {
	void __iomem *base;
	unsigned int reg_shift;
	unsigned int reg_io_width;
	wait_queue_head_t wait;
	struct i2c_msg *msg;
	int pos;
	int nmsgs;
	int state; /* see STATE_ */
	int ip_clock_khz;
	int bus_clock_khz;
	void (*setreg)(struct ocores_i2c *i2c, int reg, unsigned char value);
	unsigned char (*getreg)(struct ocores_i2c *i2c, int reg);
};

static inline u8 i2c_8bit_addr_from_msg(const struct i2c_msg *msg)
{
	return (msg->addr << 1) | (msg->flags & I2C_M_RD ? 1 : 0);
}

/* registers */
#define OCI2C_PRELOW		0
#define OCI2C_PREHIGH		1
#define OCI2C_CONTROL		2
#define OCI2C_DATA		3
#define OCI2C_CMD		4 /* write only */
#define OCI2C_STATUS		4 /* read only, same address as OCI2C_CMD */

#define OCI2C_CTRL_IEN		0x40
#define OCI2C_CTRL_EN		0x80

#define OCI2C_CMD_START		0x91
#define OCI2C_CMD_STOP		0x41
#define OCI2C_CMD_READ		0x21
#define OCI2C_CMD_WRITE		0x11
#define OCI2C_CMD_READ_ACK	0x21
#define OCI2C_CMD_READ_NACK	0x29
#define OCI2C_CMD_IACK		0x01

#define OCI2C_STAT_IF		0x01
#define OCI2C_STAT_TIP		0x02
#define OCI2C_STAT_ARBLOST	0x20
#define OCI2C_STAT_BUSY		0x40
#define OCI2C_STAT_NACK		0x80

#define STATE_DONE		0
#define STATE_START		1
#define STATE_WRITE		2
#define STATE_READ		3
#define STATE_ERROR		4

#define TYPE_OCORES		0
#define TYPE_GRLIB		1
#define TYPE_RCMODULE	2

static inline void oc_setreg(struct ocores_i2c *i2c, int reg, u8 value)
{
	i2c->setreg(i2c, reg, value);
}

static inline u8 oc_getreg(struct ocores_i2c *i2c, int reg)
{
	return i2c->getreg(i2c, reg);
}



static void ocores_process(struct ocores_i2c *i2c)
{
	struct i2c_msg *msg = i2c->msg;
	u8 stat = oc_getreg(i2c, OCI2C_STATUS);

	// if no int - return;
	if(stat & OCI2C_STAT_TIP)
		return;
	//debug("TIP %x\n", stat);

	if ((i2c->state == STATE_DONE) || (i2c->state == STATE_ERROR)) {
		/* stop has been sent */
		oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
		return;
	}

	/* error? */
	if (stat & OCI2C_STAT_ARBLOST) {
		i2c->state = STATE_ERROR;
		oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
		debug("error1 pos: %d\n", i2c->pos);
		return;
	}

	if ((i2c->state == STATE_START) || (i2c->state == STATE_WRITE)) {
		i2c->state =
			(msg->flags & I2C_M_RD) ? STATE_READ : STATE_WRITE;

		if (stat & OCI2C_STAT_NACK) {
			i2c->state = STATE_ERROR;
			oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
			debug("error2 pos: %d\n", i2c->pos);
			return;
		}
	} else
		msg->buf[i2c->pos++] = oc_getreg(i2c, OCI2C_DATA);

	/* end of msg? */
	if (i2c->pos == msg->len) {
		i2c->nmsgs--;
		i2c->msg++;
		i2c->pos = 0;
		msg = i2c->msg;

		if (i2c->nmsgs) {	/* end? */
			/* send start? */
			if (!(msg->flags & I2C_M_NOSTART)) {
				u8 addr = i2c_8bit_addr_from_msg(msg);

				i2c->state = STATE_START;

				oc_setreg(i2c, OCI2C_DATA, addr);
				oc_setreg(i2c, OCI2C_CMD,  OCI2C_CMD_START);
				return;
			} else
				i2c->state = (msg->flags & I2C_M_RD)
					? STATE_READ : STATE_WRITE;
		} else {
			i2c->state = STATE_DONE;
			oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_STOP);
			return;
		}
	}

	if (i2c->state == STATE_READ) {
		oc_setreg(i2c, OCI2C_CMD, i2c->pos == (msg->len-1) ?
			  OCI2C_CMD_READ_NACK : OCI2C_CMD_READ_ACK);
	} else {
		oc_setreg(i2c, OCI2C_DATA, msg->buf[i2c->pos++]);
		oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_WRITE);
	}
}


static int ocores_i2c_xfer(struct udevice *bus, struct i2c_msg *msg,
			  int nmsgs)
{
	struct ocores_i2c *i2c = dev_get_priv(bus);

	i2c->msg = msg;
	i2c->pos = 0;
	i2c->nmsgs = nmsgs;
	i2c->state = STATE_START;

#ifdef DEBUG
	{
		int i;
		debug("ocores_i2c_xfer: %d msgs:\n", nmsgs);
		for(i = 0 ; i < nmsgs; i++)
			debug("    %d addr, %d len, %x flags\n", msg[i].addr, msg[i].len, msg[i].flags);
	}
#endif		

	oc_setreg(i2c, OCI2C_DATA, i2c_8bit_addr_from_msg(i2c->msg));
	oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_START);

	while ((i2c->state != STATE_DONE) && (i2c->state != STATE_ERROR))
		ocores_process(i2c);		

	debug("ocores_i2c_xfer: done with state %d\n", i2c->state);

#ifdef DEBUG	
	if(i2c->state == STATE_DONE)
	{
		int i;
		for(i = 0 ; i < nmsgs; i++)
		{
			debug("message %d - %d bytes:\n", i, msg[i].len);
			unsigned char *p = msg[i].buf;
			int j=0;
			while(p < (msg[i].buf+msg[i].len))
			{
				debug("%02X ", *p++);
				if(++j == 16)
				{
					debug("\n");
					j = 0;
				}
			}
			debug("\n");
		}
	}
#endif

	return (i2c->state == STATE_DONE)?0:-EIO;
}

/* Read and write functions for the RC Module port of the controller. Registers are
 * 32-bit little endian and the PRELOW and PREHIGH registers are merged into one
 * register. The subsequent registers has their offset decreased accordingly. */

static u8 oc_getreg_rcmodule(struct ocores_i2c *i2c, int reg)
{
	u32 rd;
	int rreg = reg;
	if (reg != OCI2C_PRELOW)
		rreg--;
	rd = readl(i2c->base + (rreg << i2c->reg_shift));
	if (reg == OCI2C_PREHIGH)
		return (u8)(rd >> 8);
	else
		return (u8)rd;
}

static void oc_setreg_rcmodule(struct ocores_i2c *i2c, int reg, u8 value)
{
	u32 curr, wr;
	int rreg = reg;
	if (reg != OCI2C_PRELOW)
		rreg--;
	if (reg == OCI2C_PRELOW || reg == OCI2C_PREHIGH) {
		curr = readl(i2c->base + (rreg << i2c->reg_shift));
		if (reg == OCI2C_PRELOW)
			wr = (curr & 0xff00) | value;
		else
			wr = (((u32)value) << 8) | (curr & 0xff);
	} else {
		wr = value;
	}
	writel(wr, i2c->base + (rreg << i2c->reg_shift));
}

static int ocores_i2c_ofdata_to_platdata(struct udevice *dev)
{
	struct ocores_i2c *i2c = dev_get_priv(dev);

	debug("%s:\n", __func__);

	i2c->base = map_physmem(devfdt_get_addr(dev), 0x1000, MAP_NOCACHE);

	if (!i2c->base)
			return -ENOMEM;

	i2c->ip_clock_khz = dev_read_u32_default(dev, "clock-frequency", 50000000)/1000;
	i2c->bus_clock_khz = dev_read_u32_default(dev, "bus-clock", 100000)/1000;
	i2c->reg_io_width = dev_read_u32_default(dev, "reg-io-width", 4);
	i2c->reg_shift = dev_read_u32_default(dev, "reg_shift", 2);

	debug("OpenCores I2C settings: clock-frequency: %d, bus-clock: %d, reg-io-width: %d, reg_shift: %d\n",
		i2c->ip_clock_khz * 1000, i2c->bus_clock_khz * 1000, i2c->reg_io_width, i2c->reg_shift);

	return 0;
}


static int ocores_init(struct udevice *dev, struct ocores_i2c *i2c)
{
	int prescale;
	int diff;
	unsigned char ctrl = oc_getreg(i2c, OCI2C_CONTROL);

	/* make sure the device is disabled */
	oc_setreg(i2c, OCI2C_CONTROL, ctrl & ~(OCI2C_CTRL_EN|OCI2C_CTRL_IEN));

	prescale = (i2c->ip_clock_khz / (5 * i2c->bus_clock_khz)) - 1;
	prescale = clamp(prescale, 0, 0xffff);

	diff = i2c->ip_clock_khz / (5 * (prescale + 1)) - i2c->bus_clock_khz;
	if (abs(diff) > i2c->bus_clock_khz / 10) {
		dev_err(dev,
			"Unsupported clock settings: core: %d KHz, bus: %d KHz\n",
			i2c->ip_clock_khz, i2c->bus_clock_khz);
		return -EINVAL;
	}

	oc_setreg(i2c, OCI2C_PRELOW, prescale & 0xff);
	oc_setreg(i2c, OCI2C_PREHIGH, prescale >> 8);

	/* Init the device */
	oc_setreg(i2c, OCI2C_CMD, OCI2C_CMD_IACK);
	oc_setreg(i2c, OCI2C_CONTROL, ctrl | OCI2C_CTRL_EN | OCI2C_CTRL_IEN);

	return 0;
}

static int ocores_i2c_probe(struct udevice *bus)
{
	struct ocores_i2c *i2c = dev_get_priv(bus);
	int ret;

	i2c->setreg = oc_setreg_rcmodule;
	i2c->getreg = oc_getreg_rcmodule;

	ret = ocores_init(bus, i2c);

	debug("%s: %d\n", __func__, ret);

	return ret;
}



static const struct dm_i2c_ops ocores_i2c_ops = {
	.xfer          = ocores_i2c_xfer,
};

static const struct udevice_id ocores_i2c_ids[] = {
	{ .compatible = "rc-module,i2cmst" },
	{ }
};

U_BOOT_DRIVER(i2c_ocores) = {
	.name = "i2c_ocores",
	.id   = UCLASS_I2C,
	.of_match = ocores_i2c_ids,
	.probe = ocores_i2c_probe,
	.ofdata_to_platdata = ocores_i2c_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct ocores_i2c),
	.ops = &ocores_i2c_ops,
};

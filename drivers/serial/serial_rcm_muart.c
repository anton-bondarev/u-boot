/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 */

#include <common.h>
#include <dm.h>
#include <clk.h>
#include <errno.h>
#include <serial.h>
#include <linux/io.h>

#define MUART_ID 0x55415254
#define MUART_VERSION 0x10190

//MUART_CTRL
#define MUART_MEN_i 0
#define MUART_LBE_i 1
#define MUART_APB_MD 2
#define MUART_MDS_i 3
#define MUART_RTSen_i 4
#define MUART_CTSen_i 5
#define MUART_RTS_POL 6
#define MUART_PEN_i 7
#define MUART_EPS_i 8
#define MUART_SPS_i 9
#define MUART_STP2_i 10
#define MUART_WLEN_i 12
#define MUART_DUM_i 15

//MUART_BDIV
#define MUART_BAUD_DIV_i 0
#define MUART_N_DIV 24


//MUART_FIFO_STATE
#define MUART_RXFS_i 0
#define MUART_TXFS_i 16

/*
 *Information about a serial port
 *
 * @base: Register base address
 */

struct muart_regs {
    unsigned long id;            // 0x000
    unsigned long version;       // 0x004
    unsigned long sw_rst;        // 0x008
    unsigned long reserve_1;     // 0x00C
    unsigned long gen_status;    // 0x010
    unsigned long fifo_state;    // 0x014
    unsigned long status;        // 0x018
    unsigned long reserve_2;     // 0x01C
    unsigned long dtrans;        // 0x020
    unsigned long reserve_3;     // 0x024
    unsigned long drec;          // 0x028
    unsigned long reserve_4;     // 0x02C
    unsigned long bdiv;          // 0x030
    unsigned long reserve_5;     // 0x034
    unsigned long reserve_6;     // 0x038
    unsigned long reserve_7;     // 0x03C
    unsigned long fifowm;        // 0x040
    unsigned long ctrl;          // 0x044
    unsigned long mask;          // 0x048
    unsigned long rxtimeout;     // 0x04C
    unsigned long reserve_8;     // 0x050
    unsigned long txtimeout;     // 0x054
    // dma control registers - not needed for a while
};

struct muart_platdata {
	unsigned long base;
	unsigned long freq;
};

struct muart_priv {
	struct muart_regs *regs;
	unsigned long freq;
};

// default values
// id:             0x55415254
// version:        0x00010190
// sw_rst:         0x00000000
// gen_status:     0x00000000
// fifo_state:     0x004e0000
// status:         0x00000000
// dtrans:         0x00000078
// drec:           0x00000000
// bdiv:           0x00000008
// fifowm:         0x02000200
// ctrl:           0x00003005
// mask:           0x00000000
// rxtimeout:      0x00000000
// txtimeout:      0x00000000

int muart_probe(struct udevice *dev)
{
	struct muart_platdata *plat = dev_get_platdata(dev);
	struct muart_priv *priv = dev_get_priv(dev);

    priv->regs = (struct muart_regs *)plat->base;

    if(ioread32(&priv->regs->id) != MUART_ID || ioread32(&priv->regs->version) != MUART_VERSION)
    {
        printf("%s: error: Detected illegal version of uart core: 0x%08x 0x%08x\n", dev->name,
            ioread32(&priv->regs->id), 
            ioread32(&priv->regs->version));
        return -EINVAL;
    }

    priv->freq = plat->freq;
    if(priv->freq == 0)
        printf("%s: warning: Unable to get parent clocks - disabling setbrg\n", dev->name); 

    // switch on muart with APB mode (no dma), 8n1 
    iowrite32(
        (1 << MUART_MEN_i)  |   // enable
        (1 << MUART_APB_MD) |   // APB mode
        (0 << MUART_EPS_i)  |   // n prity
        (0 << MUART_STP2_i) |   // 1 stop bit
        (3 << MUART_WLEN_i),    // 8 data bit 
        &priv->regs->ctrl);

    return 0;
}

int muart_getc(struct udevice *dev)
{
	struct muart_priv *priv = dev_get_priv(dev);
    struct muart_regs* regs = priv->regs;

    int fifo_len = (ioread32(&regs->fifo_state) >> MUART_RXFS_i) & 0x7ff;
    if(fifo_len == 0)
        return -EAGAIN;
    
    return ioread32(&regs->drec) & 0xff;    
}

int muart_putc(struct udevice *dev, const char ch)
{
	struct muart_priv *priv = dev_get_priv(dev);
    struct muart_regs* regs = priv->regs;
    int fifo_len = (ioread32(&regs->fifo_state) >> MUART_TXFS_i) & 0x7ff;
    
    if(fifo_len == 0x7ff)
    {
        return -EAGAIN;
    }

	iowrite32(ch, &regs->dtrans);

    return 0;
}

int muart_pending(struct udevice *dev, bool input)
{
	struct muart_priv *priv = dev_get_priv(dev);
    struct muart_regs* regs = priv->regs;

    int fifo_len = (ioread32(&regs->fifo_state) >> MUART_RXFS_i) & 0x7ff;

    return fifo_len;
}

int muart_setbrg(struct udevice *dev, int baudrate)
{
	struct muart_priv *priv = dev_get_priv(dev);
    struct muart_regs* regs = priv->regs;
    unsigned int N = 0;
    unsigned int divisor;

    if(priv->freq == 0)
        return -EINVAL;

    if(baudrate > (priv->freq/8))
    {
        printf("%s: errorunable to set requested baudrate %d - too fast\n", dev->name, baudrate);
        return -EINVAL;
    }

    // there are two possible predividers 8 and 10 - choose best one to reach exact BR
    if(((priv->freq / 8) % baudrate) != 0)
        if(((priv->freq / 10) % baudrate) == 0)
            N = 1;

    divisor = priv->freq / (N?10:8 * baudrate);

    if(divisor > 0xFFFFFF)
    {
        printf("%s: errorunable to set requested baudrate %d - too slow\n", dev->name, baudrate);
        return -EINVAL;
    }

    // disable MUART
    iowrite32(ioread32(&regs->ctrl) & ~(1 << MUART_MEN_i), &regs->ctrl);

    // set new baud rate
    iowrite32((N<<MUART_N_DIV) | (divisor << MUART_BAUD_DIV_i), &regs->bdiv);

    // enable MUART
    iowrite32(ioread32(&regs->ctrl) | (1 << MUART_MEN_i), &regs->ctrl);

    return 0;
};

static const struct dm_serial_ops muart_ops = {
	.putc = muart_putc,
	.pending =  muart_pending,
	.getc =  muart_getc,
	.setbrg =  muart_setbrg,
};

static const struct udevice_id muart_id[] ={
	{.compatible = "rcm,muart", .data = 0},
	{}
};

int muart_ofdata_to_platdata(struct udevice *dev)
{
	struct muart_platdata *plat = dev_get_platdata(dev);
	fdt_addr_t addr;
	struct clk clkdev;
	int ret;

	addr = devfdt_get_addr(dev);
	if (addr == FDT_ADDR_T_NONE)
		return -EINVAL;
	plat->base = addr;

	ret = clk_get_by_index(dev, 0, &clkdev);
	if (ret)
		return ret;

	plat->freq = clk_get_rate(&clkdev);

	return 0;
}

U_BOOT_DRIVER(serial_rcm_muart) = {
	.name	= "serial_rcm_muart",
	.id	= UCLASS_SERIAL,
	.of_match = of_match_ptr(muart_id),
	.ofdata_to_platdata = of_match_ptr(muart_ofdata_to_platdata),
	.platdata_auto_alloc_size = sizeof(struct muart_platdata),
	.probe = muart_probe,
	.ops	= &muart_ops,
	.flags = DM_FLAG_PRE_RELOC,
	.priv_auto_alloc_size = sizeof(struct muart_priv),
};

#ifdef CONFIG_DEBUG_UART_RCM

#include <debug_uart.h>

static inline void _debug_uart_init(void)
{
}

static inline void _debug_uart_putc(int ch)
{
    struct muart_regs* regs = (struct muart_regs*) CONFIG_DEBUG_UART_BASE;

	while ((ioread32(&regs->fifo_state) & 0x7ff0000) >= 0x3ff0000);
	iowrite32(ch, &regs->dtrans);
}

DEBUG_UART_FUNCS

#endif
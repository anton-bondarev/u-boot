/* Gaisler.com GRETH 10/100/1000 Ethernet MAC driver
 *
 * Driver use polling mode (no Interrupt)
 *
 * (C) Copyright 2007
 * Daniel Hellstrom, Gaisler Research, daniel@gaisler.com
 *
 * Update to DM mode, skip SPARK specific things
 * (C) Copyright 2018
 * Alexey Spirkov, AstroSoft, alexeis@astrosoft.ru
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define DEBUG

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include "greth.h"
#include <asm/io.h>

/* Default to 3s timeout on autonegotiation */
#ifndef GRETH_PHY_TIMEOUT_MS
#define GRETH_PHY_TIMEOUT_MS 3000
#endif

#ifndef GRETH_SEND_TIMEOUT_MS
#define GRETH_SEND_TIMEOUT_MS 3000
#endif

/* Default to PHY adrress 0 not not specified */
#ifdef CONFIG_SYS_GRLIB_GRETH_PHYADDR
#define GRETH_PHY_ADR_DEFAULT CONFIG_SYS_GRLIB_GRETH_PHYADDR
#else
#define GRETH_PHY_ADR_DEFAULT 0
#endif

#define GRETH_REGLOAD(addr )readl(addr)
#define GRETH_REGSAVE(addr,data) writel(data,addr)
#define GRETH_REGORIN(addr,data) GRETH_REGSAVE(addr,GRETH_REGLOAD(addr)|data)
#define GRETH_REGANDIN(addr,data) GRETH_REGSAVE(addr,GRETH_REGLOAD(addr)&data)

#define GRETH_RXBD_CNT 4
#define GRETH_TXBD_CNT 1

#define GRETH_RXBUF_SIZE 1540
#define GRETH_BUF_ALIGN 4
#define GRETH_RXBUF_EFF_SIZE \
	( (GRETH_RXBUF_SIZE&~(GRETH_BUF_ALIGN-1))+GRETH_BUF_ALIGN*2 )

typedef struct {
	greth_regs *regs;
	int irq;
	struct eth_device *dev;

	/* Hardware info */
	unsigned char phyaddr;
	int gbit_mac;

	/* Current operating Mode */
	int gb;			/* GigaBit */
	int fd;			/* Full Duplex */
	int sp;			/* 10/100Mbps speed (1=100,0=10) */
	int auto_neg;		/* Auto negotiate done */

	unsigned char hwaddr[6];	/* MAC Address */

	/* Descriptors */
	greth_bd *rxbd_base, *rxbd_max;
	greth_bd *txbd_base, *txbd_max;

	greth_bd *rxbd_curr;

	/* rx buffers in rx descriptors */
	void *rxbuf_base;	/* (GRETH_RXBUF_SIZE+ALIGNBYTES) * GRETH_RXBD_CNT */

	/* unused for gbit_mac, temp buffer for sending packets with unligned
	 * start.
	 * Pointer to packet allocated with malloc.
	 */
	void *txbuf;

	struct {
		/* rx status */
		unsigned int rx_packets,
		    rx_crc_errors, rx_frame_errors, rx_length_errors, rx_errors;

		/* tx stats */
		unsigned int tx_packets,
		    tx_latecol_errors,
		    tx_underrun_errors, tx_limit_errors, tx_errors;
	} stats;
} greth_priv;

/* Read MII register 'addr' from core 'regs' */
static int read_mii(int phyaddr, int regaddr, volatile greth_regs * regs)
{
	debug("read_mii %d - wait until busy\n", regaddr);

	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
	}

	debug("done - write phy & regaddr\n");

	GRETH_REGSAVE(&regs->mdio, ((phyaddr & 0x1F) << 11) | ((regaddr & 0x1F) << 6) | 2);

	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
	}
debug("done\n");

	if (!(GRETH_REGLOAD(&regs->mdio) & GRETH_MII_NVALID)) {
		uint32_t val = GRETH_REGLOAD(&regs->mdio);
		debug("MDIO reg %d val %08x\n", regaddr, val); 
		return (val >> 16) & 0xFFFF;
	} else {
		return -1;
	}
}

static void write_mii(int phyaddr, int regaddr, int data, volatile greth_regs * regs)
{
	debug("write_mii wait until busy\n");
	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
	}
	debug("done\n");

	GRETH_REGSAVE(&regs->mdio,
		      ((data & 0xFFFF) << 16) | ((phyaddr & 0x1F) << 11) |
		      ((regaddr & 0x1F) << 6) | 1);

	while (GRETH_REGLOAD(&regs->mdio) & GRETH_MII_BUSY) {
	}

}

int fix_endian (int data_in){

#ifdef CONFIG_MPW7705__
    int data_out = 0;
    
    data_out = data_in << 24 & 0xff000000;
    data_out = data_out | (data_in << 8  & 0x00ff0000);
    data_out = data_out | (data_in >> 8  & 0x0000ff00);
    data_out = data_out | (data_in >> 24 & 0x000000ff);
    
    return data_out;
#else
	return data_in;
#endif
}

/* init/start hardware and allocate descriptor buffers for rx side
 *
 */
int greth_init(struct udevice *dev)
{
	int i;

	greth_priv *greth = dev_get_priv(dev);
	greth_regs *regs = greth->regs;

	debug("greth_init\n");

	/* Reset core */
	GRETH_REGSAVE(&regs->control, (GRETH_RESET | (greth->gb << 8) |
		(greth->sp << 7) | (greth->fd << 4)));

	/* Wait for Reset to complete */
	while ( GRETH_REGLOAD(&regs->control) & GRETH_RESET) ;

	GRETH_REGSAVE(&regs->control,
		((greth->gb << 8) | (greth->sp << 7) | (greth->fd << 4)));

	if (!greth->rxbd_base) {

		/* allocate descriptors */
		greth->rxbd_base = (greth_bd *)
		    memalign(0x1000, GRETH_RXBD_CNT * sizeof(greth_bd));
		greth->txbd_base = (greth_bd *)
		    memalign(0x1000, GRETH_TXBD_CNT * sizeof(greth_bd));

		/* allocate buffers to all descriptors  */
		greth->rxbuf_base =
		    malloc(GRETH_RXBUF_EFF_SIZE * GRETH_RXBD_CNT);
	}

	/* initate rx decriptors */
	for (i = 0; i < GRETH_RXBD_CNT; i++) {
		greth->rxbd_base[i].addr = fix_endian((unsigned int)
		    greth->rxbuf_base + (GRETH_RXBUF_EFF_SIZE * i));
		/* enable desciptor & set wrap bit if last descriptor */
		if (i >= (GRETH_RXBD_CNT - 1)) {
			greth->rxbd_base[i].stat = fix_endian(GRETH_BD_EN | GRETH_BD_WR);
		} else {
			greth->rxbd_base[i].stat = fix_endian(GRETH_BD_EN);
		}
	}

	/* initiate indexes */
	greth->rxbd_curr = greth->rxbd_base;
	greth->rxbd_max = greth->rxbd_base + (GRETH_RXBD_CNT - 1);
	greth->txbd_max = greth->txbd_base + (GRETH_TXBD_CNT - 1);
	/*
	 * greth->txbd_base->addr = 0;
	 * greth->txbd_base->stat = GRETH_BD_WR;
	 */

	/* initate tx decriptors */
	for (i = 0; i < GRETH_TXBD_CNT; i++) {
		greth->txbd_base[i].addr = 0;
		/* enable desciptor & set wrap bit if last descriptor */
		if (i >= (GRETH_TXBD_CNT - 1)) {
			greth->txbd_base[i].stat = fix_endian(GRETH_BD_WR);
		} else {
			greth->txbd_base[i].stat = 0;
		}
	}

	/**** SET HARDWARE REGS ****/

	/* Set pointer to tx/rx descriptor areas */
	GRETH_REGSAVE(&regs->rx_desc_p, (unsigned int)&greth->rxbd_base[0]);
	GRETH_REGSAVE(&regs->tx_desc_p, (unsigned int)&greth->txbd_base[0]);

	/* Enable Transmitter, GRETH will now scan descriptors for packets
	 * to transmitt */
	debug("greth_init: enabling receiver\n");
	GRETH_REGORIN(&regs->control, GRETH_RXEN);

	return 0;
}

#define SKIP_PHY_INIT
/* Initiate PHY to a relevant speed
 * return:
 *  - 0 = success
 *  - 1 = timeout/fail
 */
static int greth_init_phy(greth_priv * dev)
{
	greth_regs *regs = dev->regs;
	int tmp, tmp1, tmp2, i;
	unsigned int start, timeout;
	int phyaddr = GRETH_PHY_ADR_DEFAULT;


#ifdef SKIP_PHY_INIT
	dev->phyaddr = 0;
	dev->gb = 0;
	dev->fd = 1;
	dev->sp = 1;
	dev->auto_neg = 0;
#else

#ifndef CONFIG_SYS_GRLIB_GRETH_PHYADDR
	/* If BSP doesn't provide a hardcoded PHY address the driver will
	 * try to autodetect PHY address by stopping the search on the first
	 * PHY address which has REG0 implemented.
	 */
	for (i=0; i<32; i++) {
		tmp = read_mii(i, 0, regs);
		if ( (tmp != 0) && (tmp != 0xffff) ) {
			phyaddr = i;
			break;
		}
	}
#endif

	/* Save PHY Address */
	dev->phyaddr = phyaddr;

	debug("GRETH PHY ADDRESS: %d\n", phyaddr);

	timeout = GRETH_PHY_TIMEOUT_MS;

	/* Get system timer0 current value
	 * Total timeout is 5s
	 */
	start = get_timer(0);

	/* get phy control register default values */

	while ((tmp = read_mii(phyaddr, 0, regs)) & 0x8000) {
		if (get_timer(start) > timeout) {
			debug("greth_init_phy: PHY read 1 failed\n");
			return 1;	/* Fail */
		}
	}

	/* reset PHY and wait for completion */
	write_mii(phyaddr, 0, 0x8000 | tmp, regs);
	while (((tmp = read_mii(phyaddr, 0, regs))) & 0x8000) {
		if (get_timer(start) > timeout) {
			debug("greth_init_phy: PHY read 2 failed\n");
			return 1;	/* Fail */
		}
	}

	debug("MI regs: %04X, %04X, %04X, %04X, %04X, %04X, %04X\n",
		read_mii(phyaddr, 0, regs),
		read_mii(phyaddr, 1, regs),
		read_mii(phyaddr, 2, regs),
		read_mii(phyaddr, 3, regs),
		read_mii(phyaddr, 4, regs),
		read_mii(phyaddr, 5, regs),
		read_mii(phyaddr, 6, regs));
		


	/* Check if PHY is autoneg capable and then determine operating
	 * mode, otherwise force it to 10 Mbit halfduplex
	 */
	dev->gb = 0;
	dev->fd = 0;
	dev->sp = 0;
	dev->auto_neg = 0;
	if (!((tmp >> 12) & 1)) {
		write_mii(phyaddr, 0, 0, regs);
	} else {
		/* wait for auto negotiation to complete and then check operating mode */
		dev->auto_neg = 1;
		i = 0;
		while (!(((tmp = read_mii(phyaddr, 1, regs)) >> 5) & 1)) {
			if (get_timer(start) > timeout) {
				printf("Auto negotiation timed out. "
				       "Selecting default config\n");
				tmp = read_mii(phyaddr, 0, regs);
				dev->gb = ((tmp >> 6) & 1)
				    && !((tmp >> 13) & 1);
				dev->sp = !((tmp >> 6) & 1)
				    && ((tmp >> 13) & 1);
				dev->fd = (tmp >> 8) & 1;
				goto auto_neg_done;
			}
		}

		if ((tmp >> 8) & 1) {
			tmp1 = read_mii(phyaddr, 9, regs);
			tmp2 = read_mii(phyaddr, 10, regs);
			if ((tmp1 & GRETH_MII_EXTADV_1000FD) &&
			    (tmp2 & GRETH_MII_EXTPRT_1000FD)) {
				dev->gb = 1;
				dev->fd = 1;
			}
			if ((tmp1 & GRETH_MII_EXTADV_1000HD) &&
			    (tmp2 & GRETH_MII_EXTPRT_1000HD)) {
				dev->gb = 1;
				dev->fd = 0;
			}
		}
		if ((dev->gb == 0) || ((dev->gb == 1) && (dev->gbit_mac == 0))) {
			tmp1 = read_mii(phyaddr, 4, regs);
			tmp2 = read_mii(phyaddr, 5, regs);
			if ((tmp1 & GRETH_MII_100TXFD) &&
			    (tmp2 & GRETH_MII_100TXFD)) {
				dev->sp = 1;
				dev->fd = 1;
			}
			if ((tmp1 & GRETH_MII_100TXHD) &&
			    (tmp2 & GRETH_MII_100TXHD)) {
				dev->sp = 1;
				dev->fd = 0;
			}
			if ((tmp1 & GRETH_MII_10FD) && (tmp2 & GRETH_MII_10FD)) {
				dev->fd = 1;
			}
			if ((dev->gb == 1) && (dev->gbit_mac == 0)) {
				dev->gb = 0;
				dev->fd = 0;
				write_mii(phyaddr, 0, dev->sp << 13, regs);
			}
		}

	}
      auto_neg_done:
	debug("%s GRETH Ethermac at [0x%x] irq %d. Running \
		%d Mbps %s duplex\n", dev->gbit_mac ? "10/100/1000" : "10/100", (unsigned int)(regs), (unsigned int)(dev->irq), dev->gb ? 1000 : (dev->sp ? 100 : 10), dev->fd ? "full" : "half");
	/* Read out PHY info if extended registers are available */
	if (tmp & 1) {
		tmp1 = read_mii(phyaddr, 2, regs);
		tmp2 = read_mii(phyaddr, 3, regs);
		tmp1 = (tmp1 << 6) | ((tmp2 >> 10) & 0x3F);
		tmp = tmp2 & 0xF;

		tmp2 = (tmp2 >> 4) & 0x3F;
		debug("PHY: Vendor %x   Device %x    Revision %d\n", tmp1,
		       tmp2, tmp);
	} else {
		printf("PHY info not available\n");
	}
#endif
	/* set speed and duplex bits in control register */
	GRETH_REGORIN(&regs->control,
		      (dev->gb << 8) | (dev->sp << 7) | (dev->fd << 4));

	return 0;
}

void greth_halt(struct udevice *dev)
{
	greth_priv *greth;
	greth_regs *regs;
	int i;

	debug("greth_halt\n");

	if (!dev)
		return;

	greth = dev_get_priv(dev);
	regs = greth->regs;

	if (!regs)
		return;

	/* disable receiver/transmitter by clearing the enable bits */
	GRETH_REGANDIN(&regs->control, ~(GRETH_RXEN | GRETH_TXEN));

	/* reset rx/tx descriptors */
	if (greth->rxbd_base) {
		for (i = 0; i < GRETH_RXBD_CNT; i++) {
			greth->rxbd_base[i].stat = fix_endian(
			    (i >= (GRETH_RXBD_CNT - 1)) ? GRETH_BD_WR : 0);
		}
	}

	if (greth->txbd_base) {
		for (i = 0; i < GRETH_TXBD_CNT; i++) {
			greth->txbd_base[i].stat = fix_endian(
			    (i >= (GRETH_TXBD_CNT - 1)) ? GRETH_BD_WR : 0);
		}
	}
}


int greth_send(struct udevice *dev, void *eth_data, int data_length)
{
	greth_priv *greth = dev_get_priv(dev);
	greth_regs *regs = greth->regs;
	greth_bd *txbd;
	void *txbuf;
	unsigned int status;
	unsigned int start, timeout;


	debug("greth_send\n");
#ifdef DEBUG
	debug("packet (%d):\n", data_length);
	unsigned char *p = eth_data;
	int i=0;
	while(p < (eth_data+data_length))
	{
		debug("%02X ", *p++);
		if(++i == 16)
		{
			debug("\n");
			i = 0;
		}
	}
	debug("\n");
#endif

#ifndef CONFIG_MPW7705_
	/* send data, wait for data to be sent, then return */
	if (((unsigned int)eth_data & (GRETH_BUF_ALIGN - 1))
	    && !greth->gbit_mac) {
		/* data not aligned as needed by GRETH 10/100, solve this by allocating 4 byte aligned buffer
		 * and copy data to before giving it to GRETH.
		 */
#endif		
		if (!greth->txbuf) {
			greth->txbuf = malloc(GRETH_RXBUF_EFF_SIZE);
		}

		txbuf = greth->txbuf;

#ifdef CONFIG_MPW7705_
		// HW bug? swap everything
		int j = 0;
		u32* data = (u32*) eth_data;
		u32* outdata = (u32*) txbuf;
		for(j = 0; j < data_length; j+=sizeof(u32) )
			*outdata++ = fix_endian(*data++);
#else
		/* copy data info buffer */
		memcpy((char *)txbuf, (char *)eth_data, data_length);

		/* keep buffer to next time */
	} else {
		txbuf = (void *)eth_data;
	}
#endif		
	/* get descriptor to use, only 1 supported... hehe easy */
	txbd = greth->txbd_base;

	/* setup descriptor to wrap around to it self */
	txbd->addr = fix_endian((unsigned int)txbuf);
	txbd->stat = fix_endian(GRETH_BD_EN | GRETH_BD_WR | data_length);

	/* Remind Core which descriptor to use when sending */

	GRETH_REGSAVE(&regs->tx_desc_p, (unsigned int)txbd);

	/* initate send by enabling transmitter */
	GRETH_REGORIN(&regs->control, GRETH_TXEN);

	start = get_timer(0);
	timeout = GRETH_SEND_TIMEOUT_MS;

	/* Wait for data to be sent */
	while ((status = fix_endian(GRETH_REGLOAD(&txbd->stat))) & GRETH_BD_EN) {
		if (get_timer(start) > timeout) {
			debug("greth_send: hangs on status wait, Stat: %x\n", GRETH_REGLOAD(&regs->status));
			return -2;	/* Fail */
		}
	}

	/* was the packet transmitted succesfully? */
	if (status & GRETH_TXBD_ERR_AL) {
		greth->stats.tx_limit_errors++;
	}

	if (status & GRETH_TXBD_ERR_UE) {
		greth->stats.tx_underrun_errors++;
	}

	if (status & GRETH_TXBD_ERR_LC) {
		greth->stats.tx_latecol_errors++;
	}

	if (status &
	    (GRETH_TXBD_ERR_LC | GRETH_TXBD_ERR_UE | GRETH_TXBD_ERR_AL)) {
		/* any error */
		greth->stats.tx_errors++;
		return -1;
	}

	/* bump tx packet counter */
	greth->stats.tx_packets++;

	/* return succefully */
	return 0;
}

int greth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	greth_priv *greth = dev_get_priv(dev);
	greth_regs *regs = greth->regs;
	greth_bd *rxbd;
	unsigned int status, len = 0, bad;
	char *d;
	int enable = 0;
	int i;

	/* Receive One packet only, but clear as many error packets as there are
	 * available.
	 */
	{
		/* current receive descriptor */
		rxbd = greth->rxbd_curr;

		/* get status of next received packet */
		status = fix_endian(GRETH_REGLOAD(&rxbd->stat));

		bad = 0;

		/* stop if no more packets received */
		if (status & GRETH_BD_EN) {
			goto done;
		}

		debug("greth_recv: packet 0x%x, 0x%x, len: %d, ",
		       (unsigned int)rxbd, status, status & GRETH_BD_LEN);

		/* Check status for errors.
		 */
		if (status & GRETH_RXBD_ERR_FT) {
			greth->stats.rx_length_errors++;
			bad = 1;
		}
		if (status & (GRETH_RXBD_ERR_AE | GRETH_RXBD_ERR_OE)) {
			greth->stats.rx_frame_errors++;
			bad = 1;
		}
		if (status & GRETH_RXBD_ERR_CRC) {
			greth->stats.rx_crc_errors++;
			bad = 1;
		}
		if (bad) {
			greth->stats.rx_errors++;
			printf
			    ("greth_recv: Bad packet (%d, %d, %d, 0x%08x, %d)\n",
			     greth->stats.rx_length_errors,
			     greth->stats.rx_frame_errors,
			     greth->stats.rx_crc_errors, status,
			     greth->stats.rx_packets);
			/* print all rx descriptors */
			for (i = 0; i < GRETH_RXBD_CNT; i++) {
				printf("[%d]: Stat=0x%lx, Addr=0x%lx\n", i,
				       (unsigned long) fix_endian((long)GRETH_REGLOAD(&greth->rxbd_base[i].stat)),
				       (unsigned long) fix_endian((long)GRETH_REGLOAD(&greth->rxbd_base[i].addr)));
			}
		} else {
			/* Process the incoming packet. */			
			len = status & GRETH_BD_LEN;
			d = (char *)fix_endian(rxbd->addr);

			// flush cache
			flush_cache((unsigned long)d, len);

#ifdef CONFIG_MPW7705_
			// HW bug? swap everything
			int j = 0;
			if(((u32)d)%4)
				debug("Network unaligned access\n");
			int setlen = len/4;
			int rem = len%4;
			u32* data = (u32*) d;
			
			if(rem)
			{
				unsigned char* p = (unsigned char*)(data+setlen-1);
				debug("Rem: %x %x %x %x %x %x %x %x\n", (unsigned)p[0], (unsigned)p[1], (unsigned)p[2], (unsigned)p[3],
													   (unsigned)p[4], (unsigned)p[5], (unsigned)p[6], (unsigned)p[7]);
				setlen++;
				
			}
			for(j = 0; j < setlen; j++)
			{
				*data = fix_endian(*data);
				data++;
			}

#endif

#ifdef DEBUG
	debug("recv packet (%d):\n", len);
	unsigned char *p = d;
	int i=0;
	while(p < (d+len))
	{
		debug("%02X ", *p++);
		if(++i == 16)
		{
			debug("\n");
			i = 0;
		}
	}
	debug("\n");
#endif

			/* pass packet on to network subsystem */
			net_process_received_packet((void *)d, len);

			/* bump stats counters */
			greth->stats.rx_packets++;

			/* bad is now 0 ==> will stop loop */
		}

		/* reenable descriptor to receive more packet with this descriptor, wrap around if needed */
		rxbd->stat =
		    fix_endian(GRETH_BD_EN |
		    (((unsigned int)greth->rxbd_curr >=
		      (unsigned int)greth->rxbd_max) ? GRETH_BD_WR : 0));
		enable = 1;

		/* increase index */
		greth->rxbd_curr =
		    ((unsigned int)greth->rxbd_curr >=
		     (unsigned int)greth->rxbd_max) ? greth->
		    rxbd_base : (greth->rxbd_curr + 1);

	}

	if (enable) {
		GRETH_REGORIN(&regs->control, GRETH_RXEN);
	}
      done:
	/* return positive length of packet or 0 if non received */
	return len;
}

int greth_set_hwaddr(struct udevice *dev)
{
	greth_priv *greth = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_platdata(dev);
	uchar *mac = pdata->enetaddr;

	greth->regs->esa_msb = ((mac[0] << 8) | mac[1]);
	greth->regs->esa_lsb = (
	    (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5]);

	debug("GRETH: New MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
	       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	return 0;
}

static void iowrite32(uint32_t const value, uint32_t const base_addr)
{
    *((volatile uint32_t*)(base_addr)) = value;
}

static int greth_probe(struct udevice *dev)
{

	debug("%08x, %08x\n",
		*((volatile uint32_t*)(0x3C03F030)),
		*((volatile uint32_t*)(0x3C03F034))
	);
	// todo move somewhere initialization of extmem muxer
	//iowrite32(0x1, 0x3C03F030);
	//iowrite32(0x0, 0x3C03F034);


	//struct eth_pdata *pdata = dev_get_platdata(dev);
	greth_priv *greth = dev_get_priv(dev);
 
	/* Reset Core */
	GRETH_REGSAVE(&greth->regs->control, GRETH_RESET);

	/* Wait for core to finish reset cycle */
	while (GRETH_REGLOAD(&greth->regs->control) & GRETH_RESET) ;

	/* Get the phy address which assumed to have been set
	   correctly with the reset value in hardware */
	greth->phyaddr = (GRETH_REGLOAD(&greth->regs->mdio) >> 11) & 0x1F;

	/* Check if mac is gigabit capable */
	greth->gbit_mac = (GRETH_REGLOAD(&greth->regs->control) >> 27) & 1;

	debug("MDIO: %x, CONTROL: %x\n", GRETH_REGLOAD(&greth->regs->mdio), GRETH_REGLOAD(&greth->regs->control));

	/* initiate PHY, select speed/duplex depending on connected PHY */
	if (greth_init_phy(greth)) {
		/* Failed to init PHY (timedout) */
		debug("GRETH[%p]: Failed to init PHY\n", greth->regs);
		return -1;
	}

	debug("GRETH[%p]: Initialized successfully\n", greth->regs);

	return 0;
}


static int greth_remove(struct udevice *dev)
{
	greth_priv *priv = dev_get_priv(dev);

	if(priv->rxbuf_base)
	{
		free(priv->rxbuf_base);
		priv->rxbuf_base = 0;
	}
	if(priv->txbuf)
	{
		free(priv->txbuf);
		priv->txbuf = 0;
	}
	return 0;
}


static int greth_ofdata_to_platdata(struct udevice *dev)
{
        greth_priv *priv = dev_get_priv(dev);
		struct eth_pdata *pdata = dev_get_platdata(dev);
		const char *mac;
		int len;

        priv->regs = (greth_regs*) devfdt_get_addr(dev);

		mac = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "mac-address", &len);
		if (mac && is_valid_ethaddr((u8 *)mac))
		{
			pdata->enetaddr[0] = mac[0];
			pdata->enetaddr[1] = mac[1];
			pdata->enetaddr[2] = mac[2];
			pdata->enetaddr[3] = mac[3];
			pdata->enetaddr[4] = mac[4];
			pdata->enetaddr[5] = mac[5];
		}

//        pdata->phy_interface = -1;
//        phy_mode = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "phy-mode",
//                               NULL);
//        if (phy_mode)
//                pdata->phy_interface = phy_get_interface_by_name(phy_mode);
//        if (pdata->phy_interface == -1) {
//                debug("%s: Invalid PHY interface '%s'\n", __func__, phy_mode);
//                return -EINVAL;
//        }

        return 0;
}


static const struct eth_ops greth_ops = {
        .start                  = greth_init,
        .send                   = greth_send,
        .recv                   = greth_recv,
        .stop                   = greth_halt,
        .write_hwaddr           = greth_set_hwaddr,
};


static const struct udevice_id greth_ids[] = {
        { .compatible = "rc-module,greth" },
        { }
};

U_BOOT_DRIVER(greth) = {
        .name   = "greth",
        .id     = UCLASS_ETH,
        .of_match = greth_ids,
        .ofdata_to_platdata = greth_ofdata_to_platdata,
        .probe  = greth_probe,
        .remove = greth_remove,
        .ops    = &greth_ops,
        .priv_auto_alloc_size = sizeof(greth_priv),
        .platdata_auto_alloc_size = sizeof(struct eth_pdata),
};






/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Alexey Spirkov <dev@alsp.net>
 *
 *  Current limitations 
 *      - works in pull mode, 
 *      - only one channel used for rx and one for tx
 */
//#define DEBUG
//#define DETAILED_DEBUG
#define DBGPREFIX "[rcm-mgeth]: "

#include <version.h>
#include <common.h>
#include <config.h>
#include <dm.h>
#include <fdt_support.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <asm/io.h>
#include <cpu_func.h>
#include <linux/io.h>
#include <generic-phy.h>
#include <wait_bit.h>
//#include "rcm_mdma_chan.h"

#define MGETH_ID 0x48544547
#define MGETH_VER 0x01900144

// RXBD_COUNT should be multiply of 4
#define MGETH_RXBD_CNT 4
#define MGETH_TXBD_CNT 1

#define MGETH_BD_POLL_ALIGN 0x1000

// descriptor flags
#define MGETH_BD_OWN 0x80000000
#define MGETH_BD_LINK 0x40000000
#define MGETH_BD_STOP 0x10000000

// channel settings
#define MGETH_CHAN_DESC_NORMAL 0x00000000
#define MGETH_CHAN_DESC_LONG 0x00000002
#define MGETH_CHAN_DESC_PITCH 0x00000003
#define MGETH_CHAN_ADD_INFO 0x00000010
#define MGETH_CHAN_DESC_GAP_SHIFT 16

// generic flags
#define MGETH_ENABLE 0x1

#define MGETH_MIN_PACKET_LEN 60
#define MGETH_MAX_PACKET_LEN 0x3fff

#define MGETH_RXBUF_SIZE 1540

typedef const volatile unsigned int roreg32;
typedef volatile unsigned int rwreg32;
typedef const volatile unsigned long long roreg64;
typedef volatile unsigned long long rwreg64;

#define BUF_SIZE 2048

typedef struct _mgeth_rx_regs {
/************************* RX Channel registers *****************************/
    rwreg32 rx_eth_mask_value[32];          /* 0x000-0x07C - packets mask   */
    rwreg32 rx_eth_mask_activ[32];          /* 0x080-0x0FC - mask activation*/
/*********************** WDMA Channel registers *****************************/
    rwreg32 enable;                         /* 0x100 - enable channel       */
    rwreg32 suspend;                        /* 0x104 - suspend channel      */
    rwreg32 cancel;                         /* 0x108 - cancel channel       */
    roreg32 _skip01;                        /* 0x10C                        */
    rwreg32 settings;                       /* 0x110 - channel settings     */
    rwreg32 irq_mask;                       /* 0x114 - channel irq mask     */
    roreg32 status;                         /* 0x118 - channel status       */
    roreg32 _skip02;                        /* 0x11C                        */
    rwreg32 desc_addr;                      /* 0x120 - first decriptor addr */
    roreg32 _skip03;                        /* 0x124                        */
    roreg32 curr_desc_addr;                 /* 0x128 - current decript addr */
    roreg32 curr_addr;                      /* 0x12C - current trans addr   */
    roreg32 dma_state;                      /* 0x130 - state of DMA         */
    roreg32 _skip04[3];                     /* 0x134 - 0x13C                */
    rwreg32 desc_axlen;                     /* 0x140 - axlen for desc ops   */
    rwreg32 desc_axcache;                   /* 0x144 - axcache for desc ops */
    rwreg32 desc_axprot;                    /* 0x148 - axprot for desc ops  */
    rwreg32 desc_axlock;                    /* 0x14C - axlock for desc ops  */
    roreg32 desc_rresp;                     /* 0x150 - rresp of desc ops    */
    roreg32 desc_raxi_err_addr;             /* 0x154 - addr of axi read err */
    roreg32 desc_bresp;                     /* 0x158 - bresp of desc ops    */
    roreg32 desc_waxi_err_addr;             /* 0x15C - addr of axi write err*/
    rwreg32 desc_permut;                    /* 0x160 - byte swapping scheme */
    roreg32 _skip05[7];                     /* 0x164 - 0x17C                */
    rwreg32 max_trans;                      /* 0x180 - max unfinished trans */
    rwreg32 awlen;                          /* 0x184 - axi awlen            */
    rwreg32 awcache;                        /* 0x188 - axi awcache          */
    rwreg32 awprot;                         /* 0x18C - axi awprot           */
    rwreg32 awlock;                         /* 0x190 - axi awlock           */
    roreg32 bresp;                          /* 0x194 - axi operation bresp  */
    roreg32 waxi_err_addr;                  /* 0x198 - addr of axi write err*/
    roreg32 _skip06;                        /* 0x19C                        */
    roreg32 state;                          /* 0x1A0 - axi state            */
    roreg32 avaliable_space;                /* 0x1A4 - num of free bytes    */
    rwreg32 permutation;                    /* 0x1A8 - byte swapping scheme */
    roreg32 _skip07[5];                     /* 0x1AC - 0x1BC                */
/************************** Statistc counters *******************************/
    roreg32 a_frames_received_ok;           /* 0x1C0                        */
    roreg64 a_octets_received_ok;           /* 0x1C4                        */
    roreg32 if_in_ucast_pkts;               /* 0x1CC                        */
    roreg32 if_in_multicast_pkts;           /* 0x1D0                        */
    roreg32 if_in_broadcast_pkts;           /* 0x1D4                        */
    roreg32 descriptor_short;               /* 0x1D8                        */
    roreg32 rtp_overmuch_line;              /* 0x1DC                        */
    roreg32 _skip08[8];                     /* 0x1E0 - 0x1FC                */
} __attribute__ ((packed)) mgeth_rx_regs;

typedef struct _mgeth_tx_regs {
    rwreg32 enable;                         /* 0x000 - enable channel       */
    rwreg32 suspend;                        /* 0x004 - suspend channel      */
    rwreg32 cancel;                         /* 0x008 - cancel channel       */
    roreg32 _skip01;                        /* 0x00C                        */
    rwreg32 settings;                       /* 0x010 - channel settings     */
    rwreg32 irq_mask;                       /* 0x014 - channel irq mask     */
    roreg32 status;                         /* 0x018 - channel status       */
    roreg32 _skip02;                        /* 0x01C                        */
    rwreg32 desc_addr;                      /* 0x020 - first decriptor addr */
    roreg32 _skip03;                        /* 0x024                        */
    roreg32 curr_desc_addr;                 /* 0x028 - current decript addr */
    roreg32 curr_addr;                      /* 0x02C - current trans addr   */
    roreg32 dma_state;                      /* 0x030 - state of DMA         */
    roreg32 _skip04[3];                     /* 0x034 - 0x13C                */
    rwreg32 desc_axlen;                     /* 0x040 - axlen for desc ops   */
    rwreg32 desc_axcache;                   /* 0x044 - axcache for desc ops */
    rwreg32 desc_axprot;                    /* 0x048 - axprot for desc ops  */
    rwreg32 desc_axlock;                    /* 0x04C - axlock for desc ops  */
    roreg32 desc_rresp;                     /* 0x050 - rresp of desc ops    */
    roreg32 desc_raxi_err_addr;             /* 0x054 - addr of axi read err */
    roreg32 desc_bresp;                     /* 0x058 - bresp of desc ops    */
    roreg32 desc_waxi_err_addr;             /* 0x05C - addr of axi write err*/
    rwreg32 desc_permut;                    /* 0x060 - byte swapping scheme */
    roreg32 _skip05[7];                     /* 0x064 - 0x17C                */
    rwreg32 max_trans;                      /* 0x080 - max unfinished trans */
    rwreg32 arlen;                          /* 0x084 - axi arlen            */
    rwreg32 arcache;                        /* 0x088 - axi arcache          */
    rwreg32 arprot;                         /* 0x08C - axi arprot           */
    rwreg32 arlock;                         /* 0x090 - axi arlock           */
    roreg32 rresp;                          /* 0x094 - axi operation rresp  */
    roreg32 waxi_err_addr;                  /* 0x098 - addr of axi write err*/
    roreg32 _skip06;                        /* 0x09C                        */
    roreg32 state;                          /* 0x0A0 - axi state            */
    roreg32 avaliable_space;                /* 0x0A4 - num of free bytes    */
    rwreg32 permutation;                    /* 0x0A8 - byte swapping scheme */
    roreg32 _skip07[5];                     /* 0x0AC - 0x1BC                */
/************************** Statistc counters *******************************/
    roreg32 a_frames_transmitted_ok;        /* 0x0C0                        */
    roreg64 a_octets_transmitted_ok;        /* 0x0C4                        */
    roreg32 if_out_ucast_pkts;              /* 0x0CC                        */
    roreg32 if_out_multicast_pkts;          /* 0x0D0                        */
    roreg32 if_out_broadcast_pkts;          /* 0x0D4                        */
    roreg32 _skip08[10];                    /* 0x0D8 - 0x0FC                */
} __attribute__ ((packed)) mgeth_tx_regs;

typedef struct _mgeth_regs {
/***************** Common registers for MGETH and MDMA **********************/
    roreg32 id;                             /* 0x000 - device id            */
    roreg32 version;                        /* 0x004 - device version       */
    rwreg32 sw_rst;                         /* 0x008 - program reset        */
    roreg32 global_status;                  /* 0x00C - status               */
/**************************** MGETH registers *******************************/
    roreg32 mg_status;                      /* 0x010 - mgeth status         */
    roreg32 _skip01;                        /* 0x014                        */
    rwreg32 mg_irq_mask;                    /* 0x018 - mgeth irq mask       */
    rwreg32 mg_control;                     /* 0x01C - mgeth control reg    */
    rwreg32 mg_len_mask_ch0;                /* 0x020 - mgeth mask len ch0   */
    rwreg32 mg_len_mask_ch1;                /* 0x024 - mgeth mask len ch1   */
    rwreg32 mg_len_mask_ch2;                /* 0x028 - mgeth mask len ch2   */
    rwreg32 mg_len_mask_ch3;                /* 0x02C - mgeth mask len ch3   */
    rwreg32 tx0_delay_timer;                /* 0x030 - delay timer for tx0  */
    rwreg32 tx1_delay_timer;                /* 0x034 - delay timer for tx1  */
    rwreg32 tx2_delay_timer;                /* 0x038 - delay timer for tx2  */
    rwreg32 tx3_delay_timer;                /* 0x03C - delay timer for tx3  */
    rwreg32 hd_sgmii_mode;                  /* 0x040 - SGMII mode           */
    roreg32 _skip02[47];                    /* 0x044 - 0x0FC                */
/************************** Statistc counters *******************************/
    roreg32 a_frames_received_ok;           /* 0x100                        */
    roreg64 a_octets_received_ok;           /* 0x104                        */
    roreg32 if_in_ucast_pkts;               /* 0x10C                        */
    roreg32 if_in_multicast_pkts;           /* 0x110                        */
    roreg32 if_in_broadcast_pkts;           /* 0x114                        */
    roreg32 a_frame_check_sequence_errors;  /* 0x118                        */
    roreg32 if_in_errors;                   /* 0x11C                        */
    roreg32 ether_stats_drop_events;        /* 0x120                        */
    roreg64 ether_stats_octets;             /* 0x124                        */
    roreg32 ether_stats_pkts;               /* 0x12C                        */
    roreg32 ether_stats_undersize_pkts;     /* 0x130                        */
    roreg32 ether_stats_oversize_pkts;      /* 0x134                        */
    roreg32 ether_stats_64_octets;          /* 0x138                        */
    roreg32 ether_stats_65_127_octets;      /* 0x13C                        */
    roreg32 ether_stats_128_255_octets;     /* 0x140                        */
    roreg32 ether_stats_256_511_octets;     /* 0x144                        */
    roreg32 ether_stats_512_1023_octets;    /* 0x148                        */
    roreg32 ether_stats_1024_1518_octets;   /* 0x14C                        */
    roreg32 ether_stats_1519_10240_octets;  /* 0x150                        */
    roreg32 ether_stats_jabbers;            /* 0x154                        */
    roreg32 ether_stats_fragments;          /* 0x158                        */
    roreg32 _skip03[9];                     /* 0x15C - 0x17C                */
    roreg32 a_frames_transmitted_ok;        /* 0x180                        */
    roreg64 a_octets_transmitted_ok;        /* 0x184                        */
    roreg32 if_out_ucast_pkts;              /* 0x18C                        */
    roreg32 if_out_multicast_pkts;          /* 0x190                        */
    roreg32 if_out_broadcast_pkts;          /* 0x194                        */
    roreg32 _skip04[26];                    /* 0x198 - 0x1FC                */
/****************************** RX channels *********************************/
    mgeth_rx_regs rx[4];                    /* 0x200 - 0x9FC                */
/****************************** TX channels *********************************/
    mgeth_tx_regs tx[4];                    /* 0xA00 - 0xDFC                */
/******************************* Reserved ***********************************/
    roreg32 _skip05[128];                   /* 0xE00 - 0xFFC                */
}   __attribute__ ((packed)) mgeth_regs;

typedef struct __long_desc {
	unsigned int usrdata_l;
	unsigned int usrdata_h;
	unsigned int memptr;
	unsigned int flags_length;
} __attribute__((packed, aligned(16))) long_desc;

typedef struct {
	mgeth_regs *regs;
	struct eth_device *dev;
	struct phy_device *phy;
	struct phy sgmii_phy;

	struct mdma_chan *tx_chan;
	struct mdma_chan *rx_chan;

	long_desc *rxbd_base;
	long_desc *txbd_base;
    int rxbd_no;

	void *rxbuf_base;
	void *txbuf_base;

	unsigned int speed;
	unsigned int duplex;
	unsigned int link;

	unsigned char *buffer;

	unsigned char dev_addr[ETH_ALEN];
} mgeth_priv;

#define CTRL_FD_S 0
#define CTRL_FD_M 0x00000001
#define CTRL_SPEED_S 1
#define CTRL_SPEED_M 0x00000006

/* Ajust phy link to mgeth settings */
static void mgeth_link_event(mgeth_priv *priv)
{
	struct phy_device *phydev = priv->phy;
	mgeth_regs *regs = priv->regs;
	int status_change = 0;
	unsigned int val;

	dev_dbg(priv->dev, DBGPREFIX "link_event\n");

	if (phydev->link) {
		if ((priv->speed != phydev->speed) ||
		    (priv->duplex != phydev->duplex)) {
			priv->speed = phydev->speed;
			priv->duplex = phydev->duplex;
            status_change = 1;
		}
        val = ioread32(&regs->mg_control);
        val &= ~(CTRL_FD_M | CTRL_SPEED_M);
        if (phydev->duplex)
            val |= CTRL_FD_M;

        if (phydev->speed == SPEED_1000)
            val |= 2 << CTRL_SPEED_S;
        else if (phydev->speed == SPEED_100)
            val |= 1 << CTRL_SPEED_S;
        iowrite32(val, &regs->mg_control);
        dev_dbg(priv->dev, DBGPREFIX "write %x to %p\n", val,
            &regs->mg_control);
	}

	if (phydev->link != priv->link) {
		if (!phydev->link) {
			priv->duplex = -1;
			priv->speed = 0;
		}

		priv->link = phydev->link;
		dev_dbg(priv->dev, DBGPREFIX "changing link to %d\n",
			priv->link);
		status_change = 1;
	}

	if (status_change) {
        if(priv->link)
        {
			dev_info(priv->dev,
				DBGPREFIX
				"phy link at %d %s duplex\n",
				priv->speed, priv->duplex?"full":"half");
        }
        else
        {
			dev_info(priv->dev, DBGPREFIX
				"link down\n");
        }
	}
}

static int mgeth_config_phy(struct udevice *dev)
{
	mgeth_priv *priv = dev_get_priv(dev);

	int ret = generic_phy_get_by_index(dev, 0, &priv->sgmii_phy);
	if (ret && ret != -ENOENT) {
		dev_err(dev, DBGPREFIX "unable find SGMII PHY\n");
		return ret;
	}
	ret = generic_phy_init(&priv->sgmii_phy);
	if (ret) {
		dev_err(dev, DBGPREFIX "unable init SGMII PHY\n");
		return ret;
	}

	priv->phy = dm_eth_phy_connect(dev);
	if (!priv->phy) {
		dev_err(dev, DBGPREFIX "unable to configure PHY\n");
		return -EINVAL;
	}
	return phy_config(priv->phy);
}

static int mgeth_reset(mgeth_priv *priv)
{
    int ret = 0;
	iowrite32(MGETH_ENABLE, &priv->regs->sw_rst);
	ret = wait_for_bit_le32((void *)&priv->regs->sw_rst, BIT(1), false,
				CONFIG_SYS_HZ * 2, false);
	if (ret < 0) {
		dev_err(dev, DBGPREFIX "Unable to reset\n");
		return ret;
	}
    return ret;
}

static int mgeth_probe(struct udevice *dev)
{
	mgeth_priv *priv = dev_get_priv(dev);
	mgeth_regs *regs = priv->regs;
	int ret;

	dev_dbg(dev, DBGPREFIX "probe %p, %p\n",
		&regs->rx[1].a_frames_received_ok, &regs->tx[0]);

	if (ioread32(&regs->id) != MGETH_ID ||
	    ioread32(&regs->version) != MGETH_VER) {
		dev_err(dev,
			DBGPREFIX
			"detected illegal version of MGETH core: 0x%08x 0x%08x\n",
			ioread32(&regs->id), ioread32(&regs->version));
		return -ENOTSUPP;
	}

    ret = mgeth_reset(priv);
    
    if(ret)
        return ret;

    iowrite32(0, &regs->mg_irq_mask); // disable inerrupts

	return mgeth_config_phy(dev);
}

static void init_descr(mgeth_priv *priv)
{
	int i;

	if (!priv->rxbd_base) {
		/* allocate descriptors */
		priv->rxbd_base = (long_desc *)memalign(
			MGETH_BD_POLL_ALIGN,
			MGETH_RXBD_CNT * sizeof(long_desc));
		priv->txbd_base = (long_desc *)memalign(
			MGETH_BD_POLL_ALIGN,
			MGETH_TXBD_CNT * sizeof(long_desc));

		memset(priv->rxbd_base, 0, MGETH_RXBD_CNT * sizeof(long_desc));
		memset(priv->txbd_base, 0, MGETH_RXBD_CNT * sizeof(long_desc));

		/* allocate buffers to all descriptors  */
		priv->rxbuf_base = malloc(MGETH_RXBUF_SIZE * MGETH_RXBD_CNT);

		if (!priv->rxbuf_base)
			dev_err(priv->phy->dev, DBGPREFIX
				"Unable to allocate enough rx descriptor buffers\n");

		priv->txbuf_base = malloc(MGETH_MIN_PACKET_LEN);

		if (!priv->txbuf_base)
			dev_err(priv->phy->dev, DBGPREFIX
				"Unable to allocate enough tx buffers\n");

		dev_dbg(priv->phy->dev,
			DBGPREFIX
			"rxbd_base: 0x%p, txbd_base:0x%p, rxbuf_base: 0x%p\n",
			priv->rxbd_base, priv->txbd_base, priv->rxbuf_base);
	}

	/* initate rx decriptors */
	for (i = 0; i < MGETH_RXBD_CNT; i++) {
		priv->rxbd_base[i].usrdata_h = 0;
		priv->rxbd_base[i].usrdata_l = 0;
		/* enable desciptor & set wrap last to first descriptor */
		if (i >= (MGETH_RXBD_CNT - 1)) {
			priv->rxbd_base[i].flags_length =
				MGETH_BD_LINK; // link descriptor
			priv->rxbd_base[i].memptr =
				(unsigned int)&priv->rxbd_base[0];
		} else {
			priv->rxbd_base[i].flags_length =
				MGETH_RXBUF_SIZE; // usual descriptor
            priv->rxbd_base[i].memptr =
                (unsigned int)(priv->rxbuf_base +
				       (MGETH_RXBUF_SIZE * i));	
    	}
	}

	/* initiate indexes */
    priv->rxbd_no = 0;

	/* initate tx decriptor */
	priv->txbd_base[0].usrdata_h = 0;
	priv->txbd_base[0].usrdata_l = 0;
	priv->txbd_base[0].memptr = 0;
	priv->txbd_base[0].flags_length = MGETH_BD_STOP;

	flush_cache((unsigned long)&priv->rxbd_base[0],
		    sizeof(long_desc) * MGETH_RXBD_CNT);
	flush_cache((unsigned long)&priv->txbd_base[0],
		    sizeof(long_desc) * MGETH_TXBD_CNT);

	/* Set pointer to rx descriptor areas */

	iowrite32(MGETH_CHAN_DESC_LONG | MGETH_CHAN_ADD_INFO |
			  (sizeof(long_desc) << MGETH_CHAN_DESC_GAP_SHIFT),
		  &priv->regs->rx[0].settings);
	iowrite32((unsigned int)&priv->rxbd_base[0],
		  &priv->regs->rx[0].desc_addr);

	dev_dbg(priv->phy->dev,
		DBGPREFIX
		"&priv->regs->tx[0].desc_addr: 0x%p, &priv->regs->tx[0].settings: 0x%p\n",
		&priv->regs->tx[0].desc_addr, &priv->regs->tx[0].settings);
}

static void mgeth_set_packet_filter(mgeth_priv *priv)
{
	dev_dbg(priv->phy->dev, DBGPREFIX "Set RX filter to %x:%x:%x:%x:%x:%x\n",
	       priv->dev_addr[0], priv->dev_addr[1], priv->dev_addr[2],
	       priv->dev_addr[3], priv->dev_addr[4], priv->dev_addr[5]);
	iowrite32(priv->dev_addr[0] | priv->dev_addr[1] << 8 |
			  priv->dev_addr[2] << 16 | priv->dev_addr[3] << 24,
		  &priv->regs->rx[0].rx_eth_mask_value[0]);
	iowrite32(priv->dev_addr[4] | priv->dev_addr[5] << 8,
		  &priv->regs->rx[0].rx_eth_mask_value[1]);
	iowrite32(0xffffffff, &priv->regs->rx[0].rx_eth_mask_activ[0]);
	iowrite32(0x0000ffff, &priv->regs->rx[0].rx_eth_mask_activ[1]);
	iowrite32(6, &priv->regs->mg_len_mask_ch0);
}

/* init/start hardware and allocate descriptor buffers for rx side
 *
 */
static int mgeth_start(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	mgeth_priv *priv = dev_get_priv(dev);
	int ret;

	dev_dbg(dev, DBGPREFIX "start called\n");

	/* Load current MAC address */
	memcpy(priv->dev_addr, pdata->enetaddr, ETH_ALEN);

    ret = mgeth_reset(priv);
    
    if(ret)
        return ret;

    iowrite32(0, &priv->regs->mg_irq_mask); // disable interrupts

	// set filters for given MAC
	mgeth_set_packet_filter(priv);

	ret = generic_phy_power_on(&priv->sgmii_phy);
	if (ret)
		return ret;

	// startup PHY
	ret = phy_startup(priv->phy);
	if (!priv->phy->link)
		printf("%s: No link\n", priv->phy->dev->name);

	mgeth_link_event(priv);

	// setup RX/TX queues
	init_descr(priv);

	// enable rx
	iowrite32(MGETH_ENABLE, &priv->regs->rx[0].enable);

	return ret;
}

/* Stop the hardware from looking for packets - may be called even if
 *	 state == PASSIVE
 */
static void mgeth_stop(struct udevice *dev)
{
	mgeth_priv *priv = dev_get_priv(dev);

	dev_dbg(dev, DBGPREFIX "stop called\n");

	iowrite32(0, &priv->regs->rx[0].enable);
	iowrite32(0, &priv->regs->tx[0].enable);
}

/* Send the bytes passed in "packet" as a packet on the wire */
static int mgeth_send(struct udevice *dev, void *eth_data, int data_length)
{
	mgeth_priv *priv = dev_get_priv(dev);
	int ret;

	dev_dbg(dev, DBGPREFIX "send called %d\n", data_length);

	// copy to internal buffer if size less than min
	if (data_length < MGETH_MIN_PACKET_LEN) {
		dev_dbg(dev, DBGPREFIX "copy data to internal buffer\n");
		memset(priv->txbuf_base, 0, MGETH_MIN_PACKET_LEN);
		memcpy(priv->txbuf_base, eth_data, data_length);
		data_length = MGETH_MIN_PACKET_LEN;
		eth_data = priv->txbuf_base;
	}

	dev_dbg(dev,
		DBGPREFIX
		"global_status: %x, status %x, desc_status %x, rx desc_status %x (%x)\n",
		ioread32(&priv->regs->global_status),
		ioread32(&priv->regs->mg_status),
		ioread32(&priv->regs->tx[0].status),
		ioread32(&priv->regs->rx[0].status),
		ioread32(&priv->regs->rx[0].curr_desc_addr));

#ifdef DETAILED_DEBUG
	dev_dbg(dev, DBGPREFIX "transmitted: %d:%d\n",
		ioread32(&priv->regs->a_frames_transmitted_ok),
		ioread32(&priv->regs->a_frame_check_sequence_errors));

	dev_dbg(dev, DBGPREFIX "curr_descr: %x\n",
		ioread32(&priv->regs->tx[0].curr_desc_addr));

	dev_dbg(dev, DBGPREFIX "rx curr_descr: %x\n",
		ioread32(&priv->regs->rx[0].curr_desc_addr));

	dev_dbg(dev, DBGPREFIX "axierr descr: %x:%x\n",
		ioread32(&priv->regs->tx[0].desc_raxi_err_addr),
		ioread32(&priv->regs->tx[0].desc_waxi_err_addr));

	dev_dbg(dev, DBGPREFIX "rx axierr descr: %x:%x\n",
		ioread32(&priv->regs->rx[0].desc_raxi_err_addr),
		ioread32(&priv->regs->rx[0].desc_waxi_err_addr));

	dev_dbg(dev, DBGPREFIX "axierr: %x\n",
		ioread32(&priv->regs->tx[0].waxi_err_addr));

	dev_dbg(dev, DBGPREFIX "rx axierr: %x\n",
		ioread32(&priv->regs->rx[0].waxi_err_addr));
#endif

	// wait for completion of previous transfer
	ret = wait_for_bit_le32((void *)&priv->regs->tx[0].enable, BIT(1),
				false, CONFIG_SYS_HZ * 2, false);
	if (ret < 0) {
		dev_dbg(dev,
			DBGPREFIX
			"WARNING: Failed wait for tx transaction end (ret = %d)!\n",
			ret);
		return -EIO;
	}

	priv->txbd_base[0].flags_length = MGETH_BD_STOP | data_length;
	priv->txbd_base[0].memptr = (unsigned int)eth_data;
	flush_cache((unsigned long)&priv->txbd_base[0], sizeof(long_desc));
	flush_cache((unsigned long)eth_data, data_length);

	// enable tx
	dev_dbg(dev, DBGPREFIX "Enable TX at %p\n", &priv->regs->tx[0].enable);
	iowrite32(MGETH_CHAN_DESC_LONG | MGETH_CHAN_ADD_INFO |
			  (sizeof(long_desc) << MGETH_CHAN_DESC_GAP_SHIFT),
		  &priv->regs->tx[0].settings);
	iowrite32((unsigned int)&priv->txbd_base[0],
		  &priv->regs->tx[0].desc_addr);
	iowrite32(MGETH_ENABLE, &priv->regs->tx[0].enable);

	return 0;
}

/* Check if the hardware received a packet. If so, set the pointer to the
 *	 packet buffer in the packetp parameter. If not, return an error or 0 to
 *	 indicate that the hardware receive FIFO is empty. If 0 is returned, the
 *	 network stack will not process the empty packet, but free_pkt() will be
 *	 called if supplied
 */

static void invalidate_dcache(unsigned long start, unsigned long size)
{
	unsigned long aligned_start = start & ~(CONFIG_SYS_CACHELINE_SIZE - 1);
	unsigned long aligned_end =
		((start + size) & ~(CONFIG_SYS_CACHELINE_SIZE - 1)) +
		CONFIG_SYS_CACHELINE_SIZE;
	invalidate_dcache_range(aligned_start, aligned_end);
}

static int mgeth_recv(struct udevice *dev, int flags, unsigned char **packetp)
{
	mgeth_priv *priv = dev_get_priv(dev);
	// mgeth_regs *regs = priv->regs;
	long_desc *curr_bd = &priv->rxbd_base[priv->rxbd_no];
	int len;

	invalidate_dcache((unsigned long)curr_bd, sizeof(long_desc));
	if (!(curr_bd->flags_length & MGETH_BD_OWN))
		return -1;

    int curr_no = priv->rxbd_no;

	if (priv->rxbd_no == 0) // start again
	{
		// update link descriptor flags
		dev_dbg(dev, DBGPREFIX "update link descriptor\n");
		priv->rxbd_base[MGETH_RXBD_CNT - 1].flags_length =
			MGETH_BD_LINK; // link descriptor
		flush_cache((unsigned long)&priv->rxbd_base[MGETH_RXBD_CNT - 1],
			    sizeof(long_desc));
	}

	// increment current descriptor
	priv->rxbd_no++;
	if (priv->rxbd_no == (MGETH_RXBD_CNT - 1))
		priv->rxbd_no = 0;

	*packetp = (priv->rxbuf_base +
				       (MGETH_RXBUF_SIZE * curr_no));

	len = curr_bd->flags_length & MGETH_MAX_PACKET_LEN;
	invalidate_dcache((unsigned long)(*packetp), len);

	dev_dbg(dev, DBGPREFIX "packet received desc %d, len %d\n",
		(*packetp - (unsigned char *)(priv->rxbuf_base)) /
			MGETH_RXBUF_SIZE,
		len);

	return len;
}

/* Give the driver an opportunity to manage its packet buffer memory
 *	     when the network stack is finished processing it. This will only be
 *	     called when no error was returned from recv
 */
static int mgeth_free_pkt(struct udevice *dev, unsigned char *packet,
			  int length)
{
	mgeth_priv *priv = dev_get_priv(dev);

	int desc_no = (packet - (unsigned char *)(priv->rxbuf_base)) /
		      MGETH_RXBUF_SIZE;

	dev_dbg(mdio_dev, DBGPREFIX "free_pkt called %d\n", desc_no);

	// reset flags
	priv->rxbd_base[desc_no].flags_length =
		MGETH_RXBUF_SIZE; // usual descriptor
    priv->rxbd_base[desc_no].memptr = (unsigned int) packet;
	flush_cache((unsigned long)&priv->rxbd_base[desc_no],
		    sizeof(long_desc));

	return 0;
}

static int mgeth_remove(struct udevice *dev)
{
	mgeth_priv *priv = dev_get_priv(dev);
	dev_dbg(mdio_dev, DBGPREFIX "remove called\n");

	mgeth_stop(dev);

	if (priv->rxbd_base)
		free(priv->rxbd_base);

	if (priv->txbd_base)
		free(priv->txbd_base);

	if (priv->rxbuf_base)
		free(priv->rxbuf_base);

	if (priv->txbuf_base)
		free(priv->txbuf_base);

	return 0;
}

static int mgeth_ofdata_to_platdata(struct udevice *dev)
{
	mgeth_priv *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_platdata(dev);
	const char *mac;
	int len;

	dev_dbg(mdio_dev, DBGPREFIX "ofdata_to_platdata called\n");

	priv->regs = (mgeth_regs *)(uintptr_t)devfdt_get_addr(dev);

	mac = fdt_getprop(gd->fdt_blob, dev_of_offset(dev), "mac-address",
			  &len);
	if (mac && is_valid_ethaddr((unsigned char *)mac)) {
		pdata->enetaddr[0] = mac[0];
		pdata->enetaddr[1] = mac[1];
		pdata->enetaddr[2] = mac[2];
		pdata->enetaddr[3] = mac[3];
		pdata->enetaddr[4] = mac[4];
		pdata->enetaddr[5] = mac[5];
	}
	return 0;
}

static const struct eth_ops mgeth_ops = {
	.start = mgeth_start,
	.send = mgeth_send,
	.recv = mgeth_recv,
	.free_pkt = mgeth_free_pkt,
	.stop = mgeth_stop,
};

static const struct udevice_id mgeth_ids[] = { { .compatible = "rcm,mgeth" },
					       {} };

U_BOOT_DRIVER(mgeth) = {
	.name = "mgeth",
	.id = UCLASS_ETH,
	.of_match = mgeth_ids,
	.ofdata_to_platdata = mgeth_ofdata_to_platdata,
	.probe = mgeth_probe,
	.remove = mgeth_remove,
	.ops = &mgeth_ops,
	.priv_auto_alloc_size = sizeof(mgeth_priv),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
};

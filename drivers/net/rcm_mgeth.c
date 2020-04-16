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
#define DEBUG
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
#include <linux/io.h>
#include <generic-phy.h>

#define MGETH_ID 0x48544547
#define MGETH_VER 0x01900144

typedef const volatile unsigned int roreg32;
typedef volatile unsigned int rwreg32;
typedef const volatile unsigned long long roreg64;
typedef volatile unsigned long long rwreg64;

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
} mgeth_rx_regs;

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
} mgeth_tx_regs;

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
} mgeth_regs;

typedef struct {
	mgeth_regs *regs;
	struct eth_device *dev;
	struct phy_device *phy;
    struct phy sgmii_phy;

    unsigned int speed;
    unsigned int duplex;
    unsigned int link;

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
    mgeth_regs* regs = priv->regs; 
	int status_change = 0;
	unsigned int val;

    dev_dbg(priv->dev, DBGPREFIX "link_event\n");

    if(phydev->link)
    {
		if ((priv->speed != phydev->speed) ||
		    (priv->duplex != phydev->duplex)) {            
            val = ioread32(&regs->mg_control);
            val &= ~(CTRL_FD_M | CTRL_SPEED_M);
            if(phydev->duplex)
                val |= CTRL_FD_M;

            if(phydev->speed == SPEED_1000)
                val |= 2 << CTRL_SPEED_S;
            else if(phydev->speed == SPEED_100)
                val |= 1 << CTRL_SPEED_S;
            iowrite32(val, &regs->mg_control);
            priv->speed = phydev->speed;
            priv->duplex = phydev->duplex;
            dev_dbg(priv->dev, DBGPREFIX "changing speed to %d, duplex to %d\n", priv->speed, priv->duplex);
        }
    }

	if (phydev->link != priv->link) {
		if (!phydev->link) {
			priv->duplex = -1;
			priv->speed = 0;
		}

		priv->link = phydev->link;
        dev_dbg(priv->dev, DBGPREFIX "changing link to %d\n", priv->link);
		status_change = 1;
	}

	if (status_change) {
        // todo
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
	if (!priv->phy)
    {
        dev_err(dev, DBGPREFIX "unable to configure PHY\n");
		return -EINVAL;
    }
    return phy_config(priv->phy);
}


static int mgeth_probe(struct udevice *dev)
{
    mgeth_priv *priv = dev_get_priv(dev);
    mgeth_regs *regs = priv->regs;

    dev_dbg(dev, DBGPREFIX "probe\n");

    if(ioread32(&regs->id) != MGETH_ID || ioread32(&regs->version) != MGETH_VER)
    {
        dev_err(dev, DBGPREFIX "detected illegal version of MGETH core: 0x%08x 0x%08x\n",
            ioread32(&regs->id), 
            ioread32(&regs->version));
        return -ENOTSUPP;
    }

	return mgeth_config_phy(dev);
}

static int mgeth_remove(struct udevice *dev)
{
	//mgeth_priv *priv = dev_get_priv(dev);

    dev_dbg(mdio_dev, DBGPREFIX "remove called\n");

	return 0;
}

/* init/start hardware and allocate descriptor buffers for rx side
 *
 */
static int mgeth_start(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_platdata(dev);
	mgeth_priv *priv = dev_get_priv(dev);
    int ret;

    dev_dbg(mdio_dev, DBGPREFIX "start called\n");

	/* Load current MAC address */
	memcpy(priv->dev_addr, pdata->enetaddr, ETH_ALEN);

    // ToDo: set filters for given MAC

    ret = generic_phy_power_on(&priv->sgmii_phy);
    if(ret)
        return ret;

    // setup RT/TX queues

    // startup PHY
    ret = phy_startup(priv->phy);
    if (!priv->phy->link)
        printf("%s: No link\n", priv->phy->dev->name);

    mgeth_link_event(priv);

	return ret;
}

/* Stop the hardware from looking for packets - may be called even if
 *	 state == PASSIVE
 */
static void mgeth_stop(struct udevice *dev)
{
	//mgeth_priv *priv = dev_get_priv(dev);

    dev_dbg(mdio_dev, DBGPREFIX "stop called\n");

}

/* Send the bytes passed in "packet" as a packet on the wire */
static int mgeth_send(struct udevice *dev, void *eth_data, int data_length)
{
	//mgeth_priv *priv = dev_get_priv(dev);
	//mgeth_regs *regs = priv->regs;

    dev_dbg(mdio_dev, DBGPREFIX "send called\n");

	return 0;
}

/* Check if the hardware received a packet. If so, set the pointer to the
 *	 packet buffer in the packetp parameter. If not, return an error or 0 to
 *	 indicate that the hardware receive FIFO is empty. If 0 is returned, the
 *	 network stack will not process the empty packet, but free_pkt() will be
 *	 called if supplied
 */
static int mgeth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	//mgeth_priv *priv = dev_get_priv(dev);
	//mgeth_regs *regs = priv->regs;

    dev_dbg(mdio_dev, DBGPREFIX "recv called\n");

	return 0;
}

/* Give the driver an opportunity to manage its packet buffer memory
 *	     when the network stack is finished processing it. This will only be
 *	     called when no error was returned from recv
 */
static int mgeth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	//mgeth_priv *priv = dev_get_priv(dev);
	//mgeth_regs *regs = priv->regs;

    dev_dbg(mdio_dev, DBGPREFIX "free_pkt called\n");

	return 0;
}

/* Write a MAC address to the hardware (used to pass it to Linux
 *		 on some platforms like ARM). This function expects the
 *		 eth_pdata::enetaddr field to be populated. The method can
 *		 return -ENOSYS to indicate that this is not implemented for
		 this hardware
 */
// static int mgeth_write_hwaddr(struct udevice *dev)
// {
// 	//mgeth_priv *priv = dev_get_priv(dev);
// 	//struct eth_pdata *pdata = dev_get_platdata(dev);

//     dev_dbg(mdio_dev, DBGPREFIX "write_hwaddr called\n");

// 	return 0;
// }

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
//	.write_hwaddr = mgeth_write_hwaddr,
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

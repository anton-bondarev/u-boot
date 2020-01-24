/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2019 Alexey Spirkov <alexeis@astrosoft.ru>
 */

#include <linux/err.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <configs/1888tx018.h>

#ifndef __UBOOT__
        #include <linux/module.h>
        #include <linux/slab.h>
        #include <linux/mtd/map.h>
        #include <linux/mtd/cfi.h>
        #include <linux/platform_device.h>
        #include <linux/mtd/physmap.h>
        #include <linux/of.h>
        #include <linux/mfd/syscon.h>
        #include <linux/regmap.h>
        #ifdef CONFIG_PPC_DCR
                #include <asm/dcr.h>
        #endif
#else
        typedef unsigned char uchar;
        #include <dm/of.h>
        #include <dm/device.h>
        #include <dm/of_access.h>
        #include <dm/of_addr.h>
        #include <dm/uclass.h>
        #include <regmap.h>
        #include <flash.h>
        #include <mtd/cfi_flash.h>
        #include <asm/tlb47x.h>
#endif

#define NORMC_ID_VAL_lsif               0x20524F4E
#define NORMC_ID_VAL_mcif               0x524f4e53
#define NORMC_VERSION_VAL               0x00000201

#define SRAMNOR__chip_num_i             0
#define SRAMNOR__ce_mode_i              8
#define SRAMNOR__addr_size_i            16
#define SRAMNOR__ecc_mode_i             24
#define SRAMNOR_TIMINGS                 0x1f1f0808

#define SRAMNOR_REG_id                  0x00
#define SRAMNOR_REG_version             0x04
#define SRAMNOR_REG_config              0x08
#define SRAMNOR_REG_timings             0x0c
#define SRAMNOR_REG_mask_irq            0x10
#define SRAMNOR_REG_status_irq          0x14
#define SRAMNOR_REG_ecc_err_addr        0x18
#define SRAMNOR_REG_reserve             0x1c
#define SRAMNOR_REG_management_ecc      0x20
#define SRAMNOR_REG_data_ecc_write_mem  0x24
#define SRAMNOR_REG_data_ecc_read_mem   0x28

// #define RCM_SRAM_NOR_DBG

#ifdef RCM_SRAM_NOR_DBG
        #ifndef __UBOOT__
                #define DBG_PRINT(...) printk( "[RCM_SRAM_NOR] " __VA_ARGS__ );
        #else
                #define DBG_PRINT(...) printf( "[RCM_SRAM_NOR] " __VA_ARGS__ );
        #endif
#else
        #define DBG_PRINT(...) while(0);
#endif

#ifndef __UBOOT__
        typedef struct platform_device rcm_sram_nor_device;

        #define WSWAP(B) (B)
        #define RSWAP(B) (B)

#else // is defined __UBOOT__
        typedef struct udevice rcm_sram_nor_device;

        #define WSWAP(B) cpu_to_le32(B)
        #define RSWAP(B) le32_to_cpu(B)

        #define of_property_read_bool(DEVNODE,NAME) \
                ofnode_read_bool(np_to_ofnode(DEVNODE),NAME)

        #define of_property_read_u32(DEVNODE,NAME,PVAL) \
                ofnode_read_u32(np_to_ofnode(DEVNODE),NAME,PVAL)

        #define of_property_read_u32_array(DEVNODE,NAME,PARR,PSIZE) \
                ofnode_read_u32_array(np_to_ofnode(DEVNODE),NAME,PARR,PSIZE)

        void cfi_flash_bank_setup(  u32 addr, unsigned int idx );

#ifdef CONFIG_PPC_DCR // page 982
        static u32 DCR_READ(u32 addr,u32 offs) {
                u32 res;
                addr+=offs; // mfdcrx RT, RA
                asm volatile ( "mfdcrx (%0), (%1) \n\t" :"=r"(res) :"r"((u32) addr) : );
                DBG_PRINT( "DCR_READ: %08x,%08x\n", addr, res )
                return res;
        };

        static void DCR_WRITE(u32 addr, u32 offs, u32 value) {
                addr+=offs; // mtdcrx RA, RS
                asm volatile ( "mtdcrx (%0), (%1) \n\t" ::"r"((u32) addr), "r"((u32) value) : );
                DBG_PRINT( "DCR_WRITE: %08x,%08x\n", addr, value )
        };

#endif /* CONFIG_PPC_DCR */

#endif  // is defined __UBOOT__

typedef u32 (*reg_readl_fn)(void *rcm_mtd, u32 offset);
typedef void (*reg_writel_read_fn)(void *rcm_mtd, u32 offset, u32 val);

struct rcm_mtd {
        void *regs;
#ifdef CONFIG_PPC_DCR
    #ifndef __UBOOT__
            dcr_host_t dcr_host;
    #else
            u32 dcr_host;
    #endif
#endif /* CONFIG_PPC_DCR */
        u32 high_addr;
        reg_readl_fn readl_fn;
        reg_writel_read_fn writel_fn;
#ifdef __UBOOT__
        u32 flash_base[2];
#endif
};

u32 lsif_reg_readl(void *base, u32 offset)
{
    struct rcm_mtd *rcm_mtd = (struct rcm_mtd *)base;
    return RSWAP(readl(rcm_mtd->regs + offset));
}

void lsif_reg_writel(void *base, u32 offset, u32 val)
{
    struct rcm_mtd *rcm_mtd = (struct rcm_mtd *)base;
    writel(WSWAP(val), rcm_mtd->regs + offset);
}

#ifdef CONFIG_PPC_DCR

u32 dcr_reg_readl(void *base, u32 offset)
{
    struct rcm_mtd *rcm_mtd = (struct rcm_mtd *)base;
    return DCR_READ(rcm_mtd->dcr_host, offset);
}

void dcr_reg_writel(void *base, u32 offset, u32 val)
{
    struct rcm_mtd *rcm_mtd = (struct rcm_mtd *)base;
    DCR_WRITE(rcm_mtd->dcr_host, offset, val);
}

#endif /* CONFIG_PPC_DCR */

static int rcm_controller_setup( rcm_sram_nor_device *pdev )
{
        struct rcm_mtd *rcm_mtd;
        struct device_node *of_node;
    u32 chip_num, timings, cs_mode, addr_size;
    int ret;

#ifndef __UBOOT__
        rcm_mtd = platform_get_drvdata(pdev);
        of_node = pdev->dev.of_node;
#else
        rcm_mtd = pdev->priv;
        of_node = (struct device_node*)ofnode_to_np( pdev->node );
#endif

    // get mux mode, chip-num, cs_mode and timings
    ret = of_property_read_u32(of_node, "chip-num", &chip_num);
    if (ret != 0) {
        dev_err(&pdev->dev, "chip-num must be defined\n");
        return ret;
    }
    if (chip_num > 0x3F) {
        dev_err(&pdev->dev, "chip_num must be between 0 and 0x3F\n");
        return -EINVAL;
    }

    ret = of_property_read_u32(of_node, "cs-mode", &cs_mode);
    if (ret != 0) {
        dev_err(&pdev->dev, "cs_mode must be defined\n");
        return ret;
    }
    if (cs_mode > 1) {
        dev_err(&pdev->dev, "cs-mode must be 0 or 1\n");
        return -EINVAL;
    }

    ret = of_property_read_u32(of_node, "addr-size", &addr_size);
    if (ret != 0) {
        dev_err(&pdev->dev, "addr-size must be defined\n");
        return ret;
    }
    if (addr_size < 10 || addr_size > 26) {
        dev_err(&pdev->dev, "addr-size must be between 10 and 26\n");
        return -EINVAL;
    }

    if (of_property_read_u32(of_node, "timings",
                 &timings)) {
        dev_warn(&pdev->dev, "Setup default NOR timings\n");
        timings = SRAMNOR_TIMINGS;
    }

    {
        // check id and version first
        u32 id = rcm_mtd->readl_fn(rcm_mtd, SRAMNOR_REG_id);
        u32 version = rcm_mtd->readl_fn(rcm_mtd, SRAMNOR_REG_version);
        u32 config = rcm_mtd->readl_fn(rcm_mtd, SRAMNOR_REG_config);
        DBG_PRINT("TRACE: rcm_controller_setup: id %x, ver %x, conf %x\n", id, version, config);

        if ((id != NORMC_ID_VAL_lsif && id != NORMC_ID_VAL_mcif) ||
            version != NORMC_VERSION_VAL)
            dev_warn(&pdev->dev,
                 "Check chip ID (%x) and version (%x)\n", id,
                 version);

        // configure controller
        config = (chip_num << SRAMNOR__chip_num_i) |
             (cs_mode << SRAMNOR__ce_mode_i) |
             (addr_size << SRAMNOR__addr_size_i) |
             (0 << SRAMNOR__ecc_mode_i); // switch off ECC

        rcm_mtd->writel_fn(rcm_mtd, SRAMNOR_REG_config, config);
#ifdef RCM_SRAM_NOR_DBG
        DBG_PRINT( "TRACE: rcm_controller_setup: config write %x, read %x\n",
                   config,
                   rcm_mtd->readl_fn(rcm_mtd, SRAMNOR_REG_config) );
#endif
        rcm_mtd->writel_fn(rcm_mtd, SRAMNOR_REG_reserve,
                rcm_mtd->high_addr << 28);

        // setup timings
        rcm_mtd->writel_fn(rcm_mtd, SRAMNOR_REG_timings, timings);
    }

    return 0;
}

static int rcm_mtd_probe( rcm_sram_nor_device* pdev )
{
        struct rcm_mtd *rcm_mtd;
        struct device_node *of_node;
        bool dcr_reg;

#ifndef __UBOOT__
        struct resource *ctrl;
        rcm_mtd = devm_kzalloc(&pdev->dev, sizeof(struct rcm_mtd), GFP_KERNEL);
        of_node = pdev->dev.of_node;
#else
        rcm_mtd = calloc( 1,  sizeof(struct rcm_mtd) );
        of_node = (struct device_node*)ofnode_to_np( pdev->node );
#endif
        if( !rcm_mtd )
                return -ENOMEM;

#ifndef __UBOOT__
        platform_set_drvdata( pdev, rcm_mtd );
#else
        pdev->priv = rcm_mtd;
#endif

        dcr_reg = of_property_read_bool( of_node, "dcr-reg" );

        if( dcr_reg ) {
#ifdef CONFIG_PPC_DCR
    #ifndef __UBOOT__
        const u32 *ranges;
        u32 rlen;
        phys_addr_t phys_addr =
            dcr_resource_start(pdev->dev.of_node, 0);
        rcm_mtd->dcr_host =
            dcr_map(pdev->dev.of_node, phys_addr,
                dcr_resource_len(pdev->dev.of_node, 0));
        ranges = of_get_property(of_node, "ranges", &rlen);
        if (!ranges || rlen < 2) {
            dev_err(&pdev->dev, "memory ranges must be defined\n");
            return -ENOENT;
        }
    #else
        u32 dcr_reg[2];
        u32 ranges[4];
        if( of_property_read_u32_array( of_node, "dcr-reg", dcr_reg, 2 ) ) {
            dev_err(&pdev->dev, "read dcr-rev property failed\n");
            return -ENOENT;
        }
        DBG_PRINT( "dcr-reg=%08x,%08x\n", dcr_reg[0], dcr_reg[1] )
        rcm_mtd->dcr_host = dcr_reg[0];
        if( of_property_read_u32_array( of_node, "ranges", ranges, 4 ) ) {
            dev_err(&pdev->dev, "read memory ranges failed\n");
            return -ENOENT;
        }
        DBG_PRINT( "addr-high=%08x\n", ranges[1] )
    #endif
        rcm_mtd->high_addr =
            ranges[1]; // save high addr of ranges for controller setup
        rcm_mtd->readl_fn = &dcr_reg_readl;
        rcm_mtd->writel_fn = &dcr_reg_writel;
#else // !CONFIG_PPC_DCR
        dev_err(&pdev->dev, "DCR functionality not avaliable\n");
        return -ENOENT;
#endif
    } else {
#ifndef __UBOOT__
        ctrl = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!ctrl) {
            dev_err(&pdev->dev, "failed to get control resource\n");
            return -ENOENT;
        }
        rcm_mtd->regs = devm_ioremap_resource(&pdev->dev, ctrl);
        if (IS_ERR(rcm_mtd->regs))
            return PTR_ERR(rcm_mtd->regs);
#else
                u32 reg[2];
                if( of_property_read_u32_array( of_node, "reg", reg, 2 ) ) {
                        dev_err( &pdev->dev, "failed to get control resource\n" );
                        return -ENOENT;
                }
                rcm_mtd->regs = (void*)reg[0];
                rcm_mtd->high_addr = 0;
#endif
                rcm_mtd->readl_fn = &lsif_reg_readl;
                rcm_mtd->writel_fn = &lsif_reg_writel;
        }

        if (rcm_controller_setup(pdev)) {
                dev_err(&pdev->dev, "hw setup failed\n");
                return -ENXIO;
        }

#ifdef __UBOOT__

        if( of_property_read_u32_array( of_node, "mmap_flash_base", rcm_mtd->flash_base, dcr_reg ? 2 : 1 ) ) {
                dev_err(&pdev->dev, "failed to get resource for plb remap\n");
                return -ENOENT;
        }

        if( dcr_reg ) { // flash connect via MCIF
                tlb47x_inval( rcm_mtd->flash_base[0], TLBSID_256M );
                tlb47x_map( 0x0600000000ull, rcm_mtd->flash_base[0], TLBSID_256M, TLB_MODE_RWX );
                tlb47x_inval( rcm_mtd->flash_base[1], TLBSID_256M );
                tlb47x_map( 0x0610000000ull, rcm_mtd->flash_base[1], TLBSID_256M, TLB_MODE_RWX );
        }
        else { // flash connect via LSIF
                tlb47x_inval(  rcm_mtd->flash_base[0], TLBSID_256M );
                tlb47x_map( 0x1020000000ull,  rcm_mtd->flash_base[0], TLBSID_256M, TLB_MODE_RWX );
        }
#endif

        dev_info(&pdev->dev, "registered\n");
        return 0;
}

#ifndef __UBOOT__

static int rcm_mtd_remove(struct platform_device *pdev)
{
    return 0;
}

static const struct of_device_id rcm_mtd_match[] = {
    { .compatible = "rcm,sram-nor" },
    {},
};
MODULE_DEVICE_TABLE(of, rcm_mtd_match);

static struct platform_driver rcm_mtd_driver = {
    .probe = rcm_mtd_probe,
    .remove = rcm_mtd_remove,
    .driver =
        {
            .name = "rcm-sram-nor",
            .of_match_table = rcm_mtd_match,
        },
};

module_platform_driver(rcm_mtd_driver);

#else

static const struct udevice_id rcm_sram_nor_ids[] = {
        { .compatible = "rcm,sram-nor" },
        { /* end of list */ }
};

U_BOOT_DRIVER(rcm_sram_nor) = {
        .name = "rcm-sram-nor",
        .id = UCLASS_MTD,
        .of_match = rcm_sram_nor_ids,
        .probe = rcm_mtd_probe
};

void rcm_sram_nor_init( void ) {
        struct udevice *dev;
        int ret;

        ret = uclass_get_device_by_driver( UCLASS_MTD,
                                           DM_GET_DRIVER(rcm_sram_nor),
                                           &dev );
        if( ret && ret != -ENODEV ) {
                dev_err(&pdev->dev, "Failed to initialize RCM SRAM NOR,error=%d\n", ret );
        }
}

#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexey Spirkov <alexeis@astrosoft.ru>");
MODULE_DESCRIPTION("RCM SoC SRAM/NOR controller driver");


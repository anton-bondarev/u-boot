/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2019 Alexey Spirkov <alexeis@astrosoft.ru>
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/io.h> 

#ifndef __UBOOT__
        #include <linux/module.h>
        #include <linux/of.h>
        #include <linux/platform_device.h>
        #include <linux/mfd/syscon.h>
        #include <linux/regmap.h>
        #ifdef CONFIG_PPC_DCR
                #include <asm/dcr.h>
        #endif
#else
        #include <dm/of.h>
        #include <dm/device.h>
        #include <dm/of_access.h>
        #include <dm/of_addr.h>
        #include <dm/uclass.h>
        #include <regmap.h>
#endif

#define CONTROL_REG_EXT_MEM_MUX 0x30
#define SRAM_NOR_CTRL           0x1C
#define CONTROL_REG_ID          0x0
#define CONTROL_REG_CE_MANAGE   0x04
#define EM2_PLB6MCIF2_DCR_BASE  0x80160000
#define EM3_PLB6MCIF2_DCR_BASE  0x80180000
#define LSIF0_ID                0x3046494c

#ifdef CONFIG_PPC_DCR
// briges magic

static inline void mtdcrx( uint32_t const addr, uint32_t const wval ) // потом убрать,повтор
{
    asm volatile
    (
        "mtdcrx %0, %1 \n\t"
        ::"r"(addr), "r"(wval)
    );
}

static inline uint32_t mfdcrx( uint32_t const addr ) // потом убрать,повтор
{
    uint32_t rval=0;
    asm volatile
    (
        "mfdcrx %0, %1 \n\t"
        :"=r"(rval)
        :"r"(addr)
    );
    return rval;
}

static void plb6mcif_initbridge(void)
{
        mtdcrx(EM2_PLB6MCIF2_DCR_BASE + 0x0f, 0x00000000);
        mtdcrx(EM2_PLB6MCIF2_DCR_BASE + 0x10, 0x00000004);
        mtdcrx(EM2_PLB6MCIF2_DCR_BASE + 0x11, 0x40009001);
        mtdcrx(EM2_PLB6MCIF2_DCR_BASE + 0x09, 0x400000f1);

        mtdcrx(EM3_PLB6MCIF2_DCR_BASE + 0x0f, 0x00000000);
        mtdcrx(EM3_PLB6MCIF2_DCR_BASE + 0x10, 0x00000006);
        mtdcrx(EM3_PLB6MCIF2_DCR_BASE + 0x11, 0x60009001);
        mtdcrx(EM3_PLB6MCIF2_DCR_BASE + 0x09, 0x400000f1);
}

static void plb6mcif_test_read( void ) { // потом убрать
        uint32_t read_val = mfdcrx( EM2_PLB6MCIF2_DCR_BASE + 0x04 );
        printf( "EM2_PLB6MCIF2_DCR_BASE-INTR_EN: %08x\n", read_val );
        read_val = mfdcrx( EM3_PLB6MCIF2_DCR_BASE + 0x04 );
        printf( "EM3_PLB6MCIF2_DCR_BASE-INTR_EN: %08x\n", read_val );
} // 0xFFE0_01C0

#endif

#ifndef __UBOOT__
        typedef struct platform_device rcm_mtd_arbiter_device;
        #define REGMAP_WRITE(MAP,REG,VAL) regmap_write(MAP,REG,VAL)
        #define REGMAP_READ(MAP,REG,VAL) regmap_read(MAP,REG,VAL)
#else
        #define RCM_ARB_DBG
        #ifdef RCM_ARB_DBG
                #define DBG_PRINT(...) printf( "[RCM_ARB] " __VA_ARGS__ );
        #else
                #define DBG_PRINT(...) while(0);
        #endif

        typedef struct udevice rcm_mtd_arbiter_device;

        static int REGMAP_WRITE( struct regmap* map, unsigned int reg, unsigned int val ) {
                return regmap_write( map, reg, cpu_to_le32( val ) );
        }

        static int REGMAP_READ( struct regmap* map, unsigned int reg, unsigned int* val ) {
                unsigned int retval;
                int ret = regmap_read( map, reg, &retval );
                if( ret == 0 )
                        *val = le32_to_cpu( retval );
                return ret;
        }

        #define of_property_read_u32(DEVNODE,NAME,PVAL) \
                ofnode_read_u32(np_to_ofnode(DEVNODE),NAME,PVAL)

        #define of_property_read_u32_array(DEVNODE,NAME,PARR,PSIZE) \
                ofnode_read_u32_array(np_to_ofnode(DEVNODE),NAME,PARR,PSIZE)

        static struct device_node* of_property_read_regmap( struct device_node* of_node, const char* name, struct regmap* regmap ) {
                u32 reg[2];
                int ret = of_property_read_u32_array(of_node, name, reg, 2 );
                if( ret == 0 ) {
                        regmap->base = (phys_addr_t)reg[0];
                        regmap->range_count = 1;
                        regmap->range = &regmap->base_range;
                        regmap->base_range.start = reg[0];
                        regmap->base_range.size = reg[1];
                        return of_node;
                }
                return NULL;
        }
#endif

static int rcm_mtd_arbiter_probe(rcm_mtd_arbiter_device *pdev)
{
#ifdef __UBOOT__
        struct regmap _control;
        struct regmap _sctl;
#endif
        struct regmap *control;
        struct regmap *sctl;
        struct device_node *of_node;
        struct device_node *tmp;
        u32 sram_nor_mux = 0;
        u32 ext_mem_mux_mode;
        u32 ce_manage;
        u32 read_val = 0;
        int ret;

#ifndef __UBOOT__
        of_node = pdev->dev.of_node;
#else
        DBG_PRINT( "rcm_mtd_arbiter_probe start\n" )
        of_node = (struct device_node*)ofnode_to_np( pdev->node );
#endif

        ret = of_property_read_u32( of_node, "sram-nor-mux", &sram_nor_mux );
        if( ret != 0 || sram_nor_mux > 1 ) {
                dev_err( &pdev->dev, "sram-nor-mux should be explicitly defined to 0 or 1\n" );
                return ret;
        }
        DBG_PRINT( "sram-nor-mux=%u\n", sram_nor_mux )

        ret = of_property_read_u32( of_node, "ce-manage", &ce_manage );
        if( ret != 0 || ce_manage > 7 ) {
                dev_err( &pdev->dev, "ce-manage should be explicitly defined between 0 to 7\n" );
                return ret;
        }
        DBG_PRINT( "ce_manage=%u\n", ce_manage )

#ifndef __UBOOT__
        tmp = of_parse_phandle( of_node, "sctl", 0 );
#else
        tmp = of_property_read_regmap( of_node, "sctl-reg", sctl = &_sctl );
#endif
        if (!tmp) {
                dev_err( &pdev->dev, "failed to find sctl register reference\n" );
                return -ENOENT;
        }

#ifndef __UBOOT__
        sctl = syscon_node_to_regmap( tmp );
#else
        DBG_PRINT( "sctl: start=%08x,size=%08x\n", (u32)sctl->base_range.start, (u32)sctl->base_range.size )
#endif

        if( sram_nor_mux == 1 ) {                                                       // конфигурация для mcif
#ifdef CONFIG_PPC_DCR
                plb6mcif_initbridge();
#endif
                ret = REGMAP_WRITE( sctl, SRAM_NOR_CTRL, ce_manage << 1 | 1 );          // стр.922
                if( ret != 0 ) {
                        dev_err( &pdev->dev, "Write SCTL register error %i\n", ret );
                        return ret;
                }

                ret = REGMAP_READ( sctl, SRAM_NOR_CTRL, &read_val );                    // потом убрать
                if( ret != 0 || read_val != ( ce_manage << 1 | 1 ) ) {
                        dev_err( &pdev->dev, "Bad SCTL register check %08x\n", read_val );
                        return -EINVAL; //?
                }
        }
        else {                                                                          // конфигурация для lsif0
                if( ce_manage != 7 )
                        ce_manage = 6 - ce_manage;
#ifndef __UBOOT__
                tmp = of_parse_phandle( of_node, "control", 0 );
#else
                tmp = of_property_read_regmap( of_node, "lsif-reg", control = &_control );
#endif
                if (!tmp) {
                        dev_err( &pdev->dev, "failed to find control register reference\n" );
                        return -ENOENT;
                }
#ifndef __UBOOT__
                control = syscon_node_to_regmap(tmp);
#else
                DBG_PRINT( "lsif:start=%08x,size=%08x\n", (u32)control->base_range.start, (u32)control->base_range.size );
#endif
                ret = REGMAP_WRITE( sctl, SRAM_NOR_CTRL, 0 );
                if( ret != 0 ) {
                        dev_err( &pdev->dev, "Write SCTL register error %i\n", ret );
                        return ret;
                }

                ret = of_property_read_u32( of_node, "ext-mem-mux-mode", &ext_mem_mux_mode );
                if( ret == 0 ) {
                        if( ext_mem_mux_mode < 0 || ext_mem_mux_mode > 2 ) {
                                dev_err( &pdev->dev, "Illegal ext-mem-mux-mode shuld be between 0 and 2\n" );
                                return -EINVAL;
                        }
                        ret = REGMAP_WRITE( control, CONTROL_REG_EXT_MEM_MUX, ext_mem_mux_mode ); // стр.913
                        if (ret != 0) {
                                dev_err( &pdev->dev, "Write MII_MUX register error %i\n", ret );
                                return ret;
                        }
                }

                ret = REGMAP_READ( control, CONTROL_REG_ID, &read_val ); // потом убрать
                if( ret != 0 || read_val != LSIF0_ID ) {
                        dev_err( &pdev->dev, "Bad lsif0 id read (%08x)\n", read_val );
                        return -EINVAL; //?
                }

                ret = REGMAP_WRITE( control, CONTROL_REG_CE_MANAGE, ce_manage );
                if( ret != 0 ) {
                        dev_err( &pdev->dev, "Write MII_MUX register error %i\n", ret );
                        return ret;
                }

                ret = REGMAP_READ( control, CONTROL_REG_CE_MANAGE, &read_val ); // потом убрать
                if( ret != 0 || read_val != ce_manage ) {
                        dev_err( &pdev->dev, "Bad ce_manage check (%08x)\n", read_val );
                        return -EINVAL; //?
                }
        }
        dev_info( &pdev->dev, "External memory signals routed to %s\n", sram_nor_mux ? "MCIF" : "LSIF0" );

        if( ce_manage == 7 )
                dev_info( &pdev->dev, "SRAM/NOR chip selects disabled\n" );
        else
                dev_info( &pdev->dev, "%d SRAM/NOR chip selects allocated to first controller\n", sram_nor_mux ? 6 - ce_manage : ce_manage );

        return 0;
}

#ifndef __UBOOT__

static int rcm_mtd_arbiter_remove(rcm_mtd_arbiter_device *pdev)
{
        return 0;
}

static const struct of_device_id rcm_mtd_arbiter_match[] = {
        { .compatible = "rcm,sram-nor-arbiter" },
        {},
};
MODULE_DEVICE_TABLE(of, rcm_mtd_match);

static struct platform_driver rcm_mtd_driver = {
        .probe = rcm_mtd_arbiter_probe,
        .remove = rcm_mtd_arbiter_remove,
        .driver = {
                .name = "rcm-sram-nor-arbiter",
                .of_match_table = rcm_mtd_arbiter_match,
        },
};

module_platform_driver(rcm_mtd_driver);

#else

static const struct udevice_id rcm_mtd_arbiter_ids[] = {
        { .compatible = "rcm,sram-nor-arbiter" },
        { /* end of list */ }
};

U_BOOT_DRIVER(rcm_mtd_arbiter) = {
        .name = "rcm-sram-nor-arbiter",
        .id = UCLASS_MISC,
        .of_match = rcm_mtd_arbiter_ids,
        .probe = rcm_mtd_arbiter_probe
};

void rcm_mtd_arbiter_init( void ) {
        struct udevice *dev;
        int ret;

        ret = uclass_get_device_by_driver( UCLASS_MISC,
                                           DM_GET_DRIVER(rcm_mtd_arbiter),
                                           &dev );
        if( ret && ret != -ENODEV ) {
                dev_err(&pdev->dev, "Failed to initialize RCM MTD arbiter,error=%d\n", ret );
        }
}

#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexey Spirkov <alexeis@astrosoft.ru>");
MODULE_DESCRIPTION("RCM SoC SRAM/NOR controller arbiter driver");

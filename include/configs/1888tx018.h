#ifndef __1888TX018_H
#define __1888TX018_H

#include <linux/sizes.h>

/* memory map for 1888tx018 
 *      0x00040000 - 0x18 bytes romboot header
 *      0x00040018 - Start of SPL
 *      0x00076000 - Start of SPL heap 
 *      0x00078000 - End of SPL heap
 *      0x00080000 - initial stack pointer for main core
 *      
 *      0x40000000 - Start of DDR
 *      0x4D000000 - Start of U-Boot header (8Mb for U-boot itself)
 *      0x4D000040 - Start of U-Boot binary
 *      0x4E000000 - Start of U-Boot RAM (heap - 16Mb)
 *      0x4F000000 - End of U-Boot RAM (heap)
 *      0x4F100000 - Initial stack (1Mb)
 *
 */

#define CONFIG_1888TX018

/* need to define CONFIG_SPL_TEXT_BASE first because of u-boot scripts */
#define CONFIG_SPL_TEXT_BASE	0x40000
#define CONFIG_SYS_UBOOT_BASE	CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_START  CONFIG_SYS_TEXT_BASE
#define RCM_1888TX018_IM0_START           CONFIG_SPL_TEXT_BASE
#define RCM_1888TX018_IM0_SIZE            (0x40000)

#define RCM_1888TX018_SPL_STACK_SIZE 0x4000
#define CONFIG_SYS_SPL_MALLOC_SIZE  0x2000

/* stack for first processor 0x0043C000 */
#define CONFIG_SPL_STACK        (RCM_1888TX018_IM0_START + RCM_1888TX018_IM0_SIZE - RCM_1888TX018_SPL_STACK_SIZE)

#define RCM_1888TX018_SPL_SPINTABLE_SIZE 32

#define RCM_1888TX018_SPL_SPINTABLE (RCM_1888TX018_IM0_START + RCM_1888TX018_IM0_SIZE - RCM_1888TX018_SPL_SPINTABLE_SIZE)

/* stack for second processor 0x043FFFE0 */
#define RCM_1888TX018_SPL_STACK_SECONDARY   (RCM_1888TX018_SPL_SPINTABLE)


/* dual stack size for second CPU */
#define CONFIG_SYS_SPL_MALLOC_START ((CONFIG_SPL_STACK - RCM_1888TX018_SPL_STACK_SIZE*2) \
                                    - CONFIG_SYS_SPL_MALLOC_SIZE)

#ifndef CONFIG_SPL_BUILD
/*		Start address of memory area that can be used for
		initial data and stack; */        
#define CONFIG_SYS_INIT_RAM_ADDR		0x4E000000
/*      16 Megabyte for U-boot           */
#define CONFIG_SYS_INIT_RAM_SIZE		0x01000000

#define CONFIG_SYS_MONITOR_LEN  SZ_256K
#else
#define CONFIG_SYS_INIT_RAM_SIZE		CONFIG_SYS_MALLOC_F_LEN
#define CONFIG_SYS_INIT_RAM_ADDR		(CONFIG_SYS_SPL_MALLOC_START - CONFIG_SYS_INIT_RAM_SIZE) 
#endif

#define RCM_1888TX018_SPL_FDT_MAX_LEN 0x1000
#define RCM_1888TX018_SPL_ADDR_LIMIT (CONFIG_SYS_INIT_RAM_ADDR - RCM_1888TX018_SPL_FDT_MAX_LEN)


/* #define CONFIG_SPL_FRAMEWORK */

#define CONFIG_SYS_MALLOC_LEN   (2*2*1024*1024)

#define CONFIG_VERY_BIG_RAM
#define CONFIG_MAX_MEM_MAPPED   ((phys_size_t)256 << 20)
#define CONFIG_SYS_DDR_BASE     0x40000000
#define CONFIG_SYS_SDRAM_BASE   CONFIG_SYS_DDR_BASE
#define CONFIG_SYS_DDR_SIZE     SZ_2G
#define CONFIG_SYS_LOAD_ADDR    CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_MEMTEST_START	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_DDR_BASE + (CONFIG_MAX_MEM_MAPPED - 1))
/*#define CONFIG_SYS_DRAM_TEST*/

#define CONFIG_SYS_MAX_FLASH_BANKS      2 //?
#define CONFIG_SYS_MAX_FLASH_SECT       1024

#define CONFIG_CHIP_SELECTS_PER_CTRL    2
#define CONFIG_DIMM_SLOTS_PER_CTLR      2

#define CONFIG_USE_STDINT 1
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SPL_EDCL_LOAD

#define BOOT_DEVICE_NAND 10
#define BOOT_DEVICE_SPI 11
#define BOOT_DEVICE_EDCL 12

#define BOOT_ROM_HOST_MODE 0xfffc04d8
#define BOOT_ROM_MAIN 0xfffc0594

#define CONFIG_SYS_SPI_U_BOOT_OFFS      0x40000
#define CONFIG_SYS_SPI_CLK 100000000

#define CONFIG_BOOTCOMMAND "run kernelsd"
#define CONFIG_USE_BOOTARGS
#define CONFIG_BOOTARGS "console=ttyAMA0 root=/dev/mmcblk0p2 rootwait"
#define CONFIG_IPADDR 192.168.0.2
#define CONFIG_SERVERIP 192.168.0.1
#define CONFIG_NETMASK 255.255.255.0
#define CONFIG_HOSTNAME "tx011"
#define CONFIG_LOADADDR 50000000
#define CONFIG_EXTRA_ENV_SETTINGS \
        "baudrate=1000000\0" \
        "bootfile=tx011/uImage\0" \
        "bootm_low=08000000\0" \
        "bootm_size=04000000\0" \
        "fdt_addr_r=50f00000\0" \
        "fileaddr=50f00000\0" \
        "kernel=run setmem; run loadfdt; run loadkernel; bootm ${loadaddr} - ${fdt_addr_r}\0" \
        "kernelsd=run setmem; run loadsd; bootm ${loadaddr} - ${fdt_addr_r}\0" \
        "loadfdt=tftp ${fdt_addr_r} tx011/tx011.dtb\0" \
        "loadfdtsd=ext4load mmc 0:2 ${fdt_addr_r} /boot/uImage-tx011.dtb\0" \
        "loadkernel=tftp ${loadaddr} ${bootfile}\0" \
        "loadkernelsd=ext4load mmc 0:2 ${loadaddr} /boot/uImage-tx011.bin\0" \
        "loadsd=run loadfdtsd; run loadkernelsd\0" \
        "setmem=mmap drop all; mmap drop 0 1m; mmap set 0 256m 00000000; mmap set ${loadaddr} 16m 10000000\0" \
        "tftptimeout=1000\0" \
        "tftptimeoutcountmax=100\0"


#define CONFIG_PL01X_SERIAL
#define CONFIG_BAUDRATE 1000000
/*#define CONFIG_DEBUG_UART_SKIP_INIT 1*/

#define CONFIG_SYS_PBSIZE 1024

/* todo - count by freq values */
#define TIMER_TICKS_PER_US  800

#define CONFIG_USB_MUSB_PIO_ONLY

#define CONFIG_TFTP_BLOCKSIZE		1466
#define CONFIG_TFTP_TSIZE

#define CONFIG_SYS_BOOTM_LEN 0x1000000

#define CONFIG_SPD_EEPROM

#define CONFIG_MTD_PARTITIONS
#define CONFIG_SYS_MAX_NAND_DEVICE 8
#define CONFIG_SYS_NAND_MAX_CHIPS 8

#define CONFIG_CMD_UBIFS

#ifndef CONFIG_NAND
#define CONFIG_NAND 
#endif
/*
#ifndef CONFIG_CMD_NAND
#define CONFIG_CMD_NAND
#endif
*/
#ifndef CONFIG_CMD_UBI
#define CONFIG_CMD_UBI
#endif

#ifndef CONFIG_RBTREE
#define CONFIG_RBTREE
#endif

#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS

#ifndef CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MTDPARTS
#endif

#ifndef CONFIG_LZO
#define CONFIG_LZO
#endif

#define CONFIG_MTD_UBI_WL_THRESHOLD 4096
#define CONFIG_MTD_UBI_BEB_LIMIT 20

#define CONFIG_SYS_NAND_SELF_INIT

#ifdef CONFIG_ENV_IS_IN_NAND
    #define CONFIG_ENV_OFFSET               0x40000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x20000
#elif defined CONFIG_ENV_IS_IN_FLASH
    #define CONFIG_ENV_OFFSET               0x1040000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x20000
    #define CONFIG_SPL_ENV_SUPPORT
    #define CONFIG_TPL_ENV_SUPPORT
#else
    #define CONFIG_ENV_OFFSET               0x140000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x10000
#endif

#ifdef CONFIG_SPL_NAND_SUPPORT
    /*#define CONFIG_SPL_NAND_RAW_ONLY*/
    #define CONFIG_SYS_NAND_U_BOOT_OFFS     0x880000
    #define CONFIG_SYS_NAND_U_BOOT_SIZE     0x160000
#endif

#ifdef CONFIG_MTD_RCM_NOR
    /* #define CONFIG_CFI_FLASH */
    #define CONFIG_FLASH_CFI_DRIVER
    #define CONFIG_FLASH_CFI_MTD
    #define CONFIG_SYS_FLASH_CFI
    #define CONFIG_SYS_FLASH_CFI_WIDTH      FLASH_CFI_16BIT
    #define CONFIG_SYS_FLASH_BASE0          0x20000000
    #define CONFIG_SYS_FLASH_BASE1          0x28000000
    #define CONFIG_SYS_FLASH_BASE           CONFIG_SYS_FLASH_BASE0
    #define CONFIG_SYS_MONITOR_BASE         CONFIG_SYS_FLASH_BASE0
    #define CONFIG_SYS_FLASH_BANKS_LIST     { CONFIG_SYS_FLASH_BASE0, CONFIG_SYS_FLASH_BASE1 }
    #define CONFIG_SYS_FLASH_EMPTY_INFO     /* flinfo show E and/or RO */
    #define CONFIG_FLASH_SHOW_PROGRESS      100
    #define CONFIG_SYS_FLASH_USE_BUFFER_WRITE
    /*#define CONFIG_SYS_WRITE_SWAPPED_DATA*/
    /* #define CONFIG_SYS_MAX_FLASH_BANKS_DETECT not now,later */
    /* #define CONFIG_SYS_MAX_FLASH_BANKS      2 */
#endif

#endif /* __1888TX018_H */

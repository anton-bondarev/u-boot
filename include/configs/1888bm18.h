#ifndef __1888BM18_H
#define __1888BM18_H

#include <linux/sizes.h>

// ???
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
// ???
/* memory map for 1888bm18
 *	0x20000000  0x40     - U-Boot header (64B)
 *	0x20000040  0xB00000 - U-Boot binary image (11MB - 64B)
 *	0x20B00000  0x400000 - U-Boot RAM + heap (4MB)
 *	0x20F00000  0x100000 - U-Boot stack (1MB)
 *	0x21000000 0x1000000 - Kernel load space (16MB)
 *
 *	0x80000000   0x20000 - IM0 used by rumboot
 *	0x80020000   0x20000 - IM1 <- SPL
 *	0x80040000   0x20000 - IM2 rumboot & SPL stack
 */

#define CONFIG_SYS_UBOOT_BASE CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_START CONFIG_SYS_TEXT_BASE

#define RCM_1888BM18_IM1_START CONFIG_SPL_TEXT_BASE
#define RCM_1888BM18_IM1_SIZE (0x20000)

#define CONFIG_SYS_SPL_MALLOC_SIZE 0x2000
#define CONFIG_SYS_SPL_MALLOC_START (RCM_1888BM18_IM1_START + RCM_1888BM18_IM1_SIZE - CONFIG_SYS_SPL_MALLOC_SIZE)

#ifndef CONFIG_SPL_BUILD
#define CONFIG_SYS_INIT_RAM_ADDR 0x20B00000
#define CONFIG_SYS_INIT_RAM_SIZE 0x400000
#define CONFIG_SYS_MONITOR_LEN SZ_256K /* ??? SZ_11M*/
// ??? #define CONFIG_SYS_MONITOR_BASE CONFIG_SYS_TEXT_BASE
#else
#define CONFIG_SYS_INIT_RAM_SIZE CONFIG_SYS_MALLOC_F_LEN
#define CONFIG_SYS_INIT_RAM_ADDR (CONFIG_SYS_SPL_MALLOC_START - CONFIG_SYS_INIT_RAM_SIZE) 
#endif

// ??? name
#define RCM_1888TX018_SPL_FDT_MAX_LEN 0x1000
#define RCM_1888TX018_SPL_ADDR_LIMIT (CONFIG_SYS_INIT_RAM_ADDR - RCM_1888TX018_SPL_FDT_MAX_LEN)

// ???
/* #define CONFIG_SPL_FRAMEWORK */

#define CONFIG_SYS_MALLOC_LEN SZ_2M

#define CONFIG_VERY_BIG_RAM
#ifndef CONFIG_MAX_MEM_MAPPED // ???
#define CONFIG_MAX_MEM_MAPPED   ((phys_size_t)256 << 20) // ???
#endif
// ??? in Kconfig <-- #define CONFIG_SYS_DDR_BASE 0x20000000
#define CONFIG_SYS_SDRAM_BASE CONFIG_SYS_DDR_BASE
// ??? #define CONFIG_SYS_DDR_SIZE     SZ_2G
#define CONFIG_SYS_DDR_SIZE SZ_32M
#define CONFIG_SYS_LOAD_ADDR CONFIG_SYS_TEXT_BASE

// ???
#define CONFIG_SYS_MEMTEST_START	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_DDR_BASE + (CONFIG_MAX_MEM_MAPPED - 1))
/*#define CONFIG_SYS_DRAM_TEST*/

#define CONFIG_SYS_MAX_FLASH_BANKS      2       // if NOR via LSIF need 1!!! (little window)+correct mtdpart
#define CONFIG_SYS_MAX_FLASH_SECT       1024
#define MAX_SRAM_BANKS                  4       // SRAM chip select number for board mb115-02

#define CONFIG_CHIP_SELECTS_PER_CTRL    2
#define CONFIG_DIMM_SLOTS_PER_CTLR      2

#define CONFIG_USE_STDINT 1
// ??? #define CONFIG_SPL_SPI_LOAD
// ??? #define CONFIG_SPL_EDCL_LOAD

// ??? #define BOOT_DEVICE_NAND 10
// ??? #define BOOT_DEVICE_SPI 11
// ??? #define BOOT_DEVICE_EDCL 12 // ??? del
#define BOOT_DEVICE_UART 13

#define BOOT_ROM_HOST_MODE 0xfffc04d8
#define BOOT_ROM_MAIN 0xfffc0594

#define CONFIG_SYS_SPI_U_BOOT_OFFS      0x40000
#define CONFIG_SYS_SPI_CLK 100000000

#define CONFIG_SYS_FLASH_QUIET_TEST 1

#ifdef CONFIG_CMD_RCM_SRAM_NOR_TEST_AUTO // special mode, for testing only
    #define CONFIG_BOOTCOMMAND "run sramnortest"
#else
    #define CONFIG_BOOTCOMMAND "run kernelsd"
#endif

#define CONFIG_USE_BOOTARGS
#define CONFIG_BOOTARGS "console=ttyAMA0 root=/dev/mmcblk0p2 rootwait"
#define CONFIG_IPADDR 192.168.0.12
#define CONFIG_SERVERIP 192.168.0.1
#define CONFIG_NETMASK 255.255.255.0
#define CONFIG_HOSTNAME "bm18"
#define CONFIG_LOADADDR 21000000
// ??? see below
#define CONFIG_EXTRA_ENV_SETTINGS \
        "baudrate=115200\0" \
        "bootfile=bm18/uImage\0" \
        "bootm_low=21000000\0" \
        "bootm_size=01000000\0" \
        "fdt_addr_r=21FF8000\0" \
        "kernel=run loadfdt; run loadkernel; bootm ${loadaddr} - ${fdt_addr_r}\0" \
        "kernelsd=run loadsd; bootm ${loadaddr} - ${fdt_addr_r}\0" \
        "loadfdt=tftp ${fdt_addr_r} bm18/bm18.dtb\0" \
        "loadfdtsd=ext4load mmc 0:2 ${fdt_addr_r} /boot/uImage-bm18.dtb\0" \
        "loadkernel=tftp ${loadaddr} ${bootfile}\0" \
        "loadkernelsd=ext4load mmc 0:2 ${loadaddr} /boot/uImage-bm18.bin\0" \
        "loadsd=run loadfdtsd; run loadkernelsd\0" \
        "tftptimeout=1000\0" \
        "tftptimeoutcountmax=100\0" \
        "sramnortest=sramtest run rand; nortest run\0"
// ??? up sramnortest

// ??? #define CONFIG_PL01X_SERIAL
// ??? #define CONFIG_BAUDRATE 1000000
/*#define CONFIG_DEBUG_UART_SKIP_INIT 1*/

// ???
#define CONFIG_SYS_PBSIZE 1024

// ????
/* todo - count by freq values */
#define TIMER_TICKS_PER_US  200

// ??? #define CONFIG_USB_MUSB_PIO_ONLY

// ???
#ifndef CONFIG_TFTP_BLOCKSIZE
#define CONFIG_TFTP_BLOCKSIZE		1466
#endif
#define CONFIG_TFTP_TSIZE

// ???
// ??? #define CONFIG_SYS_BOOTM_LEN 0x1000000

// ??? #define CONFIG_SPD_EEPROM

// ??? here
#ifndef CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_PARTITIONS
#endif

// ??? here
#define CONFIG_SYS_MAX_NAND_DEVICE 8
#define CONFIG_SYS_NAND_MAX_CHIPS 8

// ??? #define CONFIG_CMD_UBIFS
/*
#ifndef CONFIG_NAND
#define CONFIG_NAND 
#endif
*/
/*
#ifndef CONFIG_CMD_NAND
#define CONFIG_CMD_NAND
#endif
*/
/* ??? #ifndef CONFIG_CMD_UBI
#define CONFIG_CMD_UBI
#endif*/

// ??? here
#ifndef CONFIG_RBTREE
#define CONFIG_RBTREE
#endif

/*#ifndef CONFIG_MTD_DEVICE*/
/*#define CONFIG_MTD_DEVICE*/
/*#endif*/

/* ??? #ifndef CONFIG_CMD_MTDPARTS
#define CONFIG_CMD_MTDPARTS
#endif*/

#ifndef CONFIG_LZO
#define CONFIG_LZO
#endif

// ??? #define CONFIG_MTD_UBI_WL_THRESHOLD 4096
// ??? #define CONFIG_MTD_UBI_BEB_LIMIT 20

#define CONFIG_SYS_MMC_ENV_DEV 0

/* in board/rcm/mb115-01/Kconfig now
#ifdef CONFIG_ENV_IS_IN_NAND 
    #define CONFIG_ENV_OFFSET               0x40000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x20000 
#elif defined CONFIG_ENV_IS_IN_FLASH
    #define CONFIG_ENV_OFFSET               0x1040000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x40000
    #define CONFIG_SPL_ENV_SUPPORT
    #define CONFIG_TPL_ENV_SUPPORT
#elif defined CONFIG_ENV_IS_IN_MMC
    #define CONFIG_ENV_OFFSET               0x50000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x10000
#else
    #define CONFIG_ENV_OFFSET               0x140000
    #define CONFIG_ENV_SIZE                 0x4000
    #define CONFIG_ENV_SECT_SIZE            0x10000
#endif
*/
/* ??? #ifdef CONFIG_SPL_NAND_SUPPORT
    #define CONFIG_SYS_NAND_U_BOOT_OFFS     0x880000
    #define CONFIG_SYS_NAND_U_BOOT_SIZE     0x160000
#endif*/

// ??? #define CONFIG_SYS_NAND_BASE_LIST           {1}; /*v2020.01: it's dummy!!!but need...*/

// ???
// #ifdef CONFIG_MTD_RCM_NOR
//     /* #define CONFIG_CFI_FLASH */
//     #define CONFIG_FLASH_CFI_DRIVER
//     #define CONFIG_FLASH_CFI_MTD
//     #define CONFIG_SYS_FLASH_CFI
//     #define CONFIG_SYS_FLASH_CFI_WIDTH      FLASH_CFI_16BIT
//     #define CONFIG_SYS_FLASH_EMPTY_INFO     /* flinfo show E and/or RO */
//     #define CONFIG_FLASH_SHOW_PROGRESS      100
//     #define CONFIG_SYS_FLASH_USE_BUFFER_WRITE
//     #define CONFIG_SYS_WRITE_SWAPPED_DATA
//     #ifndef CONFIG_PPC_DCR
//         #define CONFIG_PPC_DCR
//     #endif

//     #define CONFIG_SYS_FLASH_BASE0          0x20000000      /* base address 0 for work via MCIF and LSIF */
//     #define CONFIG_SYS_FLASH_BASE1          0x10000000      /* base address 1 for work via MCIF only */
//     #define CONFIG_SYS_FLASH_BANKS_LIST     {CONFIG_SYS_FLASH_BASE0,CONFIG_SYS_FLASH_BASE1}
//     #define CONFIG_SYS_FLASH_BASE           CONFIG_SYS_FLASH_BASE0
//     #define CONFIG_SYS_MONITOR_BASE         0x40000000      /* CONFIG_SYS_FLASH_BASE */

//     #ifdef CONFIG_SYS_UBOOT_BASE
//         #undef CONFIG_SYS_UBOOT_BASE
//     	#define CONFIG_SYS_UBOOT_BASE       0x20040000 
//     #endif

// #endif

#endif /* __1888BM18_H */


#ifndef __MPW7705_H
#define __MPW7705_H

#include <linux/sizes.h>

/* memory map for mpw7705 
 *      0x00040000 - 0x18 bytes romboot header
 *      0x00040018 - Start of SPL
 *      0x00076000 - Start of SPL heap 
 *      0x00078000 - End of SPL heap
 *      0x00080000 - initial stack pointer for main core
 *      
 *      0x40000000 - Start of DDR (16Mb reserved for kernel loading)
 *      0x41000000 - Start of U-Boot header (1Mb for U-boot itself)
 *      0x41000040 - Start of U-Boot binary
 *      0x41100000 - Start of U-Boot RAM (heap - 4Mb)
 *      0x41500000 - End of U-Boot RAM (heap)
 *      0x41600000 - U-Boot stack
 *
 */

#define CONFIG_MPW7705

/* need to define CONFIG_SPL_TEXT_BASE first because of u-boot scripts */
#define CONFIG_SPL_TEXT_BASE	0x40018
#define CONFIG_SYS_UBOOT_BASE	CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_START  CONFIG_SYS_TEXT_BASE
#define MPW_IM0_START           CONFIG_SPL_TEXT_BASE
#define MPW_IM0_SIZE            (0x40000 - 0x18)

#define MPW_SPL_STACK_SIZE 0x4000
#define CONFIG_SYS_SPL_MALLOC_SIZE  0x2000

#define CONFIG_SPL_STACK        (MPW_IM0_START + MPW_IM0_SIZE)

/* dual stack size for second CPU */
#define CONFIG_SYS_SPL_MALLOC_START ((CONFIG_SPL_STACK - MPW_SPL_STACK_SIZE*2) \
                                    - CONFIG_SYS_SPL_MALLOC_SIZE)

#define MPW_SPL_FDT_MAX_LEN 0x1000
#define MPW_SPL_ADDR_LIMIT (CONFIG_SYS_SPL_MALLOC_START - MPW_SPL_FDT_MAX_LEN)

/*		Start address of memory area that can be used for
		initial data and stack; */        
#define CONFIG_SYS_INIT_RAM_ADDR		0x41100000
/*      2 Megabyte for U-boot           */
#define CONFIG_SYS_INIT_RAM_SIZE		0x800000

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SYS_MONITOR_BASE CONFIG_SPL_TEXT_BASE
#else
#define CONFIG_SYS_MONITOR_BASE CONFIG_SYS_TEXT_BASE
#endif

#define CONFIG_SPL_FRAMEWORK

#define CONFIG_SYS_MALLOC_LEN   (1*1024*1024)

#define CONFIG_VERY_BIG_RAM
#define CONFIG_MAX_MEM_MAPPED   ((phys_size_t)256 << 20)
#define CONFIG_SYS_DDR_BASE     0x40000000
#define CONFIG_SYS_SDRAM_BASE   CONFIG_SYS_DDR_BASE
#define CONFIG_SYS_DDR_SIZE     SZ_2G
#define CONFIG_SYS_LOAD_ADDR    CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_MEMTEST_START	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE)
#define CONFIG_SYS_MEMTEST_END		(0x40000000u + (CONFIG_SYS_DDR_SIZE - 1))
/*#define CONFIG_SYS_DRAM_TEST*/
                     

#define CONFIG_SYS_MAX_FLASH_BANKS      0
#define CONFIG_CHIP_SELECTS_PER_CTRL    2
#define CONFIG_DIMM_SLOTS_PER_CTLR      2

#define CONFIG_USE_STDINT 1
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SPL_EDCL_LOAD

#define BOOT_DEVICE_SPI 11
#define BOOT_DEVICE_EDCL 12

#define BOOT_ROM_HOST_MODE 0xfffc0178
#define BOOT_ROM_MAIN 0xfffc0210

#define CONFIG_SYS_SPI_U_BOOT_OFFS      0x40000
#define CONFIG_SYS_SPI_CLK 100000000


#define CONFIG_ENV_OFFSET               0x140000
#define CONFIG_ENV_SIZE                 0x4000
#define CONFIG_ENV_SECT_SIZE            0x10000

#define CONFIG_PL01X_SERIAL
#define CONFIG_BAUDRATE 115200
/*#define CONFIG_DEBUG_UART_SKIP_INIT 1*/

#define CONFIG_SYS_PBSIZE 1024

/* todo - count by freq values */
#define TIMER_TICKS_PER_US  800

#define CONFIG_USB_MUSB_PIO_ONLY

#ifdef CONFIG_USB_MUSB_GADGET
#define CONFIG_USB_FUNCTION_MASS_STORAGE
#endif /* CONFIG_USB_MUSB_GADGET */

#define CONFIG_SYS_LONGHELP

#define CONFIG_TFTP_BLOCKSIZE		1466
#define CONFIG_TFTP_TSIZE

#endif /* __MPW7705_H */

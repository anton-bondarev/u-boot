#ifndef __1888BM18_H
#define __1888BM18_H

#include <linux/sizes.h>

/* Memory map for 1888bm18:
 *
 * SPL:
 *	0x80000000 0x20000 - IM0 used by rumboot
 *	0x80020000 0x20000 - IM1 <- SPL
 *	0x80040000 0x20000 - IM2 rumboot & SPL stack
 *
 * Main bootloader:
 *	0x20000000 0x1E00000 - kernel load space
 *	0x21E00000 0x40      - U-Boot header
 *	0x21E00040 0x0FFFC0  - code
 *	0x21F00000 0x80000   - data, heap etc.
 *	0x21F80000 0x80000   - initial stack
 */

#define RCM_1888BM18_IM1_START CONFIG_SPL_TEXT_BASE
#define RCM_1888BM18_IM1_SIZE (0x20000)

#define CONFIG_SYS_SPL_MALLOC_SIZE 0x2000
#define CONFIG_SYS_SPL_MALLOC_START (RCM_1888BM18_IM1_START + RCM_1888BM18_IM1_SIZE - CONFIG_SYS_SPL_MALLOC_SIZE)

#define RCM_1888BM18_SPL_FDT_MAX_LEN 0x1000
#define RCM_PPC_SPL_ADDR_LIMIT (CONFIG_SYS_SPL_MALLOC_START - RCM_1888BM18_SPL_FDT_MAX_LEN)


#define CONFIG_SYS_UBOOT_BASE CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_START CONFIG_SYS_TEXT_BASE

#ifdef CONFIG_SPL_BUILD
#define CONFIG_SYS_INIT_RAM_ADDR CONFIG_SYS_SPL_MALLOC_START // actually it is a fake value for prevent compilation errors
#define CONFIG_SYS_INIT_RAM_SIZE (RCM_1888BM18_IM1_START + RCM_1888BM18_IM1_SIZE - CONFIG_SYS_SPL_MALLOC_START) 
#else
#define CONFIG_SYS_INIT_RAM_ADDR 0x21F00000
#define CONFIG_SYS_INIT_RAM_SIZE 0x100000
#endif

#define RCM_PPC_STACK_SIZE 0x80000
#define RCM_PPC_STACK (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE) // bottom of the stack

#define CONFIG_SYS_MONITOR_LEN SZ_256K
#define CONFIG_SYS_MALLOC_LEN SZ_256K

#ifndef CONFIG_MAX_MEM_MAPPED
#define CONFIG_MAX_MEM_MAPPED ((phys_size_t)256 << 20)
#endif

// CONFIG_SYS_SDRAM_BASE is set in Kconfig
#define CONFIG_SYS_DDR_BASE CONFIG_SYS_SDRAM_BASE
#define CONFIG_SYS_LOAD_ADDR CONFIG_SYS_TEXT_BASE

#define CONFIG_SYS_MEMTEST_START (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE - RCM_PPC_STACK_SIZE) // top of the internal stack
#define CONFIG_SYS_MEMTEST_END (CONFIG_SYS_MEMTEST_START + SZ_256K - 1)

#define CONFIG_SYS_MAX_FLASH_BANKS      2       // if NOR via LSIF need 1!!! (little window)+correct mtdpart
#define CONFIG_SYS_MAX_FLASH_SECT       1024
#define MAX_SRAM_BANKS                  4       // SRAM chip select number for board mb115-02

#define CONFIG_CHIP_SELECTS_PER_CTRL    2
#define CONFIG_DIMM_SLOTS_PER_CTLR      2

#define CONFIG_SPL_XMODEM_SUPPORT // needed for CONFIG_SPL_XMODEM_EDCL_LOAD
#define CONFIG_SPL_XMODEM_EDCL_LOAD

#define BOOT_DEVICE_BOOTROM 10
#define BOOT_DEVICE_UART 11 // needed for BOOT_DEVICE_XMODEM_EDCL
#define BOOT_DEVICE_XMODEM_EDCL 12

#define CONFIG_SYS_SPI_U_BOOT_OFFS      0x40000
#define CONFIG_SYS_SPI_CLK 100000000

#define CONFIG_SYS_FLASH_QUIET_TEST 1

#define CONFIG_IPADDR 192.168.0.12
#define CONFIG_SERVERIP 192.168.0.1
#define CONFIG_NETMASK 255.255.255.0
#define CONFIG_HOSTNAME "bm18"
#define CONFIG_LOADADDR 21000000
#define CONFIG_EXTRA_ENV_SETTINGS \
	"baudrate=115200\0" \
	"bootfile=bm18/uImage\0" \
	"bootm_low=20000000\0" \
	"bootm_size=01E00000\0" \
	"fdt_addr_r=21DF8000\0" \
	"kernel=run loadfdt; run loadkernel; bootm ${loadaddr} - ${fdt_addr_r}\0" \
	"kernelsd=run loadsd; bootm ${loadaddr} - ${fdt_addr_r}\0" \
	"loadfdt=tftp ${fdt_addr_r} bm18/bm18.dtb\0" \
	"loadfdtsd=ext4load mmc 0:2 ${fdt_addr_r} /boot/uImage-bm18.dtb\0" \
	"loadkernel=tftp ${loadaddr} ${bootfile}\0" \
	"loadkernelsd=ext4load mmc 0:2 ${loadaddr} /boot/uImage-bm18.bin\0" \
	"loadsd=run loadfdtsd; run loadkernelsd\0" \
	"tftptimeout=1000\0" \
	"tftptimeoutcountmax=100\0"

#define CONFIG_SYS_PBSIZE 1024

#define TIMER_TICKS_PER_US 200

#define CONFIG_TFTP_TSIZE

#define CONFIG_SYS_MMC_ENV_DEV 0
#define CONFIG_ENV_SPI_BUS 0
#define CONFIG_ENV_SPI_CS 0

#define CONFIG_SPL_PANIC_ON_RAW_IMAGE // to prevent loading a U-Boot image without the header

#endif // __1888BM18_H

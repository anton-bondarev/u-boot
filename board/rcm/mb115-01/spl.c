/*
 * RCM 1888TX018 SPL startup code
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spl.h>
#include <spi.h>
#include <spi_flash.h>
#include <asm/io.h>
#include <asm/tlb47x.h>
#include "boot.h"
#include "timer.h"

DECLARE_GLOBAL_DATA_PTR;

extern void _start(void);
extern void _start_secondary(void);

static const __attribute__((used)) __attribute__((section(".header")))
struct rumboot_header hdr = {
	.magic = RUMBOOT_HEADER_MAGIC,
	.entry_point_0 = (uint32_t)&_start,
	.entry_point_1 = (uint32_t)&_start_secondary,
};

void ddr_init (int slowdown);
int testdramfromto(uint *pstart, uint *pend);

void rcm_mtd_arbiter_init(void);
void rcm_sram_nor_init(void);

volatile unsigned int* _test_addr;

static void init_byte_order(void)
{
	tlb47x_inval( 0x30000000, TLBSID_256M ); 
	tlb47x_map( 0x1030000000, 0x30000000, TLBSID_256M, TLB_MODE_RWX);
}

/* SPL should works without DDR usage, test part of DDR for loading main U-boot and load it */

void board_init_f(ulong dummy)
{
	init_byte_order();

	spl_early_init();

	preloader_console_init();

	spl_set_bd();

	board_init_r(NULL, 0);
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_MMC1;
}


bool is_ddr_ok(void)
{
	int i;
	_test_addr = (unsigned int*)0x4D000000; 

	for(i = 0; i < 256; i++)
		_test_addr[i] = (unsigned int) &_test_addr[i];
	printf("DDR test...");

	for(i = 0; i < 256; i++)
	{
		if(_test_addr[i] != (unsigned int) &_test_addr[i])
		{
			printf(" Error at %d [%08X != %08X]\n", i, _test_addr[i], (unsigned int)&_test_addr[i]);
			break;
		}
	}
	if(i == 256)
	{
		printf(" OK.\n");
		return 1;
	}
	else
		return 0;
}

//*************************************************DDR AND TIMER INIT FUNCTIONS*************************************************/

static void init_mmu(void)
{
    asm volatile (
	// initialize MMUCR
	"addis 0, 0, 0x0000   \n\t"  // 0x0000
	"ori   0, 0, 0x0000   \n\t"  // 0x0000
	"mtspr 946, 0          \n\t"  // SPR_MMUCR, 946
	"mtspr 48,  0          \n\t"  // SPR_PID,    48
	// Set up TLB search priority
	"addis 0, 0, 0x9ABC   \n\t"  // 0x1234
	"ori   0, 0, 0xDEF8   \n\t"  // 0x5678
	//  "addis r0, r0, 0x1234   \n\t"  // 0x1234
	//	"ori   r0, r0, 0x5678   \n\t"  // 0x5678
	"mtspr 830, 0          \n\t"  // SSPCR, 830
	"mtspr 831, 0          \n\t"  // USPCR, 831
	"mtspr 829, 0          \n\t"  // ISPCR, 829
	"isync                  \n\t"  //
	"msync                  \n\t"  //
	:::
    );
};

static uint32_t read_dcr_reg(uint32_t addr)
{
  uint32_t res;

  asm volatile   (
  "mfdcrx (%0), (%1) \n\t"       // mfdcrx RT, RA
  :"=r"(res)
  :"r"((uint32_t) addr)
  :
	);
  return res;
};

static void write_dcr_reg(uint32_t addr, uint32_t value)
{

  asm volatile   (
  "mtdcrx (%0), (%1) \n\t"       // mtdcrx RA, RS
  ::"r"((uint32_t) addr), "r"((uint32_t) value)
  :
	);
};

static void commutate_plb6(void)
{
    uint32_t dcr_val=0;
    init_mmu();

    // PLB6 commutation
    // Set Seg0, Seg1, Seg2 and BC_CR0
    write_dcr_reg(0x80000204, 0x00000010);  // BC_SGD1
    write_dcr_reg(0x80000205, 0x00000010);  // BC_SGD2
    dcr_val = read_dcr_reg(0x80000200);     // BC_CR0
    write_dcr_reg(0x80000200, dcr_val | 0x80000000);
}

static void write_tlb_entry4(void)
{
 asm volatile (
	"addis 3, 0, 0x0000   \n\t"  // 0x0000, 0x0000
	"addis 4, 0, 0x4000   \n\t"  // 0x8200, 0x0000
	"ori   4, 4, 0x0bf0   \n\t"  // 0x08f0, 0x0BF0 04.04.2017 add_dauny
	"tlbwe 4, 3, 0        \n\t"

	"addis 4, 0, 0x0000   \n\t"  // 0x8200
	"ori   4, 4, 0x0000   \n\t"  // 0x0000
	"tlbwe 4, 3, 1        \n\t"

	"addis 4, 0, 0x0003  \n\t"  // 0x0003, 0000
	"ori   4, 4, 0x043f   \n\t"  // 0x043F, 023F
	"tlbwe 4, 3, 2        \n\t"
  "isync                  \n\t"  //
  "msync                  \n\t"  //
  :::
  );
};

static void write_tlb_entry8(void)
{
 asm volatile (
	"addis 3, 0, 0x0000   \n\t"  // 0x0000, 0x0000
	"addis 4, 0, 0x8000   \n\t"  // 0x8200, 0x0000
	"ori   4, 4, 0x0bf0   \n\t"  // 0x08f0, 0x0BF0 04.04.2017 add_dauny
	"tlbwe 4, 3, 0        \n\t"

	"addis 4, 0, 0x0000   \n\t"  // 0x8200
	"ori   4, 4, 0x0002   \n\t"  // 0x0000
	"tlbwe 4, 3, 1        \n\t"

	"addis 4, 0, 0x0003   \n\t"  // 0x0003, 0000
	"ori   4, 4, 0x043f   \n\t"  // 0x043F, 023F
	"tlbwe 4, 3, 2        \n\t"
  "isync                  \n\t"  //
  "msync                  \n\t"  //
  :::
  );
};


void spl_board_init(void)
{
	commutate_plb6();
	write_tlb_entry4();
	write_tlb_entry8();
	write_dcr_reg(0x80000201, 0x3);
	uint32_t pri = read_dcr_reg(0x80000201);
	printf("priority: %x\n", pri);

#ifdef CONFIG_1888TX018_DDR
	/* init dram */
	ddr_init(0);

	if(!is_ddr_ok())
	{
		printf("Error in DDR resetting...\n");
		udelay(1000);
		do_reset(0,0,0,0);
	}
	gd->ram_size = CONFIG_SYS_DDR_SIZE;
#endif

#ifdef CONFIG_MTD_RCM_NOR
	rcm_mtd_arbiter_init();
	rcm_sram_nor_init();
#endif
}


#ifdef CONFIG_SPL_MMC_SUPPORT
u32 spl_boot_mode(const u32 boot_device)
{
	return MMCSD_MODE_RAW;
}
#endif


void board_boot_order(u32 *spl_boot_list)
{
#ifndef CONFIG_FLASHWRITER
	spl_boot_list[0] = spl_boot_device();
	switch (spl_boot_list[0]) {
	case BOOT_DEVICE_SPI:
		spl_boot_list[1] = BOOT_DEVICE_MMC1;
		spl_boot_list[2] = BOOT_DEVICE_NOR;		// NOR before NAND, becose now GPIOAFSEL is initialized for NOR earlier(see spl_board_init)
		spl_boot_list[3] = BOOT_DEVICE_NAND;	// and function for initialization NOR is absent,but for NAND this function exist as self (nand_init)
		spl_boot_list[4] = BOOT_DEVICE_XMODEM_EDCL;	// for adding new elements increase array size
		break;
	case BOOT_DEVICE_MMC1:
		spl_boot_list[1] = BOOT_DEVICE_SPI;
		spl_boot_list[2] = BOOT_DEVICE_NOR;
		spl_boot_list[3] = BOOT_DEVICE_NAND;
		spl_boot_list[4] = BOOT_DEVICE_XMODEM_EDCL;
		break;
	}
#else
	spl_boot_list[0] = BOOT_DEVICE_FLASH_WRITER; // pseudo loader
	spl_boot_list[1] = BOOT_DEVICE_XMODEM_EDCL;
#endif
}

#define SPRN_DBCR0_44X 0x134
#define DBCR0_RST_SYSTEM 0x30000000

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long tmp;

	asm volatile(
		"mfspr	%0,%1\n"
		"oris	%0,%0,%2@h\n"
		"mtspr	%1,%0"
		: "=&r"(tmp)
		: "i"(SPRN_DBCR0_44X), "i"(DBCR0_RST_SYSTEM));

	return 0;
	}

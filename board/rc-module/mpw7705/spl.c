/*
 * TX018 SPL startup code
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

DECLARE_GLOBAL_DATA_PTR;

void ddr_init (int slowdown);
int testdramfromto(uint *pstart, uint *pend);

volatile unsigned int* _test_addr;

/* SPL should works without DDR usage, test part of DDR for loading main U-boot and load it */

void board_init_f(ulong dummy)
{
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
	_test_addr = (unsigned int*)0x40000000; 

	for(i = 0; i < 16; i++)
		_test_addr[i] = (unsigned int) &_test_addr[i];
	printf("DDR test...");

	for(i = 0; i < 16; i++)
	{
		if(_test_addr[i] != (unsigned int) &_test_addr[i])
		{
			printf(" Error at %d [%08X != %08X]\n", i, _test_addr[i], (unsigned int)&_test_addr[i]);
			break;
		}
	}
	if(i == 16)
	{
		printf(" OK.\n");
		return 1;
	}
	else
		return 0;
}

void usleep(uint32_t usec);

void spl_board_init(void)
{
	/* init dram */
	ddr_init(0);

	if(!is_ddr_ok())
	{
		printf("Try to slow down DDR\n");

		ddr_init(1);
		if(!is_ddr_ok())
		{
			printf("Try to use fixed params for DDR\n");
			ddr_init(2);
			if(!is_ddr_ok())
			{
				usleep(1000);
				do_reset(0,0,0,0);
			}
		}
	}
	gd->ram_size = CONFIG_SYS_DDR_SIZE;


	u32 boot_device = spl_boot_device();

	if (boot_device == BOOT_DEVICE_SPI)
		puts("Booting from SPI flash\n");
	else if (boot_device == BOOT_DEVICE_MMC1)
		puts("Booting from SD card\n");
	else if (boot_device == BOOT_DEVICE_EDCL)
		puts("Enter HOST mode for EDCL boot\n");
	else
		puts("Unknown boot device\n");
}


#ifdef CONFIG_SPL_MMC_SUPPORT
u32 spl_boot_mode(const u32 boot_device)
{
	return MMCSD_MODE_RAW;
}
#endif


void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = spl_boot_device();
	switch (spl_boot_list[0]) {
	case BOOT_DEVICE_SPI:
		spl_boot_list[1] = BOOT_DEVICE_MMC1;
		spl_boot_list[2] = BOOT_DEVICE_EDCL;
		break;
	case BOOT_DEVICE_MMC1:
		spl_boot_list[1] = BOOT_DEVICE_SPI;
		spl_boot_list[2] = BOOT_DEVICE_EDCL;
		break;
	}
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


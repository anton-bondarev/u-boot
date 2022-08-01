/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2019 Alexey Spirkov <dev@alsp.net>
 */
#include <version.h>
#include <common.h>
#include <config.h>
#include <spl.h>
#include <dm.h>
#include <fdt_support.h>
#include <hang.h>
#include <linux/io.h>


#ifdef CONFIG_SPL_BUILD

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_MMC1;
}


void board_init_f(ulong dummy)
{
	timer_init();

    int ret = spl_early_init();
    if (ret) {
		debug("spl_early_init() failed: %d\n", ret);
		hang();
	}

	preloader_console_init();

}

volatile unsigned int* _test_addr;

bool is_ddr_ok(void)
{
	int i;
	_test_addr = (unsigned int*)CONFIG_SYS_TEXT_BASE; 

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

extern void EMIC_init (unsigned base);

#define GRETH_BASE 0x000D0000
#define GPIO_BASE 0x000CC000
#define SPI_BASE 0x000CF000
#define SD_CS_GPIO 0
#define SSP_CS 0x140
#define SSP_CPSR 0x010
#define SSP_CR0	0x000
#define SSP_CR1	0x004
#define SSP_DR 0x008
#define SSP_SR	0x00C
#define SSP_SCR_SHFT 8
#define SSP_CR1_MASK_SSE (0x1 << 1)
#define SSP_SR_MASK_TNF	(0x1 << 1) 
#define SSP_SR_MASK_RNE	(0x1 << 2) 

static void spi_xfer_data(void *base, unsigned int len,
							  const void *dout, void *din)
{
	unsigned int len_tx = 0, len_rx = 0;
	unsigned short value;
	const unsigned char *txp = dout;
	unsigned char *rxp = din;

	debug("alive. len_tx = %d, len = %d\n", len_tx, len);

	// Checking: read remaining bytes
	if (readw(base + SSP_SR) & SSP_SR_MASK_RNE)
		value = readw(base + SSP_DR);

	while (len_tx < len) {
		if (readw(base + SSP_SR) & SSP_SR_MASK_TNF) {
			value = txp ? *txp++ : 0xFF;
			writew(value, base + SSP_DR);
			len_tx++;
		}

		if (readw(base + SSP_SR) & SSP_SR_MASK_RNE) {
			value = readw(base + SSP_DR);
			if (rxp)
				*rxp++ = value;
			len_rx++;
		}
	}

	while (len_rx < len_tx) {
		if (readw(base + SSP_SR) & SSP_SR_MASK_RNE) {
			value = readw(base + SSP_DR);
			if (rxp)
				*rxp++ = value;
			len_rx++;
		}
	}

	if (readw(base + SSP_SR) & SSP_SR_MASK_RNE)
		value = readw(base + SSP_DR);	
}

// sd card should be switched to SPI mode before any activity on SPI bus
int sdcard_init(void)
{
	void *spiregs = (void*)SPI_BASE;
	void *gpioregs = (void*)GPIO_BASE;
	unsigned char cmd0[6] = {0xff,0x40,0x0,0x0,0x0,0x90};
	unsigned char answer = 0xff;
	int count0 = 10;

	puts("SDcard init\n");

#if 0
	writew(0, spiregs + SSP_CS);
	writew(250, spiregs + SSP_CPSR);
	writew((readw(spiregs + SSP_CR0) & ~0xff00) | (4 << SSP_SCR_SHFT), spiregs + SSP_CR0);

	/* Enable the SPI hardware */
	writew(readw(spiregs + SSP_CR1) | SSP_CR1_MASK_SSE,
		spiregs + SSP_CR1);
#endif

	writew(2, spiregs + SSP_CS);
	writew(250, spiregs + SSP_CPSR);
	writew(0x4C7, spiregs + SSP_CR0);

	/* Enable the SPI hardware */
	writew(0xA,spiregs + SSP_CR1);

	// alloc cs
	clrbits(le32, gpioregs+0, 1 << SD_CS_GPIO);

	// send ones over spi with enabled cs
	spi_xfer_data(spiregs, 10, 0, 0);

	// send cmd0
	for(; count0 > 0; count0--)
	{		
		int count1 = 255;
		spi_xfer_data(spiregs, 6, cmd0, 0);
		// wait for answer
		for(; count1 > 0; count1--)
		{
			spi_xfer_data(spiregs, 1, 0, &answer);
			if((answer & 0x80) == 0)
				break;
			/* code */
		}
		if(answer == 1)
			break;
	}
	// disable cs
	setbits(le32, gpioregs+0, 1 << SD_CS_GPIO);

	/* Disable the SPI hardware */
	writew(readw(spiregs + SSP_CR1) & ~SSP_CR1_MASK_SSE,
		   spiregs + SSP_CR1);

	return answer;
}

void spl_edcl_enable(void)
{
	void *edclregs = (void *)GRETH_BASE;

	clrbits(le32, edclregs+0, 1<<14);

	return;
}

void spl_board_init(void)
{
	puts("OE enable\n");

	setbits(le32, (void *)0xCC038, 0xff);

	setbits(le32, (void *)0xCC010, 1 << 7);
	clrbits(le32, (void *)0xCC000, 1 << 7);

	udelay(100000);

	/* init dram */
	EMIC_init(0x600C0000);
	EMIC_init(0x800C0000);
	EMIC_init(0xA00C0000);
	EMIC_init(0xC00C0000);
	EMIC_init(0xE00C0000);

	if(!is_ddr_ok())
	{
		printf("Error in DDR resetting...\n");
		udelay(1000000);
		do_reset(0,0,0,0);
	}

	gd->ram_size = CONFIG_SYS_DDR_SIZE;

	u32 boot_device = spl_boot_device();

	if (boot_device == BOOT_DEVICE_SPI)
		puts("Booting from SPI flash\n");
	else if (boot_device == BOOT_DEVICE_MMC1)
		puts("Booting from SD card\n");
	else if (boot_device == BOOT_DEVICE_XIP)
		puts("Enter HOST mode for EDCL boot\n");
	else
		puts("Unknown boot device\n");
}

void board_boot_order(u32 *spl_boot_list)
{
	spl_boot_list[0] = spl_boot_device();
	switch (spl_boot_list[0]) {
	case BOOT_DEVICE_SPI:
		spl_boot_list[1] = BOOT_DEVICE_MMC1;
		break;
	case BOOT_DEVICE_MMC1:
		spl_boot_list[1] = BOOT_DEVICE_SPI;
		break;
	}
    spl_boot_list[2] = BOOT_DEVICE_XIP; // just wait for marker
    spl_boot_list[3] = BOOT_DEVICE_RAM;
    spl_boot_list[4] = BOOT_DEVICE_NONE;
}


#endif


int dram_init(void)
{
    gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

    return 0;
}

int board_init(void)
{
    return 0; 
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("SoC: 1879VM8YA, ver TODO\n");
	return 0;
}
#endif

#include <common.h>
#include <spl.h>
#include <spi.h>
#include <spi_flash.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

void ddr_init (void);
int testdramfromto(uint *pstart, uint *pend);

#define CRGCPU__        0x38006000

static void failsafe(void)
{
	writel(0x1ACCE551, CRGCPU__ + 0xc);
	writel(0x02010050, CRGCPU__ + 0x9); // 500Mhz
	writel(0x1, CRGCPU__ + 0x4);
}

static void patch_da_regs(void)
{
    writel(0x1f, 0x3c067000 + 0x420);
    writel(0xff, 0x3C060000 + 0x420);
    writel(0xff, 0x3C061000 + 0x420);
    writel(0xff, 0x3C062000 + 0x420);
    writel(0xff, 0x3C063000 + 0x420);
    writel(0xff, 0x3C064000 + 0x420);
    writel(0xff, 0x3C065000 + 0x420);
    writel(0xff, 0x3C066000 + 0x420);
    writel(0xff, 0x3C067000 + 0x420);
}


/* SPL should works without DDR usage, test part of DDR for loading main U-boot and load it */

void board_init_f(ulong dummy)
{
	//failsafe();
	patch_da_regs();
	/* init dram */
	ddr_init();

	board_init_f_init_reserve((ulong) gd);
	gd->ram_size = CONFIG_SYS_DDR_SIZE;

	spl_early_init();

	preloader_console_init();

	spl_set_bd();


	board_init_r(NULL, 0);
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_SPI;
}

void spl_board_init(void)
{
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

// ASTRO TODO
int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}


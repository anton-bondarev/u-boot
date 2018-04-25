#include <common.h>
#include <spl.h>
#include <spi.h>
#include <spi_flash.h>

#include <mpw7705_baremetal.h>

DECLARE_GLOBAL_DATA_PTR;

static const char logo[] =
"_________________________________\n"
"< Hello World, from U-Boot SPL! >\n"
"_________________________________\n";

static gd_t global_data;

void ddr_init (void);

void board_init_f(ulong dummy)
{
	/* Clear global data */
	size_t len = (size_t)&__bss_end - (size_t)&__bss_start;
	memset((void *)&__bss_start, 0x00, len);

	preloader_console_init();

	gd = &global_data;
	memset(gd, 0, sizeof(gd_t));

	spl_set_bd();
	
	/* init dram */
	gd->ram_size = CONFIG_SYS_DDR_SIZE;
	ddr_init();

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
}

// ASTRO TODO
int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}


#include <common.h>
#include <spl.h>
#include <spi.h>
#include <spi_flash.h>

#include <mpw7705_baremetal.h>

DECLARE_GLOBAL_DATA_PTR;

static const char logo[] =
"_________________________\n"
"< Hello World, from SDIO! >\n"
/*"-------------------------\n"
"       \\   ^__^\n"
"        \\  (oo)\\_______\n"
"           (__)\\       )\\/\\\n"
"               ||----w |\n"
"               ||     ||\n"
"\n"*/;

static gd_t global_data;

void board_init_f(ulong dummy)
{
	puts(logo);

	/* Clear global data */
	size_t len = (size_t)&__bss_end - (size_t)&__bss_start;
	memset((void *)&__bss_start, 0x00, len);

	//get_clocks();

	preloader_console_init();

	gd = &global_data;
	memset(gd, 0, sizeof(gd_t));
	/*int ret = spl_init();
	if(ret) {
		puts("spl_init() failed!\n");
	}*/
	spl_set_bd();
	dram_init();

	board_init_r(NULL, 0);

	/*const int hsize = 18;
	char buff[hsize];
	memset(buff, 0, hsize);*/

	/*puts("gonna claim\n");
	mpw7705_spi_init();
	puts("gonna xfer\n");
	mpw7705_spi_core_read((void*)buff, 0, hsize);

	puts("gonna print\n");


	puts("done\n");*/

	/*copy_uboot_to_ram();

	// Jump to U-Boot image
	uboot = (void *)CONFIG_SYS_TEXT_BASE;
	(*uboot)(); // Never returns Here*/
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

void ddr_init (void);

static int a = 41;

static int test(int param)
{
	return param + 1;
}

int dram_init(void)
{
	puts("gonna gd->ram_size\n");
	gd->ram_size = CONFIG_SYS_DDR_SIZE;

	ddr_init();

	puts("gonna memcpy\n");
	memcpy((void *)0x40000000, (const void*)test, 512);

	int (*inDDR)(int);
	inDDR = (void *)0x40000000;
	puts("gonna inDDR\n");
	printf("a = %d\n", inDDR(a));
	return 0;
}
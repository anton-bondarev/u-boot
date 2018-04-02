#include <common.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#endif

#include <mpw7705_baremetal.h>

DECLARE_GLOBAL_DATA_PTR;

static const char logo[] =
"_________________________\n"
"< Hello World, from SDIO! >\n"
"-------------------------\n"
"       \\   ^__^\n"
"        \\  (oo)\\_______\n"
"           (__)\\       )\\/\\\n"
"               ||----w |\n"
"               ||     ||\n"
"\n";

#ifdef CONFIG_SPL_BUILD
void board_init_f(ulong dummy)
{
	puts(logo);

	/* Clear global data */
	memset((void *)gd, 0, sizeof(gd_t));

	get_clocks();
	preloader_console_init();
	spl_set_bd();
	dram_init();

	// Clear the BSS
	size_t len = (size_t)&__bss_end - (size_t)&__bss_start;
    memset((void *)&__bss_start, 0x00, len);

	/*copy_uboot_to_ram();

	// Jump to U-Boot image
	uboot = (void *)CONFIG_SYS_TEXT_BASE;
	(*uboot)(); // Never returns Here*/
}
#endif

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
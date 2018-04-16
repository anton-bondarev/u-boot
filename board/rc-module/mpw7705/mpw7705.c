#include <common.h>
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
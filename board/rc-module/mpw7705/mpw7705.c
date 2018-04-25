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

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_DDR_SIZE;
	ddr_init();
	return 0;
}
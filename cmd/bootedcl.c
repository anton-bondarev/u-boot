// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2018 AstroSoft. All rights reserved.
 *
 * Alexey Spirkov, alexeis@astrosoft.ru.
 */

#include <common.h>
#include <command.h>



static int do_bootedcl(cmd_tbl_t *cmdtp, int flag, int argc,
		      char * const argv[])
{
	// Theese command can be extended with Ctrl+C processing and more powerful 
	// loaded image check mehanisms than in romboot

    void (*bootrom_enter_host_mode)(void) = (void (*)(void)) BOOT_ROM_HOST_MODE;
	bootrom_enter_host_mode(); /* never return */
	return 0;
}

U_BOOT_CMD(
	bootedcl,	1,	1,	do_bootedcl,
	"Proceed to EDCL boot sequence",
	"Proceed to EDCL boot sequence"
);

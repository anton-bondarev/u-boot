// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2020 Alexey Spirkov <dev@alsp.net>
 * 
 * Some init for rcm platform.
 */

#include <common.h>
#include <cpu_func.h>

#if !defined(CONFIG_SYS_DCACHE_OFF)
void enable_caches(void)
{
	/* do not enable data cache for a while */
	//dcache_enable();
}
#endif
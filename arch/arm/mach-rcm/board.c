// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2020 Alexey Spirkov <dev@alsp.net>
 * 
 * Some init for rcm platform.
 */

#include <common.h>
#include <cpu_func.h>

void enable_caches(void)
{
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
	icache_enable();
#endif
#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
	dcache_enable();
#endif
}


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

void flush_cache(unsigned long start, unsigned long size)
{
	unsigned long aligned_start = start & ~(CONFIG_SYS_CACHELINE_SIZE-1);
	unsigned long aligned_end = ((start + size) & ~(CONFIG_SYS_CACHELINE_SIZE-1)) + CONFIG_SYS_CACHELINE_SIZE;
	flush_dcache_range(aligned_start, aligned_end);
}

#define DEBUG 1
#include <common.h>

// ASTRO TODO
int checkcpu(void)
{
    return 0;
}

void print_reginfo(void)
{
    debug("ToDo register print\n");
// ASTRO TODO
}

#ifdef CONFIG_GRETH
int cpu_eth_init(bd_t *bis)
{
	return greth_initialize(bis);
}
#endif

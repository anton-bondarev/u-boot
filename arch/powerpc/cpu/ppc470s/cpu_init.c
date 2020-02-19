#include <common.h>

unsigned long cpu_init_f(void)
{
    return GD_FLG_SKIP_RELOC;   // flags to board_init_f()
}

int cpu_init_r (void)
{
        return 0;
}

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

/*void get_sys_info(sys_info_t *sys_info)
{

}*/

// ASTRO TODO
int get_clocks(void)
{
    gd->cpu_clk = 500000000; // In Hz
    gd->bus_clk = 100000000;
    gd->pci_clk = 100000000;
    gd->mem_clk = 100000000;

    // arch/powerpc/include/asm/global_data.h: struct arch_global_data
    /*gd->arch.lbc_clk = 0;
    gd->arch.sdhc_clk = 0;
    gd->arch.qe_clk = 0;
    gd->arch.brg_clk = 0;
    gd->arch.csb_clk = 0;
    gd->arch.i2c1_clk = 0;
    gd->arch.tsec1_clk = 0;
    gd->arch.tsec2_clk = 0;
    gd->arch.usbdr_clk = 0;
    gd->arch.vco_out = 0;
    gd->arch.cpm_clk = 0;
    gd->arch.scc_clk = 0;*/

    return 0;
}

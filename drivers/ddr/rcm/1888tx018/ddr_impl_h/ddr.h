#ifndef __DDR_H__
#define __DDR_H__

#include "ddr/def.h"
#include "ddr/aximcif2.h"
#include "ddr/ddr34lmc.h"
#include "ddr/mcif2arb4.h"
#include "ddr/plb6mcif2.h"
#include "plb6bc.h"
#include <stdint.h>

#define PLB6_BC_BASE  0x80000200
#define CRG_CPU_BASE  0x38006000
#define CRG_DDR_BASE  0x38007000

#define PMU_BASE            0x38004000

#define EM0_PLB6MCIF2_DCR  0x80010000
#define EM0_AXIMCIF2_DCR   0x80020000
#define EM0_MCLFIR_DCR     0x80030000
#define EM0_MCIF2ARB_DCR   0x80040000
#define EM0_DDR3LMC_DCR    0x80050000

#define EM1_PLB6MCIF2_DCR  0x80100000
#define EM1_AXIMCIF2_DCR   0x80110000
#define EM1_MCLFIR_DCR     0x80120000
#define EM1_MCIF2ARB_DCR   0x80130000
#define EM1_DDR3LMC_DCR    0x80140000

#define EM2_PLB6MCIF2_DCR 0x80160000
#define EM3_PLB6MCIF2_DCR 0x80180000

#define EM0_PHY__        0x3800E000
#define EM1_PHY__        0x3800F000


void ddr_init_impl(
        DdrHlbId hlbId,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode
        );

void ddr_init_imlp_freq(
        DdrHlbId hlbId,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode,
        CrgDdrFreq FreqMode
        );

//DDR default initialization routine.
void ddr_init(int slow_down);

//Init EM0 only
void ddr_init_em0(void);

//Init EM1 only
void ddr_init_em1(void);

//Gets experimental value of T_SYS_RDLAT from EM0 DDR34LMC. The value is measured during DDR34LMC read transaction.
uint8_t ddr_get_T_SYS_RDLAT(const uint32_t baseAddr);

uint32_t ddr_get_ecc_error_status(const uint32_t baseAddr, const uint32_t portNum);
void ddr_clear_ecc_error_status(const uint32_t baseAddr, const uint32_t portNum);
void ddr_enter_self_refresh_mode(const uint32_t baseAddr);
void ddr_exit_self_refresh_mode(const uint32_t baseAddr);
void ddr_enable_dram_clk(DdrHlbId hlbId);
void ddr_disable_dram_clk(DdrHlbId hlbId);

void crg_ddr_config_freq (DdrHlbId hlbId, const DdrBurstLength burstLen);
void crg_set_writelock (void);
void crg_remove_writelock (void);

void ddr_exit_low_power_mode (void);
void ddr_enter_low_power_mode (void);

#endif // __DDR_H__

/**
 *
 */

#ifndef __DDR34LMC_REG_H__
#define __DDR34LMC_REG_H__


#include "reg_field.h"
#include "../../mivem_types.h"


BEGIN_ENUM( DDR34LMC_REG )
DECLARE_ENUM_VAL(                                           DDR34LMC_MCSTAT,                        0x10 )                  // 0x60000000

DECLARE_ENUM_VAL(                                           DDR34LMC_MCOPT1,                        0x20 )                  // 0x01020000
DECLARE_ENUM_VAL(                                           DDR34LMC_MCOPT2,                        0x21 )                  // 0x80f30000 (Normal Reset) 0x80000000 (Recovery Reset)

DECLARE_ENUM_VAL(                                           DDR34LMC_IOCNTL,                        0x30 )                  // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_PHYCNTL,                       0x31 )                  // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_PHYSTAT,                       0x32 )                  // System Dependant
DECLARE_ENUM_VAL(                                           DDR34LMC_PHYMASK,                       0x33 )                  // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_CFGR0,                         0x40 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_CFGR1,                         0x41 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_CFGR2,                         0x42 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_CFGR3,                         0x43 )              // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_RHZEN,                         0x48 )                  // 0x00000000


DECLARE_ENUM_VAL(                                           DDR34LMC_INITSEQ0,                      0x50 + (2*0) )          // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITSEQ1,                      0x50 + (2*1) )          // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITSEQ2,                      0x50 + (2*2) )          // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITSEQ3,                      0x50 + (2*3) )          // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITSEQ4,                      0x50 + (2*4) )          // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITSEQ5,                      0x50 + (2*5) )          // 0x00000000


DECLARE_ENUM_VAL(                                           DDR34LMC_INITCMD0,                   0x50 + ((2*0) + 1) )    // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITCMD1,                   0x50 + ((2*1) + 1) )    // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITCMD2,                   0x50 + ((2*2) + 1) )    // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITCMD3,                   0x50 + ((2*3) + 1) )    // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITCMD4,                   0x50 + ((2*4) + 1) )    // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_INITCMD5,                   0x50 + ((2*5) + 1) )    // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_SDTR0,                      0x80 + 0 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SDTR1,                      0x80 + 1 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SDTR2 ,                      0x80 + 2 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SDTR3 ,                      0x80 + 3 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SDTR4 ,                      0x80 + 4 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SDTR5 ,                      0x80 + 5 )              // 0x00000000



DECLARE_ENUM_VAL(                                           DDR34LMC_DBG0,                          0x8A )                  // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_SMR0,                       0x90 + 0 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SMR1,                       0x90 + 1 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SMR2,                       0x90 + 2)              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SMR3,                       0x90 + 3 )              // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_ODTR0,                      0xA0 + 0 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_ODTR1,                      0xA0 + 1 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_ODTR2,                      0xA0 + 2 )              // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_ODTR3,                      0xA0 + 3 )              // 0x00000000

DECLARE_ENUM_VAL(                                           DDR34LMC_SCRUB_CNTL,                    0xAA )                  // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SCRUB_INT,                     0xAB )                  // 0x00000000
DECLARE_ENUM_VAL(                                           DDR34LMC_SCRUB_CUR,                     0xAC )                  // 0x00000000
#ifdef DDR34LMC_USE_DDR4
DECLARE_ENUM_VAL(                                           DDR34LMC_SCRUB_CUR_EXT,                 0xAD )                  // 0x00000000   Used for DDR4 operation only
#endif

DECLARE_ENUM_VAL( DDR34LMC_CALSTAT,                                                                 0xC1 )                  // 0x00000000
DECLARE_ENUM_VAL( DDR34LMC_T_PHYUPD0,                                              0xCC + 0 )              // 0x00000000
DECLARE_ENUM_VAL( DDR34LMC_T_PHYUPD1,                                              0xCC + 1 )              // 0x00000000
DECLARE_ENUM_VAL( DDR34LMC_T_PHYUPD2,                                              0xCC + 2 )              // 0x00000000
DECLARE_ENUM_VAL( DDR34LMC_T_PHYUPD3,                                              0xCC + 3 )              // 0x00000000

DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT0,          0xF0 + 2*0 )   
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT1,          0xF0 + 2*1 )   
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT2,          0xF0 + 2*2 )   
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT3,          0xF0 + 2*3 )   

DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT0_clear ,       0xF0 + (2*0) )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT1_clear ,       0xF0 + (2*1) )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT2_clear ,       0xF0 + (2*2) )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT3_clear,        0xF0 + (2*3) )

DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT0_set ,         0xF0 + ((2*0) + 1) )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT1_set ,         0xF0 + ((2*1) + 1) )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT2_set ,         0xF0 + ((2*2) + 1) )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT3_set ,         0xF0 + ((2*3) + 1) )

DECLARE_ENUM_VAL( DDR34LMC_CE_TCNT_CLR,                                                             0xE8 )                  // 0x00000000

DECLARE_ENUM_VAL( DDR34LMC_CE_TCNT_SET,                                                             0xE9 )                  // 0x00000000

DECLARE_ENUM_VAL( DDR34LMC_ECCERR_ADDR_EXT,                                                         0xEA )                  // 0x00000000


END_ENUM( DDR34LMC_REG )

BEGIN_ENUM( DDR34LMC_REG_DFLT )
//DECLARE_ENUM_VAL( DDR34LMC_MCSTAT_DFLT,               0x60000000 )
DECLARE_ENUM_VAL( DDR34LMC_MCOPT1_DFLT,                 0x01020000 )
DECLARE_ENUM_VAL( DDR34LMC_MCOPT2_DFLT,                 0x80F30000 )
DECLARE_ENUM_VAL( DDR34LMC_MCOPTE_DFLT,                 0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_IOCNTL_DFLT,                 0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_PHYCNTL_DFLT,                0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_PHYSTAT_DFLT,                0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_PHYMASK_DFLT,                0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_CFGR_DFLT,                   0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_RHZEN_DFLT,                  0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_INITSEQ_DFLT,                0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_INITCMD_DFLT,                0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SDTR_DFLT,                   0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_DBG0_DFLT,                   0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SMR_DFLT,                    0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_ODTR_DFLT,                   0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_CNTL_DFLT,             0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_INT_DFLT,              0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_CUR_DFLT,              0x00000000 )
#ifdef DDR34LMC_USE_DDR4
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_CUR_EXT_DFLT,          0x00000000 )
#endif
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_PREFILL_DATA_DFLT,     0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_ST_RANK_PORT_DFLT,     0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_CALSTAT_DFLT,                0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_T_PHYUPD_DFLT,               0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_ED_RANK_PORT_DFLT,     0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_ADDR_PORT_DFLT,       0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_COUNT_PORT_DFLT,      0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_CE_TCNT_CLR_DFLT,            0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_CE_TCNT_SET_DFLT,            0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_ADDR_EXT_DFLT,        0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT_DFLT,            0x00000000 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_ECC_CHECK_PORT_DFLT,  0x00000000 )
END_ENUM( DDR34LMC_REG_DFLT )

BEGIN_ENUM( DDR34LMC_REG_MSK )
DECLARE_ENUM_VAL( DDR34LMC_MCSTAT_MSK,                  0xF8000000 )
DECLARE_ENUM_VAL( DDR34LMC_MCOPT2_MSK,                  0xFFFB7FF0 )
DECLARE_ENUM_VAL( DDR34LMC_MCOPTE_MSK,                  0x30000000 )
DECLARE_ENUM_VAL( DDR34LMC_CFGR_MSK,                    0x0000FF11 )
//DECLARE_ENUM_VAL( DDR34LMC_INITSEQ_MSK,               0x8FFF001F )
//DECLARE_ENUM_VAL( DDR34LMC_INITCMD_MSK,               0x8FFF001F )
DECLARE_ENUM_VAL( DDR34LMC_SDTR0_MSK,                   0xFFFF0FFF )
DECLARE_ENUM_VAL( DDR34LMC_SDTR1_MSK,                   0xEF0FFF0F )
DECLARE_ENUM_VAL( DDR34LMC_SDTR2_MSK,                   0xFFFF3F3F )
DECLARE_ENUM_VAL( DDR34LMC_SDTR3_MSK,                   0xFF3FFFFF )
DECLARE_ENUM_VAL( DDR34LMC_SDTR4_MSK,                   0x3F1FF7FF )
DECLARE_ENUM_VAL( DDR34LMC_SDTR5_MSK,                   0x0F1F0000 )
DECLARE_ENUM_VAL( DDR34LMC_DBG0_MSK,                    0x001F0000 )
DECLARE_ENUM_VAL( DDR34LMC_SMR0_MSK,                    0x0000FFFF )
DECLARE_ENUM_VAL( DDR34LMC_SMR1_MSK,                    0x00001AFF )
DECLARE_ENUM_VAL( DDR34LMC_SMR2_MSK,                    0x00001EFF )
DECLARE_ENUM_VAL( DDR34LMC_SMR3_MSK,                    0x000019FF )
DECLARE_ENUM_VAL( DDR34LMC_SMR4_MSK,                    0x00001FFE )
DECLARE_ENUM_VAL( DDR34LMC_SMR5_MSK,                    0x00001FDF )
DECLARE_ENUM_VAL( DDR34LMC_SMR6_MSK,                    0x0000187F )
DECLARE_ENUM_VAL( DDR34LMC_ODTR_MSK,                    0xFF000000 )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_CNTL_MSK,              0xF000FFFF )
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_INT_MSK,               0x0000FFFF )
#ifdef DDR34LMC_USE_DDR4
DECLARE_ENUM_VAL( DDR34LMC_SCRUB_CUR_EXT_MSK,           0x00000003 )
#endif
DECLARE_ENUM_VAL( DDR34LMC_CALSTAT_MSK,                 0xF8000000 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_COUNT_PORT_MSK,       0xFF0FFFFF )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_ADDR_EXT_MSK,         0x00007777 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_PORT_MSK,             0xFF000F87 )
DECLARE_ENUM_VAL( DDR34LMC_ECCERR_ECC_CHECK_PORT_MSK,   0x0000FFFF )
END_ENUM( DDR34LMC_REG_MSK )


#endif // __DDR34LMC_REG_H__

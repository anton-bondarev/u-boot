/*
 *  Created on: Jul 8, 2015
 *      Author: a.korablinov
 */

#ifndef PPC_476FP_ITRPT_FIELDS_H
#define PPC_476FP_ITRPT_FIELDS_H

#define IBM_BIT_INDEX( size, index )    ( ((size)-1) - ((index)%(size)) )

#define ITRPT_XSR_WE_e  45
#define ITRPT_XSR_WE_n  1
#define ITRPT_XSR_WE_i  IBM_BIT_INDEX( 64, ITRPT_XSR_WE_e )

#define ITRPT_XSR_CE_e  46
#define ITRPT_XSR_CE_n  1
#define ITRPT_XSR_CE_i  IBM_BIT_INDEX( 64, ITRPT_XSR_CE_e )

#define ITRPT_XSR_EE_e  48
#define ITRPT_XSR_EE_n  1
#define ITRPT_XSR_EE_i  IBM_BIT_INDEX( 64, ITRPT_XSR_EE_e )

#define ITRPT_XSR_PR_e  49
#define ITRPT_XSR_PR_n  1
#define ITRPT_XSR_PR_i  IBM_BIT_INDEX( 64, ITRPT_XSR_PR_e )

#define ITRPT_XSR_FP_e  50
#define ITRPT_XSR_FP_n  1
#define ITRPT_XSR_FP_i  IBM_BIT_INDEX( 64, ITRPT_XSR_FP_e )

#define ITRPT_XSR_ME_e  51
#define ITRPT_XSR_ME_n  1
#define ITRPT_XSR_ME_i  IBM_BIT_INDEX( 64, ITRPT_XSR_ME_e )

#define ITRPT_XSR_FE0_e 52
#define ITRPT_XSR_FE0_n 1
#define ITRPT_XSR_FE0_i IBM_BIT_INDEX( 64, ITRPT_XSR_FE0_e )

#define ITRPT_XSR_DWE_e 53
#define ITRPT_XSR_DWE_n 1
#define ITRPT_XSR_DWE_i IBM_BIT_INDEX( 64, ITRPT_XSR_DWE_e )

#define ITRPT_XSR_DE_e  54
#define ITRPT_XSR_DE_n  1
#define ITRPT_XSR_DE_i  IBM_BIT_INDEX( 64, ITRPT_XSR_DE_e )

#define ITRPT_XSR_FE1_e 55
#define ITRPT_XSR_FE1_n 1
#define ITRPT_XSR_FE1_i IBM_BIT_INDEX( 64, ITRPT_XSR_FE1_e )

#define ITRPT_XSR_IS_e  58
#define ITRPT_XSR_IS_n  1
#define ITRPT_XSR_IS_i  IBM_BIT_INDEX( 64, ITRPT_XSR_IS_e )

#define ITRPT_XSR_DS_e  59
#define ITRPT_XSR_DS_n  1
#define ITRPT_XSR_DS_i  IBM_BIT_INDEX( 64, ITRPT_XSR_DS_e )

#define ITRPT_XSR_PMM_e 61
#define ITRPT_XSR_PMM_n 1
#define ITRPT_XSR_PMM_i IBM_BIT_INDEX( 64, ITRPT_XSR_PMM_e )


#define ITRPT_XSRR0_ADDR_e  61
#define ITRPT_XSRR0_ADDR_n  30
#define ITRPT_XSRR0_ADDR_i  IBM_BIT_INDEX( 64, ITRPT_XSRR0_ADDR_e )


#define ITRPT_IVPR_ADDR_e   47
#define ITRPT_IVPR_ADDR_n   16
#define ITRPT_IVPR_ADDR_i   IBM_BIT_INDEX( 64, ITRPT_IVPR_ADDR_e )


#define ITRPT_IVORn_OFFSET_e    59
#define ITRPT_IVORn_OFFSET_n    12
#define ITRPT_IVORn_OFFSET_i    IBM_BIT_INDEX( 64, ITRPT_IVORn_OFFSET_e )


#define ITRPT_ESR_ISMC_e    32c
#define ITRPT_ESR_ISMC_n    1
#define ITRPT_ESR_ISMC_i    IBM_BIT_INDEX( 64, ITRPT_ESR_ISMC_e )

#define ITRPT_ESR_SS_e      33
#define ITRPT_ESR_SS_n      1
#define ITRPT_ESR_SS_i      IBM_BIT_INDEX( 64, ITRPT_ESR_SS_e )

#define ITRPT_ESR_POT1_e    34
#define ITRPT_ESR_POT1_n    1
#define ITRPT_ESR_POT1_i    IBM_BIT_INDEX( 64, ITRPT_ESR_POT1_e )

#define ITRPT_ESR_POT2_e    35
#define ITRPT_ESR_POT2_n    1
#define ITRPT_ESR_POT2_i    IBM_BIT_INDEX( 64, ITRPT_ESR_POT2_e )

#define ITRPT_ESR_PIL_e     36
#define ITRPT_ESR_PIL_n     1
#define ITRPT_ESR_PIL_i     IBM_BIT_INDEX( 64, ITRPT_ESR_PIL_e )

#define ITRPT_ESR_PPR_e     37
#define ITRPT_ESR_PPR_n     1
#define ITRPT_ESR_PPR_i     IBM_BIT_INDEX( 64, ITRPT_ESR_PPR_e )

#define ITRPT_ESR_PTR_e     38
#define ITRPT_ESR_PTR_n     1
#define ITRPT_ESR_PTR_i     IBM_BIT_INDEX( 64, ITRPT_ESR_PTR_e )

#define ITRPT_ESR_FP_e      39
#define ITRPT_ESR_FP_n      1
#define ITRPT_ESR_FP_i      IBM_BIT_INDEX( 64, ITRPT_ESR_FP_e )

#define ITRPT_ESR_ST_e      40
#define ITRPT_ESR_ST_n      1
#define ITRPT_ESR_ST_i      IBM_BIT_INDEX( 64, ITRPT_ESR_ST_e )

#define ITRPT_ESR_DLK_e     43
#define ITRPT_ESR_DLK_n     2
#define ITRPT_ESR_DLK_i     IBM_BIT_INDEX( 64, ITRPT_ESR_DLK_e )

#define ITRPT_ESR_DLK_not_occur     0b00
#define ITRPT_ESR_DLK_dcbf_issued   0b01
#define ITRPT_ESR_DLK_icbi_issued   0b10

#define ITRPT_ESR_AP_e      44
#define ITRPT_ESR_AP_n      1
#define ITRPT_ESR_AP_i      IBM_BIT_INDEX( 64, ITRPT_ESR_AP_e )

#define ITRPT_ESR_PUO_e     45
#define ITRPT_ESR_PUO_n     1
#define ITRPT_ESR_PUO_i     IBM_BIT_INDEX( 64, ITRPT_ESR_PUO_e )

#define ITRPT_ESR_BO_e      46
#define ITRPT_ESR_BO_n      1
#define ITRPT_ESR_BO_i      IBM_BIT_INDEX( 64, ITRPT_ESR_BO_e )

#define ITRPT_ESR_PIE_e     47
#define ITRPT_ESR_PIE_n     1
#define ITRPT_ESR_PIE_i     IBM_BIT_INDEX( 64, ITRPT_ESR_PIE_e )

#define ITRPT_ESR_PCRE_e    59
#define ITRPT_ESR_PCRE_n    1
#define ITRPT_ESR_PCRE_i    IBM_BIT_INDEX( 64, ITRPT_ESR_PCRE_e )

#define ITRPT_ESR_PCMP_e    60
#define ITRPT_ESR_PCMP_n    1
#define ITRPT_ESR_PCMP_i    IBM_BIT_INDEX( 64, ITRPT_ESR_PCMP_e )

#define ITRPT_ESR_PCRF_e    63
#define ITRPT_ESR_PCRF_n    1
#define ITRPT_ESR_PCRF_i    IBM_BIT_INDEX( 64, ITRPT_ESR_PCRF_e )


#define ITRPT_MCSR_MCS_e    32
#define ITRPT_MCSR_MCS_n    1
#define ITRPT_MCSR_MCS_i    IBM_BIT_INDEX( 64, ITRPT_MCSR_MCS_e )

#define ITRPT_MCSR_TLB_e    36
#define ITRPT_MCSR_TLB_n    1
#define ITRPT_MCSR_TLB_i    IBM_BIT_INDEX( 64, ITRPT_MCSR_TLB_e )

#define ITRPT_MCSR_IC_e     37
#define ITRPT_MCSR_IC_n     1
#define ITRPT_MCSR_IC_i     IBM_BIT_INDEX( 64, ITRPT_MCSR_IC_e )

#define ITRPT_MCSR_DC_e     38
#define ITRPT_MCSR_DC_n     1
#define ITRPT_MCSR_DC_i     IBM_BIT_INDEX( 64, ITRPT_MCSR_DC_e )

#define ITRPT_MCSR_GPR_e    39
#define ITRPT_MCSR_GPR_n    1
#define ITRPT_MCSR_GPR_i    IBM_BIT_INDEX( 64, ITRPT_MCSR_GPR_e )

#define ITRPT_MCSR_FPR_e    40
#define ITRPT_MCSR_FPR_n    1
#define ITRPT_MCSR_FPR_i    IBM_BIT_INDEX( 64, ITRPT_MCSR_FPR_e )

#define ITRPT_MCSR_IMP_e    41
#define ITRPT_MCSR_IMP_n    1
#define ITRPT_MCSR_IMP_i    IBM_BIT_INDEX( 64, ITRPT_MCSR_IMP_e )

#define ITRPT_MCSR_L2_e     42
#define ITRPT_MCSR_L2_n     1
#define ITRPT_MCSR_L2_i     IBM_BIT_INDEX( 64, ITRPT_MCSR_L2_e )

#define ITRPT_MCSR_DCR_e    43
#define ITRPT_MCSR_DCR_n    1
#define ITRPT_MCSR_DCR_i    IBM_BIT_INDEX( 64, ITRPT_MCSR_DCR_e )


#endif // PPC_476FP_ITRPT_FIELDS_H

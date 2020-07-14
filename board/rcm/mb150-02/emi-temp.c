#include <common.h>
#include "emi-temp.h"

// ???
#include <asm/tlb47x.h>

// ???
#include <asm/io.h>

/* ??? */

DECLARE_GLOBAL_DATA_PTR;


/* ??? */

#define DCR_PLB6_BC_BASE        0x80000200
#define DCR_EM2_PLB6MCIF2_BASE  0x80050000
#define DCR_EM2_EMI_BASE        0x80060000

#define BEGIN_ENUM( enum_name )         typedef enum enum_name {
#define DECLARE_ENUM_VAL( name, value ) name = value,
#define END_ENUM( enum_name )           } enum_name;

#define IBM_BIT_INDEX( size, index )    ( ((size)-1) - ((index)%(size)) )

#define reg_field(field_right_bit_num_from_ppc_user_manual, value)\
    ((value) << IBM_BIT_INDEX( 32, field_right_bit_num_from_ppc_user_manual ))

#define REG_READ_DCR(DEV_NAME, REG, REG_SIZE)                                                            \
inline static uint##REG_SIZE##_t DEV_NAME##_dcr_read_##REG(uint32_t const base_addr)               \
{                                                                                                       \
    return dcr_read(base_addr + REG);                                                                \
}

#define REG_WRITE_DCR(DEV_NAME, REG, REG_SIZE)                                                          \
inline static void DEV_NAME##_dcr_write_##REG(uint32_t const base_addr, uint##REG_SIZE##_t const value) \
{                                                                                                       \
    dcr_write(base_addr + REG, value);                                                                \
}

// ??? dupl
#define msync()\
    asm volatile (\
        "msync \n\t"\
    )

BEGIN_ENUM( PLB6BC_REG )
DECLARE_ENUM_VAL( PLB6BC_TSNOOP,                0x02 )          /* TSNOOP                       NA              The value of this register is hard coded during configuration and does not change.*/
END_ENUM( PLB6BC_REG )

BEGIN_ENUM( PLB6MCIF2_REG )
DECLARE_ENUM_VAL( PLB6MCIF2_INTR_EN,       0x04 )
DECLARE_ENUM_VAL( PLB6MCIF2_PLBASYNC,      0x07 )
DECLARE_ENUM_VAL( PLB6MCIF2_PLBORD,        0x08 )
DECLARE_ENUM_VAL( PLB6MCIF2_PLBCFG,        0x09 )
DECLARE_ENUM_VAL( PLB6MCIF2_MAXMEM,        0x0F )
DECLARE_ENUM_VAL( PLB6MCIF2_PUABA,         0x10 )
DECLARE_ENUM_VAL( PLB6MCIF2_MR0CF,         0x11 )
DECLARE_ENUM_VAL( PLB6MCIF2_MR1CF,         0x12 )
DECLARE_ENUM_VAL( PLB6MCIF2_MR2CF,         0x13 )
DECLARE_ENUM_VAL( PLB6MCIF2_MR3CF,         0x14 )
END_ENUM( PLB6MCIF2_REG )

BEGIN_ENUM( EMI_REG )
DECLARE_ENUM_VAL( EMI_SS0,                      0x00 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SD0,                      0x04 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SS1,                      0x08 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SD1,                      0x0C )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SS2,                      0x10 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SD2,                      0x14 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SS3,                      0x18 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SD3,                      0x1C )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SS4,                      0x20 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SD4,                      0x24 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SS5,                      0x28 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_SD5,                      0x2C )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_RFC,                      0x30 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_HSTSR,                    0x34 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_ECNT20,                   0x38 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_ECNT53,                   0x3C )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_H1ADR,                    0x40 )    //WR N/A
DECLARE_ENUM_VAL( EMI_H2ADR,                    0x44 )    //WR N/A
DECLARE_ENUM_VAL( EMI_RREQADR,                  0x48 )    //WR N/A
DECLARE_ENUM_VAL( EMI_WREQADR,                  0x4C )    //WR N/A
DECLARE_ENUM_VAL( EMI_WDADR,                    0x50 )    //WR N/A
DECLARE_ENUM_VAL( EMI_BUSEN,                    0x54 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_WECR,                     0x58 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_FLCNTRL,                  0x5C )    //WR 0x0000001C
DECLARE_ENUM_VAL( EMI_IMR,                      0x60 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_IMR_SET,                  0x64 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_IMR_RST,                  0x68 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_IRR,                      0x70 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_IRR_RST,                  0x74 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_ECCWRR,                   0x78 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_ECCRDR,                   0x7C )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_H1CMR,                    0xC0 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_H2CMR,                    0xC4 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_RREQCMR,                  0xC8 )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_WREQCMR,                  0xCC )    //WR 0x00000000
DECLARE_ENUM_VAL( EMI_WDCMR,                    0xD0 )    //WR 0x00000000
END_ENUM( EMI_REG )

/*DCR bus access*/
// ??? dup
static inline __attribute__((no_instrument_function)) __attribute__((always_inline)) void dcr_write( uint32_t const addr, uint32_t const wval ) {
	printf("WDCR 0x%08x = 0x%08x\n", addr, wval); // ???
    asm volatile (
        "mtdcrx %0, %1 \n\t"
        ::  "r"(addr), "r"(wval)
    );
}

// ??? dup
static inline __attribute__((no_instrument_function)) __attribute__((always_inline)) uint32_t dcr_read( uint32_t const addr ) {
    uint32_t rval = 0;
    asm volatile (
        "mfdcrx %0, %1 \n\t"
        :   "=r"(rval)
        :   "r"(addr)
    );
    return rval;
}

REG_READ_DCR( plb6bc, PLB6BC_TSNOOP, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_INTR_EN, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_INTR_EN, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PLBASYNC, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PLBASYNC, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PLBORD, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PLBORD, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PLBCFG, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PLBCFG, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MAXMEM, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MAXMEM, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_PUABA, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_PUABA, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR0CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR0CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR1CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR1CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR2CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR2CF, 32 )

REG_READ_DCR( plb6mcif2, PLB6MCIF2_MR3CF, 32 )
REG_WRITE_DCR( plb6mcif2, PLB6MCIF2_MR3CF, 32 )

BEGIN_ENUM(ssx_btyp_t)
DECLARE_ENUM_VAL( BTYP_SRAM,    0b000 )
DECLARE_ENUM_VAL( BTYP_NOR,     0b001 )
DECLARE_ENUM_VAL( BTYP_SSRAM,   0b010 )
DECLARE_ENUM_VAL( BTYP_PIPERDY, 0b100 )
DECLARE_ENUM_VAL( BTYP_SDRAM,   0b110 )
END_ENUM( ssx_btyp_t );

BEGIN_ENUM( ssx_ptyp_t )
DECLARE_ENUM_VAL( PTYP_NO_PAGES,  0b0)
DECLARE_ENUM_VAL( PTYP_USE_PAGES, 0b1)
END_ENUM( ssx_ptyp_t );

BEGIN_ENUM( ssx_srdy_t )
DECLARE_ENUM_VAL( SRDY_EXT_RDY_NOT_USE, 0b0)
DECLARE_ENUM_VAL( SRDY_EXT_RDY_USE,     0b1)
END_ENUM( ssx_srdy_t );

BEGIN_ENUM( ssx_twr_t )
DECLARE_ENUM_VAL( TWR_0, 0b0)
DECLARE_ENUM_VAL( TWR_1, 0b1)
END_ENUM( ssx_twr_t );

BEGIN_ENUM( ssx_sst_t )
DECLARE_ENUM_VAL( SST_Flow_Through, 0b0)
DECLARE_ENUM_VAL( SST_Pipelined,    0b1)
END_ENUM( ssx_sst_t );

BEGIN_ENUM( ssx_tssoe_t )
DECLARE_ENUM_VAL( TSSOE_1, 0b0)
DECLARE_ENUM_VAL( TSSOE_2, 0b1)
END_ENUM( ssx_tssoe_t );

BEGIN_ENUM( ssx_tsoe_t )
DECLARE_ENUM_VAL( TSOE_1, 0b0)
DECLARE_ENUM_VAL( TSOE_2, 0b1)
END_ENUM( ssx_tsoe_t );

BEGIN_ENUM( ssx_tcyc_t )
DECLARE_ENUM_VAL( TCYC_2,  0b1111)
DECLARE_ENUM_VAL( TCYC_3,  0b1110)
DECLARE_ENUM_VAL( TCYC_4,  0b1101)
DECLARE_ENUM_VAL( TCYC_5,  0b1100)
DECLARE_ENUM_VAL( TCYC_6,  0b1011)
DECLARE_ENUM_VAL( TCYC_7,  0b1010)
DECLARE_ENUM_VAL( TCYC_8,  0b1001)
DECLARE_ENUM_VAL( TCYC_9,  0b1000)
DECLARE_ENUM_VAL( TCYC_10, 0b0111)
DECLARE_ENUM_VAL( TCYC_11, 0b0110)
DECLARE_ENUM_VAL( TCYC_12, 0b0101)
DECLARE_ENUM_VAL( TCYC_13, 0b0100)
DECLARE_ENUM_VAL( TCYC_14, 0b0011)
DECLARE_ENUM_VAL( TCYC_15, 0b0010)
DECLARE_ENUM_VAL( TCYC_16, 0b0001)
DECLARE_ENUM_VAL( TCYC_17, 0b0000)
END_ENUM( ssx_tcyc_t );

BEGIN_ENUM( ssx_tdel_t )
DECLARE_ENUM_VAL( TDEL_0, 0b00)
DECLARE_ENUM_VAL( TDEL_3, 0b01)
DECLARE_ENUM_VAL( TDEL_2, 0b10)
DECLARE_ENUM_VAL( TDEL_1, 0b11)
END_ENUM( ssx_tdel_t );

BEGIN_ENUM( sdx_csp_t )
DECLARE_ENUM_VAL( CSP_256,  0b000)
DECLARE_ENUM_VAL( CSP_512,  0b001)
DECLARE_ENUM_VAL( CSP_1024, 0b011)
DECLARE_ENUM_VAL( CSP_2048, 0b101)
DECLARE_ENUM_VAL( CSP_4096, 0b111)
END_ENUM( sdx_csp_t );

BEGIN_ENUM( sdx_sds_t )
DECLARE_ENUM_VAL( SDS_2M,   0b000)
DECLARE_ENUM_VAL( SDS_4M,   0b001)
DECLARE_ENUM_VAL( SDS_8M,   0b010)
DECLARE_ENUM_VAL( SDS_16M,  0b011)
DECLARE_ENUM_VAL( SDS_32M,  0b100)
DECLARE_ENUM_VAL( SDS_64M,  0b101)
DECLARE_ENUM_VAL( SDS_128M, 0b110)
DECLARE_ENUM_VAL( SDS_256M, 0b111)
END_ENUM( sdx_sds_t );

BEGIN_ENUM( sdx_cl_t )
DECLARE_ENUM_VAL( CL_3, 0b00)
DECLARE_ENUM_VAL( CL_2, 0b01)
DECLARE_ENUM_VAL( CL_1, 0b10)
END_ENUM( sdx_cl_t );

BEGIN_ENUM( sdx_trdl_t )
DECLARE_ENUM_VAL( TRDL_1, 0b0)
DECLARE_ENUM_VAL( TRDL_2, 0b1)
END_ENUM( sdx_trdl_t );

BEGIN_ENUM( sdx_si_t )
DECLARE_ENUM_VAL( SI_EXT_INIT, 0b0)
DECLARE_ENUM_VAL( SI_CPU_INIT, 0b1)
END_ENUM( sdx_si_t );

BEGIN_ENUM( sdx_trcd_t )
DECLARE_ENUM_VAL( TRCD_2, 0b11)
DECLARE_ENUM_VAL( TRCD_3, 0b10)
DECLARE_ENUM_VAL( TRCD_4, 0b01)
DECLARE_ENUM_VAL( TRCD_5, 0b00)
END_ENUM( sdx_trcd_t );

BEGIN_ENUM( sdx_tras_t )
DECLARE_ENUM_VAL( TRAS_4,  0b101)
DECLARE_ENUM_VAL( TRAS_5,  0b100)
DECLARE_ENUM_VAL( TRAS_6,  0b011)
DECLARE_ENUM_VAL( TRAS_7,  0b010)
DECLARE_ENUM_VAL( TRAS_8,  0b001)
DECLARE_ENUM_VAL( TRAS_9,  0b000)
DECLARE_ENUM_VAL( TRAS_10, 0b111)
DECLARE_ENUM_VAL( TRAS_11, 0b110)
END_ENUM( sdx_tras_t );

BEGIN_ENUM( rfc_trfc_t )
DECLARE_ENUM_VAL( TRFC_6,  0b011)
DECLARE_ENUM_VAL( TRFC_7,  0b010)
DECLARE_ENUM_VAL( TRFC_8,  0b001)
DECLARE_ENUM_VAL( TRFC_9,  0b000)
DECLARE_ENUM_VAL( TRFC_10, 0b111)
DECLARE_ENUM_VAL( TRFC_11, 0b110)
DECLARE_ENUM_VAL( TRFC_12, 0b101)
DECLARE_ENUM_VAL( TRFC_13, 0b100)
END_ENUM( rfc_trfc_t );

typedef enum
{
    emi_b0_sram0 = 0,
    emi_b1_sdram = 1,
    emi_b2_ssram = 2,
    emi_b3_pipelined = 3,
    emi_b4_sram1 = 4,
    emi_b5_nor = 5,
    emi_bank_all = 0xF,
} emi_bank_num;

typedef struct
{
   ssx_btyp_t   BTYP;
   ssx_ptyp_t   PTYP;
   ssx_srdy_t   SRDY;
   ssx_twr_t    TWR;
   ssx_sst_t    SST;
   ssx_tssoe_t  T_SSOE;
   ssx_tsoe_t   T_SOE;
   ssx_tcyc_t   T_CYC;
   uint16_t     T_RDY;
   ssx_tdel_t   T_DEL;
} emi_ssx_reg_cfg;

typedef struct
{
   sdx_csp_t    CSP;
   sdx_sds_t    SDS;
   sdx_cl_t     CL;
   sdx_trdl_t   T_RDL;
   sdx_si_t     SI;
   sdx_trcd_t   T_RCD;
   sdx_tras_t   T_RAS;
} emi_sdx_reg_cfg;

typedef struct
{
    emi_ssx_reg_cfg ssx_cfg;
    emi_sdx_reg_cfg sdx_cfg;
} emi_bank_cfg;

typedef struct
{
    rfc_trfc_t TRFC;
    uint16_t   RP;
} emi_rfc_cfg;

typedef enum
{
    emi_ecc_off = 0,
    emi_ecc_on = 1,
} emi_ecc_status;

BEGIN_ENUM( EMI_SSx_FIELD )
DECLARE_ENUM_VAL( EMI_SSx_BTYP_i,       0 )
DECLARE_ENUM_VAL( EMI_SSx_PTYP_i,       3 )
DECLARE_ENUM_VAL( EMI_SSx_SRDY_i,       4 )
DECLARE_ENUM_VAL( EMI_SSx_TWR_i,        5 )
DECLARE_ENUM_VAL( EMI_SSx_SST_i,        6 )
DECLARE_ENUM_VAL( EMI_SSx_T_SSOE_i,     7 )
DECLARE_ENUM_VAL( EMI_SSx_T_SOE_i,      8 )
DECLARE_ENUM_VAL( EMI_SSx_T_CYC_i,      9 )
DECLARE_ENUM_VAL( EMI_SSx_T_RDY_i,      13 )
DECLARE_ENUM_VAL( EMI_SSx_T_DEL_i,      23 )
END_ENUM( EMI_SSx_FIELD )

BEGIN_ENUM( EMI_SDx_FIELD )
DECLARE_ENUM_VAL( EMI_SDx_CSP_i,        1 )
DECLARE_ENUM_VAL( EMI_SDx_SDS_i,        4 )
DECLARE_ENUM_VAL( EMI_SDx_CL_i,         7 )
DECLARE_ENUM_VAL( EMI_SDx_T_RDL_i,      9 )
DECLARE_ENUM_VAL( EMI_SDx_SI_i,         10 )
DECLARE_ENUM_VAL( EMI_SDx_T_RCD_i,      11 )
DECLARE_ENUM_VAL( EMI_SDx_T_RAS_i,      13 )
END_ENUM( EMI_SDx_FIELD )

BEGIN_ENUM( EMI_RFC_FIELD )
DECLARE_ENUM_VAL( EMI_RFC_TRFC_i,  1 )
DECLARE_ENUM_VAL( EMI_RFC_RP_i,    4 )
END_ENUM( EMI_RFC_FIELD )

void plb6mcif2_simple_init( uint32_t base_addr, const uint32_t puaba )
{
    plb6mcif2_dcr_write_PLB6MCIF2_INTR_EN(base_addr,
            //enable logging but disable interrupts
              reg_field(15, 0xFFFF)
            | reg_field(31, 0x0000));

    plb6mcif2_dcr_write_PLB6MCIF2_PLBASYNC(base_addr,
            reg_field(1, 0b1)); //PLB clock equals MCIF2 clock

    plb6mcif2_dcr_write_PLB6MCIF2_PLBORD(base_addr,
              reg_field(0, 0b0)  //Raw
            | reg_field(3, 0b010) //Threshold[1:3]
            | reg_field(7, 0b0)   //Threshold[0]
            | reg_field(16, 0b0) //IT_OOO
            | reg_field(18, 0b00) //TOM
            );

    plb6mcif2_dcr_write_PLB6MCIF2_MAXMEM(base_addr,
            reg_field(1, 0b00)); //Set MAXMEM = 8GB

    plb6mcif2_dcr_write_PLB6MCIF2_PUABA(base_addr,
            puaba); //Set PLB upper address base address from I_S_ADDR[0:30]

    plb6mcif2_dcr_write_PLB6MCIF2_MR0CF(base_addr,
            //Set Rank0 base addr
            //M_BA[0:2] = PUABA[28:30]
            //M_BA[3:12] = 0b0000000000
              reg_field(3, (puaba & 0b1110)) | reg_field(12, 0b0000000000)
            | reg_field(19, 0b1001) //Set Rank0 size = 4GB
            | reg_field(31, 0b1)); //Enable Rank0

    plb6mcif2_dcr_write_PLB6MCIF2_MR1CF(base_addr,
            //Set Rank1 base addr
            //M_BA[0:2] = PUABA[28:30]
            //M_BA[3:12] = 0b1000000000
            reg_field(3, (puaba & 0b1110)) | reg_field(12, 0b1000000000)
          | reg_field(19, 0b1001) //Set Rank1 size = 4GB
          | reg_field(31, 0b1)); //Enable Rank1

    plb6mcif2_dcr_write_PLB6MCIF2_MR2CF(base_addr,
          reg_field(31, 0b0)); //Disable Rank2

    plb6mcif2_dcr_write_PLB6MCIF2_MR3CF(base_addr,
            reg_field(31, 0b0)); //Disable Rank3

    const uint32_t Tsnoop = (plb6bc_dcr_read_PLB6BC_TSNOOP(DCR_PLB6_BC_BASE) & (0b1111 << 28)) >> 28;

    plb6mcif2_dcr_write_PLB6MCIF2_PLBCFG(base_addr,
            //PLB6MCIF2 enable [31] = 0b1
              reg_field(0, 0b0) //Non-coherent
            | reg_field(4, Tsnoop) //T-snoop
            | reg_field(5, 0b0) //PmstrID_chk_EN
            | reg_field(27, 0b1111) //Parity enables
            | reg_field(30, 0b000) //CG_Enable
            | reg_field(31, 0b1)); //BR_Enable

}

void emi_set_bank_cfg (uint32_t const emi_dcr_base, emi_bank_num const num_bank, emi_bank_cfg const * bn_cfg)
{
    // ??? TEST_ASSERT( num_bank != emi_bank_all, "Invalid argument in emi_set_bank_cfg" );
    dcr_write (emi_dcr_base + EMI_SS0 + num_bank*(EMI_SS1 - EMI_SS0),  //write SSx
            ((bn_cfg->ssx_cfg.BTYP   << EMI_SSx_BTYP_i )  |
            ( bn_cfg->ssx_cfg.PTYP   << EMI_SSx_PTYP_i)   |
            ( bn_cfg->ssx_cfg.SRDY   << EMI_SSx_SRDY_i )  |
            ( bn_cfg->ssx_cfg.TWR    << EMI_SSx_TWR_i )   |
            ( bn_cfg->ssx_cfg.SST    << EMI_SSx_SST_i )   |
            ( bn_cfg->ssx_cfg.T_SSOE << EMI_SSx_T_SSOE_i )|
            ( bn_cfg->ssx_cfg.T_SOE  << EMI_SSx_T_SOE_i ) |
            ( bn_cfg->ssx_cfg.T_CYC  << EMI_SSx_T_CYC_i ) |
            ( bn_cfg->ssx_cfg.T_RDY  << EMI_SSx_T_RDY_i ) |
            ( bn_cfg->ssx_cfg.T_DEL  << EMI_SSx_T_DEL_i ))
            );
    dcr_write (emi_dcr_base + EMI_SD0 + num_bank*(EMI_SD1 - EMI_SD0), //write SDx
            ((bn_cfg->sdx_cfg.CSP    << EMI_SDx_CSP_i )   |
            ( bn_cfg->sdx_cfg.SDS    << EMI_SDx_SDS_i )   |
            ( bn_cfg->sdx_cfg.CL     << EMI_SDx_CL_i )    |
            ( bn_cfg->sdx_cfg.T_RDL  << EMI_SDx_T_RDL_i ) |
            ( bn_cfg->sdx_cfg.SI     << EMI_SDx_SI_i )    |
            ( bn_cfg->sdx_cfg.T_RCD  << EMI_SDx_T_RCD_i ) |
            ( bn_cfg->sdx_cfg.T_RAS  << EMI_SDx_T_RAS_i ))
            );
}

void emi_set_rfc(uint32_t const emi_dcr_base, emi_rfc_cfg const * rfc)
{
    dcr_write(emi_dcr_base + EMI_RFC,
                ( rfc->TRFC << EMI_RFC_TRFC_i ) |
                ( rfc->RP   << EMI_RFC_RP_i   )
             );
}

void emi_set_ecc (uint32_t const emi_dcr_base, emi_bank_num const num_bank, emi_ecc_status const ecc_stat)
{
    if (num_bank == emi_bank_all )
    {
        if (ecc_stat == emi_ecc_off)
            dcr_write (emi_dcr_base + EMI_HSTSR, 0x00);
        else
            dcr_write (emi_dcr_base + EMI_HSTSR, 0x3F);
    }
    else
    {
        uint32_t tmp = dcr_read (emi_dcr_base + EMI_HSTSR);
        if (ecc_stat == emi_ecc_off)
        {
            tmp &= ~(1 << num_bank);
            dcr_write (emi_dcr_base + EMI_HSTSR, tmp);
        }
        else
        {
            tmp |= 1 << num_bank;
            dcr_write (emi_dcr_base + EMI_HSTSR, tmp);
        }
    }
}

void emi_init_impl (uint32_t const emi_dcr_base, uint32_t const plb6mcif2_dcr_base, uint32_t const puaba)
{
    //init bridge
    plb6mcif2_simple_init( plb6mcif2_dcr_base,  puaba );

    //init bank0 - SRAM0
    static emi_bank_cfg const b0_cfg =
    {
       //SS0
       {
           BTYP_SRAM,
           PTYP_NO_PAGES,
           SRDY_EXT_RDY_NOT_USE,
           TWR_0,
           SST_Flow_Through,
           TSSOE_1,
           TSOE_1,
           TCYC_2,
           0, //T_RDY
           TDEL_0
       },
       //SD0
       {
           CSP_256,
           SDS_2M,
           CL_3,
           TRDL_1,
           SI_EXT_INIT,
           TRCD_5,
           TRAS_9
       }
    };
    // ??? emi_set_bank_cfg(emi_dcr_base, emi_b0_sram0, &b0_cfg);

    //init bank1 - SDRAM
    //setting parameters by comment:
    //(https://jira.module.ru/jira/browse/OI10-116?focusedCommentId=43530&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-43530)
    static emi_bank_cfg const b1_cfg =
    {
       //SS1
       {
           BTYP_SDRAM,
           PTYP_NO_PAGES,
           SRDY_EXT_RDY_NOT_USE,
           TWR_0,
           SST_Flow_Through,
           TSSOE_1,
           TSOE_1,
           TCYC_8,
           0, //T_RDY
           TDEL_0
       },
       //SD1
       {
           CSP_2048,
           SDS_64M,
           CL_3,
           TRDL_2,
           SI_CPU_INIT,
           TRCD_2,
           TRAS_5
       }
    };
    // ??? emi_set_bank_cfg(emi_dcr_base, emi_b1_sdram, &b1_cfg);

    static const emi_rfc_cfg emi_rfc =
    {
            TRFC_7,
            0b11110011110011,//RP
    };
    // ??? emi_set_rfc(emi_dcr_base, &emi_rfc);

    //init bank2 - SSRAM
    static emi_bank_cfg const b2_cfg =
    {
       //SS2
       {
           BTYP_SSRAM,
           PTYP_NO_PAGES,
           SRDY_EXT_RDY_NOT_USE,
           TWR_0,
           SST_Pipelined,
           TSSOE_2,
           TSOE_1,
           TCYC_8,
           0, //T_RDY
           TDEL_0
       },
       //SD2
       {
           CSP_256,
           SDS_2M,
           CL_3,
           TRDL_1,
           SI_EXT_INIT,
           TRCD_5,
           TRAS_9
       }
    };
    // ??? emi_set_bank_cfg(emi_dcr_base, emi_b2_ssram, &b2_cfg);

    //init bank3 - PIPELINED
    static emi_bank_cfg const b3_cfg =
    {
       //SS3
       {
           BTYP_PIPERDY,
           PTYP_NO_PAGES,
           SRDY_EXT_RDY_NOT_USE,
           TWR_0,
           SST_Pipelined,
           TSSOE_1,
           TSOE_1,
           TCYC_8,
           0, //T_RDY
           TDEL_0
       },
       //SD3
       {
           CSP_256,
           SDS_2M,
           CL_3,
           TRDL_1,
           SI_EXT_INIT,
           TRCD_5,
           TRAS_9
       }
    };
    // ??? emi_set_bank_cfg(emi_dcr_base, emi_b3_pipelined, &b3_cfg);

    //init bank4 - SRAM1
    static emi_bank_cfg const b4_cfg =
    {
       //SS4
       {
           BTYP_SRAM,
           PTYP_NO_PAGES,
           SRDY_EXT_RDY_NOT_USE,
           TWR_0,
           SST_Flow_Through,
           TSSOE_1,
           TSOE_1,
           TCYC_2,
           0, //T_RDY
           TDEL_0
       },
       //SD4
       {
           CSP_256,
           SDS_2M,
           CL_3,
           TRDL_1,
           SI_EXT_INIT,
           TRCD_5,
           TRAS_9
       }
    };
    // ??? emi_set_bank_cfg(emi_dcr_base, emi_b4_sram1, &b4_cfg);

    //init bank5 - NOR
     static emi_bank_cfg const b5_cfg =
    {
        //SS5
        {
            BTYP_NOR,
            PTYP_NO_PAGES,
            SRDY_EXT_RDY_NOT_USE,
            TWR_0,
            SST_Flow_Through,
            TSSOE_1,
            TSOE_1,
            TCYC_12,
            0, //T_RDY
            TDEL_0
        },
        //SD5
        {
            CSP_256,
            SDS_2M,
            CL_3,
            TRDL_1,
            SI_EXT_INIT,
            TRCD_5,
            TRAS_9
        }
    };
    // ??? emi_set_bank_cfg(emi_dcr_base, emi_b5_nor, &b5_cfg);
    // ??? dcr_write(DCR_EM2_EMI_BASE + EMI_FLCNTRL, 0x17);
    // ??? emi_set_ecc (emi_dcr_base, emi_bank_all, emi_ecc_off);
    // ??? dcr_write(emi_dcr_base + EMI_BUSEN, 0x01);

    /* Current config */
    /* ??? bank_config_cache[0] = &b0_cfg;
    bank_config_cache[1] = &b1_cfg;
    bank_config_cache[2] = &b2_cfg;
    bank_config_cache[3] = &b3_cfg;
    bank_config_cache[4] = &b4_cfg;
    bank_config_cache[5] = &b5_cfg;*/
    // ??? msync();
}

typedef int(*enable_icache_type)(void);

int enable_icache(void)
{
	__asm__ __volatile__(
		"mfmsr r12 \n"
		"lis r11, 0xFFFF \n"
		"ori r11, r11, 0xEFFF \n"
		"and r12, r12, r11 \n"
		"mtmsr r12 \n" // disable Machine Check
		"tlbwe	%0, %3, 0 \n"
		"tlbwe	%1, %3, 1 \n"
		"tlbwe	%2, %3, 2 \n"
		"isync \n"
		"msync \n"
	:
	:
	  "r" (0x80020000 | (1 << 11) | (0x03 << 4)),
	  "r" (0xC0000020),
	  "r" ((0x7 << 3) | (0x7 << 0)),
	  "r" (0x88000000 | 3)
	:
	  "r11", "r12");
	__asm__ __volatile__(
		"mfmsr r12 \n"
		"lis r11, 0xFFFF \n"
		"ori r11, r11, 0xEFFF \n"
		"and r12, r12, r11 \n"
		"mtmsr r12 \n" // disable Machine Check
		"tlbwe	%0, %3, 0 \n"
		"tlbwe	%1, %3, 1 \n"
		"tlbwe	%2, %3, 2 \n"
		"isync \n"
		"msync \n"
	:
	:
	  "r" (0x80030000 | (1 << 11) | (0x03 << 4)),
	  "r" (0xC0010020),
	  "r" ((0x7 << 3) | (0x7 << 0)),
	  "r" (0x88000000 | 4)
	:
	  "r11", "r12");
	/* ???asm volatile (
		"lis r10, 0 \n"
		"mtspr 946, r10 \n" // MMUCR
		"lis r11, 0x8002 \n"
		"tlbsx r12, r0, r11 \n" // r12 way & index
		"oris r12, r12, 0x8000 \n"
	:
	:
	: "r10", "r11", "r12");*/
/* ???	tlb47x_inval(0x80020000, TLBSID_64K);
	tlb47x_map(0x20C0000000, 0x80020000, TLBSID_64K, TLB_MODE_RWX); // ???*/

/*		li r12, 0
	mtspr SPR_MMUCR, r12

	tlbsx. r4, r0, r3      // [r3] - EA, [r4] - Way, Index
	bne    ite_end         // branch if not found
	oris   r4, r4, 0x8000  // r4[0]=1, r4[4]=0
	tlbwe  r3, r4, 0       // [r3] - EA[0:19], V[20], [r4]- Way[1:2], Index[8:15], index is NU
ite_end:
	or     r3, r4, r4      // return [r3]
	
	isync
	msync
*/

	return 0;
}

bool emi_init(void)
{
	// ???
	/*??? {
			volatile uint32_t *p = (volatile uint32_t*)0x80000000;
		unsigned count = 0;
		unsigned long now = get_timer(0);
		while (get_timer(now) < 1000) {
			*p; *p; *p; *p; *p;
			*p; *p; *p; *p; *p;
			++count;
		}
		printf("IM0 %u\n", count);
		p = (volatile uint32_t*)0x80020000;
		count = 0;
		now = get_timer(0);
		while (get_timer(now) < 1000) {
			*p; *p; *p; *p; *p;
			*p; *p; *p; *p; *p;
			++count;
		}
		printf("IM1 %u\n", count);
		p = (volatile uint32_t*)0x80040000;
		count = 0;
		now = get_timer(0);
		while (get_timer(now) < 1000) {
			*p; *p; *p; *p; *p;
			*p; *p; *p; *p; *p;
			++count;
		}
		printf("IM2 %u\n", count);
	}*/
	// ???

	// ????
	dcr_write(0x80000600, 0xC10);
	dcr_write(0x80000604, 0x1);

	emi_init_impl (DCR_EM2_EMI_BASE, DCR_EM2_PLB6MCIF2_BASE, 0x00);
	// ??? dcr_write((DCR_EM2_EMI_BASE + 0xc), 0x9622);
	// ???? dcr_write((DCR_EM2_EMI_BASE + 0x34), 0x2);


	// ???tlb47x_inval(0x00000000, TLBSID_256M); // ???
	tlb47x_inval(0x20000000, TLBSID_256M); // ???
	tlb47x_map(0x0020000000, 0x20000000, TLBSID_256M, TLB_MODE_RWX); // ???

	uint32_t base_addr = 0x20000000; // ???
	uint32_t length = 4096; // ??? 32 * 1024 * 1024;
	// ??? uint32_t length = 32 * 1024 * 1024;
	int i;

	printf("Testing by word...\n");
	for (i = 0; i < length / 4; ++i) {
		uint32_t addr = base_addr + i * 4;
		*((volatile uint32_t*)addr) = addr;
	}
	for (i = 0; i < length / 4; ++i) {
		uint32_t addr = base_addr + i * 4;
		uint32_t data = *((volatile uint32_t*)addr);
		if (data != addr) {
			printf("word: addr = 0x%08x, value = 0x%08x, expected = 0x%08x\n", (unsigned)addr, (unsigned)data, (unsigned)addr);
			return false;
		}
	}
	printf("ok\n");

	/* ??? volatile uint32_t *p = (volatile uint32_t*)0x80000000;
	unsigned count = 0;
	unsigned long now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("IM0 %u\n", count);
	p = (volatile uint32_t*)0x80020000;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("IM1 %u\n", count);
	p = (volatile uint32_t*)0x80040000;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("IM2 %u\n", count);
	p = (volatile uint32_t*)base_addr;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("SDRAM %u\n", count);*/

	/* ???uint32_t addr = (uint32_t)(void*)enable_icache;
	addr -= 0x80020000;
	addr += 0xC0000000;
	int result = ((enable_icache_type)(void*)addr)();
	printf("result = 0x%08x\n", (unsigned)result);*/

	/* ??? {
		volatile uint32_t *p = (volatile uint32_t*)0x80020000;
		unsigned count;
		for (count = 0; count < 0x20000 / 4; ++count)
			*(p++);
	}*/

	/* ??? p = (volatile uint32_t*)0x80000000;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("IM0 %u\n", count);
	p = (volatile uint32_t*)0x80020000;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("IM1 %u\n", count);
	p = (volatile uint32_t*)0x80040000;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("IM2 %u\n", count);
	p = (volatile uint32_t*)base_addr;
	count = 0;
	now = get_timer(0);
	while (get_timer(now) < 1000) {
		*p; *p; *p; *p; *p;
		*p; *p; *p; *p; *p;
		++count;
	}
	printf("SDRAM %u\n", count);*/

	/* ??? printf("Testing by halfword...\n");
	for (i = 0; i < length / 2; ++i) {
		uint32_t addr = base_addr + i * 2;
		*((volatile uint16_t*)addr) = addr;
	}
	for (i = 0; i < length / 2; ++i) {
		uint32_t addr = base_addr + i * 2;
		uint32_t data = *((volatile uint16_t*)addr);
		if (data != (addr & 0xFFFF)) {
			printf("halfword: addr = 0x%08x, value = 0x%08x, expected = 0x%08x\n", (unsigned)addr, (unsigned)data, (unsigned)(addr & 0xFFFF));
			return false;
		}
	}
	printf("ok\n");

	printf("Testing by byte...\n");
	for (i = 0; i < length; ++i) {
		uint32_t addr = base_addr + i;
		*((volatile uint8_t*)addr) = addr;
	}
	for (i = 0; i < length; ++i) {
		uint32_t addr = base_addr + i;
		uint32_t data = *((volatile uint8_t*)addr);
		if (data != (addr & 0xFF)) {
			printf("byte: addr = 0x%08x, value = 0x%08x, expected = 0x%08x\n", (unsigned)addr, (unsigned)data, (unsigned)(addr & 0xFF));
			return false;
		}
	}
	printf("ok\n");*/

	// ???
	/* ??? {
		uint32_t p = 0xD0029000 + 0x18;
		unsigned count = 0;
		unsigned long now = get_timer(0);
		while (get_timer(now) < 1000) {
			readl(p);
			++count;
		}
		printf("count = %u\n", count);
	}*/

	// ???
	/* ??? printf("Trying to boot from UART\n");
	printf("C");

	{
		while (readl(0xD0029000 + 0x18) & 0x8) ;

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		while (readl(0xD0029000 + 0x18) & 0x10) ;
		readl(0xD0029000 + 0x0);

		unsigned count = 0;
		while (readl(0xD0029000 + 0x18) & 0x10)
			++count;

		printf("%u", count);
	}

	for (;;) ; // ???*/


	// ??? printf("Test has been passed\n");

	return true;
}
#include "ddr_impl_h/ddr.h"
#include "ddr_impl_h/ddr_impl.h"
#include "ddr_impl_h/ddr_regs.h"
#include "ddr_impl_h/mivem_assert.h"
#include <stdint.h>
#include "ddr_impl_h/mivem_macro.h"
#include <stdbool.h>
#include "timer.h"
#include "ddr_impl_h/ppc_476fp_lib_c.h"
#include "ddr_impl_h/ddr/plb6mcif2.h"
#include "ddr_impl_h/ddr/ddr34lmc.h"
#include "ddr_impl_h/ddr/mcif2arb4.h"
#include <ddr_spd.h>

DECLARE_GLOBAL_DATA_PTR;

//static uint8_t __attribute__((section(".EM0.data"),aligned(16))) volatile ddr0_init_array[128] = { 0 };
//static uint8_t __attribute__((section(".EM1.data"),aligned(16))) volatile ddr1_init_array[128] = { 0 };

unsigned int *ddr0_init_array = 0x40000000;
unsigned int *ddr1_init_array = 0x80000000;

#define reg_field(field_right_bit_num_from_ppc_user_manual, value)\
    ((value) << IBM_BIT_INDEX( 32, field_right_bit_num_from_ppc_user_manual ))

#define T_SYS_RDLAT_BC4 0xc                     // get from debug register
#define T_SYS_RDLAT_BL8 0xc

//CONFIGURATION VALUES FOR DDR3-1600 (DEFAULT)
static CrgDdrFreq DDR_FREQ;// = DDR3_1600;

static ddr3config ddr_config;/* =
{
 18,            //T_RDDATA_EN_BC4               // RL-3 = CL + AL - 3
 18,            //T_RDDATA_EN_BL8

 11,            //CL_PHY
 0b111,         //CL_MC                         // CL_MC == SMR0 CL[3:1] ; SMR0 CL[0] == 0;
 0b111,         //CL_CHIP                       // CL = CL_TIME/tCK

 8,             //CWL_PHY
 0b010,         //CWL_MC                        //== SMR2 [CWL]
 0b011,         //CWL_CHIP

 10,            //AL_PHY
 0b01,          //AL_MC                         //== SMR1 [AL]
 0b01,          //AL_CHIP

 0b110,         //WR                            // SMR0, WR = TWR/tCK

 9999,          //T_REFI                        // SDTR0, T_REFI = 25000000/tI_MC_CLOCK = 25000000/2500 = 10000 (-1 is needed according to MC db)
 108,           //T_RFC_XPR                     // SDTR0, T_RFC_XPR = (TRFC_MIN + 10ns)/tI_MC_CLOCK

 0b1110,        //T_RCD                         // SDTR2, T_RCD = tRCD/tCK
 0b1110,        //T_RP                          // SDTR2, T_RP = tRP/tCK
 42,            //T_RC                          // SDTR2, T_RC = tRC/tCK
 28,            //T_RAS                         // SDTR2, T_RAS = tRAS/tCK

 0b0110,        //T_WTR                         // SDTR3, T_WTR = tWTR/tCK
 0b0110,        //T_RTP                         // SDTR3, T_RTP = tRTP/tCK
 16,            //T_XSDLL                       // SDTR3, TXSDLL = (tXSRD/PHY_to_MC_clock_ratio)/16

 12             //T_MOD                         // SDTR4, T_MOD = tMOD/tCK
};*/

// SUPPORT FUNCTIONS DECLARATIONS
static void ddr_plb6mcif2_init(const uint32_t baseAddr, const uint32_t puaba);
static void ddr_aximcif2_init(const uint32_t baseAddr);
static void ddr_mcif2arb4_init(const uint32_t baseAddr);
static void ddr3phy_init(uint32_t baseAddr, const DdrBurstLength burstLen);
static void ddr3phy_calibrate(const uint32_t baseAddr);
static void ddr3phy_calibrate_manual(const unsigned int baseAddr);
static void ddr34lmc_dfi_init(const uint32_t baseAddr);
static void ddr_init_main(DdrHlbId hlbId, DdrInitMode initMode, DdrEccMode eccMode, DdrPmMode powerManagementMode, DdrBurstLength burstLength, DdrPartialWriteMode partialWriteMode);
static void ddr_set_nopatch_config (CrgDdrFreq FreqMode);
static void ddr_set_main_config (CrgDdrFreq FreqMode);

void crg_ddr_config_freq ( uint32_t phy_base_addr, const DdrBurstLength burstLen);
void crg_cpu_upd_cken ();
void crg_ddr_upd_cken ();
void crg_cpu_upd_ckdiv ();
void crg_ddr_upd_ckdiv ();
void crg_remove_writelock ();
void crg_set_writelock ();

volatile uint32_t ioread32(uint32_t const base_addr)
{
    return *((volatile uint32_t*)(base_addr));
}


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

    const uint32_t Tsnoop = (plb6bc_dcr_read_PLB6BC_TSNOOP(PLB6_BC_BASE) & (0b1111 << 28)) >> 28;
    usleep(1000);
    plb6mcif2_dcr_write_PLB6MCIF2_PLBCFG(base_addr,
            //PLB6MCIF2 enable [31] = 0b1
              reg_field(0, 0b0) //Non-coherent
            | reg_field(4, Tsnoop) //T-snoop
            | reg_field(5, 0b0) //PmstrID_chk_EN
            | reg_field(27, 0b1111) //Parity enables
            | reg_field(30, 0b000) //CG_Enable
            | reg_field(31, 0b1)); //BR_Enable

}

void ddr_ddr34lmc_init(const uint32_t baseAddr,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode
);

//*************************************************DDR INIT FUNCTIONS*************************************************/
void ddr_init_impl_freq(
        DdrHlbId hlbId,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode,
        CrgDdrFreq FreqMode
        )
{
    TEST_ASSERT((FreqMode != DDR3_800), "This freq mode not supported");

    DDR_FREQ = FreqMode;

    ddr_set_main_config (DDR_FREQ);
    ddr_init_main(
            hlbId,
            initMode,
            eccMode,
            powerManagementMode,
            burstLength,
            partialWriteMode);
}

void ddr_init_impl_nopatch(
        DdrHlbId hlbId,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode,
        CrgDdrFreq FreqMode
        )
{
    TEST_ASSERT((FreqMode != DDR3_800), "This freq mode not supported");

    DDR_FREQ = FreqMode;

    ddr_set_nopatch_config (FreqMode);
    ddr_init_main(
            hlbId,
            initMode,
            eccMode,
            powerManagementMode,
            burstLength,
            partialWriteMode);
}

void init_ppc_for_test() {
    asm volatile (
	// initialize MMUCR
	"addis 0, 0, 0x0000   \n\t"  // 0x0000
	"ori   0, 0, 0x0000   \n\t"  // 0x0000
	"mtspr 946, 0          \n\t"  // SPR_MMUCR, 946
	"mtspr 48,  0          \n\t"  // SPR_PID,    48
	// Set up TLB search priority
	"addis 0, 0, 0x9ABC   \n\t"  // 0x1234
	"ori   0, 0, 0xDEF8   \n\t"  // 0x5678  
	//  "addis r0, r0, 0x1234   \n\t"  // 0x1234
	//	"ori   r0, r0, 0x5678   \n\t"  // 0x5678  
	"mtspr 830, 0          \n\t"  // SSPCR, 830
	"mtspr 831, 0          \n\t"  // USPCR, 831
	"mtspr 829, 0          \n\t"  // ISPCR, 829
	"isync                  \n\t"  // 
	"msync                  \n\t"  // 
	:::
    );   
};

void write_tlb_entry4 () {

 asm volatile (
	"addis 3, 0, 0x0000   \n\t"  // 0x0000, 0x0000
	"addis 4, 0, 0x4000   \n\t"  // 0x8200, 0x0000
	"ori   4, 4, 0x0bf0   \n\t"  // 0x08f0, 0x0BF0 04.04.2017 add_dauny
	"tlbwe 4, 3, 0        \n\t"

	"addis 4, 0, 0x0000   \n\t"  // 0x8200
	"ori   4, 4, 0x0000   \n\t"  // 0x0000
	"tlbwe 4, 3, 1        \n\t"

	"addis 4, 0, 0x0003  \n\t"  // 0x0003, 0000
	"ori   4, 4, 0x043f   \n\t"  // 0x043F, 023F
	"tlbwe 4, 3, 2        \n\t"
  "isync                  \n\t"  // 
  "msync                  \n\t"  // 
  :::
  );

};

void write_tlb_entry8 () {

 asm volatile (
	"addis 3, 0, 0x0000   \n\t"  // 0x0000, 0x0000
	"addis 4, 0, 0x8000   \n\t"  // 0x8200, 0x0000
	"ori   4, 4, 0x0bf0   \n\t"  // 0x08f0, 0x0BF0 04.04.2017 add_dauny
	"tlbwe 4, 3, 0        \n\t"

	"addis 4, 0, 0x0000   \n\t"  // 0x8200
	"ori   4, 4, 0x0002   \n\t"  // 0x0000
	"tlbwe 4, 3, 1        \n\t"

	"addis 4, 0, 0x0003   \n\t"  // 0x0003, 0000
	"ori   4, 4, 0x043f   \n\t"  // 0x043F, 023F
	"tlbwe 4, 3, 2        \n\t"
  "isync                  \n\t"  // 
  "msync                  \n\t"  // 
  :::
  );

};

uint32_t read_dcr_reg(uint32_t addr) {
  uint32_t res;

  asm volatile   (
  "mfdcrx (%0), (%1) \n\t"       // mfdcrx RT, RA
  :"=r"(res)
  :"r"((uint32_t) addr)
  :  
	);
  return res;
};

void write_dcr_reg(uint32_t addr, uint32_t value) {

  asm volatile   (
  "mtdcrx (%0), (%1) \n\t"       // mtdcrx RA, RS
  ::"r"((uint32_t) addr), "r"((uint32_t) value)
  :  
	);
};


void magic_settings()
{
    uint32_t * res_dcr = 0x00070100; // result dcr

    uint32_t dcr_addr=0;
    uint32_t dcr_val=0;
    uint32_t i;

    init_ppc_for_test();
    dcr_addr=0x80000200;            // BC_CR0
    res_dcr = 0x00070100;
    
    for(i=0; i<0x12; i++) {
	dcr_val = read_dcr_reg(dcr_addr);
	*res_dcr = dcr_val; res_dcr++;
	dcr_addr++;
    };
    
    // Set Seg0, Seg1, Seg2 and BC_CR0 
    write_dcr_reg(0x80000204, 0x00000010);  // BC_SGD1
    write_dcr_reg(0x80000205, 0x00000010);  // BC_SGD2
    dcr_val = read_dcr_reg(0x80000200);     // BC_CR0
    write_dcr_reg(0x80000200, dcr_val | 0x80000000);  

    dcr_addr=0x80000200;
    res_dcr = 0x00070150;
    for(i=0; i<0x12; i++) {
	dcr_val = read_dcr_reg(dcr_addr);
	*res_dcr = dcr_val; res_dcr++;
	dcr_addr++;
    };
    
    write_tlb_entry4();
    write_tlb_entry8();
}



/* Parameters for a DDR dimm computed from the SPD */
typedef struct dimm_params_s {

	/* DIMM organization parameters */
	char mpart[19];		/* guaranteed null terminated */

	unsigned int n_ranks;
	unsigned int die_density;
	unsigned long long rank_density;
	unsigned long long capacity;
	unsigned int data_width;
	unsigned int primary_sdram_width;
	unsigned int ec_sdram_width;
	unsigned int registered_dimm;
	unsigned int package_3ds;	/* number of dies in 3DS DIMM */
	unsigned int device_width;	/* x4, x8, x16 components */

	/* SDRAM device parameters */
	unsigned int n_row_addr;
	unsigned int n_col_addr;
	unsigned int edc_config;	/* 0 = none, 1 = parity, 2 = ECC */
	unsigned int n_banks_per_sdram_device;
	unsigned int burst_lengths_bitmask;	/* BL=4 bit 2, BL=8 = bit 3 */

	/* used in computing base address of DIMMs */
	unsigned long long base_address;
	/* mirrored DIMMs */
	unsigned int mirrored_dimm;	/* only for ddr3 */

	/* DIMM timing parameters */

	int mtb_ps;	/* medium timebase ps */
	int ftb_10th_ps; /* fine timebase, in 1/10 ps */
	int taa_ps;	/* minimum CAS latency time */
	int tfaw_ps;	/* four active window delay */

	/*
	 * SDRAM clock periods
	 * The range for these are 1000-10000 so a short should be sufficient
	 */
	int tckmin_x_ps;
	int tckmin_x_minus_1_ps;
	int tckmin_x_minus_2_ps;
	int tckmax_ps;

	/* SPD-defined CAS latencies */
	unsigned int caslat_x;
	unsigned int caslat_x_minus_1;
	unsigned int caslat_x_minus_2;

	unsigned int caslat_lowest_derated;	/* Derated CAS latency */

	/* basic timing parameters */
	int trcd_ps;
	int trp_ps;
	int tras_ps;

	int twr_ps;	/* maximum = 63750 ps */
	int trfc_ps;	/* max = 255 ns + 256 ns + .75 ns
				       = 511750 ps */
	int trrd_ps;	/* maximum = 63750 ps */
	int twtr_ps;	/* maximum = 63750 ps */
	int trtp_ps;	/* byte 38, spd->trtp */

	int trc_ps;	/* maximum = 254 ns + .75 ns = 254750 ps */

	int refresh_rate_ps;
	int extended_op_srt;

	/* DDR3 & DDR4 RDIMM */
	unsigned char rcw[16];	/* Register Control Word 0-15 */
} dimm_params_t;


static void dump_dimm_params(dimm_params_t *pdimm,
					 unsigned int dimm_number)
{
    int i;
    printf("Computed DIMM parameters for %d DIMM\n", dimm_number);
    printf("\tmpart:\t\t\t%s\n", pdimm->mpart);
    printf("    DIMM organization parameters:\n");
    printf("\tn_ranks:\t\t%d\n", pdimm->n_ranks);
    printf("\tdie_density:\t\t%d\n", pdimm->die_density);
    printf("\trank_density:\t\t%lld\n", pdimm->rank_density);
    printf("\tcapacity:\t\t%lld\n", pdimm->capacity);
    printf("\tdata_width:\t\t%d\n", pdimm->data_width);
    printf("\tprimary_sdram_width:\t%d\n", pdimm->primary_sdram_width);
    printf("\tec_sdram_width:\t\t%d\n", pdimm->ec_sdram_width);
    printf("\tregistered_dimm:\t%d\n", pdimm->registered_dimm);
    printf("\tpackage_3ds:\t\t%d\n", pdimm->package_3ds);
    printf("\tdevice_width:\t\t%d\n", pdimm->device_width);
    printf("    SDRAM device parameters:\n");
    printf("\tn_row_addr:\t\t%d\n", pdimm->n_row_addr);
    printf("\tn_col_addr:\t\t%d\n", pdimm->n_col_addr);
    printf("\tedc_config:\t\t%d\n", pdimm->edc_config);
    printf("\tbanks_per_sdram_device:\t%d\n", pdimm->n_banks_per_sdram_device);
    printf("\tburst_lengths_bitmask:\t%d\n", pdimm->burst_lengths_bitmask);
    printf("\tbase_address:\t\t%llx\n", pdimm->base_address);
    printf("\tmirrored_dimm:\t\t%d\n", pdimm->mirrored_dimm);
    printf("    DIMM timing parameters:\n");
    printf("\tmtb_ps:\t\t\t%d\n", pdimm->mtb_ps);
    printf("\tftb_10th_ps:\t\t%d\n", pdimm->ftb_10th_ps);
    printf("\ttaa_ps:\t\t\t%d\n", pdimm->taa_ps);
    printf("\ttfaw_ps:\t\t%d\n", pdimm->tfaw_ps);
    printf("    SDRAM clock periods:\n");
    printf("\ttckmin_x_ps:\t\t%d\n", pdimm->tckmin_x_ps);
    printf("\ttckmin_x_minus_1_ps:\t%d\n", pdimm->tckmin_x_minus_1_ps);
    printf("\ttckmin_x_minus_2_ps:\t%d\n", pdimm->tckmin_x_minus_2_ps);
    printf("\ttckmax_ps:\t\t%d\n", pdimm->tckmax_ps);
    printf("    SPD-defined CAS latencies:\n");
    printf("\tcaslat_x:\t\t%d\n", pdimm->caslat_x);
    printf("\tcaslat_x_minus_1:\t%d\n", pdimm->caslat_x_minus_1);
    printf("\tcaslat_x_minus_2:\t%d\n", pdimm->caslat_x_minus_2);
    printf("\tcaslat_lowest_derated:\t%d\n", pdimm->caslat_lowest_derated);
    printf("    basic timing parameters:\n");
    printf("\ttrcd_ps:\t\t%d\n", pdimm->trcd_ps);
    printf("\ttrp_ps:\t\t\t%d\n", pdimm->trp_ps);
    printf("\ttras_ps:\t\t%d\n", pdimm->tras_ps);
    printf("\ttwr_ps:\t\t\t%d\n", pdimm->twr_ps);
    printf("\ttrfc_ps:\t\t%d\n", pdimm->trfc_ps);
    printf("\ttrrd_ps:\t\t%d\n", pdimm->trrd_ps);
    printf("\ttwtr_ps:\t\t%d\n", pdimm->twtr_ps);
    printf("\ttrtp_ps:\t\t%d\n", pdimm->trtp_ps);
    printf("\ttrc_ps:\t\t\t%d\n", pdimm->trc_ps);
    printf("\trefresh_rate_ps:\t%d\n", pdimm->refresh_rate_ps);
    printf("\textended_op_srt:\t%d\n", pdimm->extended_op_srt);
    printf("\trcw:\t\t\t");
    for(i = 0; i < 16; i++)
    {
        printf("%02X ", pdimm->rcw[i]);
        if(i == 7)
            printf("\n\t\t\t\t");
    }
    printf("\n");
}


/*
 * Calculate the Density of each Physical Rank.
 * Returned size is in bytes.
 *
 * each rank size =
 * sdram capacity(bit) / 8 * primary bus width / sdram width
 *
 * where: sdram capacity  = spd byte4[3:0]
 *        primary bus width = spd byte8[2:0]
 *        sdram width = spd byte7[2:0]
 *
 * SPD byte4 - sdram density and banks
 *	bit[3:0]	size(bit)	size(byte)
 *	0000		256Mb		32MB
 *	0001		512Mb		64MB
 *	0010		1Gb		128MB
 *	0011		2Gb		256MB
 *	0100		4Gb		512MB
 *	0101		8Gb		1GB
 *	0110		16Gb		2GB
 *
 * SPD byte8 - module memory bus width
 * 	bit[2:0]	primary bus width
 *	000		8bits
 * 	001		16bits
 * 	010		32bits
 * 	011		64bits
 *
 * SPD byte7 - module organiztion
 * 	bit[2:0]	sdram device width
 * 	000		4bits
 * 	001		8bits
 * 	010		16bits
 * 	011		32bits
 *
 */
static unsigned long long
compute_ranksize(const ddr3_spd_eeprom_t *spd)
{
	unsigned long long bsize;

	int nbit_sdram_cap_bsize = 0;
	int nbit_primary_bus_width = 0;
	int nbit_sdram_width = 0;

	if ((spd->density_banks & 0xf) < 7)
		nbit_sdram_cap_bsize = (spd->density_banks & 0xf) + 28;
	if ((spd->bus_width & 0x7) < 4)
		nbit_primary_bus_width = (spd->bus_width & 0x7) + 3;
	if ((spd->organization & 0x7) < 4)
		nbit_sdram_width = (spd->organization & 0x7) + 2;

	bsize = 1ULL << (nbit_sdram_cap_bsize - 3
		    + nbit_primary_bus_width - nbit_sdram_width);

#ifdef DEBUG
	printf("DDR: DDR III rank density = 0x%016llx\n", bsize);
#endif

	return bsize;
}




/*
 * ddr_compute_dimm_parameters for DDR3 SPD
 *
 * Compute DIMM parameters based upon the SPD information in spd.
 * Writes the results to the dimm_params_t structure pointed by pdimm.
 *
 */
static unsigned int ddr_compute_dimm_parameters(const ddr3_spd_eeprom_t *spd,
					 dimm_params_t *pdimm,
					 unsigned int dimm_number)
{
	unsigned int retval;
	unsigned int mtb_ps;
	int ftb_10th_ps;
	int i;

    memset(pdimm, 0, sizeof(dimm_params_t));

	if (spd->mem_type) {
		if (spd->mem_type != SPD_MEMTYPE_DDR3) {
			printf("DIMM %u: is not a DDR3 SPD.\n", dimm_number);
			return 1;
		}
	} else {
		memset(pdimm, 0, sizeof(dimm_params_t));
		return 1;
	}

	retval = ddr3_spd_check(spd);
	if (retval) {
		printf("DIMM %u: failed checksum\n", dimm_number);
		return 2;
	}

	/*
	 * The part name in ASCII in the SPD EEPROM is not null terminated.
	 * Guarantee null termination here by presetting all bytes to 0
	 * and copying the part name in ASCII from the SPD onto it
	 */
	memset(pdimm->mpart, 0, sizeof(pdimm->mpart));
	if ((spd->info_size_crc & 0xF) > 1)
		memcpy(pdimm->mpart, spd->mpart, sizeof(pdimm->mpart) - 1);

	/* DIMM organization parameters */
	pdimm->n_ranks = ((spd->organization >> 3) & 0x7) + 1;
	pdimm->rank_density = compute_ranksize(spd);
	pdimm->capacity = pdimm->n_ranks * pdimm->rank_density;
	pdimm->primary_sdram_width = 1 << (3 + (spd->bus_width & 0x7));
	if ((spd->bus_width >> 3) & 0x3)
		pdimm->ec_sdram_width = 8;
	else
		pdimm->ec_sdram_width = 0;
	pdimm->data_width = pdimm->primary_sdram_width
			  + pdimm->ec_sdram_width;
	pdimm->device_width = 1 << ((spd->organization & 0x7) + 2);

	/* These are the types defined by the JEDEC DDR3 SPD spec */
	pdimm->mirrored_dimm = 0;
	pdimm->registered_dimm = 0;
	switch (spd->module_type & DDR3_SPD_MODULETYPE_MASK) {
	case DDR3_SPD_MODULETYPE_RDIMM:
	case DDR3_SPD_MODULETYPE_MINI_RDIMM:
	case DDR3_SPD_MODULETYPE_72B_SO_RDIMM:
		/* Registered/buffered DIMMs */
		pdimm->registered_dimm = 1;
		for (i = 0; i < 16; i += 2) {
			uint8_t rcw = spd->mod_section.registered.rcw[i/2];
			pdimm->rcw[i]   = (rcw >> 0) & 0x0F;
			pdimm->rcw[i+1] = (rcw >> 4) & 0x0F;
		}
		break;

	case DDR3_SPD_MODULETYPE_UDIMM:
	case DDR3_SPD_MODULETYPE_SO_DIMM:
	case DDR3_SPD_MODULETYPE_MICRO_DIMM:
	case DDR3_SPD_MODULETYPE_MINI_UDIMM:
	case DDR3_SPD_MODULETYPE_MINI_CDIMM:
	case DDR3_SPD_MODULETYPE_72B_SO_UDIMM:
	case DDR3_SPD_MODULETYPE_72B_SO_CDIMM:
	case DDR3_SPD_MODULETYPE_LRDIMM:
	case DDR3_SPD_MODULETYPE_16B_SO_DIMM:
	case DDR3_SPD_MODULETYPE_32B_SO_DIMM:
		/* Unbuffered DIMMs */
		if (spd->mod_section.unbuffered.addr_mapping & 0x1)
			pdimm->mirrored_dimm = 1;
		break;

	default:
		printf("unknown module_type 0x%02X\n", spd->module_type);
		return 1;
	}

	/* SDRAM device parameters */
	pdimm->n_row_addr = ((spd->addressing >> 3) & 0x7) + 12;
	pdimm->n_col_addr = (spd->addressing & 0x7) + 9;
	pdimm->n_banks_per_sdram_device = 8 << ((spd->density_banks >> 4) & 0x7);

	/*
	 * The SPD spec has not the ECC bit,
	 * We consider the DIMM as ECC capability
	 * when the extension bus exist
	 */
	if (pdimm->ec_sdram_width)
		pdimm->edc_config = 0x02;
	else
		pdimm->edc_config = 0x00;

	/*
	 * The SPD spec has not the burst length byte
	 * but DDR3 spec has nature BL8 and BC4,
	 * BL8 -bit3, BC4 -bit2
	 */
	pdimm->burst_lengths_bitmask = 0x0c;

	/* MTB - medium timebase
	 * The unit in the SPD spec is ns,
	 * We convert it to ps.
	 * eg: MTB = 0.125ns (125ps)
	 */
	mtb_ps = (spd->mtb_dividend * 1000) /spd->mtb_divisor;
	pdimm->mtb_ps = mtb_ps;

	/*
	 * FTB - fine timebase
	 * use 1/10th of ps as our unit to avoid floating point
	 * eg, 10 for 1ps, 25 for 2.5ps, 50 for 5ps
	 */
	ftb_10th_ps =
		((spd->ftb_div & 0xf0) >> 4) * 10 / (spd->ftb_div & 0x0f);
	pdimm->ftb_10th_ps = ftb_10th_ps;
	/*
	 * sdram minimum cycle time
	 * we assume the MTB is 0.125ns
	 * eg:
	 * tck_min=15 MTB (1.875ns) ->DDR3-1066
	 *        =12 MTB (1.5ns) ->DDR3-1333
	 *        =10 MTB (1.25ns) ->DDR3-1600
	 */
	pdimm->tckmin_x_ps = spd->tck_min * mtb_ps +
		(spd->fine_tck_min * ftb_10th_ps) / 10;

	/*
	 * CAS latency supported
	 * bit4 - CL4
	 * bit5 - CL5
	 * bit18 - CL18
	 */
	pdimm->caslat_x  = ((spd->caslat_msb << 8) | spd->caslat_lsb) << 4;

	/*
	 * min CAS latency time
	 * eg: taa_min =
	 * DDR3-800D	100 MTB (12.5ns)
	 * DDR3-1066F	105 MTB (13.125ns)
	 * DDR3-1333H	108 MTB (13.5ns)
	 * DDR3-1600H	90 MTB (11.25ns)
	 */
	pdimm->taa_ps = spd->taa_min * mtb_ps +
		(spd->fine_taa_min * ftb_10th_ps) / 10;

	/*
	 * min write recovery time
	 * eg:
	 * twr_min = 120 MTB (15ns) -> all speed grades.
	 */
	pdimm->twr_ps = spd->twr_min * mtb_ps;

	/*
	 * min RAS to CAS delay time
	 * eg: trcd_min =
	 * DDR3-800	100 MTB (12.5ns)
	 * DDR3-1066F	105 MTB (13.125ns)
	 * DDR3-1333H	108 MTB (13.5ns)
	 * DDR3-1600H	90 MTB (11.25)
	 */
	pdimm->trcd_ps = spd->trcd_min * mtb_ps +
		(spd->fine_trcd_min * ftb_10th_ps) / 10;

	/*
	 * min row active to row active delay time
	 * eg: trrd_min =
	 * DDR3-800(1KB page)	80 MTB (10ns)
	 * DDR3-1333(1KB page)	48 MTB (6ns)
	 */
	pdimm->trrd_ps = spd->trrd_min * mtb_ps;

	/*
	 * min row precharge delay time
	 * eg: trp_min =
	 * DDR3-800D	100 MTB (12.5ns)
	 * DDR3-1066F	105 MTB (13.125ns)
	 * DDR3-1333H	108 MTB (13.5ns)
	 * DDR3-1600H	90 MTB (11.25ns)
	 */
	pdimm->trp_ps = spd->trp_min * mtb_ps +
		(spd->fine_trp_min * ftb_10th_ps) / 10;

	/* min active to precharge delay time
	 * eg: tRAS_min =
	 * DDR3-800D	300 MTB (37.5ns)
	 * DDR3-1066F	300 MTB (37.5ns)
	 * DDR3-1333H	288 MTB (36ns)
	 * DDR3-1600H	280 MTB (35ns)
	 */
	pdimm->tras_ps = (((spd->tras_trc_ext & 0xf) << 8) | spd->tras_min_lsb)
			* mtb_ps;
	/*
	 * min active to actice/refresh delay time
	 * eg: tRC_min =
	 * DDR3-800D	400 MTB (50ns)
	 * DDR3-1066F	405 MTB (50.625ns)
	 * DDR3-1333H	396 MTB (49.5ns)
	 * DDR3-1600H	370 MTB (46.25ns)
	 */
	pdimm->trc_ps = (((spd->tras_trc_ext & 0xf0) << 4) | spd->trc_min_lsb)
			* mtb_ps + (spd->fine_trc_min * ftb_10th_ps) / 10;
	/*
	 * min refresh recovery delay time
	 * eg: tRFC_min =
	 * 512Mb	720 MTB (90ns)
	 * 1Gb		880 MTB (110ns)
	 * 2Gb		1280 MTB (160ns)
	 */
	pdimm->trfc_ps = ((spd->trfc_min_msb << 8) | spd->trfc_min_lsb)
			* mtb_ps;
	/*
	 * min internal write to read command delay time
	 * eg: twtr_min = 40 MTB (7.5ns) - all speed bins.
	 * tWRT is at least 4 mclk independent of operating freq.
	 */
	pdimm->twtr_ps = spd->twtr_min * mtb_ps;

	/*
	 * min internal read to precharge command delay time
	 * eg: trtp_min = 40 MTB (7.5ns) - all speed bins.
	 * tRTP is at least 4 mclk independent of operating freq.
	 */
	pdimm->trtp_ps = spd->trtp_min * mtb_ps;

	/*
	 * Average periodic refresh interval
	 * tREFI = 7.8 us at normal temperature range
	 *       = 3.9 us at ext temperature range
	 */
	pdimm->refresh_rate_ps = 7800000;
	if ((spd->therm_ref_opt & 0x1) && !(spd->therm_ref_opt & 0x2)) {
		pdimm->refresh_rate_ps = 3900000;
		pdimm->extended_op_srt = 1;
	}

	/*
	 * min four active window delay time
	 * eg: tfaw_min =
	 * DDR3-800(1KB page)	320 MTB (40ns)
	 * DDR3-1066(1KB page)	300 MTB (37.5ns)
	 * DDR3-1333(1KB page)	240 MTB (30ns)
	 * DDR3-1600(1KB page)	240 MTB (30ns)
	 */
	pdimm->tfaw_ps = (((spd->tfaw_msb & 0xf) << 8) | spd->tfaw_min)
			* mtb_ps;

	return 0;
}



int get_ddr3_spd_bypath(const char * spdpath, ddr3_spd_eeprom_t* spd);


void ddr_init()
{
    ddr3_spd_eeprom_t spd;
    dimm_params_t dpar0, dpar1;
    int dimm0_params_invalid = 1;
    int dimm1_params_invalid = 1;
    int res;


    if(!get_ddr3_spd_bypath("spd0-path", &spd))
    {
        dimm0_params_invalid = ddr_compute_dimm_parameters(&spd, &dpar0, 0);
    }
    if(!get_ddr3_spd_bypath("spd1-path", &spd))
    {
        dimm1_params_invalid = ddr_compute_dimm_parameters(&spd, &dpar1, 0);
    }
    if(!dimm0_params_invalid)
        dump_dimm_params(&dpar0, 0);

    if(!dimm1_params_invalid)
        dump_dimm_params(&dpar1, 1);

    //printf("LSIF Divider = %d\n", (*(volatile uint32_t *) (CRG_DDR_BASE + CRG_DDR_CKDIVMODE_LSIF)));

    magic_settings();
    ddr_init_impl_freq(
            DdrHlbId_Default,
            DdrInitMode_Default,
            DdrEccMode_Default,
            DdrPmMode_Default,
            DdrBurstLength_Default,
            DdrPartialWriteMode_Default,
            DDR3_DEFAULT);
            
    usleep(1000);
}

void ddr_init_em0()
{
    ddr_init_impl_freq(
            DdrHlbId_Em0,
            DdrInitMode_Default,
            DdrEccMode_Default,
            DdrPmMode_Default,
            DdrBurstLength_Default,
            DdrPartialWriteMode_Default,
            DDR3_DEFAULT);
}

void ddr_init_em1()
{
    ddr_init_impl_freq(
            DdrHlbId_Em1,
            DdrInitMode_Default,
            DdrEccMode_Default,
            DdrPmMode_Default,
            DdrBurstLength_Default,
            DdrPartialWriteMode_Default,
            DDR3_DEFAULT);
}

void ddr_init_impl(
        DdrHlbId hlbId,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode
        )
{
    ddr_init_impl_freq(
            hlbId,
            initMode,
            eccMode,
            powerManagementMode,
            burstLength,
            partialWriteMode,
            DDR3_DEFAULT);
}

static void ddr_init_main(
        DdrHlbId hlbId,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode
        )
{
    usleep(1000);

    if (hlbId & DdrHlbId_Em0)
    {
        ddr_plb6mcif2_init(EM0_PLB6MCIF2_DCR, 0x00000000);

        ddr_aximcif2_init(EM0_AXIMCIF2_DCR);

        ddr_mcif2arb4_init(EM0_MCIF2ARB_DCR);

        crg_ddr_config_freq (EM0_PHY__, burstLength); //& phy_init

        ddr_ddr34lmc_init(EM0_DDR3LMC_DCR,
                initMode,
                eccMode,
                powerManagementMode,
                burstLength,
                partialWriteMode);

        ddr3phy_calibrate(EM0_PHY__);
    }

    uint8_t T_SYS_RDLAT_configured = burstLength == DdrBurstLength_4 ? T_SYS_RDLAT_BC4 : T_SYS_RDLAT_BL8;
    uint8_t T_SYS_RDLAT_measured;
    if (hlbId & DdrHlbId_Em0)
    {
        asm volatile (
            "stmw 0, 0(%0)\n\t"
            ::"r"(ddr0_init_array)//woro
        );
        msync();
        //TODO: need to invalidate read address to avoid cache hit.
        dcbi(ddr0_init_array);//woro
        msync();
        (void)(MEM32(ddr0_init_array)); //generate read transaction woro
        usleep(1000);
        T_SYS_RDLAT_measured = ddr_get_T_SYS_RDLAT(EM0_DDR3LMC_DCR);

        if (T_SYS_RDLAT_measured != T_SYS_RDLAT_configured)
        {
            ddr_enter_self_refresh_mode(EM0_DDR3LMC_DCR);

            T_SYS_RDLAT_configured = T_SYS_RDLAT_measured;
            uint32_t SDTR4_reg = ddr34lmc_dcr_read_DDR34LMC_SDTR4(EM0_DDR3LMC_DCR);
            SDTR4_reg &= ~reg_field(15, 0b111111);
            ddr34lmc_dcr_write_DDR34LMC_SDTR4(EM0_DDR3LMC_DCR,
                    SDTR4_reg | reg_field(15, T_SYS_RDLAT_configured));

            ddr_exit_self_refresh_mode(EM0_DDR3LMC_DCR);

            //TODO: need to invalidate read address to avoid cache hit.
            dcbi(ddr0_init_array); //woro
            msync();
            (void)(MEM32(ddr0_init_array)); //generate read transaction woro
            T_SYS_RDLAT_measured = ddr_get_T_SYS_RDLAT(EM0_DDR3LMC_DCR);
            TEST_ASSERT(T_SYS_RDLAT_measured == T_SYS_RDLAT_configured, "EM0 DDR34LMC configuration error");
        }

    }

    //EM1
    if (hlbId & DdrHlbId_Em1)
    {
        ddr_plb6mcif2_init(EM1_PLB6MCIF2_DCR, 0x00000002);

        ddr_aximcif2_init(EM1_AXIMCIF2_DCR);

        ddr_mcif2arb4_init(EM1_MCIF2ARB_DCR);

        if (hlbId & DdrHlbId_Em0) //some magic...
        {
            ddr3phy_init(EM1_PHY__, burstLength);
        }
        else
            crg_ddr_config_freq (EM1_PHY__, burstLength);  //& phy_init


        ddr_ddr34lmc_init(EM1_DDR3LMC_DCR,
                initMode,
                eccMode,
                powerManagementMode,
                burstLength,
                partialWriteMode);

        ddr3phy_calibrate(EM1_PHY__);
        
        if (hlbId & DdrHlbId_Em1)
        {
            asm volatile (
                "stmw 0, 0(%0)\n\t"
                ::"r"(ddr1_init_array)
            );
            msync();
            dcbi(ddr1_init_array);
            msync();
            (void)(MEM32((uint32_t)ddr1_init_array)); //generate read transaction
            usleep(1000);
            T_SYS_RDLAT_measured = ddr_get_T_SYS_RDLAT(EM1_DDR3LMC_DCR);

            if (T_SYS_RDLAT_measured != T_SYS_RDLAT_configured)
            {
                ddr_enter_self_refresh_mode(EM1_DDR3LMC_DCR);

                T_SYS_RDLAT_configured = T_SYS_RDLAT_measured;
                uint32_t SDTR4_reg = ddr34lmc_dcr_read_DDR34LMC_SDTR4(EM1_DDR3LMC_DCR);
                SDTR4_reg &= ~reg_field(15, 0b111111);
                ddr34lmc_dcr_write_DDR34LMC_SDTR4(EM1_DDR3LMC_DCR,
                        SDTR4_reg | reg_field(15, T_SYS_RDLAT_configured));

                ddr_exit_self_refresh_mode(EM1_DDR3LMC_DCR);

                //TODO: need to invalidate read address to avoid cache hit.
                dcbi(ddr1_init_array);
                msync();
                (void)(MEM32((uint32_t)ddr1_init_array)); //generate read transaction
                T_SYS_RDLAT_measured = ddr_get_T_SYS_RDLAT(EM1_DDR3LMC_DCR);
                TEST_ASSERT(T_SYS_RDLAT_measured == T_SYS_RDLAT_configured, "EM1 DDR34LMC configuration error");
            }

        }
    }
    //-------------------
}

static void ddr_plb6mcif2_init(const uint32_t baseAddr, const uint32_t puaba)
{
    plb6mcif2_simple_init(baseAddr,puaba);
}

static void ddr_aximcif2_init(const uint32_t baseAddr)
{
    //Software must properly configure the AXIASYNC, AXICFG, AXIRNK0, AXIRNK1, AXIRNK2 and
    //AXIRNK3 registers before starting read/write transfers.

    //Mask errors, do not generate interrupt
    aximcif2_dcr_write_AXIMCIF2_AXIERRMSK(baseAddr, 0xff000000);

    aximcif2_dcr_write_AXIMCIF2_AXIASYNC(baseAddr,
              (0b000 << 25) //AXINUM,
            | (0b0000000 << 18) //MCIFNUM
            | (0b0000 << 8) //RDRDY_TUNE
            | (0b0 << 5) //DCR_CLK_MN
            | (0b00000 << 0) //DCR_CLK_RATIO
    );

    aximcif2_dcr_write_AXIMCIF2_AXICFG(baseAddr,
              (0b0 << 15) //IT_OOO,
            | (0b00 << 13) //TOM
            | (0b0 << 1) //EQ_ID
            | (0b0 << 0) // EXM_ID
    );

    //EM0 and EM1 use 4GB x 2rank external DDR memory configuration
    uint32_t R_RA = 0;
    uint32_t R_RS = 0;
    uint32_t R_RO = 0;
    switch(baseAddr)
    {
    case EM0_AXIMCIF2_DCR:
        {
            //EM0 R_RA - AXI Address Bits [35:24]
            //EM0 AXI address 0x0_40000000 is mapped to the starting 1GB of EM0 rank0
            R_RA = 0x040;
            R_RS = 0b0110; //1GB
            R_RO = 0; //zero offset
        }
        break;
    case EM1_AXIMCIF2_DCR:
        {
            //EM1 R_RA - AXI Address Bits [35:24]
            //EM1 AXI address 0x0_80000000 is mapped to the starting 2GB of EM1 rank0
            R_RA = 0x080;
            R_RS = 0b0111; break; //2GB
            R_RO = 0; //zero offset
        }
        break;
    default: TEST_ASSERT(0, "Unexpected base address");
    }

    aximcif2_dcr_write_AXIMCIF2_MR0CF(baseAddr,
              ((R_RA & 0xfff) << 20)
            | ((R_RS & 0xf) << 16)
            | ((R_RO & 0b111111111111111) << 1)
            | (0b1 << 0) //R_RV
    );

    //disable other ranks as not accessible in AXI address space
    aximcif2_dcr_write_AXIMCIF2_MR1CF(baseAddr, 0);
    aximcif2_dcr_write_AXIMCIF2_MR2CF(baseAddr, 0);
    aximcif2_dcr_write_AXIMCIF2_MR3CF(baseAddr, 0);
}
static void ddr_mcif2arb4_init(const uint32_t baseAddr)
{
    mcif2arb4_dcr_write_MCIF2ARB4_MACR(baseAddr,
              (0b0000 << 28) //MPM
            | (0b1 << 27) //WD_DLY
            | (0b00 << 8)); //ORDER_ATT
}
static void ddr3phy_init(uint32_t baseAddr, const DdrBurstLength burstLen)
{
    uint32_t reg = 0;
    reg = ddr3phy_read_DDR3PHY_PHYREG00(baseAddr);
    ddr3phy_write_DDR3PHY_PHYREG00(baseAddr,
            reg & ~(0b11 << 2)); // Soft reset enter

    reg = ddr3phy_read_DDR3PHY_PHYREG00(baseAddr);
    ddr3phy_write_DDR3PHY_PHYREG00(baseAddr,
            reg | (0b11 << 2)); // Soft reset exit

    reg = ddr3phy_read_DDR3PHY_PHYREG01(baseAddr);
    reg &= ~0b111;
    ddr3phy_write_DDR3PHY_PHYREG01(baseAddr,
            (burstLen == DdrBurstLength_4) ? (reg | 0b000) : (reg | 0b100) ); // Set DDR3 mode

    reg = ddr3phy_read_DDR3PHY_PHYREG0b(baseAddr);
    reg &= ~0xff;
    ddr3phy_write_DDR3PHY_PHYREG0b(baseAddr,
            reg | (ddr_config.CL_PHY << 4) | (ddr_config.AL_PHY << 0)); // Set CL, AL

    reg = ddr3phy_read_DDR3PHY_PHYREG0c(baseAddr);
    reg &= ~0xf;
    ddr3phy_write_DDR3PHY_PHYREG0c(baseAddr,
            reg | (ddr_config.CWL_PHY << 0)); // CWL = 8

    //2. Configure DDR PHY PLL output to provide stable clock to SDRAM
    //Setup PHY internal PLL
    //PHY.ddr_refclk = 400MHz
    //To generate 800MHz on PHY.CK0 we have to setup PLL on 1600MHz using M/N dividers
    //where M - PLL feedback divider (= 4 for 400MHz)
    //      N - PLL pre-divider (= 1 for 400MHz)

    ddr3phy_write_DDR3PHY_PHYREGEE(baseAddr,0x2); // set PLL Pre-divide

    switch (DDR_FREQ)
    {
      case DDR3_1066:
          ddr3phy_write_DDR3PHY_PHYREGEC(baseAddr, 0x20); // set PLL Feedback divide
          break;
      case DDR3_1333:
          ddr3phy_write_DDR3PHY_PHYREGEC(baseAddr, 0x28); // set PLL Feedback divide
          break;
      case DDR3_1600:
          ddr3phy_write_DDR3PHY_PHYREGEC(baseAddr, 0x20); // set PLL Feedback divide
          break;
      default: TEST_ASSERT(0,"Invalid DDR FreqMode");
    }

    ddr3phy_write_DDR3PHY_PHYREGED(baseAddr, 0x18); //PLL clock out enable
    
    while (((ioread32(SCTL_BASE + SCTL_PLL_STATE)>>2)&1) != 1) //#ev Ожидаем установления частоты
    {
    }
    usleep(5); //let's wait 5us
    
    //usleep(4);

    ddr3phy_write_DDR3PHY_PHYREGEF(baseAddr, 0x1);

}
static void ddr34lmc_dfi_init(const uint32_t baseAddr)
{
    //DFI init must be done before step 7. enabling dfi_cke.

    //9. DFI init.

    uint32_t reg = 0;

    reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            reg | reg_field(19, 0b1)); //MCOPT2[19] = 0b1 DFI_INIT_START

    //PHY spec says we must wait for 2500ps*2000dfi_clk1x = 5us
    //let's wait a bit more
    usleep(10);

    //check DFI_INIT_COMPLETE
    reg = ddr34lmc_dcr_read_DDR34LMC_MCSTAT(baseAddr);
    TEST_ASSERT(reg & reg_field(3, 0b1), "DFI_INIT_COMPLETE timeout");

    //MCOPT2[DFI_INIT_START] must stay LOW. See PHY spec notes.
}

void ddr_ddr34lmc_init(const uint32_t baseAddr,
        DdrInitMode initMode,
        DdrEccMode eccMode,
        DdrPmMode powerManagementMode,
        DdrBurstLength burstLength,
        DdrPartialWriteMode partialWriteMode
)
{
    TEST_ASSERT(powerManagementMode == DdrPmMode_DisableAllPowerSavingFeatures, "Power saving features not supported");

    //1. System reset
    //Done by the system hardware

    //9. Trigger DDR PHY initialization to provide stable clock
    ddr34lmc_dfi_init(baseAddr);

    //Must wait for 200us after I_MC_RESET was deactivated
    if (initMode == DdrInitMode_FollowSpecWaitRequirements)
    {
        usleep(200); //in microseconds
    }
    else if (initMode == DdrInitMode_ViolateSpecWaitRequirements)
    {
    }

    //3. Deactivate SDRAM reset
    uint32_t mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            mcopt2_reg & ~(reg_field(11, 0b1111))); //MCOPT2[8:11] = 0b0000 RESET_RANK

    //4. MC enters power-on mode
    // Done automatically after reset

    //5. Configure MC regs
    // Total EM0 DDR RAM 8GB + 2GB(ECC) = 10GB	#ev - 2GB + 0.5GB(ECC)
    // PHY DQ width = 32 + 8(ECC) = 40 bit	
    // Die DQ width = x8	#ev - x16
    // Rank num = 2 (this is the DDR PHY limit, not the MC)	#ev - 1
    // Die num = (4 + 1(ECC)) x 2rank = 10 dies	#ev - (2 + 1(ECC)) x 1rank
    // Bank num = 8 (See JEDEC for 1Gbx8)
    // Quarter mode
    // MCIF_DATA_WIDTH = 128
    // BUILT_IN_ECC = 1
    // No ECC path-through
    // Column address width = 11 (See JEDEC for 1Gbx8)
    // Addressing mode = 3. Details are below
    //      Based on column bits number for 1Gbx8 in JEDEC spec table 2.11.5 Column address: A0-A9, A11.
    //      Then choose addressing mode from MC db table 3-52 based on our config: column addr width = 11, bank num = 8, quarter mode, 128 bit
    //      This is 3 | Nx11 (8) | 128
    //MCIF commands queue depth = 4
    //ODT = OFF
    //DDR SDRAM1600 11-11-11
    //I_MC_CLOCK = 400MHz //MC clock
    //tI_MC_CLOCK = 2500ps
    //dfi_clk1x = 400MHz //PHY clock
    //tdfi_clk1x = 2500ps
    //dfi_clk4x = 1600MHz //another PHY clock
    //tdfi_clk4x = 625ps
    //CK0 = 800MHz //DDR SDRAM clock rank0
    //CK1 = 800MHz //DDR SDRAM clock rank1
    //tCK = 1250ps

    ddr34lmc_dcr_write_DDR34LMC_MCOPT1(baseAddr,	//#ev
//      PROTOCOL             DM_ENABLE                       [ECC_EN : ECC_COR]      RDIMM                 PMUM               WIDTH[0]              WIDTH[1]           PORT_ID_CHECK_EN      UIOS              QUADCS_RDIMM        ZQCL_EN               WD_DLY               QDEPTH              RWOO                 WOOO                 DCOO                  DEF_REF              DEV_TYPE             CA_PTY_DLY           ECC_MUX               CE_THRESHOLD
        reg_field(0, 0b0) | reg_field(1, partialWriteMode) | reg_field(3, eccMode) | reg_field(4, 0b0) | reg_field(6, 0b00) | reg_field(7, 0b0) | reg_field(12, 0b0) | reg_field(8, 0b0) | reg_field(9, 0) | reg_field(10, 0b0) | reg_field(11, 0b0) | reg_field(13, 0b1) | reg_field(15, 0b10) | reg_field(16, 0b0) | reg_field(17, 0b0) | reg_field(18, 0b1) | reg_field(19, 0b0) | reg_field(20, 0b1) | reg_field(21, 0b0) | reg_field(23, 0b00) | reg_field(31, 0b1));

    ddr34lmc_dcr_write_DDR34LMC_IOCNTL(baseAddr, 0);

    ddr34lmc_dcr_write_DDR34LMC_PHYCNTL(baseAddr, 0);

    //Init ODTR0 - ODTR3
    ddr34lmc_dcr_write_DDR34LMC_ODTR0(baseAddr, 0x02000000);	//#ev - ODT write rank 0 enable

    ddr34lmc_dcr_write_DDR34LMC_ODTR1(baseAddr, 0);

    ddr34lmc_dcr_write_DDR34LMC_ODTR2(baseAddr, 0);

    ddr34lmc_dcr_write_DDR34LMC_ODTR3(baseAddr, 0);

    // AlSp ToDo: get number of Ranks and its parameters from SPD
    //Init CFGR0 - CFGR3
    ddr34lmc_dcr_write_DDR34LMC_CFGR0(baseAddr, //#ev - ADDR_MODE 11->10
//           ROW_WIDTH          ADDR_MODE               MIRROR              RANK_ENABLE
        reg_field(19, 0b100) | reg_field(23, 0b0010) | reg_field(27, 0b0) | reg_field(31, 0b1));

    ddr34lmc_dcr_write_DDR34LMC_CFGR1(baseAddr,	//#ev - RANK_ENABLE 1->0
//     RANK_ENABLE
        reg_field(19, 0b100) | reg_field(23, 0b0010) | reg_field(27, 0b0) | reg_field(31, 0b1));    // AlSp: both ranks enabled and initialized?

    ddr34lmc_dcr_write_DDR34LMC_CFGR2(baseAddr,
//     RANK_ENABLE
       reg_field(31, 0b0));

    ddr34lmc_dcr_write_DDR34LMC_CFGR3(baseAddr,
    //   RANK_ENABLE
            reg_field(31, 0b0));

    //Init SMR0 - SMR3
    ddr34lmc_dcr_write_DDR34LMC_SMR0(baseAddr,
        //WR = TWR/tCK = 15000/1250 = 12
        //CL = CL_TIME/tCK = 13750/1250 = 11
//      PPD                  WR                               DLL (not used)          TM (not used)                 CL[3:1]                       CL[0]                RBT                 BL
        reg_field(19, 0b1) | reg_field(22, ddr_config.WR) | reg_field(23, 0b1) | reg_field(24, 0b0) | reg_field(27, ddr_config.CL_MC) | reg_field(29, 0b0) | reg_field(28, 0b0) | reg_field(31, burstLength));

    ddr34lmc_dcr_write_DDR34LMC_SMR1(baseAddr,	//#ev bit 30, 29: 00->11, 31: 1->0
//      QOFF (not used)         TDQS (not used)     RTT_Nom[2]         RTT_Nom[1]           RTT_Nom[0]         WR_Level (not used)       DIC[1]             DIC[0]                 AL                   DLL (not used)
        reg_field(19, 0b0) | reg_field(20, 0b0) | reg_field(22, 0b0) | reg_field(25, 0b0) | reg_field(29, 0b1) | reg_field(24, 0b0) | reg_field(26, 0b0) | reg_field(30, 0b1) | reg_field(28, ddr_config.AL_MC) | reg_field(31, 0b0));

    ddr34lmc_dcr_write_DDR34LMC_SMR2(baseAddr,
    //For CWL calculation see function min_cwl in SDRAM parameters.
    //CWL = 8
    //      RTT_WR (not used)    SRT (not used)           ASR (not used)    CWL                     PASR (not used)
            reg_field(22, 0b00) | reg_field(24, 0b0) | reg_field(25, 0) |  reg_field(28, ddr_config.CWL_MC) | reg_field(31, 0b000));

    ddr34lmc_dcr_write_DDR34LMC_SMR3(baseAddr,
    //      MPR (not used)         MPR_SEL (not used)
            reg_field(29, 0b0) | reg_field(31, 0b00));

    //Init SDTR0 - SDTR5
    ddr34lmc_dcr_write_DDR34LMC_SDTR0(baseAddr,
    //TRFC_MIN=  260000ps
    //TRFC_MAX=70200000ps
    //Set refresh interval for example 25000000ps
    //T_REFI = 25000000/tI_MC_CLOCK = 25000000/2500 = 10000 (-1 is needed according to MC db)
    //T_RFC_XPR = (TRFC_MIN + 10ns)/tI_MC_CLOCK = (260000 + 10000)/2500 = 108
    //   T_REFI                             T_RFC_XPR
        reg_field(15, ddr_config.T_REFI) | reg_field(31, ddr_config.T_RFC_XPR));

    ddr34lmc_dcr_write_DDR34LMC_SDTR1(baseAddr,
    //TODO: must be consistent with PHY. See MC db Note
    //      T_LEADOFF           ODT_DELAY          ODT_WIDTH            T_WTRO                  T_RTWO                  T_RTW_ADJ               T_WTWO                  T_RTRO
            reg_field(0, 0b0) | reg_field(1, 0b0) | reg_field(2, 0b0) | reg_field(7, 0b0000) | reg_field(15, 0b0000) | reg_field(19, 0b0000) | reg_field(23, 0b0000) | reg_field(31, 0b0000));

    ddr34lmc_dcr_write_DDR34LMC_SDTR2(baseAddr,
    //T_RCD = tRCD/tCK = 13750/1250 = 11
    //T_RP = tRP/tCK = 13750/1250 = 11
    //T_RC = tRC/tCK = 48750/1250 = 39
    //T_RAS = tRAS/tCK = 35000/1250 = 28
    //      T_CWL (not used)           T_RCD[1:4]                           T_RCD[0]            T_PL                    T_RP[1:4]                        T_RP[0]               T_RC                         T_RAS
            reg_field(3, 0b1000) |     reg_field(7, ddr_config.T_RCD) | reg_field(17, 0b0) | reg_field(11, 0b0000) | reg_field(15, ddr_config.T_RP) | reg_field(16, 0b0) | reg_field(23, ddr_config.T_RC) | reg_field(31, ddr_config.T_RAS));

    ddr34lmc_dcr_write_DDR34LMC_SDTR3(baseAddr,
    //T_WTR_S = tWTR_DG/tCK = 3750/1250 = 3
    //T_WTR = tWTR/tCK = 7500/1250 = 6
    //T_RTP = tRTP/tCK = 7500/1250 = 6
    //T_RRD = TRRD_TCK = 4
    //TXSDLL = (tXSRD/PHY_to_MC_clock_ratio)/16 = (512/2)/16 = 16
    //T_FAWADJ = 2. See Table 3-19 MC db
    //      T_WTR_S (not used)      T_WTR                                T_FAWADJ                   T_RTP                   T_RRD_L (not used)      T_RRD                        T_XSDLL
            reg_field(3, 0b0011) | reg_field(7, ddr_config.T_WTR) | reg_field(11, 0b10) | reg_field(15, ddr_config.T_RTP) | reg_field(19, 0b0010) | reg_field(23, 0b0100) | reg_field(31, ddr_config.T_XSDLL));

    const uint8_t T_SYS_RDLAT = (burstLength == DdrBurstLength_4) ? T_SYS_RDLAT_BC4 : T_SYS_RDLAT_BL8; //Experimental values. TODO: update comments below after MMPPC-548
    const uint32_t T_RDDATA_EN = (burstLength == DdrBurstLength_4) ? ddr_config.T_RDDATA_EN_BC4 : ddr_config.T_RDDATA_EN_BL8; //Experimental values. TODO: update comments below after MMPPC-548
    ddr34lmc_dcr_write_DDR34LMC_SDTR4(baseAddr,
    //t_rddata_en = (RL-1)/2-1 in tdfi_clk1x. See PHY spec
    // where RL = AL+CL = 0+11. See JEDEC spec
    //T_RDDATA_EN = t_rddata_en*tdfi_clk1x/tCK = ((11-1)/2-1)*2500/1250 = 8
    //T_SYS_RDLAT = tdfi_read_latency - (AL+CL) = tdfi_read_latency - 11
    //where tdfi_read_latency is from read cmd (cs) to rtdata_valid and it can be get from wave form
    //T_SYS_RDLAT = 17 - experimental value. See T_SYS_RDLAT_DBG register
    //T_CCD = 4
    //T_CPDED = default = 1
    //T_MOD = tMOD/tCK = 15000/1250 = 12
    //      T_RDDATA_EN[0:6]            T_SYS_RDLAT                  T_CCD_L (not used)          T_CCD                 T_CPDED                 T_MOD
            reg_field(7, T_RDDATA_EN) | reg_field(15, T_SYS_RDLAT) | reg_field(19, 0b0100) | reg_field(23, 0b100) | reg_field(26, 0b000) | reg_field(31, ddr_config.T_MOD));

    ddr34lmc_dcr_write_DDR34LMC_SDTR5(baseAddr,
    //MC uses T_WHY_WRDATA value like this T_PHY_WRLAT = (CWL - 2*T_PHY_WRDATA)/2 (units: I_MC_CLOCK), where T_PHY_WRLAT is a time from WRITE cmd to dfi_wrdata_en signal
    //TWRLAT = (WL-1)/2 (units: I_MC_CLOCK) = 3.5; T_WRDAT = (CWL - 2*T_PHY_WRLAT)/2 (units: I_MC_CLOCK) = 0.5 (units: I_MC_CLOCK)
            reg_field(7, 0b000)); //TODO: how to set 0.5?

    //Init INITSEQn and INITCMDn
    //According to Micron 8Gb_DDR3L.pdf Figure 46: Initialization Sequence must be as follows
    //MRS-MR2
    //MRS-MR3
    //MRS-MR1 DLL enable
    //MRS-MR0 with DLL reset
    //ZQCL

    //MRS-MR2
    ddr34lmc_dcr_write_DDR34LMC_INITSEQ0(baseAddr,
    //Wait = TMRD*tCK/tI_MC_CLOCK = 4*1250/2500 = 2
    //RANKx is anded with CFGRx[RANK_ENABLE]. Let's set all ranks here
    //      ENABLE              WAIT                EN_MULTI_RANK_SELECT      RANK
            reg_field(0, 0b1) | reg_field(15, 2) | reg_field(27, 1) |         reg_field(31, 0b1111));

    ddr34lmc_dcr_write_DDR34LMC_INITCMD0(baseAddr,

    //See JEDEC 3.4.4 Mode Register MR2
    //DDR pin            RAS#                   CAS#                    WE#                 BA2                 BA1                  BA0                  A15 A14 A13 A12 A11      A10 A9                A8                   A7                   A6                   A5 A4 A3               A2 A1 A0
    //Value              0                      0                       0                   0                   1                    0                    0   0   0   0   0        Rtt_WR                0                    SRT                  ASR                  CWL                    PASR
    //INITCMD            CMD[0]                 CMD[1]                  CMD[2]              BANK[0]             BANK[1]              BANK[2]              ADDR[0:4]                ADDR[5:6]             ADDR[7]              ADDR[8]              ADDR[9]              ADDR[10:12]            ADDR[13:15]
                         reg_field(0, 0b0) |    reg_field(1, 0b0) |     reg_field(2, 0b0) | reg_field(9, 0b0) | reg_field(10, 0b1) | reg_field(11, 0b0) | reg_field(20, 0b00000) | reg_field(22, 0b00) | reg_field(23, 0b0) | reg_field(24, 0b0) | reg_field(25, 0b0) | reg_field(28, ddr_config.CWL_CHIP) | reg_field(31, 0b000));

    ddr34lmc_dcr_write_DDR34LMC_INITSEQ1(baseAddr,
    //Wait = TMRD*tCK/tI_MC_CLOCK = 4*1250/2500 = 2
    //RANKx is anded with CFGRx[RANK_ENABLE]. Let's set all ranks here
    //      ENABLE              WAIT                EN_MULTI_RANK_SELECT      RANK
            reg_field(0, 0b1) | reg_field(15, 2) | reg_field(27, 1) |         reg_field(31, 0b1111));

    ddr34lmc_dcr_write_DDR34LMC_INITCMD1(baseAddr,
    //See JEDEC 3.4.5 Mode Register MR3
    //DDR pin            RAS#                   CAS#                    WE#                 BA2                 BA1                  BA0                  A15 A14 A13 A12 A11 A10 A9 A8 A7 A6 A5 A4 A3      A2                    A1 A0
    //Value              0                      0                       0                   0                   1                    1                    0   0   0   0   0   0   0  0  0  0  0  0  0       MPR                   MPR_Loc
    //INITCMD            CMD[0]                 CMD[1]                  CMD[2]              BANK[0]             BANK[1]              BANK[2]              ADDR[0:12]                                        ADDR[13]              ADDR[14:15]
                         reg_field(0, 0b0) |    reg_field(1, 0b0) |     reg_field(2, 0b0) | reg_field(9, 0b0) | reg_field(10, 0b1) | reg_field(11, 0b1) | reg_field(28, 0) |                                reg_field(29, 0b0)  | reg_field(31, 0b00));

    ddr34lmc_dcr_write_DDR34LMC_INITSEQ2(baseAddr,
    //Wait = TMRD*tCK/tI_MC_CLOCK = 4*1250/2500 = 2
    //RANKx is anded with CFGRx[RANK_ENABLE]. Let's set all ranks here
    //      ENABLE              WAIT                EN_MULTI_RANK_SELECT      RANK
            reg_field(0, 0b1) | reg_field(15, 2) | reg_field(27, 1) |         reg_field(31, 0b1111));

    ddr34lmc_dcr_write_DDR34LMC_INITCMD2(baseAddr,	//#ev bit30, 29: 00->11
    //See JEDEC 3.4.3 Mode Register MR1
    //DDR pin            RAS#                   CAS#                    WE#                 BA2                 BA1                  BA0                  A15 A14 A13        A12                  A11                  A10                A9                   A8                 A7                   A6                   A5                   A4 A3                 A2                   A1                   A0
    //Value              0                      0                       0                   0                   0                    1                    0   0   0          Qoff                 TDQS                 0                  Rtt_Nom[2]           0                  Level                Rtt_Nom[1]           DIC[1]               AL                    Rtt_Nom[0]           DIC[0]               DLL
    //INITCMD            CMD[0]                 CMD[1]                  CMD[2]              BANK[0]             BANK[1]              BANK[2]              ADDR[0:2]          ADDR[3]              ADDR[4]              ADDR[5]            ADDR[6]              ADDR[7]            ADDR[8]              ADDR[9]              ADDR[10]             ADDR[11:12]           ADDR[13]             ADDR[14]             ADDR[15]
                         reg_field(0, 0b0) |    reg_field(1, 0b0) |     reg_field(2, 0b0) | reg_field(9, 0b0) | reg_field(10, 0b0) | reg_field(11, 0b1) | reg_field(18, 0) | reg_field(19, 0b0) | reg_field(20, 0b0) | reg_field(21, 0) | reg_field(22, 0b0) | reg_field(23, 0) | reg_field(24, 0b0) | reg_field(25, 0b0) | reg_field(26, 0b0) | reg_field(28, ddr_config.AL_CHIP) | reg_field(29, 0b1) | reg_field(30, 0b1) | reg_field(31, 0b0));

    ddr34lmc_dcr_write_DDR34LMC_INITSEQ3(baseAddr,
    //Wait = tMOD/tI_MC_CLOCK = 15000/2500 = 6
    //RANKx is anded with CFGRx[RANK_ENABLE]. Let's set all ranks here
    //      ENABLE              WAIT              EN_MULTI_RANK_SELECT       RANK
            reg_field(0, 0b1) | reg_field(15, 6) | reg_field(27, 1) |         reg_field(31, 0b1111));

    ddr34lmc_dcr_write_DDR34LMC_INITCMD3(baseAddr,	//#ev WR 110 -> ddr_config.WR
    //See JEDEC 3.4.2 Mode Register MR0
    //DDR pin            RAS#                   CAS#                    WE#                 BA2                 BA1                  BA0                  A15 A14 A13        A12                  A11 A10 A9             A8                   A7                   A6 A5 A4               A3                   A2                   A1 A0
    //Value              0                      0                       0                   0                   0                    0                    0   0   0          PPD                  WR                     DLL                  TM                   CL[3:1]                RBT                  CL[0]                BL
    //INITCMD            CMD[0]                 CMD[1]                  CMD[2]              BANK[0]             BANK[1]              BANK[2]              ADDR[0:2]          ADDR[3]              ADDR[4:6]              ADDR[7]              ADDR[8]              ADDR[9:11]             ADDR[12]             ADDR[13]             ADDR[14:15]
                        reg_field(0, 0b0) |    reg_field(1, 0b0) |     reg_field(2, 0b0) | reg_field(9, 0b0) | reg_field(10, 0b0) | reg_field(11, 0b0) | reg_field(18, 0) | reg_field(19, 0b1) | reg_field(22, ddr_config.WR) | reg_field(23, 0b1) | reg_field(24, 0b0) | reg_field(27, ddr_config.CL_CHIP) | reg_field(28, 0b0) | reg_field(29, 0b0) | reg_field(31, burstLength));

    ddr34lmc_dcr_write_DDR34LMC_INITSEQ4(baseAddr,
    //Wait = tZQINIT/tI_MC_CLOCK = 640000/2500 = 256
    //RANKx is anded with CFGRx[RANK_ENABLE]. Let's set all ranks here
    //          ENABLE              WAIT                EN_MULTI_RANK_SELECT       RANK
                reg_field(0, 0b1) | reg_field(15, 256) | reg_field(27, 1) |         reg_field(31, 0b1111));

    ddr34lmc_dcr_write_DDR34LMC_INITCMD4(baseAddr,
    //See JEDEC Table 6 Command Truth Table
    //DDR pin            RAS#                   CAS#                    WE#                 A10
    //Value              1                      1                       0                   0
    //INITCMD            CMD[0]                 CMD[1]                  CMD[2]              ADDR[5]
    reg_field(0, 0b1) |    reg_field(1, 0b1) |     reg_field(2, 0b0) | reg_field(21, 0b1));

    ddr34lmc_dcr_write_DDR34LMC_INITSEQ5(baseAddr, 0);

    ddr34lmc_dcr_write_DDR34LMC_INITCMD5(baseAddr, 0);

    //6. Configure MC calibration timeout regs (optional)
    ddr34lmc_dcr_write_DDR34LMC_T_PHYUPD0(baseAddr, 0x00001000);
    ddr34lmc_dcr_write_DDR34LMC_T_PHYUPD1(baseAddr, 0x00001000);
    ddr34lmc_dcr_write_DDR34LMC_T_PHYUPD2(baseAddr, 0x00001000);
    ddr34lmc_dcr_write_DDR34LMC_T_PHYUPD3(baseAddr, 0x00001000);

    //7. Exit power-on mode

    //Must wait for 500us after SDRAM reset deactivated
    if (initMode == DdrInitMode_FollowSpecWaitRequirements)
    {
        usleep(500); //in microseconds
    }
    else if (initMode == DdrInitMode_ViolateSpecWaitRequirements)
    {
    }

    mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            (mcopt2_reg & ~(reg_field(0, 0b1))) //MCOPT2[0] = 0b0 SELF_REF_EN
            | reg_field(2, 0b1)); //MCOPT2[2] = 0b1 INIT_START

    const uint32_t INIT_COMPLETE_TIMEOUT = 1000;
    uint32_t time = 0;
    uint32_t mcstat_reg = 0;

    do
    {
        mcstat_reg = ddr34lmc_dcr_read_DDR34LMC_MCSTAT(baseAddr);
        if (mcstat_reg & reg_field(0, 0b1)) // INIT_COMPLETE
            break;
    }
    while (++time != INIT_COMPLETE_TIMEOUT);

    TEST_ASSERT(time != INIT_COMPLETE_TIMEOUT, "DDR INIT_COMPLETE_TIMEOUT");


    //10. Enable MC
    mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            mcopt2_reg | reg_field(3, 0b1));//MCOPT2[3] = 0b1 MC_ENABLE
}

static void ddr3phy_calibrate(const uint32_t baseAddr)
{
    const uint32_t PHY_CALIBRATION_TIMEOUT = 100; //in us
    int time = 0;
    uint32_t reg;
    
    //8. Configure PHY calibration regs
    //rumboot_printf("	Start dqs-gating\n");
    
    ddr3phy_write_DDR3PHY_PHYREG02(baseAddr,
            0xa1); //Start calibration

    do
    {
        reg = ddr3phy_read_DDR3PHY_PHYREGFF(baseAddr);
        if ((reg & 0x1f) == 0x1f)
            break;
        usleep(1);
    } while(++time != PHY_CALIBRATION_TIMEOUT);

    TEST_ASSERT(time != PHY_CALIBRATION_TIMEOUT, "dqs-gating timeout");
    
    reg = ddr3phy_read_DDR3PHY_PHYREGFF(baseAddr);

    ddr3phy_write_DDR3PHY_PHYREG02(baseAddr,
            0x00); //Stop calibration

}

static void ddr3phy_calibrate_manual(const unsigned int baseAddr)	//#ev
{
    
   unsigned int reg;
   
    //#ev ручная подстройка задержек для скорости 1066
   if (DDR_FREQ == DDR3_1066) {
   ddr3phy_write_DDR3PHY_PHYREG26(baseAddr,0xe);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG36(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG46(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG56(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   
   ddr3phy_write_DDR3PHY_PHYREG27(baseAddr,0x2);  // Write DQS DLL delay (0-7)-младший байт (справа)
   ddr3phy_write_DDR3PHY_PHYREG37(baseAddr,0x1);  // Write DQS DLL delay (0-7)
   ddr3phy_write_DDR3PHY_PHYREG47(baseAddr,0x0);  // Write DQS DLL delay (0-7)
   ddr3phy_write_DDR3PHY_PHYREG57(baseAddr,0x1);  // Write DQS DLL delay (0-7)-старший байт (слева)
      
   }


   //#ev ручная подстройка задержек для скорости 1333
   if (DDR_FREQ == DDR3_1333) {
   
   ddr3phy_write_DDR3PHY_PHYREG26(baseAddr,0xe);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG36(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG46(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG56(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   
   ddr3phy_write_DDR3PHY_PHYREG27(baseAddr,0x2);  // Write DQS DLL delay (0-7)-младший байт (справа)
   ddr3phy_write_DDR3PHY_PHYREG37(baseAddr,0x1);  // Write DQS DLL delay (0-7)
   ddr3phy_write_DDR3PHY_PHYREG47(baseAddr,0x0);  // Write DQS DLL delay (0-7)
   ddr3phy_write_DDR3PHY_PHYREG57(baseAddr,0x1);  // Write DQS DLL delay (0-7)-старший байт (слева)
      
   }
   
   //#ev ручная подстройка задержек для скорости 1600
   if (DDR_FREQ == DDR3_1600) {
   
   ddr3phy_write_DDR3PHY_PHYREG26(baseAddr,0xe);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG36(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG46(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   ddr3phy_write_DDR3PHY_PHYREG56(baseAddr,0xc);	// Write DQ DLL delay (8-f)
   
   ddr3phy_write_DDR3PHY_PHYREG27(baseAddr,0x2);  // Write DQS DLL delay (0-7)-младший байт (справа)
   ddr3phy_write_DDR3PHY_PHYREG37(baseAddr,0x1);  // Write DQS DLL delay (0-7)
   ddr3phy_write_DDR3PHY_PHYREG47(baseAddr,0x0);  // Write DQS DLL delay (0-7)
   ddr3phy_write_DDR3PHY_PHYREG57(baseAddr,0x1);  // Write DQS DLL delay (0-7)-старший байт (слева)
    
   }
   usleep(10);  //  in microseconds #ev 
   
   
   
   // email from INNO, did
  // please configure the register 0x11b[7:4] to 0x07, this will decrease the skew of the clock
  reg = MEM32(baseAddr + 0x11B);
  reg &= ~0xf0;
  MEM32(baseAddr + 0x11B) = reg | 0x70;
}

static void ddr_set_main_config (CrgDdrFreq FreqMode)	//#ev
{   
    if (FreqMode == DDR3_1600)
    {
        ddr_config.T_RDDATA_EN_BC4 = 18;
        ddr_config.T_RDDATA_EN_BL8 = 17;

        ddr_config.CL_PHY =     11;
        ddr_config.CL_MC =      0b111;
        ddr_config.CL_CHIP =    0b111;

        ddr_config.CWL_PHY =    8;
        ddr_config.CWL_MC =     0b010;
        ddr_config.CWL_CHIP =   0b011;

        ddr_config.AL_PHY =     9;
        ddr_config.AL_MC =      0b10;
        ddr_config.AL_CHIP =    0b10;

        ddr_config.WR =         0b110;
        ddr_config.T_REFI =     3120; //            refresh_rate_ps/tckmin_x_ps
        ddr_config.T_RFC_XPR =  108;
        ddr_config.T_RCD =      9;    // 0b1111;    trcd_ps/tckmin_x_ps
        ddr_config.T_RP =       9;    //0b1111;     trp_ps/tckmin_x_ps
        ddr_config.T_RC =       39;   //            trc_ps/tckmin_x_ps
        ddr_config.T_RAS =      27;   //28          tras_ps/tckmin_x_ps
        ddr_config.T_WTR =      6;    // 0b0110;    twtr_ps/tckmin_x_ps
        ddr_config.T_RTP =      6;    //0b0110;     trtp_ps/tckmin_x_ps
        ddr_config.T_XSDLL =    32;
        ddr_config.T_MOD =      12;
    }
    
    if (FreqMode == DDR3_1333)
    {
        ddr_config.T_RDDATA_EN_BC4 = 14;	//?
        ddr_config.T_RDDATA_EN_BL8 = 14;	//?

        ddr_config.CL_PHY =     9;
        ddr_config.CL_MC =      0b101;	//001-5 010-6 011-7 100-8 101-9 110-10 111-11
        ddr_config.CL_CHIP =    0b101;	//001-5 010-6 011-7 100-8 101-9 110-10 111-11

        ddr_config.CWL_PHY =    7;
        ddr_config.CWL_MC =     0b001;
        ddr_config.CWL_CHIP =   0b010;	//000-5 001-6 010-7 011-8

        ddr_config.AL_PHY =     8;
        ddr_config.AL_MC =      0b01;	//CL-1 
        ddr_config.AL_CHIP =    0b01;	//CL-1 

        ddr_config.WR =         0b101; //10
        ddr_config.T_REFI =     7999;	//refresh interval (in cycles core clock)
        ddr_config.T_RFC_XPR =  90;	//tRFC - Refresh to ACT or REF
        ddr_config.T_RCD =      0b1110;	//ACTIVATE to READ/WRITE (in DDR clock cycles)
        ddr_config.T_RP =       0b1110;	//PRECHARGE to ACTIVATE (in DDR clock cycles)
        ddr_config.T_RC =       33;	//ACIVATE to ACIVATE (in DDR clock cycles)
        ddr_config.T_RAS =      24;	 //ACTIVATE to PRECHARGE (in DDR clock cycles)
        ddr_config.T_WTR =      0b0101;	//delay from start of internal write transaction to internal read command(in DDR clock cycles)
        ddr_config.T_RTP =      0b0101;	//internal read command to precharge delay(in DDR clock cycles)
        ddr_config.T_XSDLL =    32;	//exit self refresh and DLL lock delay (in x16 clocks)
        ddr_config.T_MOD =      10;	//MRS command update delay (in DDR clock cycles)
    }

    if (FreqMode == DDR3_1066)
    {
        ddr_config.T_RDDATA_EN_BC4 = 12;
        ddr_config.T_RDDATA_EN_BL8 = 12;

        ddr_config.CL_PHY =     8;
        ddr_config.CL_MC =      0b100;
        ddr_config.CL_CHIP =    0b100;

        ddr_config.CWL_PHY =    6;
        ddr_config.CWL_MC =     0b000;
        ddr_config.CWL_CHIP =   0b001;

        ddr_config.AL_PHY =     7;
        ddr_config.AL_MC =      0b01;
        ddr_config.AL_CHIP =    0b01;

        ddr_config.WR =         0b100; //8
        ddr_config.T_REFI =     5999;
        ddr_config.T_RFC_XPR =  72; //MC_CLOCK = 3750
        ddr_config.T_RCD =      0b1000;
        ddr_config.T_RP =       0b1000;
        ddr_config.T_RC =       28;
        ddr_config.T_RAS =      20;
        ddr_config.T_WTR =      0b0100; //4
        ddr_config.T_RTP =      0b0100;
        ddr_config.T_XSDLL =    32; //dfi_clk_1x = 3750
        ddr_config.T_MOD =      8;
    }

};

//-------------------------------------------------MISCELLANEOUS FUNCTIONS-------------------------------------------------

uint8_t ddr_get_T_SYS_RDLAT(const uint32_t baseAddr)
{
    return (uint8_t)(ddr34lmc_dcr_read_DDR34LMC_DBG0(baseAddr) >> 16) & 0b111111;
}

uint32_t ddr_get_ecc_error_status(const uint32_t baseAddr, const uint32_t portNum)
{
    switch(portNum)
    {
    case 0: return ddr34lmc_dcr_read_DDR34LMC_ECCERR_PORT0(baseAddr);
    case 1: return ddr34lmc_dcr_read_DDR34LMC_ECCERR_PORT1(baseAddr);
    case 2: return ddr34lmc_dcr_read_DDR34LMC_ECCERR_PORT2(baseAddr);
    case 3: return ddr34lmc_dcr_read_DDR34LMC_ECCERR_PORT3(baseAddr);
    default: TEST_ASSERT(0, "Unexpected port number"); return 0;
    }
}

void ddr_clear_ecc_error_status(const uint32_t baseAddr, const uint32_t portNum)
{
    switch(portNum)
    {
    case 0: ddr34lmc_dcr_clear_DDR34LMC_ECCERR_PORT0(baseAddr, 0xffffffff); break;
    case 1: ddr34lmc_dcr_clear_DDR34LMC_ECCERR_PORT1(baseAddr, 0xffffffff); break;
    case 2: ddr34lmc_dcr_clear_DDR34LMC_ECCERR_PORT2(baseAddr, 0xffffffff); break;
    case 3: ddr34lmc_dcr_clear_DDR34LMC_ECCERR_PORT3(baseAddr, 0xffffffff); break;
    default: TEST_ASSERT(0, "Unexpected port number");
    }
}

//-------------------------------------------------SELF REFRESH FUNCTIONS-------------------------------------------------

void ddr_enter_self_refresh_mode(const uint32_t baseAddr)
{
    uint32_t mcstat_reg = 0;

    mcstat_reg = ddr34lmc_dcr_read_DDR34LMC_MCSTAT(baseAddr);
    TEST_ASSERT(!(mcstat_reg & reg_field(1, 0b1)), "DDR34LMC already in self-refresh mode");
    TEST_ASSERT(mcstat_reg & reg_field(2, 0b1), "DDR34LMC must be in IDLE state");

    uint32_t mcopt2_reg = 0;

    mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            mcopt2_reg | reg_field(0, 0b1)); //SELF_REF_EN = 1
    //wait tRP + tRFC = 13750ps + 260000ps
    usleep(1); //let's wait 1us

    mcstat_reg = ddr34lmc_dcr_read_DDR34LMC_MCSTAT(baseAddr);
    TEST_ASSERT(mcstat_reg & reg_field(1, 0b1), "Failed to enter self-refresh mode");

    mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    TEST_ASSERT(!(mcopt2_reg & reg_field(3, 0b1)), "DDR34LMC must be disabled at this point");
}

void ddr_exit_self_refresh_mode(const uint32_t baseAddr)
{
    uint32_t mcstat_reg = 0;

    mcstat_reg = ddr34lmc_dcr_read_DDR34LMC_MCSTAT(baseAddr);
    TEST_ASSERT(mcstat_reg & reg_field(1, 0b1), "DDR34LMC is not in self-refresh mode");

    uint32_t mcopt2_reg = 0;

    mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    mcopt2_reg &= ~reg_field(0, 0b1); //clear SELF_REF_EN
    mcopt2_reg &= ~reg_field(1, 0b1); //clear XSR_PREVENT
    mcopt2_reg |= reg_field(2, 0b1); //set INIT_START
    mcopt2_reg |= reg_field(3, 0b1); //set MC_ENABLE
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr, mcopt2_reg);

    //wait a little tRFC_XPR + tXSDLL = 260000ps + 16*16*2500ps =  260000ps + 640000ps = 900000ps
    usleep(2); //let's wait 2us

    const uint32_t INIT_COMPLETE_TIMEOUT = 1000;
    uint32_t time = 0;

    do
    {
        mcstat_reg = ddr34lmc_dcr_read_DDR34LMC_MCSTAT(baseAddr);
        if (mcstat_reg & reg_field(0, 0b1)) // INIT_COMPLETE
            break;
    }
    while (++time != INIT_COMPLETE_TIMEOUT);

    TEST_ASSERT(time != INIT_COMPLETE_TIMEOUT, "DDR INIT_COMPLETE_TIMEOUT");
    TEST_ASSERT(!(mcstat_reg & reg_field(1, 0b1)), "Failed to exit self-refresh mode");
}

//-------------------------------------------------LOW POWER MODE FUNCTIONS-------------------------------------------------

void ddr_enter_low_power_mode ()
{

    uint32_t reg;
    crg_remove_writelock ();

    ddr_enter_self_refresh_mode(EM0_DDR3LMC_DCR);

   //isolation cells on
    reg = pmu_read_PMU_PWR_EM0 (PMU_BASE);
    reg |= (1<<0);
    pmu_write_PMU_PWR_EM0 (PMU_BASE, reg); //set ISO_ON_EM0 in PWR_EM0
    TEST_ASSERT((pmu_read_PMU_PWR_EM0 (PMU_BASE) & (1<<0)) , "Error writing to PMU_PWR_EM0");
   //

   //stop clocks: I_DDR34LMC_CLKX1, I_DCR_CLOCK, I_PLB6_CLOCK
    reg = crg_ddr_read_CRG_DDR_CKEN_DDR3_REFCLK (CRG_DDR_BASE);
    reg &= ~((1<<0) | (1<<2));
    crg_ddr_write_CRG_DDR_CKEN_DDR3_REFCLK (CRG_DDR_BASE, reg); //clear DDR3_EM0_PHY_CLKEN in CKEN_DDR3;

    reg = crg_cpu_read_CRG_CPU_CKEN_DCR (CRG_CPU_BASE);
    reg &= ~(1<<0);
    crg_cpu_write_CRG_CPU_CKEN_DCR (CRG_CPU_BASE, reg); //clear DDR3_EM0_DCRCLKEN in CKEN_DCR

    reg = crg_cpu_read_CRG_CPU_CKEN_L2C_PLB6 (CRG_CPU_BASE);
    reg &= ~(1<<6);
    crg_cpu_write_CRG_CPU_CKEN_L2C_PLB6 (CRG_CPU_BASE, reg); //clear DDR3_EM0_DCRCLKEN in CKEN_PLB6

    crg_ddr_upd_cken ();
    crg_cpu_upd_cken ();
   //

    reg = pmu_read_PMU_PWR_EM0 (PMU_BASE);
    reg |= (1<<1);
    pmu_write_PMU_PWR_EM0 (PMU_BASE, reg); //set PWR_OFF_EM0 in PWR_EM0
    TEST_ASSERT((pmu_read_PMU_PWR_EM0 (PMU_BASE) & (1<<1)) , "Error writing to PMU_PWR_EM0");

    crg_set_writelock();
}

void ddr_exit_low_power_mode ()
{

    uint32_t reg;
    crg_remove_writelock();

    reg = pmu_read_PMU_PWR_EM0 (PMU_BASE);
    reg &= ~(1<<1);
    pmu_write_PMU_PWR_EM0 (PMU_BASE, reg); //clear PWR_OFF_EM0 in PWR_EM0
    TEST_ASSERT((!(pmu_read_PMU_PWR_EM0 (PMU_BASE) & (1<<1))), "Error writing to PMU_PWR_EM0");

    //*****I_DDR34LMC_RESET, I_DDR34LMC_SELFREF_RESET, I_DCR_RESET, I_PLB6_RESET
     reg = crg_ddr_read_CRG_DDR_RST_EN (CRG_DDR_BASE);
     reg &= ~((1<<2) | (1<<4) | (1<<15) | (1<<16));
     crg_ddr_write_CRG_DDR_RST_EN (CRG_DDR_BASE, reg);

    //******

   //start clocks: I_DDR34LMC_CLKX1, I_DCR_CLOCK, I_PLB6_CLOCK
    reg = crg_ddr_read_CRG_DDR_CKEN_DDR3_REFCLK (CRG_DDR_BASE);
    reg |= (1<<0) | (1<<2);
    crg_ddr_write_CRG_DDR_CKEN_DDR3_REFCLK (CRG_DDR_BASE, reg); //set DDR3_EM0_PHY_CLKEN in CKEN_DDR3

    reg = crg_cpu_read_CRG_CPU_CKEN_DCR (CRG_CPU_BASE);
    reg |= (1<<0);
    crg_cpu_write_CRG_CPU_CKEN_DCR (CRG_CPU_BASE, reg); //set DDR3_EM0_DCRCLKEN in CKEN_DCR

    reg = crg_cpu_read_CRG_CPU_CKEN_L2C_PLB6 (CRG_CPU_BASE);
    reg |= (1<<6);
    crg_cpu_write_CRG_CPU_CKEN_L2C_PLB6 (CRG_CPU_BASE, reg); //set DDR3_EM0_DCRCLKEN in CKEN_PLB6

    crg_ddr_upd_cken ();
    crg_cpu_upd_cken ();
    usleep(4); //workaround

   //
    reg = crg_ddr_read_CRG_DDR_RST_EN (CRG_DDR_BASE);
    reg |= (1<<2) | (1<<4) | (1<<15) | (1<<16);
    crg_ddr_write_CRG_DDR_RST_EN (CRG_DDR_BASE, reg);

   //isolation cells off
    reg = pmu_read_PMU_PWR_EM0 (PMU_BASE);
    reg &= ~(1<<0);
    pmu_write_PMU_PWR_EM0 (PMU_BASE, reg); //clear ISO_ON_EM0 in PWR_EM0
    TEST_ASSERT((!(pmu_read_PMU_PWR_EM0 (PMU_BASE) & (1<<0))) , "Error writing to PMU_PWR_EM0");
   //

    crg_set_writelock();
}

//-------------------------------------------------DRAM CLK FUNCTIONS-------------------------------------------------

void ddr_enable_dram_clk(DdrHlbId hlbId)
{
    uint32_t baseAddr = 0;
    switch (hlbId)
    {
    case DdrHlbId_Em0: baseAddr = EM0_DDR3LMC_DCR; break;
    //case DdrHlbId_Em1: baseAddr = EM1_DDR3LMC_DCR; break;
    default: TEST_ASSERT(0, "Not supported");
    }

    uint32_t mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            mcopt2_reg & ~(reg_field(7, 0b1111))); //MCOPT2[4:7] = 0b0000 CLK_DISABLE
}

void ddr_disable_dram_clk(DdrHlbId hlbId)
{
    uint32_t baseAddr = 0;
    switch (hlbId)
    {
    case DdrHlbId_Em0: baseAddr = EM0_DDR3LMC_DCR; break;
    //case DdrHlbId_Em1: baseAddr = EM1_DDR3LMC_DCR; break;
    default: TEST_ASSERT(0, "Not supported");
    }

    uint32_t mcopt2_reg = ddr34lmc_dcr_read_DDR34LMC_MCOPT2(baseAddr);
    ddr34lmc_dcr_write_DDR34LMC_MCOPT2(baseAddr,
            mcopt2_reg | reg_field(7, 0b1111)); //MCOPT2[4:7] = 0b1111 CLK_DISABLE
}

//-------------------------------------------------CRG FUNCTIONS-------------------------------------------------



void crg_ddr_config_freq (uint32_t phy_base_addr, DdrBurstLength burstLength)
{
    uint32_t reg;
    crg_remove_writelock();
    //crg_ddr_write_CRG_DDR_PLL_PRDIV (CRG_DDR_BASE, 0x01);
    //crg_ddr_write_CRG_DDR_PLL_FBDIV (CRG_DDR_BASE, 0x40);
    //crg_ddr_write_CRG_DDR_PLL_PSDIV (CRG_DDR_BASE, 0x01);
    switch (DDR_FREQ)
    {
      case DDR3_800:
          crg_ddr_write_CRG_DDR_CKDIVMODE_DDR3_4X (CRG_DDR_BASE, 0x00);
          break;
      case DDR3_1066:
          crg_ddr_write_CRG_DDR_CKDIVMODE_DDR3_1X (CRG_DDR_BASE, 0x0B);
          break;
      case DDR3_1333:
          crg_ddr_write_CRG_DDR_CKDIVMODE_DDR3_1X (CRG_DDR_BASE, 0x0B);
          break;
      case DDR3_1600:
          crg_ddr_write_CRG_DDR_CKDIVMODE_DDR3_1X (CRG_DDR_BASE, 0x07);
          break;
    }
    crg_ddr_upd_ckdiv();
    //reg = 0x01;
    //crg_ddr_write_CRG_DDR_PLL_CTRL (CRG_DDR_BASE, reg); //restart PLL

    //uint32_t i = 0, timeout = 1000;
    //reg = crg_ddr_read_CRG_DDR_PLL_CTRL(CRG_DDR_BASE); //waiting restart
    //while ((reg & (1<<0)) && (i < timeout))
    //{
    //    reg = crg_ddr_read_CRG_DDR_PLL_CTRL(CRG_DDR_BASE);
    //    i++;
    //};

    crg_set_writelock();

    //TEST_ASSERT(i < timeout,"RESTART PHY PLL FAILED (TIMEOUT)!\n");

    usleep(1000);

    ddr3phy_init(phy_base_addr, burstLength);

}

void crg_cpu_upd_cken ()
{
    uint32_t reg;
    reg = crg_cpu_read_CRG_CPU_UPD_CK (CRG_CPU_BASE);
    reg |= (1<<4);
    crg_cpu_write_CRG_CPU_UPD_CK (CRG_CPU_BASE, reg);
}

void crg_ddr_upd_cken ()
{
    uint32_t reg;
    reg = crg_ddr_read_CRG_DDR_UPD_CK (CRG_DDR_BASE);
    reg |= (1<<4);
    crg_ddr_write_CRG_DDR_UPD_CK (CRG_DDR_BASE, reg);
}

void crg_cpu_upd_ckdiv ()
{
    uint32_t reg;
    reg = crg_cpu_read_CRG_CPU_UPD_CK (CRG_CPU_BASE);
    reg |= (1<<0);
    crg_cpu_write_CRG_CPU_UPD_CK (CRG_CPU_BASE, reg);
}

void crg_ddr_upd_ckdiv ()
{
    uint32_t reg;
    reg = crg_ddr_read_CRG_DDR_UPD_CK (CRG_DDR_BASE);
    reg |= (1<<0);
    crg_ddr_write_CRG_DDR_UPD_CK (CRG_DDR_BASE, reg);
    
    uint32_t i = 0, timeout = 1000;
    reg = crg_ddr_read_CRG_DDR_UPD_CK(CRG_DDR_BASE); // waiting dividers update
    while ((reg & (1<<0)) && (i < timeout)) {
        reg = crg_ddr_read_CRG_DDR_UPD_CK(CRG_DDR_BASE);
        i++;
    };
}

void crg_remove_writelock ()
{
    //remove write lock
    const bool crg_ddr_write_access = (0 == crg_ddr_read_CRG_DDR_WR_LOCK(CRG_DDR_BASE));
    if (!crg_ddr_write_access)
    {
        //temporarily remove write lock
        crg_ddr_write_CRG_DDR_WR_LOCK(CRG_DDR_BASE, 0x1ACCE551);
    }

    const bool crg_cpu_write_access = (0 == crg_cpu_read_CRG_CPU_WR_LOCK(CRG_CPU_BASE));
    if (!crg_cpu_write_access)
    {
        //temporarily remove write lock
        crg_cpu_write_CRG_CPU_WR_LOCK(CRG_CPU_BASE, 0x1ACCE551);
    }
   //
}

void crg_set_writelock ()
{
    //set write lock back
    const bool crg_cpu_write_access = (0 == crg_ddr_read_CRG_DDR_WR_LOCK(CRG_DDR_BASE));
    if (!crg_cpu_write_access)
    {
        //set write lock back
        crg_cpu_write_CRG_CPU_WR_LOCK(CRG_CPU_BASE, 0); //any value except 0x1ACCE551
    }

    const bool crg_ddr_write_access = (0 == crg_cpu_read_CRG_CPU_WR_LOCK(CRG_CPU_BASE));
    if (!crg_ddr_write_access)
    {
        //set write lock back
        crg_ddr_write_CRG_DDR_WR_LOCK(CRG_DDR_BASE, 0); //any value except 0x1ACCE551
    }
   //
}

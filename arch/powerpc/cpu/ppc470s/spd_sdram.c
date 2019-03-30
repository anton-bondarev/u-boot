/*
 * TX018 SPD related code
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#define SPD_DEBUG

#include <common.h>
#include <asm/processor.h>
#include <asm/io.h>
#include <i2c.h>
#include <ddr_spd.h>
#include <fdtdec.h>
#include <dm/uclass.h>
#include <dm/device.h>
#include <asm/mmu.h>
#include <spd_sdram.h>
#include "ddr_spd.h"
#include "rcmodule_dimm_params.h"


DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPD_EEPROM

#ifdef SPD_DEBUG


void spd_debug(const ddr3_spd_eeprom_t *spd)
{
	unsigned int i;

	/* General Section: Bytes 0-59 */
    printf("\n\nSPD debug output:\n");

#define PRINT_NXS(x, y, z...) printf("%-3d    : %02x " z "\n", x, (u8)y);
#define PRINT_NNXXS(n0, n1, x0, x1, s) \
	printf("%-3d-%3d: %02x %02x " s "\n", n0, n1, x0, x1);

	PRINT_NXS(0, spd->info_size_crc,
		"info_size_crc  bytes written into serial memory, "
		"CRC coverage");
	PRINT_NXS(1, spd->spd_rev,
		"spd_rev        SPD Revision");
	PRINT_NXS(2, spd->mem_type,
		"mem_type       Key Byte / DRAM Device Type");
	PRINT_NXS(3, spd->module_type,
		"module_type    Key Byte / Module Type");
	PRINT_NXS(4, spd->density_banks,
		"density_banks  SDRAM Density and Banks");
	PRINT_NXS(5, spd->addressing,
		"addressing     SDRAM Addressing");
	PRINT_NXS(6, spd->module_vdd,
		"module_vdd     Module Nominal Voltage, VDD");
	PRINT_NXS(7, spd->organization,
		"organization   Module Organization");
	PRINT_NXS(8, spd->bus_width,
		"bus_width      Module Memory Bus Width");
	PRINT_NXS(9, spd->ftb_div,
		"ftb_div        Fine Timebase (FTB) Dividend / Divisor");
	PRINT_NXS(10, spd->mtb_dividend,
		"mtb_dividend   Medium Timebase (MTB) Dividend");
	PRINT_NXS(11, spd->mtb_divisor,
		"mtb_divisor    Medium Timebase (MTB) Divisor");
	PRINT_NXS(12, spd->tck_min,
		  "tck_min        SDRAM Minimum Cycle Time");
	PRINT_NXS(13, spd->res_13,
		"res_13         Reserved");
	PRINT_NXS(14, spd->caslat_lsb,
		"caslat_lsb     CAS Latencies Supported, LSB");
	PRINT_NXS(15, spd->caslat_msb,
		"caslat_msb     CAS Latencies Supported, MSB");
	PRINT_NXS(16, spd->taa_min,
		  "taa_min        Min CAS Latency Time");
	PRINT_NXS(17, spd->twr_min,
		  "twr_min        Min Write REcovery Time");
	PRINT_NXS(18, spd->trcd_min,
		  "trcd_min       Min RAS# to CAS# Delay Time");
	PRINT_NXS(19, spd->trrd_min,
		  "trrd_min       Min Row Active to Row Active Delay Time");
	PRINT_NXS(20, spd->trp_min,
		  "trp_min        Min Row Precharge Delay Time");
	PRINT_NXS(21, spd->tras_trc_ext,
		  "tras_trc_ext   Upper Nibbles for tRAS and tRC");
	PRINT_NXS(22, spd->tras_min_lsb,
		  "tras_min_lsb   Min Active to Precharge Delay Time, LSB");
	PRINT_NXS(23, spd->trc_min_lsb,
		  "trc_min_lsb Min Active to Active/Refresh Delay Time, LSB");
	PRINT_NXS(24, spd->trfc_min_lsb,
		  "trfc_min_lsb   Min Refresh Recovery Delay Time LSB");
	PRINT_NXS(25, spd->trfc_min_msb,
		  "trfc_min_msb   Min Refresh Recovery Delay Time MSB");
	PRINT_NXS(26, spd->twtr_min,
		  "twtr_min Min Internal Write to Read Command Delay Time");
	PRINT_NXS(27, spd->trtp_min,
		  "trtp_min "
		  "Min Internal Read to Precharge Command Delay Time");
	PRINT_NXS(28, spd->tfaw_msb,
		  "tfaw_msb       Upper Nibble for tFAW");
	PRINT_NXS(29, spd->tfaw_min,
		  "tfaw_min       Min Four Activate Window Delay Time");
	PRINT_NXS(30, spd->opt_features,
		"opt_features   SDRAM Optional Features");
	PRINT_NXS(31, spd->therm_ref_opt,
		"therm_ref_opt  SDRAM Thermal and Refresh Opts");
	PRINT_NXS(32, spd->therm_sensor,
		"therm_sensor  SDRAM Thermal Sensor");
	PRINT_NXS(33, spd->device_type,
		"device_type  SDRAM Device Type");
	PRINT_NXS(34, spd->fine_tck_min,
		  "fine_tck_min  Fine offset for tCKmin");
	PRINT_NXS(35, spd->fine_taa_min,
		  "fine_taa_min  Fine offset for tAAmin");
	PRINT_NXS(36, spd->fine_trcd_min,
		  "fine_trcd_min Fine offset for tRCDmin");
	PRINT_NXS(37, spd->fine_trp_min,
		  "fine_trp_min  Fine offset for tRPmin");
	PRINT_NXS(38, spd->fine_trc_min,
		  "fine_trc_min  Fine offset for tRCmin");

	printf("%-3d-%3d: ",  39, 59);  /* Reserved, General Section */

	for (i = 39; i <= 59; i++)
		printf("%02x ", spd->res_39_59[i - 39]);

	puts("\n");

	switch (spd->module_type) {
	case 0x02:  /* UDIMM */
	case 0x03:  /* SO-DIMM */
	case 0x04:  /* Micro-DIMM */
	case 0x06:  /* Mini-UDIMM */
		PRINT_NXS(60, spd->mod_section.unbuffered.mod_height,
			"mod_height    (Unbuffered) Module Nominal Height");
		PRINT_NXS(61, spd->mod_section.unbuffered.mod_thickness,
			"mod_thickness (Unbuffered) Module Maximum Thickness");
		PRINT_NXS(62, spd->mod_section.unbuffered.ref_raw_card,
			"ref_raw_card  (Unbuffered) Reference Raw Card Used");
		PRINT_NXS(63, spd->mod_section.unbuffered.addr_mapping,
			"addr_mapping  (Unbuffered) Address mapping from "
			"Edge Connector to DRAM");
		break;
	case 0x01:  /* RDIMM */
	case 0x05:  /* Mini-RDIMM */
		PRINT_NXS(60, spd->mod_section.registered.mod_height,
			"mod_height    (Registered) Module Nominal Height");
		PRINT_NXS(61, spd->mod_section.registered.mod_thickness,
			"mod_thickness (Registered) Module Maximum Thickness");
		PRINT_NXS(62, spd->mod_section.registered.ref_raw_card,
			"ref_raw_card  (Registered) Reference Raw Card Used");
		PRINT_NXS(63, spd->mod_section.registered.modu_attr,
			"modu_attr     (Registered) DIMM Module Attributes");
		PRINT_NXS(64, spd->mod_section.registered.thermal,
			"thermal       (Registered) Thermal Heat "
			"Spreader Solution");
		PRINT_NXS(65, spd->mod_section.registered.reg_id_lo,
			"reg_id_lo     (Registered) Register Manufacturer ID "
			"Code, LSB");
		PRINT_NXS(66, spd->mod_section.registered.reg_id_hi,
			"reg_id_hi     (Registered) Register Manufacturer ID "
			"Code, MSB");
		PRINT_NXS(67, spd->mod_section.registered.reg_rev,
			"reg_rev       (Registered) Register "
			"Revision Number");
		PRINT_NXS(68, spd->mod_section.registered.reg_type,
			"reg_type      (Registered) Register Type");
		for (i = 69; i <= 76; i++) {
			printf("%-3d    : %02x rcw[%d]\n", i,
				spd->mod_section.registered.rcw[i-69], i-69);
		}
		break;
	default:
		/* Module-specific Section, Unsupported Module Type */
		printf("%-3d-%3d: ", 60, 116);

		for (i = 60; i <= 116; i++)
			printf("%02x", spd->mod_section.uc[i - 60]);

		break;
	}

	/* Unique Module ID: Bytes 117-125 */
	PRINT_NXS(117, spd->mmid_lsb, "Module MfgID Code LSB - JEP-106");
	PRINT_NXS(118, spd->mmid_msb, "Module MfgID Code MSB - JEP-106");
	PRINT_NXS(119, spd->mloc,     "Mfg Location");
	PRINT_NNXXS(120, 121, spd->mdate[0], spd->mdate[1], "Mfg Date");

	printf("%-3d-%3d: ", 122, 125);

	for (i = 122; i <= 125; i++)
		printf("%02x ", spd->sernum[i - 122]);
	printf("   Module Serial Number\n");

	/* CRC: Bytes 126-127 */
	PRINT_NNXXS(126, 127, spd->crc[0], spd->crc[1], "  SPD CRC");

	/* Other Manufacturer Fields and User Space: Bytes 128-255 */
	printf("%-3d-%3d: ", 128, 145);
	for (i = 128; i <= 145; i++)
		printf("%02x ", spd->mpart[i - 128]);
	printf("   Mfg's Module Part Number\n");

	PRINT_NNXXS(146, 147, spd->mrev[0], spd->mrev[1],
		"Module Revision code");

	PRINT_NXS(148, spd->dmid_lsb, "DRAM MfgID Code LSB - JEP-106");
	PRINT_NXS(149, spd->dmid_msb, "DRAM MfgID Code MSB - JEP-106");

	printf("%-3d-%3d: ", 150, 175);
	for (i = 150; i <= 175; i++)
		printf("%02x ", spd->msd[i - 150]);
	printf("   Mfg's Specific Data\n");

	printf("%-3d-%3d: ", 176, 255);
	for (i = 176; i <= 255; i++)
		printf("%02x", spd->cust[i - 176]);
	printf("   Mfg's Specific Data\n");

}

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

#endif /* SPD_DEBUG */

int get_ddr3_spd_bypath(const char * spdpath, ddr3_spd_eeprom_t* spd)
{
    memset(spd, 0, sizeof(ddr3_spd_eeprom_t));
	int node;
    struct udevice *devp = 0; 

	node = fdtdec_get_chosen_node(gd->fdt_blob, spdpath);
	if (node>0) {
		int ret = uclass_get_device_by_of_offset(UCLASS_I2C_EEPROM, node, &devp);
		if (!ret)
		{
			debug("Get SPD device: %s\n", devp->name);
            if(devp)
            {

                if(dm_i2c_read(devp, 0, (uint8_t*)spd, sizeof(ddr3_spd_eeprom_t)))
                {
                    printf("Unable to read SPD\n");
                    return -EIO;
                }
#ifdef SPD_DEBUG
				spd_debug(spd);
#endif				
                return 0;
            }
		}
		else
			debug("Unable to get SPD device %d\n", ret);
	}
    else
		debug("Unable to get spd-path %d\n", node);

    return -ENXIO;
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
unsigned int rcmodule_compute_dimm_parameters(const ddr3_spd_eeprom_t *spd,
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

#ifdef SPD_DEBUG
	dump_dimm_params(pdimm, dimm_number);
#endif

	return 0;
}

#endif /* CONFIG_SPD_EEPROM */
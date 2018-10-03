/*
 * TX018 SPD related code
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define SPD_DEBUG

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

                if(!dm_i2c_read(devp, 0, (uint8_t*)spd, sizeof(ddr3_spd_eeprom_t)))
                    spd_debug(spd);
                else
                {
                    printf("Unable to read SPD\n");
                    return -EIO;
                }

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

#endif /* CONFIG_SPD_EEPROM */
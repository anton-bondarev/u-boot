/*
 * RCM EMI core driver
 *
 * Copyright (C) 2020 MIR
 *	Mikhail.Petrov@mir.dev
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

// #define DEBUG

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <asm/dcr.h>
#include <asm/io.h>
#include <asm/tlb47x.h>
#include <rcm-emi.h>
#include <dt-bindings/memory/rcm-emi.h>

DECLARE_GLOBAL_DATA_PTR;

#define RCM_EMI_SS0 0x00
#define RCM_EMI_SD0 0x04
#define RCM_EMI_SS1 0x08
#define RCM_EMI_SD1 0x0C
#define RCM_EMI_SS2 0x10
#define RCM_EMI_SD2 0x14
#define RCM_EMI_SS3 0x18
#define RCM_EMI_SD3 0x1C
#define RCM_EMI_SS4 0x20
#define RCM_EMI_SD4 0x24
#define RCM_EMI_SS5 0x28
#define RCM_EMI_SD5 0x2C
#define RCM_EMI_RFC 0x30
#define RCM_EMI_HSTSR 0x34
#define RCM_EMI_ECNT20 0x38
#define RCM_EMI_ECNT53 0x3C
#define RCM_EMI_H1ADR 0x40
#define RCM_EMI_H2ADR 0x44
#define RCM_EMI_RREQADR 0x48
#define RCM_EMI_WREQADR 0x4C
#define RCM_EMI_WDADR 0x50
#define RCM_EMI_BUSEN 0x54
#define RCM_EMI_WECR 0x58
#define RCM_EMI_FLCNTRL 0x5C
#define RCM_EMI_IMR 0x60
#define RCM_EMI_IMR_SET 0x64
#define RCM_EMI_IMR_RST 0x68
#define RCM_EMI_IRR 0x70
#define RCM_EMI_IRR_RST 0x74
#define RCM_EMI_ECCWRR 0x78
#define RCM_EMI_ECCRDR 0x7C
#define RCM_EMI_H1CMR 0xC0
#define RCM_EMI_H2CMR 0xC4
#define RCM_EMI_RREQCMR 0xC8
#define RCM_EMI_WREQCMR 0xCC
#define RCM_EMI_WDCMR 0xD0

#define RCM_EMI_DRIVER_NAME "rcm-emi"

struct rcm_emi_platdata_bank
{
	uint32_t ss;
	uint32_t sd;
	bool ecc;
	uint32_t size;
};

struct rcm_emi_platdata
{
	uint32_t dcr_base;
	struct rcm_emi_platdata_bank banks[RCM_EMI_BANK_NUMBER];
	uint32_t emi_frc;
};

static const uint32_t BASE_ADDRESSES[RCM_EMI_BANK_NUMBER] = {
	0x00000000, 0x20000000, 0x40000000, 0x50000000, 0x60000000, 0x70000000
};

static struct rcm_emi_memory_config memory_config;

#ifdef DEBUG
static void dump_platdata(struct rcm_emi_platdata *otp)
{
	int i;

	debug("dcr_base = 0x%08x\n", otp->dcr_base);

	debug("bank     ss         sd     ecc    size\n");
	for (i = 0; i < RCM_EMI_BANK_NUMBER; ++i) {
		struct rcm_emi_platdata_bank *bank = &otp->banks[i];
		debug(" %u   0x%08x 0x%08x  %u  0x%08x\n", i, bank->ss, bank->sd, (int)bank->ecc, bank->size);
	}

	debug("emi_frc = 0x%08x\n", otp->emi_frc);
}
#endif // DEBUG

// init hardware only once (in SPL if SPL enabled)
#if (defined(CONFIG_SPL_RCM_EMI_CORE) && defined(CONFIG_SPL_BUILD)) || (!defined(CONFIG_SPL_RCM_EMI_CORE) && !defined(CONFIG_SPL_BUILD))

static void fill_for_ecc(uint32_t base, uint32_t size)
{
	printf("Filling memory region 0x%08x-0x%08x for ECC", base, base + size);

	base &= ~(0xFFFFF); // align by 1 MB
	size &= ~(0xFFFFF); // align by 1 MB

	while (size) {
		tlb47x_map_nocache(base, 0, TLBSID_1M, TLB_MODE_RW); // map 1 MB
		memset(0, 0, 0x100000); // fill 1 MB
		tlb47x_inval(0, TLBSID_1M); // unmap 1 MB

		base += 0x100000;
		size -= 0x100000;

		printf(".");
	}

	printf("\n");
}

static int init(struct udevice *udev)
{
	struct rcm_emi_platdata *otp = dev_get_platdata(udev);
	uint32_t hstsr = 0;
	int i;

	for (i = 0; i < RCM_EMI_BANK_NUMBER; ++i) {
		dcr_write(otp->dcr_base + RCM_EMI_SS0 + (RCM_EMI_SS1 - RCM_EMI_SS0) * i, otp->banks[i].ss);
		dcr_write(otp->dcr_base + RCM_EMI_SD0 + (RCM_EMI_SD1 - RCM_EMI_SD0) * i, otp->banks[i].sd);
		if (otp->banks[i].ecc)
			hstsr |= (1 << i);
	}

	dcr_write(otp->dcr_base + RCM_EMI_RFC, otp->emi_frc);
	dcr_write(otp->dcr_base + RCM_EMI_FLCNTRL, 0x17);
	dcr_write(otp->dcr_base + RCM_EMI_HSTSR, hstsr);
	dcr_write(otp->dcr_base + RCM_EMI_BUSEN, 0x01);

	mb();

	for (i = 0; i < RCM_EMI_BANK_NUMBER; ++i) {
		if (otp->banks[i].ecc) {
			switch (otp->banks[i].ss & RCM_EMI_SSx_BTYP_MASK) {
			case RCM_EMI_SSx_BTYP_SRAM:
			case RCM_EMI_SSx_BTYP_SSRAM:
			case RCM_EMI_SSx_BTYP_SDRAM:
				fill_for_ecc(BASE_ADDRESSES[i], otp->banks[i].size);
				break;
			}
		}
	}

	return 0;
}

#endif // (defined(CONFIG_SPL_RCM_EMI_CORE) && defined(CONFIG_SPL_BUILD)) || (!defined(CONFIG_SPL_RCM_EMI_CORE) && !defined(CONFIG_SPL_BUILD))

static void rcm_emi_fill_memory_config(struct udevice *udev)
{
	struct rcm_emi_platdata *otp = dev_get_platdata(udev);
	int i;
	int index;

	for (i = 0; i < RCM_EMI_BANK_NUMBER; ++i) {
		struct rcm_emi_memory_range range;
		range.base = BASE_ADDRESSES[i];
		range.size = otp->banks[i].size;

		switch (otp->banks[i].ss & RCM_EMI_SSx_BTYP_MASK)
		{
		case RCM_EMI_SSx_BTYP_SRAM:
		case RCM_EMI_SSx_BTYP_SSRAM:
			index = memory_config.memory_type_config[RCM_EMI_MEMORY_TYPE_ID_SRAM].bank_number++;
			memory_config.memory_type_config[RCM_EMI_MEMORY_TYPE_ID_SRAM].ranges[index] = range;
			break;
		case RCM_EMI_SSx_BTYP_SDRAM:
			index = memory_config.memory_type_config[RCM_EMI_MEMORY_TYPE_ID_SDRAM].bank_number++;
			memory_config.memory_type_config[RCM_EMI_MEMORY_TYPE_ID_SDRAM].ranges[index] = range;
			break;
		}
	}
}

static int rcm_emi_probe(struct udevice *udev)
{
	memset(&memory_config, 0, sizeof memory_config);

// init hardware only once (in SPL if SPL enabled)
#if (defined(CONFIG_SPL_RCM_EMI_CORE) && defined(CONFIG_SPL_BUILD)) || (!defined(CONFIG_SPL_RCM_EMI_CORE) && !defined(CONFIG_SPL_BUILD))
	int ret;
	debug("RCM EMI driver initializing...\n");
	ret = init(udev);
	if (ret != 0) {
		debug("RCM EMI driver initialization error\n");
		return ret;
	}
#endif

	rcm_emi_fill_memory_config(udev);

	debug("RCM EMI driver had been probed successfully\n");

	return 0;
}

static int rcm_emi_ofdata_to_platdata(struct udevice *dev)
{
	struct rcm_emi_platdata *otp = dev_get_platdata(dev);
	char prop_name[128];
	int i;
	int ret;

	ret = dev_read_u32(dev, "dcr-reg", &otp->dcr_base);
	if (ret != 0)
		return ret;

	for (i = 0; i < RCM_EMI_BANK_NUMBER; ++i) {
		sprintf(prop_name, "bank-%u-ss", i);
		ret = dev_read_u32(dev, prop_name, &otp->banks[i].ss);
		if (ret != 0)
			continue;

		sprintf(prop_name, "bank-%u-sd", i);
		ret = dev_read_u32(dev, prop_name, &otp->banks[i].sd);
		if (ret != 0)
			return ret;

		sprintf(prop_name, "bank-%u-ecc", i);
		otp->banks[i].ecc = dev_read_bool(dev, prop_name);

		sprintf(prop_name, "bank-%u-size", i);
		ret = dev_read_u32(dev, prop_name, &otp->banks[i].size);
		if (ret != 0)
			return ret;
	}

	ret = dev_read_u32(dev, "emi-frc", &otp->emi_frc);
	if (ret != 0)
		return ret;

#ifdef DEBUG
	dump_platdata(otp);
#endif

	return 0;
}

static const struct udevice_id rcm_emi_ids[] = {
	{ .compatible = "rcm,emi" },
	{ /* end of list */ }
};

void rcm_emi_init(void)
{
	struct uclass *uc;
	struct udevice *dev, *next;

	if (!uclass_get(UCLASS_MISC, &uc)) {
		uclass_foreach_dev_safe(dev, next, uc) {
			if (strcmp(dev->driver->name, RCM_EMI_DRIVER_NAME) == 0)
				device_probe(dev);
		}
	}
}

const struct rcm_emi_memory_config *rmc_emi_get_memory_config(void)
{
	return &memory_config;
}

U_BOOT_DRIVER(rcm_emi) = {
	.name = RCM_EMI_DRIVER_NAME,
	.id = UCLASS_MISC,
	.of_match = rcm_emi_ids,
	.probe = rcm_emi_probe,
	.flags = DM_FLAG_PRE_RELOC,
	.ofdata_to_platdata = rcm_emi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct rcm_emi_platdata)
};

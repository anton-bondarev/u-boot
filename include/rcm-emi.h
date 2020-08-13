/*
 * RCM EMI device helpers
 *
 * Copyright (C) 2020 MIR
 *	Mikhail.Petrov@mir.dev
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __RCM_EMI_H_INCLUDED_
#define __RCM_EMI_H_INCLUDED_

#define RCM_EMI_BANK_NUMBER 6

enum rcm_emi_memory_type_id
{
	RCM_EMI_MEMORY_TYPE_ID_SRAM = 0, // SRAM, SSRAM etc.
	RCM_EMI_MEMORY_TYPE_ID_SDRAM,
	RCM_EMI_MEMORY_TYPE_ID_NUMBER
};

struct rcm_emi_memory_range
{
	uint32_t base;
	uint32_t size;
};

struct rcm_emi_memory_type_config
{
	unsigned bank_number;
	struct rcm_emi_memory_range ranges[RCM_EMI_BANK_NUMBER];
};

struct rcm_emi_memory_config
{
	struct rcm_emi_memory_type_config memory_type_config[RCM_EMI_MEMORY_TYPE_ID_NUMBER];
};

extern void rcm_emi_init(void);
extern const struct rcm_emi_memory_config *rmc_emi_get_memory_config(void);

#endif // !__RCM_EMI_H_INCLUDED_

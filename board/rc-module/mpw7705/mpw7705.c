#undef DEBUG
#include <common.h>
#include <usb.h>
#include <dm.h>
#include "ddr_spd.h"
#include "rcmodule_dimm_params.h"
#include <fdt_support.h>
#include <environment.h>

DECLARE_GLOBAL_DATA_PTR;

typedef void (*voidcall)(void);

#define SPRN_DBCR0_44X 0x134
#define DBCR0_RST_SYSTEM 0x30000000

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{

	unsigned long tmp;

	asm volatile(
		"mfspr	%0,%1\n"
		"oris	%0,%0,%2@h\n"
		"mtspr	%1,%0"
		: "=&r"(tmp)
		: "i"(SPRN_DBCR0_44X), "i"(DBCR0_RST_SYSTEM));

	return 0;
}

int get_ddr3_spd_bypath(const char *spdpath, ddr3_spd_eeprom_t *spd);
unsigned int rcmodule_compute_dimm_parameters(const ddr3_spd_eeprom_t *spd,
											  dimm_params_t *pdimm,
											  unsigned int dimm_number);

void ddr_init(void);

static phys_size_t ddr_em0_size = 0;
static phys_size_t ddr_em1_size = 0;

#if defined(CONFIG_OF_LIBFDT)

int arch_fixup_fdt(void *blob)
{
	int num_banks = 0;
	u64 base[2];
	u64 size[2];

	if (ddr_em0_size != 0)
	{
		base[num_banks] = 0x0000000000;
		size[num_banks] = ddr_em0_size;
		num_banks++;
	}

	if (ddr_em1_size != 0)
	{
		base[num_banks] = 0x0200000000;
		size[num_banks] = ddr_em1_size;
		num_banks++;
	}

	printf("Setup memory for kernel for %d banks...\n", num_banks);

	return fdt_fixup_memory_banks(blob, base, size, num_banks);
}

#endif

phys_size_t get_physical_mem_size(void)
{
	int dimm0_params_invalid = 1;
	int dimm1_params_invalid = 1;
	dimm_params_t dpar0, dpar1;
	phys_size_t result = 0;

#ifdef CONFIG_SPD_EEPROM
	ddr3_spd_eeprom_t spd;

	if (!get_ddr3_spd_bypath("spd0-path", &spd))
		dimm0_params_invalid = rcmodule_compute_dimm_parameters(&spd, &dpar0, 0);

	if (!get_ddr3_spd_bypath("spd1-path", &spd))
		dimm1_params_invalid = rcmodule_compute_dimm_parameters(&spd, &dpar1, 1);

	if (!dimm0_params_invalid)
	{
		ddr_em0_size = dpar0.capacity;
		result += dpar0.capacity;
	}

	if (!dimm1_params_invalid)
	{
		result += dpar1.capacity;
		ddr_em1_size = dpar1.capacity;
	}

#else
	result = CONFIG_SYS_DDR_SIZE;
#endif
	return result;
}

int dram_init(void)
{
#ifdef CONFIG_SPL_BUILD
	ddr_init();
#else
	gd->ram_size = get_physical_mem_size();
#endif
	return 0;
}

#define SPR_TBL_R 0x10C
#define SPR_TBU_R 0x10D

static uint64_t tcurrent(void)
{
	uint32_t valuel = 0;
	uint32_t valueh = 0;

	__asm volatile(
		"mfspr %0, %1 \n\t"
		: "=r"(valuel)
		: "i"(SPR_TBL_R)
		:);

	__asm volatile(
		"mfspr %0, %1 \n\t"
		: "=r"(valueh)
		: "i"(SPR_TBU_R)
		:);


	return (((uint64_t)valueh) << 32) + valuel;
}

static uint32_t mcurrent(void)
{
	return (tcurrent() / (1000 * TIMER_TICKS_PER_US));
}

int __weak timer_init(void)
{
	return 0;
}
/* Returns time in milliseconds */
ulong get_timer(ulong base)
{
	return mcurrent() - base;
}

#ifdef CONFIG_CMD_USB_MASS_STORAGE

int board_usb_init(int index, enum usb_init_type init)
{
	if (init == USB_INIT_DEVICE)
	{
		struct udevice *dev;
		debug("board_usb_init called\n");
		int ret = uclass_get_device(UCLASS_USB, 0, &dev);
		if (ret)
			debug("unable to find USB device in FDT\n");
	}
	return 0;
}

#endif

#if defined(CONFIG_SYS_DRAM_TEST)

int testdramfromto(uint *pstart, uint *pend)
{
	uint *p;

	printf("Testing DRAM from 0x%08x to 0x%08x\n",
		   CONFIG_SYS_MEMTEST_START,
		   CONFIG_SYS_MEMTEST_END);

	printf("DRAM test phase 1:\n");
	for (p = pstart; p < pend; p++)
		*p = 0xaaaaaaaa;

	for (p = pstart; p < pend; p++)
	{
		if (*p != 0xaaaaaaaa)
		{
			printf("DRAM test fails at: %08x\n", (uint)p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p++)
		*p = 0x55555555;

	for (p = pstart; p < pend; p++)
	{
		if (*p != 0x55555555)
		{
			printf("DRAM test fails at: %08x\n", (uint)p);
			return 1;
		}
	}

	printf("DRAM test passed.\n");
	return 0;
}

int testdram(void)
{
	uint *pstart = (uint *)CONFIG_SYS_MEMTEST_START;
	uint *pend = (uint *)CONFIG_SYS_MEMTEST_END;
	return testdramfromto(pstart, pend);
}

#endif
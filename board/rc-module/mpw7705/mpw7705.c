#include <common.h>
#include <mpw7705_baremetal.h>

DECLARE_GLOBAL_DATA_PTR;


void (*bootrom_enter_host_mode)() = BOOT_ROM_HOST_MODE;

// ASTRO TODO
int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	// for a while - enter host mode
	bootrom_enter_host_mode();
	return 0;
}

void ddr_init (void);

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_DDR_SIZE;
#ifdef CONFIG_SPL_BUILD
	ddr_init();
#endif
	return 0;
}

#define SPR_TBL_R           0x10C

static uint32_t tcurrent(){
  uint32_t value = 0;

  __asm volatile
  (
      "mfspr %0, %1 \n\t"
      :"=r"(value)
        :"i"(SPR_TBL_R)
      :
  );

  return value;
}

static uint32_t ucurrent(){
  return (tcurrent()/TIMER_TICKS_PER_US);
}

static uint32_t mcurrent(){
  return (tcurrent()/(1000*TIMER_TICKS_PER_US));
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

	for (p = pstart; p < pend; p++) {
		if (*p != 0xaaaaaaaa) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test phase 2:\n");
	for (p = pstart; p < pend; p++)
		*p = 0x55555555;

	for (p = pstart; p < pend; p++) {
		if (*p != 0x55555555) {
			printf ("DRAM test fails at: %08x\n", (uint) p);
			return 1;
		}
	}

	printf("DRAM test passed.\n");
	return 0;
}

int
testdram(void)
{
	uint *pstart = (uint *) CONFIG_SYS_MEMTEST_START;
	uint *pend = (uint *) CONFIG_SYS_MEMTEST_END;
	return testdramfromto(pstart, pend);
}

#endif
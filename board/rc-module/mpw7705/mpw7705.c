#define DEBUG
#include <common.h>
#include <mpw7705_baremetal.h>
#include <usb.h>
#include <dm.h>

DECLARE_GLOBAL_DATA_PTR;

typedef void (*voidcall)(void);

static voidcall bootrom_enter_host_mode = (voidcall)BOOT_ROM_HOST_MODE;
static voidcall bootrom_main = (voidcall)BOOT_ROM_MAIN;


#define SPRN_DBCR0_44X 0x134
#define DBCR0_RST_SYSTEM 0x30000000

int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{

	unsigned long tmp;

	asm volatile (
		"mfspr	%0,%1\n"
		"oris	%0,%0,%2@h\n"
		"mtspr	%1,%0"
		: "=&r"(tmp) : "i"(SPRN_DBCR0_44X), "i"(DBCR0_RST_SYSTEM)
		);

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

static uint32_t tcurrent(void)
{
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

static uint32_t ucurrent(void)
{
  return (tcurrent()/TIMER_TICKS_PER_US);
}

static uint32_t mcurrent(void){
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

#ifdef CONFIG_CMD_USB_MASS_STORAGE

int board_usb_init(int index, enum usb_init_type init)
{
	if(init == USB_INIT_DEVICE)
	{
		struct udevice* dev;
		debug("board_usb_init called\n");
		int ret = uclass_get_device(UCLASS_USB, 0, &dev);
		if(ret)
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
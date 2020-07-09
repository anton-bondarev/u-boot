#undef DEBUG
#include <common.h>
#include <usb.h>
#include <dm.h>
#include "ddr_spd.h"
#include "rcm_dimm_params.h"
#include <fdt_support.h>
#include <env.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_OF_LIBFDT)
int arch_fixup_fdt(void *blob)
{
	u64 base[1];
	u64 size[1];

	base[0] = CONFIG_SYS_SDRAM_BASE;
	size[0] = gd->ram_size;

	printf("Setup memory for kernel: base=0x%x, size=0x%x\n", (unsigned)base[0], (unsigned)size[0]);

	return fdt_fixup_memory_banks(blob, base, size, 1);
}
#endif // CONFIG_OF_LIBFDT

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE; // ??? make through DT

	return 0;
}

// clon of weak procedure from file common/memsize.c
phys_size_t get_effective_memsize(void)
{
	return gd->ram_size;
}

// clon of weak procedure from file common/board_f.c
// now is same,but may be rewrite
int dram_init_banksize(void)
{
#if defined(CONFIG_NR_DRAM_BANKS) && defined(CONFIG_SYS_SDRAM_BASE)
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_effective_memsize();
#endif
	return 0;
}

void board_lmb_reserve(struct lmb *lmb)
{
	lmb_reserve(lmb, CONFIG_SYS_TEXT_BASE, CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE - CONFIG_SYS_TEXT_BASE);
}

void bi_dram_print_info( void ) {
#if defined(CONFIG_NR_DRAM_BANKS)
	unsigned int i;
	printf( "Dram regions: \n");
	for( i=0; i<CONFIG_NR_DRAM_BANKS; i++ ) {
		printf( "%u: %016llx-%016llx  (%s)\n", i, gd->bd->bi_dram[i].start, gd->bd->bi_dram[i].size, gd->bd->bi_dram[i].size ? "u" :"f" );
	}
#endif
}

int bi_dram_add_region(uint32_t start, uint32_t size)
{
#if defined(CONFIG_NR_DRAM_BANKS)
	unsigned int i;
	for( i=1; i<CONFIG_NR_DRAM_BANKS; i++ ) {
		if( gd->bd->bi_dram[i].size == 0 ) {
			gd->bd->bi_dram[i].start = start;
			gd->bd->bi_dram[i].size = size;
			break;
		}
	}
#endif
	return 0;
}

int bi_dram_drop_region(uint32_t start, uint32_t size)
{
#if defined(CONFIG_NR_DRAM_BANKS)
	unsigned int i;
	for( i=1; i<CONFIG_NR_DRAM_BANKS; i++ ) {
		if( gd->bd->bi_dram[i].start == start && gd->bd->bi_dram[i].size == size ) {
			gd->bd->bi_dram[i].start = 0;
			gd->bd->bi_dram[i].size = 0;
			break;
		}
	}
#endif
	return 0;
}

int bi_dram_drop_all(void)
{
#if defined(CONFIG_NR_DRAM_BANKS)
	unsigned int i;
	for( i=1; i<CONFIG_NR_DRAM_BANKS; i++ ) {
		gd->bd->bi_dram[i].start = 0;
		gd->bd->bi_dram[i].size = 0;
	}
#endif
	return 0;
}

int add_code_guard(void)
{
	// no code guard because then we need to create 17 TLB (16x1MB + 1x16MB) entries intead of 1
	return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
	return CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE - RCM_PPC_STACK_SIZE;
}

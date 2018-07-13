#define DEBUG 1
#include <common.h>
#include <asm/tlb47x.h>


// ASTRO TODO
int checkcpu(void)
{
    return 0;
}

void print_reginfo(void)
{
    debug("ToDo register print\n");
// ASTRO TODO
}

void tlb47x_inval(uint32_t cpu_adr, tlb_size_id tlb_sid)
{
	tlb_entr_data_0 tlb_0;
	tlb_0.data = 0x00000000;
	tlb_0.entr.epn = (cpu_adr >> 12);
	tlb_0.entr.v = 0;
	tlb_0.entr.ts = 0;
	tlb_0.entr.dsiz = tlb_sid;
		
	_invalidate_tlb_entry(tlb_0.data);
}


static void tlb47x_map_internal(uint64_t physical, uint32_t logical, uint32_t wimg, tlb_size_id size, tlb_rwx_mode smode, tlb_rwx_mode umode)
{
	tlb_entr_data_0 tlb_0;
	tlb_0.data = 0x00000000;
    tlb_0.entr.epn = (logical >> 12); 
    tlb_0.entr.v = 1;
    tlb_0.entr.ts = 0;
    tlb_0.entr.dsiz = size; 

	tlb_entr_data_1 tlb_1;
	tlb_1.data = 0x00000000;
    tlb_1.entr.rpn = (uint32_t)((physical>> 12) & 0xFFFFFFFF);
    tlb_1.entr.erpn = (physical>>32) & 0x3F;

	tlb_entr_data_2 tlb_2;
	tlb_2.data = 0x00000000;
    tlb_2.entr.il1i = 1;
    tlb_2.entr.il1d = 1;
    tlb_2.entr.u = 0;
    tlb_2.entr.wimg = wimg;
    tlb_2.entr.e = 0;  // BE   
    tlb_2.entr.uxwr = umode;     
    tlb_2.entr.sxwr = smode;     
	
	_write_tlb_entry(tlb_0.data, tlb_1.data, tlb_2.data, 0); 
}

void tlb47x_map(uint64_t physical, uint32_t logical, tlb_size_id size, tlb_rwx_mode smode)
{
    tlb47x_map_internal(physical, logical, 0x4, size, smode, TLB_MODE_NONE);
}

void tlb47x_map_nocache(uint64_t physical, uint32_t logical, tlb_size_id size, tlb_rwx_mode smode)
{
    tlb47x_map_internal(physical, logical, 0x2, size, smode, TLB_MODE_NONE);    
}

int add_code_guard(void)
{
    const uint32_t code_start = CONFIG_SYS_TEXT_BASE & ~0xFFFFFF;
    const uint32_t data_start = CONFIG_SYS_INIT_RAM_ADDR & ~0xFFFFFF;
    const uint32_t stack_start = (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE) & ~0xFFFFF;

    // code
    tlb47x_map(code_start - CONFIG_SYS_DDR_BASE, code_start, TLBSID_16M, TLB_MODE_RX);
    // data
    tlb47x_map(data_start - CONFIG_SYS_DDR_BASE, data_start, TLBSID_16M, TLB_MODE_RW);   
    // initial stack
    tlb47x_map(stack_start - CONFIG_SYS_DDR_BASE, stack_start, TLBSID_1M, TLB_MODE_RW);
    // invalidate original page
    tlb47x_inval(CONFIG_SYS_DDR_BASE, TLBSID_256M);

    return 0;
}

ulong board_get_usable_ram_top(ulong total_size)
{
    return CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE;
}
#include <common.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#endif

#include <mpw7705_baremetal.h>

DECLARE_GLOBAL_DATA_PTR;

static const char logo[] =
"_________________________\n"
"< Hello World, from SDIO! >\n"
"-------------------------\n"
"       \\   ^__^\n"
"        \\  (oo)\\_______\n"
"           (__)\\       )\\/\\\n"
"               ||----w |\n"
"               ||     ||\n"
"\n";

#ifdef CONFIG_SPL_BUILD
void board_init_f(ulong dummy)
{
	puts(logo);

	/* Clear global data */
	memset((void *)gd, 0, sizeof(gd_t));

	get_clocks();
	preloader_console_init();
	spl_set_bd();
	dram_init();

	// Clear the BSS
	size_t len = (size_t)&__bss_end - (size_t)&__bss_start;
    memset((void *)&__bss_start, 0x00, len);

	/*copy_uboot_to_ram();

	// Jump to U-Boot image
	uboot = (void *)CONFIG_SYS_TEXT_BASE;
	(*uboot)(); // Never returns Here*/
}
#endif

// ASTRO TODO
int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    return 0;
}

static uint32_t invalidate_tlb_entry (void)
{
    uint32_t res;
    asm volatile
    (
        "addis  %0, r0, 0x0000 \n\t"  // res  = 0x00000000
        "addis  r3, r0, 0x4000 \n\t"  // [r3] = 0x4000_0000, (%2)
        "ori    r3, r3, 0x0070 \n\t"  // [r3] = 0x4000_03F0, (%1) // ? 0870
        "tlbsx. r4, r0, r3     \n\t"  // [r3] - EA, [r4] - Way, Index
        "bne end               \n\t"  // branch if not found
        "oris   r4, r4, 0x8000 \n\t"  // r4[0]=1, r4[4]=0
        "tlbwe  r3, r4, 0      \n\t"  // [r3] - EA[0:19], V[20], [r4]- Way[1:2], Index[8:15], index is NU
        "end:or     %0, r4, r4     \n\t"
    
        "isync \n\t"
        "msync \n\t"
        :"=r"(res) 
        ::
    );
    return res;
};

static void write_tlb_entry_1stGB(void)
{
    asm volatile
    (
        "addis r3, r0, 0x0000 \n\t"  // 0x0000
        "addis r4, r0, 0x4000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0BF0 \n\t"  // 0x0BF0  set 0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F (4K, 16K, 64K, 1M, 16M, 256M, 1GB)
        "tlbwe r4, r3, 0      \n\t"
        
        "addis r4, r0, 0x0000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0000 \n\t"  // 0x0000
        "tlbwe r4, r3, 1      \n\t"
        
        "addis r4, r0, 0x0003 \n\t"  // 0000
        "ori   r4, r4, 0x043F \n\t"  // 023F
        "tlbwe r4, r3, 2      \n\t"
        
        "isync \n\t"
        "msync \n\t"
        :::
    );  
};

/*
static void write_tlb_entry_2ndGB(void)
{
    asm volatile
    (
        "addis r3, r0, 0x0000 \n\t"  // 0x0000
        "addis r4, r0, 0x4000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0BF0 \n\t"  // 0x0BF0  set 0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F (4K, 16K, 64K, 1M, 16M, 256M, 1GB)
        "tlbwe r4, r3, 0      \n\t"
        
        "addis r4, r0, 0x4000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0000 \n\t"  // 0x0000
        "tlbwe r4, r3, 1      \n\t"
        
        "addis r4, r0, 0x0003 \n\t"  // 0000
        "ori   r4, r4, 0x043F \n\t"  // 023F
        "tlbwe r4, r3, 2      \n\t"
        
        "isync \n\t"
        "msync \n\t"
        :::
    );  
};

static void write_tlb_entry_3rdGB(void)
{
    asm volatile
    (
        "addis r3, r0, 0x0000 \n\t"  // 0x0000
        "addis r4, r0, 0x4000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0BF0 \n\t"  // 0x0BF0  set 0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F (4K, 16K, 64K, 1M, 16M, 256M, 1GB)
        "tlbwe r4, r3, 0      \n\t"
        
        "addis r4, r0, 0x8000 \n\t"  // 0x0000
        "ori   r4, r4, 0x0000 \n\t"  // 0x0000
        "tlbwe r4, r3, 1      \n\t"
        
        "addis r4, r0, 0x0003 \n\t"  // 0000
        "ori   r4, r4, 0x043F \n\t"  // 023F
        "tlbwe r4, r3, 2      \n\t"
        
        "isync \n\t"
        "msync \n\t"
        :::
    );  
};

static void mem_check_1stGB(void)
{
    uint32_t EM_ptr_offset;
    
    invalidate_tlb_entry   ();
    write_tlb_entry_1stGB  ();
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        *((uint32_t *) (EM_ptr_offset)) = 0x00000000;
    }
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        *((uint32_t *) (EM_ptr_offset)) = EM_ptr_offset;
    }
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        if (*((uint32_t *) (EM_ptr_offset)) != EM_ptr_offset)
        {
            *((uint32_t *) (0x00070000)) = 0xDEADBEEF;
            *((uint32_t *) (0x00070004)) = EM_ptr_offset;
			puts("mem_check_1stGB failed\n");
            while (1);
        }
    }
    *((uint32_t *) (0x00070000)) = 0x7A557A55;
}

static void mem_check_2ndGB(void)
{
    uint32_t EM_ptr_offset;
    
    invalidate_tlb_entry   ();
    write_tlb_entry_2ndGB  ();
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        *((uint32_t *) (EM_ptr_offset)) = 0x00000000;
    }
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        *((uint32_t *) (EM_ptr_offset)) = EM_ptr_offset + 0x40000000;
    }
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        if (*((uint32_t *) (EM_ptr_offset)) != (EM_ptr_offset + 0x40000000))
        {
            *((uint32_t *) (0x00070010)) = 0xDEADBEEF;
            *((uint32_t *) (0x00070014)) = EM_ptr_offset;
			puts("mem_check_2ndGB failed\n");
            while (1);
        }
    }
    *((uint32_t *) (0x00070010)) = 0x7A557A55;
}

static void mem_check_3rdGB(void)
{
    uint32_t EM_ptr_offset;
    
    invalidate_tlb_entry   ();
    write_tlb_entry_3rdGB  ();
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        *((uint32_t *) (EM_ptr_offset)) = 0x00000000;
    }
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        *((uint32_t *) (EM_ptr_offset)) = EM_ptr_offset + 0x80000000;
    }
    
    for (EM_ptr_offset = 0x40000000; EM_ptr_offset < 0x80000000; EM_ptr_offset += 4)
    {
        if (*((uint32_t *) (EM_ptr_offset)) != (EM_ptr_offset + 0x80000000))
        {
            *((uint32_t *) (0x00070020)) = 0xDEADBEEF;
            *((uint32_t *) (0x00070024)) = EM_ptr_offset;
			puts("mem_check_3rdGB failed\n");
            while (1);
        }
    }
    *((uint32_t *) (0x00070020)) = 0x7A557A55;
}
*/

static void init_ppc(void)
{
    asm volatile
    (
        // initialize MMUCR
        "addis r0, r0, 0x0000   \n\t"  // 0x0000
        "ori   r0, r0, 0x0000   \n\t"  // 0x0000
        "mtspr 946, r0          \n\t"  // SPR_MMUCR, 946
        "mtspr 48,  r0          \n\t"  // SPR_PID,    48
        // Set up TLB search priority
        "addis r0, r0, 0x1234   \n\t"  // 0x1234
        "ori   r0, r0, 0x5678   \n\t"  // 0x5678  
        "mtspr 830, r0          \n\t"  // SSPCR, 830
        "mtspr 831, r0          \n\t"  // USPCR, 831
        "mtspr 829, r0          \n\t"  // ISPCR, 829
        :::
    );   
};

static void TLB_interconnect_init(void)
{
    uint32_t dcr_val = 0;
    
    //***************************************************************
    //  Setting PPC for translation elaboration
    //    Made by assembler insertion. No other way to do it.
    //***************************************************************
    init_ppc();
    //***************************************************************

    // Set Seg0, Seg1, Seg2 and BC_CR0
    dcrwrite32(0x00000010, 0x80000204);  // BC_SGD1
    dcrwrite32(0x00000010, 0x80000205);  // BC_SGD2
    dcr_val = dcrread32(0x80000200);     // BC_CR0
    dcrwrite32( dcr_val | 0x80000000, 0x80000200);
}

void ddr_init (void);

static int a = 0;

static int test(int param)
{
	return param + 1;
}

int dram_init(void)
{
	puts("gonna gd->ram_size\n");
	gd->ram_size = CONFIG_SYS_DDR_SIZE;

	ddr_init();
    TLB_interconnect_init();

	invalidate_tlb_entry();
    write_tlb_entry_1stGB();

	puts("gonna memcpy\n");
	memcpy((void *)0x40000000, (const void*)test, 512);

	int (*inDDR)(int);
	inDDR = (void *)0x40000000;
	puts("gonna inDDR\n");
	//inDDR();

	printf("a = %d\n", inDDR(a));

    /*mem_check_1stGB();
    mem_check_2ndGB();
    mem_check_3rdGB();
	puts("ddr tests done\n");*/

    return 0;
}
#include <asm-offsets.h>
#include <config.h>
#include <mpc83xx.h>
#include <version.h>
#include <ppc_asm.tmpl>
#include <ppc_defs.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <asm/u-boot.h>


.macro  load_const_link rD, constant
    lis     \rD, \constant@h
    ori     \rD, \rD, \constant@l
.endm

.global board_init_f

.section ".text.start","ax",@progbits
.global _start
_start:

// setup stack
#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_SPL_STACK
    load_const_link sp, CONFIG_SPL_STACK
#endif
#else
    load_const_link sp, CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE + 0x400000
#endif

clear_bss:
    /*
    * Now clear BSS segment
    */
    load_const_link  r3,__bss_start
    load_const_link  r4,__bss_end

    cmplw   0, r3, r4
    beq     6f

    li      r0, 0
5:
    stw     r0, 0(r3)
    addi    r3, r3, 4
    cmplw   0, r3, r4
    bne     5b
6:

	bl	cpu_init_early_f

    lis	r3,(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE)@h
	ori	r3,r3,(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE)@l

    bl board_init_f_alloc_reserve
    ori r2, r3, r3
#ifndef CONFIG_SPL_BUILD
    bl board_init_f_init_reserve
#endif 

	bl	cpu_init_f	/* return boot_flag for calling board_init_f */
//#ifdef CONFIG_SPL_BUILD
	bl	board_init_f
//#else
	bl	board_init_r    
//#endif
    isync

# ASTRO TODO
.global relocate_code
relocate_code:
    b relocate_code

endless_loop:
    b endless_loop

.global crash
crash:
    div 0,0,0


    .globl	dcache_enable
dcache_enable:
    blr

    .globl	dcache_disable
dcache_disable:
    blr

    .globl	dcache_status
dcache_status:
    blr

   .globl  trap_init
trap_init:
// all traps  initiated by romboot
    blr

		
.global _invalidate_tlb_entry
_invalidate_tlb_entry:
	tlbsx. r4, r0, r3      // [r3] - EA, [r4] - Way, Index
	bne    ite_end         // branch if not found
	oris   r4, r4, 0x8000  // r4[0]=1, r4[4]=0
	tlbwe  r3, r4, 0       // [r3] - EA[0:19], V[20], [r4]- Way[1:2], Index[8:15], index is NU
ite_end:
	or     r3, r4, r4      // return [r3]
	
	isync
	msync
	blr
	
	
.global _write_tlb_entry
_write_tlb_entry:
	addis r6, r0, 0x0000  // zero [r6]
	tlbwe r3, r6, 0
	tlbwe r4, r6, 1
	tlbwe r5, r6, 2
        
	isync
	msync
	blr
		
		
.global _read_tlb_entry
_read_tlb_entry:
	tlbsx. r5, r0, r3  // [r3] - EA, [r5] - Way, Index
	bne    rte_nf      // branch if not found
	
	tlbre r3, r5, 0
	stw   r3, 0(r4)
	tlbre r3, r5, 1
	stw   r3, 4(r4)
	tlbre r3, r5, 2
	stw   r3, 8(r4)

	add r3, r0, r5
	blr
	
rte_nf:
	addi r3, r0, 0
	blr


#ifdef CONFIG_SPL_BUILD

.section ".text.start_secondary","ax",@progbits
.global _start_secondary
_start_secondary:
    load_const_link sp, MPW_SPL_STACK_SECONDARY
	bl	process_spintable    

#endif


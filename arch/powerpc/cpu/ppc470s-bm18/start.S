#include <asm-offsets.h>
#include <config.h>
#include <mpc83xx.h>
#include <version.h>
#include <ppc_asm.tmpl>
#include <ppc_defs.h>
#include <asm/cache.h>
#include <asm/mmu.h>
#include <asm/u-boot.h>
#include <asm/arch/ppc470s_regs.h>


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
// ??? #ifdef CONFIG_SPL_STACK /// ??? no symbol
// ???     load_const_link sp, RCM_1888TX018_SPL_STACK_SECONDARY
// ??? #endif
// ???     bl _fix_fpu // ???? maybe need
#else
     load_const_link sp, CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE + 0x100000
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
#ifndef CONFIG_SPL_BUILD
    /* now copy data to new place */
    load_const_link r3, CONFIG_SYS_INIT_RAM_ADDR
    load_const_link r4, __init_start
    load_const_link r5, __init_size
    bl memcpy
#endif 

	bl	cpu_init_early_f

    load_const_link r3, (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_RAM_SIZE)

    bl board_init_f_alloc_reserve
    ori r2, r3, r3
    bl board_init_f_init_reserve

	bl	cpu_init_f	/* return boot_flag for calling board_init_f */
	bl	board_init_f

    // ???
// ???     li r9, '&'
// ???     lis r10, 0xC002
// ???     ori r10, r10, 0x9000
// ???     stw r9, 0(r10)
// ??? mmm:
// ???     b mmm
    // ???

	bl	board_init_r    // ??? it is in board_init_f for SPL (but not for Main core)????
// ???
mmm:
	b mmm // ???

# ASTRO TODO
.global relocate_code
relocate_code: // ???? WTF
    b relocate_code

// ???
endless_loop:
    b endless_loop

// ???
.global crash
crash:
    div 0,0,0

// ???
    .globl	dcache_enable
dcache_enable:
    blr

// ???
    .globl	dcache_disable
dcache_disable:
    blr

// ???
    .globl	dcache_status
dcache_status:
    blr

// ???
   .globl  trap_init
trap_init:
// all traps  initiated by romboot
    blr

// for TID=0, TS=0
.global _invalidate_tlb_entry
_invalidate_tlb_entry:
	// set TID=0, TS=0
	li r12, 0
	mtspr SPR_MMUCR, r12

	tlbsx. r4, r0, r3      // [r3] - EA, [r4] - Way, Index
	bne    ite_end         // branch if not found
	oris   r4, r4, 0x8000  // r4[0]=1, r4[4]=0
	tlbwe  r3, r4, 0       // [r3] - EA[0:19], V[20], [r4]- Way[1:2], Index[8:15], index is NU
ite_end:
	or     r3, r4, r4      // return [r3]
	
	isync
	msync
	blr
	
// for TID=0, TS=0
.global _write_tlb_entry
_write_tlb_entry:
	// set TID=0, TS=0
	li r12, 0
	mtspr SPR_MMUCR, r12

	tlbwe r3, r12, 0
	tlbwe r4, r12, 1
	tlbwe r5, r12, 2

	isync
	msync
	blr
		

// ???
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

_fix_fpu:
    mfmsr r0
    ori r0, r0, 0x2000
    mtmsr r0
    isync
    load_const_link  r0,__doublezero
    lfs f0,0(r0)
    lfs f1,0(r0)
    lfs f2,0(r0)
    lfs f3,0(r0)
    lfs f4,0(r0)
    lfs f5,0(r0)
    lfs f6,0(r0)
    lfs f7,0(r0)
    lfs f8,0(r0)
    lfs f9,0(r0)
    lfs f10,0(r0)
    lfs f11,0(r0)
    lfs f12,0(r0)
    lfs f13,0(r0)
    lfs f14,0(r0)
    lfs f15,0(r0)
    lfs f16,0(r0)
    lfs f17,0(r0)
    lfs f18,0(r0)
    lfs f19,0(r0)
    lfs f20,0(r0)
    lfs f21,0(r0)
    lfs f22,0(r0)
    lfs f23,0(r0)
    lfs f24,0(r0)
    lfs f25,0(r0)
    lfs f26,0(r0)
    lfs f27,0(r0)
    lfs f28,0(r0)
    lfs f29,0(r0)
    lfs f30,0(r0)
    lfs f31,0(r0)
 
    load_const_link r3, ~0x2000
    mfmsr r0
    and r0, r0, r3
    mtmsr r0
 
    blr
 
__doublezero:
    .double 0.0

#endif

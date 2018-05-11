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
 
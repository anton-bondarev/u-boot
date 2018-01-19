.macro  load_const_link rD, constant
    lis     \rD, \constant@h
    ori     \rD, \rD, \constant@l
.endm

.global board_init_f

.section ".text.start","ax",@progbits
.global _start
_start:
    b board_init_f

# ASTRO TODO
.global relocate_code
relocate_code:
    b relocate_code

endless_loop:
    b endless_loop

.global crash
crash:
    div 0,0,0

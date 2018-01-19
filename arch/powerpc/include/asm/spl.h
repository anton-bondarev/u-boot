/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2012
 * Texas Instruments, <www.ti.com>
 */
#ifndef	_ASM_SPL_H_
#define	_ASM_SPL_H_

#define BOOT_DEVICE_NOR		1
#define BOOT_DEVICE_MMC1	2
#define BOOT_DEVICE_MMC2    3
#define BOOT_DEVICE_MMC2_2  4

/* Linker symbols */
extern char __bss_start[], __bss_end[];

#endif

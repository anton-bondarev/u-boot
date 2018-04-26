/*
 * PL061 GPIO driver header
 *
 * Copyright (C) 2018 AstroSoft
 *               Alexey Spirkov <alexeis@astrosoft.ru>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PL061_H
#define _PL061_H

#include <common.h>

struct pl061_regs {
	u8 skip[0x3fc];	            
    u8	data; u8  skipx[3];     /* Data registers */
	u8  dir; u8  skip0[3];		/* +0x400 - direction register */	
	u8  is;  u8  skip1[3];		/* +0x404 - interrupt detection register */
	u8  ibe; u8  skip2[3];		/* +0x408 - dual front interrupt register */
	u8  iev; u8  skip3[3];		/* +0x40C - interrupt event register */
	u8  ie;  u8  skip4[3];		/* +0x410 - interrupt mask register */
	const u8  ris; u8  skip5[3];/* +0x414 - interrupt state register w/o mask */
	const u8  mis; u8  skip6[3];/* +0x418 - interrupt state register with mask */
	u8  ic;  u8  skip7[3];		/* +0x41C - interrupt clear register */
	u8  afsel; u8  skip8[3];	/* +0x420 - mode setup register */
	u8 reserved[0xbbc];
	const u8 pid0; u8  skip9[3];/* +0xfe0 - peripheral identifier 0x61 */
	const u8 pid1; u8  skipa[3];/* +0xfe4 - peripheral identifier 0x10 */
	const u8 pid2; u8  skipb[3];/* +0xfe8 - peripheral identifier 0x04 */
	const u8 pid3; u8  skipc[3];/* +0xfec - peripheral identifier 0x00 */	
	const u8 cid0; u8  skipd[3];/* +0xff0 - cell identifier 0x0d */
	const u8 cid1; u8  skipe[3];/* +0xff4 - cell identifier 0xf0 */
	const u8 cid2; u8  skipf[3];/* +0xff8 - cell identifier 0x05 */
	const u8 cid3; u8  skipg[3];/* +0xffc - cell identifier 0xb1 */	
};


#endif

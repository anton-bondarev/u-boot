/*
 * SPDX-License-Identifier: GPL-2.0+
 */

static inline __attribute__((no_instrument_function)) __attribute__((always_inline)) void dcr_write(uint32_t const addr, uint32_t const wval)
{
	asm volatile (
		"mtdcrx %0, %1 \n\t"
		:: "r"(addr), "r"(wval)
	);
}

static inline __attribute__((no_instrument_function)) __attribute__((always_inline)) uint32_t dcr_read(uint32_t const addr) {
	uint32_t rval = 0;
	asm volatile (
		"mfdcrx %0, %1 \n\t"
		: "=r"(rval)
	:   "r"(addr)
	);
	return rval;
}

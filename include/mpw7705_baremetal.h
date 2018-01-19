#ifndef MPW7705_BAREMETAL_H
#define MPW7705_BAREMETAL_H

#include <stdint.h>

volatile uint32_t ioread32(uint32_t const base_addr);
void iowrite32(uint32_t const value, uint32_t const base_addr);
void mpw7705_writeb(uint8_t const value, uint32_t const base_addr);

void dcrwrite32( uint32_t const wval, uint32_t const addr);
volatile uint32_t dcrread32( uint32_t const addr );

#endif /* end of include guard: MPW7705_BAREMETAL_H */

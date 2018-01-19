#include <stdint.h>
#include <mpw7705_baremetal.h>

volatile uint32_t ioread32(uint32_t const base_addr)
{
    return *((volatile uint32_t*)(base_addr));
}

void iowrite32(uint32_t const value, uint32_t const base_addr)
{
    *((volatile uint32_t*)(base_addr)) = value;
}

void mpw7705_writeb(uint8_t const value, uint32_t const base_addr)
{
    *((volatile uint8_t*)(base_addr)) = value;
}

//DCR bus access write, usage: <dcrwrite32(value, base_addr + REG)>
void dcrwrite32(uint32_t const wval, uint32_t const addr)
{
    asm volatile
    (
        "mtdcrx %0, %1 \n\t"
        ::"r"(addr), "r"(wval)
    );
}

//DCR bus access read, usage: <dcrread32(base_addr + REG)>
volatile uint32_t dcrread32(uint32_t const addr)
{
    uint32_t rval=0;
    asm volatile
    (
        "mfdcrx %0, %1 \n\t"
        :"=r"(rval)
        :"r"(addr)
    );
    return rval;
}


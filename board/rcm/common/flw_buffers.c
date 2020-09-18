#include "flw_buffers.h"

volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));

#ifdef CONFIG_TARGET_1888TX018
volatile char blk_buf[512] ;
#endif

#ifdef CONFIG_TARGET_1888BM18
#define IM1_BASE_ADDRESS 0xC0000000
volatile char* blk_buf = (volatile char*)(IM1_BASE_ADDRESS);
#endif

uint32_t flw_virt_to_dma(volatile void* ptr)
{
    return (uint32_t)ptr;
}

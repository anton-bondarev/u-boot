#include "flw_buffers.h"

#ifdef CONFIG_TARGET_1888TX018
volatile char blk_buf[512] ;
volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
#endif

#ifdef CONFIG_TARGET_1888BM18
#define IM1_BASE_ADDRESS 0xC00000000
volatile char* blk_buf = (volatile char*)(IM1_BASE_ADDRESS);
volatile char* edcl_xmodem_buf0 = (volatile char*)(IM1_BASE_ADDRESS + 512);
volatile char* edcl_xmodem_buf1 = (volatile char*)(IM1_BASE_ADDRESS + 512 + EDCL_XMODEM_BUF_LEN);
volatile char* edcl_xmodem_buf_sync = (volatile char*)(IM1_BASE_ADDRESS + 512 + 2*EDCL_XMODEM_BUF_LEN);
#endif

uint32_t flw_virt_to_dma(volatile void* ptr)
{
    return (uint32_t)ptr;
}

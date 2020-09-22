#include "flw_buffers.h"

#ifdef CONFIG_TARGET_1888TX018
volatile char blk_buf[512];
volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
#endif

#ifdef CONFIG_TARGET_1888BM18
#define IM1_BASE_ADDRESS 0xC00000000
volatile char* blk_buf = (volatile char*)(IM1_BASE_ADDRESS);
volatile char* edcl_xmodem_buf0 = (volatile char*)(IM1_BASE_ADDRESS + 512);
volatile char* edcl_xmodem_buf1 = (volatile char*)(IM1_BASE_ADDRESS + 512 + EDCL_XMODEM_BUF_LEN);
#endif

#ifdef CONFIG_TARGET_1888BC048
volatile char blk_buf[512];
volatile char newin[EDCL_XMODEM_BUF_LEN+64];
volatile char newout[EDCL_XMODEM_BUF_LEN+64];
volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
#endif

volatile char* edcl_xmodem_buf_sync __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));

uint32_t flw_virt_to_dma(volatile void* ptr)
{
    return (uint32_t)ptr;
}

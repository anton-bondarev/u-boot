#include "flw_buffers.h"


#ifdef CONFIG_TARGET_1888TX018
#ifdef USE_DSP_MEM
#define IM1_BASE_ADDRESS 0x100000
volatile char* blk_buf = (volatile char*)(IM1_BASE_ADDRESS);
volatile char* edcl_xmodem_buf0 = (volatile char*)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN);
volatile char* edcl_xmodem_buf1 = (volatile char*)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN * 2);
volatile char** p_edcl_xmodem_buf_sync = (volatile char**)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN * 3);
volatile char** p_edcl_xmodem_buf_sync_ack = (volatile char**)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN * 3 + 0x100);
#else
volatile char blk_buf[512];
volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync_ack __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
#endif
#endif // CONFIG_TARGET_1888TX018

#ifdef CONFIG_TARGET_1888BM18
#define IM1_BASE_ADDRESS 0xC0040000
volatile char* blk_buf = (volatile char*)(IM1_BASE_ADDRESS);
volatile char* edcl_xmodem_buf0 = (volatile char*)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN);
volatile char* edcl_xmodem_buf1 = (volatile char*)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN * 2);
volatile char** p_edcl_xmodem_buf_sync = (volatile char**)(IM1_BASE_ADDRESS + (EDCL_XMODEM_BUF_LEN * 3));
volatile char** p_edcl_xmodem_buf_sync_ack = (volatile char**)(IM1_BASE_ADDRESS + (EDCL_XMODEM_BUF_LEN * 3) + 4);
#endif // CONFIG_TARGET_1888BM18

#ifdef CONFIG_TARGET_1888BC048
volatile char blk_buf[512];
volatile char newin[EDCL_XMODEM_BUF_LEN+64];
volatile char newout[EDCL_XMODEM_BUF_LEN+64];
volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync_ack __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
#endif // CONFIG_TARGET_1888BC048

#ifdef CONFIG_TARGET_1879VM8YA
volatile char blk_buf[512];
volatile char newin[EDCL_XMODEM_BUF_LEN+64];
volatile char newout[EDCL_XMODEM_BUF_LEN+64];
volatile char edcl_xmodem_buf0[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char edcl_xmodem_buf1[EDCL_XMODEM_BUF_LEN] __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
volatile char* edcl_xmodem_buf_sync_ack __attribute__ ((aligned (EDCL_XMODEM_BUF_ALIGN)));
#endif // CONFIG_TARGET_1879VM8YA

uint32_t flw_virt_to_dma(volatile void* ptr)
{   
    uint32_t _the_ptr = (uint32_t ) ptr; 
    if (_the_ptr >= 0x100000 && _the_ptr < 0x200000) {
        return (uint32_t) _the_ptr + 0x39000000 - 0x100000; 
    }
    return (uint32_t)_the_ptr;
}

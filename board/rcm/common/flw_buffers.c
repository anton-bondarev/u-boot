#include "flw_buffers.h"

#ifdef CONFIG_TARGET_1888TX018
volatile char* edcl_xmodem_buf0;
volatile char* edcl_xmodem_buf1;
volatile char* edcl_xmodem_buf_sync;
#endif

#ifdef CONFIG_TARGET_1888BM18
#define IM1_BASE_ADDRESS 0xC00000000
char* edcl_xmodem_buf0 = (char*)IM1_BASE_ADDRESS;
char* edcl_xmodem_buf1 = (char*)(IM1_BASE_ADDRESS + EDCL_XMODEM_BUF_LEN);
char* edcl_xmodem_buf_sync = (char*)(IM1_BASE_ADDRESS + 2*EDCL_XMODEM_BUF_LEN);
char* blk_buf = (char*)(IM1_BASE_ADDRESS + 2*EDCL_XMODEM_BUF_LEN+sizeof(char*));
#endif

uint32_t flw_virt_to_dma(volatile void* ptr)
{
    return (uint32_t)ptr;
}

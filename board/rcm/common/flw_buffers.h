#ifndef FLW_BUFFERS_H
#define FLW_BUFFERS_H

#include <linux/types.h>

#define EDCL_XMODEM_BUF_LEN 4096
#define EDCL_XMODEM_BUF_ALIGN 64

extern volatile char blk_buf[];
extern volatile char edcl_xmodem_buf0[];
extern volatile char edcl_xmodem_buf1[];
extern volatile char* edcl_xmodem_buf_sync;

uint32_t flw_virt_to_dma(volatile void* ptr);

#endif // FLW_BUFFERS_H
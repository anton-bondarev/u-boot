#ifndef FLW_BUFFERS_H
#define FLW_BUFFERS_H

#include <linux/types.h>

#define EDCL_XMODEM_BUF_LEN 4096
#define EDCL_XMODEM_BUF_ALIGN 4

extern volatile char* edcl_xmodem_buf0;
extern volatile char* edcl_xmodem_buf1;
extern volatile char* edcl_xmodem_buf_sync;

#ifdef CONFIG_TARGET_1888BM18
extern char* blk_buf;
#endif

uint32_t flw_virt_to_dma(volatile void* ptr);

#endif // FLW_BUFFERS_H
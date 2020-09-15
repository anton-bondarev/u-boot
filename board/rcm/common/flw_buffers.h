#ifndef FLW_BUFFERS_H
#define FLW_BUFFERS_H

#include <linux/types.h>

#define EDCL_XMODEM_BUF_LEN 4096
#define EDCL_XMODEM_BUF_ALIGN 256

#ifdef CONFIG_TARGET_1888TX018
extern char edcl_xmodem_buf0[];
extern char edcl_xmodem_buf1[];
extern char* edcl_xmodem_buf_sync;
#endif

#ifdef CONFIG_TARGET_1888BM18
extern char* edcl_xmodem_buf0;
extern char* edcl_xmodem_buf1;
extern char* edcl_xmodem_buf_sync;
extern char* blk_buf;
#endif

uint32_t flw_virt_to_dma(void* ptr);

#endif // FLW_BUFFERS_H
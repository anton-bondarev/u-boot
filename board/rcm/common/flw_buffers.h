#ifndef FLW_BUFFERS_H
#define FLW_BUFFERS_H

#include <linux/types.h>

//#define CONFIRM_HANDSHAKE

#define EDCL_XMODEM_BUF_LEN 4096
#define EDCL_XMODEM_BUF_ALIGN 64

#ifdef CONFIG_TARGET_1888TX018
extern volatile char blk_buf[];
extern volatile char edcl_xmodem_buf0[];
extern volatile char edcl_xmodem_buf1[];
extern volatile char* edcl_xmodem_buf_sync;
#ifdef CONFIRM_HANDSHAKE
    extern volatile char* edcl_xmodem_sync_conf;
#endif // CONFIRM_HANDSHAKE
#endif // CONFIG_TARGET_1888TX018

#ifdef CONFIG_TARGET_1888BM18
extern volatile char* blk_buf;
extern volatile char* edcl_xmodem_buf0;
extern volatile char* edcl_xmodem_buf1;
extern volatile char** p_edcl_xmodem_buf_sync;
#define edcl_xmodem_buf_sync (*p_edcl_xmodem_buf_sync)
#ifdef CONFIRM_HANDSHAKE
    extern volatile char** p_edcl_xmodem_sync_conf;
    #define edcl_xmodem_sync_conf (*p_edcl_xmodem_sync_conf)
#endif // CONFIRM_HANDSHAKE
#endif // CONFIG_TARGET_1888BM18

#ifdef CONFIG_TARGET_1888BC048
extern volatile char newin[];
extern volatile char newout[];
extern volatile char blk_buf[];
extern volatile char edcl_xmodem_buf0[];
extern volatile char edcl_xmodem_buf1[];
extern volatile char* edcl_xmodem_buf_sync;
#ifdef CONFIRM_HANDSHAKE
    extern volatile char* edcl_xmodem_sync_conf;
#endif // CONFIRM_HANDSHAKE
#endif // CONFIG_TARGET_1888BC048

#ifdef CONFIG_TARGET_1879VM8YA
extern volatile char newin[];
extern volatile char newout[];
extern volatile char blk_buf[];
extern volatile char edcl_xmodem_buf0[];
extern volatile char edcl_xmodem_buf1[];
extern volatile char* edcl_xmodem_buf_sync;
#ifdef CONFIRM_HANDSHAKE
    extern volatile char* edcl_xmodem_sync_conf;
#endif // CONFIRM_HANDSHAKE
#endif // CONFIG_TARGET_1879VM8YA

uint32_t flw_virt_to_dma(volatile void* ptr);

#endif // FLW_BUFFERS_H

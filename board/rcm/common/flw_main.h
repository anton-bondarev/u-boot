#ifndef FLW_MAIN_H
#define FLW_MAIN_H

#include <serial.h>
#include <xyzModem.h>

#include "flw_serial.h"
#include "flw_xmodem.h"
#include "flw_dev_list.h"
#include "flw_sf.h"
#include "flw_mmc.h"
#include "flw_nor.h"
#include "flw_nand.h"

#define FLW_VERSION "1.0.0"

//#define SIMPLE_BUFFER_ONLY

#ifndef CONFIG_SPL_XMODEM_SUPPORT
    #error "CONFIG_SPL_XMODEM_SUPPORT must been on!"
#endif

#define EDCL_XMODEM_BUF_LEN 4096

struct prog_ctx_t
{
    char* src_ptr;
    unsigned long len;
    unsigned long dst_addr;
    struct flw_dev_t* dev;
};

#endif // FLW_MAIN_H
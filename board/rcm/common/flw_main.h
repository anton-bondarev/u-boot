#ifndef FLW_MAIN_H
#define FLW_MAIN_H

#include <serial.h>
#include <xyzModem.h>

#include "flw_dev_list.h"
#include "flw_sf.h"
#include "flw_mmc.h"
#include "flw_nor.h"
#include "flw_nand.h"

#define FLW_VERSION "1.0.0"

#ifndef CONFIG_SPL_XMODEM_SUPPORT
    #error "CONFIG_SPL_XMODEM_SUPPORT must been on!"
#endif

#define EDCL_BUF_LEN 4096

#endif // FLW_MAIN_H
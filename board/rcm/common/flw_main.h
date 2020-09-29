#ifndef FLW_MAIN_H
#define FLW_MAIN_H

#include <serial.h>
#include <xyzModem.h>

#include "flw_buffers.h"
#include "flw_serial.h"
#include "flw_xmodem.h"
#include "flw_dev_list.h"

#ifdef CONFIG_SPL_SPI_FLASH_SUPPORT
    #include "flw_sf.h"
#endif

#ifdef CONFIG_SPL_MMC_SUPPORT
    #include "flw_mmc.h"
#endif

#ifdef CONFIG_SPL_NOR_SUPPORT
    #include "flw_nor.h"
#endif

#ifdef CONFIG_SPL_NAND_SUPPORT
    #include "flw_nand.h"
#endif

#ifdef CONFIG_SPL_I2C_SUPPORT
#include "flw_i2c_eeprom.h"
#endif

#ifndef CONFIG_SPL_CLK
    #error "CONFIG_SPL_CLK must been on!"
#endif

#define FLW_VERSION "1.0.0"

//#define SIMPLE_BUFFER_ONLY

struct prog_ctx_t
{
    char* src_ptr;
    unsigned long len;
    unsigned long dst_addr;
    struct flw_dev_t* dev;
};

#endif // FLW_MAIN_H
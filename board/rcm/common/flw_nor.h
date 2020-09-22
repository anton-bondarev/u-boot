#ifndef FLW_NOR_H
#define FLW_NOR_H

#include <flash.h>
#include <mtd/cfi_flash.h>

#include "flw_dev_list.h"

#ifndef CONFIG_FLASH_CFI_DRIVER
    #error "CONFIG_FLASH_CFI_DRIVER must been on!" 
#endif

#ifdef CONFIG_TARGET_1888TX018
    #define FLW_MAX_NOR_BANK 2
#endif

#ifdef CONFIG_TARGET_1888BM18
    #define FLW_MAX_NOR_BANK 1
#endif

#ifdef CONFIG_TARGET_1888BC048
    #define FLW_MAX_NOR_BANK 0
#endif


int flw_nor_found(unsigned int bank, struct flw_dev_info_t* dev_info, flash_info_t* info);

int flw_nor_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size);

int flw_nor_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data);

int flw_nor_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data);

void flw_nor_list_add(void);

#endif // FLW_NOR_H
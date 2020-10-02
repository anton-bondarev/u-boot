#ifndef FLW_MMC_H
#define FLW_MMC_H

#include <mmc.h>

#include "flw_dev_list.h"

#ifndef CONFIG_SPL_SYS_MALLOC_SIMPLE
    #error "CONFIG_SPL_SYS_MALLOC_SIMPLE must been on!" 
#endif

#ifndef CONFIG_SPL_MMC_WRITE
    #error "CONFIG_SPL_MMC_WRITE must been on!" 
#endif

#ifdef CONFIG_TARGET_1888TX018
    #define FLW_MAX_MMC_SLOT 1
#endif

#ifdef CONFIG_TARGET_1888BM18
    #define FLW_MAX_MMC_SLOT 2
#endif

#ifdef CONFIG_TARGET_1888BC048
    #define FLW_MAX_MMC_SLOT 2
#endif

#ifdef CONFIG_TARGET_1879VM8YA
    #define FLW_MAX_MMC_SLOT 1
#endif

#define FLW_BUS_MODE_CNT 14

int flw_mmc_found(unsigned int slot, struct flw_dev_info_t* dev_info, struct mmc** mmc);

int flw_mmc_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size);

int flw_mmc_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data);

int flw_mmc_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data);

void flw_mmc_list_add(void);

#endif // FLW_MMC_H
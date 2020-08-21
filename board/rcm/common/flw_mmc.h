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

#define FLW_MAX_MMC_SLOT 3

#define FLW_MMC_SECT_SIZE 512

int flw_mmc_found(unsigned int slot, unsigned int info, struct mmc** mmc);

int flw_mmc_erase(struct flw_dev_t* fd, unsigned int addr, unsigned int size);

int flw_mmc_write(struct flw_dev_t* fd, unsigned int addr, unsigned int size, const char* data);

int flw_mmc_read(struct flw_dev_t* fd, unsigned int addr, unsigned int size, char* data);

void flw_mmc_list_add(void);

#endif // FLW_MMC_H
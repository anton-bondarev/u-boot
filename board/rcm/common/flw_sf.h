#ifndef FLW_SF_H
#define FLW_SF_H

#include "flw_dev_list.h"

#ifdef CONFIG_SPL_SPI_FLASH_TINY
    #error "CONFIG_SPL_SPI_FLASH_TINY must been off!"
#endif

#define FLW_MAX_SF_BUS_NUM 1
#define FLW_MAX_SF_CS_NUM 3

int flw_spi_flash_found(unsigned int bus, unsigned int cs);

int flw_spi_flash_erase(struct flw_dev_t* fd, unsigned long addr, unsigned long size);

int flw_spi_flash_write(struct flw_dev_t* fd, unsigned long addr, unsigned long size, const char* data);

int flw_spi_flash_read(struct flw_dev_t* fd, unsigned long addr, unsigned long size, char* data);

void flw_spi_flash_list_add(void);

#endif // FLW_SF_H
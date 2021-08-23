#ifndef FLW_SF_H
#define FLW_SF_H

#include "flw_dev_list.h"

#ifndef CONFIG_SPL_SPI_SUPPORT
    #error "CONFIG_SPL_SPI_SUPPORT must been on!"
#endif

#ifndef CONFIG_SPL_GPIO_SUPPORT
    #error "CONFIG_SPL_GPIO_SUPPORT must been on!"
#endif

#ifdef CONFIG_SPL_SPI_FLASH_TINY
    #error "CONFIG_SPL_SPI_FLASH_TINY must been off!"
#endif

#ifdef CONFIG_TARGET_1888TX018
    #define FLW_MAX_SF_BUS_NUM 1
    #define FLW_MAX_SF_CS_NUM 1
#endif

#ifdef CONFIG_TARGET_1888BM18
    #define FLW_MAX_SF_BUS_NUM 2
    #define FLW_MAX_SF_CS_NUM 2
#endif

#ifdef CONFIG_TARGET_1888BC048
    #define FLW_MAX_SF_BUS_NUM 2
    #define FLW_MAX_SF_CS_NUM 2
#endif

#ifdef CONFIG_TARGET_1879VM8YA
    #define FLW_MAX_SF_BUS_NUM 1
    #define FLW_MAX_SF_CS_NUM 1
#endif

int flw_spi_flash_found(unsigned int bus, unsigned int cs, struct flw_dev_info_t* dev_info);

int flw_spi_flash_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size);

int flw_spi_flash_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data);

int flw_spi_flash_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data);

void flw_spi_flash_list_add(void);

#endif // FLW_SF_H
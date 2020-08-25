#include <common.h>
#include <spi_flash.h>
#include <dm/device-internal.h>

#include "flw_sf.h"

int flw_spi_flash_found(unsigned int bus, unsigned int cs)
{
    struct spi_flash* flash = spi_flash_probe(bus, cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    printf("%s SPI flash at bus %u, cs %u\n", flash ? "Found" : "Failed to initialize", bus, cs);
    if (flash) {
        printf("Name %s, size 0x%x, page size 0x%x, erase size 0x%x\n", flash->name, flash->size, flash->page_size, flash->erase_size);
        spi_flash_free(flash);
        return 0;
    }
    return -1;
}

int flw_spi_flash_erase(struct flw_dev_t* fd, unsigned long addr, unsigned long size)
{
    int ret = -1;
    struct spi_flash* flash = spi_flash_probe(fd->bus, fd->cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    if (flash)
    {
        ret = spi_flash_erase(flash, addr, size);
        spi_flash_free(flash);
    }
    return ret;
}

int flw_spi_flash_write(struct flw_dev_t* fd, unsigned long addr, unsigned long size, const char* data)
{
    int ret = -1;
    struct spi_flash* flash = spi_flash_probe(fd->bus, fd->cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    if (flash)
    {
        ret = spi_flash_write(flash, addr, size, data);
        spi_flash_free(flash);
    }
    return ret;
}

int flw_spi_flash_read(struct flw_dev_t* fd, unsigned long addr, unsigned long size, char* data)
{
    int ret = -1;
    struct spi_flash* flash = spi_flash_probe(fd->bus, fd->cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    if (flash)
    {
        ret = spi_flash_read(flash, addr, size, data);
        spi_flash_free(flash);
    }
    return ret;
}

void flw_spi_flash_list_add(void)
{
    unsigned int bus, cs;
    char* name = "sf**";

    for (bus=0; bus<FLW_MAX_SF_BUS_NUM; bus++)
    {
        for (cs=0; cs<FLW_MAX_SF_CS_NUM; cs++)
        {
            if (!flw_spi_flash_found(bus, cs)) {
                name[2] = bus + '0';
                name[3] = cs +'0';
                flash_dev_list_add(name, FLWDT_SF, bus, cs, flw_spi_flash_erase, flw_spi_flash_write, flw_spi_flash_read);
            }
        }
    }
}

#include <common.h>
#include <spi_flash.h>
#include <dm/device-internal.h>

#include "flw_sf.h"

int flw_spi_flash_found(unsigned int bus, unsigned int cs, struct flw_dev_info_t* dev_info)
{
    struct spi_flash* flash = spi_flash_probe(bus, cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    printf("%s SPI flash at bus %u, cs %u\n", flash ? "Found" : "Failed to initialize", bus, cs);
    if (flash) {
        if (dev_info) {
            strlcpy(dev_info->part, flash->name, FLW_PART_BUF_SIZE );
            dev_info->full_size = flash->size;
            dev_info->erase_size = flash->erase_size;
            dev_info->write_size = flash->page_size;
        }
        spi_flash_free(flash);
        return 0;
    }
    return -1;
}

int flw_spi_flash_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size)
{
    int ret = -1;
    struct spi_flash* flash = spi_flash_probe(fd->dev_info.bus, fd->dev_info.cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    if (flash)
    {
        ret = spi_flash_erase(flash, addr, size);
        spi_flash_free(flash);
    }
    return ret;
}

int flw_spi_flash_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data)
{
    int ret = -1;
    struct spi_flash* flash = spi_flash_probe(fd->dev_info.bus, fd->dev_info.cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
    if (flash)
    {
        ret = spi_flash_write(flash, addr, size, data);
        spi_flash_free(flash);
    }
    return ret;
}

int flw_spi_flash_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data)
{
    int ret = -1;
    struct spi_flash* flash = spi_flash_probe(fd->dev_info.bus, fd->dev_info.cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);
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
    struct flw_dev_info_t dev_info = {0};

    for (bus=0; bus<FLW_MAX_SF_BUS_NUM; bus++)
    {
        for (cs=0; cs<FLW_MAX_SF_CS_NUM; cs++)
        {
            if (!flw_spi_flash_found(bus, cs, &dev_info)) {
                name[2] = bus + '0';
                name[3] = cs +'0';
                dev_info.bus = bus;
                dev_info.cs = cs;
                flash_dev_list_add(name, FLWDT_SF, &dev_info, flw_spi_flash_erase, flw_spi_flash_write, flw_spi_flash_read);
            }
        }
    }
}

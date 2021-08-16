#include <common.h>
#include <dm.h>
#include <dm/uclass.h>
#include <i2c.h>

#include "flw_i2c_eeprom.h"

#if defined CONFIG_TARGET_1888TX018 || defined CONFIG_TARGET_1888BM18
// TODO if need
#endif

#ifdef CONFIG_TARGET_1888BC048
    static const i2c_eeprom_param_t i2c_eeprom_param[FLW_MAX_I2C_BUS][FLW_MAX_I2C_DEV] = {{{"24aa64", 0x50, 2, 8192}}};

struct udevice* flw_i2c_eeprom_found(unsigned int i2c_bus, unsigned int i2c_addr)
{
    int ret;
    struct udevice *bus, *dev;

    ret = uclass_get_device_by_seq(UCLASS_I2C, i2c_bus, &bus);
    if (ret)
        return NULL;
    ret = dm_i2c_probe(bus, i2c_addr, 0, &dev);
    if (ret)
        return NULL;
    return dev;
}

int flw_i2c_eeprom_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size)
{ // for compatible only,separate erase is not necessary
    return 0;
}

static int flw_i2c_eeprom_rd_wr(struct flw_dev_t* fd, bool write, unsigned long long addr, unsigned long long size, unsigned char* data)
{
    int ret;
    unsigned int i, chunk_size = EEPROM_CHUNK_SIZE;
    struct udevice* dev = flw_i2c_eeprom_found(fd->dev_info.i2c_bus, fd->dev_info.i2c_addr);
    struct dm_i2c_chip *chip;

    if (!dev) {
        puts("Device is not found\n");
        return -1;
    }

    if (size % chunk_size) {
        printf("Size(%x) invalid or not multiple of block size(%x)\n", (unsigned int)size, chunk_size);
        return -1;
    }

    chip = dev_get_parent_platdata(dev);
    chip->offset_len = fd->dev_info.i2c_alen;

    for (i=0; i<size; i+=chunk_size) {
        if (write)
            ret = dm_i2c_write(dev, addr, data, chunk_size);
        else
            ret = dm_i2c_read(dev, addr, data, chunk_size);
        if (ret)
            return ret;
        addr += chunk_size;
        data += chunk_size;
    }
    return 0;
}

int flw_i2c_eeprom_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data)
{
    return flw_i2c_eeprom_rd_wr(fd, true, addr, size, (unsigned char*)data);
}

int flw_i2c_eeprom_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data)
{
    return flw_i2c_eeprom_rd_wr(fd, false, addr, size, (unsigned char*)data);
}

#endif

void flw_i2c_eeprom_list_add(void)
{
#ifdef CONFIG_TARGET_1888BC048
unsigned int bus, dev;
    char name[64];

    for (bus=0; bus<FLW_MAX_I2C_BUS; bus++)
    {
        for (dev=0; dev<FLW_MAX_I2C_DEV; dev++)
        {
            struct flw_dev_info_t dev_info = {0};
            unsigned char addr = i2c_eeprom_param[bus][dev].devaddr;
            if (flw_i2c_eeprom_found(bus, addr)) {
                sprintf(name, "i2c%d-%x", bus, addr);
                dev_info.i2c_bus = bus;
                dev_info.i2c_addr = addr;
                dev_info.i2c_alen = i2c_eeprom_param[bus][dev].alen;
                dev_info.full_size = i2c_eeprom_param[bus][dev].size;
                strcpy(dev_info.part, i2c_eeprom_param[bus][dev].part);
                dev_info.erase_size = 1;
                dev_info.write_size = 1;
                flash_dev_list_add(name, FLWDT_I2C, &dev_info, flw_i2c_eeprom_erase, flw_i2c_eeprom_write, flw_i2c_eeprom_read);
            }
        }
    }
#endif
}


#ifndef FLW_I2C_EEPROM_H
#define FLW_I2C_EEPROM_H

#include <i2c.h>

#include "flw_dev_list.h"

#ifndef CONFIG_DM_I2C
    #error "CONFIG_DM_I2C must been on!" 
#endif

#if defined CONFIG_TARGET_1888TX018 || defined CONFIG_TARGET_1888BM18
// TODO if need
#endif

#ifdef CONFIG_TARGET_1888BC048
#ifndef CONFIG_SYS_I2C_RCM_BASIS
    #error "CONFIG_SYS_I2C_RCM_BASIS must been on!" 
#endif
    #define FLW_MAX_I2C_BUS 1
    #define FLW_MAX_I2C_DEV 1
    #define EEPROM_CHUNK_SIZE 32
#endif

typedef struct {
    const char* part;
    unsigned char devaddr;
    unsigned char alen;
    unsigned int size;
} i2c_eeprom_param_t;

struct udevice* flw_i2c_eeprom_found(unsigned int i2c_bus, unsigned int i2c_addr);

int flw_i2c_eeprom_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size);

int flw_i2c_eeprom_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data);

int flw_i2c_eeprom_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data);

void flw_i2c_eeprom_list_add(void);

#endif // FLW_I2C_EEPROM_H
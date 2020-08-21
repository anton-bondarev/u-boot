#ifndef FLW_DEV_LIST_H
#define FLW_DEV_LIST_H

#include <common.h>

#define MAX_DEV_NUM 16

enum FLW_DEV_TYPE {FLWDT_UNKNOWN=0, FLWDT_SF, FLWDT_MMC, FLWDT_NAND, FLWDT_NOR};

struct flw_dev_t;

typedef int (*flw_erase)(struct flw_dev_t* fd, unsigned int addr, unsigned int size);
typedef int (*flw_write)(struct flw_dev_t* fd, unsigned int addr, unsigned int size, const char* data);
typedef int (*flw_read)(struct flw_dev_t* fd, unsigned int addr, unsigned int size, char* data);

struct flw_dev_t
{
    char name[16];
    unsigned int type;  // FLW_DEV_TYPE
    union               // зависит от типа устройства
    {
        struct {
            unsigned int par0;
            unsigned int par1;
        };
        struct
        {
            unsigned int bus;
            unsigned int cs;
        };
        struct
        {
            unsigned int slot;
            unsigned int dummy_mmc;
        };
        struct
        {
            /* data */
        } nand;
        struct
        {
            /* data */
        } nor;
    };
    flw_erase erase;
    flw_write write;
    flw_read read;
};

void flash_dev_list_clear(void);

void flash_dev_list_add(const char* name, unsigned int type, unsigned int par0, unsigned int par1, flw_erase erase, flw_write write, flw_read read);

void flash_dev_list_print(void);

struct flw_dev_t* flash_dev_list_find(const char* name);

#endif // FLW_DEV_LIST_H

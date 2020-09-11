#ifndef FLW_DEV_LIST_H
#define FLW_DEV_LIST_H

#include <common.h>

#define MAX_DEV_NUM 16

enum FLW_DEV_TYPE {FLWDT_UNKNOWN=0, FLWDT_SF, FLWDT_MMC, FLWDT_NAND, FLWDT_NOR};
#define FLW_PART_BUF_SIZE 32

struct flw_dev_t;

typedef int (*flw_erase)(struct flw_dev_t* fd, unsigned long addr, unsigned long size);
typedef int (*flw_write)(struct flw_dev_t* fd, unsigned long addr, unsigned long size, const char* data);
typedef int (*flw_read)(struct flw_dev_t* fd, unsigned long addr, unsigned long size, char* data);

struct flw_dev_info_t
{
    uint64_t full_size;
    uint32_t write_size;
    uint32_t erase_size;
    char part[FLW_PART_BUF_SIZE];
    union               // зависит от типа устройства
    {
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
            unsigned int dummy_nand0;
            unsigned int dummy_nand1;
        };
        struct
        {
            unsigned int bank;
            unsigned int dummy_nor;
        };
    };
};

struct flw_dev_t
{
    char name[16];
    unsigned int type;  // FLW_DEV_TYPE
    struct flw_dev_info_t dev_info;
    flw_erase erase;
    flw_write write;
    flw_read read;
};

void flash_dev_list_clear(void);

void flash_dev_list_add(const char* name, unsigned int type, const struct flw_dev_info_t* dev_info, flw_erase erase, flw_write write, flw_read read);

void flash_dev_list_print(void);

struct flw_dev_t* flash_dev_list_find(const char* name);

#endif // FLW_DEV_LIST_H

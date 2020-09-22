#include <common.h>
#include <flash.h>

#include "flw_nor.h"

extern flash_info_t flash_info[];
ulong monitor_flash_len;

int flw_nor_found(unsigned int bank, struct flw_dev_info_t* dev_info, flash_info_t* info)
{
    if (info->size == 0 || info->sector_count == 0 || info->portwidth == 0 || info->chipwidth == 0)
        return -1; // advanced check?

    dev_info->full_size = info->size;
    dev_info->write_size = 1; // may be xmodem/edcl buffer size is better?
    dev_info->erase_size = flash_sector_size(info, 0);
    strlcpy(dev_info->part, info->name, sizeof(dev_info->part));
    return 0;
}

static int conv_addr_size(unsigned long addr, unsigned long size, unsigned int sect_size, unsigned int* start, unsigned int* end)
{
    if (sect_size == 0) {
        puts("Sector size 0!\n");
        return -1;
    }
    if (addr % sect_size || size % sect_size) {
        puts("Erase address/length not multiple of sector size\n");
        return -1;
    }
    *start = addr / sect_size;
    *end = *start + size / sect_size - 1;
    return 0;
}

int flw_nor_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size)
{
    int ret;
    flash_info_t* info = &flash_info[fd->dev_info.bank];
    unsigned int start, end;
    unsigned long sect_size =  flash_sector_size(info, 0);
    ret = conv_addr_size(addr, size, sect_size, &start, &end);
    if (ret == 0) {
        //printf("erase sectors: %x-%x\n", start, end);
        flash_protect(FLAG_PROTECT_CLEAR, start, end, info);
        ret = flash_erase(info, start, end);
    }
    if (ret)
        flash_perror(ret);
    return ret;
}

int flw_nor_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data)
{
    int ret;
    unsigned long src = flash_info[fd->dev_info.bank].start[0] + (unsigned long)addr;
    flash_set_verbose(0);
    ret = flash_write((char*)data, src, (unsigned long)size);
    //printf("write: %lx-%lx\n", src, (unsigned long)size);
    if (ret)
        flash_perror(ret);
    flash_set_verbose(1);
    return ret;
}

int flw_nor_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data)
{
    unsigned long i;
    unsigned long src = flash_info[fd->dev_info.bank].start[0] + (unsigned long)addr;
    flash_set_verbose(0);
    //printf("read: %lx-%lx\n", src, (unsigned long)size);
    for (i=0; i<(unsigned long)size; i++) *data++ = *(char*)src++;
    flash_set_verbose(1);
    return 0;
}

void flw_nor_list_add(void)
{
    char* name = "nor*";
    unsigned int bank;

    flash_init();
    for (bank=0; bank < FLW_MAX_NOR_BANK; ++bank)
    {
        name[3] = bank + '0';
        flash_info_t* info = &flash_info[bank];
        struct flw_dev_info_t dev_info = {0};
        dev_info.bank = bank;
        dev_info.dummy_nor = UINT_MAX;
        if (!flw_nor_found(bank, &dev_info, info)) {
            flash_dev_list_add(name, FLWDT_NOR, &dev_info, flw_nor_erase, flw_nor_write, flw_nor_read);
        }
     }
}

#include <common.h>
#include <flash.h>

#include "flw_nor.h"

extern flash_info_t flash_info[];
ulong monitor_flash_len;

int flw_nor_found(unsigned int bank, unsigned int show_info, flash_info_t* info)
{
    if (info->size == 0)
        return -1;

    if (show_info)
    {
        printf("Bank #%d: %s flash (%d x %d)", bank, info->name, (info->portwidth << 3), (info->chipwidth << 3));

        if (info->size < 1024 * 1024)
            printf("  Size: %ld kB in %d Sectors\n", info->size >> 10, info->sector_count);
        else
            printf("  Size: %ld MB in %d sectors\n", info->size >> 20, info->sector_count);

        unsigned int sect_size = flash_sector_size(info, 0);

        printf("addresses: %08x..%08x,sector size %x\n",
                (unsigned int)info->start[0], (unsigned int)(info->start[info->sector_count-1]+sect_size-1), sect_size);
    }
    return 0;
}

static int conv_addr_size(unsigned int addr, unsigned int size, unsigned int sect_size, unsigned int* start, unsigned int* end)
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

int flw_nor_erase(struct flw_dev_t* fd, unsigned int addr, unsigned int size)
{
    int ret;
    flash_info_t* info = &flash_info[fd->bank];
    unsigned int start, end, sect_size =  flash_sector_size(info, 0);
    ret = conv_addr_size( addr, size, sect_size, &start, &end);
    if (ret == 0) {
        printf("erase sectors: %x-%x\n", start, end);
        flash_protect(FLAG_PROTECT_CLEAR, start, end, info);
        ret = flash_erase(info, start, end);
    }
    if (ret)
        flash_perror(ret);
    return ret;
}

int flw_nor_write(struct flw_dev_t* fd, unsigned int addr, unsigned int size, const char* data)
{
    int ret;
    unsigned int src = flash_info[fd->bank].start[0] + addr;
    ret = flash_write((char*)data, src, size);
    printf("write: %x-%x\n", src, size);
    if (ret)
        flash_perror(ret);
    return ret;
}

int flw_nor_read(struct flw_dev_t* fd, unsigned int addr, unsigned int size, char* data)
{
    unsigned int i;
    char* src = (char*)(flash_info[fd->bank].start[0] + addr);
    for (i=0; i<size; i++) *data++ = *src++;
    printf("read: %x-%x\n", (unsigned int)src, size);
    return 0;
}

void flw_nor_list_add(void)
{
    char* name = "nor*";
    unsigned int bank;

    flash_init();

    for (bank=0; bank < FLW_MAX_NOR_BANK; ++bank)
    {
        if (!flw_nor_found(bank, 1, &flash_info[bank])) {
            name[3] = bank + '0';
            flash_dev_list_add(name, FLWDT_NOR, bank, UINT_MAX, flw_nor_erase, flw_nor_write, flw_nor_read);
        }
     }
}

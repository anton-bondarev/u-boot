#include <common.h>
#include <mmc.h>

#include "flw_mmc.h"

static const char* bus_mode[FLW_BUS_MODE_CNT] = {
    "MMC_LEGACY",
    "MMC_HS",
    "SD_HS",
    "MMC_HS_52",
    "MMC_DDR_52",
    "UHS_SDR12",
    "UHS_SDR25",
    "UHS_SDR50",
    "UHS_DDR50",
    "UHS_SDR104",
    "MMC_HS_200",
    "MMC_HS_400",
    "MMC_HS_400_ES",
    "MODE_ERROR"
};

int flw_mmc_found(unsigned int slot, struct flw_dev_info_t* dev_info, struct mmc** mmc)
{ // mmcgetcd?
    struct mmc* mmc_tmp = NULL;
    int err;

    err = mmc_init_device(slot);
    if (!err)
        mmc_tmp = find_mmc_device(slot);

    if (mmc_tmp) {
        err = mmc_init(mmc_tmp);
        if (!err) {
            if (dev_info) {
                if (mmc_tmp->selected_mode > FLW_BUS_MODE_CNT-1)
                    mmc_tmp->selected_mode = FLW_BUS_MODE_CNT-1;
                const char* mode = bus_mode[mmc_tmp->selected_mode];
                unsigned int len = strlen(mode);
                char* p = dev_info->part;
                *p++ = (char)(mmc_tmp->cid[0] >> 16);
                *p++ = (char)(mmc_tmp->cid[0] >> 8);
                *p++ = ' ';
                strcpy(p, mode), p += len;
                *p++ = ' ';
                *p++ = (char)mmc_tmp->cid[0];
                *p++ = (char)(mmc_tmp->cid[1] >> 24);
                *p++ = (char)(mmc_tmp->cid[1] >> 16);
                *p++ = (char)(mmc_tmp->cid[1] >> 8);
                *p++ = (char)mmc_tmp->cid[1];
                *p = 0;
                dev_info->full_size =  mmc_tmp->capacity;
                dev_info->erase_size = mmc_tmp->erase_grp_size * 512;
                dev_info->write_size = mmc_tmp->write_bl_len;
            }
            if (mmc)
                *mmc = mmc_tmp;
            return 0;
        }
    }
    return -1;
}

static int conv_addr_size(unsigned long long* addr, unsigned long long* size, unsigned int check)
{
    if (!check) {
        puts("Block/page size is null\n");
        return -1;
    }
    if( *addr % check || *size % check ) {
        puts("Erase/write address/length not multiple of block size\n");
        return -1;
    }
    *addr >>= 9; // /= 512
    *size >>= 9;
    return 0;
}

static int check_protect(struct mmc* mmc)
{
    if (mmc_getwp(mmc) == 1)
    {
        printf("Card is write protected!\n");
        return -1;
    }
    return 0;
}

int flw_mmc_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size)
{
    struct mmc* mmc;
    unsigned int erase_num;

    if (conv_addr_size(&addr, &size, fd->dev_info.erase_size))
        return -1;

    if (flw_mmc_found(fd->dev_info.slot, 0, &mmc))
        return -1;

    if (check_protect(mmc))
        return -1;

    erase_num = blk_derase(mmc_get_blk_desc(mmc), addr, size);
    if (erase_num == size) {
        //printf("erase: %x-%x\n", addr, size);
        return 0;
    }
    return -1;
}

int flw_mmc_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data)
{
    struct mmc* mmc;
    unsigned int write_num;

    if (conv_addr_size(&addr, &size, fd->dev_info.write_size))
        return -1;

    if (flw_mmc_found(fd->dev_info.slot, 0, &mmc))
        return -1;

    if (check_protect(mmc))
        return -1;

    write_num = blk_dwrite(mmc_get_blk_desc(mmc), addr, size, data);
    if (write_num == size) {
        //printf("write: %x-%x\n", addr, size);
        return 0;
    }
    return -1;
}

int flw_mmc_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data)
{
    struct mmc* mmc;
    unsigned int read_num;

    if (conv_addr_size(&addr, &size, fd->dev_info.write_size))
        return -1;

    if (flw_mmc_found(fd->dev_info.slot, NULL, &mmc))
        return -1;

    read_num = blk_dread(mmc_get_blk_desc(mmc), addr, size, data);
    if (read_num == size) {
        //("read: %x-%x\n", addr, size);
        return 0;
    }
    return -1;;
}

void flw_mmc_list_add(void)
{
    unsigned int slot;
    char* name = "mmc*";

    for (slot=0; slot<FLW_MAX_MMC_SLOT; slot++)
    {
        struct flw_dev_info_t dev_info = {0};
        if (!flw_mmc_found(slot, &dev_info, NULL)) {
            name[3] = slot +'0';
            dev_info.slot = slot;
            dev_info.dummy_mmc = UINT_MAX;
            flash_dev_list_add(name, FLWDT_MMC, &dev_info, flw_mmc_erase, flw_mmc_write, flw_mmc_read);
        }
    }
}

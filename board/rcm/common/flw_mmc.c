#include <common.h>
#include <mmc.h>

#include "flw_mmc.h"

int flw_mmc_found(unsigned int slot, unsigned int info, struct mmc** mmc)
{ // mmcgetcd?
    struct mmc* mmc_tmp = NULL;
    int err;

    err = mmc_init_device(slot);
    if (!err)
        mmc_tmp = find_mmc_device(slot);

    if (info)
        printf("%s MMC at slot %u\n", mmc_tmp ? "Found" : "Not found", slot);

    if (mmc_tmp) {
        err = mmc_init(mmc_tmp);
        if (!err) {
            if (info) {
                printf("Name %s,block length read 0x%x,block length write 0x%x,erase size(x512 byte) 0x%x\n",
                    mmc_tmp->cfg->name, mmc_tmp->read_bl_len, mmc_tmp->write_bl_len,  mmc_tmp->erase_grp_size );
            }
            if (mmc)
                *mmc = mmc_tmp;
            return 0;
        }
        else if (info)
            printf("Failed to initialize\n");
    }
    return -1;
}

static int conv_addr_size(unsigned int* addr, unsigned int* size)
{
    if( *addr % FLW_MMC_SECT_SIZE || *size % FLW_MMC_SECT_SIZE )
    {
        puts("Erase/write address/length not multiple of block size\n");
        return -1;
    }
    *addr >>= 9; // /= 512
    *size >>= 9; // /= 512
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

int flw_mmc_erase(struct flw_dev_t* fd, unsigned int addr, unsigned int size)
{
    struct mmc* mmc;
    unsigned int erase_num;

    if (conv_addr_size(&addr, &size))
        return -1;

    if (flw_mmc_found(fd->slot, 0, &mmc))
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

int flw_mmc_write(struct flw_dev_t* fd, unsigned int addr, unsigned int size, const char* data)
{
    struct mmc* mmc;
    unsigned int write_num;

    if (conv_addr_size(&addr, &size))
        return -1;

    if (flw_mmc_found(fd->slot, 0, &mmc))
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

int flw_mmc_read(struct flw_dev_t* fd, unsigned int addr, unsigned int size, char* data)
{
    struct mmc* mmc;
    unsigned int read_num;

    if (conv_addr_size(&addr, &size))
        return -1;

    if (flw_mmc_found(fd->slot, 0, &mmc))
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
        if (!flw_mmc_found(slot, 1, NULL)) {
            name[3] = slot +'0';
            flash_dev_list_add(name, FLWDT_MMC, slot, UINT_MAX, flw_mmc_erase, flw_mmc_write, flw_mmc_read);
        }
    }
}

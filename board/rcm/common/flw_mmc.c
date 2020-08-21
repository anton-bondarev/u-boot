#include <common.h>
#include <mmc.h>

#include "flw_mmc.h"

int flw_mmc_found(unsigned int slot, struct mmc** mmc)
{
    struct mmc* mmc_tmp = NULL;
    int err;

    err = mmc_init_device(slot);
    if (!err)
        mmc_tmp = find_mmc_device(slot);

    printf("%s MMC at slot %u\n", mmc_tmp ? "Found" : "Not found", slot);
    if (mmc_tmp) {
        err = mmc_init(mmc_tmp);
        if (!err) {
            printf("Name %s,block length read 0x%x,block length write 0x%x,erase size(x512 byte) 0x%x\n",
                mmc_tmp->cfg->name, mmc_tmp->read_bl_len, mmc_tmp->write_bl_len,  mmc_tmp->erase_grp_size );
            if (mmc)
                *mmc = mmc_tmp;
            return 0;
        }
        else
            printf("Failed to initialize\n");
    }
    return -1;
}

int flw_mmc_erase(struct flw_dev_t* fd, unsigned int addr, unsigned int size)
{
    return 0;
}

int flw_mmc_write(struct flw_dev_t* fd, unsigned int addr, unsigned int size, const char* data)
{
    return 0;
}

int flw_mmc_read(struct flw_dev_t* fd, unsigned int addr, unsigned int size, char* data)
{
    return 0;
}

void flw_mmc_list_add(void)
{
    unsigned int slot;
    char* name = "mmc*";

    for (slot=0; slot<FLW_MAX_MMC_SLOT; slot++)
    {
        if (!flw_mmc_found(slot, NULL)) {
            name[3] = slot +'0';
            flash_dev_list_add(name, FLWDT_MMC, flw_mmc_erase, flw_mmc_write, flw_mmc_read);
        }
    }
}

#include <common.h>
#include <nand.h>

#include "flw_nand.h"
#include "flw_buffers.h"

int rcm_nand_flw_init(char* dma_buf, char* part, unsigned int partlen, uint64_t* full_size, uint32_t* write_size, uint32_t* erase_size);
int rcm_nand_flw_erase_block(unsigned long addr);
int rcm_nand_flw_write_page(unsigned long addr, char* data);
int rcm_nand_flw_read_page(unsigned long addr, char* data);

int flw_nand_found(struct flw_dev_info_t* dev_info)
{
    int ret;
    ret = rcm_nand_flw_init(edcl_xmodem_buf0, dev_info->part, sizeof(dev_info->part), &dev_info->full_size, &dev_info->write_size, &dev_info->erase_size);
    return ret;
}

static int check_addr_size( unsigned long long* addr, unsigned long long* size, unsigned long check )
{
    if (check == 0) {
        puts("Page/block size is null\n");
        return -1;
    }
    if (*addr % check || *size % check) {
        puts("Address/length not multiple of page/block size\n");
        return -1;
    }
    return 0;
}

int flw_nand_erase(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size)
{
    int err;
    unsigned long long curr, end = addr+size-1;
    unsigned int flw_nand_erase_size = fd->dev_info.erase_size;

    err = check_addr_size(&addr, &size, flw_nand_erase_size);
    if (!err) {
        for (curr = addr; curr <= end; curr += flw_nand_erase_size)
        {
            err = rcm_nand_flw_erase_block(curr);
            if (err) break;
        }
    }
    return err;
}

static int flw_nand_rdwr(struct flw_dev_t* fd, int wr, unsigned long long addr, unsigned long long size, char* data)
{
    unsigned long long curr, end = addr+size-1;
    unsigned int flw_nand_write_size = fd->dev_info.write_size;
    int err = check_addr_size(&addr, &size, flw_nand_write_size);
    if (!err) {
        for (curr = addr; curr <= end; curr += flw_nand_write_size) {
            if (wr )
                err = rcm_nand_flw_write_page(curr, data);
            else
                err = rcm_nand_flw_read_page(curr, data);
            data += flw_nand_write_size;
            if (err)
                break;
        }
    }
    return err;
}

int flw_nand_write(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, const char* data)
{
    return flw_nand_rdwr(fd, 1, addr, size, (char*)data);
}

int flw_nand_read(struct flw_dev_t* fd, unsigned long long addr, unsigned long long size, char* data)
{
    return flw_nand_rdwr(fd, 0, addr, size, data);
}

void flw_nand_list_add(void)
{
    struct flw_dev_info_t dev_info = {0};
    dev_info.dummy_nand0 = dev_info.dummy_nand1 = UINT_MAX;
    if (!flw_nand_found(&dev_info)) {
        flash_dev_list_add("nand0", FLWDT_NAND, &dev_info, flw_nand_erase, flw_nand_write, flw_nand_read);
    }
}

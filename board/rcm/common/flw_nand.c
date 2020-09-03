#include <common.h>
#include <nand.h>

#include "flw_nand.h"

int rcm_nand_flw_init(char* dma_buf);
int rcm_nand_flw_erase_block(unsigned long addr);
int rcm_nand_flw_write_page(unsigned long addr, char* data);
int rcm_nand_flw_read_page(unsigned long addr, char* data);

int flw_nand_found(void)
{
    extern char edcl_xmodem_buf0[];
    return rcm_nand_flw_init(edcl_xmodem_buf0);
}

static int check_addr_size( unsigned long* addr, unsigned long* size, unsigned long check )
{
    if (*addr % check || *size % check)
    {
        puts("Address/length not multiple of page/block size\n");
        return -1;
    }
    return 0;
}

int flw_nand_erase(struct flw_dev_t* fd, unsigned long addr, unsigned long size)
{
    int err;
    unsigned long curr, end = addr+size-1;

    err = check_addr_size(&addr, &size, FLW_NAND_ERASE_SIZE);
    if (!err) {
        for (curr = addr; curr <= end; curr += FLW_NAND_ERASE_SIZE)
        {
            err = rcm_nand_flw_erase_block(curr);
            if (err) break;
        }
    }
    return err;
}

static int flw_nand_rdwr(struct flw_dev_t* fd, int wr, unsigned long addr, unsigned long size, char* data)
{
    unsigned long curr, end = addr+size-1;
    int err = check_addr_size(&addr, &size, FLW_NAND_WRITE_SIZE);
    if (!err) {
        for (curr = addr; curr <= end; curr += FLW_NAND_WRITE_SIZE) {
            if (wr )
                err = rcm_nand_flw_write_page(curr, data);
            else
                err = rcm_nand_flw_read_page(curr, data);
            data += FLW_NAND_WRITE_SIZE;
            if (err)
                break;
        }
    }
    return err;
}

int flw_nand_write(struct flw_dev_t* fd, unsigned long addr, unsigned long size, const char* data)
{
    return flw_nand_rdwr(fd, 1, addr, size, (char*)data);
}

int flw_nand_read(struct flw_dev_t* fd, unsigned long addr, unsigned long size, char* data)
{
    return flw_nand_rdwr(fd, 0, addr, size, data);
}

void flw_nand_list_add(void)
{
    if (!flw_nand_found()) {
        flash_dev_list_add("nand0", FLWDT_NAND, UINT_MAX, UINT_MAX, flw_nand_erase, flw_nand_write, flw_nand_read);
    }
}

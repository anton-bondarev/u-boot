#ifndef FLW_NAND_H
#define FLW_NAND_H

#include <mtd.h>
#include <nand.h>

#include "flw_dev_list.h"

int flw_nand_found(struct flw_dev_info_t* dev_info);

int flw_nand_erase(struct flw_dev_t* fd, unsigned long addr, unsigned long size);

int flw_nand_write(struct flw_dev_t* fd, unsigned long addr, unsigned long size, const char* data);

int flw_nand_read(struct flw_dev_t* fd, unsigned long addr, unsigned long size, char* data);

void flw_nand_list_add(void);

#endif // FLW_NAND_H
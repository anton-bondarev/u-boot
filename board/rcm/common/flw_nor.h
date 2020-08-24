#ifndef FLW_NOR_H
#define FLW_NOR_H

#include <flash.h>
#include <mtd/cfi_flash.h>

#include "flw_dev_list.h"

#define FLW_MAX_NOR_BANK 3

int flw_nor_found(unsigned int bank, unsigned int show_info, flash_info_t* info);

int flw_nor_erase(struct flw_dev_t* fd, unsigned int addr, unsigned int size);

int flw_nor_write(struct flw_dev_t* fd, unsigned int addr, unsigned int size, const char* data);

int flw_nor_read(struct flw_dev_t* fd, unsigned int addr, unsigned int size, char* data);

void flw_nor_list_add(void);

#endif // FLW_NOR_H
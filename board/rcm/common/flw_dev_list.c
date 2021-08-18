#include "flw_dev_list.h"

static struct flash_dev_list
{
    unsigned int num;
    struct flw_dev_t dev[MAX_DEV_NUM];
} fdl;

void flash_dev_list_clear(void)
{
    memset(&fdl, 0, sizeof(fdl));
}

void flash_dev_list_add(const char* name, unsigned int type, const struct flw_dev_info_t* dev_info, flw_erase erase, flw_write write, flw_read read)
{
    printf("Registering device: %s\n", name);
    if (fdl.num < MAX_DEV_NUM)
    {
        strcpy(fdl.dev[fdl.num].name, name);
        fdl.dev[fdl.num].type = type;
        fdl.dev[fdl.num].dev_info = *dev_info;
        fdl.dev[fdl.num].erase = erase;
        fdl.dev[fdl.num].write = write;
        fdl.dev[fdl.num].read = read;
        fdl.num++;
    }
}

void flash_dev_list_print(void)
{
//#define DL_VERB_PRINT
    unsigned int i;
    puts("Flash devices list:\n");
    for (i=0; i<fdl.num; i++) {
    printf("id: %s part: %s size: %x%08x erasesize: %08x writesize: %08x\n",
        fdl.dev[i].name,
        fdl.dev[i].dev_info.part,
        (unsigned int)(fdl.dev[i].dev_info.full_size >> 32), (unsigned int)fdl.dev[i].dev_info.full_size,
        fdl.dev[i].dev_info.erase_size, fdl.dev[i].dev_info.write_size);
#ifdef DL_VERB_PRINT
        printf(",%u,%x,%x,%x,%x,%x",
                fdl.dev[i].type, fdl.dev[i].par0, fdl.dev[i].par1,
                (uint32_t)fdl.dev[i].erase, (uint32_t)fdl.dev[i].write, (uint32_t)fdl.dev[i].read);
#endif
    }
    puts("completed\n");
}

struct flw_dev_t* flash_dev_list_find(const char* name)
{
    unsigned int i;
    for (i=0; i<fdl.num; i++)
        if (!strcmp(fdl.dev[i].name, name))
            return &fdl.dev[i];
    return NULL;
}

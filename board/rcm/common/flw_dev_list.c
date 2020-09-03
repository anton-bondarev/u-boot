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

void flash_dev_list_add(const char* name, unsigned int type, unsigned int par0, unsigned int par1, flw_erase erase, flw_write write, flw_read read)
{
    if (fdl.num < MAX_DEV_NUM)
    {
        strcpy(fdl.dev[fdl.num].name, name);
        fdl.dev[fdl.num].type = type;
        fdl.dev[fdl.num].par0 = par0;
        fdl.dev[fdl.num].par1 = par1;
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
        puts(fdl.dev[i].name);
#ifdef DL_VERB_PRINT
        printf(",%u,%x,%x,%x,%x,%x",
                fdl.dev[i].type, fdl.dev[i].par0, fdl.dev[i].par1,
                (uint32_t)fdl.dev[i].erase, (uint32_t)fdl.dev[i].write, (uint32_t)fdl.dev[i].read);
#endif
    putc('\n');
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

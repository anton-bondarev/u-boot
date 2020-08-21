#include <dm.h>
#include <common.h>
#include <spl.h>
#include <exports.h>

#include "flw_main.h"

static void fill_buf(char* wr_buf, char* rd_buf, unsigned int len)
{
    unsigned int i;
    srand(1);
    for (i=0; i<len; i++)
    {
        wr_buf[i] = rand();
        if (rd_buf)
            rd_buf[i] = ~wr_buf[i];
    }
}

static int cmp_buf(char* wr_buf, char* rd_buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
    {
        if (wr_buf[i] != rd_buf[i])
            return -1;
    }
    return 0;
}

static void print_buf(char* buf, unsigned int len)
{
    #define STRLEN 16
    unsigned int i;
    for (i=0; i<len; i++)
    {
        if(i)
            printf( (i % STRLEN) ? " " : "\n");
        printf("%02x", buf[i]);
    }
    putc('\n');
}

static char* find_space(char* p)
{
    do
    {
        if (*p == ' ')
            return p+1;
    } while(++p);
    return NULL;
}

static int get_addr_size(char* p, unsigned int* addr, unsigned int* size)
{
    if ((p = find_space(p)) == NULL)
        return -1;
    *addr = simple_strtoul(p, NULL, 16);
    if ((p = find_space(p)) == NULL)
        return -1;
    *size = simple_strtoul(p, NULL, 16);
    return 0;
}

static void get_cmd(char* buf, unsigned int len)
{
    #define ECHO
    unsigned int idx = 0;
    while(1)
    {
        char c = getc();
#ifdef ECHO
        putc(c);
#endif
        if( c != '\n' && c != '\r' ) {
            buf[idx] = c, ++idx, idx %= len;
            continue;
        }
        buf[idx] = 0;
        return;
    }
}

static void cmd_dec(void)
{
    static char edcl_buf[4096];
    char buf[256];
    struct flw_dev_t* seldev = NULL;
    unsigned int addr, size;
    int ret;

    while (1)
    {
        get_cmd(buf, sizeof(buf));
        putc('\n');

        if (!strcmp(buf,"help"))
        {
            puts("Usage: help exit list select bufptr rand print erase write read\n");
        }
        else if (!strcmp(buf,"exit"))
        {
            return;
        }
        else if (!strcmp(buf,"list"))
        {
            flash_dev_list_clear();
            flw_spi_flash_list_add();
            flw_mmc_list_add();
            // other devices adding
            flash_dev_list_print();
        }
        else if (!strncmp(buf, "select", 6)) // "select sf00"
        {
            seldev = flash_dev_list_find(buf+7);
            if (!seldev)
                puts("Bad selection!\n");
            else
                printf("Device %s selected\n", seldev->name);
        }
        else if (!strcmp(buf, "bufptr"))
        {
            printf("EDCL buffer %08x\n", (uint32_t)edcl_buf);
        }
        else if (!strcmp(buf, "rand"))
        {
            fill_buf(edcl_buf, NULL, sizeof(edcl_buf));
            print_buf(edcl_buf, sizeof(edcl_buf));
        }
        else if (!strncmp(buf, "print", 5))
        {
            if(get_addr_size(buf, &addr, &size) || addr+size > sizeof(edcl_buf))
                puts("Bad parameters");
            else
                print_buf(edcl_buf+addr, size);
        }
        else
        {
            int erase = !strncmp(buf,"erase", 5),   // "erase 1000 3000"
                write = !strncmp(buf,"write", 5),   // "write 1000 3000"
                read = !strncmp(buf,"read", 4);     // "read 1000 3000"

            if (erase || write || read)
            {
                if (!seldev)
                    puts("Device no select!\n");
                else if(get_addr_size(buf, &addr, &size))
                    puts("Bad parameters");
                else
                {
                    if((write || read) && size > sizeof(edcl_buf)) {
                        size =sizeof(edcl_buf);
                        puts("truncate buffer size\n");
                    }
                    if (erase)
                    {
                        printf("Erase: address %x,size %x...", addr, size);
                        ret = seldev->erase(seldev, addr, size);
                    }
                    else if (write)
                    {
                        printf("Write: address %x,size %x...", addr, size);
                        ret = seldev->write(seldev, addr, size, edcl_buf);
                    }
                    else// if (read)
                    {
                        printf("Read: address %x,size %x...", addr, size);
                        ret = seldev->read(seldev, addr, size, edcl_buf);
                    }
                    puts( ret ? "error\n" : "completed\n");
                }
            }
        }
    }
}

static int flash_writer_pseudo_loader(struct spl_image_info *spl_image, struct spl_boot_device *bootdev)
{
    printf("Flashwriter running(help for information):\n");
    cmd_dec(); 
    return -1;
}

SPL_LOAD_IMAGE_METHOD("Fashwriter", 1, BOOT_DEVICE_FLASH_WRITER, flash_writer_pseudo_loader);
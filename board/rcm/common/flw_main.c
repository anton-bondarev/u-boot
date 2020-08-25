#include <dm.h>
#include <common.h>
#include <spl.h>
#include <exports.h>
#include <time.h>

#include "flw_main.h"

static void fill_buf(char* wr_buf, unsigned int len)
{
    unsigned int i;
    srand(wr_buf[0]|1);
    for (i=0; i<len; i++)
        wr_buf[i] = rand();
}

static int cmp_buf(char* buf0, char* buf1, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
    {
         if (buf0[i] != (char)(~buf1[i]) ) {
            return -1;
        }
    }
    return 0;
}

static void buf_bitrev(char* buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
        buf[i] = ~buf[i];
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

static int get_addr_size(char* p, unsigned long* addr, unsigned long* size)
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
    static char edcl_buf0[EDCL_BUF_LEN], edcl_buf1[EDCL_BUF_LEN];
    char* edcl_buf = edcl_buf0;
    char buf[256];
    struct flw_dev_t* seldev = NULL;
    unsigned long addr, size;
    int ret;

    while (1)
    {
        get_cmd(buf, sizeof(buf));
        putc('\n');

        if (!strcmp(buf,"help"))
        {
            puts("Usage: help version exit list select bufptr rand print erase write read\n");
        }
        else if (!strcmp(buf,"version"))
        {
            puts(FLW_VERSION"\n");
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
            flw_nor_list_add();
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
        else if (!strcmp(buf, "bufsel0"))
        {
            edcl_buf = edcl_buf0;
            puts("selected\n");
        }
        else if (!strcmp(buf, "bufsel1"))
        {
            edcl_buf = edcl_buf1;
            puts("selected\n");
        }
        else if (!strcmp(buf, "bufrev"))
        {
            buf_bitrev(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(buf, "bufcmp"))
        {
            if (!cmp_buf(edcl_buf0, edcl_buf1, EDCL_BUF_LEN))
                printf("buffers are identical\n");
            else
                printf("buffers are different\n");
        }
        else if (!strcmp(buf, "bufptr"))
        {
            printf("EDCL buffer 0x%08x,0x%x\n", (uint32_t)edcl_buf, EDCL_BUF_LEN);
        }
        else if (!strcmp(buf, "rand"))
        {
            fill_buf(edcl_buf, EDCL_BUF_LEN);
            //print_buf(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else if (!strncmp(buf, "print", 5))
        {
            if(get_addr_size(buf, &addr, &size) || addr+size > EDCL_BUF_LEN)
                puts("Bad parameters\n");
            else
                print_buf(edcl_buf+addr, size);
        }
        else
        {
            int erase = !strncmp(buf,"erase", 5),   // "erase <addr> <size>"
                write = !strncmp(buf,"write", 5),   // "write <addr> <size>"
                read = !strncmp(buf,"read", 4);     // "read <addr> <size>"

            if (erase || write || read)
            {
                if (!seldev)
                    puts("Device no select!\n");
                else if(get_addr_size(buf, &addr, &size))
                    puts("Bad parameters\n");
                else
                {
                    if((write || read) && size > EDCL_BUF_LEN) {
                        size = EDCL_BUF_LEN;
                        puts("truncate buffer size\n");
                    }
                    if (erase)
                    {
                        printf("Erase: address %lx,size %lx...", addr, size);
                        ret = seldev->erase(seldev, addr, size);
                    }
                    else if (write)
                    {
                        printf("Write: address %lx,size %lx...", addr, size);
                        ret = seldev->write(seldev, addr, size, edcl_buf);
                    }
                    else// if (read)
                    {
                        printf("Read: address %lx,size %lx...", addr, size);
                        ret = seldev->read(seldev, addr, size, edcl_buf);
                    }
                    if (ret)
                        printf("error %d\n", ret);
                    else
                        puts("completed\n");
                }
            }
        }
    }
}

static int flash_writer_pseudo_loader(struct spl_image_info *spl_image, struct spl_boot_device *bootdev)
{
    printf("Flashwriter(%s) running(help for information):\n", FLW_VERSION);
    cmd_dec(); 
    return -1;
}

SPL_LOAD_IMAGE_METHOD("Fashwriter", 1, BOOT_DEVICE_FLASH_WRITER, flash_writer_pseudo_loader);
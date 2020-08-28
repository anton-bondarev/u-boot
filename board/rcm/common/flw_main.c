#include <dm.h>
#include <common.h>
#include <spl.h>
#include <exports.h>
#include <time.h>

#include "flw_main.h"

static uint32_t get_sys_timer(void)
{ // for 1888tx018 only
    #define TMR_ADDR 0x000a2004
    return *(uint32_t*)TMR_ADDR;
}

static void fill_buf(char* wr_buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
        wr_buf[i] = rand();
}

static int cmp_buf(char* buf0, char* buf1, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
    {
         if (buf0[i] != (char)(~buf1[i]) ) {
            //printf("buffers  different %u:%02x-%02x\n", i, buf0[i], buf1[i]);
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

static void delay(unsigned int value)
{
    unsigned int i;
    for (i=0; i < value; i++) asm("nop\n");
}

static void load_buf(char* buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
        buf[i] = getc();
        putc(buf[i]);
    }
}

static void load_buf_xmodem(char* buf, unsigned int len)
{
    int err, res;
    connection_info_t info;
    delay(0x800000);
    res = xyzModem_stream_open(&info, &err);
    if (!res) {
        while ((res = xyzModem_stream_read( buf, 1024, &err )) > 0) {
            buf += res;
            len -= res;
        }
        xyzModem_stream_close(&err);
    }
}

static void send_buf(const char* buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
        getc();
        putc(buf[i]);
    }
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

char edcl_buf0[EDCL_BUF_LEN], edcl_buf1[EDCL_BUF_LEN];

static void cmd_dec(void)
{
    char* edcl_buf = edcl_buf0;
    char cmd_buf[256];
    struct flw_dev_t* seldev = NULL;
    unsigned long addr, size;
    int ret;

    while (1)
    {
        get_cmd(cmd_buf, sizeof(cmd_buf));
        putc('\n');

        if (!strcmp(cmd_buf,"help"))
        {
            puts("Usage: help version exit list select bufptr rand print setbuf setbufx getbuf erase write read\n");
        }
        else if (!strcmp(cmd_buf,"version"))
        {
            puts(FLW_VERSION"\n");
        }
        else if (!strcmp(cmd_buf,"exit"))
        {
            return;
        }
        else if (!strcmp(cmd_buf,"list"))
        {
            flash_dev_list_clear();
#ifdef CONFIG_SPL_SPI_FLASH_SUPPORT
            flw_spi_flash_list_add();
#endif
#ifdef CONFIG_SPL_MMC_SUPPORT
            flw_mmc_list_add();
#endif
#ifdef CONFIG_SPL_NOR_SUPPORT
            flw_nor_list_add();
#endif
#ifdef CONFIG_SPL_NAND_SUPPORT
            flw_nand_list_add();
#endif
            flash_dev_list_print();
        }
        else if (!strncmp(cmd_buf, "select", 6)) // "select sf00"
        {
            seldev = flash_dev_list_find(cmd_buf+7);
            if (!seldev)
                puts("Bad selection!\n");
            else
                printf("Device %s selected\n", seldev->name);
        }
        else if (!strcmp(cmd_buf, "bufsel0"))
        {
            edcl_buf = edcl_buf0;
            puts("selected\n");
        }
        else if (!strcmp(cmd_buf, "bufsel1"))
        {
            edcl_buf = edcl_buf1;
            puts("selected\n");
        }
        else if (!strcmp(cmd_buf, "bufrev"))
        {
            buf_bitrev(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "bufcmp"))
        {
            if (!cmp_buf(edcl_buf0, edcl_buf1, EDCL_BUF_LEN))
                printf("buffers are identical\n");
            else
                printf("buffers are different\n");
        }
        else if (!strcmp(cmd_buf, "bufptr"))
        {
            printf("EDCL buffer 0x%08x,0x%08x,0x%x\n", (uint32_t)edcl_buf[0], (uint32_t)edcl_buf[1], EDCL_BUF_LEN);
        }
        else if (!strcmp(cmd_buf, "rand"))
        {
            fill_buf(edcl_buf, EDCL_BUF_LEN);
            //print_buf(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else if (!strncmp(cmd_buf, "print", 5))
        {
            if(get_addr_size(cmd_buf, &addr, &size) || addr+size > EDCL_BUF_LEN)
                puts("Bad parameters\n");
            else
                print_buf(edcl_buf+addr, size);
        }
        else if (!strcmp(cmd_buf, "setbufx"))
        {
            puts("ready\n");
            load_buf_xmodem(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "setbuf"))
        {
            puts("ready\n");
            load_buf(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "getbuf"))
        {
            puts("ready\n");
            send_buf(edcl_buf, EDCL_BUF_LEN);
            puts("completed\n");
        }
        else
        {
            int erase = !strncmp(cmd_buf,"erase", 5),   // "erase <addr> <size>"
                write = !strncmp(cmd_buf,"write", 5),   // "write <addr> <size>"
                read = !strncmp(cmd_buf,"read", 4);     // "read <addr> <size>"

            if (erase || write || read)
            {
                if (!seldev)
                    puts("Device no select!\n");
                else if(get_addr_size(cmd_buf, &addr, &size))
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
    srand(get_sys_timer());
    cmd_dec(); 
    return -1;
}

SPL_LOAD_IMAGE_METHOD("Fashwriter", 1, BOOT_DEVICE_FLASH_WRITER, flash_writer_pseudo_loader);
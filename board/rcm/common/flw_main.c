#include <dm.h>
#include <common.h>
#include <spl.h>
#include <exports.h>
#include <time.h>

#include "flw_main.h"

static int last_err; // for saving possible transactions error, if not error - packet szie

#ifdef CONFIG_TARGET_1888TX018
static uint32_t get_sys_timer(void)
{ // for 1888tx018 only
    #define TMR_ADDR 0x000a2004
    return *(uint32_t*)TMR_ADDR;
}
#endif

static void fill_buf(volatile char* wr_buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
        wr_buf[i] = rand();
}

static int cmp_buf(volatile char* buf0, volatile char* buf1, unsigned int len)
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
        if (!(i % STRLEN))
            printf("%04x: ", i);
        printf("%02x", buf[i]);
    }
    putc('\n');
}

static void load_buf(char* buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++) {
        buf[i] = flw_getc(FLW_TOUT);
        flw_putc(buf[i]);
    }
}

static void send_buf(const char* buf, unsigned int len)
{
    unsigned int i;
    flw_getc(FLW_TOUT);
    for (i=0; i<len; i++) {
        flw_putc(buf[i]);
    }
}

#ifndef SIMPLE_BUFFER_ONLY
static size_t write_data_cb(size_t curpos, void *data, size_t length, void *arg)
{
	struct prog_ctx_t* pc = arg;

	memcpy(pc->src_ptr+curpos, data, length);

    curpos += length;

    if (curpos == EDCL_XMODEM_BUF_LEN) {
        if (pc->dev) {
            last_err = pc->dev->write(pc->dev, pc->dst_addr, EDCL_XMODEM_BUF_LEN, pc->src_ptr);
            pc->dst_addr += EDCL_XMODEM_BUF_LEN;
        }
        curpos = 0;
    }
    return curpos;
}
#endif

static int load_buf_xmodem(struct prog_ctx_t* pc)
{
#ifdef SIMPLE_BUFFER_ONLY
    return xmodem_get(pc->src_ptr, pc->len );
#else
    return xmodem_get_async(pc->len, write_data_cb, pc);
#endif
}

#ifndef SIMPLE_BUFFER_ONLY
static size_t read_data_cb(size_t curpos, void *ptr, size_t length, void *arg)
{
	struct prog_ctx_t* pc = arg;

    if (curpos == 0) {
        if (pc->dev) {
            last_err = pc->dev->read(pc->dev, pc->dst_addr, EDCL_XMODEM_BUF_LEN, pc->src_ptr);
            pc->dst_addr += EDCL_XMODEM_BUF_LEN;
        }
    }
    memcpy(ptr, pc->src_ptr+curpos, length);

    return (curpos + length) % EDCL_XMODEM_BUF_LEN;
}
#endif

static int send_buf_xmodem(struct prog_ctx_t* pc)
{
#ifdef SIMPLE_BUFFER_ONLY
    return xmodem_send(pc->buf, pc->len );
#else
    return xmodem_tx_async(read_data_cb, pc->len, pc);
#endif
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

static int get_addr_size(char* p, unsigned long long* addr, unsigned long long* size)
{
    char* endptr;
    if ((p = find_space(p)) == NULL)
        return -1;
    *addr =  ustrtoull(p, &endptr, 16);
    if ((p = find_space(p)) == NULL)
        return -1;
    *size =  ustrtoull(p, &endptr, 16);
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

#ifdef __PPC__
#define __MSYNC() \
    asm("mbar 0");
    asm("mbar 1");
    asm("msync");
    asm("isync");
#else
#define __MSYNC()
#endif

#define wait_sync(magic) { \
    volatile void *__tmp; \
    do { \
        __tmp = edcl_xmodem_buf_sync; \
        __MSYNC(); \
        flw_delay(1); \
    } while (__tmp != (void *) magic); \
}

#define write_sync(value) \
    edcl_xmodem_buf_sync = (void *) value; \
    __MSYNC(); \

#define write_sync_ack(value) \
    edcl_xmodem_buf_sync_ack = (void *) value; \
    __MSYNC();

static int prog_dev(char* edcl_xmodem_buf, struct flw_dev_t* seldev, char mode, unsigned long long addr, unsigned long long size)
{
    int res;

    puts("erasing\n");
    res = seldev->erase(seldev, addr, size);
    if (res) {
        puts("error\n");
        return res;
    }
    puts("completed\n");

    flw_delay(8000000l/1000);
    if (mode == 'X') {
        struct prog_ctx_t pc;
        pc.src_ptr = edcl_xmodem_buf;
        pc.len = size;
        pc.dst_addr = addr;
        pc.dev = seldev;
        res = last_err = load_buf_xmodem(&pc);
    }
    else {
        write_sync(NULL);
        volatile void *curbuf = edcl_xmodem_buf0;
        wait_sync(curbuf);
        write_sync_ack(curbuf);
        while (size > 0) { 
            write_sync(NULL);
            volatile void *nextbuf = (curbuf == edcl_xmodem_buf0) ? edcl_xmodem_buf1 : edcl_xmodem_buf0;
            /* Handle last packet in batch */
            if (size > EDCL_XMODEM_BUF_LEN) {
                wait_sync(nextbuf);
                write_sync_ack(nextbuf);
            }
            unsigned long long towrite = (size > EDCL_XMODEM_BUF_LEN) ? EDCL_XMODEM_BUF_LEN : size;
            res = seldev->write(seldev, addr, towrite, (void *) curbuf);
            if (res) {                                                              // if error,break operation and rememeber error code
                last_err = res;
                break;
            }
            curbuf = nextbuf;
            addr += towrite;
            size -= towrite;
        }
    write_sync(NULL);
    }
    return res;
}

static int dupl_dev(char* edcl_xmodem_buf, struct flw_dev_t* seldev, char mode, unsigned long long addr, unsigned long long size)
{
    int res = -1;

    puts("ready\n");

    if (mode == 'X') {
        struct prog_ctx_t pc;
        pc.src_ptr = (char*)edcl_xmodem_buf0;
        pc.len = size;
        pc.dst_addr = addr;
        pc.dev = seldev;
        res = last_err = send_buf_xmodem(&pc);
    }
    else {
        write_sync(NULL);
        char* curbuf = (char*)edcl_xmodem_buf0;
        while (size > 0) {                                                          // check size % EDCL_XMODEM_BUF_LEN is 0 before
            wait_sync(curbuf);
            write_sync_ack(curbuf);
            volatile void *nextbuf = (curbuf == edcl_xmodem_buf0) ? edcl_xmodem_buf1 : edcl_xmodem_buf0;
            unsigned long long toread = (size > EDCL_XMODEM_BUF_LEN) ? EDCL_XMODEM_BUF_LEN : size;
            res = seldev->read(seldev, addr, toread, curbuf);        // reading
            if (res) {                                                              // if error,break operation and rememeber error code
                last_err = res;
                break;
            }
            write_sync(NULL);
            addr += toread;
            size -= toread;
            curbuf = nextbuf;
        }
        write_sync(NULL);
    }
    return res;
}

static void upd_dev_list(void)
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
#ifdef CONFIG_SPL_I2C_SUPPORT
    flw_i2c_eeprom_list_add();
#endif
    flash_dev_list_print();
}

static void dtest(const char* devname)
{
    unsigned long long addr = 0, size = EDCL_XMODEM_BUF_LEN;
    char* src = (char*)edcl_xmodem_buf0;
    char* dst = (char*)edcl_xmodem_buf1;

    upd_dev_list();

    struct flw_dev_t* seldev = flash_dev_list_find(devname);
    if (!seldev) {
        printf("Unknown device (%s)\n", devname);
        return;
    }

    while (addr < seldev->dev_info.full_size) {
        fill_buf(src, size);
        if (seldev->erase(seldev, addr, size)) {
            puts("Erase failed\n");
            break;
        }
        if (seldev->write(seldev, addr, size, src)) {
            puts("Write failed\n");
            break;
        }
        if (seldev->read(seldev, addr, size, dst)) {
            puts("Read failed\n");
            break;
        }
        int err = memcmp(src, dst, size);
        printf("Address %x%08x,size %x%08x,ret=%d\n",
            (unsigned int)(addr>>32), (unsigned int)addr, (unsigned int)(size>>32), (unsigned int)size, err);
        if (err)
            break;
        addr += size;
    }
}

static void set_br(const char* br_str)
{
    char* endptr;
    unsigned long new_baudrate = ustrtoul(br_str, &endptr, 10);
    printf("Baudrate: current=%u", gd->baudrate);
    if (new_baudrate >= BAUDRATE_MIN && new_baudrate <= BAUDRATE_MAX)
    {
        gd->baudrate = (unsigned int)new_baudrate;
        printf(",new=%u\n", gd->baudrate);
        udelay(100000);
        serial_setbrg();
    }  
    puts("\n");
}

static void cmd_dec(void)
{
    char* edcl_xmodem_buf = (char*)edcl_xmodem_buf0;
    char cmd_buf[256];
    struct flw_dev_t* seldev = NULL;
    struct prog_ctx_t pc = {0};
    unsigned long long addr, size;
    int ret;

    while (1)
    {
        get_cmd(cmd_buf, sizeof(cmd_buf));
        putc('\n');

        if (!strcmp(cmd_buf,"help"))
        {
            puts("Usage: help version reset exit baudrate list select dtest bufptr rand print bufrev bufcmp setbuf setbufx getbuf getbufx erase write read program duplicate lasterr\n");
        }
        else if (!strcmp(cmd_buf,"version"))
        {
            puts(FLW_VERSION"\n");
        }
        else if (!strcmp(cmd_buf,"reset"))
        {
            do_reset(NULL, 0, 0, NULL);
        }
        else if (!strcmp(cmd_buf,"exit"))
        {
            return;
        }
        else if (!strncmp(cmd_buf,"baudrate", 8))
        {
            set_br(cmd_buf+9);
        }
        else if (!strcmp(cmd_buf,"list"))
        {
            upd_dev_list();
        }
        else if (!strncmp(cmd_buf, "select", 6)) // "select sf00"
        {
            seldev = flash_dev_list_find(cmd_buf+7);
            if (!seldev)
                puts("Bad selection!\n");
            else
                printf("Device %s selected\n", seldev->name);
        }
        else if (!strcmp(cmd_buf, "sync"))
        {
            printf("sync 0x%08x\n", (unsigned int)edcl_xmodem_buf_sync);
        }
        else if (!strcmp(cmd_buf, "bufsel0"))
        {
            edcl_xmodem_buf = (char*)edcl_xmodem_buf0;
            puts("selected\n");
        }
        else if (!strcmp(cmd_buf, "bufsel1"))
        {
            edcl_xmodem_buf = (char*)edcl_xmodem_buf1;
            puts("selected\n");
        }
        else if (!strcmp(cmd_buf, "bufrev"))
        {
            buf_bitrev(edcl_xmodem_buf, EDCL_XMODEM_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "bufcmp"))
        {
            if (!cmp_buf(edcl_xmodem_buf0, edcl_xmodem_buf1, EDCL_XMODEM_BUF_LEN))
                printf("buffers are identical\n");
            else
                printf("buffers are different\n");
        }
        else if (!strcmp(cmd_buf, "bufptr"))
        {
            printf("buffers 0x%x 0x%x sync 0x%x ack 0x%x length 0x%x\n",
                flw_virt_to_dma(edcl_xmodem_buf0), 
                flw_virt_to_dma(edcl_xmodem_buf1), 
                flw_virt_to_dma(&edcl_xmodem_buf_sync), 
                flw_virt_to_dma(&edcl_xmodem_buf_sync_ack),
                EDCL_XMODEM_BUF_LEN);
        }
        else if (!strcmp(cmd_buf, "rand"))
        {
            fill_buf(edcl_xmodem_buf, EDCL_XMODEM_BUF_LEN);
            //print_buf(edcl_xmodem_buf, EDCL_XMODEM_BUF_LEN);
            puts("completed\n");
        }
        else if (!strncmp(cmd_buf, "print", 5))
        {
            if(get_addr_size(cmd_buf, &addr, &size) || addr+size > EDCL_XMODEM_BUF_LEN)
                puts("Bad parameters\n");
            else
                print_buf(edcl_xmodem_buf+addr, size);
        }
        else if (!strcmp(cmd_buf, "setbufx"))
        { // just load buffer,one or multiple time
            puts("ready\n");
            pc.src_ptr = edcl_xmodem_buf;
            pc.len = EDCL_XMODEM_BUF_LEN;
            pc.dst_addr = 0;
            pc.dev = NULL;
            last_err = load_buf_xmodem(&pc);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "getbufx"))
        { // just read buffer,last loading data will read
            puts("ready\n");
            pc.src_ptr = edcl_xmodem_buf;
            pc.len = EDCL_XMODEM_BUF_LEN;
            pc.dst_addr = 0;
            pc.dev = NULL;
            last_err = send_buf_xmodem(&pc);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "setbuf"))
        {
            puts("ready\n");
            load_buf(edcl_xmodem_buf, EDCL_XMODEM_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "getbuf"))
        {
            puts("ready\n");
            send_buf(edcl_xmodem_buf, EDCL_XMODEM_BUF_LEN);
            puts("completed\n");
        }
        else if (!strcmp(cmd_buf, "lasterr"))
        {
            printf("Last Xmodem error %d\n", last_err);
        }
        else if (!strncmp(cmd_buf, "dtest", 5))                 // "dtest <devname>"
        {
            char* devname = cmd_buf+6;
            dtest(devname);
            puts("completed\n");
        }
        else
        {
            int erase = !strncmp(cmd_buf,"erase", 5),           // "erase <addr> <size>"
                write = !strncmp(cmd_buf,"write", 5),           // "write <addr> <size>"
                read = !strncmp(cmd_buf,"read", 4),             // "read <addr> <size>"
                program = !strncmp(cmd_buf, "program", 7),      // "program <E,X> <addr> <size>"
                duplicate = !strncmp(cmd_buf, "duplicate", 9);  // "duplicate <E,X> <addr> <size>"

            if (erase || write || read || program || duplicate)
            {
                if (!seldev)
                    puts("Device no select!\n");
                else if (get_addr_size( program ? cmd_buf+8 : duplicate ? cmd_buf+10 : cmd_buf, &addr, &size) ||
                        (program && ( cmd_buf[8] != 'E' && cmd_buf[8] != 'X' )) ||
                        (duplicate && ( cmd_buf[10] != 'E' && cmd_buf[10] != 'X' )))
                    puts("Bad parameters\n");
                else if ((program || duplicate) && (size % seldev->dev_info.erase_size))
                    puts("Data size not multiple of buffer size\n");
                else
                {
                    if((write || read) && size > EDCL_XMODEM_BUF_LEN) {
                        size = EDCL_XMODEM_BUF_LEN;
                        puts("truncate buffer size\n");
                    }
                    if (erase || write || read) {
                        printf("%s: address %x%08x,size %x%08x...",
                            erase ? "Erase" : write ? "Write" : "Read",
                            (unsigned int)(addr>>32), (unsigned int)addr, (unsigned int)(size>>32), (unsigned int)size);
                    }
                    if (erase)
                    {
                        ret = seldev->erase(seldev, addr, size);
                    }
                    else if (write)
                    {
                        ret = seldev->write(seldev, addr, size, edcl_xmodem_buf);
                    }
                    else if (read)
                    {
                        ret = seldev->read(seldev, addr, size, edcl_xmodem_buf);
                    }
                    else if (program)
                    {
                        ret = prog_dev(edcl_xmodem_buf, seldev, cmd_buf[8], addr, size);
                    }
                    else // if (duplicate)
                    {
                        ret = dupl_dev(edcl_xmodem_buf, seldev, cmd_buf[10], addr, size);
                    }
                    if (ret < 0)
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
    printf("FlashWriter (%s) (help for useless info)\n", FLW_VERSION);
#ifdef CONFIG_TARGET_1888TX018 // for testing only,remove after
    srand(get_sys_timer());
#endif
    cmd_dec();
    return -1;
}

SPL_LOAD_IMAGE_METHOD("Flashwriter", 1, BOOT_DEVICE_FLASH_WRITER, flash_writer_pseudo_loader);
#include <dm.h>
#include <common.h>
#include <spl.h>
#include <spi_flash.h>
#include "flashwriter.h"
#include <dm/device-internal.h>

static void fill_buf(char* wr_buf, char* rd_buf, unsigned int len)
{
    unsigned int i;
    for (i=0; i<len; i++)
    {
        wr_buf[i] = rand();
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
}

static struct spi_flash *flash; // CONFIG_SPL_SPI_FLASH_TINY выключить в конфиге!!!

static int flw_spi_flash_found( unsigned int bus, unsigned int cs )
{
    if (flash)
        spi_flash_free(flash);

    flash = spi_flash_probe(bus, cs, CONFIG_SF_DEFAULT_SPEED, CONFIG_SF_DEFAULT_MODE);

    printf("%s SPI flash at bus %u, cs %u\n", flash ? "Found" : "Failed to initialize", bus, cs);
    if (flash) {
        printf("Name %s, size 0x%x, sector size 0x%x, erase size 0x%x\n", flash->name, flash->size, flash->sector_size, flash->erase_size);
        return 0;
    }
    return -1;
}

const char* flw_spi_flash_list(void)
{
    #define BUS_NUM 1
    #define CS_NUM 3
    unsigned int bus, cs;
    static char buf[256]; // зависит от числа шин и селектов

    for (bus=0; bus<BUS_NUM; bus++)
    {
        for (cs=0; cs<CS_NUM; cs++)
        {
            if (!flw_spi_flash_found(bus, cs))
            {
                char buf1[16];
                sprintf( buf1, "sf%u%u,", bus, cs );
                strcat(buf, buf1);
            }
        }
    }
    strcat(buf, "\n");
    return buf;
}

int flw_erase_all(struct spi_flash *flash)
{
    #define SKIP_ERASE
    unsigned int off;
#ifndef SKIP_ERASE
    for (off=0; off<flash->size; off+=flash->erase_size)
    {
        if (spi_flash_erase(flash, off, flash->erase_size))
        {
            printf( "Erase error at %x\n", off);
            return -1;
        }
        printf( "Erase at %x..%x-OK\n", off, off+flash->erase_size-1);
    }
#endif
    return 0;
}

void flw_spi_flash_test(void)
{
    puts("spi_flash_test:\n");
    return;

// спросим,что-же включено на spi,ответ-строка sf00,sf01 и.т.д (номер шины+номер селекта)
    puts(flw_spi_flash_list());

// в реальной жизни надо будет отдать адрес буфера wr_buf,и закинуть туда данные по edcl/xmodem
// если без проверки, второй буфер будет не нужен
    #define BUFLEN (2048*4)
    static char wr_buf[BUFLEN], rd_buf[BUFLEN];
    unsigned int off;

    if (!flw_spi_flash_found(0, 0) && !flw_erase_all(flash))
    {
        for (off=0; off<flash->size; off+=BUFLEN) {

                fill_buf(wr_buf, rd_buf, BUFLEN);

                if (spi_flash_write(flash, off, BUFLEN, wr_buf)) {
                    printf( "Write error at %x\n", off);
                    break;
                }

                if (spi_flash_read(flash, off, BUFLEN, rd_buf)) {
                    printf( "Read error at %x\n", off);
                    break;
                }

                if (cmp_buf(wr_buf, rd_buf, BUFLEN)) {
                    printf( "Compare error at %x\n", off);
                    break;
                }

                //print_buf(rd_buf, BUFLEN); while(1);

                printf("Address %x..%x-OK\n", off, off+BUFLEN-1);
        }
        print_buf(rd_buf, BUFLEN); while(1);
    }
}

static int flash_writer_pseudo_loader(struct spl_image_info *spl_image, struct spl_boot_device *bootdev)
{
    printf("Flashwriter running:\n");
    return -1;
}

SPL_LOAD_IMAGE_METHOD("Fashwriter", 1, BOOT_DEVICE_FLASH_WRITER, flash_writer_pseudo_loader);
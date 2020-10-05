#include <serial.h>
#include <exports.h>
#include <common.h>
#include <dm.h>

#include "flw_serial.h"

int flw_putc(char ch)
{
    int err;
    struct dm_serial_ops *ops = serial_get_ops(gd->cur_serial_dev);

    while (1) {
#ifdef CONFIG_TARGET_1879VM8YA
        flw_delay(10000);
#endif
        err = ops->putc(gd->cur_serial_dev, ch);
        if (err == -EAGAIN)
            continue;

        return err;
    }
}

int flw_getc(unsigned long tout)
{
    int err;
    struct dm_serial_ops *ops = serial_get_ops(gd->cur_serial_dev);

    while (1) {
        if (tout-- == 0)
            return -EIO;

        err = ops->getc(gd->cur_serial_dev);

        if (err == -EAGAIN)
            continue;

        return err;
    }
}

void flw_delay(unsigned long value)
{
    unsigned long i;
    for (i=0; i < value; i++) asm("nop\n");
}

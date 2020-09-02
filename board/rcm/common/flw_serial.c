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

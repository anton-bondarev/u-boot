#include <serial.h>
#include <exports.h>
#include <common.h>
#include <dm.h>

void flw_putc(char ch)
{
    int err;
    struct dm_serial_ops *ops = serial_get_ops(gd->cur_serial_dev);
    do {
        err = ops->putc(gd->cur_serial_dev, ch);
    } while (err == -EAGAIN);
}

char flw_getc(void)
{
    int err;
    struct dm_serial_ops *ops = serial_get_ops(gd->cur_serial_dev);

    do {
        err = ops->getc(gd->cur_serial_dev);
	} while (err == -EAGAIN);
	return err >= 0 ? err : 0;
}

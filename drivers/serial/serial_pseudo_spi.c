/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2019 Alexey Spirkov <dev@alsp.net>
 */

#include <common.h>
#include <dm.h>
#include <serial.h>
#include <spi.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

#define PUART_BUFFER_SIZE 1024

struct pseudo_spi_serial_priv
{
    struct spi_slave *slave;
    unsigned char buffer[PUART_BUFFER_SIZE];
    unsigned int bytes_in_buffer;
    unsigned char waiting;
};

static int pseudo_spi_serial_probe(struct udevice *dev)
{
    struct pseudo_spi_serial_priv *priv = dev_get_priv(dev);
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);

    priv->waiting = 0;
    priv->bytes_in_buffer = 0;

    priv->slave = spi_setup_slave(0, slave_plat->cs, slave_plat->max_hz, slave_plat->mode);
    if (priv->slave)
        return 0;

    return -EIO;
}

static int pseudo_spi_serial_setbrg(struct udevice *dev, int baudrate)
{
    return 0;
}

static int pseudo_spi_serial_getc(struct udevice *dev)
{
    int ret;
    struct pseudo_spi_serial_priv *priv = dev_get_priv(dev);

    if (priv->waiting)
    {
        ret = priv->waiting;
        priv->waiting = 0;
        return ret;
    }

    unsigned char din[2];
    unsigned char dout[2];
    dout[0] = dout[1] = 0;

    if(!spi_claim_bus(priv->slave))
    {
        ret = spi_xfer(priv->slave, 16, &dout[0], &din[0], SPI_XFER_ONCE);
        spi_release_bus(priv->slave);
        if (!ret)
        {
            if (din[0] == 0x1)
                return din[1];
            else
                return -EAGAIN;
        }
    }
    else
        return -EAGAIN;

    return -EIO;
}

static int pseudo_spi_serial_putc(struct udevice *dev, const char ch)
{
    struct pseudo_spi_serial_priv *priv = dev_get_priv(dev);
    int ret = 0;

    unsigned char din[4];
    unsigned char dout[4];

    dout[0] = 0x1;

    if(!spi_claim_bus(priv->slave))
    {
        if(priv->bytes_in_buffer)
        {
            for(int i = 0; i < priv->bytes_in_buffer; i++)
            {
                dout[1] = priv->buffer[i];
                ret = spi_xfer(priv->slave, 16, &dout[0], &din[0], SPI_XFER_ONCE);
                if(ret)
                    break;
            }
            priv->bytes_in_buffer = 0;
        }
        dout[1] = ch;
        ret = spi_xfer(priv->slave, 16, &dout[0], &din[0], SPI_XFER_ONCE);
        spi_release_bus(priv->slave);
    }
    else
    {
        // put to buffer
        if(priv->bytes_in_buffer < (PUART_BUFFER_SIZE-1))
            priv->buffer[priv->bytes_in_buffer++] = ch;
        
        if(priv->bytes_in_buffer == (PUART_BUFFER_SIZE-4))
        {
            priv->buffer[priv->bytes_in_buffer++] = '.';
            priv->buffer[priv->bytes_in_buffer++] = '.';
            priv->buffer[priv->bytes_in_buffer++] = '.';
        }
    }

    return ret;
}

static int pseudo_spi_serial_pending(struct udevice *dev, bool input)
{
    struct pseudo_spi_serial_priv *priv = dev_get_priv(dev);

    if (input)
    {
        int ret = pseudo_spi_serial_getc(dev);
        if (ret > 0)
        {
            priv->waiting = (unsigned char)ret;
            return 1;
        }
    }

    return 0;
}

static const struct dm_serial_ops pseudo_spi_serial_ops = {
    .putc = pseudo_spi_serial_putc,
    .pending = pseudo_spi_serial_pending,
    .getc = pseudo_spi_serial_getc,
    .setbrg = pseudo_spi_serial_setbrg,
};

#ifdef CONFIG_OF_CONTROL
static const struct udevice_id pseudo_spi_serial_id[] = {
    {.compatible = "rcm,serial_pseudo_spi"},
    {}};
#endif

U_BOOT_DRIVER(pseudo_spi_serial) = {
    .name = "pseudo_spi_serial",
    .id = UCLASS_SERIAL,
    .of_match = of_match_ptr(pseudo_spi_serial_id),
    .probe = pseudo_spi_serial_probe,
    .ops = &pseudo_spi_serial_ops,
    .flags = DM_FLAG_PRE_RELOC,
    .priv_auto_alloc_size = sizeof(struct pseudo_spi_serial_priv),

};

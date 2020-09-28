/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  Copyright (C) 2020 Vladimir Shalyt <Vladimir.Shalyt@mir.dev>
 */

#include <common.h>
#include <asm/io.h>
#include <dm.h>
#include <i2c.h>

//#define I2C_BASIS_DEBUG

#ifdef I2C_BASIS_DEBUG
    #define DBG_PRINT(...) printf(__VA_ARGS__);
    #define PROC_INF DBG_PRINT("%s(%u)\n", __FUNCTION__, __LINE__);
    #define PRINT_BIT(BIT) DBG_PRINT("%s=%u\n", #BIT, status & BIT ? 1 : 0)
    #define PRINT_BUF(BUF) \
        DBG_PRINT("%s: buf(%x)=%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n", \
            __FUNCTION__, (u32)BUF, BUF[0], BUF[1], BUF[2], BUF[3], BUF[4], BUF[5], BUF[6], BUF[7]);
    //#define VERBOSE_STATUS
#else
    #define DBG_PRINT(...) while(0);
    #define PROC_INF DBG_PRINT while(0);
    #define PRINT_BIT(BIT) while(0);
    #define PRINT_BUF(BUF) while(0);
#endif

#define I2C_BASIS_ID_REG                0x000
#define I2C_BASIS_ISR_REG               0х004
#define I2C_BASIS_IER_REG               0x008
#define I2C_BASIS_SOFTR                 0x00C
#define I2C_BASIS_CR                    0x010
#define I2C_BASIS_SR                    0x018
#define I2C_BASIS_NUMBR                 0x01C
#define I2C_BASIS_STAT_RST              0x020
#define I2C_BASIS_CLKPR                 0x024
#define I2C_BASIS_FIFOFIL               0x028
#define I2C_BASIS_FILTR                 0x02C
#define I2C_BASIS_TX_FIFO               0x030       // 0x030-0х42c 
#define I2C_BASIS_RX_FIFO               0x430       // 0х430-0х82с

#define ISR_INT_DONE                    0x00000001  // Byte operation completed
#define ISR_INT_AL                      0x00000002  // A lost arbitration occurred
#define ISR_INT_TRN_EMPTY               0x00000004  // Transmit buffer empty
#define ISR_INT_RCV_FULL                0x00000008  // Received buffer full
#define ISR_INT_TRN_ALMOST_EMPTY        0x00000010  // When sending the next byte, the transmitting buffer reached the fullness level specified in the FIFOFIL register
#define ISR_INT_RCV_ALMOST_FULL         0x00000020  // When the next byte was received, the receive buffer reached the fullness level indicated in the FIFOFIL register
#define ISR_INT_NACK                    0x00000040  // No confirmation received (ACK)
#define ISR_INT_RCV_FINISH              0x00000080  // NUMBR bytes received in the current transaction
#define ISR_INT_BUSY_END                0x00000100  // The controller ends the operation and releases the I2C interface

#define IER_EN_INT_DONE                 0x00000001  // Interrupt enable for each of the bits of the ISR register: 0 - interrupt disabled, 1 - interrupt enabled
#define IER_EN_INT_AL                   0x00000002
#define IER_EN_INT_TRN_EMPTY            0x00000004
#define IER_EN_INT_RCV_FULL             0x00000008
#define IER_EN_INT_TRN_EMPTY_ALM        0x00000010
#define IER_EN_INT_RCV_FULL_ALM         0x00000020
#define IER_EN_INT_NACK                 0x00000040
#define IER_EN_INT_RCV_FINISH           0x00000080
#define IER_EN_INT_BUSY_END             0x00000100

#define CR_REG_I2C_EN                   0x00000001  // Enabling the I2C controller
#define CR_REG_START                    0x00000002  // Command begin with a start character (S).
#define CR_REG_TX_FIFO_RST              0x00000004  // Reset TX FIFO.
#define CR_REG_RD                       0x00000008  // Reading command
#define CR_REG_WR                       0x00000010  // Writing command
#define CR_REG_REPEAT                   0x00000020  // Command begin with a repeat start character  (Sr).
#define CR_REG_STOP                     0x00000040  // Command ends with a stop character (P).

#define SR_REG_IBUSY                    0x00000001  // I2C bus is busy (was symbol S, but P don't present).
#define SR_REG_AL                       0x00000002  // A lost arbitration occurred
#define SR_REG_TRN_ALMOST_EMPTY         0x00000004  // When sending the next byte, the transmitting buffer reached the fullness level specified in the FIFOFIL register
#define SR_REG_RCV_ALMOST_FULL          0x00000008  // When the next byte was received, the receive buffer reached the fullness level indicated in the FIFOFIL register
#define SR_REG_TRN_EMPTY                0x00000010  // Transmit buffer empty
#define SR_REG_RCV_FULL                 0x00000020  // Received buffer full
#define SR_REG_TRN_FULL                 0x00000040  // Transmit buffer full
#define SR_REG_RCV_EMPTY                0x00000080  // Received buffer empty
#define SR_REG_RES                      0x00000100  // Reserved
#define SR_REG_NACK                     0x00000200  // NACK was received
#define SR_REG_DONE                     0x00000400  // Byte operation completed
#define SR_REG_STATE_CMD                0x00007800  // State of the send / receive state mashine
// 0b0000 – void,0b0001 – start sending (S),0b0010 – stop sending (P),0b0100 – transmiting, 0b1000 – receiving
#define SR_REG_CNT                      0x007F8000  // The number of bytes received in the current transaction (counts from 0 to NUMBR)
#define SR_REG_SCL_IN                   0x00800000  // The current value of the SCL signal at the external pin
#define SR_REG_SCL_OEN                  0x01000000  // 0 - the controller outputs 0 to the SCL bus, 1 - does not
#define SR_REG_SDA_IN                   0x02000000  // The current value of the SDA signal at the external pin
#define SR_REG_SDA_OEN                  0x04000000  // 0 - the controller outputs 0 to the SDA bus, 1 - does not

// Stop. If the previous operation completed in a busy bus state, this command causes a completion character (P) to be sent
#define CMD_STOP (CR_REG_I2C_EN | CR_REG_STOP)
#define CMD_START (CR_REG_I2C_EN | CR_REG_START)
// The transmit buffer is empty
#define CMD_RST_BUF (CR_REG_I2C_EN | CR_REG_TX_FIFO_RST)
// Complete write transaction. With rpt = 0 it starts with the start symbol (S), with rpt = 1 - with the restart symbol (Sr). Ends with a termination character (P)
#define CMD_WRITE_FULL (CR_REG_I2C_EN | CR_REG_WR | CR_REG_START | CR_REG_STOP)
#define CMD_WRITE_FULL_REP (CMD_WRITE_FULL | CR_REG_REPEAT)
// Start of a write transaction. With rpt = 0 it starts with the start symbol (S), with rpt = 1 - with the restart symbol (Sr). After performing this operation, the bus remains busy (SCL = 0)
#define CMD_WRITE_BEGIN (CR_REG_I2C_EN | CR_REG_WR | CR_REG_START)
#define CMD_WRITE_BEGIN_REP (CMD_WRITE_BEGIN | CR_REG_REPEAT)
// Continue a previously started write operation. After performing this operation, the bus remains busy (SCL = 0)
#define CMD_WRITE_CONTINUE (CR_REG_I2C_EN | CR_REG_WR)
// Completion of a previously started write operation. Ends with a termination character (P)
#define CMD_WRITE_FINISH (CR_REG_I2C_EN | CR_REG_WR | CR_REG_STOP)
// Complete read transaction. With rpt = 0 it starts with the start symbol (S), with rpt = 1 - with the restart symbol (Sr). Ends with a completion character (P)
#define CMD_READ_FULL (CR_REG_I2C_EN | CR_REG_RD | CR_REG_START | CR_REG_STOP)
#define CMD_READ_FULL_REP (CMD_READ_FULL | CR_REG_REPEAT)
// Start of a read transaction. With rpt = 0 it starts with the start symbol (S), with rpt = 1 - with the restart symbol (Sr). After performing this operation, the bus remains busy (SCL = 0)
#define CMD_READ_BEGIN (CR_REG_I2C_EN | CR_REG_RD | CR_REG_START)
#define CMD_READ_BEGIN_REP (CMD_READ_BEGIN | CR_REG_REPEAT)
// Continuation of a previously started read operation. After performing this Read Data (from 1 to 255 bytes) operation, the bus remains busy (SCL = 0)
#define CMD_READ_CONTINUE (CR_REG_I2C_EN | CR_REG_RD)
// Completion of a previously started read operation. Ends with character Read data (1 to 255 bytes) completion (P)
#define CMD_READ_FINISH (CR_REG_I2C_EN | CR_REG_RD | CR_REG_STOP)

#define I2C_BASIS_CTRL_ID               0x012c012c
#define I2C_BASIS_TOUT                  10000000

#define I2C_CNV_WR_ADDR(addr) ((u8)(addr<<1))
#define I2C_CNV_RD_ADDR(addr) ((u8)((addr<<1)+1))

struct i2c_basis {
    void __iomem* base;
    int irq_en;
    uint32_t bus_freq;
    uint32_t scl_freq;
    unsigned int write_delay;
    unsigned int rx_fifo_size;
    unsigned int tx_fifo_size;
};

static void setreg32(struct i2c_basis* i2c, int reg, u32 value)
{
    writel(value, i2c->base + reg);
}

static u32 getreg32(struct i2c_basis* i2c, int reg)
{
    u32 value = readl(i2c->base + reg);
    return value;
}

static void setreg8(struct i2c_basis* i2c, int reg, u8 value)
{
    writeb(value, i2c->base + reg);
}

static u8 getreg8(struct i2c_basis* i2c, int reg)
{
    u8 value = readb(i2c->base + reg);
    return value;
}

#ifdef VERBOSE_STATUS
static void i2c_basis_print_status(u32 status, unsigned int num)
{
    DBG_PRINT("%u: %08x\n", num, status )
    PRINT_BIT(SR_REG_IBUSY)
    PRINT_BIT(SR_REG_AL)
    PRINT_BIT(SR_REG_TRN_ALMOST_EMPTY)
    PRINT_BIT(SR_REG_RCV_ALMOST_FULL)
    PRINT_BIT(SR_REG_TRN_EMPTY)
    PRINT_BIT(SR_REG_RCV_FULL)
    PRINT_BIT(SR_REG_TRN_FULL)
    PRINT_BIT(SR_REG_RCV_EMPTY)
    PRINT_BIT(SR_REG_NACK)
    PRINT_BIT(SR_REG_DONE)
    printf("SR_REG_STATE_CMD=%u\n", (status & SR_REG_STATE_CMD) >> 11);
    printf("SR_REG_CNT=%u\n", (status & SR_REG_CNT) >> 15);
}
#endif

static int i2c_basis_wait_status(struct i2c_basis* i2c, u32 mask)
{
    u32 status;
    int ret = -1;
    unsigned int i;
#ifdef VERBOSE_STATUS
    unsigned int curr_state = -1, new_state;
#endif
    for (i=0; i<I2C_BASIS_TOUT; i++) {
        status = getreg32(i2c, I2C_BASIS_SR);
#ifdef VERBOSE_STATUS
        new_state =  (status & SR_REG_STATE_CMD) >> 11;
        if (new_state != curr_state)
            printf("SR_REG_STATE_CMD=%u\n", curr_state = new_state);
#endif
        if (status & mask) {
            ret = 0;
            break;
        }
    }
#ifdef VERBOSE_STATUS
    i2c_basis_print_status(status, i);
#endif
    return ret;
}

static void i2c_basis_set_fill(struct i2c_basis* i2c, unsigned int rx_fifo_count, unsigned int tx_fifo_count)
{
    setreg32(i2c, I2C_BASIS_FIFOFIL, (rx_fifo_count << 16) | tx_fifo_count);
}

static void i2c_basis_write_fifo(struct i2c_basis* i2c, unsigned char* wr_buf, unsigned int wr_len)
{
    unsigned int i;
    PRINT_BUF(wr_buf)
    for (i=0; i<wr_len; i++)
        setreg8(i2c, I2C_BASIS_TX_FIFO, wr_buf[i]);
}

static int i2c_basis_write_buffer(struct i2c_basis* i2c, u32 cmd, unsigned char* buf, unsigned int len)
{
    i2c_basis_write_fifo(i2c,  buf, len);
    setreg32(i2c, I2C_BASIS_NUMBR, len);
    i2c_basis_set_fill(i2c, 1, 1);
    setreg32(i2c, I2C_BASIS_CR, cmd);
//    DBG_PRINT("CR=%08x,FIFOFIL=%08x,NUMBR=%08x\n", getreg32(i2c, I2C_BASIS_CR), getreg32(i2c, I2C_BASIS_FIFOFIL), getreg32(i2c, I2C_BASIS_NUMBR));
    return i2c_basis_wait_status(i2c, SR_REG_TRN_EMPTY);
}

static int i2c_basis_write_data(struct i2c_basis* i2c, struct i2c_msg* msg)
{
    int ret;

    if (msg->len == 0)
        return 0;

    if (msg->len > i2c->tx_fifo_size)
        return -EINVAL; // todo it

    setreg32(i2c, I2C_BASIS_STAT_RST, 1);
    setreg8(i2c, I2C_BASIS_TX_FIFO, I2C_CNV_WR_ADDR(msg->addr));
    ret = i2c_basis_write_buffer(i2c, CMD_WRITE_FULL, msg->buf, msg->len);
    setreg32(i2c, I2C_BASIS_STAT_RST, 0);
    return ret;
}

static void i2c_basis_flush_rx_fifo(struct i2c_basis* i2c)
{
     while (1) {
        u32 status = getreg32(i2c, I2C_BASIS_SR);
        if (status & SR_REG_RCV_EMPTY)
            return;
        getreg8(i2c, I2C_BASIS_RX_FIFO);
    }
}

static void i2c_basis_read_fifo(struct i2c_basis* i2c, unsigned char* rd_buf, unsigned int rd_len)
{
    unsigned int i;
    for (i=0; i<rd_len; i++)
        rd_buf[i] = getreg8(i2c, I2C_BASIS_RX_FIFO);
    PRINT_BUF(rd_buf)
}

static int i2c_basis_read_buffer(struct i2c_basis* i2c, u32 cmd, unsigned char* buf, unsigned int len)
{
    int ret;
    setreg32(i2c, I2C_BASIS_STAT_RST, 1);
    setreg32(i2c, I2C_BASIS_NUMBR, len);
    i2c_basis_set_fill(i2c, len, 1);
    setreg32(i2c, I2C_BASIS_CR, cmd);
//    DBG_PRINT("CR=%08x,FIFOFIL=%08x,NUMBR=%08x\n", getreg32(i2c, I2C_BASIS_CR), getreg32(i2c, I2C_BASIS_FIFOFIL), getreg32(i2c, I2C_BASIS_NUMBR));
    ret = i2c_basis_wait_status(i2c, SR_REG_RCV_ALMOST_FULL);
    i2c_basis_read_fifo(i2c, buf, len);
    return ret;
}

static int i2c_basis_read_simple(struct i2c_basis* i2c, struct i2c_msg* msg)
{
    int ret;
    if (msg->len > i2c->rx_fifo_size)
        return -EINVAL; // todo it

    i2c_basis_flush_rx_fifo(i2c);
    setreg8(i2c, I2C_BASIS_TX_FIFO, I2C_CNV_RD_ADDR(msg->addr));
    ret = i2c_basis_read_buffer(i2c, CMD_READ_FULL, msg->buf, msg->len);
    setreg32(i2c, I2C_BASIS_STAT_RST, 0);
    return ret;
}

static int i2c_basis_xfer(struct udevice* bus, struct i2c_msg* msg, int nmsgs)
{
    int ret = -1;
    unsigned int i;
    struct i2c_basis* i2c = dev_get_priv(bus);

    for (i=0; i<nmsgs; i++, msg++)
    {
        DBG_PRINT("%s: nmsgs=%u(from %u),addr=%x,flags=%x,len=%x,buf=%x\n", __FUNCTION__, i, nmsgs, msg->addr, msg->flags, msg->len, (u32)msg->buf);

        if (msg->flags & I2C_M_RD) {
            ret = i2c_basis_read_simple(i2c, msg);
            if (ret) {
                dev_err(dev,"Read failed, ret=%d\n", ret);
                //break;
            }
        }
        else {
            ret = i2c_basis_write_data(i2c, msg);
            if (ret) {
                dev_err(dev,"Write failed, ret=%d\n", ret);
                //break;
            }
            udelay(i2c->write_delay);
        }
    }
    return ret;
}

static int i2c_basis_read_bus_clock(struct udevice* dev, u32* freq)
{
    int ret;
    unsigned int phandle;
    ret = ofnode_read_u32(dev->node, "clocks", &phandle);
    if (ret)
        return ret;
    ret = ofnode_read_u32(ofnode_get_by_phandle(phandle), "clock-frequency", freq);
    return ret;
}

static int i2c_basis_set_clkdiv(struct i2c_basis* i2c)
{
    if (i2c->scl_freq) {
        setreg32(i2c, I2C_BASIS_CLKPR, i2c->bus_freq / (i2c->scl_freq * 5) - 1);
        DBG_PRINT("CLKPR=%08x\n", getreg32(i2c, I2C_BASIS_CLKPR));
        return 0;
    }
    return -EINVAL;
}

static int i2c_basis_set_bus_speed(struct udevice* dev, unsigned int speed)
{
    struct i2c_basis* i2c = dev_get_priv(dev);

    if (speed < 100000 || speed > 1000000) {
        dev_err(dev, "Bus speed invalid\n");
        return -EINVAL;
    }
    i2c->scl_freq = speed;
    return i2c_basis_set_clkdiv(i2c);
}

static int i2c_basis_get_bus_speed(struct udevice *bus)
{
    struct i2c_basis* i2c = dev_get_priv(bus);
    u32 clkdiv = getreg32(i2c, I2C_BASIS_CLKPR) + 1;
    return clkdiv ? i2c->bus_freq / (5*clkdiv) : -EINVAL;
}

static int i2c_basis_i2c_probe(struct udevice* bus)
{
    struct i2c_basis* i2c = dev_get_priv(bus);

    if (getreg32(i2c, I2C_BASIS_ID_REG) != I2C_BASIS_CTRL_ID) {
        dev_err(dev, "I2C controller not found\n");
        return -ENXIO;
    }
    i2c_basis_set_clkdiv(i2c);
    i2c->irq_en = 0;
    return 0;
}

static int i2c_basis_ofdata_to_platdata(struct udevice* dev)
{
    int ret;
    struct i2c_basis* i2c = dev_get_priv(dev);
    
    i2c->base = map_physmem(devfdt_get_addr(dev), 0x1000, MAP_NOCACHE);
    if (!i2c->base)
        return -ENOMEM;

    ret = i2c_basis_read_bus_clock(dev, &i2c->bus_freq);
    if (ret) {
        dev_err(dev, "Bus frequency read error\n");
        return ret;
    }
    i2c->scl_freq = dev_read_u32_default(dev, "bus-clock", 100000);
    i2c->write_delay = dev_read_u32_default(dev, "write-delay", 2000);
    i2c->tx_fifo_size = dev_read_u32_default(dev, "tx-fifo-size", 16);
    i2c->rx_fifo_size = dev_read_u32_default(dev, "rx-fifo-size", 16);
    i2c->irq_en = false;

    DBG_PRINT("Scl freq=%u,bus freq=%u,delay time=%u,tx fifo=%u, rx fifo=%u\n", i2c->scl_freq, i2c->bus_freq, i2c->write_delay, i2c->tx_fifo_size, i2c->rx_fifo_size);
    return 0;
}

static int i2c_basis_reset(struct udevice *bus)
{
    struct i2c_basis* i2c = dev_get_priv(bus);
    setreg32(i2c, I2C_BASIS_SOFTR, 0);
    udelay(5000);
    return 0;
}

static const struct dm_i2c_ops i2c_basis_ops = {
    .xfer = i2c_basis_xfer,
    .set_bus_speed = i2c_basis_set_bus_speed,
    .get_bus_speed = i2c_basis_get_bus_speed,
    .deblock = i2c_basis_reset
};

static const struct udevice_id i2c_basis_ids[] = {
    { .compatible = "rcm,i2cbasis" },
    { }
};

U_BOOT_DRIVER(i2c_basis) = {
    .name = "i2c_basis",
    .id   = UCLASS_I2C,
    .of_match = i2c_basis_ids,
    .probe = i2c_basis_i2c_probe,
    .ofdata_to_platdata = i2c_basis_ofdata_to_platdata,
    .priv_auto_alloc_size = sizeof(struct i2c_basis),
    .ops = &i2c_basis_ops,
};

#include <stdint.h>

typedef unsigned int uint;
#include <debug_uart.h>
#include <serial.h>
#include <mpw7705_baremetal.h>

#define UART0__                 0x3C034000

#define PL011_FIFO_SIZE  32
#define PL061_DATA_BASE  0x000
#define PL061_DIR        0x400

#define     PL011_UARTDR          0x000
#define     PL011_UARTRSR         0x004
#define     PL011_UARTECR         0x004
#define     PL011_UARTFR          0x018
#define     PL011_UARTILPR        0x020
#define     PL011_UARTIBRD        0x024
#define     PL011_UARTFBRD        0x028
#define     PL011_UARTLCR_H       0x02C
#define     PL011_UARTCR          0x030
#define     PL011_UARTIFLS        0x034
#define     PL011_UARTIMSC        0x038
#define     PL011_UARTRIS         0x03C
#define     PL011_UARTMIS         0x040
#define     PL011_UARTICR         0x044
#define     PL011_UARTDMACR       0x048
#define     PL011_UARTPeriphID0   0xFE0
#define     PL011_UARTPeriphID1   0xFE4
#define     PL011_UARTPeriphID2   0xFE8
#define     PL011_UARTPeriphID3   0xFEC
#define     PL011_UARTPCellID0    0xFF0
#define     PL011_UARTPCellID1    0xFF4
#define     PL011_UARTPCellID2    0xFF8
#define     PL011_UARTPCellID3    0xFFC

//uartcr
#define PL011_EN_i       0
#define PL011_SIREN_i    1
#define PL011_SIRLP_i    2
#define PL011_LBE_i      7
#define PL011_TXE_i      8
#define PL011_RXE_i      9
#define PL011_DTR_i      10
#define PL011_RTS_i      11
#define PL011_OUT1_i     12
#define PL011_OUT2_i     13
#define PL011_RTSEN_i    14
#define PL011_CTSEN_i    15

//uartlcr_h
#define PL011_BRK_i      0
#define PL011_PEN_i      1
#define PL011_EPS_i      2
#define PL011_STP2_i     3
#define PL011_FEN_i      4
#define PL011_WLEN_i     5
#define PL011_SPS_i      7

//uartfr
#define PL011_CTS_i      0
#define PL011_DSR_i      1
#define PL011_DCD_i      2
#define PL011_BUSY_i     3
#define PL011_RXFE_i     4
#define PL011_TXFF_i     5
#define PL011_RXFF_i     6
#define PL011_TXFE_i     7
#define PL011_RI_i       8

//uartifsel
#define PL011_RXIFSEL_i  3
#define PL011_TXIFSEL_i  0

//uartimsc
#define PL011_TXIM_i    5

//uartmis
#define PL011_TXMIS_i    5

//uartris
#define PL011_TXRIS_i    5

//uarticr
#define PL011_TXIC_i    5

//PL011IFLS
#define PL011_TXIFLSEL_i   0

uint8_t uart_tx_fifo_level_to_bytes(uint32_t base_addr)
{
    //    switch ((read_PL011_UARTIFLS(base_addr) & (0b111 << PL011_TXIFLSEL_i)) >> PL011_TXIFLSEL_i)
    switch ((ioread32(base_addr + PL011_UARTIFLS) & (0b111 << PL011_TXIFLSEL_i)) >> PL011_TXIFLSEL_i)
    {
    case 0b000: return PL011_FIFO_SIZE - PL011_FIFO_SIZE/8;
    case 0b001: return PL011_FIFO_SIZE - PL011_FIFO_SIZE/4;
    case 0b010: return PL011_FIFO_SIZE - PL011_FIFO_SIZE/2;
    case 0b011: return PL011_FIFO_SIZE - 3 * PL011_FIFO_SIZE/4;
    case 0b100: return PL011_FIFO_SIZE - 7 * PL011_FIFO_SIZE/8;
    default: return 0;
    }
}


static uint8_t uart_wait_tx_fifo_ready(uint32_t base_addr)
{
//    uint16_t fr = read_PL011_UARTFR(base_addr);
    uint16_t fr = ioread32(base_addr + PL011_UARTFR);
    if (fr & (1 << PL011_TXFE_i))
        return PL011_FIFO_SIZE;

    //check raw status
//    if (read_PL011_UARTRIS(base_addr) & (1 << PL011_TXRIS_i))
        if (ioread32(base_addr + PL011_UARTRIS) & (1 << PL011_TXRIS_i))
        return uart_tx_fifo_level_to_bytes(base_addr);

    if (!(fr & (1 << PL011_TXFF_i)))
        return 1; //FIFO is not full, i.e. at least 1 byte can be pushed

    do
    {
        if (!(ioread32(base_addr + PL011_UARTFR) & (1 << PL011_TXFF_i)))
            return 1; //FIFO is not full, i.e. at least 1 byte can be pushed
    }
    while(1);

    //timeout
    return 0;
}

static void uart_write_char(uint32_t base_addr, unsigned char c)
{

	uint8_t size = uart_wait_tx_fifo_ready(base_addr);
	if (0 != size)
		mpw7705_writeb(c, base_addr + PL011_UARTDR);
}


static void serial_trace_char(char c)
{
	if (c == '\n')
		serial_trace_char('\r');
	uart_write_char(UART0__, c);
}

void rumboot_putstring_real_serial(const char *str, int len)
{
	while (len--)
	   serial_trace_char(*str++);
}

void rumboot_putstring_real(const char *str, int len)
{
    rumboot_putstring_real_serial(str, len);
}

void rumboot_putstring(const char *str)
{
	while (*str)
	   serial_trace_char(*str++);
}

int mpw7705_serial_init(void)
{
    return 0;
}

static struct serial_device mpw7705_serial_drv = {
	.name	= "mpw7705_serial",
	.start	= mpw7705_serial_init,
	.stop	= NULL,
	.setbrg	= NULL,
	.putc	= serial_trace_char,
	.puts	= rumboot_putstring,
	.getc	= NULL,
	.tstc	= NULL,
    .next   = NULL
};

struct serial_device *default_serial_console(void)
{
	return &mpw7705_serial_drv;
}

static void _debug_uart_init(void)
{
}

static inline void _debug_uart_putc(int ch)
{
	uart_write_char(UART0__, ch);
}

DEBUG_UART_FUNCS

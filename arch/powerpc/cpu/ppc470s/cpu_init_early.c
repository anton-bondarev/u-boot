#include <common.h>
#include <asm/io.h>



static enum err_code {
  OK       = 0,
  NOT_WAIT_TX = -1,
  NOT_WAIT_RX = -2,
};

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

//uartris
#define PL011_TXRIS_i    5
#define PL011_RXRIS_i    4

#define TIMEOUT 100

#define UART0__                 0x3C05D000
#define TRACE_UART  UART0__


static uint32_t ioread32(uint32_t const base_addr)
{
    return *((volatile uint32_t*)(base_addr));
}

static iowrite32(uint32_t const value, uint32_t const base_addr)
{
    *((volatile uint32_t*)(base_addr)) = value;
}

static enum err_code tx_fifo_ready(uint32_t base_addr)
{
  if (ioread32(base_addr + PL011_UARTFR) & (1 << PL011_TXFE_i))
  return OK;

  if (ioread32(base_addr + PL011_UARTRIS) & (1 << PL011_TXRIS_i))
  return OK;

  uint32_t start = ucurrent();
  do{
    if (!(ioread32(base_addr + PL011_UARTFR) & (1 << PL011_TXFF_i)))
    return OK; //FIFO is not full, i.e. at least 1 byte can be pushed
  }
  while(ucurrent()-start < TIMEOUT);

  return NOT_WAIT_TX;
}


static void uart_write_char(uint32_t base_addr, unsigned char c, int* err_code)
{
  *err_code = tx_fifo_ready(base_addr);
  if ((*err_code) == OK)
  iowrite32((char)c, base_addr + PL011_UARTDR);
}


static void serial_trace_char(char c)
{
  if (c == '\n')
  serial_trace_char('\r');
  
  int err_code;
  uart_write_char(TRACE_UART, c, &err_code);
}

void rumboot_putstring(const char *str)
{
  while (*str)
  serial_trace_char(*str++);
}

// todo if needed
void cpu_init_early_f(void *fdt)
{

}
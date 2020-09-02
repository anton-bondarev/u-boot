#ifndef FLW_SERIAL_H
#define FLW_SERIAL_H

#define  FLW_TOUT 1000000

int flw_putc(char ch);

int flw_getc(unsigned long tout);

static inline void flw_delay(unsigned long value)
{
    unsigned long i;
    for (i=0; i < value; i++) asm("nop\n");
}

#endif // FLW_SERIAL_H
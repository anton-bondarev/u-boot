#ifndef FLW_SERIAL_H
#define FLW_SERIAL_H

#define  FLW_TOUT 1000000

int flw_putc(char ch);

int flw_getc(unsigned long tout);

void flw_delay(unsigned long value);

#endif // FLW_SERIAL_H
#ifndef FLW_SERIAL_H
#define FLW_SERIAL_H

#ifdef CONFIG_TARGET_1888TX018
    #define  FLW_TOUT 1000000
#endif

#ifdef CONFIG_TARGET_1888BM18
    #define  FLW_TOUT 1000
#endif

int flw_putc(char ch);

int flw_getc(unsigned long tout);

void flw_delay(unsigned long value);

#endif // FLW_SERIAL_H
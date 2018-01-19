#include <stdint.h>
#include "timer.h"

void usleep(uint32_t usec)
{
	uint32_t counts;
	counts = usec * TIMER_FREQ_MHZ;
	SPR_DEC_write(counts);
	while (SPR_DEC_read()){}
}


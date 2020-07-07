#include <common.h>

unsigned long get_tbclk(void)
{
	return TIMER_TICKS_PER_US * 1000000;
}

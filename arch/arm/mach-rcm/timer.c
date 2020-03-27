

#include <common.h>
#include <asm/io.h>
#include <asm/arch-armv7/systimer.h>

#define TIMER_BASE	0x000CD000

static struct systimer *systimer_base = (struct systimer *)TIMER_BASE;

// Value was measured experimentally. If someone find right value - fix it!
#define TIMER_1US_VAL	200

#define TIMER_START_VAL	0xFFFFFFF0
#define TIMER_US_MAX	53687091

#define TIMER_CTRL_ONESHOT	1
#define TIMER_CTRL_32BIT	(1 << 1)
#define TIMER_CTRL_DIV1	(0 << 2)
#define TIMER_CTRL_DIV16	(1 << 2)
#define TIMER_CTRL_DIV256	(2 << 2)
#define TIMER_CTRL_IE	(1 << 5)
#define TIMER_CTRL_PERIODIC	(1 << 6)
#define TIMER_CTRL_ENABLE	(1 << 7)

int timer_init (void)
{
	u32 ctrl;

	/*
	 * Initialise to a known state (all timers off)
	 */
	ctrl = 0;
	writel(ctrl, &(systimer_base->timer0control));
	writel(ctrl, &(systimer_base->timer1control));

	ctrl = (TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC | TIMER_CTRL_32BIT);

	writel(TIMER_START_VAL, &(systimer_base->timer1bgload));
	writel(TIMER_START_VAL, &(systimer_base->timer1load));
	writel(ctrl, &(systimer_base->timer1control));
	return 0;
}

// Timer counts on the order of 50s.
uint64_t get_timer_us(uint64_t base)
{
	ulong cur_val, delta;

	if (base > TIMER_US_MAX)
		base = TIMER_US_MAX;

	base *= TIMER_1US_VAL;
	cur_val = (ulong) TIMER_START_VAL - readl(&(systimer_base->timer1value));

	if (cur_val >= base) {
		delta = cur_val - base;
	}
	else {
		delta = (ulong)TIMER_START_VAL - (base - cur_val);
	}

	delta /= TIMER_1US_VAL;

	return delta;
}

ulong get_timer(ulong base)
{
	ulong tm;

	tm = TIMER_US_MAX / (1000000 /  CONFIG_SYS_HZ);

	if (base > tm)
		base = TIMER_US_MAX;
	else
		base *= (1000000 / CONFIG_SYS_HZ);

	tm = get_timer_us(base);

	tm /= (1000000 / CONFIG_SYS_HZ);

	return tm;
}

unsigned long long get_ticks(void)
{
	return get_timer(0);
}

ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}

void __udelay(unsigned long usec)
{
	ulong now;

	now = get_timer_us(0);

	while (get_timer_us(now) < usec) {};
}

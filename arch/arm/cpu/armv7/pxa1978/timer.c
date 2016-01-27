/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa1978.h>

#define TIMER			0	/* Use TIMER 0 */

/* Each timer has 3 match registers */
#define MATCH_CMP(x)		((3 * TIMER) + x)
#define TIMER_LOAD_VAL		0xffffffff
#define	COUNT_RD_REQ		0x1

DECLARE_GLOBAL_DATA_PTR;
/* Using gd->arch.tbu from timestamp and gd->arch.tbl for lastdec */

/*
 * For preventing risk of instability in reading counter value,
 * first set read request to register cvwr and then read same
 * register after it captures counter value.
 */
ulong read_timer(void)
{
	struct pxa1978tmr_registers *timers =
		(struct pxa1978tmr_registers *)PXA1978_TMR1_BASE;
	int loop = 100;
	ulong val;

	writel(COUNT_RD_REQ, &timers->cvwr);
	while (loop--)
		val = readl(&timers->cvwr);

	/*
	 * This stop gcc complain and prevent loop mistake init to 0
	 */
	val = readl(&timers->cvwr);
	return val;
}

ulong get_timer_masked(void)
{
	ulong now = read_timer();

	if (now >= gd->arch.tbl) {
		/* normal mode */
		gd->arch.tbu += now - gd->arch.tbl;
	} else {
		/* we have an overflow ... */
		gd->arch.tbu += now + TIMER_LOAD_VAL - gd->arch.tbl;
	}
	gd->arch.tbl = now;

	return gd->arch.tbu;
}

ulong get_timer(ulong base)
{
	return (get_timer_masked() / (CONFIG_SYS_HZ_CLOCK / 1000)) - base;
}

void __udelay(unsigned long usec)
{
	ulong delayticks;
	ulong endtime;

	delayticks = (usec * (CONFIG_SYS_HZ_CLOCK / 1000000));
	endtime = get_timer_masked() + delayticks;

	while (get_timer_masked() < endtime)
		;
}

/*
 * init the Timer
 */
int timer_init(void)
{
	struct pxa1978tmr_registers *timers =
		(struct pxa1978tmr_registers *)PXA1978_TMR1_BASE;

	/* load value into timer */
	writel(0x0, &timers->clk_ctrl);
	/* Use Timer 0 Match Resiger 0 */
	writel(TIMER_LOAD_VAL, &timers->match[MATCH_CMP(0)]);
	/* Preload value is 0 */
	writel(0x0, &timers->preload[TIMER]);
	/* Enable match comparator 0 for Timer 0 */
	writel(0x1, &timers->preload_ctrl[TIMER]);

	/* Enable timer 0 */
	writel(0x1, &timers->cer);

	/* init the gd->arch.tbu and gd->arch.tbl value */
	gd->arch.tbl = read_timer();
	gd->arch.tbu = 0;

	return 0;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return (ulong)CONFIG_SYS_HZ_CLOCK;
}

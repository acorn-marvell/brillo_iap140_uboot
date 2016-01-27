/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <serial.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa1978.h>

#define UARTCLK_PLL1DIV27	(1<<24)
#define UARTCLK_PLL1DIV108	(0<<24)
#define UARTCLK14745KHZ	(APBC_RST | APBC_APBCLK | APBC_FNCLK \
			| UARTCLK_PLL1DIV108)
#define TIMERCLK_26MHZ (APBC_RST | APBC_APBCLK | APBC_FNCLK \
			| APBC_FNCLKSEL(ACCUCLK_VCXO))
int arch_cpu_init(void)
{
	struct pxa1978accu_registers *accu =
		(struct pxa1978accu_registers *)PXA1978_ACCU_BASE;

	writel(UARTCLK14745KHZ, &accu->uart1);

	/* Set timer clock CONFIG_SYS_HZ_CLOCK = 26MHz */
	writel(TIMERCLK_26MHZ, &accu->timer1);

#ifdef CONFIG_I2C_MV
	/* Enable I2C clock */
	/* TODO */
#endif

#ifdef CONFIG_MMP_DISP
	/* TODO */
#endif

#ifdef CONFIG_MV_SDHCI
	/* Enable mmc clock */
	/* TODO */
#endif

#ifdef CONFIG_MV_UDC
	/* Enable usb clock */
	/* TODO */
#endif

	icache_enable();

	return 0;
}

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	u32 id;

	id = readl(PXA1978_PRODUCT_ID);
	printf("SoC:   PXA1978 %X-%X\n", (id & 0xFFFF), (id >> 0x10));
	return 0;
}
#endif

#ifdef CONFIG_I2C_MV
void i2c_clk_enable(void)
{
}
#endif

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#ifdef CONFIG_PXA_AMP_SUPPORT
void pxa_cpu_reset(int cpu, u32 addr)
{
	if (!cpu || cpu >= CONFIG_NR_CPUS) {
		printf("Wrong CPU number. Should be 1 to %d\n",
		       CONFIG_NR_CPUS - 1);
		return;
	}
	/* TODO: release the core */
}
#endif

#define WHITNEY_L2_CFG 0xe0010000
#define WHITNEY_L2_CLEANALL (WHITNEY_L2_CFG + 0x784)
/*
 * While SL2C is architecturally visible, reflected in CLIDR/CCSIDR
 * cp15 regs and therefore common ARMv7 clean by set/way is supposed to
 * be effective, it is NOT (does not evict dirty lines).
 * Therefore, added clean using the implementation specific register.
 */
void v7_outer_cache_flush_all(void)
{
	__raw_writel(1, WHITNEY_L2_CLEANALL);
	while (__raw_readl(WHITNEY_L2_CLEANALL) & 1)
		;
}

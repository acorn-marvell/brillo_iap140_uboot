/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Jane Li <jiel@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/features.h>
#include <asm/processor.h>
#include <power/pxa_common.h>
DECLARE_GLOBAL_DATA_PTR;

#define AXI_PHYS_BASE		0xd4200000
#define APB_PHYS_BASE		0xd4000000
#define APBC_PHYS_BASE		(APB_PHYS_BASE + 0x015000)
#define APBC_REG(x)             (APBC_PHYS_BASE + (x))

#define APMU_PHYS_BASE		(AXI_PHYS_BASE + 0x82800)
#define APMU_REG(x)		(APMU_PHYS_BASE + (x))
#define APMU_MC_HW_SLP_TYPE     APMU_REG(0x00b0)

#define PMU_CORE_STATUS			APMU_REG(0x0090)
#define PMU_CC2_AP			APMU_REG(0x0100)
#define PMU_CORE0_IDLE_CFG              APMU_REG(0x0124)
#define PMU_CORE1_IDLE_CFG              APMU_REG(0x0128)
#define PMU_CORE2_IDLE_CFG              APMU_REG(0x0160)
#define PMU_CORE3_IDLE_CFG              APMU_REG(0x0164)
#define PMU_CP_IDLE_CFG                 APMU_REG(0x014)
#define PMU_SP_IDLE_CFG                 APMU_REG(0x0f8)

#define PMU_MP_IDLE_CFG0                APMU_REG(0x0120)
#define PMU_MP_IDLE_CFG1                APMU_REG(0x00e4)
#define PMU_MP_IDLE_CFG2                APMU_REG(0x0150)
#define PMU_MP_IDLE_CFG3                APMU_REG(0x0154)

#define PMU_DEBUG_REG			APMU_REG(0x0088)

#define ICU_PHYS_BASE	(AXI_PHYS_BASE + 0x82000)
#define ICU_REG(x)	(ICU_PHYS_BASE + (x))


#define PXA1088_ICU_APC0_GBL_INT_MSK    ICU_REG(0x228)
#define PXA1088_ICU_APC1_GBL_INT_MSK    ICU_REG(0x238)
#define PXA1088_ICU_APC2_GBL_INT_MSK    ICU_REG(0x248)
#define PXA1088_ICU_APC3_GBL_INT_MSK    ICU_REG(0x258)
#define PXA988_ICU_A9C0_GBL_INT_MSK	ICU_REG(0x114)
#define PXA988_ICU_A9C1_GBL_INT_MSK	ICU_REG(0x144)

#define CIU_PHYS_BASE		(AXI_PHYS_BASE + 0x82c00)
#define CIU_REG(x)		(CIU_PHYS_BASE + (x))
#define CIU_CA9_WARM_RESET_VECTOR	CIU_REG(0x00d8)
#define CIU_PJ4MP1_PDWN_CFG_CTL		CIU_REG(0x007c)

#define MPMU_PHYS_BASE		(APB_PHYS_BASE + 0x50000)
#define MPMU_REG(off)		(MPMU_PHYS_BASE + (off))
#define MPMU_AWUCRM		MPMU_REG(0x104c)
#define MPMU_APCR		MPMU_REG(0x1000)
#define MPMU_CPCR		MPMU_REG(0x0000)
#define MPMU_SCCR		MPMU_REG(0x0038)

#define APBC_TIMERS0		APBC_REG(0x034)
#define APBC_TIMERS1		APBC_REG(0x044)
#define APBC_TIMERS2		APBC_REG(0x068)

#define TIMERS_PHYS_BASE_0	(APB_PHYS_BASE + 0x14000)
#define TIMERS_PHYS_BASE_1	(APB_PHYS_BASE + 0x16000)
#define TIMERS_PHYS_BASE_2	(APB_PHYS_BASE + 0x1F000)

#define TMR_CCR			(0x0000)
#define TMR_TN_MM(n, m)		(0x0004 + ((n) << 3) + (((n) + (m)) << 2))
#define TMR_CR(n)		(0x0028 + ((n) << 2))
#define TMR_SR(n)		(0x0034 + ((n) << 2))
#define TMR_IER(n)		(0x0040 + ((n) << 2))
#define TMR_PLVR(n)		(0x004c + ((n) << 2))
#define TMR_PLCR(n)		(0x0058 + ((n) << 2))
#define TMR_WMER		(0x0064)
#define TMR_WMR			(0x0068)
#define TMR_WVR			(0x006c)
#define TMR_WSR			(0x0070)
#define TMR_ICR(n)		(0x0074 + ((n) << 2))
#define TMR_WICR		(0x0080)
#define TMR_CER			(0x0084)
#define TMR_CMR			(0x0088)
#define TMR_ILR(n)		(0x008c + ((n) << 2))
#define TMR_WCR			(0x0098)
#define TMR_WFAR		(0x009c)
#define TMR_WSAR		(0x00A0)
#define TMR_CVWR(n)		(0x00A4 + ((n) << 2))

#define TMR_CCR_CS_0(x)		(((x) & 0x3) << 0)
#define TMR_CCR_CS_1(x)		(((x) & 0x3) << 2)
#define TMR_CCR_CS_2(x)		(((x) & 0x3) << 5)

#ifdef CONFIG_PXA1088
#define PGU_PHYS_BASE		0xd1df8000
#endif

#define GIC_DIST_PHYS_BASE	(PGU_PHYS_BASE + 0x1000)
#define GIC_CPU_PHYS_BASE	(PGU_PHYS_BASE + 0x0100)

#define GIC_CPU_CTRL		0x00
#define GIC_CPU_PRIMASK		0x04
#define GIC_CPU_BINPOINT	0x08
#define GIC_CPU_INTACK		0x0c
#define GIC_CPU_EOI		0x10
#define GIC_CPU_RUNNINGPRI	0x14
#define GIC_CPU_HIGHPRI		0x18

#define GIC_DIST_CTRL		0x000
#define GIC_DIST_CTR		0x004
#define GIC_DIST_ENABLE_SET	0x100
#define GIC_DIST_ENABLE_CLEAR	0x180
#define GIC_DIST_PENDING_SET	0x200
#define GIC_DIST_PENDING_CLEAR	0x280
#define GIC_DIST_ACTIVE_BIT	0x300
#define GIC_DIST_PRI		0x400
#define GIC_DIST_TARGET		0x800
#define GIC_DIST_CONFIG		0xc00
#define GIC_DIST_SOFTINT	0xf00

#define PMUA_CORE_IDLE			(1 << 0)
#define PMUA_CORE_POWER_DOWN		(1 << 1)
#define PMUA_CORE_L1_SRAM_POWER_DOWN	(1 << 2)
#define PMUA_GIC_IRQ_GLOBAL_MASK	(1 << 3)
#define PMUA_GIC_FIQ_GLOBAL_MASK	(1 << 4)

#define PMUA_MP_IDLE			(1 << 0)
#define PMUA_MP_POWER_DOWN		(1 << 1)
#define PMUA_MP_L2_SRAM_POWER_DOWN	(1 << 2)
#define PMUA_MP_SCU_SRAM_POWER_DOWN	(1 << 3)

#define ICU_MASK_FIQ			(1 << 0)
#define ICU_MASK_IRQ			(1 << 1)

#define PMUM_AXISD		(1 << 31)
#define PMUM_DSPSD		(1 << 30)
#define PMUM_SLPEN		(1 << 29)
#define PMUM_DTCMSD		(1 << 28)
#define PMUM_DDRCORSD		(1 << 27)
#define PMUM_APBSD		(1 << 26)
#define PMUM_BBSD		(1 << 25)
#define PMUM_INTCLR		(1 << 24)
#define PMUM_SLPWP0		(1 << 23)
#define PMUM_SLPWP1		(1 << 22)
#define PMUM_SLPWP2		(1 << 21)
#define PMUM_SLPWP3		(1 << 20)
#define PMUM_VCTCXOSD		(1 << 19)
#define PMUM_SLPWP4		(1 << 18)
#define PMUM_SLPWP5		(1 << 17)
#define PMUM_SLPWP6		(1 << 16)
#define PMUM_SLPWP7		(1 << 15)
#define PMUM_MSASLPEN		(1 << 14)
#define PMUM_STBYEN		(1 << 13)
#define PMUM_SPDTCMSD		(1 << 12)
#define PMUM_LDMA_MASK		(1 << 3)

#define PMUM_GSM_WAKEUPWMX	(1 << 29)
#define PMUM_WCDMA_WAKEUPX	(1 << 28)
#define PMUM_GSM_WAKEUPWM	(1 << 27)
#define PMUM_WCDMA_WAKEUPWM	(1 << 26)
#define PMUM_AP_ASYNC_INT	(1 << 25)
#define PMUM_AP_FULL_IDLE	(1 << 24)
#define PMUM_SQU		(1 << 23)
#define PMUM_SDH		(1 << 22)
#define PMUM_KEYPRESS		(1 << 21)
#define PMUM_TRACKBALL		(1 << 20)
#define PMUM_NEWROTARY		(1 << 19)
#define PMUM_WDT		(1 << 18)
#define PMUM_RTC_ALARM		(1 << 17)
#define PMUM_CP_TIMER_3		(1 << 16)
#define PMUM_CP_TIMER_2		(1 << 15)
#define PMUM_CP_TIMER_1		(1 << 14)
/*
 * For Helan2 and HelanLTE, AP_TIMER0 and
 * AP_TIMER2 shares the same wakeup bits
 * in AWUCRM.
 */
#define PMUM_AP1_TIMER_3	(1 << 13)
#define PMUM_AP1_TIMER_2	(1 << 12)
#define PMUM_AP1_TIMER_1	(1 << 11)
#define PMUM_AP0_2_TIMER_3	(1 << 10)
#define PMUM_AP0_2_TIMER_2	(1 << 9)
#define PMUM_AP0_2_TIMER_1	(1 << 8)
#define PMUM_WAKEUP7		(1 << 7)
#define PMUM_WAKEUP6		(1 << 6)
#define PMUM_WAKEUP5		(1 << 5)
#define PMUM_WAKEUP4		(1 << 4)
#define PMUM_WAKEUP3		(1 << 3)
#define PMUM_WAKEUP2		(1 << 2)
#define PMUM_WAKEUP1		(1 << 1)
#define PMUM_WAKEUP0		(1 << 0)

#define CPU_MASK(n)		(1 << n)
#define CORE0_1_WAKEUP		(PMUM_AP1_TIMER_1 | PMUM_AP1_TIMER_2 | \
				PMUM_AP1_TIMER_3)
#define CORE2_3_WAKEUP		(PMUM_AP0_2_TIMER_1 | PMUM_AP0_2_TIMER_2 | \
				PMUM_AP0_2_TIMER_3)

#ifdef CONFIG_PXA1088
#define LPM_WAKEUP		(CORE0_1_WAKEUP | CORE2_3_WAKEUP | PMUM_WAKEUP4)
#endif

/*Core Active means: Core C2/WFI Flag Set*/
#ifdef CONFIG_PXA1088
#define C2_BIT(n)		(7 + 3 * n)
#define C1_BIT(n)		(6 + 3 * n)
#define WFI_BIT(n)		(5 + 3 * n)

#define DBG_BIT(n)		(18 + 3 * n)
#define SW_BIT(n)		(17 + 3 * n)
#define POR_BIT(n)		(16 + 3 * n)

#define CORE_N_DBG(n)		(1 << (DBG_BIT(n)))
#define CORE_N_SW(n)		(1 << (SW_BIT(n)))
#define CORE_N_POR(n)		(1 << (POR_BIT(n)))
#define CORE_N_RESET(n)		((CORE_N_POR(n)) | (CORE_N_SW(n)) | \
				 (CORE_N_DBG(n)))
#endif
#define CORE_N_C2(n)		(1 << (C2_BIT(n)))
#define CORE_N_C1(n)		(1 << (C1_BIT(n)))
#define CORE_N_WFI(n)		(1 << (WFI_BIT(n)))
#define CORE_N_SLEEP(n)		((CORE_N_C2(n)) | (CORE_N_C1(n)) | \
				 (CORE_N_WFI(n)))

/*This is for pxa988 Zx only*/
#define IDLE_BIT(n)		(3 << (2 + 2 * n))

enum pxa988_lowpower_state {
	POWER_MODE_ACTIVE = 0,			/* not used */
	/* core level power modes */
	POWER_MODE_CORE_INTIDLE,		/* C12 */
	POWER_MODE_CORE_EXTIDLE,		/* C13 */
	POWER_MODE_CORE_PWRDOWN_L1_ON,		/* C22P, only for PXA988*/
	POWER_MODE_CORE_PWRDOWN,		/* C22 */
	/* MP level power modes */
	POWER_MODE_MP_IDLE_CORE_EXTIDLE,	/* M11_C13 */
	POWER_MODE_MP_IDLE_CORE_PWRDOWN_L1_ON,	/* M11_C22P, only for PXA988 */
	POWER_MODE_MP_IDLE_CORE_PWRDOWN,	/* M11_C22 */
	POWER_MODE_MP_PWRDOWN_SCU_ON,		/* M21P, only for PXA988 */
	POWER_MODE_MP_PWRDOWN,			/* M21 */
	POWER_MODE_MP_PWRDOWN_L2_OFF,			/* M22 */
	/* AP subsystem level power modes */
	POWER_MODE_APPS_IDLE,			/* D1P */
	POWER_MODE_APPS_IDLE_DDR,		/* D1PDDR */
	POWER_MODE_APPS_SLEEP,			/* D1PP */
	POWER_MODE_APPS_SLEEP_UDR,		/* D1PPSTDBY */
	/* chip level power modes*/
	POWER_MODE_SYS_SLEEP,			/* D1 */
	POWER_MODE_UDR_VCTCXO_ON_L2_ON,		/* D2 */
	POWER_MODE_UDR_VCTCXO_OFF_L2_ON,	/* D2P */
	POWER_MODE_UDR_VCTCXO_OFF_L2_OFF,	/* D2PP */
};

static inline void core_exit_coherency(void)
{
	unsigned int v;
	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	bic	%0, %0, #(1 << 6)\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	.word	0xf57ff06f\n"/* isb */
	: "=&r" (v) : : "cc");
}

static inline void disable_l1_cache(void)
{
	unsigned int v;
	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 0\n"
	"	bic	%0, %0, %1\n"
	"	mcr	p15, 0, %0, c1, c0, 0\n"
	"	.word	0xf57ff06f\n"/* isb */
	: "=&r" (v) : "Ir" (CR_C) : "cc");
}

static inline void core_enter_coherency(void)
{
	unsigned int v;
	asm volatile(
	"	mrc	p15, 0, %0, c1, c0, 1\n"
	"	orr	%0, %0, #(1 << 6)\n"
	"	mcr	p15, 0, %0, c1, c0, 1\n"
	"	.word	0xf57ff06f\n"/* isb */
	: "=&r" (v) : : "cc");
}

static inline void enable_l1_cache(void)
{
	unsigned int v;
	asm volatile(
	"	mrc     p15, 0, %0, c1, c0, 0\n"
	"	orr     %0, %0, %1\n"
	"	mcr     p15, 0, %0, c1, c0, 0\n"
	"	.word	0xf57ff06f\n"/* isb */
	: "=&r" (v) : "Ir" (CR_C) : "cc");
}


#define hard_smp_processor_id()			\
	({						\
		unsigned int cpunum;			\
		__asm__("mrc p15, 0, %0, c0, c0, 5"	\
			: "=r" (cpunum));		\
		cpunum &= 0x0F;				\
	})

static inline void uartsw_to_active_core(void)
{
#ifdef CONFIG_PXA_AMP_SUPPORT
	int cpu, cur_cpu = hard_smp_processor_id();
	char cmd[20];
	unsigned int core_active_status, core_active_tmp_status;
	unsigned int core_reset_status, core_reset_tmp_status;

	core_active_status = __raw_readl(PMU_CORE_STATUS);
	core_reset_status = __raw_readl(PMU_CC2_AP);
	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		if (cur_cpu == cpu)
			continue;

		if (*(u32 *)CONFIG_CORE_BUSY_ADDR & (1 << cpu))
			continue;

		if (has_feat_legacy_apmu_core_status())
			core_active_tmp_status =
				core_active_status & IDLE_BIT(cpu);
		else
			core_active_tmp_status =
				core_active_status & CORE_N_SLEEP(cpu);

		core_reset_tmp_status = core_reset_status & CORE_N_RESET(cpu);

		if (!core_active_tmp_status && !core_reset_tmp_status) {
			printf("console switch from: %d to %d\n", cur_cpu, cpu);
			sprintf(cmd, "uartsw %d", cpu);
			run_command(cmd, 0);
			break;
		}
	}
	printf("UART NOT Switch, the CPU%d is the last man\n", cur_cpu);
#endif
}

static inline void core_wfi(void)
{
	uartsw_to_active_core();
	/* wfi */
	__asm__ __volatile__ (".word    0xe320f003");
}

static inline void core_pwr_down_fallback(void)
{
	core_enter_coherency();
	enable_l1_cache();
}

static inline void core_pwr_down(void)
{
	if (cpu_is_pxa988() || cpu_is_pxa986()) {
		flush_dcache_all();
		/* dsb */
		__asm__ __volatile__ (".word    0xf57ff04f");
		invalidate_icache_all();
	} else {
		disable_l1_cache();
		flush_dcache_all();
		/* clrex */
		__asm__ __volatile__ (".word    0xf57ff01f");
		core_exit_coherency();
		/* dsb */
		__asm__ __volatile__ (".word    0xf57ff04f");
	}

	core_wfi();

	if ((cpu_is_pxa1088() || cpu_is_pxa1L88() || cpu_is_pxa1U88())) {
		/* The fall back shouldn't be run in normal cases */
		printf("C2 Core Power Down fall back!");
		core_pwr_down_fallback();
	}
}

static void __attribute__ ((unused)) gic_dist_init(int base)
{
	unsigned int max_irq, i;
	/* Hard coding the cpumask as CPU0 */
	int cpumask = 1;

	cpumask |= cpumask << 8;
	cpumask |= cpumask << 16;

	writel(0, base + GIC_DIST_CTRL);

	/*
	 * Find out how many interrupts are supported.
	 */
	max_irq = readl(base + GIC_DIST_CTR) & 0x1f;
	max_irq = (max_irq + 1) * 32;

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < max_irq; i += 16)
		writel(0, base + GIC_DIST_CONFIG + i * 4 / 16);

	/*
	 * Set all global interrupts to this CPU only.
	 */
	for (i = 32; i < max_irq; i += 4)
		writel(cpumask, base + GIC_DIST_TARGET + i * 4 / 4);

	/* irq 13 route to cpu0, irq 14 route to cpu1 */
	writel((0x1 << 8) | (0x2 << 16),
	       (base + GIC_DIST_TARGET + (32 + 12) * 4 / 4));

	/*
	 * Set priority on all interrupts.
	 */
	for (i = 0; i < max_irq; i += 4)
		writel(0xa0a0a0a0, base + GIC_DIST_PRI + i * 4 / 4);

	/*
	 * Disable all interrupts.
	 */
	for (i = 0; i < max_irq; i += 32)
		writel(0xffffffff, base + GIC_DIST_ENABLE_CLEAR + i * 4 / 32);

	writel(1, base + GIC_DIST_CTRL);
}

static void __attribute__ ((unused)) gic_cpu_init(int base)
{
	writel(0xf0, base + GIC_CPU_PRIMASK);
	writel(1, base + GIC_CPU_CTRL);
}

static void __attribute__ ((unused)) gic_init(void)
{
	int irq;
	gic_dist_init(GIC_DIST_PHYS_BASE);
	gic_cpu_init(GIC_CPU_PHYS_BASE);

	/* enable AP timer0 and timer1 interrupt */
	irq = 32 + 13;
	writel((1 << (irq % 32)), GIC_DIST_PHYS_BASE + GIC_DIST_ENABLE_SET +
		(irq / 32) * 4);
	irq = 32 + 14;
	writel((1 << (irq % 32)), GIC_DIST_PHYS_BASE + GIC_DIST_ENABLE_SET +
		(irq / 32) * 4);
}

static void icu_enable_timer0_1(void)
{
	/* Timer 1_0 route to CPU 0 */
	__raw_writel(0x1 | (0x1 << 6) | (0x1 << 4), ICU_REG(29 << 2));
	/* Timer 1_1 route to CPU 1 */
	__raw_writel(0x1 | (0x1 << 7) | (0x1 << 4), ICU_REG(30 << 2));
	/*
	 * TIMER0 is security accessed only in PXA1088 and PXA1L88, but in
	 * PXA1U88 it changes to TIMER2.
	 */
	if (cpu_is_pxa1088() || cpu_is_pxa1L88()) {
		/* Timer 1_0 route to CPU 2 */
		__raw_writel(0x1 | (0x1 << 8) | (0x1 << 4), ICU_REG(64 << 2));
		/* Timer 1_1 route to CPU 3 */
		__raw_writel(0x1 | (0x1 << 9) | (0x1 << 4), ICU_REG(65 << 2));
	} else if (cpu_is_pxa1U88()) {
		/* Timer 0_0 route to CPU 2 */
		__raw_writel(0x1 | (0x1 << 8) | (0x1 << 4), ICU_REG(13 << 2));
		/* Timer 0_1 route to CPU 3 */
		__raw_writel(0x1 | (0x1 << 9) | (0x1 << 4), ICU_REG(14 << 2));
	}
}

static void timer_config(int cpu)
{
	unsigned long timer_phy_base = (cpu < 2) ? TIMERS_PHYS_BASE_1 :
			cpu_is_pxa1U88() ? TIMERS_PHYS_BASE_0 : TIMERS_PHYS_BASE_2;
	unsigned long timer_clk_base = (cpu < 2) ? APBC_TIMERS1 :
			cpu_is_pxa1U88() ? APBC_TIMERS0 : APBC_TIMERS2;
	/* Helan2 adds extal bit 7 to enable timer0 wakeup */
	unsigned long timer_clk_val = (cpu >= 2 && cpu_is_pxa1U88()) ?
			(0x1 << 7) | APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(3) :
			APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(3);
	unsigned long ccr = __raw_readl(timer_phy_base + TMR_CCR);
	unsigned long cer = __raw_readl(timer_phy_base + TMR_CER);
	unsigned long cmr = __raw_readl(timer_phy_base + TMR_CMR);
	unsigned long cur_clk_val = __raw_readl(timer_clk_base);

	if (cur_clk_val != timer_clk_val) {
		__raw_writel(APBC_APBCLK | APBC_RST, timer_clk_base);
		__raw_writel(timer_clk_val, timer_clk_base);
	}

	cpu = cpu % 2;

	/* disable Timer */
	cer &= ~CPU_MASK(cpu);
	__raw_writel(cer, timer_phy_base + TMR_CER);

	/* clock frequency from clock/reset control register for Timer 0 */
	if (!cpu) {
		ccr &= ~TMR_CCR_CS_0(3);
		ccr |= TMR_CCR_CS_0(1);		/* Timer 0 -- 32KHz */
	} else {
		ccr &= ~TMR_CCR_CS_1(3);
		ccr |= TMR_CCR_CS_1(1);         /* Timer 1 -- 32KHz */
	}

	__raw_writel(ccr, timer_phy_base + TMR_CCR);

	/* periodic mode */
	__raw_writel(cmr & ~CPU_MASK(cpu), timer_phy_base + TMR_CMR);

	/* All this is for Comparator 0 */
	/* clear status */
	__raw_writel(0x01, timer_phy_base + TMR_ICR(cpu));
	/* disable int */
	__raw_writel(0x00, timer_phy_base + TMR_IER(cpu));

	/* enable Timer */
	cer |= CPU_MASK(cpu);
	__raw_writel(cer, timer_phy_base + TMR_CER);
}

static void timer_set_match(int cpu, int seconds)
{
	unsigned long cer;
	unsigned long timer_phy_base = (cpu < 2) ? TIMERS_PHYS_BASE_1 :
			cpu_is_pxa1U88() ? TIMERS_PHYS_BASE_0 : TIMERS_PHYS_BASE_2;
	unsigned long ccr = __raw_readl(timer_phy_base + TMR_CCR);
	unsigned long ccr_saved = ccr;

	/* save the info that timer has been set to wakeup cpu */
	*(u32 *)(CONFIG_LPM_TIMER_WAKEUP) |= 1 << cpu;

	cpu = cpu % 2;

	/* disable APx_Timer */
	cer = __raw_readl(timer_phy_base + TMR_CER);
	cer &= ~CPU_MASK(cpu);
	__raw_writel(cer, timer_phy_base + TMR_CER);

	if (!cpu)
		ccr &= ~TMR_CCR_CS_0(3);
	else
		ccr &= ~TMR_CCR_CS_1(3);

	__raw_writel(ccr, timer_phy_base + TMR_CCR);

	/* clear pending interrupt status and enable */
	/* All this is for Comparator 0 */
	__raw_writel(0x01, timer_phy_base + TMR_ICR(cpu));
	__raw_writel(0x01, timer_phy_base + TMR_IER(cpu));

	/* We use 32k Timer here */
	__raw_writel(32768 * seconds, timer_phy_base + TMR_TN_MM(cpu, 0));

	__raw_writel(ccr_saved, timer_phy_base + TMR_CCR);

	/* enable APx_Timer */
	cer |= CPU_MASK(cpu);
	__raw_writel(cer, timer_phy_base + TMR_CER);
}

static void timer_clear_irq(int cpu)
{
	unsigned long timer_phy_base = (cpu < 2) ? TIMERS_PHYS_BASE_1 :
			cpu_is_pxa1U88() ? TIMERS_PHYS_BASE_0 : TIMERS_PHYS_BASE_2;

	/* clr the info that timer has been set to wakeup cpu */
	*(u32 *)(CONFIG_LPM_TIMER_WAKEUP) &= ~(1 << cpu);

	cpu = cpu % 2;

	/* disable and clear pending interrupt status */
	__raw_writel(0x0, timer_phy_base + TMR_IER(cpu));
	__raw_writel(0x1, timer_phy_base + TMR_ICR(cpu));
}

static const u32 APMU_CORE_IDLE_CFG[] = {
	(u32)PMU_CORE0_IDLE_CFG, (u32)PMU_CORE1_IDLE_CFG,
#ifdef CONFIG_PXA1088
	(u32)PMU_CORE2_IDLE_CFG, (u32)PMU_CORE3_IDLE_CFG,
#endif
};

static const u32 APMU_MP_IDLE_CFG[] = {
	(u32)PMU_MP_IDLE_CFG0, (u32)PMU_MP_IDLE_CFG1,
#ifdef CONFIG_PXA1088
	(u32)PMU_MP_IDLE_CFG2, (u32)PMU_MP_IDLE_CFG3,
#endif
};

static const u32 ICU_GBL_INT_MSK[] = {
#ifdef CONFIG_PXA1088
	(u32)PXA1088_ICU_APC0_GBL_INT_MSK, (u32)PXA1088_ICU_APC1_GBL_INT_MSK,
	(u32)PXA1088_ICU_APC2_GBL_INT_MSK, (u32)PXA1088_ICU_APC3_GBL_INT_MSK,
#endif
};

static void pxa988_lowpower_config(u32 cpu,
			u32 power_state, u32 lowpower_enable)
{
	u32 core_idle_cfg, mp_idle_cfg, apcr, mc_slp_type = 0, debug = 0;

	core_idle_cfg = __raw_readl(APMU_CORE_IDLE_CFG[cpu]);
	mp_idle_cfg = __raw_readl(APMU_MP_IDLE_CFG[cpu]);
	apcr = __raw_readl(MPMU_APCR);
	if (has_feat_debugreg_in_d1p() && !has_feat_d1p_hipwr()) {
		mc_slp_type = __raw_readl(APMU_MC_HW_SLP_TYPE);
		debug = __raw_readl(PMU_DEBUG_REG);
	}

	if (lowpower_enable) {
		switch (power_state) {
		case POWER_MODE_UDR_VCTCXO_OFF_L2_OFF:
			mp_idle_cfg |= PMUA_MP_L2_SRAM_POWER_DOWN;
			/* fall through */
		case POWER_MODE_UDR_VCTCXO_OFF_L2_ON:
			apcr |= PMUM_VCTCXOSD;
			/* fall through */
		case POWER_MODE_UDR_VCTCXO_ON_L2_ON:
			apcr |= PMUM_STBYEN;
			/* fall through */
		case POWER_MODE_SYS_SLEEP:
			apcr |= PMUM_APBSD;
			/*
			 * AP subsystem should shutdown before chip level
			 * power modes
			 */
			apcr |= PMUM_SLPEN | PMUM_DDRCORSD | PMUM_AXISD;
			if (cpu_is_pxa988() || cpu_is_pxa986()) {
				core_idle_cfg |= PMUA_CORE_L1_SRAM_POWER_DOWN;
				mp_idle_cfg |= PMUA_MP_SCU_SRAM_POWER_DOWN;
			}
			mp_idle_cfg |= PMUA_MP_POWER_DOWN | PMUA_MP_IDLE;
			core_idle_cfg |= PMUA_CORE_POWER_DOWN | PMUA_CORE_IDLE;
			break;

		/* AP SUB SYSTEM Power modes */
		case POWER_MODE_APPS_SLEEP_UDR:
			apcr |= PMUM_STBYEN;
			/* fall through */
		case POWER_MODE_APPS_SLEEP:
			apcr |= PMUM_SLPEN;
			/* fall through */
		case POWER_MODE_APPS_IDLE_DDR:
			if (!has_feat_debugreg_in_d1p())
				apcr |= PMUM_DDRCORSD;
			/* fall through */
		case POWER_MODE_APPS_IDLE:
			apcr |= PMUM_AXISD;
			/* MP should shutdown before AP subsystem power modes */
			/*
			 * FIXME: This is for PXA988 Z0, for A0 here we only
			 * need to vote PMUM_AXISD.
			 * Note that on Z0 we have to modify APMU_MC_HW_SLP_TYPE
			 * to change ddr sleep type from self-refresh to active
			 * power down. This makes ddr accessable in AP_IDLE.
			 * This is supposed to be fixed on A0.
			 */
			if (has_feat_debugreg_in_d1p() && !has_feat_d1p_hipwr())
				apcr |= PMUM_DDRCORSD;
			if (cpu_is_pxa988() || cpu_is_pxa986()) {
				core_idle_cfg |= PMUA_CORE_L1_SRAM_POWER_DOWN;
				mp_idle_cfg |= PMUA_MP_SCU_SRAM_POWER_DOWN;
			}
			mp_idle_cfg |= PMUA_MP_POWER_DOWN | PMUA_MP_IDLE;
			core_idle_cfg |= PMUA_CORE_POWER_DOWN | PMUA_CORE_IDLE;
			break;

		/* MP level Power modes */
		case POWER_MODE_MP_PWRDOWN_L2_OFF:
			mp_idle_cfg |= PMUA_MP_L2_SRAM_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_PWRDOWN:
			if (cpu_is_pxa988() || cpu_is_pxa986())
				mp_idle_cfg |= PMUA_MP_SCU_SRAM_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_PWRDOWN_SCU_ON:
			mp_idle_cfg |= PMUA_MP_POWER_DOWN;
		case POWER_MODE_MP_IDLE_CORE_PWRDOWN:
			if (cpu_is_pxa988() || cpu_is_pxa986())
				core_idle_cfg |= PMUA_CORE_L1_SRAM_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_IDLE_CORE_PWRDOWN_L1_ON:
			core_idle_cfg |= PMUA_CORE_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_IDLE_CORE_EXTIDLE:
			mp_idle_cfg |= PMUA_MP_IDLE;
			/* Core should be idle before MP level power modes */
			core_idle_cfg |= PMUA_CORE_IDLE;
			break;

		/* Core level Power Modes */
		case POWER_MODE_CORE_PWRDOWN:
			if (cpu_is_pxa988() || cpu_is_pxa986())
				core_idle_cfg |= PMUA_CORE_L1_SRAM_POWER_DOWN;
			/* fall through */
		case POWER_MODE_CORE_PWRDOWN_L1_ON:
			core_idle_cfg |= PMUA_CORE_POWER_DOWN;
			/* fall through */
		case POWER_MODE_CORE_EXTIDLE:
			core_idle_cfg |= PMUA_CORE_IDLE;
			/* fall through */
		case POWER_MODE_CORE_INTIDLE:
			break;
		default:
			printf("Invalid power state!\n");
		}

		/* FIXME: need to set APMU_DEBUG_REGISTER for DDR access */
		if (has_feat_debugreg_in_d1p() && !has_feat_d1p_hipwr() &&
		    (power_state == POWER_MODE_APPS_IDLE)) {
			mc_slp_type &= ~0x7;
			mc_slp_type |= 0x4;

			debug |= (1 << 14) | (1 << 23);
		}
	} else {
		core_idle_cfg &= ~(PMUA_CORE_IDLE | PMUA_CORE_POWER_DOWN);
		mp_idle_cfg &= ~(PMUA_MP_IDLE | PMUA_MP_POWER_DOWN |
				PMUA_MP_L2_SRAM_POWER_DOWN);
		if (cpu_is_pxa988() || cpu_is_pxa986()) {
			core_idle_cfg &= PMUA_CORE_L1_SRAM_POWER_DOWN;
			mp_idle_cfg &= PMUA_MP_SCU_SRAM_POWER_DOWN;
		}
		apcr &= ~(PMUM_DDRCORSD | PMUM_APBSD | PMUM_AXISD |
			PMUM_VCTCXOSD | PMUM_STBYEN | PMUM_SLPEN);
		if (has_feat_debugreg_in_d1p() && !has_feat_d1p_hipwr()) {
			mc_slp_type &= ~0x7;
			debug &= ~((1 << 14) | (1 << 23));
		}
	}

	/* set DSPSD, DTCMSD, BBSD, MSASLPEN */
	apcr |= PMUM_DTCMSD | PMUM_BBSD | PMUM_MSASLPEN;
	if (cpu_is_pxa988() || cpu_is_pxa986())
		apcr |= PMUM_DSPSD;
	if (cpu_is_pxa1U88())
		apcr |= PMUM_SPDTCMSD | PMUM_LDMA_MASK;
	/*
	 * FIXME: PXA920 was always setting SLEPEN bit but it seems no need
	 * to do that according to the power measurement.
	 */
	/* apcr |= PMUM_SLPEN; */

	__raw_writel(core_idle_cfg, APMU_CORE_IDLE_CFG[cpu]);
	__raw_writel(mp_idle_cfg, APMU_MP_IDLE_CFG[cpu]);
	__raw_writel(apcr, MPMU_APCR);
	if (has_feat_debugreg_in_d1p() && !has_feat_d1p_hipwr()) {
		__raw_writel(mc_slp_type, APMU_MC_HW_SLP_TYPE);
		__raw_writel(debug, PMU_DEBUG_REG);
	}
}

static void pxa988_gic_global_mask(u32 cpu, u32 mask)
{
	u32 core_idle_cfg;

	core_idle_cfg = __raw_readl(APMU_CORE_IDLE_CFG[cpu]);

	if (mask) {
		core_idle_cfg |= PMUA_GIC_IRQ_GLOBAL_MASK;
		core_idle_cfg |= PMUA_GIC_FIQ_GLOBAL_MASK;

	} else {
		core_idle_cfg &= ~(PMUA_GIC_IRQ_GLOBAL_MASK |
					PMUA_GIC_FIQ_GLOBAL_MASK);
	}
	__raw_writel(core_idle_cfg, APMU_CORE_IDLE_CFG[cpu]);
}

static void pxa988_icu_global_mask(u32 cpu, u32 mask)
{
	u32 icu_msk;

	icu_msk = __raw_readl(ICU_GBL_INT_MSK[cpu]);

	if (mask) {
		icu_msk |= ICU_MASK_FIQ;
		icu_msk |= ICU_MASK_IRQ;
	} else {
		icu_msk &= ~(ICU_MASK_FIQ | ICU_MASK_IRQ);
	}
	__raw_writel(icu_msk, ICU_GBL_INT_MSK[cpu]);
}

#define SCU_PM_NORMAL		0
#define SCU_PM_DORMANT		2
#define SCU_PM_POWEROFF		3
#define SCU_CPU_STATUS		0x08

#define SL2C_PHYS_BASE		0xd1dfb000
#define L2X0_POWER_CTRL		0xF80
#define SCU_CTRL		0x00

int scu_power_mode(void *scu_base, int mode)
{
	unsigned int val;
	int cpu = hard_smp_processor_id();

	val = __raw_readb(scu_base + SCU_CPU_STATUS + cpu) & ~0x03;
	val |= mode;
	__raw_writeb(val, scu_base + SCU_CPU_STATUS + cpu);

	return 0;
}

static void pxa988_pre_enter_lpm(u32 cpu, u32 power_mode)
{
	pxa988_lowpower_config(cpu, power_mode, 1);

	/* Mask GIC global interrupt */
	pxa988_gic_global_mask(cpu, 1);
	/* Mask ICU global interrupt */
	pxa988_icu_global_mask(cpu, 1);

	if (cpu_is_pxa988() || cpu_is_pxa986()) {
		/* Seems not useful to enter M1/M2? */
		scu_power_mode((void *)PGU_PHYS_BASE, SCU_PM_POWEROFF);
	}
}

static void pxa988_post_enter_lpm(u32 cpu, u32 power_mode)
{
	/* Unmask GIC interrtup */
	pxa988_gic_global_mask(cpu, 0);
	/*
	 * FIXME: Do we need to mask ICU before cpu_cluster_pm_exit
	 * to avoid GIC ID31 interrupt?
	 */
	/* Mask ICU global interrupt */
	pxa988_icu_global_mask(cpu, 1);

	pxa988_lowpower_config(cpu, power_mode, 0);
}

int do_lpm_debug(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i;

	printf("cpu_id\t\t");
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		printf("cpu%d\t\t", i);
	printf("\n");

	printf("CORE_IDLE_CFG\t");
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		printf("0x%08x\t", __raw_readl(APMU_CORE_IDLE_CFG[i]));
	printf("\n");

	printf("MP_IDLE_CFG\t");
	for (i = 0; i < CONFIG_NR_CPUS; i++)
		printf("0x%08x\t", __raw_readl(APMU_MP_IDLE_CFG[i]));
	printf("\n");

	printf("CP_IDLE_CFG\t");
	printf("0x%08x\n", __raw_readl(PMU_CP_IDLE_CFG));

	if (cpu_is_pxa1U88()) {
		printf("SP_IDLE_CFG\t");
		printf("0x%08x\n", __raw_readl(PMU_SP_IDLE_CFG));
	}

	printf("APCR\t");
	printf("0x%08x\n", __raw_readl(MPMU_APCR));

	printf("CPCR\t");
	printf("0x%08x\n", __raw_readl(MPMU_CPCR));

	printf("WARM_RESET_VECTOR\t");
	printf("0x%08x\n", __raw_readl(CIU_CA9_WARM_RESET_VECTOR));

	return 0;
}

U_BOOT_CMD(
	lpm_debug,	1,	1,	do_lpm_debug,
	"lpm debug",
	""
);

static void do_c12(void)
{
	int cpu = hard_smp_processor_id();

	/* Mask ICU global interrupt, ICU could wakeup WFI */
	pxa988_icu_global_mask(cpu, 0);

	pxa988_lowpower_config(cpu, POWER_MODE_CORE_INTIDLE, 1);

	core_wfi();

	pxa988_lowpower_config(cpu, POWER_MODE_CORE_INTIDLE, 0);
}

static void do_c13(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_lowpower_config(cpu, POWER_MODE_CORE_EXTIDLE, 1);

	core_wfi();

	pxa988_lowpower_config(cpu, POWER_MODE_CORE_EXTIDLE, 0);
}

static void do_c22p(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_CORE_PWRDOWN_L1_ON);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_CORE_PWRDOWN_L1_ON);
}

static void do_c22(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_CORE_PWRDOWN);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_CORE_PWRDOWN);
}

void do_m11_c13(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_lowpower_config(cpu, POWER_MODE_MP_IDLE_CORE_EXTIDLE, 1);

	core_wfi();

	pxa988_lowpower_config(cpu, POWER_MODE_MP_IDLE_CORE_EXTIDLE, 0);
}

void do_m11_c22p(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_MP_IDLE_CORE_PWRDOWN_L1_ON);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_MP_IDLE_CORE_PWRDOWN_L1_ON);
}

void do_m11_c22(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_MP_IDLE_CORE_PWRDOWN);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_MP_IDLE_CORE_PWRDOWN);
}

void do_m21p(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN_SCU_ON);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN_SCU_ON);
}

void do_m21(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN);
}

void do_m22(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN_L2_OFF);

	core_pwr_down();

	pxa988_post_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN_L2_OFF);
}

static void do_d1p(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_APPS_IDLE);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_APPS_IDLE);
}

static void do_d1pddr(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_APPS_IDLE_DDR);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_APPS_IDLE_DDR);
}

static void do_d1pp(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_APPS_SLEEP);

	/* Need ASYNC enable here */
	__raw_writel(PMUM_AP_ASYNC_INT, MPMU_AWUCRM);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_APPS_SLEEP);
}

static void do_d1ppstdby(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_APPS_SLEEP_UDR);

	/* Need ASYNC enable here */
	__raw_writel(PMUM_AP_ASYNC_INT, MPMU_AWUCRM);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_APPS_SLEEP_UDR);
}

static void do_d1(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_SYS_SLEEP);

	/* Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_SYS_SLEEP);
}

static void do_d2(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_ON_L2_ON);

	/* Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_ON_L2_ON);
}

static void do_d2p(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_ON);

	/* FIXME: Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_ON);
}

static void do_d2pp(void)
{
	int cpu = hard_smp_processor_id();

	pxa988_pre_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF);

	/* FIXME: Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	/* FIXME: need cpu_suspend/resume and CP voting support */

	pxa988_post_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF);
}

u32 get_idle_count(u32 cpu)
{
	unsigned int *addr = (unsigned int *)IDLE_CNT_ADDR(cpu);
	return *addr;
}
void set_idle_count(u32 cpu, u32 val)
{
	unsigned int *addr = (unsigned int *)IDLE_CNT_ADDR(cpu);
	*addr = val;
}

int do_lpm_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i = 0;
	int seconds;
	int cpu = hard_smp_processor_id();
	int v;
	int idle_counts = get_idle_count(cpu);
	set_idle_count(cpu, ++idle_counts);

	/* Disable dbgnopwd */
	int coresight_addr = 0xd4100000;
	printf("coresight: 0x%08x, 0x%08x\n",
	       __raw_readl(coresight_addr+0x10310),
	       __raw_readl(coresight_addr+0x12310));
	/* Disable dbgnopowerdown */
	/* step 1  to enable the SW access first */
	int read_data;
	read_data = __raw_readl(0xd4282cd0);
	read_data = read_data | 0x100000;
	__raw_writel(read_data, 0xd4282cd0);

	read_data = __raw_readl(0xd4282ce0);
	read_data = read_data | 0x100000;
	__raw_writel(read_data, 0xd4282ce0);

	/* step 2 enable the write access to Core0 and Core1 */
	__raw_writel(0xC5ACCE55, coresight_addr + 0x10000 + 0xFB0);
	__raw_writel(0xC5ACCE55, coresight_addr + 0x12000 + 0xFB0);

	/* step3 enable the core0 */
	read_data = __raw_readl(coresight_addr+0x10310);
	read_data = read_data & 0xFFFFFFFE;
	__raw_writel(read_data, coresight_addr+0x10310);

	/* step3 enable the core1 */
	read_data = __raw_readl(coresight_addr+0x12310);
	read_data = read_data & 0xFFFFFFFE;
	__raw_writel(read_data, coresight_addr+0x12310);

	printf("coresight: 0x%08x, 0x%08x\n",
	       __raw_readl(coresight_addr+0x10310),
	       __raw_readl(coresight_addr+0x12310));

	pxa988_lowpower_config(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF, 0);

	if (argc < 2)
		return 0;

	if (idle_counts == 1) {
		/* gic_init(); */
		/* Disable GIC in cpu interface */
		v = __raw_readl(GIC_CPU_PHYS_BASE);
		if (v & 0x1)
			__raw_writel(v & (~0x1), GIC_CPU_PHYS_BASE);

		/*
		 * We have to use ICU to wake up since in M1/M2
		 * GIC is clock gated
		 */
		icu_enable_timer0_1();
		timer_config(cpu);

		/*
		 * need this udelay otherwise the first time
		 * reading timer0 not correct
		 * Avoid to use udelay to corrupt timer.
		 */
		/* udelay(10); */
		while (i < 3000000)
			i++;

		if (cpu_is_pxa988() || cpu_is_pxa986()) {
			/* According to Emei Dragon Application Guide */
			__raw_writel(0x3, SL2C_PHYS_BASE + L2X0_POWER_CTRL);

			/* Seems not useful to enter M2? */
			__raw_writel((__raw_readl(PGU_PHYS_BASE + SCU_CTRL) |
				      (1 << 5)),
				     PGU_PHYS_BASE + SCU_CTRL);
		}
	}

	printf("set timer\n");
	if (argc == 2)
		seconds = 10;
	else
		seconds = simple_strtoul(argv[2], NULL, 10);
	printf("timer interrupt will come after %d seconds\n", seconds);
	timer_set_match(cpu, seconds);

	printf("%u: core %u enter %s\n",
	       idle_counts, hard_smp_processor_id(), argv[1]);

	if (strcmp("c12", argv[1]) == 0 ||
	    strcmp("C12", argv[1]) == 0) {
		do_c12();
	} else if (strcmp("c13", argv[1]) == 0 ||
		 strcmp("C13", argv[1]) == 0) {
		do_c13();
	} else if (strcmp("c22p", argv[1]) == 0 ||
		 strcmp("C22P", argv[1]) == 0) {
		if (cpu_is_pxa988() || cpu_is_pxa986())
			do_c22p();
		else
			printf("No %s state!\n", argv[1]);
	} else if (strcmp("c22", argv[1]) == 0 ||
		   strcmp("C22", argv[1]) == 0) {
		do_c22();
	} else if (strcmp("m11_c13", argv[1]) == 0 ||
		 strcmp("M11_C13", argv[1]) == 0) {
		do_m11_c13();
	} else if (strcmp("m11_c22p", argv[1]) == 0 ||
		 strcmp("M11_C22P", argv[1]) == 0) {
		if (cpu_is_pxa988() || cpu_is_pxa986())
			do_m11_c22p();
		else
			printf("No %s state!\n", argv[1]);
	} else if (strcmp("m11_c22", argv[1]) == 0 ||
		   strcmp("M11_C22", argv[1]) == 0) {
		do_m11_c22();
	} else if (strcmp("m21p", argv[1]) == 0 ||
		 strcmp("M21P", argv[1]) == 0) {
		if (cpu_is_pxa988() || cpu_is_pxa986())
			do_m21p();
		else
			printf("No %s state!\n", argv[1]);
	} else if (strcmp("m21", argv[1]) == 0 ||
		   strcmp("M21", argv[1]) == 0) {
		do_m21();
	} else if (strcmp("m22", argv[1]) == 0 ||
		 strcmp("M22", argv[1]) == 0) {
		do_m22();
	} else if (strcmp("d1p", argv[1]) == 0 ||
		 strcmp("D1P", argv[1]) == 0) {
		do_d1p();
	} else if (strcmp("d1pddr", argv[1]) == 0 ||
		 strcmp("D1PDDR", argv[1]) == 0) {
		do_d1pddr();
	} else if (strcmp("d1pp", argv[1]) == 0 ||
		 strcmp("D1PP", argv[1]) == 0) {
		do_d1pp();
	} else if (strcmp("d1ppstdby", argv[1]) == 0 ||
		 strcmp("D1PPstdby", argv[1]) == 0) {
		do_d1ppstdby();
	} else if (strcmp("d1", argv[1]) == 0 ||
		 strcmp("D1", argv[1]) == 0) {
		do_d1();
	} else if (strcmp("d2", argv[1]) == 0 ||
		 strcmp("D2", argv[1]) == 0) {
		do_d2();
	} else if (strcmp("d2p", argv[1]) == 0 ||
		 strcmp("D2P", argv[1]) == 0) {
		do_d2p();
	} else if (strcmp("d2pp", argv[1]) == 0 ||
		 strcmp("D2PP", argv[1]) == 0) {
		do_d2pp();
	} else {
		printf("Unsupported power state!\n");
	}

	printf("%u: exit %s\n ", idle_counts, argv[1]);

	timer_clear_irq(cpu);

	return 0;
}

U_BOOT_CMD(
	lpm,	3,	1,	do_lpm_test,
	"lpm test",
	"USAGE: lpm state timeout\n"
	"1. timeout is measured with secs\n"
	"2. Current support states are:\n"
	"   c12/C12: WFI\n"
	"   c13/C13: core external idle\n"
	"   c22p/C22P[PXA988 ONLY]: core power down with L1 on\n"
	"   c22/c22: core power down\n"
	"   m11_c13/M11_C13: all cores in C13 and MP internal idle\n"
	"   m11_c22p/M11_C22P[PXA988 ONLY]: all cores in C22P and MP internal idle\n"
	"   m11_c22/M11_C22: all cores in C22 and MP internl idle\n"
	"   m21p/M21P[PXA988 ONLY]: all cores in C22 and MP power down with SCU on\n"
	"   m21/M21: MP in m21p and SCU off\n"
	"   m22/M22: MP in M21 and L2 off\n"
	"   d1p/D1P: axi vote\n"
	"   d1pddr/D1PDDR: axi and ddr vote\n"
	"   d1pp/D1PP: axi, ddr, ap sleep vote\n"
	"   d1ppstdby: axi, ddr, ap sleep, ap standby vote\n"
	"   d1/D1: axi, ddr, ap sleep and apb vote\n"
	"   d2/D2: axi, ddr, ap sleep, apb and ap standby vote\n"
	"   d2p/D2P: axi, ddr, ap sleep, apb, ap standby and vctcxo vote\n"
	"   d2pp/D2PP: axi, ddr, ap sleep, apb, ap standby and vctcxo vote,L2 off\n"
);

int cp_load(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	__raw_writel(0x06000000, 0xd4282c24);
	__raw_writel(__raw_readl(0xd4051020) & 0xfffffffe, 0xd4051020);

	return 0;
}
U_BOOT_CMD(
	cpload,	1,	1,	cp_load,
	"Load CP and MSA images",
	""
);

int cp_vote(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	pxa988_lowpower_config(0, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF, 1);

	return 0;
}
U_BOOT_CMD(
	cpvote,	1,	1,	cp_vote,
	"Vote CP LPM bits",
	""
);

enum deadidlelvl {
	DEADIDLE_VPU = (1 << 0),
	DEADIDLE_GPU = (1 << 1),
	DEADIDLE_ISP = (1 << 2),
	DEADIDLE_OTHERPERI = (1 << 3),
	DEADIDLE_MCKFAB = (1 << 4),
	DEADIDLE_GNSS = (1 << 5),
	DEADIDLE_ALL = 0xffffffff,
};

static void pxa1x88_deadidle(unsigned int cond)
{
	u32 rstbit, enbit;

	if (cond & DEADIDLE_MCKFAB) {
		/* Fabric0, Fabric1 dynamic clk gate enable */
		*(unsigned int *)(APMU_REG(0x02c)) = 0x0;
		*(unsigned int *)(APMU_REG(0x030)) = 0x0;

		/* other Fabric enalbe */
		/*
		 * Set 19bit & 3bit to 0x0, will enalbe all SOC and MCK dyn
		 * clk gate.
		 */
		*(unsigned int *)(CIU_REG(0x040)) &= ~(0x1<<19);
		*(unsigned int *)(CIU_REG(0x040)) |= (0xff<<8);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x7<<16);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x1 << 21);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x3<<26);
		if (cpu_is_pxa1088() || cpu_is_pxa1L88()) {
			*(unsigned int *)(CIU_REG(0x040)) |= (0x1 << 30);
			*(unsigned int *)(CIU_REG(0x040)) |= (0x1 << 29);
		} else if (cpu_is_pxa988() || cpu_is_pxa986()) {
			*(unsigned int *)(CIU_REG(0x040)) |= (0x1 << 20);
			*(unsigned int *)(CIU_REG(0x040)) &= ~(0x1<<3);
		} else if (cpu_is_pxa1U88()) {
			*(unsigned int *)(CIU_REG(0x040)) |=
				((0x3 << 29) | (1 << 20));
		}
	}

	/* on1, on2, off time */
	*(unsigned int *)(APMU_REG(0x0dc)) = 0x20001fff;

	/* VPU off */
	if (cond & DEADIDLE_VPU) {
		printf("VPU Off\n");
		/* 1. Enable VPU Isolation */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x1<<16);

		/* 2. Assert bus reset, func and AHB reset */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x7<<0);

		/* 3. Reset bus clock enable and func and AHB clock enable */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x7<<3);

		/* 4. Set Dx8 in HW mod */
		*(unsigned int *)(APMU_REG(0x0A4)) |= (0x1<<19);

		/* 5. ref Clock Reset */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x1<<24);

		/* 6. ref Clock Disable */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x1<<25);

		/* 7. power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1<<2);
	}

	/*  GC off */
	if (cond & DEADIDLE_GPU) {
		printf("GC_3D Off\n");

		/* 1. Enable GC3D Isolation */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x1<<8);
		if (has_feat_gcshader()) {
			/* 2. Assert shader clock reset */
			*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x1<<24);
			/* 3. Disable shader clock */
			*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x1<<25);
		}
		/* 4. Assert bus, func and HCLK reset */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x7<<0);

		/* 5. Reset bus, func and HCLK clock enable */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x7<<3);

		/* 6. Set GC in HW mod */
		*(unsigned int *)(APMU_REG(0x0CC)) |= (0x1<<11);

		if (has_feat_gc2d()) {
			printf("GC_2D Off\n");

			/* 1. Enable GC2D Isolation */
			*(unsigned int *)(APMU_REG(0x0F4)) &= ~(0x1<<8);

			/* 2. Assert bus, func and HCLK reset */
			*(unsigned int *)(APMU_REG(0x0F4)) &= ~(0x7<<0);

			/* 3. Reset bus, func and HCLK clock enable */
			*(unsigned int *)(APMU_REG(0x0F4)) &= ~(0x7<<3);

			/* 4. Set GC2D in HW mod */
			*(unsigned int *)(APMU_REG(0x0F4)) |= (0x1<<11);

			/* 5. GC 2D power off */
			if (has_feat_gc2d_separatepwr())
				*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1<<6);
		}

		/* 7. GC 3D(on pxa1L88) / All of GC(on pxa1088) power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1<<0);
	}


	/* ISP off */
	if (cond & DEADIDLE_ISP) {
		printf("ISP Off\n");

		if (cpu_is_pxa1U88()) {
			rstbit = (1 << 0) | (1 << 8) | (1 << 16) | (1 << 27);
			enbit = (1 << 1) | (1 << 17) | (1 << 28);
		} else {
			rstbit = (1 << 0) | (1 << 8) | (1 << 10);
			enbit = (1 << 1) | (1 << 9) | (1 << 11);
		}

		/* 1. Enable ISP Isolation */
		*(unsigned int *)(APMU_REG(0x038)) &= ~(1<<12);

		/* 2. Assert func, AXI and AHB reset */
		*(unsigned int *)(APMU_REG(0x038)) &= ~rstbit;

		/* 3. Reset func, AXI and AHB clock enable */
		*(unsigned int *)(APMU_REG(0x038)) &= ~enbit;

		/* 4. Set ISP in HW mod */
		*(unsigned int *)(APMU_REG(0x038)) |= (0x1<<15);

		/* 5. power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1<<4);
	}

	if (cond & DEADIDLE_GNSS) {
		if (has_feat_gnss()) {
			printf("GNSS Off\n");
			/* 1. Enable Isolation */
			*(unsigned int *)(APMU_REG(0x0FC)) &= ~(1 << 2);

			/* 2. Assert reset */
			*(unsigned int *)(APMU_REG(0x0FC)) &= ~(1 << 1);

			/* 3. Set in HW mod */
			*(unsigned int *)(APMU_REG(0x0FC)) |= (1 << 0);

			/* 4. power off */
			*(unsigned int *)(APMU_REG(0x0d8)) &= ~(1 << 8);
		}
	}

	/* other devs */
	if (cond & DEADIDLE_OTHERPERI) {
		printf("Other devices reset\n");

		/* USB */
		printf("USB reset\n");
		*(unsigned int *)(APMU_REG(0x034)) = 0x0;
		*(unsigned int *)(APMU_REG(0x05C)) = 0x0;

		/* HSI/LTE_DMA */
		printf("HSI/LTE_DMA reset\n");
		*(unsigned int *)(APMU_REG(0x048)) = 0x0;

		/* SRAM power down, need to check */
		printf("SRAM Power Down\n");
		/* Let L1 and L2 Power down to APMU Core power down sequence */
		/* Config L2 SRAM to HW control */
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3<<18);
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3<<20);
		/* Config L1 SRAM to HW control */
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3<<8);
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3<<0);

		/* Power down it SQU SRAM */
		*(unsigned int *)(APMU_REG(0x08C)) |= (0x3<<16);

		/* LCD */
		printf("LCD clk gate reset\n");
		/* clk reset */
		*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1<<16);
		*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x7<<0);

		/* clk enable */
		if (cpu_is_pxa1U88()) {
			*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x3f<<3);
		} else {
			*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x7<<3);
			*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x3<<8);
		}
		udelay(100);

		/* DSI PHY ESC reset and disable */
		*(unsigned int *)(APMU_REG(0x044)) &= ~(0x3<<3);
		*(unsigned int *)(APMU_REG(0x044)) &= ~(0x1<<2);
		*(unsigned int *)(APMU_REG(0x044)) &= ~(0x1<<5);
		udelay(100);

		/* ISCCR0/1 */
		printf("I2S clk gate reset\n");
		/* sysclk, baseclk disable */
		*(unsigned int *)(MPMU_REG(0x040)) &= ~(0x1<<31);
		*(unsigned int *)(MPMU_REG(0x040)) &= ~(0x1<<29);
		/* sysclk, baseclk disable */
		*(unsigned int *)(MPMU_REG(0x044)) &= ~(0x1<<31);
		*(unsigned int *)(MPMU_REG(0x044)) &= ~(0x1<<29);

		/* CCIC */
		/* CCIC all dynamic change enalbe */
		*(unsigned int *)(APMU_REG(0x028)) = 0x0;
		if (cpu_is_pxa1U88()) {
			rstbit = (1 << 1) | (1 << 2) | (1 << 22);
			enbit = (1 << 4) | (1 << 5) | (1 << 21);
			*(unsigned int *)(APMU_REG(0x050)) &= ~rstbit;
			*(unsigned int *)(APMU_REG(0x050)) &= ~enbit;
		} else {
			/* Disable PHY */
			*(unsigned int *)(APMU_REG(0x050)) &=
				~((0x1<<5) | (0x1<<9));
			*(unsigned int *)(APMU_REG(0x088)) &= ~(0x3<<25);
			/* Disable func clock */
			*(unsigned int *)(APMU_REG(0x050)) &= ~(0x1<<4);
			/* Disable AXI clock */
			*(unsigned int *)(APMU_REG(0x050)) &= ~(0x1<<3);
		}

		/* For CCIC2 */
		if (has_feat_ccic2()) {
			if (cpu_is_pxa1U88()) {
				rstbit = (1 << 1) | (1 << 2);
				enbit = (1 << 4) | (1 << 5) | (1 << 26);
				*(unsigned int *)(APMU_REG(0x024)) &= ~rstbit;
				*(unsigned int *)(APMU_REG(0x024)) &= ~enbit;
			} else {
				/* Disable PHY for CCIC2*/
				*(unsigned int *)(APMU_REG(0x024)) &=
					~((0x1<<5) | (0x1<<9));

				/* Disable func clock for CCIC2 */
				*(unsigned int *)(APMU_REG(0x024)) &= ~(0x1<<4);

				/* Disable AXI clock for CCIC2*/
				*(unsigned int *)(APMU_REG(0x024)) &= ~(0x1<<3);
			}
		}

		/* SDH0/1/2 */
		printf("SDH clk gate reset\n");
		/* reset and disable */
		/* SDH0 Reset and Disable */
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1<<1);
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1<<4);
		/* SDH1 Reset and Disable */
		*(unsigned int *)(APMU_REG(0x058)) &= ~(0x1<<1);
		*(unsigned int *)(APMU_REG(0x058)) &= ~(0x1<<4);
		/* SDH2 Reset and Disable */
		*(unsigned int *)(APMU_REG(0x0e0)) &= ~(0x1<<1);
		*(unsigned int *)(APMU_REG(0x0e0)) &= ~(0x1<<4);
		/* ALL SDH AXI Reset and Disable */
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1<<0);
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1<<3);

		/* NF */
		printf("NF clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x060)) = 0x0;

		/* AES */
		printf("AES clk gate reset\n");
		/*
		 * In PXA1U88, reset AES clk will not decrease but increase
		 * VCC_MAIN current. Thus we only shutdown its clk.
		 */
		if (cpu_is_pxa1U88())
			*(unsigned int *)(APMU_REG(0x068)) = 0x50;
		else
			*(unsigned int *)(APMU_REG(0x068)) = 0x0;

		/* DTC */
		printf("DTC clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x0aC)) = 0x0;

		/* SMC */
		printf("SMC clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x0d4)) &= ~(0x3<<0);
		*(unsigned int *)(APMU_REG(0x0d4)) &= ~(0x3<<3);

		/* MCK4 AHB HCLK */
		printf("MCK4 AHB HCLK clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x0e8)) = 0x0;

		/* trace reset  */
		printf("Trace clk gate power off\n");
		*(unsigned int *)(APMU_REG(0x108)) &= ~(0x1<<16);
		*(unsigned int *)(APMU_REG(0x108)) &= ~(0x3<<3);

		/* ACLK gate enable  */
		printf("ACLK gate enable\n");
		printf("ciu=%d\n", *(unsigned int *)(CIU_REG(0x05c)));
		/* Gated or not no influnce */
		/* *(unsigned int *)(CIU_REG(0x05c)) |= (0x1<<9); */

		/* DCLK gate enable  */
		printf("DCLK gate enable\n");
		/* Gated or not no influnce */
		/* *(unsigned int *)(CIU_REG(0x05c)) &= ~(0x1<10); */


		/* Global clk gat baypas and debug gate disenable  */
		if (cpu_is_pxa988()) {
			printf("Debug enable\n");
			*(unsigned int *)(CIU_REG(0x068)) &= ~(0x1<<8);
			*(unsigned int *)(CIU_REG(0x068)) &= ~(0x1<<9);
		}

		/* SQU dynamic clk gate enable  */
		printf("SQU gate enable\n");
		*(unsigned int *)(APMU_REG(0x01c)) &= ~(0x3fffffff<<0);

		/* CCGR & ACGR */
		/* printf("CCGR & ACGR\n"); */
		/* Please use this setting when Getting Deadidle power
		 * and OP(CORE/DDR/AXI) has been set to: 312/104/104
		 */
		/* *(unsigned int *)(PMUM_ADDR+0x0024) = 0xa010; */
		/* *(unsigned int *)(PMUM_ADDR+0x1024) = 0x2812; */

		/* APB */
		printf("APB\n");
		printf("APB UART1\n");
		*(unsigned int *)(APBC_REG(0x0004)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0004)) |= (0x1<<2);

		printf("APB GPIO\n");
		*(unsigned int *)(APBC_REG(0x0008)) &= ~(0x3<<0);
		/* Do not set, will chang pmic, then fix voltage to 1.2V */
		/* *(unsigned int *)(APBC_REG(0x0008)) |= (0x1<<2); */

		printf("APB PWM0\n");
		*(unsigned int *)(APBC_REG(0x000c)) |= (0x1<<2);
		*(unsigned int *)(APBC_REG(0x000c)) &= ~(0x3<<0);

		printf("APB PWM1\n");
		*(unsigned int *)(APBC_REG(0x0010)) |= (0x1<<2);
		*(unsigned int *)(APBC_REG(0x0010)) &= ~(0x2<<0);

		printf("APB PWM2\n");
		*(unsigned int *)(APBC_REG(0x0014)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0014)) |= (0x1<<2);

		printf("APB PWM3\n");
		*(unsigned int *)(APBC_REG(0x0018)) &= ~(0x2<<0);
		*(unsigned int *)(APBC_REG(0x0018)) |= (0x1<<2);

		printf("APB SSP0\n");
		*(unsigned int *)(APBC_REG(0x001c)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x001c)) |= (0x1<<2);

		printf("APB SSP1\n");
		*(unsigned int *)(APBC_REG(0x0020)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0020)) |= (0x1<<2);

		printf("APB IPC\n");
		*(unsigned int *)(APBC_REG(0x0024)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0024)) |= (0x1<<2);

		printf("APB RTC\n");
		*(unsigned int *)(APBC_REG(0x0028)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0028)) |= (0x1<<2);
		*(unsigned int *)(APBC_REG(0x0028)) &= ~(0x1<<7);

		printf("APB TWSI\n");
		*(unsigned int *)(APBC_REG(0x002c)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x002c)) |= (0x1<<2);

		printf("APB KPC\n");
		*(unsigned int *)(APBC_REG(0x0030)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0030)) |= (0x1<<2);

		printf("APB TIMERS\n");
		*(unsigned int *)(APBC_REG(0x0034)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0034)) |= (0x1<<2);

		printf("APB TB_ROTARY\n");
		*(unsigned int *)(APBC_REG(0x0038)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0038)) |= (0x1<<2);

		printf("APB ATB\n");
		*(unsigned int *)(APBC_REG(0x003c)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x003c)) |= (0x1<<2);

		printf("APB SW_JTAG\n");
		*(unsigned int *)(APBC_REG(0x0040)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0040)) |= (0x1<<2);

		/*
		 * Do not shutdown timer1 clk if it has already been
		 * set to wakeup cpu0/1
		 */
		if ((*(u32 *)CONFIG_LPM_TIMER_WAKEUP & (1 << 0 | 1 << 1))
		    == 0) {
			printf("APB TIMERS1\n");
			*(unsigned int *)(APBC_REG(0x0044)) &= ~(0x3<<0);
			*(unsigned int *)(APBC_REG(0x0044)) |= (0x1<<2);
		}

		printf("APB ONEWIRE\n");
		*(unsigned int *)(APBC_REG(0x0048)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0048)) |= (0x1<<2);

		printf("APB SSP2\n");
		*(unsigned int *)(APBC_REG(0x004c)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x004c)) |= (0x1<<2);

		printf("APB DRO_TS\n");
		*(unsigned int *)(APBC_REG(0x0058)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0058)) |= (0x1<<2);
		printf("APB I2C1\n");
		*(unsigned int *)(APBC_REG(0x0060)) &= ~(0x3<<0);
		*(unsigned int *)(APBC_REG(0x0060)) |= (0x1<<2);

		if (has_feat_timer2()) {
			/*
			 * Do not shutdown timer2 clk if it has already been
			 * set to wakeup cpu2/3.
			 */
			if ((*(u32 *)CONFIG_LPM_TIMER_WAKEUP &
			     (1 << 2 | 1 << 3)) == 0) {
				printf("APB TIMERS2\n");
				*(unsigned int *)(APBC_REG(0x0068)) &=
					~(0x3<<0);
				*(unsigned int *)(APBC_REG(0x0068)) |=
					(0x1<<2);
			}
		}

		/* hardware dynamic clk gate */
		printf("Hardware dynamic clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x040)) = 0x0;

		printf("APB UART0\n");
		/* *(unsigned int *)(APBC_REG(0x0000)) &= ~(0x3<<0); */
		/* *(unsigned int *)(APBC_REG(0x0000)) |= (0x1<<2); */
	}
}




int do_deadidle(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	printf("deadidle\n");
	u32 condition = simple_strtoul(argv[1], NULL, 10);
	if (!condition)
		condition = DEADIDLE_ALL;
	pxa1x88_deadidle(condition);
	printf("done\n");

	printf("0xd42828f0=0x%010x\n", *(unsigned int *)(0xd42828f0));

	return 0;
}

U_BOOT_CMD(
	deadidle,       2,      1,      do_deadidle,
	"ready to do deadidle",
	"[ op number ]"
);


int do_core_busy(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cur_cpu = hard_smp_processor_id();

	*(u32 *)(CONFIG_CORE_BUSY_ADDR) |= 1 << cur_cpu;
	uartsw_to_active_core();

	while (1)
		cpu_relax();
}

U_BOOT_CMD(
	core_busy,       2,      1,      do_core_busy,
	"let current cpu do while(1)",
	"[ core_busy]"
);

uint32_t seagull_wfi_op = 0xee074f90;
uint32_t seagull_loop_ops[] = {0xe1a00000, 0xeafffffd};
uint32_t msa_d2_ops[] = {
	0x9ffce109, 0xd1e0e149, 0x327e3271, 0x0000e19c,
	0x0000e19d, 0x0000e19e, 0x0000e19f, 0x0000e10a,
	0xd407e14a, 0x60029111, 0x03d8e102, 0x3221504a,
	0x00000064, 0x00000000, 0x00000000, 0x00000000,
	0x00002000, 0x0035e800, 0x0004e100, 0x34094281,
	0xffe0e151, 0xba709d08, 0xe141b870, 0x3401d101,
	0xe10a9f00, 0xe14a1004, 0x9110ffe0, 0xb941bb40,
	0x0008e100, 0xe1404280, 0x3208d101, 0x60089309,
	0x00249f08, 0x00249310, 0x0000e100, 0xe1404280,
	0x3200d101, 0x93016001, 0x91002001, 0xb920bb20,
	0x1c020e18, 0x91002063, 0xb910bb10, 0x4a104f80,
	0x1300e112, 0xffe0e152, 0x91009f10, 0xb900bb00,
	0x4f804a50, 0x9f104a10, 0xba909100, 0x4a58b890,
	0x4a104f80, 0x91009f10, 0xb880ba80, 0x4a584a50,
	0x4a104f80, 0x91009f10, 0xb8d0bad0, 0x4a004f80,
	0x9f104a10, 0xbac09100, 0x4a50b8c0, 0x4a004f80,
	0x9f104a10, 0xbab09100, 0x4a58b8b0, 0x4a004f80,
	0x9f104a10, 0xbaa09100, 0x4a50b8a0, 0x4f804a58,
	0x4a104a00, 0x91009f10, 0xb932bb30, 0xe1804f82,
	0x56824005, 0x91029f12, 0xb8e2bae2, 0x4f824a52,
	0x9f125682, 0xbaf29102, 0x4a5ab8f2, 0x56824f82,
	0x91029f12, 0xb9b2bbb2, 0x4a5a4a52, 0x56024f82,
	0x91009f10, 0x93006408, 0x93012f99, 0x91002001,
	0xb9e0bbe0, 0x1c020e18, 0x91002067, 0xb9d0bbd0,
	0x4a104f80, 0x0300e112, 0xffe0e152, 0x91009f10,
	0xb9c0bbc0, 0x4f804a50, 0x9f104a10, 0xbb609100,
	0x4a58b960, 0x4a104f80, 0x91009f10, 0xb950bb50,
	0x4a584a50, 0x4a104f80, 0x91009f10, 0xb990bb90,
	0x4a004f80, 0x9f104a10, 0xbb809100, 0x4a50b980,
	0x4a004f80, 0x9f104a10, 0xbb709100, 0x4a58b970,
	0x4a004f80, 0x9f104a10, 0xbba09100, 0x4a50b9a0,
	0x4f804a58, 0x4a104a00, 0x91009f10, 0xb812ba10,
	0xe1804f82, 0x56824005, 0x91029f12, 0xffdae63a,
	0xffdae43a, 0x4f824a52, 0x9f125682, 0xba629102,
	0x4a5ab862, 0x56824f82, 0x91029f12, 0xffd8e63a,
	0xffd8e43a, 0x4a5a4a52, 0x56024f82, 0x91009f10,
	0x93006408, 0x93012f95, 0x91002001, 0xffd7e638,
	0xffd7e438, 0x1c020e38, 0x9100201d, 0xffd6e638,
	0xffd6e438, 0x4a004f68, 0x1300e112, 0xffe0e152,
	0x91009f10, 0xffcfe638, 0xffcfe438, 0x4a004f68,
	0x0300e112, 0xffe0e152, 0x91009f10, 0x93006408,
	0x93012fdd, 0x91002001, 0xffcee638, 0xffcee438,
	0x1c020e18, 0x9100208b, 0xffd3e638, 0xffd3e438,
	0x4a104f80, 0x1300e112, 0xffe0e152, 0x91009f10,
	0xffd2e638, 0xffd2e438, 0x4f804a50, 0x9f104a10,
	0xe6389100, 0xe438ffd1, 0x4a58ffd1, 0x4a104f80,
	0x91009f10, 0xffd0e638, 0xffd0e438, 0x4a584a50,
	0x4a104f80, 0x91009f10, 0xffd9e638, 0xffd9e438,
	0x9f104f80, 0xe6389100, 0xe438ffd4, 0x4a50ffd4,
	0x9f104f80, 0xe6389100, 0xe438ffd5, 0x4a58ffd5,
	0x9f104f80, 0xba209100, 0x4a50b820, 0x4f804a58,
	0x91009f10, 0xb850ba50, 0x4a104f80, 0x0300e112,
	0xffe0e152, 0x91009f10, 0xb840ba40, 0x4f804a50,
	0x9f104a10, 0xba309100, 0x4a58b830, 0x4a104f80,
	0x91009f10, 0xffdce638, 0xffdce438, 0x4a584a50,
	0x4a104f80, 0x91009f10, 0xffdbe638, 0xffdbe438,
	0x9f104f80, 0xba009100, 0x4a50b800, 0x9f104f80,
	0xe6389100, 0xe438ffdf, 0x4a58ffdf, 0x9f104f80,
	0xe6389100, 0xe438ffde, 0x4a50ffde, 0x4f804a58,
	0x91009f10, 0x93006408, 0x9d002f6f, 0xffdde638,
	0xffdde438, 0x00249f08, 0xbbf09108, 0x9310b9f0,
	0xe8010024, 0x00100000, 0xe80005f5, 0xe10d0004,
	0xe14d200c, 0xe100ffa8, 0x42860000, 0xe147303e,
	0x932fd408, 0xe1000024, 0x42800014, 0xf024e140,
	0xe1003200, 0xe14046e0, 0x9300c001, 0xe7fce108,
	0xd401e148, 0x0049e180, 0xe3ff9300, 0xe108fe15,
	0xe1480100, 0x9100f00e, 0xb9f1bbf0, 0xffffe100,
	0xe1404280, 0x5401fe3f, 0x93004a00, 0xe1080024,
	0xe1485050, 0x6008ffd0, 0x00249300, 0x1008e146,
	0x0024932e, 0x2000e108, 0xffa8e148, 0x5008e100,
	0xfe08e140, 0x00249300, 0x00240020, 0x00000000,
	0x00000000, 0x00000000, 0x0024932f
};
uint32_t msa_loop_ops[] = {0x64086000, 0x00002fff};

void cp_msa_sleep(void)
{
	if (__raw_readl(0xd4051004) & (1 << 31)) {
		printf("C2 and MSA are already in D2!\n");
		return;
	}

	if (cpu_is_pxa1U88()) {
		/*
		 * Helan2 adds 2 extra bits: bit3 and bit12. They have
		 * to be set to ensure system can enter idle states.
		 */
		*(uint32_t *)(0xd4051000) = 0x12005008;
		/*
		 * CPCR: must disable wakeup port0(GSM) and port1(TD) since
		 * there will be a constant GSM/TD wakeup event after board
		 * boots up.
		 * Pls note for Helan2, port5 must be enabled since it's used
		 * for GPS wakeup.
		 */
		*(uint32_t *)(0xD4050000) = 0xBEC87008;
	} else {
		*(uint32_t *)(0xd4051000) = 0x52004000;
		*(uint32_t *)(0xD4050000) = 0xFECA6000;
	}

	/* POCR */
	*(uint32_t *)(0xD405000c) = 0x001F0500;
	/* SCCR */
	*(uint32_t *)(0xD4050038) = 0x00000005;
	/*
	 * PMUA_CP_IDLE_CFG, set 0x803FC720 (both retain),
	 * set 0x803f0760 for both power down, [15:14]-L2,
	 * [6]-L1
	 */
	*(uint32_t *)(0xd4282814) = 0x803fc720;

	/* ICU_CP_GBL_INT_MSK */
	*(uint32_t *)(0xd4282110) |= 0x3;
	printf("put Seagull into D2 state now...\r\n");
	*(uint32_t *)(0xd4282c24) = (uint32_t)&seagull_wfi_op;
	/* release CP and Vote MSA SS available */
	*(uint32_t *)(0xd4051020) &= (~0x5);
	if (cpu_is_pxa988() || cpu_is_pxa1088() || cpu_is_pxa1L88()) {
		*(uint32_t *)(0xd4200200) =
			(((uint32_t)(&msa_d2_ops[0])) & 0xFF000000) >> 24;
		*(uint32_t *)(0xd4070000) =
			0xd0000000 + (((uint32_t)(&msa_d2_ops[0])) & 0xFFFFFF);
	} else if (cpu_is_pxa1U88()) {
		*(uint32_t *)(0xd4282d24) =
			(((uint32_t)(&msa_d2_ops[0])) & 0xFC000000) >> 26;
		*(uint32_t *)(0xd4070000) =
			0xd8000000 + (((uint32_t)(&msa_d2_ops[0])) & 0x3FFFFFF);
	}
	printf("put MSA into D2 state now...\r\n");
	*(uint32_t *)(0xd405003c) &= 0xfffffffc;
	*(uint32_t *)(0xd4050020) |= 0x2C;
	/* 500us */
	udelay(512);
	*(uint32_t *)(0xd4050020) &= ~(1 << 3);
	*(uint32_t *)(0xd4050020) &= ~(1 << 2);
	/* 20us */
	udelay(32);
	*(uint32_t *)(0xd4050020) &= ~(1 << 5);
	udelay(1000);
	/* until D2 is really entered */
	while (!(__raw_readl(0xd4051004) & (1 << 31)))
		cpu_relax();
	printf("C2 and MSA in D2!\n");
}

int comm_d2(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cp_msa_sleep();
	return 0;
}

U_BOOT_CMD(
	cp_d2,	1,	1,	comm_d2,
	"CP & MSA enter low power mode",
	""
);

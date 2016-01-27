/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Fangsuo Wu <fswu@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/features.h>
#include <power/pxa_common.h>

DECLARE_GLOBAL_DATA_PTR;
#define AXI_PHYS_BASE		0xd4200000
#define APB_PHYS_BASE		0xd4000000
#define APBC_PHYS_BASE		(APB_PHYS_BASE + 0x015000)
#define APBC_REG(x)             (APBC_PHYS_BASE + (x))
#define APMU_PHYS_BASE		(AXI_PHYS_BASE + 0x82800)
#define APMU_REG(x)		(APMU_PHYS_BASE + (x))

#define PMU_CORE_STATUS		APMU_REG(0x0090)
#define PMU_CC2_AP		APMU_REG(0x0100)
#define PMU_CORE0_IDLE_CFG	APMU_REG(0x0124)
#define PMU_CORE1_IDLE_CFG	APMU_REG(0x0128)
#define PMU_CORE2_IDLE_CFG	APMU_REG(0x0160)
#define PMU_CORE3_IDLE_CFG	APMU_REG(0x0164)
#define PMU_CP_IDLE_CFG		APMU_REG(0x014)
#define PMU_MP_IDLE_CFG0	APMU_REG(0x0120)
#define PMU_MP_IDLE_CFG1	APMU_REG(0x00e4)
#define PMU_MP_IDLE_CFG2	APMU_REG(0x0150)
#define PMU_MP_IDLE_CFG3	APMU_REG(0x0154)
#define PMU_DEBUG2_REG		APMU_REG(0x01b0)
#define PMU_DEBUG_REG		APMU_REG(0x0088)

#define ICU_PHYS_BASE		(AXI_PHYS_BASE + 0x82000)
#define ICU_REG(x)		(ICU_PHYS_BASE + (x))

#define ICU_APC0_GBL_INT_MSK	ICU_REG(0x228)
#define ICU_APC1_GBL_INT_MSK	ICU_REG(0x238)
#define ICU_APC2_GBL_INT_MSK	ICU_REG(0x248)
#define ICU_APC3_GBL_INT_MSK	ICU_REG(0x258)

#define CIU_PHYS_BASE		(AXI_PHYS_BASE + 0x82c00)
#define CIU_REG(x)		(CIU_PHYS_BASE + (x))
#define CIU_WARM_RESET_VECTOR	CIU_REG(0x00d8)

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

#define GIC_CPU_INTERFACE	(0xd1dfa000)
#define GIC_CPU_REG(x)		(0xd1dfa000 + (x))
#define GICC_CTRL		GIC_CPU_REG(0x0)

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

#define PMUA_CORE_IDLE			(1 << 0)
#define PMUA_CORE_POWER_DOWN		(1 << 1)
#define PMUA_GIC_IRQ_GLOBAL_MASK	(1 << 3)
#define PMUA_GIC_FIQ_GLOBAL_MASK	(1 << 4)

#define PMUA_MP_IDLE			(1 << 0)
#define PMUA_MP_POWER_DOWN		(1 << 1)
#define PMUA_MP_L2_SRAM_POWER_DOWN	(1 << 2)

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
#define LPM_WAKEUP		(CORE0_1_WAKEUP | CORE2_3_WAKEUP | PMUM_WAKEUP4)

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

#define CORE_N_C2(n)		(1 << (C2_BIT(n)))
#define CORE_N_C1(n)		(1 << (C1_BIT(n)))
#define CORE_N_WFI(n)		(1 << (WFI_BIT(n)))
#define CORE_N_SLEEP(n)		((CORE_N_C2(n)) | (CORE_N_C1(n)) | \
				 (CORE_N_WFI(n)))

#define cpu_relax()		barrier()

enum pxa1908_lowpower_state {
	POWER_MODE_ACTIVE = 0,			/* not used */
	/* core level power modes */
	POWER_MODE_CORE_INTIDLE,		/* C12 */
	POWER_MODE_CORE_EXTIDLE,		/* C13 */
	POWER_MODE_CORE_PWRDOWN,		/* C22 */
	/* MP level power modes */
	POWER_MODE_MP_IDLE_CORE_EXTIDLE,	/* M11_C13 */
	POWER_MODE_MP_IDLE_CORE_PWRDOWN,	/* M11_C22 */
	POWER_MODE_MP_PWRDOWN,			/* M21 */
	POWER_MODE_MP_PWRDOWN_L2_OFF,		/* M22 */
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

static inline void uartsw_to_active_core(void)
{
#ifdef CONFIG_PXA_AMP_SUPPORT
	int cpu, cur_cpu = smp_hw_cpuid();
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

		core_active_tmp_status = core_active_status & CORE_N_SLEEP(cpu);
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
	asm volatile("dsb sy");
	asm volatile("wfi");
}

static void lpm_debug(void);

static inline void core_pwr_down(void)
{
	lpm_debug();
	uartsw_to_active_core();
	invoke_fn_smc(0x85000001, CONFIG_SYS_TEXT_BASE, 1, 0);
	return;
}

static void icu_enable_timer0_1(void)
{
	/* Timer 2_0 route to CPU 0 */
	__raw_writel(0x1 | (0x1 << 6) | (0x1 << 4), ICU_REG(64 << 2));
	/* Timer 2_1 route to CPU 1 */
	__raw_writel(0x1 | (0x1 << 7) | (0x1 << 4), ICU_REG(65 << 2));
	/* Timer 0_0 route to CPU 2 */
	__raw_writel(0x1 | (0x1 << 8) | (0x1 << 4), ICU_REG(13 << 2));
	/* Timer 0_1 route to CPU 3 */
	__raw_writel(0x1 | (0x1 << 9) | (0x1 << 4), ICU_REG(14 << 2));
}

static void timer_config(int cpu)
{
	unsigned long timer_phy_base = (cpu < 2) ? TIMERS_PHYS_BASE_2 :
				TIMERS_PHYS_BASE_0;
	unsigned long timer_clk_base = (cpu < 2) ? APBC_TIMERS2 : APBC_TIMERS0;
	/* Helan2 adds extal bit 7 to enable timer0 wakeup */
	unsigned long timer_clk_val = (cpu >= 2) ?
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
	unsigned long timer_phy_base = (cpu < 2) ? TIMERS_PHYS_BASE_2 : TIMERS_PHYS_BASE_0;
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
	unsigned long timer_phy_base = (cpu < 2) ? TIMERS_PHYS_BASE_2 : TIMERS_PHYS_BASE_0;

	/* clr the info that timer has been set to wakeup cpu */
	*(u32 *)(CONFIG_LPM_TIMER_WAKEUP) &= ~(1 << cpu);

	cpu = cpu % 2;
	/* disable and clear pending interrupt status */
	__raw_writel(0x0, timer_phy_base + TMR_IER(cpu));
	__raw_writel(0x1, timer_phy_base + TMR_ICR(cpu));
}

static const void *APMU_CORE_IDLE_CFG[] = {
	(void *)PMU_CORE0_IDLE_CFG, (void *)PMU_CORE1_IDLE_CFG,
	(void *)PMU_CORE2_IDLE_CFG, (void *)PMU_CORE3_IDLE_CFG,
};

static const void *APMU_MP_IDLE_CFG[] = {
	(void *)PMU_MP_IDLE_CFG0, (void *)PMU_MP_IDLE_CFG1,
	(void *)PMU_MP_IDLE_CFG2, (void *)PMU_MP_IDLE_CFG3,
};

static const void *ICU_GBL_INT_MSK[] = {
	(void *)ICU_APC0_GBL_INT_MSK, (void *)ICU_APC1_GBL_INT_MSK,
	(void *)ICU_APC2_GBL_INT_MSK, (void *)ICU_APC3_GBL_INT_MSK,
};

static void lpm_debug(void)
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

	printf("APCR\t");
	printf("0x%08x\n", __raw_readl(MPMU_APCR));

	printf("CPCR\t");
	printf("0x%08x\n", __raw_readl(MPMU_CPCR));

	printf("WARM_RESET_VECTOR\t");
	printf("0x%08x\n", __raw_readl(CIU_WARM_RESET_VECTOR));

	printf("GNSS VOTING\t");
	printf("0x%08x\n", __raw_readl(PMU_DEBUG2_REG));

	printf("SCCR\t");
	printf("0x%08x\n", __raw_readl(MPMU_SCCR));
}

static void pxa1908_lowpower_config(u32 cpu,
			u32 power_state, u32 lowpower_enable)
{
	u32 core_idle_cfg, mp_idle_cfg, apcr;

	core_idle_cfg = __raw_readl(APMU_CORE_IDLE_CFG[cpu]);
	mp_idle_cfg = __raw_readl(APMU_MP_IDLE_CFG[cpu]);
	apcr = __raw_readl(MPMU_APCR);

	if (lowpower_enable) {
		switch (power_state) {
		/* Chip Level Power Modes */
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
			/* AP subsystem should shutdown before chip level lpms */
			apcr |= PMUM_SLPEN | PMUM_DDRCORSD | PMUM_AXISD;
			mp_idle_cfg |= PMUA_MP_POWER_DOWN | PMUA_MP_IDLE;
			core_idle_cfg |= PMUA_CORE_POWER_DOWN | PMUA_CORE_IDLE;
			break;

		/* AP SUB SYSTEM Power Modes */
		case POWER_MODE_APPS_SLEEP_UDR:
			apcr |= PMUM_STBYEN;
			/* fall through */
		case POWER_MODE_APPS_SLEEP:
			apcr |= PMUM_SLPEN;
			/* fall through */
		case POWER_MODE_APPS_IDLE_DDR:
			apcr |= PMUM_DDRCORSD;
			/* fall through */
		case POWER_MODE_APPS_IDLE:
			apcr |= PMUM_AXISD;
			/* MP should shutdown before AP subsystem power modes */
			mp_idle_cfg |= PMUA_MP_POWER_DOWN | PMUA_MP_IDLE;
			core_idle_cfg |= PMUA_CORE_POWER_DOWN | PMUA_CORE_IDLE;
			break;

		/* MP Level Power Modes */
		case POWER_MODE_MP_PWRDOWN_L2_OFF:
			mp_idle_cfg |= PMUA_MP_L2_SRAM_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_PWRDOWN:
			mp_idle_cfg |= PMUA_MP_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_IDLE_CORE_PWRDOWN:
			core_idle_cfg |= PMUA_CORE_POWER_DOWN;
			/* fall through */
		case POWER_MODE_MP_IDLE_CORE_EXTIDLE:
			mp_idle_cfg |= PMUA_MP_IDLE;
			/* Core should be idle before MP level power modes */
			core_idle_cfg |= PMUA_CORE_IDLE;
			break;

		/* Core level Power Modes */
		case POWER_MODE_CORE_PWRDOWN:
			/* fall through */
			core_idle_cfg |= PMUA_CORE_POWER_DOWN;
		case POWER_MODE_CORE_EXTIDLE:
			core_idle_cfg |= PMUA_CORE_IDLE;
			/* fall through */
		case POWER_MODE_CORE_INTIDLE:
			break;
		default:
			printf("Invalid power state!\n");
		}
	} else {
		core_idle_cfg &= ~(PMUA_CORE_IDLE | PMUA_CORE_POWER_DOWN);
		mp_idle_cfg &= ~(PMUA_MP_IDLE | PMUA_MP_POWER_DOWN |
				PMUA_MP_L2_SRAM_POWER_DOWN);
		apcr &= ~(PMUM_DDRCORSD | PMUM_APBSD | PMUM_AXISD |
			PMUM_VCTCXOSD | PMUM_STBYEN | PMUM_SLPEN);
	}
	/* set DSPSD, DTCMSD, BBSD, MSASLPEN */
	apcr |= PMUM_DTCMSD | PMUM_BBSD | PMUM_MSASLPEN;

	__raw_writel(core_idle_cfg, APMU_CORE_IDLE_CFG[cpu]);
	__raw_writel(mp_idle_cfg, APMU_MP_IDLE_CFG[cpu]);
	__raw_writel(apcr, MPMU_APCR);
}

static void pxa1908_gic_global_mask(u32 cpu, u32 mask)
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

static void pxa1908_icu_global_mask(u32 cpu, u32 mask)
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

static void pxa1908_pre_enter_lpm(u32 cpu, u32 power_mode)
{
	pxa1908_lowpower_config(cpu, power_mode, 1);

	/* Mask GIC global interrupt */
	pxa1908_gic_global_mask(cpu, 1);
	/* Mask ICU global interrupt */
	pxa1908_icu_global_mask(cpu, 1);
}

static void pxa1908_post_enter_lpm(u32 cpu, u32 power_mode)
{
	/* Unmask GIC interrtup */
	pxa1908_gic_global_mask(cpu, 0);
	/* Mask ICU global interrupt */
	pxa1908_icu_global_mask(cpu, 1);

	pxa1908_lowpower_config(cpu, power_mode, 0);
}


int do_lpm_debug(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	lpm_debug();
	return 0;
}

U_BOOT_CMD(
	lpm_debug,	1,	1,	do_lpm_debug,
	"lpm debug",
	""
);

static void do_c12(void)
{
	int cpu = smp_hw_cpuid();

	/* Mask ICU global interrupt, ICU could wakeup WFI */
	pxa1908_icu_global_mask(cpu, 0);

	pxa1908_lowpower_config(cpu, POWER_MODE_CORE_INTIDLE, 1);

	core_wfi();

	pxa1908_lowpower_config(cpu, POWER_MODE_CORE_INTIDLE, 0);
}

static void do_c13(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_lowpower_config(cpu, POWER_MODE_CORE_EXTIDLE, 1);

	core_wfi();

	pxa1908_lowpower_config(cpu, POWER_MODE_CORE_EXTIDLE, 0);
}

static void do_c22(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_CORE_PWRDOWN);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_CORE_PWRDOWN);
}

void do_m11_c13(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_lowpower_config(cpu, POWER_MODE_MP_IDLE_CORE_EXTIDLE, 1);

	core_wfi();

	pxa1908_lowpower_config(cpu, POWER_MODE_MP_IDLE_CORE_EXTIDLE, 0);
}

void do_m11_c22(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_MP_IDLE_CORE_PWRDOWN);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_MP_IDLE_CORE_PWRDOWN);
}

void do_m21(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN);
}

void do_m22(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN_L2_OFF);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_MP_PWRDOWN_L2_OFF);
}

static void do_d1p(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_APPS_IDLE);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_APPS_IDLE);
}

static void do_d1pddr(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_APPS_IDLE_DDR);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_APPS_IDLE_DDR);
}

static void do_d1pp(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_APPS_SLEEP);
	/* Need ASYNC enable here */
	__raw_writel(PMUM_AP_ASYNC_INT, MPMU_AWUCRM);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_APPS_SLEEP);
}

static void do_d1ppstdby(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_APPS_SLEEP_UDR);
	/* Need ASYNC enable here */
	__raw_writel(PMUM_AP_ASYNC_INT, MPMU_AWUCRM);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_APPS_SLEEP_UDR);
}

static void do_d1(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_SYS_SLEEP);
	/* Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_SYS_SLEEP);
}

static void do_d2(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_ON_L2_ON);
	/* Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_ON_L2_ON);
}

static void do_d2p(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_ON);
	/* Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_ON);
}

static void do_d2pp(void)
{
	int cpu = smp_hw_cpuid();

	pxa1908_pre_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF);
	/* Need to set wake up source here */
	__raw_writel(LPM_WAKEUP, MPMU_AWUCRM);
	__raw_writel(__raw_readl(MPMU_APCR) & 0xff087fff, MPMU_APCR);

	core_pwr_down();

	pxa1908_post_enter_lpm(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF);
}

/* Handle interrupt in el2 level */
static inline void ulc_hcr_init(void)
{
	unsigned int v;
	asm volatile(
	"	mrs	x0, hcr_el2\n"
	"	orr	x0, x0, #(1 << 4)\n"
	"	msr	hcr_el2, x0\n"
	: "=&r" (v) : : "cc");
}

int do_lpm_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i = 0;
	int seconds;
	int cpu = smp_hw_cpuid();
	static u32 idle_counts;

	idle_counts++;

	pxa1908_lowpower_config(cpu, POWER_MODE_UDR_VCTCXO_OFF_L2_OFF, 0);

	if (argc < 2)
		return 0;

	if (idle_counts == 1) {
		/*
		 * If gic is not used, bit6 of GICC_CTRL must be
		 * cleared to bypass GIC so that ICU signal can
		 * directly routed to CPU interface.
		 */
		__raw_writel(0x0, GICC_CTRL);
		/*
		 * The default timer interrupt is handled in el1.
		 * But current uboot runs in el2, so we need to
		 * configure hcr_el2 reg to handle interrupt in
		 * el2. Or core can't be woke up by interrupt.
		 */
		ulc_hcr_init();

		printf("timer init!\n");
		icu_enable_timer0_1();
		timer_config(cpu);
		/*
		 * need this udelay otherwise the first time
		 * reading timer0 not correct
		 * Avoid to use udelay to corrupt timer.
		 */
		while (i < 3000000)
			i++;
	}

	if (argc == 2)
		seconds = 10;
	else
		seconds = simple_strtoul(argv[2], NULL, 10);

	printf("timer interrupt will come after %d seconds\n", seconds);
	timer_set_match(cpu, seconds);

	printf("%u: core %u enter %s\n",
	       idle_counts, smp_hw_cpuid(), argv[1]);

	if (strcmp("c12", argv[1]) == 0)
		do_c12();
	else if (strcmp("c13", argv[1]) == 0)
		do_c13();
	else if (strcmp("c22", argv[1]) == 0)
		do_c22();
	else if (strcmp("m11_c13", argv[1]) == 0)
		do_m11_c13();
	else if (strcmp("m11_c22", argv[1]) == 0)
		do_m11_c22();
	else if (strcmp("m21", argv[1]) == 0)
		do_m21();
	else if (strcmp("m22", argv[1]) == 0)
		do_m22();
	else if (strcmp("d1p", argv[1]) == 0)
		do_d1p();
	else if (strcmp("d1pddr", argv[1]) == 0)
		do_d1pddr();
	else if (strcmp("d1pp", argv[1]) == 0)
		do_d1pp();
	else if (strcmp("d1ppstdby", argv[1]) == 0)
		do_d1ppstdby();
	else if (strcmp("d1", argv[1]) == 0)
		do_d1();
	else if (strcmp("d2", argv[1]) == 0)
		do_d2();
	else if (strcmp("d2p", argv[1]) == 0)
		do_d2p();
	else if (strcmp("d2pp", argv[1]) == 0)
		do_d2pp();
	else
		printf("Unsupported power state!\n");

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
	"   c12: WFI\n"
	"   c13: core external idle\n"
	"   c22: core power down\n"
	"   m11_c13: all cores in C13 and MP internal idle\n"
	"   m11_c22: all cores in C22 and MP internl idle\n"
	"   m21: MP power down and SCU off, but l2 still on\n"
	"   m22: MP in M21 and L2 off\n"
	"   d1p: axi vote\n"
	"   d1pddr: axi and ddr vote\n"
	"   d1pp: axi, ddr, ap sleep vote\n"
	"   d1ppstdby: axi, ddr, ap sleep, ap standby vote\n"
	"   d1: axi, ddr, ap sleep and apb vote\n"
	"   d2: axi, ddr, ap sleep, apb and ap standby vote\n"
	"   d2p: axi, ddr, ap sleep, apb, ap standby and vctcxo vote\n"
	"   d2pp: axi, ddr, ap sleep, apb, ap standby and vctcxo vote,L2 off\n"
);

int do_core_busy(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cur_cpu = smp_hw_cpuid();

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
		 * Set 19bit to 0x0, will enalbe MCK dyn clk gate.
		 */
		*(unsigned int *)(CIU_REG(0x040)) &= ~(0x1 << 19);
		*(unsigned int *)(CIU_REG(0x040)) |= (0xff << 8);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x7 << 16);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x3 << 20);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x3 << 26);
		*(unsigned int *)(CIU_REG(0x040)) |= (0x3 << 29);
	}

	/* on1, on2, off time */
	*(unsigned int *)(APMU_REG(0x0dc)) = 0x20001fff;
	printf("0xd42828dc:0x%08x\n", __raw_readl(APMU_REG(0x0dc)));
	/* VPU off */
	if (cond & DEADIDLE_VPU) {
		printf("VPU Off\n");
		/* 1. Enable VPU Isolation */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x1 << 16);
		/* 2. Assert bus reset, func and AHB reset */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x7 << 0);
		/* 3. Reset bus clock enable and func and AHB clock enable */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x7 << 3);
		/* 4. Set Dx8 in HW mod */
		*(unsigned int *)(APMU_REG(0x0A4)) |= (0x1 << 19);
		/* 5. ref Clock Reset */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x1 << 24);
		/* 6. ref Clock Disable */
		*(unsigned int *)(APMU_REG(0x0A4)) &= ~(0x1 << 25);
		/* 7. power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1 << 2);
		printf("0xd42828a4:0x%08x\n", __raw_readl(APMU_REG(0x0a4)));
	}

	/*  GC off */
	if (cond & DEADIDLE_GPU) {
		printf("GC_3D Off\n");
		/* 1. Enable GC3D Isolation */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x1 << 8);
		/* 2. Assert shader clock reset */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x1 << 24);
		/* 3. Disable shader clock */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x1 << 25);
		/* 4. Assert bus, func and HCLK reset */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x7 << 0);
		/* 5. Reset bus, func and HCLK clock enable */
		*(unsigned int *)(APMU_REG(0x0CC)) &= ~(0x7 << 3);
		/* 6. Set GC in HW mod */
		*(unsigned int *)(APMU_REG(0x0CC)) |= (0x1 << 11);
		/* 7. GC 3D power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1 << 0);
		printf("0xd42828cc:0x%08x\n", __raw_readl(APMU_REG(0x0cc)));

		printf("GC_2D Off\n");
		/* 1. Enable GC2D Isolation */
		*(unsigned int *)(APMU_REG(0x0F4)) &= ~(0x1 << 8);
		/* 2. Assert bus, func and HCLK reset */
		*(unsigned int *)(APMU_REG(0x0F4)) &= ~(0x7 << 0);
		/* 3. Reset bus, func and HCLK clock enable */
		*(unsigned int *)(APMU_REG(0x0F4)) &= ~(0x7 << 3);
		/* 4. Set GC2D in HW mod */
		*(unsigned int *)(APMU_REG(0x0F4)) |= (0x1 << 11);
		/* 5. GC 2D power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1 << 6);
		printf("0xd42828f4:0x%08x\n", __raw_readl(APMU_REG(0x0f4)));
	}

	/* ISP off */
	if (cond & DEADIDLE_ISP) {
		printf("ISP Off\n");

		rstbit = (1 << 0) | (1 << 8) | (1 << 16) | (1 << 27);
		enbit = (1 << 1) | (1 << 17) | (1 << 28);
		/* 1. Enable ISP Isolation */
		*(unsigned int *)(APMU_REG(0x038)) &= ~(1 << 12);
		/* 2. Assert func, AXI and AHB reset */
		*(unsigned int *)(APMU_REG(0x038)) &= ~rstbit;
		/* 3. Reset func, AXI and AHB clock enable */
		*(unsigned int *)(APMU_REG(0x038)) &= ~enbit;
		/* 4. Set ISP in HW mod */
		*(unsigned int *)(APMU_REG(0x038)) |= (0x1 << 15);
		/* 5. power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(0x1 << 4);
		printf("0xd4282838:0x%08x\n", __raw_readl(APMU_REG(0x038)));
	}

	if (cond & DEADIDLE_GNSS) {
		printf("GNSS Off\n");
		/* 1. Enable Isolation */
		*(unsigned int *)(APMU_REG(0x0FC)) &= ~(1 << 2);
		/* 2. Assert reset */
		*(unsigned int *)(APMU_REG(0x0FC)) &= ~(1 << 1);
		/* 3. Set in HW mod */
		*(unsigned int *)(APMU_REG(0x0FC)) |= (1 << 0);
		/* 4. power off */
		*(unsigned int *)(APMU_REG(0x0d8)) &= ~(1 << 8);
		printf("0xd42828fc:0x%08x\n", __raw_readl(APMU_REG(0x0fc)));
	}
	printf("0xd42828d8:0x%08x\n", __raw_readl(APMU_REG(0x0d8)));

	/* other devs */
	if (cond & DEADIDLE_OTHERPERI) {
		printf("Other devices reset\n");

		/* USB */
		printf("USB reset\n");
		*(unsigned int *)(APMU_REG(0x034)) = 0x0;
		printf("0xd4282834:0x%08x\n", __raw_readl(APMU_REG(0x034)));
		*(unsigned int *)(APMU_REG(0x05C)) = 0x0;
		printf("0xd428285c:0x%08x\n", __raw_readl(APMU_REG(0x05c)));

		/* HSI/LTE_DMA */
		printf("HSI/LTE_DMA reset\n");
		*(unsigned int *)(APMU_REG(0x048)) = 0x0;
		printf("0xd4282848:0x%08x\n", __raw_readl(APMU_REG(0x048)));

		/* SRAM */
		printf("SRAM Power Down\n");
		/* Config L2 SRAM to HW control */
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3 << 18);
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3 << 20);
		/* Config L1 SRAM to HW control */
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3 << 8);
		*(unsigned int *)(APMU_REG(0x08C)) &= ~(0x3 << 0);
		/* SQU SRAM */
		*(unsigned int *)(APMU_REG(0x08C)) |= (0x3 << 16);
		printf("0xd428288c:0x%08x\n", __raw_readl(APMU_REG(0x08c)));

		/* LCD */
                printf("LCD clk gate reset\n");
                /* hclk reset */
                *(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1 << 2);
                /* LCD_GNSS_ACLK_RST */
		*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1 << 16);
                /* axi reset */
		*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1 << 0);
		/* fclk reset */
		*(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1 << 1);
		/* escape clock reset */ 
                *(unsigned int *)(APMU_REG(0x044)) &= ~(0x1 << 3);
		udelay(100);
		/* gate ahb clock */
                *(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1 << 4);
		/* gate axi clock */
                *(unsigned int *)(APMU_REG(0x04C)) &= ~(0x1 << 3);
                /* gate func clock */
                *(unsigned int *)(APMU_REG(0x04C)) &= ~(0xf << 5);
                /* gate escape clock */
		*(unsigned int *)(APMU_REG(0x044)) &= ~(0x1 << 2);
                udelay(100);
		printf("0xd428284c:0x%08x\n", __raw_readl(APMU_REG(0x04c)));
                printf("0xd4282844:0x%08x\n", __raw_readl(APMU_REG(0x044)));

		/* ISCCR0/1 */
		printf("I2S clk gate reset\n");
		/* sysclk, baseclk disable */
		*(unsigned int *)(MPMU_REG(0x040)) &= ~(0x1 << 31);
		*(unsigned int *)(MPMU_REG(0x040)) &= ~(0x1 << 29);
		printf("0xd4050040:0x%08x\n", __raw_readl(MPMU_REG(0x040)));
		/* sysclk, baseclk disable */
		*(unsigned int *)(MPMU_REG(0x044)) &= ~(0x1 << 31);
		*(unsigned int *)(MPMU_REG(0x044)) &= ~(0x1 << 29);
		printf("0xd4050044:0x%08x\n", __raw_readl(MPMU_REG(0x044)));

		/* CCIC */
		/* CCIC all dynamic change enalbe */
		*(unsigned int *)(APMU_REG(0x028)) = 0x0;
		printf("0xd4282828:0x%08x\n", __raw_readl(APMU_REG(0x028)));

		rstbit = (1 << 1) | (1 << 2) | (1 << 22);
		enbit = (1 << 4) | (1 << 5) | (1 << 21);
		*(unsigned int *)(APMU_REG(0x050)) &= ~rstbit;
		*(unsigned int *)(APMU_REG(0x050)) &= ~enbit;
		printf("0xd4282850:0x%08x\n", __raw_readl(APMU_REG(0x050)));

		/* CCIC2 */
		rstbit = (1 << 1) | (1 << 2);
		enbit = (1 << 4) | (1 << 5) | (1 << 26);
		*(unsigned int *)(APMU_REG(0x024)) &= ~rstbit;
		*(unsigned int *)(APMU_REG(0x024)) &= ~enbit;
		printf("0xd4282824:0x%08x\n", __raw_readl(APMU_REG(0x024)));

		/* SDH0/1/2 */
		printf("SDH clk gate reset\n");
		/* reset and disable */
		/* SDH0 Reset and Disable */
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1 << 1);
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1 << 4);
		/* SDH1 Reset and Disable */
		*(unsigned int *)(APMU_REG(0x058)) &= ~(0x1 << 1);
		*(unsigned int *)(APMU_REG(0x058)) &= ~(0x1 << 4);
		/* SDH2 Reset and Disable */
		*(unsigned int *)(APMU_REG(0x0e0)) &= ~(0x1 << 1);
		*(unsigned int *)(APMU_REG(0x0e0)) &= ~(0x1 << 4);
		/* ALL SDH AXI Reset and Disable */
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1 << 0);
		*(unsigned int *)(APMU_REG(0x054)) &= ~(0x1 << 3);
		printf("0xd4282854:0x%08x\n", __raw_readl(APMU_REG(0x054)));
		printf("0xd4282858:0x%08x\n", __raw_readl(APMU_REG(0x058)));
		printf("0xd42828e0:0x%08x\n", __raw_readl(APMU_REG(0x0e0)));

		/* NF */
		printf("NF clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x060)) = 0x0;
		printf("0xd4282860:0x%08x\n", __raw_readl(APMU_REG(0x060)));

		/* AES */
		printf("AES clk gate reset\n");
		/*
		 * In PXA1U88, reset AES clk will not decrease but increase
		 * VCC_MAIN current. Thus we only shutdown its clk.
		 */
		*(unsigned int *)(APMU_REG(0x068)) = 0x50;
		printf("0xd4282868:0x%08x\n", __raw_readl(APMU_REG(0x068)));

		/* DTC */
		printf("DTC clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x0ac)) = 0x0;
		printf("0xd42828ac:0x%08x\n", __raw_readl(APMU_REG(0x0ac)));

		/* SMC */
		printf("SMC clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x0d4)) &= ~(0x3 << 0);
		*(unsigned int *)(APMU_REG(0x0d4)) &= ~(0x3 << 3);
		printf("0xd42828d4:0x%08x\n", __raw_readl(APMU_REG(0x0d4)));

		/* MCK AHB HCLK */
		printf("MCK AHB HCLK clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x0e8)) = 0x0;
		printf("0xd42828e8:0x%08x\n", __raw_readl(APMU_REG(0x0e8)));

		/* trace reset  */
		printf("Trace clk gate power off\n");
		*(unsigned int *)(APMU_REG(0x108)) &= ~(0x1 << 16);
		*(unsigned int *)(APMU_REG(0x108)) &= ~(0x3 << 3);
		printf("0xd4282908:0x%08x\n", __raw_readl(APMU_REG(0x108)));

		/* ACLK gate enable  */
		printf("ACLK gate enable\n");
		/* Gated or not no influnce */
		/* *(unsigned int *)(CIU_REG(0x05c)) |= (0x1<<9); */

		/* DCLK gate enable  */
		printf("DCLK gate enable\n");
		/* Gated or not no influnce */
		/* *(unsigned int *)(CIU_REG(0x05c)) &= ~(0x1<10); */
		printf("0xd4282c5c:0x%08x\n", __raw_readl(CIU_REG(0x05c)));

		/* SQU dynamic clk gate enable  */
		printf("SQU gate enable\n");
		*(unsigned int *)(APMU_REG(0x01c)) &= ~(0x3fffffff << 0);
		printf("0xd428281c:0x%08x\n", __raw_readl(APMU_REG(0x01c)));

		/* APB */
		printf("APB\n");

		printf("APB UART1\n");
		*(unsigned int *)(APBC_REG(0x0004)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0004)) |= (0x1 << 2);

		printf("APB GPIO\n");
		*(unsigned int *)(APBC_REG(0x0008)) &= ~(0x3 << 0);

		printf("APB PWM0\n");
		*(unsigned int *)(APBC_REG(0x000c)) |= (0x1 << 2);
		*(unsigned int *)(APBC_REG(0x000c)) &= ~(0x3 << 0);

		printf("APB PWM1\n");
		*(unsigned int *)(APBC_REG(0x0010)) |= (0x1 << 2);
		*(unsigned int *)(APBC_REG(0x0010)) &= ~(0x2 << 0);

		printf("APB PWM2\n");
		*(unsigned int *)(APBC_REG(0x0014)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0014)) |= (0x1 << 2);

		printf("APB PWM3\n");
		*(unsigned int *)(APBC_REG(0x0018)) &= ~(0x2 << 0);
		*(unsigned int *)(APBC_REG(0x0018)) |= (0x1 << 2);

		printf("APB SSP0\n");
		*(unsigned int *)(APBC_REG(0x001c)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x001c)) |= (0x1 << 2);

		printf("APB SSP1\n");
		*(unsigned int *)(APBC_REG(0x0020)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0020)) |= (0x1 << 2);

		printf("APB IPC\n");
		*(unsigned int *)(APBC_REG(0x0024)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0024)) |= (0x1 << 2);

		printf("APB RTC\n");
		*(unsigned int *)(APBC_REG(0x0028)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0028)) |= (0x1 << 2);
		*(unsigned int *)(APBC_REG(0x0028)) &= ~(0x1 << 7);

		printf("APB TWSI\n");
		*(unsigned int *)(APBC_REG(0x002c)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x002c)) |= (0x1 << 2);

		printf("APB KPC\n");
		*(unsigned int *)(APBC_REG(0x0030)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0030)) |= (0x1 << 2);

		printf("APB TIMERS\n");
		*(unsigned int *)(APBC_REG(0x0044)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0044)) |= (0x1 << 2);

		printf("APB TB_ROTARY\n");
		*(unsigned int *)(APBC_REG(0x0038)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0038)) |= (0x1 << 2);

		printf("APB ATB\n");
		*(unsigned int *)(APBC_REG(0x003c)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x003c)) |= (0x1 << 2);

		printf("APB SW_JTAG\n");
		*(unsigned int *)(APBC_REG(0x0040)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0040)) |= (0x1 << 2);
		/*
		 * Do not shutdown timer1 clk if it has already been
		 * set to wakeup cpu0/1
		 */
		if ((*(u32 *)CONFIG_LPM_TIMER_WAKEUP &
			(1 << 0 | 1 << 1)) == 0) {
			printf("APB TIMERS2\n");
			*(unsigned int *)(APBC_REG(0x0068)) &= ~(0x3 << 0);
			*(unsigned int *)(APBC_REG(0x0068)) |= (0x1 << 2);
		}

		printf("APB ONEWIRE\n");
		*(unsigned int *)(APBC_REG(0x0048)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0048)) |= (0x1 << 2);

		printf("APB SSP2\n");
		*(unsigned int *)(APBC_REG(0x004c)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x004c)) |= (0x1 << 2);

		printf("APB DRO_TS\n");
		*(unsigned int *)(APBC_REG(0x0058)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0058)) |= (0x1 << 2);

		printf("APB I2C1\n");
		*(unsigned int *)(APBC_REG(0x0060)) &= ~(0x3 << 0);
		*(unsigned int *)(APBC_REG(0x0060)) |= (0x1 << 2);

		/*
		 * Do not shutdown timer2 clk if it has already been
		 * set to wakeup cpu2/3.
		 */
		if ((*(u32 *)CONFIG_LPM_TIMER_WAKEUP &
		     (1 << 2 | 1 << 3)) == 0) {
			printf("APB TIMERS0\n");
			*(unsigned int *)(APBC_REG(0x0034)) &= ~(0x3 << 0);
			*(unsigned int *)(APBC_REG(0x0034)) |= (0x1 << 2);
		}

		/* hardware dynamic clk gate */
		printf("Hardware dynamic clk gate reset\n");
		*(unsigned int *)(APMU_REG(0x040)) = 0x0;

		printf("APB UART0 not shut down.\n");
		/*
		 *(unsigned int *)(APBC_REG(0x0000)) &= ~(0x3<<0);
		 *(unsigned int *)(APBC_REG(0x0000)) |= (0x1<<2);
		 */
	}
}

int do_deadidle(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t reg, condition;

	if (argc == 1)
		condition = DEADIDLE_ALL;
	else if (argc == 2)
		condition = simple_strtoul(argv[1], NULL, 10);
	else {
		printf("Wrong parameters!\n");
		return 0;
	}

	printf("Start deadidle....\n");
	pxa1x88_deadidle(condition);
	printf("done. WARNING: Type this command before the last cpu!\n");

	reg = __raw_readl(0xd42828f0);
	if (reg & (1 << 9))
		printf("GNSS_SD is still power on!\n");
	if (reg & (1 << 8))
		printf("GNSS is still power on!\n");
	if (reg & (1 << 6))
		printf("2D GPU is still power on!\n");
	if (reg & (1 << 4))
		printf("ISP is still power on!\n");
	if (reg & (1 << 2))
		printf("VPU is still power on!\n");
	if (reg & (1 << 0))
		printf("GPU is still power on!\n");

	return 0;
}

U_BOOT_CMD(
	deadidle,       2,      1,      do_deadidle,
	"ready to do deadidle",
	"[ op number ]"
);

uint32_t seagull_loop_ops[] = {0xe1a00000, 0xeafffffd};

uint32_t seagull_wfi_op[] = {
	0xea000006, 0xea00000f, 0xea00000e, 0xea00000d,
	0xea00000c, 0xea00000b, 0xea00000a, 0xea000009,
	0xe3a004d1, 0xe3800902, 0xe3800028, 0xe3a01001,
	0xe5801000, 0xe3a00335, 0xe3800805, 0xe3800024,
	0xe3a01000, 0xe5801000, 0xee074f90, 0xeafffffd
};

uint32_t msa_d2_ops[] = {
	0x9ffce109, 0xd100e149, 0x327e3271, 0x0000e19c,
	0x0000e19d, 0x0000e19e, 0x0000e19f, 0x0000e10a,
	0xd407e14a, 0x60029111, 0x0044e102, 0x3221504a,
	0x00000064, 0x00000000, 0x00000000, 0x00000000,
	0x00002000, 0xe8000147, 0xe1120007, 0xe152200c,
	0x63faffa8, 0x00249f12, 0x0100e111, 0xf00ee151,
	0xbbc09d08, 0xe100b9c1, 0x42800000, 0x01c0e140,
	0x9f085601, 0x20010024, 0x0118e110, 0xf00ee150,
	0xbbd09d00, 0xe181b9d0, 0x540800e0, 0x14020808,
	0x2ff32002, 0x2000e10a, 0xffa8e14a, 0x5000e100,
	0xfc08e140, 0x00249310, 0x8010e109, 0xd100e149,
	0x93086000, 0x5050e108, 0xffd0e148, 0x9303600b,
	0x9d0b0024, 0xb9e7bbe3, 0xfffce103, 0xe143429b,
	0x54dffe3f, 0x9f0b4a03, 0x20010024, 0xbbf39d03,
	0x54cbb9f3, 0x14020c03, 0x2ff92002, 0x9f104ae0,
	0xe1000024, 0xe1405008, 0x9310fe08, 0x00200024,
	0x00000024, 0x00000000, 0x00000000, 0x60100000,
	0x9f129308, 0x20000024
};

uint32_t msa_loop_ops[] = {0x64086000, 0x00002fff};

const void *cp_wfi_op = (void *)(0xd4282c24);

void cp_msa_sleep(void)
{
	uint32_t reg;
	uint32_t timeout = 0;

	/* Check CP and MSA power status */
	reg = __raw_readl(0xd4051004);
	if ((reg & (1 << 31)) && (reg & (1 << 29))) {
		printf("CP and MSA are already in D2!\n");
		return;
	}

	/* Vote MSA, CP idle bits in APCR */
	*(uint32_t *)(0xd4051000) = 0x12004000;
	/*
	 * CPCR: must disable wakeup port0(GSM) and port1(TD) since
	 * there will be a constant GSM/TD wakeup event after board
	 * boots up.
	 * Pls note for Helan2 and ULC, port5 must be enabled since
	 * it's used for GPS wakeup.
	 */
	*(uint32_t *)(0xD4050000) = 0xBEC87008;
	/* POCR */
	*(uint32_t *)(0xD405000c) = 0x001F0500;
	/*
	 * Always set SCCR bit0 and bit2.
	 * Bit0: Select 32k clk source. value 0 means it's from vctcxo
	 * division and 1 means it comes from external PMIC clk. If use
	 * value 0, system can't be woken up when vctcxo is off;
	 * Bit2: Force baseband enters sleep mode. If not set, system
	 * can't enter D1.
	 */
	*(uint32_t *)(0xD4050038) = 0x00000005;
	/*
	 * PMUA_CP_IDLE_CFG, set 0x803FC720 (both retain),
	 * set 0x803f0760 for both power down, [15:14]-L2,
	 * [6]-L1
	 */
	*(uint32_t *)(0xd4282814) = 0x803fc720;
	/* ICU_CP_GBL_INT_MSK */
	*(uint32_t *)(0xd4282218) |= 0x3;
	/*
	 * ulc uses 64bit bootrom which doesn't support 32bit seagull
	 * execution. Thus when segaull releases, it can't jump to
	 * bootrom address.
	 * In ulc, we use SEAGULL_REMAP reg to remap seagull boot up
	 * address. With this method, unlike in helan2, seagull in ulc
	 * will not jump to bootrom when power up. Instead it will go
	 * to the address 0xSEAGULL_REMAP[bit31~16],0000. The latter
	 * 4 zeros means the address MUST be 64K align.
	 */
	memcpy((void *)0x6000000, seagull_wfi_op, (ARRAY_SIZE(seagull_wfi_op) * 4));
	flush_cache((unsigned long)(0x6000000), 0x100);
	printf("SEAGULL_REMAP is 0x%x\n", *(uint32_t *)(0xd4282c94));
	/* Set MSA boot address */
	*(uint32_t *)(0xd4282d24) =
		(((uint32_t)(u64)(&msa_d2_ops[0])) & 0xFC000000) >> 26;
	*(uint32_t *)(0xd4070000) =
		0xd8000000 + (((uint32_t)(u64)(&msa_d2_ops[0])) & 0x3FFFFFF);
	/* release CP and Vote MSA SS available */
	printf("Release Seagull and MSA now...\r\n");
	*(uint32_t *)(0xd4051020) &= (~0x5);
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

	printf("Wait for done...\r\n");

	timeout = 1000;
	/* until D2 is really entered */
	while (!(__raw_readl(0xd4051004) & (1 << 29))) {
		cpu_relax();
		if (--timeout == 0)
			break;
		udelay(1000);
	}
	if (timeout != 0)
		printf("CP is in idle!\n");
	else
		printf("Warning: CP is NOT in idle!\n");

	timeout = 1000;
	while (!(__raw_readl(0xd4051004) & (1 << 31))) {
		cpu_relax();
		if (--timeout == 0)
			break;
		udelay(1000);
	}
	if (timeout != 0)
		printf("MSA is in idle!\n");
	else
		printf("Warning: MSA is NOT in idle!\n");
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

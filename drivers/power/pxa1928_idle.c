/*
 *  U-Boot command for LPM test support
 *
 *  Copyright (C) 2008, 2009 Marvell International Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <linux/types.h>

DECLARE_GLOBAL_DATA_PTR;
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

#define AXI_PHYS_BASE			(0xd4200000ul)
#define APB_PHYS_BASE			(0xd4000000ul)
#define APBC_PHYS_BASE			(APB_PHYS_BASE + 0x015000)
#define APBC_REG(x)			(APBC_PHYS_BASE + (x))

#define PMUA_PHYS_BASE			(AXI_PHYS_BASE + 0x82800)
#define PMUA_REG(x)			(PMUA_PHYS_BASE + (x))

#define PMUM_PHYS_BASE			(APB_PHYS_BASE + 0x50000)
#define PMUM_REG(x)			(PMUM_PHYS_BASE + (x))

#define MCU_PHYS_BASE			(0xd0000000ul)
#define MCU_REG(x)			(MCU_PHYS_BASE + (x))

#define ICU_PHYS_BASE			(AXI_PHYS_BASE + 0x84000)
#define ICU_REG(x)			(ICU_PHYS_BASE + (x))

#define PGU_PHYS_BASE			(0xd1e00000ul)
#define GIC_DIST_PHYS_BASE		(PGU_PHYS_BASE + 0x1000)
#define GIC_CPU_PHYS_BASE		(PGU_PHYS_BASE + 0x2000)

#define PMUA_MC_CLK_RES_CTRL		PMUA_REG(0x0258)
#define PMUA_COREAP0_PWRMODE		PMUA_REG(0x0280)
#define PMUA_COREAP1_PWRMODE		PMUA_REG(0x0284)
#define PMUA_COREAP2_PWRMODE		PMUA_REG(0x0288)
#define PMUA_COREAP3_PWRMODE		PMUA_REG(0x028c)

static const void  *PMUA_COREAP_PWRMODE[] = {
	(void *)PMUA_COREAP0_PWRMODE, (void *)PMUA_COREAP1_PWRMODE,
	(void *)PMUA_COREAP2_PWRMODE, (void *)PMUA_COREAP3_PWRMODE,
};

#define CPU_PWRMODE			((0x3) << 0)
#define CPU_PWRMODE_WFI			((0x0) << 0)
#define CPU_PWRMODE_C1			((0x1) << 0)
#define CPU_PWRMODE_C2			((0x3) << 0)


#define L2SR_PWROFF_VT			((0x1) << 4)
#define L2SR_PWROFF_VT_M2		((0x0) << 4)
#define L2SR_PWROFF_VT_M4		((0x1) << 4)

#define CORESS_PWRMODE_VT		((0x3) << 8)
#define CORESS_PWRMODE_VT_MP0		((0x0) << 8)
#define CORESS_PWRMODE_VT_MP1		((0x2) << 8)
#define CORESS_PWRMODE_VT_MP2		((0x3) << 8)

#define MSK_CPU_IRQ			((0x1) << 12)
#define MSK_CPU_FIQ			((0x1) << 13)

#define LPM_STATUS			((0x1) << 25)
#define POWER_ON_STATUS			((0x1) << 26)

#define CLR_C2_STATUS			((0x1) << 30)

#define PMUM_PCR_PJ			PMUM_REG(0x1000)
#define VCXOSD				((0x1) << 19)

#define PMUA_APPS_PWRMODE		PMUA_REG(0x02c0)
#define APPS_PWRMODE			((0x3) << 0)
#define APPS_PWRMODE_D0			((0x0) << 0)
#define APPS_PWRMODE_D0CG		((0x1) << 0)
#define APPS_PWRMODE_D1D2		((0x2) << 0)

#define LCD_RFSH_EN			((0x1) << 4)
#define CLR_D2_STATUS			((0x1) << 30)
#define D2_STATUS			((0x1) << 31)

#define PMUA_WKUP_MASK			PMUA_REG(0x02c8)
#define PMUA_DEBUG_REG			PMUA_REG(0x0088)
#define PMUA_AP_DEBUG2			PMUA_REG(0x0390)

#define MCU_REGTABLE_CONTROL		MCU_REG(0x0c0)
#define MCU_REGTABLE_DATA_0		MCU_REG(0x0c4)
#define MCU_REGTABLE_DATA_1		MCU_REG(0x0c8)

#define ICU_TIMER1_IRQ_CONF		ICU_REG(14 << 2)
#define ICU_GBL_IRQ0_MSK		ICU_REG(0x10c)
#define ICU_GBL_IRQ1_MSK		ICU_REG(0x110)
#define ICU_GBL_IRQ2_MSK		ICU_REG(0x114)
#define ICU_GBL_IRQ3_MSK		ICU_REG(0x190)
#define ICU_GBL_IRQ4_MSK		ICU_REG(0x1d8)
#define ICU_GBL_IRQ5_MSK		ICU_REG(0x1f0)
#define ICU_GBL_IRQ6_MSK		ICU_REG(0x208)
#define ICU_GBL_IRQ7_MSK		ICU_REG(0x220)
#define ICU_GBL_IRQ8_MSK		ICU_REG(0x238)
#define ICU_GBL_IRQ9_MSK		ICU_REG(0x000)

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
#define GIC_DIST_SOFTINT		0xf00

#define WAKEUP_EVT_TIMER1_1		((0x1) << 0)
#define WAKEUP_EVT_TIMER1_2		((0x1) << 1)
#define WAKEUP_EVT_TIMER1_3		((0x1) << 2)
#define WAKEUP_EVT_TIMER2_1		((0x1) << 3)
#define WAKEUP_EVT_TIMER2_2		((0x1) << 4)
#define WAKEUP_EVT_TIMER2_3		((0x1) << 5)
#define WAKEUP_EVT_TIMER3_1		((0x1) << 6)
#define WAKEUP_EVT_TIMER3_2		((0x1) << 7)
#define WAKEUP_EVT_TIMER3_3		((0x1) << 8)

#define LPM_WAKEUP			(WAKEUP_EVT_TIMER3_1 |\
					WAKEUP_EVT_TIMER3_2 |\
					WAKEUP_EVT_TIMER3_3)


#define IRQ_GIC_TIMER1_1		(13)
#define IRQ_GIC_TIMER1_2		(14)
#define IRQ_GIC_TIMER1_3		(15)
#define IRQ_GIC_TIMER2_1		(31)
#define IRQ_GIC_TIMER2_2		(32)
#define IRQ_GIC_TIMER2_3		(33)
#define IRQ_GIC_TIMER3_1		(21)
#define IRQ_GIC_TIMER3_2		(38)
#define IRQ_GIC_TIMER3_3		(58)

#define LPM_DDR_REGTABLE_INDEX		30

#define TIMERS_PHYS_BASE_1		(APB_PHYS_BASE + 0x14000)
#define TIMER3_PHYS_BASE		(APB_PHYS_BASE + 0x88000)
#define CPU_MASK(n)			(1 << n)
#define TMR_CCR				(0x0000)
#define TMR_TN_MM(n, m)			(0x0004 + ((n) << 3) + \
					(((n) + (m)) << 2))
#define TMR_CR(n)			(0x0028 + ((n) << 2))
#define TMR_SR(n)			(0x0034 + ((n) << 2))
#define TMR_IER(n)			(0x0040 + ((n) << 2))
#define TMR_PLVR(n)			(0x004c + ((n) << 2))
#define TMR_PLCR(n)			(0x0058 + ((n) << 2))
#define TMR_WMER			(0x0064)
#define TMR_WMR				(0x0068)
#define TMR_WVR				(0x006c)
#define TMR_WSR				(0x0070)
#define TMR_ICR(n)			(0x0074 + ((n) << 2))
#define TMR_WICR			(0x0080)
#define TMR_CER				(0x0084)
#define TMR_CMR				(0x0088)
#define TMR_ILR(n)			(0x008c + ((n) << 2))
#define TMR_WCR				(0x0098)
#define TMR_WFAR			(0x009c)
#define TMR_WSAR			(0x00A0)
#define TMR_CVWR(n)			(0x00A4 + ((n) << 2))

#define TMR_CCR_CS_0(x)			(((x) & 0x3) << 0)
#define TMR_CCR_CS_1(x)			(((x) & 0x3) << 2)
#define TMR_CCR_CS_2(x)			(((x) & 0x3) << 5)

#define APBC_TIMERS1_CLK_RST		APBC_REG(0x024)




enum pxa1928_lowpower_state {
	POWER_MODE_ACTIVE = 0,
	POWER_MODE_C1_MP0,
	POWER_MODE_C2_MP0,
	POWER_MODE_C1_MP1,
	POWER_MODE_C2_M2_MP2,
	POWER_MODE_C2_M4_MP2,
	POWER_MODE_D0CG,
	POWER_MODE_D0CGLCD,
	POWER_MODE_D1,
	POWER_MODE_D2,
	POWER_MODE_POWERDOWN,
};

#define hard_smp_processor_id()				\
({							\
	u64 __val;					\
	asm("mrs %0, mpidr_el1" : "=r" (__val));	\
	__val &= 0x0F;					\
})

static inline void uartsw_to_active_core(void)
{
#ifdef CONFIG_PXA_AMP_SUPPORT
	int cpu, cur_cpu = hard_smp_processor_id();
	char cmd[20];
	unsigned int core_lpm_status;
	unsigned int core_on_status;

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++) {
		core_lpm_status = __raw_readl(PMUA_COREAP_PWRMODE[cpu])
			& LPM_STATUS;
		core_on_status = __raw_readl(PMUA_COREAP_PWRMODE[cpu])
			& POWER_ON_STATUS;
		if (cur_cpu == cpu)
			continue;

		if (*(u32 *)CONFIG_CORE_BUSY_ADDR & (1 << cpu))
			continue;

		if (core_on_status && !core_lpm_status) {
			printf("console switch from: %d to %d\n",
					cur_cpu, cpu);
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

int invoke_mcpm_fn(u64 function_id, u64 arg0, u64 arg1,
					 u64 arg2)
{
	asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x2")
			__asmeq("%3", "x3")
			"smc	#0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));
	return function_id;
}

static inline void core_pwr_down(void)
{
	uartsw_to_active_core();
	invoke_mcpm_fn(0x85000001, CONFIG_SYS_TEXT_BASE, 1, 0);
	return;
}

static void gic_dist_init(void *base)
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

	/*
	 * Set priority on all interrupts.
	 */
	for (i = 0; i < max_irq; i += 4)
		writel(0xa0a0a0a0, base + GIC_DIST_PRI + i * 4 / 4);

	/*
	 * Disable all interrupts.
	 */
	for (i = 0; i < max_irq; i += 32)
		writel(0xffffffff, base +
				GIC_DIST_ENABLE_CLEAR + i * 4 / 32);

	writel(1, base + GIC_DIST_CTRL);
}

static void gic_cpu_init(void *base)
{
	writel(0xf0, base + GIC_CPU_PRIMASK);
	writel(1, base + GIC_CPU_CTRL);
}

static void gic_init_eden(void)
{
	gic_dist_init((void *)GIC_DIST_PHYS_BASE);
	gic_cpu_init((void *)GIC_CPU_PHYS_BASE);
}

static void pxa1928_lowpower_config(u32 cpu,
			u32 power_state, u32 lowpower_enable)
{
	u32 coreap_pwrmode = 0;
	u32 apps_pwrmode = 0;
	u32 pcr_pj = 0;

	/* keep bit 13 - 16 unchanged
	 * keept bit [31, 29..25] to 1 and others 0 */
	pcr_pj = 0xBE000000;
	pcr_pj |= (__raw_readl(PMUM_PCR_PJ) & 0x1E000);

	coreap_pwrmode = __raw_readl(PMUA_COREAP_PWRMODE[cpu]);
	apps_pwrmode = __raw_readl(PMUA_APPS_PWRMODE);

	if (lowpower_enable) {
		switch (power_state) {
		case POWER_MODE_C1_MP1:
			/* vote for MP1 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP1;
			/* set c1 bit */
			coreap_pwrmode |= CPU_PWRMODE_C1;
			break;
		case POWER_MODE_C2_M2_MP2:
			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* vote for MP2 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP2;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;

		case POWER_MODE_C2_M4_MP2:
			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* vote for MP2 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP2;
			/* vote for l2 pwr down */
			coreap_pwrmode |= L2SR_PWROFF_VT_M4;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;
		case POWER_MODE_C1_MP0:
			/* set c1 bit */
			coreap_pwrmode |= CPU_PWRMODE_C1;
			break;
		case POWER_MODE_C2_MP0:
			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;
		case POWER_MODE_D0CG:
			/* set d0cg mode */
			apps_pwrmode |= APPS_PWRMODE_D0CG;

			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* vote for MP2 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP2;
			/* vote for l2 pwr down */
			coreap_pwrmode |= L2SR_PWROFF_VT_M4;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;
		case POWER_MODE_D0CGLCD:
			/* set d0cg mode */
			apps_pwrmode |= APPS_PWRMODE_D0CG;

			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* vote for MP2 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP2;
			/* vote for l2 pwr down */
			coreap_pwrmode |= L2SR_PWROFF_VT_M4;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;
		case POWER_MODE_D1:
			/* set d1/d2  mode */
			apps_pwrmode |= APPS_PWRMODE_D1D2;

			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* vote for MP2 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP2;
			/* vote for l2 pwr down */
			coreap_pwrmode |= L2SR_PWROFF_VT_M4;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;
		case POWER_MODE_D2:
			/* clear d2 status */
			apps_pwrmode |= CLR_D2_STATUS;
			/* set d1/d2  mode */
			apps_pwrmode |= APPS_PWRMODE_D1D2;

			/* VCXO shutdown allowed */
			pcr_pj |= VCXOSD;

			/* clear c2 status */
			coreap_pwrmode |= CLR_C2_STATUS;
			/* vote for MP2 mode */
			coreap_pwrmode |= CORESS_PWRMODE_VT_MP2;
			/* vote for l2 pwr down */
			coreap_pwrmode |= L2SR_PWROFF_VT_M4;
			/* set c2 bit */
			coreap_pwrmode |= CPU_PWRMODE_C2;
			break;
		default:
			printf("Invalid power state!\n");
			break;
		}
	} else {
		coreap_pwrmode &= ~(CPU_PWRMODE | CORESS_PWRMODE_VT);
		coreap_pwrmode |= (CPU_PWRMODE_WFI | CORESS_PWRMODE_VT_MP0);

		pcr_pj &= ~(VCXOSD);

		apps_pwrmode &= ~(APPS_PWRMODE);
		apps_pwrmode |= (APPS_PWRMODE_D0);
	}

	__raw_writel(coreap_pwrmode, PMUA_COREAP_PWRMODE[cpu]);
	coreap_pwrmode = __raw_readl(PMUA_COREAP_PWRMODE[cpu]);
	__raw_writel(apps_pwrmode, PMUA_APPS_PWRMODE);
	apps_pwrmode = __raw_readl(PMUA_APPS_PWRMODE);
	__raw_writel(pcr_pj, PMUM_PCR_PJ);

	return;
}

static void pxa1928_pmu_global_mask(u32 cpu, u32 mask)
{
	u32 coreap_pwrmode;

	coreap_pwrmode = __raw_readl(PMUA_COREAP_PWRMODE[cpu]);

	if (mask)
		coreap_pwrmode |= (MSK_CPU_IRQ | MSK_CPU_FIQ);
	else
		coreap_pwrmode &= ~(MSK_CPU_IRQ | MSK_CPU_FIQ);

	__raw_writel(coreap_pwrmode, PMUA_COREAP_PWRMODE[cpu]);
}

static void pxa1928_pre_enter_lpm(u32 cpu, u32 power_mode)
{
	pxa1928_lowpower_config(cpu, power_mode, 1);

	/* Mask GIC global interrupt */
	pxa1928_pmu_global_mask(cpu, 1);
	return;
}

static void pxa1928_post_enter_lpm(u32 cpu, u32 power_mode)
{
	/* Unmask GIC interrtup */
	pxa1928_pmu_global_mask(cpu, 0);
	pxa1928_lowpower_config(cpu, power_mode, 0);
}


#define DMCU_HWTPAUSE		0x00010000
#define DMCU_HWTEND		0x00020000
#define DMCU_HWTWRITE		0x80000000

#define MC_CONTROL_0		(0x44)
#define CH0_PHY_CONTROL_9	(0x420)
#define USER_COMMAND_1		(0x024)

#define INSERT_ENTRY_EX(reg, val, entry, pause, last)			\
	do {								\
		u32 tmpvalue;						\
		__raw_writel(val, MCU_REGTABLE_DATA_0);			\
		tmpvalue = reg;						\
		if (pause)						\
			tmpvalue |= DMCU_HWTPAUSE;			\
		if (last)						\
			tmpvalue |= DMCU_HWTEND;			\
		__raw_writel(tmpvalue, MCU_REGTABLE_DATA_1);		\
		tmpvalue = (entry & 0x3ff);				\
		__raw_writel(tmpvalue, MCU_REGTABLE_CONTROL);		\
	} while (0)


void  mck5_regtbl_ddr_dll_calibration(unsigned int
		regtable_num)
{
	int tmp_table_entry_addr;
	u32 tmp_regdata;
	u32 mc_control0;
	u32 mc_clkres_ctrl;
	tmp_table_entry_addr = (regtable_num<<5);
	/* Step 1
	   * make sure that Mck5 is halt for any read\write commands
	   */
	mc_control0 = __raw_readl(MCU_REG(MC_CONTROL_0));
	tmp_regdata = 0x00000002;
	INSERT_ENTRY_EX(MC_CONTROL_0, tmp_regdata, tmp_table_entry_addr,
			0, 0);
	tmp_table_entry_addr = tmp_table_entry_addr + 1;
	/* Step 2
	   * Do the PHY DLL reset, update and sync 2x clock
	   * */
	/* PHY DLL reset */
	tmp_regdata = 0x20000000;
	INSERT_ENTRY_EX(CH0_PHY_CONTROL_9, tmp_regdata,
			tmp_table_entry_addr, 0, 0);
	tmp_table_entry_addr = tmp_table_entry_addr + 1;
	/* PHY DLL Update */
	tmp_regdata = 0x40000000;
	INSERT_ENTRY_EX(CH0_PHY_CONTROL_9, tmp_regdata,
			tmp_table_entry_addr, 0, 0);
	tmp_table_entry_addr = tmp_table_entry_addr + 1;
	/* Sync 2x clock (PAUSE) */
	tmp_regdata = 0x80000000;
	INSERT_ENTRY_EX(CH0_PHY_CONTROL_9, tmp_regdata,
			tmp_table_entry_addr, 1, 0);
	tmp_table_entry_addr = tmp_table_entry_addr + 1;
	/* Step3
	   * Mode update
	   * */
	/*
	   * MR 1
	   */
	tmp_regdata = 0x13020001;
	INSERT_ENTRY_EX(USER_COMMAND_1, tmp_regdata,
			tmp_table_entry_addr, 0, 0);
	tmp_table_entry_addr = tmp_table_entry_addr + 1;
	/*
	   * MR 2
	   */
	tmp_regdata = 0x13020002;
	INSERT_ENTRY_EX(USER_COMMAND_1, tmp_regdata,
			tmp_table_entry_addr, 0, 0);
	tmp_table_entry_addr = tmp_table_entry_addr + 1;
	/* Step 4
	   * resume scheduler (EOP)
	   */
	tmp_regdata = mc_control0;
	tmp_regdata &= ~(1<<1);
	INSERT_ENTRY_EX(MC_CONTROL_0, tmp_regdata, tmp_table_entry_addr,
			0, 1);

	/* set the MC table */
	mc_clkres_ctrl = __raw_readl(PMUA_MC_CLK_RES_CTRL);
	mc_clkres_ctrl &= ~(0xf << 15);
	mc_clkres_ctrl |= (regtable_num << 15);
	__raw_writel(mc_clkres_ctrl, PMUA_MC_CLK_RES_CTRL);
}

static void do_c1_mp0(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_C1_MP0);

	core_wfi();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_C1_MP0);
}

static void do_c2_mp0(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_C2_MP0);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_C2_MP0);
}

static void do_c1_mp1(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_C1_MP1);

	core_wfi();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_C1_MP1);
}

void do_c2_m2_mp2(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_C2_M2_MP2);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_C2_M2_MP2);
}

void do_c2_m4_mp2(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_C2_M4_MP2);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_C2_M4_MP2);
}

static void do_d0cg(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_D0CG);

	/* Set wake up source here */
	__raw_writel(LPM_WAKEUP, PMUA_WKUP_MASK);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_D0CG);
}

static void do_d0cglcd(void)
{
	int cpu = hard_smp_processor_id();

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_D0CGLCD);

	/* Set wake up source here */
	__raw_writel(LPM_WAKEUP, PMUA_WKUP_MASK);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_D0CGLCD);
}

static void do_d1(void)
{
	int cpu = hard_smp_processor_id();

	 mck5_regtbl_ddr_dll_calibration(LPM_DDR_REGTABLE_INDEX);

	pxa1928_pre_enter_lpm(cpu, POWER_MODE_D1);

	/* Set wake up source here */
	__raw_writel(LPM_WAKEUP, PMUA_WKUP_MASK);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_D1);
}

static void do_d2(void)
{
	int cpu = hard_smp_processor_id();

	 mck5_regtbl_ddr_dll_calibration(LPM_DDR_REGTABLE_INDEX);
	pxa1928_pre_enter_lpm(cpu, POWER_MODE_D2);

	/* Set wake up source here */
	__raw_writel(LPM_WAKEUP, PMUA_WKUP_MASK);

	core_pwr_down();

	pxa1928_post_enter_lpm(cpu, POWER_MODE_D2);
}

static void gic_enable_timer_irq(void)
{
	u32 irq;
	irq = 32 + IRQ_GIC_TIMER3_1;
	writel((1 << (irq % 32)), GIC_DIST_PHYS_BASE + GIC_DIST_ENABLE_SET +
		(irq / 32) * 4);
	irq = 32 + IRQ_GIC_TIMER3_2;
	writel((1 << (irq % 32)), GIC_DIST_PHYS_BASE + GIC_DIST_ENABLE_SET +
		(irq / 32) * 4);
	irq = 32 + IRQ_GIC_TIMER3_3;
	writel((1 << (irq % 32)), GIC_DIST_PHYS_BASE + GIC_DIST_ENABLE_SET +
		(irq / 32) * 4);
}



static void timer_config(int cpu)
{
	const void *timer_phy_base = (void *)TIMER3_PHYS_BASE;
	u32 ccr = __raw_readl(timer_phy_base + TMR_CCR);
	u32 cer = __raw_readl(timer_phy_base + TMR_CER);
	u32 cmr = __raw_readl(timer_phy_base + TMR_CMR);
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
	__raw_writel(0x01, timer_phy_base + TMR_ICR(cpu));/* clear status */
	__raw_writel(0x00, timer_phy_base + TMR_IER(cpu));/* disable int */

	/* enable Timer */
	cer |= CPU_MASK(cpu);
	__raw_writel(cer, timer_phy_base + TMR_CER);
}

static void timer_set_match(int cpu, int seconds)
{
	u32 cer;
	int delay;
	const void *timer_phy_base = (void *)TIMER3_PHYS_BASE;
	u32 ccr = __raw_readl(timer_phy_base + TMR_CCR);
	u32 ccr_saved = ccr;

	/*
	 * The calculation formula for the loop cycle is:
	 *
	 * (1) need wait for 2 timer's clock cycle:
	 *        1             2
	 *     ------- x 2 = -------
	 *     fc_freq       fc_freq
	 *
	 * (2) convert to apb clock cycle:
	 *        2          1        apb_freq * 2
	 *     ------- / -------- = ----------------
	 *     fc_freq   apb_freq       fc_freq
	 *
	 * (3) every apb register's accessing will take 8 apb clock cycle,
	 *     also consider add extral one more time for safe way;
	 *     so finally need loop times for the apb register accessing:
	 *
	 *       (apb_freq * 2)
	 *     ------------------ / 8 + 1
	 *          fc_freq
	 */
	delay = ((26 * 1000000 * 2) / 32000 / 8) + 1;


	cpu = cpu % 2;

	/* disable APx_Timer */
	cer = __raw_readl(timer_phy_base + TMR_CER);
	cer &= ~CPU_MASK(cpu);
	__raw_writel(cer, timer_phy_base + TMR_CER);

	if (!cpu) {
		ccr &= ~TMR_CCR_CS_0(3);
		ccr |= TMR_CCR_CS_0(1);		/* Timer 0 -- 32KHz */
	} else {
		ccr &= ~TMR_CCR_CS_1(3);
		ccr |= TMR_CCR_CS_1(1);         /* Timer 1 -- 32KHz */
	}

	__raw_writel(ccr, timer_phy_base + TMR_CCR);

	/* clear pending interrupt status and enable */
	/* All this is for Comparator 0 */
	while (delay--)
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
	void *timer_phy_base = (void *)TIMER3_PHYS_BASE;

	cpu = cpu % 2;

	/* disable and clear pending interrupt status */
	__raw_writel(0x0, timer_phy_base + TMR_IER(cpu));
	__raw_writel(0x1, timer_phy_base + TMR_ICR(cpu));
}

static inline void eden_hcr_init(void)
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
	int cpu = hard_smp_processor_id();
	static int idle_counts;
	idle_counts++;
	pxa1928_lowpower_config(cpu, POWER_MODE_D2, 0);

	if (argc < 2)
		return 0;
	if (cpu == 0) {
		if (idle_counts == 1) {
			gic_init_eden();
			gic_enable_timer_irq();
			timer_config(cpu);
			eden_hcr_init();

			/*
			 * need this udelay otherwise the first time
			 * reading timer0 not correct
			 * Avoid to use udelay to corrupt timer.
			 */
			/* udelay(10); */
			while (i < 3000000)
				i++;
		}

		printf("set timer\n");
		if (argc == 2)
			seconds = 10;
		else
			seconds = simple_strtoul(argv[2], NULL, 10);
		printf("timer interrupt will come after %d seconds\n",
				seconds);
		timer_set_match(cpu, seconds);
	}


	if (strcmp("c1mp0", argv[1]) == 0 ||
			strcmp("C1MP0", argv[1]) == 0)
		do_c1_mp0();

	else if (strcmp("c2mp0", argv[1]) == 0 ||
			strcmp("C2MP0", argv[1]) == 0)
		do_c2_mp0();

	else if (strcmp("c1mp1", argv[1]) == 0 ||
			strcmp("C1MP1", argv[1]) == 0)
		do_c1_mp1();

	else if (strcmp("c2m2mp2", argv[1]) == 0 ||
			strcmp("C2M2MP2", argv[1]) == 0)
		do_c2_m2_mp2();

	else if (strcmp("c2m4mp2", argv[1]) == 0 ||
			strcmp("C2M4MP2", argv[1]) == 0)
		do_c2_m4_mp2();

	else if (strcmp("d0cg", argv[1]) == 0 ||
			strcmp("D0CG", argv[1]) == 0)
		do_d0cg();

	else if (strcmp("d0cglcd", argv[1]) == 0 ||
			strcmp("D0CGLCD", argv[1]) == 0)
		do_d0cglcd();

	else if (strcmp("d1", argv[1]) == 0 ||
			strcmp("D1", argv[1]) == 0)
		do_d1();

	else if (strcmp("d2", argv[1]) == 0 ||
			strcmp("D2", argv[1]) == 0)
		do_d2();

	printf("%u: exit %s\n ", idle_counts, argv[1]);

	if (cpu == 0)
		timer_clear_irq(cpu);

	return 0;
}

U_BOOT_CMD(
	lpm,	3,	1,	do_lpm_test,
	"lpm test",
	""
);


#define endless_loop()                 \
	__asm__ __volatile__ (         \
		"1: b   1b\n\t"        \
			)

int do_core_busy(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int cur_cpu = hard_smp_processor_id();

	*(u32 *)(CONFIG_CORE_BUSY_ADDR) |= 1 << cur_cpu;
	uartsw_to_active_core();
	endless_loop();
	return 0;
}

U_BOOT_CMD(
	core_busy,       2,      1,      do_core_busy,
	"let current cpu do while(1)",
	"[ core_busy]"
);

void peripheral_phy_clk_gate(int common, int defeature1,
		int defeature2, int defeature3, int release)
{
	u32 reg_val;

	if (common) {
		/* 0xD4282820 PMUA_IRE_CLK_GATE_CTRL dynamic gating*/
		reg_val = __raw_readl(PMUA_REG(0x0020));
		reg_val &= ~(0x3 << 4 | 0x3 << 2 | 0x3 << 0);
		reg_val |= (0x2 << 4 | 0x2 << 2 | 0x2 << 0);
		__raw_writel(reg_val, PMUA_REG(0x0020));

		/* 0xD4282848 PMUA_IRE_CLK_RES_CTRL release*/
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0048));
			reg_val |= (0x1 << 0);
			__raw_writel(reg_val, PMUA_REG(0x0048));
		}
		/* 0xD4282848 PMUA_IRE_CLK_RES_CTRL AXI clk disable*/
		reg_val = __raw_readl(PMUA_REG(0x0048));
		reg_val &= ~(0x1 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0048));

		/* 0xD4282850 PMUA_CCIC_CLK_RES_CTRL release */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0050));
			reg_val |= (0x1 << 0 | 0x1 << 1 | 0x1 << 2
					| 0x1 << 9 | 0x1 << 16);
			__raw_writel(reg_val, PMUA_REG(0x0050));
		}
		/* 0xD4282850 PMUA_CCIC_CLK_RES_CTRL clk disable */
		reg_val = __raw_readl(PMUA_REG(0x0050));
		reg_val &= ~(0xf << 17 | 0x1 << 15 | 0x1f << 10
				| 0x3 << 6 | 0x1 << 5 | 0x1 << 4 | 0x1 << 3);
		reg_val |= (0xf << 17 | 0x0 << 15 | 0x1f << 10
				| 0x3 << 6 | 0x0 << 5 | 0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0050));

		/* 0xD4282858 PMUA_SDH2_CLK_RES_CTRL release*/
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0058));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x0058));
		}
		/* 0xD4282858 PMUA_SDH2_CLK_RES_CTRL clk disable*/
		reg_val = __raw_readl(PMUA_REG(0x0058));
		reg_val &= ~(0x1 << 4 | 0x1 << 3);
		reg_val |= (0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0058));

		/* 0xD42828A4 PMUA_VPU_CLK_RES_CTRL release*/
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00a4));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x00a4));
		}

		/* 0xD42828A4 PMUA_VPU_CLK_RES_CTRL clk disable*/
		reg_val = __raw_readl(PMUA_REG(0x00a4));
		reg_val &= ~(0x7 << 24 | 0x3 << 22 | 0x7 << 19 | 0x7 << 16
				| 0x3 << 12 | 0x3 << 6);
		reg_val |= (0x7 << 24 | 0x3 << 22 | 0x7 << 19 | 0x7 << 16
				| 0x3 << 12 | 0x3 << 6);
		__raw_writel(reg_val, PMUA_REG(0x00a4));

		/* 0xD42828CC PMUA_GC_CLK_RES_CTRL release*/
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00cc));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x00cc));
		}

		/* 0xD42828CC PMUA_GC_CLK_RES_CTRL clk disable*/
		reg_val = __raw_readl(PMUA_REG(0x00cc));
		reg_val &= ~(0x7 << 29 | 0x7 << 26 | 0x7 << 23);
		reg_val |= (0x7 << 29 | 0x7 << 26 | 0x7 << 23);
		__raw_writel(reg_val, PMUA_REG(0x00cc));

		/* 0xD42828EC PMUA_SDH4_CLK_RES_CTRL release */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00ec));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x00ec));
		}
		/* 0xD42828EC PMUA_SDH4_CLK_RES_CTRL clk disable */
		reg_val = __raw_readl(PMUA_REG(0x00ec));
		reg_val &= ~(0x1 << 4 | 0x1 << 3 | 0x1 << 1);
		reg_val |= (0x0 << 4 | 0x0 << 3 | 0x0 << 1);
		__raw_writel(reg_val, PMUA_REG(0x00ec));

		/* 0xD42828F4 PMUA_CCIC2_CLK_RES_CTRL release */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00f4));
			reg_val |= (0x1 << 0 | 0x1 << 1 | 0x1 << 2);
			__raw_writel(reg_val, PMUA_REG(0x00f4));
		}

		/* 0xD42828F4 PMUA_CCIC2_CLK_RES_CTRL clk disable */
		reg_val = __raw_readl(PMUA_REG(0x00f4));
		reg_val &= ~(0xf << 16 | 0x1f << 10 | 0x1 << 9 | 0x3 << 6
				| 0x1 << 4 | 0x1 << 3);
		reg_val |= (0xf << 16 | 0x2 << 10 | 0x0 << 9 | 0x3 << 6
				| 0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x00f4));

		/* 0xD42828F8 PMUA_HSIC1_CLK_RES_CTRL release */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00f8));
			reg_val |= (0x1 << 0);
			__raw_writel(reg_val, PMUA_REG(0x00f8));
		}

		/* 0xD42828F8 PMUA_HSIC1_CLK_RES_CTRL clk disable */
		reg_val = __raw_readl(PMUA_REG(0x00f8));
		reg_val &= ~(0x1 << 3);
		reg_val |= (0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x00f8));

		/* 0xD428295C PMUA_SDH5_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x015c));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x015c));
		}
		/* 0xD428295C PMUA_SDH5_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x015c));
		reg_val &= ~(0x1 << 4 | 0x1 << 3);
		reg_val |= (0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x015c));

		/* 0xD4282960 PMUA_SDH6_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0160));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x0160));
		}
		/* 0xD4282960 PMUA_SDH6_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x0160));
		reg_val &= ~(0x1 << 4 | 0x1 << 3);
		reg_val |= (0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0160));

		/* 0xD4282A7C PMUA_GC_CLK_RES_CTRL2 */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x027c));
			reg_val |= (0x1 << 14 | 0x1 << 18);
			__raw_writel(reg_val, PMUA_REG(0x027c));
		}
		/* 0xD4282A7C PMUA_GC_CLK_RES_CTRL2 */
		reg_val = __raw_readl(PMUA_REG(0x027c));
		reg_val &= ~(0x7 << 20 | 0x1 << 19);
		reg_val |= (0x7 << 20 | 0x0 << 19);
		__raw_writel(reg_val, PMUA_REG(0x027c));

		/* 0xD4282860 PMUA_NF_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0060));
			reg_val |= (0x1 << 0 | 0x1 << 1 | 0x1 << 2);
			__raw_writel(reg_val, PMUA_REG(0x0060));
		}
		/* 0xD4282860 PMUA_NF_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x0060));
		reg_val &= ~(0x7 << 6 | 0x1 << 5 | 0x1 << 4 | 0x1 << 3);
		reg_val |= (0x7 << 6 | 0x0 << 5 | 0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0060));

		/* 0xD42828D4 PMUA_SMC_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00d4));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x00d4));
		}
		/* 0xD42828D4 PMUA_SMC_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x00d4));
		reg_val &= ~(0x1f << 16 | 0x1 << 4 | 0x1 << 3);
		reg_val |= (0xf << 16 | 0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x00d4));

		/* 0xD4282834 PMUA_USB_CLK_GATE_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x0034));
		reg_val &= ~(0x3 << 8 | 0x3 << 6 | 0x3 << 2 | 0x3 << 0);
		reg_val |= (0x0 << 8 | 0x0 << 6 | 0x0 << 2 | 0x0 << 0);
		__raw_writel(reg_val, PMUA_REG(0x0034));

		/* 0xD4282b94 PMUA_AP_DEBUG3 */
		if (!cpu_is_pxa1928_a0()) {
			reg_val = __raw_readl(PMUA_REG(0x0394));
			reg_val &= ~(0x1 << 26);
			__raw_writel(reg_val, PMUA_REG(0x0394));
		}
	}

	if (defeature1) {
		/* 0xD4282854 PMUA_SDH1_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0054));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x0054));
		}
		/* 0xD4282854 PMUA_SDH1_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x0054));
		reg_val &= ~(0xf << 10 | 0x3 << 8 | 0x1 << 4 | 0x1 << 3);
		reg_val |= (0xf << 10 | 0x3 << 8 | 0x0 << 4 | 0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0054));
	}

	if (defeature2) {
		/* 0xD4282868 PMUA_WTM_CLK_RES_CTRL release*/
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0068));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x0068));
		}
		/* 0xD4282868 PMUA_WTM_CLK_RES_CTRL clk disable*/
		reg_val = __raw_readl(PMUA_REG(0x0068));
		reg_val &= ~(0x3 << 6 | 0x1 << 4 | 0x1 << 3);
		reg_val |= (0x3 << 6 | 0x1 << 4 | 0x1 << 3);
		__raw_writel(reg_val, PMUA_REG(0x0068));

		/* 0xD4282828 PMUA_CCIC_CLK_GATE_CTRL dynamic gating*/
		reg_val = __raw_readl(PMUA_REG(0x0028));
		reg_val &= ~(0x3 << 10 | 0x3 << 8 | 0x3 << 6 | 0x3 << 4
				| 0x3 << 2 | 0x3 << 0);
		reg_val |= (0x2 << 10 | 0x2 << 8 | 0x2 << 6 | 0x2 << 4
				| 0x2 << 2 | 0x2 << 0);
		__raw_writel(reg_val, PMUA_REG(0x0028));

		/* 0xD4282918 PMUA_CCIC2_CLK_GATE_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x0118));
		reg_val &= ~(0x3 << 10 | 0x3 << 8 | 0x3 << 6 | 0x3 << 4
				| 0x3 << 2 | 0x3 << 0);
		reg_val |= (0x2 << 10 | 0x2 << 8 | 0x2 << 6 | 0x2 << 4
				| 0x2 << 2 | 0x2 << 0);
		__raw_writel(reg_val, PMUA_REG(0x0118));

		/* {0xD4282B40 PMUA_APDBG_CLKRSTCTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x0340));
			reg_val |= (0x1 << 4);
			__raw_writel(reg_val, PMUA_REG(0x00340));
		}
		/* {0xD4282B40 PMUA_APDBG_CLKRSTCTRL */
		reg_val = __raw_readl(PMUA_REG(0x0340));
		reg_val &= ~(0x1f << 8 |  0x7 << 0);
		reg_val |= (0x0 << 8 | 0x0 << 0);
		__raw_writel(reg_val, PMUA_REG(0x00340));

		/* {0xD4050024 PMUM_CGR_SP */
		reg_val = 0xa010;
		__raw_writel(reg_val, PMUM_REG(0x24));

		/* {0xD4051024 PMUM_CGR_PJ */
		reg_val = 0x10000000;
		__raw_writel(reg_val, PMUM_REG(0x1024));

		/* 0xD428285C PMUA_USB_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x005c));
			reg_val |= (0x1 << 0);
			__raw_writel(reg_val, PMUA_REG(0x005c));
		}
		/* 0xD428285C PMUA_USB_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x005c));
		reg_val &= ~(0x1 << 3);
		reg_val |= (0x0 << 3);
		__raw_writel(reg_val, PMUA_REG(0x005c));

		/* 0xD428286C PMUA_BUS_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x006c));
		reg_val &= ~(0x7 << 6);
		reg_val |= (0x4 << 6);
		__raw_writel(reg_val, PMUA_REG(0x006c));

		/* 0xD42828DC PMUA_GLB_CLK_CTRL */
		reg_val = 0x2;
		__raw_writel(reg_val, PMUA_REG(0x00dc));

	}

	if (defeature3) {

		/* 0xD42828E8 PMUA_SDH3_CLK_RES_CTRL */
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x00e8));
			reg_val |= (0x1 << 0 | 0x1 << 1);
			__raw_writel(reg_val, PMUA_REG(0x00e8));
		}

		/* 0xD42828E8 PMUA_SDH3_CLK_RES_CTRL */
		reg_val = __raw_readl(PMUA_REG(0x00e8));
		reg_val &= ~(0x1 << 4 | 0x1 << 3 | 0x1 << 1);
		reg_val |= (0x0 << 4 | 0x0 << 3 | 0x0 << 1);
		__raw_writel(reg_val, PMUA_REG(0x00e8));

		/* 0xD428284C PMUA_DISPLAY1_CLK_RES_CTRL release*/
		if (release) {
			reg_val = __raw_readl(PMUA_REG(0x004c));
			reg_val |= (0x1 << 0 | 0x1 << 2);
			__raw_writel(reg_val, PMUA_REG(0x004c));
		}
		/* 0xD428284C PMUA_DISPLAY1_CLK_RES_CTRL clk disable*/
		reg_val = __raw_readl(PMUA_REG(0x004c));
		reg_val &= ~(0x3 << 22 | 0x1f << 15 | 0x1 << 14 | 0x1 << 13 |
			0x1 << 12 | 0xf << 8) | 0x3 << 6 | 0x1 << 5
			| 0x1 << 4 | 0x1 << 3;
		reg_val |= (0x3 << 22 | 0x2 << 15 | 0x0 << 14 | 0x0 << 13 |
			0x0 << 12 | 0xf << 8) | 0x3 << 6 | 0x0 << 5
			| 0x0 << 4 | 0x0 << 3;
		__raw_writel(reg_val, PMUA_REG(0x004c));
	}
}

int do_clk_gate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong common, defeature1, defeature2, defeature3, release;

	if (argc != 6) {
		printf("usage: clk_gate [common] [defeature1] [defeature2]");
		printf("[defeature3] [release]\n");
		printf("common: disable the clk which should not impact");
		printf("the uboot feature\n");
		printf("fetature1: disable the clk which should impact");
		printf("the SD card\n");
		printf("fetature2: disable the clk which should impact the");
		printf("XDB WTM DFC etc\n");
		printf("fetature3: disable the clk which should impact the");
		printf("emmc,display etc\n");
		printf("release: should release the peripheral before");
		printf("disable its clk\n");
		printf("\n");
		return -1;
	}

	if ((strict_strtoul(argv[1], 0, &common) == 0) &&
		(strict_strtoul(argv[2], 0, &defeature1) == 0) &&
		(strict_strtoul(argv[3], 0, &defeature2) == 0) &&
		(strict_strtoul(argv[4], 0, &defeature3) == 0) &&
		(strict_strtoul(argv[5], 0, &release) == 0)) {
		peripheral_phy_clk_gate(common, defeature1, defeature2,
				defeature3, release);
	}
	return 0;
}

U_BOOT_CMD(
	clk_gate,   6,      0,      do_clk_gate,
	"gate/disable clk for power optimization",
	"[common] [defeature1] [defeature2] [defeature3] [release]"
);

int do_deadidle(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	peripheral_phy_clk_gate(1, 1, 1, 1, 1);
	return 0;
}

U_BOOT_CMD(
	deadidle,   1,      0,      do_deadidle,
	"do clk gate as possible",
	""
);

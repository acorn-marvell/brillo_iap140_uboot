/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1936_CPU_H
#define _PXA1936_CPU_H

#include <asm/io.h>
#include <asm/system.h>
#include <asm/armv7.h>

/* Stepping check */
#define cpu_is_pxa988_z1() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xf0c928)
#define cpu_is_pxa988_z2() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xf1c988)
#define cpu_is_pxa988_z3() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xf2c988)
#define cpu_is_pxa988_a0() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xa0c928)
#define cpu_is_pxa988() \
	(((readl(PXA1936_CIU_BASE) & 0xffff) == 0xc988) || \
	((readl(PXA1936_CIU_BASE) & 0xffff) == 0xc928))

#define cpu_is_pxa986_z1() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xf0c926)
#define cpu_is_pxa986_z2() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xf1c986)
#define cpu_is_pxa986_z3() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xf2c986)
#define cpu_is_pxa986_a0() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xa0c926)
#define cpu_is_pxa986() \
	(((readl(PXA1936_CIU_BASE) & 0xffff) == 0xc986) || \
	((readl(PXA1936_CIU_BASE) & 0xffff) == 0xc926))

#define cpu_is_pxa1088() \
	((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1088)
#define cpu_is_pxa1088_a0() \
	(((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1088) && \
	(readl(BOOT_ROM_VER) == 0x11122012))
#define cpu_is_pxa1088_a1() \
	(((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1088) && \
	(readl(BOOT_ROM_VER) == 0x01282013))

#define cpu_is_pxa1L88()	((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1188)
#define cpu_is_pxa1L88_a0() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xa01188)
#define cpu_is_pxa1L88_a0c() \
	((readl(PXA1936_CIU_BASE) & 0xffffff) == 0xb01188)

#define cpu_is_pxa1U88()	((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1098)
#define cpu_is_pxa1908()	((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1908)
#define cpu_is_pxa1936()	((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1936)
#define cpu_is_pxa1956()	((readl(PXA1936_CIU_BASE) & 0xffff) == 0x1956)

/*
 * Main Power Management (MPMU) Registers
 * Refer Register Datasheet 9.1
 */
struct pxa1936mpmu_registers {
	u8 pad0[0x0024];
	u32 ccgr;	/*0x0024 Seagull/Mohawk Clock Gating Register (cp) */
	u8 pad1[0x0200 - 0x024 - 4];
	u32 wdtpcr;	/*0x0200 WDT (CP Timers) Control Register */
	u8 pad2[0x1020 - 0x200 - 4];
	u32 aprr;	/*0x1020 Seagull/Mohawk Programmable Reset Register */
	u32 acgr;	/*0x1024 Seagull/Mohawk Clock Gating Register (ap) */
};

/*
 * Application Power Management (APMU) Registers
 * Refer Register Datasheet 9.2
 */
struct pxa1936apmu_registers {
	u8 pad0[0x0044];
	u32 dsi;	/*0x0044*/
	u8 pad1[0x004c - 0x044 - 4];
	u32 lcd_dsi;	/*0x004c*/
	u8 pad2[0x0054 - 0x04c - 4];
	u32 sd0;	/*0x0054*/
	u8 pad3[0x005c - 0x054 - 4];
	u32 usb;	/*0x005c*/
	u8 pad4[0x00e0 - 0x05c - 4];
	u32 sd2;	/*0x00e0*/
};

/*
 * APB Clock Reset/Control Registers
 * Refer Register Datasheet 6.14
 */
struct pxa1936apbc_registers {
	u32 uart1;	/*0x000 ap uart*/
	u32 uart2;	/*0x004 ap uart*/
	u32 gpio;	/*0x008*/
	u8 pad0[0x02c - 0x08 - 4];
	u32 twsi0;	/*0x02c*/
	u8 pad1[0x034 - 0x2c - 4];
	u32 timers;	/*0x034*/
	u8 pad2[0x060 - 0x034 - 4];
	u32 twsi1;	/*0x60*/
	u8 pad3[0x070 - 0x060 - 4];
	u32 twsi2;	/*0x70*/
};

/*
 * APB CP Clock Reset/Control Registers
 */
struct pxa1936apbcp_registers {
	u8 pad0[0x1c - 4];
	u32 uart0;	/*0x1c cp uart*/
	u8 pad1[0x28 - 0x1c - 4];
	u32 twsi;	/*0x28*/
};

/*
 * CPU Interface Registers
 * Refer Register Datasheet 4.3
 */
struct pxa1936cpu_registers {
	u32 chip_id;		/* Chip Id Reg */
	u32 pad0;
	u32 cpu_conf;		/* CPU Conf Reg */
	u32 pad1;
	u32 cpu_sram_spd;	/* CPU SRAM Speed Config Register */
	u32 pad2;
	u32 cpu_l2c_spd;	/* CPU L2cache Speed Config Conf */
	u32 mcb_conf0;		/* MCB Conf Reg */
	u32 sys_boot_ctl;	/* Sytem Boot Control */
};

/*
 * Timer registers
 */
struct pxa1936tmr_registers {
	u32 clk_ctrl;		/* Timer clk control reg */
	u32 match[9];		/* Timer match registers */
	u32 count[3];		/* Timer count registers */
	u32 status[3];
	u32 ie[3];
	u32 preload[3];		/* Timer preload value */
	u32 preload_ctrl[3];
	u32 wdt_match_en;	/* 0x64 */
	u32 wdt_match_r;
	u32 wdt_val;
	u32 wdt_sts;
	u32 icr[3];
	u32 wdt_icr;
	u32 cer;		/* Timer count enable reg */
	u32 cmr;
	u32 ilr[3];
	u32 wcr;
	u32 wfar;
	u32 wsar;
	u32 cvwr[3];
};

#define CORE0_ID 0x000
#define CORE1_ID 0x001
#define CORE2_ID 0x002
#define CORE3_ID 0x003
#define CORE4_ID 0x100
#define CORE5_ID 0x101
#define CORE6_ID 0x102
#define CORE7_ID 0x103

static const int CORE_ID[] = {
	CORE0_ID, CORE1_ID,
	CORE2_ID, CORE3_ID,
	CORE4_ID, CORE5_ID,
	CORE6_ID, CORE7_ID,
};

#define PMU_BASE		0xd4282800
#define PMU_CORE_WAKEUP(x)	(PMU_BASE + (x))

#define PMU_CA7_CORE0_WAKEUP	PMU_CORE_WAKEUP(0x12c)
#define PMU_CA7_CORE1_WAKEUP	PMU_CORE_WAKEUP(0x130)
#define PMU_CA7_CORE2_WAKEUP	PMU_CORE_WAKEUP(0x134)
#define PMU_CA7_CORE3_WAKEUP	PMU_CORE_WAKEUP(0x138)
#define PMU_CA7_CORE4_WAKEUP	PMU_CORE_WAKEUP(0x324)
#define PMU_CA7_CORE5_WAKEUP	PMU_CORE_WAKEUP(0x328)
#define PMU_CA7_CORE6_WAKEUP	PMU_CORE_WAKEUP(0x32c)
#define PMU_CA7_CORE7_WAKEUP	PMU_CORE_WAKEUP(0x330)

#define PXA1936_CPU_WAKEUP(n)	(1 << n)

static const long CORE_WAKEUP[] = {
	PMU_CA7_CORE0_WAKEUP, PMU_CA7_CORE1_WAKEUP,
	PMU_CA7_CORE2_WAKEUP, PMU_CA7_CORE3_WAKEUP,
	PMU_CA7_CORE4_WAKEUP, PMU_CA7_CORE5_WAKEUP,
	PMU_CA7_CORE6_WAKEUP, PMU_CA7_CORE7_WAKEUP,
};

#define	PSCI_FN_CPU_SUSPEND	0x84000001
#define	PSCI_FN_CPU_ON		0x84000003
#define	MCPM_PN_CPU_ON		0x85000002

enum pxa1936_lowpower_mothod {
	LPM_PSCI,	/* psci */
	LPM_MCPM,	/* mcpm */
};
extern u32 pxa1936_sdram_base(int chip_sel, u32 base);
extern u32 smp_hw_cpuid(void);

extern void invoke_fn_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
extern void loop_delay(u64 delay);

#ifdef CONFIG_PXA_AMP_SUPPORT
extern void pxa_cpu_reset(u64 cpu, u64 addr);
#endif

#endif/* _PXA1936_CPU_H */

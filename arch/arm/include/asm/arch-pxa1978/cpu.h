/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1978_CPU_H
#define _PXA1978_CPU_H

#include <asm/io.h>
#include <asm/system.h>
#include <asm/armv7.h>

/*
 * Main Power Management (MPMU) Registers
 */
struct pxa1978mpmu_registers {
	u32 pcr0;	/*0x0000*/
	u32 pcr1;	/*0x0004*/
	u32 pcr2;	/*0x0008*/
	u32 pcr3;	/*0x000c*/
	u32 pcr4;	/*0x0010*/
	u32 vcxocr;	/*0x0014*/
	u32 dfccr;	/*0x0018*/
};

/*
 * Application subsystem Power Management (APMU) Registers
 */
struct pxa1978apmu_registers {
	u32 pcr0;	/*0x0000*/
	u32 pcr1;	/*0x0004*/
	u32 pcr2;	/*0x0008*/
	u32 pcr3;	/*0x000c*/
	u32 pcr4;	/*0x0010*/
	u32 psr;	/*0x0014*/
	u32 corereset0; /*0x0018*/
	u32 corereset1; /*0x001c*/
	u32 corereset2; /*0x0020*/
	u32 corereset3; /*0x0024*/
	u32 corereset4; /*0x0028*/
	u32 corestatus; /*0x002c*/
	u32 spidlcfg;	/*0x0030*/
	u32 coreidlcfg0;/*0x0034*/
	u32 coreidlcfg1;/*0x0038*/
	u32 coreidlcfg2;/*0x003c*/
	u32 coreidlcfg3;/*0x0040*/
};

/*
 * ACCU Registers
 */
struct pxa1978accu_registers {
	u8 pad0[0x005c];
	u32 disp1;	/*0x005c*/
	u32 disp2;	/*0x0060*/
	u32 disp_unit;	/*0x0064*/
	u8 pad1[0x00b0 - 0x0068];
	u32 timer1;	/*0x00b0*/
	u32 timer2;	/*0x00b4*/
	u32 timer3;	/*0x00b8*/
	u8 pad2[0x00c4 - 0x00bc];
	u32 kpc;	/*0x00c4*/
	u32 rtc;	/*0x00c8*/
	u8 pad3[0x00d0 - 0x00cc];
	u32 ssp1;	/*0x00d0*/
	u32 ssp2;	/*0x00d4*/
	u8 pad4[0x00dc - 0x00d8];
	u32 i2c1;	/*0x00dc*/
	u32 i2c2;	/*0x00e0*/
	u32 i2c3;	/*0x00e4*/
	u32 i2c4;	/*0x00e8*/
	u32 i2c5;	/*0x00ec*/
	u32 i2c6;	/*0x00f0*/
	u8 pad5[0x0100 - 0x00f4];
	u32 ts1;	/*0x0100*/
	u32 ts2;	/*0x0104*/
	u32 ts3;	/*0x0108*/
	u8 pad6[0x0110 - 0x010c];
	u32 uart1;	/*0x0110*/
	u32 uart2;	/*0x0114*/
	u32 uart3;	/*0x0118*/
	u8 pad7[0x0120 - 0x011c];
	u32 apclk;	/*0x0120*/
	u32 apclk2;	/*0x0124*/
	u32 apclk3;	/*0x0128*/
	u32 apatclk;	/*0x012c*/
	u32 apperclk;	/*0x0130*/
	u8 pad8[0x0154 - 0x0134];
	u32 spare;	/*0x0154*/
	u8 pad9[0x170 - 0x0158];
	u32 n7clk;	/*0x0170*/
	u32 sdh;	/*0x0174*/
	u32 sdh5;	/*0x0178*/
	u8 pad10[0x0190 - 0x017c];
	u32 hb;		/*0x0190*/
};

/*
 * Timer registers
 */
struct pxa1978tmr_registers {
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

extern u32 pxa1978_sdram_base(int chip_sel, u32 base);

extern u32 smp_hw_cpuid(void);
extern void loop_delay(u32 delay);

#ifdef CONFIG_PXA_AMP_SUPPORT
extern void pxa_cpu_reset(int cpu, u32 addr);
#endif

#endif /* _PXA1978_CPU_H */

/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1928_CPU_H
#define _PXA1928_CPU_H

#include <asm/io.h>
#include <asm/system.h>

/*
 * Main Power Management (MPMU) Registers
 */
struct pxa1928mpmu_registers {
	u32 pcr_sp;	/*0x0*/
	u8 pad0[0x38 - 4];
	u32 sccr;	/*0x38*/
	u8 pad1[0x1000 - 0x38 - 4];
	u32 pcr_pj;	/*0x1000*/
	u8 pad2[0x1024 - 0x1000 - 4];
	u32 cgr_pj;	/*0x1024*/
	u32 rsr_pj;	/*0x1028*/
	u8 pad3[0x1140 - 0x1028 - 4];
	u32 dvc_stb1;	/*0x1140*/
	u32 dvc_stb2;	/*0x1144*/
	u32 dvc_stb3;	/*0x1148*/
	u32 dvc_stb4;	/*0x114C*/
	u32 dvc_debug;	/*0x1150*/
};

/*
 * I/O domains power control registers
 */
struct pxa1928aib_registers {
	u8 pad0[0x20];
	u32 nand;	/*0x20*/
};

/*
 * APB Clock Reset/Control Registers
 */
struct pxa1928apbc_registers {
	u8 pad0[0x004];
	u32 twsi1;	/*0x004*/
	u32 twsi2;	/*0x008*/
	u32 twsi3;	/*0x00c*/
	u32 twsi4;	/*0x010*/
	u8 pad1[0x024 - 0x10 - 4];
	u32 timers;	/*0x024*/
	u8 pad2[0x02C - 0x24 - 4];
	u32 uart1;	/*0x02C*/
	u8 pad3[0x034 - 0x2C - 4];
	u32 uart3;	/*0x034*/
	u32 gpio;	/*0x038*/
	u8 pad4[0x044 - 0x38 - 4];
	u32 pwm3;	/*0x044*/
	u8 pad5[0x064 - 0x44 - 4];
	u32 aib;	/*0x064*/
#define FIRST_ACCESS_KEY        0xBABA  /* AIB Code Locked First Access Key */
#define SECOND_ACCESS_KEY       0xEB10  /* AIB Code Locked Second Access Key */
	u32 asfar;	/*0x068*/
	u32 assar;	/*0x06c*/
	u8 pad6[0x074 - 0x6c - 4];
	u32 mpmu;	/*0x74*/
	u8 pad7[0x07c - 0x74 - 4];
	u32 twsi5;	/*0x07c*/
	u32 twsi6;	/*0x080*/
	u8 pad8[0x0B0 - 0x80 - 4];
	u32 mpmu1;	/*0x0B0*/
};

/*
 * Application Subsystem PMU(APMU) Registers
 */
struct pxa1928apmu_registers {
	u32 cc_sp;		/*0x0*/
	u32 cc_pj;		/*0x4*/
	u8 pad0[0x034 - 0x8];
	u32 usb_dyn_gate;	/*0x034*/
	u8 pad1[0x04c - 0x34 - 4];
	union {
		u32 display1;	/*0x04c PXA1928 Zx*/
		u32 pad2;
	};
	u8 pad3[0x054 - 0x4c - 4];
	u32 sd1;		/*0x054*/
	u8 pad4[0x05c - 0x54 - 4];
	u32 usb;		/*0x05c*/
	u8 pad5[0x064 - 0x5c - 4];
	union {
		u32 pad6;
		u32 dma_clk;	/*0x064 PXA1928 A0*/
	};
	u8 pad7[0x088 - 0x64 - 4];
	u32 debug;		/*0x088*/
	u8 pad8[0x0dc - 0x88 - 4];
	u32 gbl_clkctrl;	/*0x0dc*/
	u8 pad9[0x0e8 - 0xdc - 4];
	u32 sd3;		/*0x0e8*/
	u8 pad10[0x110 - 0xe8 - 4];
	union {
	u32 display2;		/*0x110 PXA1928 Zx*/
	u32 pad11;
	};
	u8 pad12[0x134 - 0x110 - 4];
	u32 apb2_clkctrl;	/*0x134*/
	u8 pad13[0x184 - 0x134 - 8];
	union {
		u32 pad14;
		u32 disp_rstctrl;/*0x180*/
	};
	union {
		u32 pad15;
		u32 disp_clkctrl;/*0x184*/
	};
	union {
		u32 pad16;
		u32 disp_clkctrl2;/*0x188*/
	};
	u8 pad17[0x1ac - 0x184 - 8];
	u32 isld_lcd_ctrl;	/*0x1ac*/
	u8 pad18[0x204 - 0x1ac - 4];
	u32 isld_lcd_pwrctrl;	/*0x204*/
	u8 pad19[0x390 - 0x204 - 4];
	u32 debug2;	/*0x390*/
};

/*
 * Timer registers
 */
struct pxa1928timer_registers {
	u32 ccr;	/* Timer clk control reg */
	u32 match[9];	/* Timer match registers */
	u32 cr[3];	/* Timer count registers */
	u32 sr[3];
	u32 ier[3];
	u32 plvr[3];	/* Timer preload value */
	u32 plcr[3];
	u32 wmer;
	u32 wmr;
	u32 wvr;
	u32 wsr;
	u32 icr[3];
	u32 wicr;
	u32 cer;	/* Timer count enable reg */
	u32 cmr;
	u32 ilr[3];
	u32 wcr;
	u32 wfar;
	u32 wsar;
	u32 cvwr[3];
	u32 crsr;
};

struct pxa1928cpu_registers {
	u32 chip_id;            /* Chip Id Reg */
};

int mv_sdh_init(u32 regbase, u32 max_clk, u32 min_clk, u32 quirks);

#define SD_FIFO_PARAM                  0x104
#define  FORCE_CLK_ON		(1 << 12)
#define  OVRRD_CLK_OE		(1 << 11)
#define  CLK_GATE_ON		(1 << 9)
#define  CLK_GATE_CTL		(1 << 8)
#define  WTC_DEF		0x1
#define  WTC(x)			((x & 0x3) << 2)
#define  RTC_DEF		0x1
#define  RTC(x)			(x & 0x3)

#define SD_CLOCK_AND_BURST_SIZE_SETUP  0x10a
#define  SDCLK_SEL		(1 << 8)
#define  WR_ENDIAN		(1 << 7)
#define  RD_ENDIAN		(1 << 6)
#define  DMA_FIFO_128		1
#define  DMA_SIZE(x)		((x & 0x3) << 2)
#define  BURST_64		1
#define  BURST_SIZE(x)		(x & 0x3)

#define RX_CFG_REG		0x114
#define TUNING_DLY_INC(x)	((x & 0x1ff) << 17)
#define SDCLK_DELAY(x)		((x & 0x1ff) << 8)
#define SDCLK_SEL0(x)		((x & 0x3) << 0)
#define SDCLK_SEL1(x)		((x & 0x3) << 2)

#define TX_CFG_REG		0x118
#define TX_MUX_DLL		(1 << 31)
#define TX_INT_CLK_INV		(1 << 30)
#define TX_HOLD_DELAY0(x)	((x & 0x1ff) << 0)
#define TX_HOLD_DELAY1(x)	((x & 0x1ff) << 16)

#define CHIP_ID			0xD4282C00
static unsigned int mmp_chip_id(void)
{
	static unsigned int chip_id;
	if (!chip_id)
		chip_id = readl(CHIP_ID);
	printf("chip_id = %x\n", chip_id);
	return chip_id;
}

static inline int cpu_is_pxa1928_b0(void)
{
	return ((mmp_chip_id() & 0xffffff) == 0xb0c198);
}

static inline int cpu_is_pxa1928_a0(void)
{
	return ((mmp_chip_id() & 0xffffff) == 0xa0c198);
}

extern u32 smp_config(void);
extern u32 smp_hw_cpuid(void);
extern void invoke_fn_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);
extern void loop_delay(u64 delay);

#ifdef CONFIG_PXA_AMP_SUPPORT
extern void pxa_cpu_reset(u64 cpu, u64 addr);
#endif

extern void pxa1928_pll5_set_rate(unsigned long rate);
extern void powerup_display_controller(struct pxa1928apmu_registers *apmu);

enum reset_reason {
	rr_poweron,
	rr_wdt,
	rr_thermal,
	rr_reset
};

extern enum reset_reason get_reset_reason(void);

#endif /* _PXA1928_CPU_H */

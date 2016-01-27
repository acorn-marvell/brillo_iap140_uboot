/*
 *  U-Boot command for frequency change support
 *
 *  Copyright (C) 2013 Marvell International Ltd.
 *  All Rights Reserved
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <errno.h>
#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <asm/arch/cpu.h>
#include <linux/list.h>
#include <i2c.h>
#include <div64.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
DECLARE_GLOBAL_DATA_PTR;

/* #define REG_DRYRUN_DEBUG */
/* #define REAL_SILICON_DEBUG */

#define __raw_readl_dfc __raw_readl
#define __raw_writel_dfc __raw_writel

#ifdef REG_DRYRUN_DEBUG
#define __raw_readl_dfc __raw_readl_dr_dg
#define __raw_writel_dfc __raw_writel_dr_dg
#endif

#ifdef REAL_SILICON_DEBUG
#define __raw_readl_dfc __raw_readl_rs_dg
#define __raw_writel_dfc __raw_writel_rs_dg
#endif

#if (defined REG_DRYRUN_DEBUG) || (defined REAL_SILICON_DEBUG)
#define REG_POLL_NU	100
struct reg_poll {
	u32 *reg_addr;
	u32 reg_val;
	char reg_name[30];
};
/* reg add, reg default/reset value, reg name */
struct reg_poll pxa1928_reg_poll[REG_POLL_NU] = {
	{0xD4282800, 0x00040001, "PMUA_CC_SP"},
	{0xD4282804, 0x00018000, "PMUA_CC_PJ"},
	{0xD428286C, 0x0000F003, "PMUA_BUS_CLK_RES_CTRL"},
	{0xD4282888, 0x0000000A, "PMUA_DEBUG"},
	{0xD4282950, 0x00000000, "PMUA_CC2_PJ"},
	{0xD4282A58, 0x00002811, "PMUA_MC_CLK_RES_CTRL"},
	{0xD4282AE0, 0x08434201, "PMUA_COREAPSS_CLKCTRL"},
	{0xD4282B48, 0x00000000, "PMUA_AP_PLL_CTRL"},
	{0xD4282B50, 0x00002607, "PMUA_DVC_APPCORESS"},
	{0xD4282B54, 0x00010606, "PMUA_DVC_APSS"},
	{0xD4050010, 0x11958000, "PMUM_POSR"},
	{0xD4050054, 0x018000B9, "PMUM_POSR2"},
	{0xD4050034, 0x00195A00, "PMUM_PLL2CR"},
	{0xD4050050, 0x0018E600, "PMUM_PLL3CR"},
	{0xD4050058, 0x04EC8014, "PMUM_PLL3_CTRL1"},
	{0xD405005C, 0x00E08014, "PMUM_PLL1_CTRL"},
	{0xD4050060, 0x0000000D, "PMUM_PLL3_CTRL2"},
	{0xD4050064, 0x00000000, "PMUM_PLL3_CTRL3"},
	{0xD4050068, 0x00A40000, "PMUM_PLL2_CTRL4"},
	{0xD405006C, 0x00A40000, "PMUM_PLL3_CTRL4"},
	{0xD4050414, 0x04FC8014, "PMUM_PLL2_CTRL1"},
	{0xD4050418, 0x0000000D, "PMUM_PLL2_CTRL2"},
	{0xD405041C, 0x00000000, "PMUM_PLL2_CTRL3"},
	{0xD4051100, 0x0018F600, "PMUM_PLL4CR"},
	{0xD4051104, 0x04F08014, "PMUM_PLL4_CTRL1"},
	{0xD4051108, 0x0000000D, "PMUM_PLL4_CTRL2"},
	{0xD405110C, 0x00000000, "PMUM_PLL4_CTRL3"},
	{0xD4051110, 0x00A40000, "PMUM_PLL4_CTRL4"},
	{0xD4051114, 0x0018BA00, "PMUM_PLL5CR"},
	{0xD4051118, 0x04E88014, "PMUM_PLL5_CTRL1"},
	{0xD405111C, 0x0000000D, "PMUM_PLL5_CTRL2"},
	{0xD4051120, 0x00000000, "PMUM_PLL5_CTRL3"},
	{0xD4051124, 0x00A40000, "PMUM_PLL5_CTRL4"},
};
#endif

#ifdef REG_DRYRUN_DEBUG
u32 __raw_readl_dr_dg(u32 *reg_addr)
{
	u32 reg_val;
	char *reg_name = NULL;
	int i, j = 0;
	for (i = 0; i < REG_POLL_NU; i++) {
		if (pxa1928_reg_poll[i].reg_addr == reg_addr) {
			reg_val = pxa1928_reg_poll[i].reg_val;
			reg_name = pxa1928_reg_poll[i].reg_name;
			break;
		}
	}
	if (i == REG_POLL_NU) {
		reg_val = 0x0;
		printf("reg_read : %-22s(add=0x%08x); val=0x%08x*\n", reg_name, reg_addr, reg_val);
	} else {
		printf("reg_read : %-22s(add=0x%08x); val=0x%08x\n", reg_name, reg_addr, reg_val);
	}
	return reg_val;
}

void  __raw_writel_dr_dg(u32 reg_val, u32 *reg_addr)
{
	int i, j = 0;
	char *reg_name = NULL;
	for (i = 0; i < REG_POLL_NU; i++) {
		if (pxa1928_reg_poll[i].reg_addr == reg_addr) {
			pxa1928_reg_poll[i].reg_val = reg_val;
			reg_name = pxa1928_reg_poll[i].reg_name;
			break;
		}
	}
	if (i == REG_POLL_NU) {
		for (j = 0; j < REG_POLL_NU; j++) {
			if (pxa1928_reg_poll[j].reg_addr == 0) {
				pxa1928_reg_poll[j].reg_val = reg_val;
				pxa1928_reg_poll[j].reg_addr = reg_addr;
				break;
			}
		}
		printf("reg_write: %-22s(add=0x%08x); val=0x%08x*\n", reg_name, reg_addr, reg_val);
		return;
	} else {
		printf("reg_write: %-22s(add=0x%08x); val=0x%08x\n", reg_name, reg_addr, reg_val);
		return;
	}
}
#endif

#ifdef REAL_SILICON_DEBUG
void  __raw_writel_rs_dg(u32 reg_val, u32 *reg_addr)
{
	int i;
	char *reg_name = NULL;
	__raw_writel(reg_val, reg_addr);
	for (i = 0; i < REG_POLL_NU; i++) {
		if (pxa1928_reg_poll[i].reg_addr == reg_addr) {
			reg_name = pxa1928_reg_poll[i].reg_name;
			break;
		}
	}
	printf("reg_write: %-22s(add=0x%08x); val=0x%08x\n", reg_name, reg_addr, reg_val);
	return;
}

u32 __raw_readl_rs_dg(u32 *reg_addr)
{
	u32 reg_val;
	char *reg_name = NULL;
	int i = 0;
	reg_val = __raw_readl(reg_addr);

	for (i = 0; i < REG_POLL_NU; i++) {
		if (pxa1928_reg_poll[i].reg_addr == reg_addr) {
			reg_name = pxa1928_reg_poll[i].reg_name;
			break;
		}
	}
	printf("reg_read : %-22s(add=0x%08x); val=0x%08x\n", reg_name, reg_addr, reg_val);
	return reg_val;
}
#endif

/*#define BIND_OP_SUPPORT*/

#define PROFILE_NUM	1
#define PLL_OFFSET_EN   0

#define AXI_PHYS_BASE			(0xd4200000)
#define APB_PHYS_BASE			(0xd4000000)
#define APBC_PHYS_BASE			(APB_PHYS_BASE + 0x015000)
#define APBC_REG(x)			(APBC_PHYS_BASE + (x))

#define PMUA_PHYS_BASE			(AXI_PHYS_BASE + 0x82800)
#define PMUA_REG(x)			(PMUA_PHYS_BASE + (x))

#define PMUM_PHYS_BASE			(APB_PHYS_BASE + 0x50000)
#define PMUM_REG(x)			(PMUM_PHYS_BASE + (x))

#define MCU_PHYS_BASE			(0xd0000000)
#define MCU_REG(x)			(MCU_PHYS_BASE + (x))

#define ICU_PHYS_BASE			(AXI_PHYS_BASE + 0x82000)
#define ICU_REG(x)			(ICU_PHYS_BASE + (x))

#define PGU_PHYS_BASE			(0xd1e00000)
#define GIC_DIST_PHYS_BASE		(PGU_PHYS_BASE + 0x1000)
#define GIC_CPU_PHYS_BASE		(PGU_PHYS_BASE + 0x0100)

#define PXA1928_TIMER_BASE			(APB_PHYS_BASE + 14000)

#define PMUA_DDRDFC_CTRL		(0xd4282600UL)

#define PMUA_CC_SP			PMUA_REG(0x0000)
#define PMUA_CC_PJ			PMUA_REG(0x0004)
#define PMUA_BUS_CLK_RES_CTRL		PMUA_REG(0x006C)
#define PMUA_DEBUG			PMUA_REG(0x0088)
#define PMUA_PLL_SEL_STATUS		PMUA_REG(0x00C4)

#define PMUA_CC2_PJ			PMUA_REG(0x0150)

#define PMUA_ISLD_ISP_CTRL		PMUA_REG(0x01A4)
#define PMUA_ISLD_AU_CTRL		PMUA_REG(0x01A8)
#define PMUA_APPS_MEM_CTRL		PMUA_REG(0x0194)
#define PMUA_MC_MEM_CTRL		PMUA_REG(0x0198)
#define PMUA_ISLD_SP_CTRL		PMUA_REG(0x019c)
#define PMUA_ISLD_USB_CTRL		PMUA_REG(0x01a0)
#define PMUA_ISLD_LCD_CTRL		PMUA_REG(0x01ac)
#define PMUA_ISLD_VPU_CTRL		PMUA_REG(0x01b0)
#define PMUA_ISLD_GC_CTRL		PMUA_REG(0x01b4)
#define PMUA_COREAPSS_CFG		PMUA_REG(0x39C)

#define PMUA_MC_CLK_RES_CTRL		PMUA_REG(0x0258)
#define PLL4_VCODIV_SEL_SE_SHIFT	(12)
#define PMUA_COREAPSS_CLKCTRL		PMUA_REG(0x02E0)
#define PMUA_AP_PLL_CTRL		PMUA_REG(0x0348)
#define PMUA_AP_DEBUG1			PMUA_REG(0x038C)
#define PMUA_AP_DEBUG2			PMUA_REG(0x0390)
#define PLL_GATING_CTRL_SHIFT(id)       (((id) << 1) + 18)

#define PMUM_POSR			PMUM_REG(0x0010)
#define PMUM_POSR2			PMUM_REG(0x0054)
#define POSR_PLL_LOCK(id)		(1 << ((id) + 27))
#define POSR2_PLL_LOCK(id)		(1 << ((id) + 11))

#define PMUA_DVC_APPCORESS		PMUA_REG(0x0350)
#define PMUA_DVC_APSS			PMUA_REG(0x0354)
#define PMUA_DVC_CTRL			PMUA_REG(0x0360)

#define PMUM_PLL2CR			PMUM_REG(0x0034)
#define PMUM_PLL3CR			PMUM_REG(0x0050)
#define PMUM_PLL3_CTRL1			PMUM_REG(0x0058)
#define PMUM_PLL1_CTRL			PMUM_REG(0x005c)
#define PMUM_PLL3_CTRL2			PMUM_REG(0x0060)
#define PMUM_PLL3_CTRL3			PMUM_REG(0x0064)
#define PMUM_PLL2_CTRL4			PMUM_REG(0x0068)
#define PMUM_PLL3_CTRL4			PMUM_REG(0x006c)
#define PMUM_PLL2_CTRL1			PMUM_REG(0x0414)
#define PMUM_PLL2_CTRL2			PMUM_REG(0x0418)
#define PMUM_PLL2_CTRL3			PMUM_REG(0x041c)
#define PMUM_PLL4CR			PMUM_REG(0x1100)
#define PMUM_PLL4_CTRL1			PMUM_REG(0x1104)
#define PMUM_PLL4_CTRL2			PMUM_REG(0x1108)
#define PMUM_PLL4_CTRL3			PMUM_REG(0x110c)
#define PMUM_PLL4_CTRL4			PMUM_REG(0x1110)
#define PMUM_PLL5CR			PMUM_REG(0x1114)
#define PMUM_PLL5_CTRL1			PMUM_REG(0x1118)
#define PMUM_PLL5_CTRL2			PMUM_REG(0x111c)
#define PMUM_PLL5_CTRL3			PMUM_REG(0x1120)
#define PMUM_PLL5_CTRL4			PMUM_REG(0x1124)
#define PMUM_DVC_STB1			PMUM_REG(0x1140)
#define PMUM_DVC_STB2			PMUM_REG(0x1144)
#define PMUM_DVC_STB3			PMUM_REG(0x1148)
#define PMUM_DVC_STB4			PMUM_REG(0x114c)
#define PMUM_DVC_DEBUG			PMUM_REG(0x1150)

#define COREAPSS_CFG_WTCRTC_MSK		(0xFFFFFF00)
#define COREAPSS_CFG_WTCRTC_HMIPS	(0x50505000)
#define COREAPSS_CFG_WTCRTC_EHMIPS	(0x60A0A000)

enum ddr_type {
	LPDDR2_533M = 0,
	LPDDR3_600M,
	LPDDR3_HYN2G,
	LPDDR3_ELP2G,
	LPDDR3_HYN2G_DIS,
};

enum pxa1928_power_pin {
	VCC_MAIN = 0,
	VCC_MAIN_CORE,
	VCC_MAIN_GC,
	VCC_PIN_MAX,
};

enum dvc_id {
	DVC_ID_MP0_VL = 0,
	DVC_ID_MP1_VL = 1,
	DVC_ID_MP2L2ON_VL = 2,
	DVC_ID_MP2L2OFF_VL = 3,
	DVC_ID_D0_VL = 4,
	DVC_ID_D0CG_VL = 5,
	DVC_ID_D1_VL = 6,
	DVC_ID_D2_VL = 7,
};

static int dvc_offset[8] = {0, 8, 12, 16, 0, 8, 16, 20};

enum {
	DVC_VL_0000 = 0,
	DVC_VL_0001 = 1,
	DVC_VL_0010 = 2,
	DVC_VL_0011 = 3,
	DVC_VL_0100 = 4,
	DVC_VL_0101 = 5,
	DVC_VL_0110 = 6,
	DVC_VL_0111 = 7,
	DVC_VL_1000 = 8,
	DVC_VL_1001 = 9,
	DVC_VL_1010 = 10,
	DVC_VL_1011 = 11,
	DVC_VL_1100 = 12,
	DVC_VL_1101 = 13,
	DVC_VL_1110 = 14,
	DVC_VL_1111 = 15,
	DVC_VL_SLEEP = 16,
};

enum pll_id {
	PLL_ID_PLL0 = 0,
	PLL_ID_PLL1 = 1,
	PLL_ID_PLL2 = 2,
	PLL_ID_PLL3 = 3,
	PLL_ID_PLL4 = 4,
	PLL_ID_PLL5 = 5,
};

enum pll_subid {
	PLL_SUBID_1 = 0,
	PLL_SUBID_2 = 1,
	PLL_SUBID_3 = 2,
	PLL_SUBID_4 = 3,
};

struct pll_freq_tb {
	unsigned long refclk;
	unsigned int pll_refcnt;
	unsigned int ssc_en;
	u32 bw_sel:1;
	u32 icp:4;
	u32 kvco:4;
	u32 refdiv:9;
	u32 fbdiv:9;
	u32 freq_offset_en:1;
	unsigned long fvco;
	unsigned long fvco_offseted;
	u32 bypass_en:1;
	u32 clk_src_sel:1;
	u32 post_div_sel:3;
	unsigned long pll_clkout;
	u32 diff_post_div_sel:3;
	unsigned long pll_clkoutp;
	/* multi-function bit */
	u32 mystical_bit:1;
};

/*
 * pll_ctrl1, pll_ctrl2, pll_ctrl3, pll_ctrl4, pll_cr defined for
 * PLL2, PLL3, PLL4, PLL5
 */
union pll_ctrl1 {
	struct {
		u32 vddl:3;          /* 2:0 */
		u32 bw_sel:1;        /* 3 */
		u32 vddm:2;          /* 5:4 */
		u32 reserved0:9;     /* 14:6 */
		u32 ctune:2;         /* 16:15 */
		u32 reserved1:1;     /* 17 */
		u32 kvco:4;          /* 21:18 */
		u32 icp:4;           /* 15:22 */
		u32 post_div_sel:3;  /* 28:26 */
		u32 pll_rst:1;       /* 29 */
		u32 ctrl1_pll_bit:1; /* 30 */
		u32 bypass_en:1;     /* 31 */
	} bit;
	u32 val;
};

union pll_ctrl2 {
	struct {
		u32 src_sel:1;           /* 0 */
		u32 intpi:4;             /* 4:1 */
		u32 pi_en:1;             /* 5 */
		u32 ssc_clk_en:1;        /* 6 */
		u32 clk_det_en:1;        /* 7 */
		u32 freq_offset_en:1;    /* 8 */
		u32 freq_offset:17;      /* 25:9 */
		u32 freq_offset_mode:1;  /* 26 */
		u32 freq_offset_valid:1; /* 27 */
		u32 reserved:4;          /* 31:28 */
	} bit;
	u32 val;
};

union pll_ctrl3 {
	struct {
		u32 reserved0:1;     /* 0 */
		u32 ssc_mode:1;      /* 1 */
		u32 ssc_freq_div:16; /* 17:2 */
		u32 ssc_rnge:11;     /* 28:18 */
		u32 reserved1:3;     /* 31:29 */
	} bit;
	u32 val;
};

union pll_ctrl4 {
	struct {
		u32 reserved0:5;    /* 4:0 */
		u32 diff_div_sel:3; /* 7:5 */
		u32 reserved1:1;    /* 8 */
		u32 diff_en:1;      /* 9 */
		u32 reserved2:6;    /* 15:10 */
		u32 fd:3;           /* 18:16 */
		u32 intpr:3;        /* 21:19 */
		u32 pi_loop_mode:1; /* 22 */
		u32 avdd_sel:1;     /* 23 */
		u32 reserved3:8;    /* 31:24 */
	} bit;
	u32 val;
};

union pll_cr {
	struct {
		u32 reserved0:8;  /* 7:0 */
		u32 sw_en:1;      /* 8 */
		u32 ctrl:1;       /* 9 */
		u32 fbdiv:9;      /* 18:10 */
		u32 refdiv:9;     /* 27:19 */
		u32 reserved1:4;  /* 31:28 */
	} bit;
	u32 val;
};

const u32 *mpmu_pll_cr[6] = {0, 0,
	(u32 *)PMUM_PLL2CR,
	(u32 *)PMUM_PLL3CR,
	(u32 *)PMUM_PLL4CR,
	(u32 *)PMUM_PLL5CR,
};

const u32 *mpmu_pll_ctrl1[6] = {0, 0,
	(u32 *)PMUM_PLL2_CTRL1,
	(u32 *)PMUM_PLL3_CTRL1,
	(u32 *)PMUM_PLL4_CTRL1,
	(u32 *)PMUM_PLL5_CTRL1,
};

const u32 *mpmu_pll_ctrl2[6] = {0, 0,
	(u32 *)PMUM_PLL2_CTRL2,
	(u32 *)PMUM_PLL3_CTRL2,
	(u32 *)PMUM_PLL4_CTRL2,
	(u32 *)PMUM_PLL5_CTRL2,
};

const u32 *mpmu_pll_ctrl3[6] = {0, 0,
	(u32 *)PMUM_PLL2_CTRL3,
	(u32 *)PMUM_PLL3_CTRL3,
	(u32 *)PMUM_PLL4_CTRL3,
	(u32 *)PMUM_PLL5_CTRL3,
};

const u32 *mpmu_pll_ctrl4[6] = {0, 0,
	(u32 *)PMUM_PLL2_CTRL4,
	(u32 *)PMUM_PLL3_CTRL4,
	(u32 *)PMUM_PLL4_CTRL4,
	(u32 *)PMUM_PLL5_CTRL4,
};

struct pll_freq_tb pll2_freq_tb[] = {
	/* FIXME: pls add more freq point if needed */
	{
		.refclk = 26000000,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xf,
		.refdiv = 3,
		.fbdiv = 80,
		.fvco = 2773333333UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		.pll_clkout = 1386666666UL,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 693333333,
		.mystical_bit = 1,
	},
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xc,
		.refdiv = 3,
		.fbdiv = 61,
		.fvco = 2114666666UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		.pll_clkout = 1057333333UL,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 528666666,
		.mystical_bit = 1,
	},
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xa,
		.refdiv = 3,
		.fbdiv = 46,
		.fvco = 1594666666UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		.pll_clkout = 797333333,
		.pll_clkoutp = 531555555,
		/* select diff_post_div as 3 */
		.mystical_bit = 0,
	}
};

struct pll_freq_tb pll3_freq_tb[] = {
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xb,
		.refdiv = 3,
		.fbdiv = 51,
		.fvco = 1768000000UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		/*
		 * for display 1080p panel:
		 *     60*(1080+40+150+2)*(1920+4+4+2)*24/4 = 883785600
		 */
		.pll_clkout = 884000000,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 442000000,
		.mystical_bit = 0,
	},
};

struct pll_freq_tb pll4_freq_tb[] = {
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xc,
		.refdiv = 3,
		.fbdiv = 61,
		.freq_offset_en = 1,
		.fvco = 2114666666UL,
		.fvco_offseted = 2133333333UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		/* after fvco offseted, it should be 1066666666 */
		.pll_clkout = 1057333333UL,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 528666666,
		/* select post div from DDR register */
		.mystical_bit = 1,
	},
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xd,
		.refdiv = 3,
		.fbdiv = 69,
		.freq_offset_en = 1,
		.fvco = 2392000000UL,
		.fvco_offseted = 2400000000UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		/* after fvco offseted, it should be 1200000000 */
		.pll_clkout = 1196000000UL,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 598000000,
		/* select post div from DDR register */
		.mystical_bit = 1,
	},
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xf,
		.refdiv = 3,
		.fbdiv = 77,
		.freq_offset_en = 0,
		.fvco = 2669333333UL,
		.fvco_offseted = 2400000000UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		/* after fvco offseted, it should be 1200000000 */
		.pll_clkout = 1334666666UL,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 667333333,
		/* select post div from DDR register */
		.mystical_bit = 1,
	},
	{
		.refclk = 26000000,
		.pll_refcnt = 0,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xf,
		.refdiv = 3,
		.fbdiv = 92,
		.freq_offset_en = 0,
		.fvco = 3189333333UL,
		.fvco_offseted = 2400000000UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 1,
		/* after fvco offseted, it should be 1200000000 */
		.pll_clkout = 1594666666UL,
		.diff_post_div_sel = 2,
		.pll_clkoutp = 797333333,
		/* select post div from DDR register */
		.mystical_bit = 1,
	},


};


static struct pll_freq_tb pll5_freq_tb[] = {
	{
		.refclk = 26000000,
		.bw_sel = 0,
		.icp = 3,
		.kvco = 0xc,
		.refdiv = 3,
		.fbdiv = 61,
		.fvco = 2114666666UL,
		.bypass_en = 0,
		.clk_src_sel = 1,
		.post_div_sel = 2,
		.pll_clkout = 528666666,
		.diff_post_div_sel = 1,
		.pll_clkoutp = 1057333333UL,
		.mystical_bit = 1,
	},
};

/* DDR TBL */
/* DDR tbl macro */
#define ALLBITS (0xFFFFFFFF)
#define NONBITS (0x00000000)
#ifndef CONFIG_PXA1928ZX
#define FIXADDR(base, offset) ((u32 *)(((u64)base)+offset))
#else
#define FIXADDR(base, offset) ((u32 *)(((u32)base)+offset))
#endif
#define DMCU_HWTCTRL(base) FIXADDR(base, 0xc0)
#define DMCU_HWTDAT0(base) FIXADDR(base, 0xc4)
#define DMCU_HWTDAT1(base) FIXADDR(base, 0xc8)
#define DMCU_HWTPAUSE 0x00010000
#define DMCU_HWTEND 0x00020000
#define DMCU_CTRL_0 0x44
#define DMCU_PHY_CTRL9 0x420
#define DMCU_MAP_CS0 0x200
#define DMCU_MAP_CS1 0x204
#define DMCU_MAP_VALID (1u << 0)
#define DMCU_CMD_CSSEL_CS0 (1u << 24)
#define DMCU_CMD_CSSEL_CS1 (1u << 25)
#define DMCU_USER_COMMAND0 0x20
#define DMCU_USER_COMMAND1 0x24

#define COMMON_DEF(param)						\
	u32 tmpvalue, tab, ent, map;					\
	u32 *base = (u32 *)param;					\
	do {								\
		tab = 0;						\
		ent = 0;						\
		map = 0;						\
		if (__raw_readl_dfc(FIXADDR(base, DMCU_MAP_CS0))	\
					& DMCU_MAP_VALID)		\
			map |= DMCU_CMD_CSSEL_CS0;			\
		if (__raw_readl_dfc(FIXADDR(base, DMCU_MAP_CS1))	\
					& DMCU_MAP_VALID)		\
			map |= DMCU_CMD_CSSEL_CS1;			\
	} while (0)

#define BEGIN_TABLE(tabidx)						\
	do {								\
		tab = tabidx;						\
		ent = 0;						\
	} while (0)

#define UPDATE_REG(val, reg)						\
		__raw_writel_dfc(val, reg);				\
		/*printf(" %08x ==> [%p]\n", val, reg);*/	\

#define INSERT_ENTRY_EX(reg, b2c, b2s, pause, last)			\
	do {								\
		if (ent >= 32) {					\
			printf("INSERT_ENTRY_EX too much entry\n");	\
		}							\
		if (b2c == 0xFFFFFFFF) {				\
			tmpvalue = b2s;					\
		} else {						\
			tmpvalue = __raw_readl_dfc(FIXADDR(base, reg));	\
			tmpvalue = (((tmpvalue) & (~(b2c))) | (b2s));	\
		}							\
		UPDATE_REG(tmpvalue, DMCU_HWTDAT0(base));		\
		tmpvalue = reg;						\
		if (pause)						\
			tmpvalue |= DMCU_HWTPAUSE;			\
		if (last)						\
			tmpvalue |= DMCU_HWTEND;			\
		UPDATE_REG(tmpvalue, DMCU_HWTDAT1(base));		\
		tmpvalue = ((tab << 5) + ent) & 0x3ff;			\
		UPDATE_REG(tmpvalue, DMCU_HWTCTRL(base));		\
		ent++;							\
	} while (0)

#define INSERT_ENTRIES(entries, entcount, pause, last)			\
	do {								\
		u32 li;							\
		for (li = 0; li < entcount; li++) {			\
			if ((li + 1) == entcount) {			\
				INSERT_ENTRY_EX(entries[li].reg,	\
						entries[li].b2c,	\
						entries[li].b2s,	\
						pause, last);		\
			} else {					\
				INSERT_ENTRY_EX(entries[li].reg,	\
						entries[li].b2c,	\
						entries[li].b2s,	\
						0, 0);			\
			}						\
		}							\
	} while (0)

#define INSERT_ENTRY(reg, b2c, b2s) INSERT_ENTRY_EX(reg, b2c, b2s, 0, 0)
#define PAUSE_ENTRY(reg, b2c, b2s) INSERT_ENTRY_EX(reg, b2c, b2s, 1, 0)
#define LAST_ENTRY(reg, b2c, b2s) INSERT_ENTRY_EX(reg, b2c, b2s, 1, 1)

struct ddr_regtbl {
	u32 reg;
	u32 b2c;
	u32 b2s;
};

void pxa1928_ddrhwt_lpddr2_h2l(u32 *dmcu, struct ddr_regtbl *newtiming,
				u32 timing_cnt, u32 table_index)
{
	COMMON_DEF(dmcu);
	/*
	 * 1 PMU asserts 'mc_sleep_req' on type 'mc_sleep_type'; MC4 enters
	 *    self-refresh mode and hold scheduler for DDR access
	 * 2 Frequency change
	 *
	 * Step 1-2 should be triggered by PMU upon DDR change request
	 *
	 * Step 3-6 programmed in the 1st table
	 */
	BEGIN_TABLE(table_index);
	/* Halt MC4 scheduler*/
	INSERT_ENTRY(DMCU_CTRL_0, ALLBITS, 0x00010003);
	/* 3 update timing, we use the boot timing which is for high clock */
	INSERT_ENTRIES(newtiming, timing_cnt, 0, 0);

	/* 4 reset master DLL */
	INSERT_ENTRY(DMCU_PHY_CTRL9, ALLBITS, 0x20000000);
	/* 5 update master DLL */
	INSERT_ENTRY(DMCU_PHY_CTRL9, ALLBITS, 0x40000000);
	/* 6. synchronize 2x clock */
	PAUSE_ENTRY(DMCU_PHY_CTRL9, ALLBITS, 0x80000000);

	/*
	 * 7 wake up SDRAM; when first table done (acked) PMU will de-assert
	 *    mc_sleep_req to wake up SDRAM from self-refresh
	 *
	 * 8 update SDRAM mode register, programmed in 2nd table
	 */
	INSERT_ENTRY(DMCU_CTRL_0, ALLBITS, 0x00010002);
	/* 9 do a long ZQ Cal */
	INSERT_ENTRY(DMCU_USER_COMMAND1, ALLBITS, (map | 0x10020001));
	INSERT_ENTRY(DMCU_USER_COMMAND1, ALLBITS, (map | 0x10020002));
	INSERT_ENTRY(DMCU_USER_COMMAND1, ALLBITS, (map | 0x10020003));
	INSERT_ENTRY(DMCU_USER_COMMAND0, ALLBITS, (map | 0x10001000));
	/* resume scheduler*/
	LAST_ENTRY(DMCU_CTRL_0, ALLBITS, 0);
}

void pxa1928_ddrhwt_lpddr2_l2h(u32 *dmcu, struct ddr_regtbl *newtiming,
				u32 timing_cnt, u32 table_index)
{
	COMMON_DEF(dmcu);
	/*
	 * 1 PMU asserts 'mc_sleep_req' on type 'mc_sleep_type'; MC4 enters
	 *    self-refresh mode and hold scheduler for DDR access
	 * 2 Frequency change
	 *
	 * Step 1-2 should be triggered by PMU upon DDR change request
	 *
	 * Step 3-6 programmed in the 1st table
	 */
	BEGIN_TABLE(table_index);
	/* Halt MC4 scheduler*/
	INSERT_ENTRY(DMCU_CTRL_0, ALLBITS, 0x00010003);
	/* 3 update timing, we use the boot timing which is for high clock */
	INSERT_ENTRIES(newtiming, timing_cnt, 1, 0);

	/* 4 reset master DLL */
	INSERT_ENTRY(DMCU_PHY_CTRL9, ALLBITS, 0x20000000);
	/* 5 update master DLL */
	INSERT_ENTRY(DMCU_PHY_CTRL9, ALLBITS, 0x40000000);
	/* 6. synchronize 2x clock */
	PAUSE_ENTRY(DMCU_PHY_CTRL9, ALLBITS, 0x80000000);

	/*
	 * 7 wake up SDRAM; when first table done (acked) PMU will de-assert
	 *    mc_sleep_req to wake up SDRAM from self-refresh
	 *
	 * 8 update SDRAM mode register, programmed in 2nd table
	 */
	INSERT_ENTRY(DMCU_CTRL_0, ALLBITS, 0x00010002);
	/* 9 do a long ZQ Cal */
	INSERT_ENTRY(DMCU_USER_COMMAND1, ALLBITS, (map | 0x10020001));
	INSERT_ENTRY(DMCU_USER_COMMAND1, ALLBITS, (map | 0x10020002));
	INSERT_ENTRY(DMCU_USER_COMMAND1, ALLBITS, (map | 0x10020003));
	INSERT_ENTRY(DMCU_USER_COMMAND0, ALLBITS, (map | 0x10001000));
	/* resume scheduler*/
	LAST_ENTRY(DMCU_CTRL_0, ALLBITS, 0);
}


#define DMCU_SDRAM_TIMING1	(0x300)
#define DMCU_SDRAM_TIMING2	(0x380)
#define DMCU_SDRAM_TIMING3	(0x384)
#define DMCU_SDRAM_TIMING4	(0x388)
#define DMCU_SDRAM_TIMING5	(0x38c)
#define DMCU_SDRAM_TIMING6	(0x390)
#define DMCU_SDRAM_TIMING7	(0x394)
#define DMCU_SDRAM_TIMING8	(0x398)
#define DMCU_SDRAM_TIMING9	(0x39c)
#define DMCU_SDRAM_TIMING10	(0x3a0)
#define DMCU_SDRAM_TIMING11	(0x3a4)
#define DMCU_SDRAM_TIMING12	(0x3a8)
#define DMCU_SDRAM_TIMING13	(0x3b0)
#define DMCU_SDRAM_TIMING14	(0x404)
#define DMCU_SDRAM_TIMING15	(0x408)
#define DMCU_SDRAM_TIMING16	(0x40c)
#define DMCU_SDRAM_TIMING17	(0x43c)

/* lpddr2 timing */
static struct ddr_regtbl mt42l256m32d2lg18_2x156mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030309B},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x000079E0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x18600010},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0008009C},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000F0039},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00150065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30163016},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030202},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x00000025},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x080A0307},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00403204},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20200020},
};

static struct ddr_regtbl mt42l256m32d2lg18_2x312mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030509B},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000F3C0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x30C00020},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00100138},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x001D0071},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00290065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x502C502C},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11050303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x00000025},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1014060E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00705307},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x40200030},
};

static struct ddr_regtbl mt42l256m32d2lg18_2x528mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030809B},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0001A068},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x53480036},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001B0215},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003000C0},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00460065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x804B804B},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x00000025},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1B220A17},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00C0840C},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60200040},
};

static struct ddr_regtbl mt42l256m32d2lg18_2x208mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030409B},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000A280},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x20800015},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x000B00D0},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0013004B},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x001C0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x401E401E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11040202},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x00000025},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x0B0E0409},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00504205},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x30200020},
};


/* Samsung lpddr3 timing */
static struct ddr_regtbl lpddr3_2x156mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x000079E0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x18600010},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0008009C},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000F0039},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00150065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30163016},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x080A0307},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00404404},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000000},
};

static struct ddr_regtbl lpddr3_2x208mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000A280},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x20800015},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x000B00D0},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0013004B},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x001C0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x401E401E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11040303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x0B0E0409},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00504405},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x30400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000000},
};

static struct ddr_regtbl lpddr3_2x312mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000F3C0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x30C00020},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00100138},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x001D0071},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00290065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x502C502C},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11050303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1014060E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00705407},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x40400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000000},
};

static struct ddr_regtbl lpddr3_2x528mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030C0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00026EA9},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x7C8C0050},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0028031E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0048011F},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00680065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xC070C070},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110C0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x28330F22},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x0110C611},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000050},
};

/*
static struct ddr_regtbl lpddr3_2x598mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003090A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0001D331},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x5D74003C},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001E0256},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003600D8},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x004E0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x90549054},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11090505},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1E260B1A},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00D0950D},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400050},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000050},
};
*/

static struct ddr_regtbl lpddr3_2x667mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030A0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00020918},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x68380043},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0022029B},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003D00F1},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00570065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB05EB05E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x222B0D1D},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x70400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000050},
};

static struct ddr_regtbl lpddr3_2x797mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030C0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00026EA9},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x7C8C0050},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0028031E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0048011F},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00680065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xC070C070},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110C0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x28330F22},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x0110C611},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x00000050},
};

/* Hynix 2GB lpddr3 timing */
static struct ddr_regtbl lpddr3_hynix2g_2x78mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00003CF0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x0C300008},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0004004E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0008001D},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00110065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30123012},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x08050304},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00304403},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x104mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00005140},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x1040000B},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00060068},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000A0026},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00160065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30173017},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x08070305},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00304403},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};


static struct ddr_regtbl lpddr3_hynix2g_2x156mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x000079E0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x18600010},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0008009C},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000F0039},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00210065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30233023},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x080A0307},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00404404},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x208mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000A280},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x20800015},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x000B00D0},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0013004B},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x002C0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x402E402E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11040303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x0B0E0409},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00504405},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x30400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x312mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000F3C0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x30C00020},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00100138},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x001D0071},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00420065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x50455045},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11050303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1014060E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00705407},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x40400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700FF9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300AA0},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CCAA},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x502mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003080A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00018830},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x4E700033},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001A01F6},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x002E00B5},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x006A0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x806F806F},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1A200A16},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00B0840B},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700FF9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300AA0},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CCAA},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x528mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003080A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00019C80},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x52800035},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001B0210},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003000BF},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x006F0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x80758075},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1B220A17},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00C0840C},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700FF9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300AA0},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CCAA},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x667mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030A0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00020918},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x68380043},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0022029B},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003D00F1},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x008D0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB093B093},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x222B0D1D},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x70400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x702mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030B0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00022470},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x6DB00047},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x002402BE},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x004000FD},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00940065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB09BB09B},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x242D0D1E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x745mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030C0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00024608},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x7468004B},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x002602E9},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0044010D},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x009D0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xC0A4C0A4},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110C0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x262F0E20},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x0100C610},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_hynix2g_2x797mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030C0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00026EA9},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x7C8C0050},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0028031E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0048011F},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00A80065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xC0B0C0B0},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110C0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x28330F22},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x0110C611},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

/* Hynix 2GB discrete lpddr3 timing */
static struct ddr_regtbl lpddr3_hynix2g_dis_2x78mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00003CF0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x0C300008},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0004004E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0008001D},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00110065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30123012},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x08050304},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00304403},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x104mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00005140},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x1040000B},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00060068},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000A0026},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00160065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30173017},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x08070305},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00304403},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x156mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x000079E0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x18600010},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0008009C},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000F0039},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00210065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30233023},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x080A0307},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00404404},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x208mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000A280},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x20800015},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x000B00D0},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0013004B},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x002C0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x402E402E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11040303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x0B0E0409},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00504405},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x30400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x312mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000F3C0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x30C00020},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00100138},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x001D0071},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00420065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x50455045},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11050303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1014060E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00705407},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x40400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x502mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003080A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00018830},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x4E700033},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001A01F6},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x002E00B5},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x006A0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x806F806F},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1A200A16},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00B0840B},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x528mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003080A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00019C80},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x52800035},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001B0210},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003000BF},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x006F0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x80758075},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1B220A17},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00C0840C},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
};

/*
static struct ddr_regtbl lpddr3_hynix2g_dis_2x598mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003090A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0001D331},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x5D74003C},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001E0256},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003600D8},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x007E0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x90849084},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11090505},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1E260B1A},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00D0950D},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400050},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
};
*/

/* No 624MHz timing, use 667 instead */
static struct ddr_regtbl lpddr3_hynix2g_dis_2x624mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030A0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00020918},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x68380043},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0022029B},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003D00F1},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x008D0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB093B093},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x222B0D1D},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x70400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
};

static struct ddr_regtbl lpddr3_hynix2g_dis_2x667mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030A0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00020918},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x68380043},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0022029B},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003D00F1},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x008D0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB093B093},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x222B0D1D},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x70400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
};


/* Elpida lpddr3 */
static struct ddr_regtbl lpddr3_elpida2g_2x78mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00003CF0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x0C300008},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0004004E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0008001D},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x000B0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x300B300B},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x08050304},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00304403},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x104mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00005140},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x1040000B},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00060068},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000A0026},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x000E0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x300F300F},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x08070305},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00304403},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x156mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003030A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x000079E0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x18600010},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0008009C},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x000F0039},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00150065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x30163016},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11030303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x080A0307},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00404404},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x20400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x208mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000A280},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x20800015},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x000B00D0},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0013004B},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x001C0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x401E401E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11040303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x0B0E0409},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00504405},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x30400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700779},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300770},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CC77},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x312mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003060A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x0000F3C0},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x30C00020},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x00100138},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x001D0071},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00290065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x502C502C},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11050303},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1014060E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00705407},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x40400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700FF9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300AA0},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CCAA},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x502mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003080A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00018830},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x4E700033},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001A01F6},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x002E00B5},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00420065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x80478047},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1A200A16},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00B0840B},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000001},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700FF9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300AA0},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CCAA},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x528mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x003080A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00019C80},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x52800035},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x001B0210},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003000BF},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00450065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0x804A804A},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x11080404},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x1B220A17},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00C0840C},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x60400040},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0x17700FF9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0x03300AA0},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CCAA},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x667mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030A0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00020918},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x68380043},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0022029B},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x003D00F1},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00570065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB05EB05E},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x222B0D1D},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x70400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x702mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030B0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00022470},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x6DB00047},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x002402BE},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x004000FD},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x005C0065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xB063B063},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110B0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x242D0D1E},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x00F0B60F},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl lpddr3_elpida2g_2x745mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030C0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00024608},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x7468004B},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x002602E9},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0044010D},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00610065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xC069C069},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110C0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x262F0E20},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x0100C610},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};


static struct ddr_regtbl lpddr3_elpida2g_2x797mhz[] = {
	{DMCU_SDRAM_TIMING1, ALLBITS, 0x0030C0A3},
	{DMCU_SDRAM_TIMING2, ALLBITS, 0x00026EA9},
	{DMCU_SDRAM_TIMING3, ALLBITS, 0x7C8C0050},
	{DMCU_SDRAM_TIMING4, ALLBITS, 0x0028031E},
	{DMCU_SDRAM_TIMING5, ALLBITS, 0x0048011F},
	{DMCU_SDRAM_TIMING6, ALLBITS, 0x00680065},
	{DMCU_SDRAM_TIMING7, ALLBITS, 0xC070C070},
	{DMCU_SDRAM_TIMING8, ALLBITS, 0x110C0606},
	{DMCU_SDRAM_TIMING9, ALLBITS, 0x0000002A},
	{DMCU_SDRAM_TIMING10, ALLBITS, 0x28330F22},
	{DMCU_SDRAM_TIMING11, ALLBITS, 0x0110C611},
	{DMCU_SDRAM_TIMING12, ALLBITS, 0x80400060},
	{DMCU_SDRAM_TIMING13, ALLBITS, 0x00000002},
	{DMCU_SDRAM_TIMING14, ALLBITS, 0xD7700AA9},
	{DMCU_SDRAM_TIMING15, ALLBITS, 0xC3300220},
	{DMCU_SDRAM_TIMING16, ALLBITS, 0x4040CF22},
	{DMCU_SDRAM_TIMING17, ALLBITS, 0x00000020},
};

static struct ddr_regtbl *lpddr2_regtbl_array[] = {
	mt42l256m32d2lg18_2x156mhz,
	mt42l256m32d2lg18_2x208mhz,
	mt42l256m32d2lg18_2x312mhz,
	mt42l256m32d2lg18_2x528mhz,
};

static struct ddr_regtbl *lpddr3_samsung1g[] = {
	lpddr3_2x156mhz,
	lpddr3_2x208mhz,
	lpddr3_2x312mhz,
	lpddr3_2x528mhz,
	lpddr3_2x667mhz,
	lpddr3_2x797mhz,
};

static struct ddr_regtbl *lpddr3_hynix2g_regtbl[] = {
	lpddr3_hynix2g_2x78mhz,
	lpddr3_hynix2g_2x104mhz,
	lpddr3_hynix2g_2x156mhz,
	lpddr3_hynix2g_2x208mhz,
	lpddr3_hynix2g_2x312mhz,
	lpddr3_hynix2g_2x502mhz,
	lpddr3_hynix2g_2x528mhz,
	lpddr3_hynix2g_2x667mhz,
	lpddr3_hynix2g_2x702mhz,
	lpddr3_hynix2g_2x745mhz,
	lpddr3_hynix2g_2x797mhz,
};

static struct ddr_regtbl *lpddr3_hynix2g_dis_regtbl[] = {
	lpddr3_hynix2g_dis_2x78mhz,
	lpddr3_hynix2g_dis_2x104mhz,
	lpddr3_hynix2g_dis_2x156mhz,
	lpddr3_hynix2g_dis_2x208mhz,
	lpddr3_hynix2g_dis_2x312mhz,
	lpddr3_hynix2g_dis_2x502mhz,
	lpddr3_hynix2g_dis_2x528mhz,
	lpddr3_hynix2g_dis_2x624mhz,
	lpddr3_hynix2g_dis_2x667mhz,
};


static struct ddr_regtbl *lpddr3_elpida2g_regtbl[] = {
	lpddr3_elpida2g_2x78mhz,
	lpddr3_elpida2g_2x104mhz,
	lpddr3_elpida2g_2x156mhz,
	lpddr3_elpida2g_2x208mhz,
	lpddr3_elpida2g_2x312mhz,
	lpddr3_elpida2g_2x502mhz,
	lpddr3_elpida2g_2x528mhz,
	lpddr3_elpida2g_2x667mhz,
	lpddr3_elpida2g_2x702mhz,
	lpddr3_elpida2g_2x745mhz,
	lpddr3_elpida2g_2x797mhz,
};


void __init_ddr_regtable(int freq_num, int timing_cnt, enum ddr_type ddrtype)
{
	printf("ddrtyp is %d\n", ddrtype);
	int i;

	if (ddrtype == LPDDR2_533M) {
		for (i = 0; i < freq_num; i++) {
			pxa1928_ddrhwt_lpddr2_h2l((u32 *)MCU_PHYS_BASE,
					lpddr2_regtbl_array[i], timing_cnt, i * 2);
			pxa1928_ddrhwt_lpddr2_l2h((u32 *)MCU_PHYS_BASE,
					lpddr2_regtbl_array[i], timing_cnt, i * 2 + 1);
		}
	} else if (ddrtype == LPDDR3_600M) {
		for (i = 0; i < freq_num; i++) {
			pxa1928_ddrhwt_lpddr2_h2l((u32 *)MCU_PHYS_BASE,
					lpddr3_samsung1g[i], timing_cnt, i * 2);
			pxa1928_ddrhwt_lpddr2_l2h((u32 *)MCU_PHYS_BASE,
					lpddr3_samsung1g[i], timing_cnt, i * 2 + 1);
		}
	} else if (ddrtype == LPDDR3_HYN2G) {
		for (i = 0; i < freq_num; i++) {
			pxa1928_ddrhwt_lpddr2_h2l((u32 *)MCU_PHYS_BASE,
					lpddr3_hynix2g_regtbl[i], timing_cnt, i * 2);
			pxa1928_ddrhwt_lpddr2_l2h((u32 *)MCU_PHYS_BASE,
					lpddr3_hynix2g_regtbl[i], timing_cnt, i * 2 + 1);
		}
	} else if (ddrtype == LPDDR3_ELP2G) {
		for (i = 0; i < freq_num; i++) {
			pxa1928_ddrhwt_lpddr2_h2l((u32 *)MCU_PHYS_BASE,
					lpddr3_elpida2g_regtbl[i], timing_cnt, i * 2);
			pxa1928_ddrhwt_lpddr2_l2h((u32 *)MCU_PHYS_BASE,
					lpddr3_elpida2g_regtbl[i], timing_cnt, i * 2 + 1);
		}
	} else if (ddrtype == LPDDR3_HYN2G_DIS) {
		for (i = 0; i < freq_num; i++) {
			pxa1928_ddrhwt_lpddr2_h2l((u32 *)MCU_PHYS_BASE,
					lpddr3_hynix2g_dis_regtbl[i], timing_cnt, i * 2);
			pxa1928_ddrhwt_lpddr2_l2h((u32 *)MCU_PHYS_BASE,
					lpddr3_hynix2g_dis_regtbl[i], timing_cnt, i * 2 + 1);
		}
	} else {
		printf("ddr types is wrong !!!\n");
	}

	return;
}

union mc_res_ctrl {
	struct {
		unsigned int mc_sw_rstn:1;
		unsigned int pmu_ddr_sel:2;
		unsigned int pmu_ddr_div:3;
		unsigned int reserved1:3;
		unsigned int mc_dfc_type:2;
		unsigned int mc_reg_table_en:1;
		unsigned int pll4vcodivselse:3;
#ifndef CONFIG_PXA1928ZX
		unsigned int mc_reg_table:5;
		unsigned int reserved0:3;
		unsigned int rtcwtc_sel:1;
		unsigned int reserved2:4;
#else
		unsigned int mc_reg_table:4;
		unsigned int reserved0:9;
#endif
		unsigned int mc_clk_dfc:1;
		unsigned int mc_clk_sfc:1;
		unsigned int mc_clk_src_sel:1;
		unsigned int mc_4x_mode:1;
	} b;
	unsigned int v;
};

union pmua_bus_clk_res_ctrl {
	struct {
#ifndef CONFIG_PXA1928ZX
		unsigned int mcrst:1;
		unsigned int reserved6:5;
		unsigned int axiclksel:3;
		unsigned int reserved5:7;
		unsigned int axi11skip:1;
		unsigned int reserved4:1;
		unsigned int axi4skip:1;
		unsigned int axi2skip:1;
		unsigned int axi5skip:1;
		unsigned int reserved3:1;
		unsigned int ahbskip:1;
		unsigned int axi11skipratio:1;
		unsigned int reserved2:1;
		unsigned int axi4skipratio:1;
		unsigned int axi2skipratio:1;
		unsigned int axi5skipratio:1;
		unsigned int reserved1:1;
		unsigned int ahbskipratio:1;
		unsigned int reserved0:2;
#else
		unsigned int mcrst:1;
		unsigned int mc2rst:1;
		unsigned int reserved3:4;
		unsigned int axiclksel:3;
		unsigned int reserved2:3;
		unsigned int dmaliteaxires:1;
		unsigned int dmaliteaxien:1;
		unsigned int hsiaxires:1;
		unsigned int hsiaxien:1;
		unsigned int axi1skip:1;
		unsigned int axi3skip:1;
		unsigned int axi4skip:1;
		unsigned int axi2skip:1;
		unsigned int axi5skip:1;
		unsigned int axi6skip:1;
		unsigned int ahbskip:1;
		unsigned int axi1skipratio:1;
		unsigned int axi3skipratio:1;
		unsigned int axi4skipratio:1;
		unsigned int axi2skipratio:1;
		unsigned int axi5skipratio:1;
		unsigned int axi6skipratio:1;
		unsigned int ahbskipratio:1;
		unsigned int reserved0:2;
#endif
	} b;
	unsigned int v;
};

union pmua_cc_pj {
	struct {
#ifndef CONFIG_PXA1928ZX
		unsigned int reserved2:30;
		unsigned int aclk_fc_req:1;
		unsigned int reserved0:1;
#else
		unsigned int reserved3:15;
		unsigned int axi_div:3;
		unsigned int reserved2:9;
		unsigned int pj_ddrfc_vote:1;
		unsigned int reserved1:2;
		unsigned int aclk_fc_req:1;
		unsigned int reserved0:1;
#endif
	} b;
	unsigned int v;
};

union pmua_cc2_pj {
	struct {
#ifndef CONFIG_PXA1928ZX
		unsigned int axi2_div:3;
		unsigned int reserved:29;
#else
		unsigned int axi2_div:3;
		unsigned int axi3_div:3;
		unsigned int axi4_div:3;
		unsigned int reserved:23;
#endif
	} b;
	unsigned int v;
};
union pmua_pll_sel_status {
	struct {
		unsigned int sp_pll_sel:3;
		unsigned int reserved1:6;
		unsigned int soc_axi_pll_sel:3;
		unsigned int reserved0:20;
	} b;
	unsigned int v;
};
enum clk_sel {
	CLK_SRC_PLL1_312	= 0x0,
	CLK_SRC_PLL1_416	= 0x1,
	CLK_SRC_PLL1_624	= 0x2,
	CLK_SRC_PLL2_CLKOUT_1	= 0x3,
	CLK_SRC_PLL2_CLKOUTP_1	= 0x4,
	CLK_SRC_PLL2_CLKOUT_2	= 0x5,
	CLK_SRC_PLL2_CLKOUTP_2	= 0x6,
	CLK_SRC_PLL2_CLKOUT_3	= 0x7,
	CLK_SRC_PLL2_CLKOUTP_3	= 0x8,
	CLK_SRC_PLL4_CLKOUT_1	= 0x9,
	CLK_SRC_PLL4_CLKOUTP_1	= 0xA,
	CLK_SRC_PLL4_CLKOUT_2	= 0xB,
	CLK_SRC_PLL4_CLKOUTP_2	= 0xC,
	CLK_SRC_PLL4_CLKOUT_3	= 0xD,
	CLK_SRC_PLL4_CLKOUTP_3	= 0xE,
	CLK_SRC_PLL4_CLKOUT_4	= 0xF,
	CLK_SRC_PLL4_CLKOUTP_4	= 0x10,
	CLK_SRC_PLL5_CLKOUT_1	= 0x11,
	CLK_SRC_PLL5_CLKOUTP_1	= 0x12,
};
enum {
	LMIPS_RTCWTC = 0x0,
	HMIPS_RTCWTC = 0x1,
};

struct pxa1928_core_opt {
	unsigned int core_clk_src_rate;	/* core clock src rate */
	unsigned int pclk;		/* core p clock */
	unsigned int baclk;		/* bus interface clock */
	unsigned int periphclk;		/* PERIPHCLK */
	enum clk_sel core_clk_sel;	/* core clock src sel*/
	unsigned int core_clk_sel_regval;	/* core clock src sel reg val */
	unsigned int pclk_div_regval;		/* core clk divider reg val*/
	unsigned int baclk_div_regval;		/* bus interface clock divider reg val*/
	unsigned int periphclk_div_regval;	/* PERIPHCLK divider reg val */
	unsigned int rtc_wtc_sel;		/* LMIPS_RTCWTC or HMIPS_RTCWTC */
	unsigned int rtc_wtc_sel_regval;	/* rtc wtc sel reg val */
	struct list_head node;
};

struct pxa1928_ddr_opt {
	unsigned int ddr_clk_src_rate;	/* ddr clock src rate */
	unsigned int dclk;		/* ddr clock */
	unsigned int ddr_tbl_index;	/* ddr FC table index */
	enum clk_sel ddr_clk_sel;	/* ddr src sel */
	unsigned int mc_clk_src_sel_regval;
	unsigned int pmu_ddr_clk_sel_regval;
	unsigned int dclk_div_regval;	/* ddr clk divider reg val */
	unsigned int ddr_freq_level;
	unsigned int ddr_vl;
	unsigned int rtcwtc;
	unsigned int rtcwtc_lvl;
};

struct pxa1928_axi_opt {
#ifndef CONFIG_PXA1928ZX
	unsigned int axi_clk_src_rate;		/* axi clock src rate */
	unsigned int aclk2;			/* axi clock */
	enum clk_sel axi_clk_sel;		/* axi src sel val */
	unsigned int axi_clk_sel_regval;	/* axi src sel reg val */
	unsigned int aclk2_div_regval;		/* axi clk divider */
#else
	unsigned int axi_clk_src_rate;		/* axi clock src rate */
	unsigned int aclk1;			/* axi clock */
	unsigned int aclk2;			/* axi clock */
	unsigned int aclk3;			/* axi clock */
	unsigned int aclk4;			/* axi clock */
	enum clk_sel axi_clk_sel;		/* axi src sel val */
	unsigned int axi_clk_sel_regval;	/* axi src sel reg val */
	unsigned int aclk1_div_regval;		/* axi clk divider */
	unsigned int aclk2_div_regval;		/* axi clk divider */
	unsigned int aclk3_div_regval;		/* axi clk divider */
	unsigned int aclk4_div_regval;		/* axi clk divider */

#endif
};

#ifdef	BIND_OP_SUPPORT
/*
 * used to show the bind relationship of cpu,ddr
 * only used when core,ddr  FC bind together
 */
struct pxa1928_group_opt {
	u32 pclk;
	u32 dclk;
	u32 aclk;
	u32 gclk;
	u32 v_vcc_main;
	u32 v_vcc_main_core;
	u32 v_vcc_main_gc;
}
#define OP(p, d, a, g)			\
	{				\
		.pclk	= p,		\
		.dclk	= d,		\
		.aclk	= a,		\
		.gclk	= g,		\
		.v_vcc_main	= 0,	\
		.v_vcc_main_core = 0,	\
		.v_vcc_main_gc	= 0,	\
	}
#endif

struct platform_opt {
	unsigned int cpuid;
	unsigned int chipid;
	enum ddr_type ddrtype;
	char *cpu_name;
#ifdef	BIND_OP_SUPPORT
	struct pxa1928_group_opt *group_opt;
	unsigned int group_opt_size;
#endif
	struct pxa1928_core_opt *core_opt;
	unsigned int core_opt_size;
	struct pxa1928_ddr_opt *ddr_opt;
	unsigned int ddr_opt_size;
	struct pxa1928_axi_opt *axi_opt;
	unsigned int axi_opt_size;
	/* the default max cpu rate could be supported */
	unsigned int max_pclk_rate;
	unsigned int d0_dvc_level;
	/* the plat rule for filter core ops */
	unsigned int (*is_core_op_invalid_plt)(struct pxa1928_core_opt *cop);
};
/* current platform OP struct */
static struct platform_opt *cur_platform_opt;

static LIST_HEAD(core_op_list);

/* current core OP */
static struct pxa1928_core_opt *cur_core_op;

/* current DDR OP */
static struct pxa1928_ddr_opt *cur_ddr_op;

/* current AXI OP */
static struct pxa1928_axi_opt *cur_axi_op;

static unsigned int clksel_to_clkrate(enum clk_sel src_clk_sel);

static int set_dvc_level(enum dvc_id dvc_id, unsigned int dvc_level);
static int set_volt(enum pxa1928_power_pin power_pin, u32 dvc_level, u32 vol);
static u32 get_volt(enum pxa1928_power_pin power_pin, u32 dvc_level);
#ifdef	BIND_OP_SUPPORT
static int set_group_volt(u32 *vol_vcc_main, u32 *vol_vcc_core, u32 *vol_vcc_gc, u32 dvc_level);
#endif

/*
 * used to record dummy boot up PP, define it here
 * as the default src and div value read from register
 * is NOT the real core and ddr,axi rate.
 * This default boot OP is from PP table.
 */
static struct pxa1928_core_opt pxa1928_core_bootop = {
	.core_clk_src_rate = 624,
	.pclk = 156,
	.baclk = 156,
	.periphclk = 39,
	.core_clk_sel = CLK_SRC_PLL1_624,
	.core_clk_sel_regval = 0x0,
	.pclk_div_regval = 0x4,
	.baclk_div_regval = 0x1,
	.periphclk_div_regval = 0x4,
	.rtc_wtc_sel = LMIPS_RTCWTC,
	.rtc_wtc_sel_regval = 0x0,
};

static struct pxa1928_ddr_opt pxa1928_ddr_bootop = {
	.ddr_clk_src_rate = 624,
	.dclk = 156,
	.ddr_tbl_index = 0,
	.ddr_clk_sel = CLK_SRC_PLL1_624,
	.mc_clk_src_sel_regval = 0x0,
	.pmu_ddr_clk_sel_regval = 0x1,
	.dclk_div_regval = 0x1,
	.ddr_freq_level = 0,
	.ddr_vl = 0,
	.rtcwtc = 0xA9AA5555,
	.rtcwtc_lvl = 0,
};

static struct pxa1928_axi_opt pxa1928_axi_bootop = {
#ifndef CONFIG_PXA1928ZX
	.axi_clk_src_rate = 624,
	.aclk2 = 78,
	.axi_clk_sel = CLK_SRC_PLL1_624,
	.axi_clk_sel_regval = 0x1,
	.aclk2_div_regval = 7,
#else
	.axi_clk_src_rate = 624,
	.aclk1 = 78,
	.aclk2 = 78,
	.aclk3 = 78,
	.aclk4 = 78,
	.axi_clk_sel = CLK_SRC_PLL1_624,
	.axi_clk_sel_regval = 0x1,
	.aclk1_div_regval = 7,
	.aclk2_div_regval = 7,
	.aclk3_div_regval = 7,
	.aclk4_div_regval = 7,
#endif
};

static struct pxa1928_core_opt core_op_array[] = {
	{
		.core_clk_src_rate = 624,
		.pclk = 156,
		.baclk = 156,
		.periphclk = 39,
		.core_clk_sel = CLK_SRC_PLL1_624,
		.core_clk_sel_regval = 0x0,
		.pclk_div_regval = 0x4,
		.baclk_div_regval = 0x1,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = LMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x0,
	},
	{
		.core_clk_src_rate = 624,
		.pclk = 312,
		.baclk = 312,
		.periphclk = 78,
		.core_clk_sel = CLK_SRC_PLL1_624,
		.core_clk_sel_regval = 0x0,
		.pclk_div_regval = 0x2,
		.baclk_div_regval = 0x1,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = LMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x0,
	},
	{
		.core_clk_src_rate = 797,
		.pclk = 398,
		.baclk = 199,
		.periphclk = 49,
		.core_clk_sel = CLK_SRC_PLL2_CLKOUT_3,
#ifndef CONFIG_PXA1928ZX
		.core_clk_sel_regval = 0x2,
#else
		.core_clk_sel_regval = 0x1,
#endif
		.pclk_div_regval = 0x2,
		.baclk_div_regval = 0x2,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = LMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x0,
	},
	{
		.core_clk_src_rate = 531,
		.pclk = 531,
		.baclk = 265,
		.periphclk = 66,
		.core_clk_sel = CLK_SRC_PLL2_CLKOUTP_3,
#ifndef CONFIG_PXA1928ZX
		.core_clk_sel_regval = 0x6,
#else
		.core_clk_sel_regval = 0x2,
#endif
		.pclk_div_regval = 0x1,
		.baclk_div_regval = 0x2,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = LMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x0,
	},
	{
		.core_clk_src_rate = 624,
		.pclk = 624,
		.baclk = 312,
		.periphclk = 78,
		.core_clk_sel = CLK_SRC_PLL1_624,
		.core_clk_sel_regval = 0x0,
		.pclk_div_regval = 0x1,
		.baclk_div_regval = 0x2,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = LMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x0,
	},
	{
		.core_clk_src_rate = 797,
		.pclk = 797,
		.baclk = 398,
		.periphclk = 99,
		.core_clk_sel = CLK_SRC_PLL2_CLKOUT_3,
#ifndef CONFIG_PXA1928ZX
		.core_clk_sel_regval = 0x2,
#else
		.core_clk_sel_regval = 0x1,
#endif
		.pclk_div_regval = 0x1,
		.baclk_div_regval = 0x2,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = HMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x1,
	},
	{
		.core_clk_src_rate = 1057,
		.pclk = 1057,
		.baclk = 528,
		.periphclk = 132,
		.core_clk_sel = CLK_SRC_PLL2_CLKOUT_2,
#ifndef CONFIG_PXA1928ZX
		.core_clk_sel_regval = 0x2,
#else
		.core_clk_sel_regval = 0x1,
#endif
		.pclk_div_regval = 0x1,
		.baclk_div_regval = 0x2,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = HMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x1,
	},
	{
		.core_clk_src_rate = 1386,
		.pclk = 1386,
		.baclk = 693,
		.periphclk = 173,
		.core_clk_sel = CLK_SRC_PLL2_CLKOUT_1,
#ifndef CONFIG_PXA1928ZX
		.core_clk_sel_regval = 0x2,
#else
		.core_clk_sel_regval = 0x1,
#endif
		.pclk_div_regval = 0x1,
		.baclk_div_regval = 0x2,
		.periphclk_div_regval = 0x4,
		.rtc_wtc_sel = HMIPS_RTCWTC,
		.rtc_wtc_sel_regval = 0x1,
	},
};

static struct pxa1928_ddr_opt lpddr2_op_oparray[] = {
	{
		.ddr_clk_src_rate = 624,
		.dclk = 156,
		.ddr_tbl_index = 2,
		.ddr_clk_sel = CLK_SRC_PLL1_624,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x1,
		.dclk_div_regval = 0x1,
		.ddr_freq_level = 0,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,

	},
	{
		.ddr_clk_src_rate = 416,
		.dclk = 208,
		.ddr_tbl_index = 3,
		.ddr_clk_sel = CLK_SRC_PLL1_416,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x0,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 1,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
	{
		.ddr_clk_src_rate = 624,
		.dclk = 312,
		.ddr_tbl_index = 4,
		.ddr_clk_sel = CLK_SRC_PLL1_624,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x1,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 2,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
	{
		.ddr_clk_src_rate = 1057,
		.dclk = 528,
		.ddr_tbl_index = 7,
		.ddr_clk_sel = CLK_SRC_PLL4_CLKOUT_1,
		.mc_clk_src_sel_regval = 0x1,
		.pmu_ddr_clk_sel_regval = 0x0,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 3,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},


};
static struct pxa1928_ddr_opt lpddr3_op_oparray[] = {
	{
		.ddr_clk_src_rate = 624,
		.dclk = 78,
		.ddr_tbl_index = 0,
		.ddr_clk_sel = CLK_SRC_PLL1_624,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x1,
		.dclk_div_regval = 0x3,
		.ddr_freq_level = 0,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,


	},
	{
		.ddr_clk_src_rate = 624,
		.dclk = 104,
		.ddr_tbl_index = 1,
		.ddr_clk_sel = CLK_SRC_PLL1_624,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x1,
		.dclk_div_regval = 0x2,
		.ddr_freq_level = 1,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,


	},
	{
		.ddr_clk_src_rate = 624,
		.dclk = 156,
		.ddr_tbl_index = 2,
		.ddr_clk_sel = CLK_SRC_PLL1_624,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x1,
		.dclk_div_regval = 0x1,
		.ddr_freq_level = 2,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,


	},
	{
		.ddr_clk_src_rate = 416,
		.dclk = 208,
		.ddr_tbl_index = 3,
		.ddr_clk_sel = CLK_SRC_PLL1_416,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x0,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 3,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
	{
		.ddr_clk_src_rate = 624,
		.dclk = 312,
		.ddr_tbl_index = 4,
		.ddr_clk_sel = CLK_SRC_PLL1_624,
		.mc_clk_src_sel_regval = 0x0,
		.pmu_ddr_clk_sel_regval = 0x1,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 4,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
	{
		.ddr_clk_src_rate = 1057,
		.dclk = 528,
		.ddr_tbl_index = 7,
		.ddr_clk_sel = CLK_SRC_PLL4_CLKOUT_1,
		.mc_clk_src_sel_regval = 0x1,
		.pmu_ddr_clk_sel_regval = 0x0,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 5,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
	{
		.ddr_clk_src_rate = 1334,
		.dclk = 667,
		.ddr_tbl_index = 8,
		.ddr_clk_sel = CLK_SRC_PLL4_CLKOUT_3,
		.mc_clk_src_sel_regval = 0x1,
		.pmu_ddr_clk_sel_regval = 0x0,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 6,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
	{
		.ddr_clk_src_rate = 1594,
		.dclk = 797,
		.ddr_tbl_index = 11,
		.ddr_clk_sel = CLK_SRC_PLL4_CLKOUT_4,
		.mc_clk_src_sel_regval = 0x1,
		.pmu_ddr_clk_sel_regval = 0x0,
		.dclk_div_regval = 0x0,
		.ddr_freq_level = 7,
		.ddr_vl = 0,
		.rtcwtc = 0xA9AA5555,
		.rtcwtc_lvl = 0,
	},
};

static struct pxa1928_axi_opt axi_op_array[] = {
#ifndef CONFIG_PXA1928ZX
	{
		.axi_clk_src_rate = 624,
		.aclk2 = 78,
		.axi_clk_sel = CLK_SRC_PLL1_624,
		.axi_clk_sel_regval = 0x1,
		.aclk2_div_regval = 7,
	},
	{
		.aclk2 = 104,
		.axi_clk_sel = CLK_SRC_PLL1_416,
		.axi_clk_sel_regval = 0x3,
		.aclk2_div_regval = 3,
	},
	{
		.axi_clk_src_rate = 624,
		.aclk2 = 156,
		.axi_clk_sel = CLK_SRC_PLL1_624,
		.axi_clk_sel_regval = 0x1,
		.aclk2_div_regval = 3,
	},
	{
		.axi_clk_src_rate = 416,
		.aclk2 = 208,
		.axi_clk_sel = CLK_SRC_PLL1_416,
		.axi_clk_sel_regval = 0x3,
		.aclk2_div_regval = 1,
	},
	{
		.axi_clk_src_rate = 624,
		.aclk2 = 312,
		.axi_clk_sel = CLK_SRC_PLL1_624,
		.axi_clk_sel_regval = 0x1,
		.aclk2_div_regval = 1,
	},


#else
	{
		.axi_clk_src_rate = 624,
		.aclk1 = 78,
		.aclk2 = 78,
		.aclk3 = 78,
		.aclk4 = 78,
		.axi_clk_sel = CLK_SRC_PLL1_624,
		.axi_clk_sel_regval = 0x1,
		.aclk1_div_regval = 7,
		.aclk2_div_regval = 7,
		.aclk3_div_regval = 7,
		.aclk4_div_regval = 7,
	},
	{
		.aclk1 = 104,
		.aclk2 = 104,
		.aclk3 = 104,
		.aclk4 = 104,
		.axi_clk_sel = CLK_SRC_PLL1_416,
		.axi_clk_sel_regval = 0x3,
		.aclk1_div_regval = 3,
		.aclk2_div_regval = 3,
		.aclk3_div_regval = 3,
		.aclk4_div_regval = 3,
	},
	{
		.axi_clk_src_rate = 624,
		.aclk1 = 156,
		.aclk2 = 156,
		.aclk3 = 156,
		.aclk4 = 156,
		.axi_clk_sel = CLK_SRC_PLL1_624,
		.axi_clk_sel_regval = 0x1,
		.aclk1_div_regval = 3,
		.aclk2_div_regval = 3,
		.aclk3_div_regval = 3,
		.aclk4_div_regval = 3,
	},
	{
		.axi_clk_src_rate = 416,
		.aclk1 = 208,
		.aclk2 = 208,
		.aclk3 = 208,
		.aclk4 = 208,
		.axi_clk_sel = CLK_SRC_PLL1_416,
		.axi_clk_sel_regval = 0x3,
		.aclk1_div_regval = 1,
		.aclk2_div_regval = 1,
		.aclk3_div_regval = 1,
		.aclk4_div_regval = 1,
	},
	{
		.axi_clk_src_rate = 624,
		.aclk1 = 312,
		.aclk2 = 312,
		.aclk3 = 312,
		.aclk4 = 312,
		.axi_clk_sel = CLK_SRC_PLL1_624,
		.axi_clk_sel_regval = 0x1,
		.aclk1_div_regval = 1,
		.aclk2_div_regval = 1,
		.aclk3_div_regval = 1,
		.aclk4_div_regval = 1,
	},
#endif
};


#ifdef	BIND_OP_SUPPORT
struct pxa1928_group_opt group_op_array[] = {
	/* pclk dclk aclk vcc_core */
	OP(531, 264, 104, 0),	/* op0 */
	OP(1057, 398, 208, 0),	/* op1 */
};
#endif

static struct platform_opt platform_op_arrays[] = {
	{
		.cpuid = 0x0,
		.chipid = 0x0,
		.ddrtype = LPDDR2_533M,
#ifndef CONFIG_PXA1928ZX
		.cpu_name = "pxa1928",
#else
		.cpu_name = "pxa1928 zx",
#endif
#ifdef	BIND_OP_SUPPORT
		.group_opt = group_op_array,
		.group_opt_size = ARRAY_SIZE(group_op_array),
#endif
		.core_opt = core_op_array,
		.core_opt_size = ARRAY_SIZE(core_op_array),
		.ddr_opt = lpddr2_op_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr2_op_oparray),
		.axi_opt = axi_op_array,
		.axi_opt_size = ARRAY_SIZE(axi_op_array),
	},
	{
		.cpuid = 0x0,
		.chipid = 0x0,
		.ddrtype = LPDDR3_600M,
#ifndef CONFIG_PXA1928ZX
		.cpu_name = "pxa1928",
#else
		.cpu_name = "pxa1928 zx",
#endif
#ifdef	BIND_OP_SUPPORT
		.group_opt = group_op_array,
		.group_opt_size = ARRAY_SIZE(group_op_array),
#endif
		.core_opt = core_op_array,
		.core_opt_size = ARRAY_SIZE(core_op_array),
		.ddr_opt = lpddr3_op_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr3_op_oparray),
		.axi_opt = axi_op_array,
		.axi_opt_size = ARRAY_SIZE(axi_op_array),
	},
	{
		.cpuid = 0x0,
		.chipid = 0x0,
		.ddrtype = LPDDR3_HYN2G,
#ifndef CONFIG_PXA1928ZX
		.cpu_name = "pxa1928",
#else
		.cpu_name = "pxa1928 zx",
#endif
#ifdef	BIND_OP_SUPPORT
		.group_opt = group_op_array,
		.group_opt_size = ARRAY_SIZE(group_op_array),
#endif
		.core_opt = core_op_array,
		.core_opt_size = ARRAY_SIZE(core_op_array),
		.ddr_opt = lpddr3_op_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr3_op_oparray),
		.axi_opt = axi_op_array,
		.axi_opt_size = ARRAY_SIZE(axi_op_array),
	},
	{
		.cpuid = 0x0,
		.chipid = 0x0,
		.ddrtype = LPDDR3_ELP2G,
#ifndef CONFIG_PXA1928ZX
		.cpu_name = "pxa1928",
#else
		.cpu_name = "pxa1928 zx",
#endif
#ifdef	BIND_OP_SUPPORT
		.group_opt = group_op_array,
		.group_opt_size = ARRAY_SIZE(group_op_array),
#endif
		.core_opt = core_op_array,
		.core_opt_size = ARRAY_SIZE(core_op_array),
		.ddr_opt = lpddr3_op_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr3_op_oparray),
		.axi_opt = axi_op_array,
		.axi_opt_size = ARRAY_SIZE(axi_op_array),
	},
	{
		.cpuid = 0x0,
		.chipid = 0x0,
		.ddrtype = LPDDR3_HYN2G_DIS,
#ifndef CONFIG_PXA1928ZX
		.cpu_name = "pxa1928",
#else
		.cpu_name = "pxa1928 zx",
#endif
#ifdef	BIND_OP_SUPPORT
		.group_opt = group_op_array,
		.group_opt_size = ARRAY_SIZE(group_op_array),
#endif
		.core_opt = core_op_array,
		.core_opt_size = ARRAY_SIZE(core_op_array),
		.ddr_opt = lpddr3_op_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr3_op_oparray),
		.axi_opt = axi_op_array,
		.axi_opt_size = ARRAY_SIZE(axi_op_array),
	},


};

#ifdef	BIND_OP_SUPPORT
enum {
	VL0 = 0,
	VL1,
	VL2,
	VL_MAX,
};

enum {
	CORE = 0,
	DDR,
	AXI,
	GC,
	VM_RAIL_MAX,
};
#define MAX_FREQ_CORE	1386
#define MAX_FREQ_DDR	533
#define MAX_FREQ_AXI	256
#define MAX_FREQ_GC	533

static unsigned int freqs_vcc_main_limit[VM_RAIL_MAX][VL_MAX] = {
	{ 531, 1057, 1386 },				/* CORE */
	{ 264, 416, 528 },				/* DDR */
	{ 104, 208, 312 },				/* AXI */
	{ MAX_FREQ_GC, MAX_FREQ_GC, MAX_FREQ_GC },	/* GC2200 */
};
static int volt_vcc_main_svc[PROFILE_NUM][VL_MAX] = {
	{945, 1050, 1200 },	/* profile 0 */
};
static unsigned int freqs_vcc_main_core_limit[VM_RAIL_MAX][VL_MAX] = {
	{ 531, 1057, 1386 },				/* CORE */
	{ MAX_FREQ_DDR, MAX_FREQ_DDR, MAX_FREQ_DDR },	/* DDR */
	{ MAX_FREQ_AXI, MAX_FREQ_AXI, MAX_FREQ_AXI },	/* AXI */
	{ MAX_FREQ_GC, MAX_FREQ_GC, MAX_FREQ_GC },	/* GC2200 */
};
static int volt_vcc_main_core_svc[PROFILE_NUM][VL_MAX] = {
	{945, 1050, 1200 },	/* profile 0 */
};
static unsigned int freqs_vcc_main_gc_limit[VM_RAIL_MAX][VL_MAX] = {
	{ MAX_FREQ_CORE, MAX_FREQ_CORE, MAX_FREQ_CORE },/* CORE */
	{ MAX_FREQ_DDR, MAX_FREQ_DDR, MAX_FREQ_DDR },	/* DDR */
	{ MAX_FREQ_AXI, MAX_FREQ_AXI, MAX_FREQ_AXI },	/* AXI */
	{ 156, 208, MAX_FREQ_GC },			/* GC2200 */
};
static int volt_vcc_main_gc_svc[PROFILE_NUM][VL_MAX] = {
	{945, 1050, 1200 },	/* profile 0 */
};
#endif


static int __clk_pll_vco_enable(int id)
{
	union pll_cr pllx_cr;
	union pll_ctrl1 pllx_ctrl1;
	u32 val, count = 1000;

	/*
	 * 1~5 steps in setrate.
	 * 6. Enable PLL. */
	pllx_cr.val = __raw_readl_dfc(mpmu_pll_cr[id]);
	pllx_cr.bit.sw_en = 1;
	__raw_writel_dfc(pllx_cr.val, mpmu_pll_cr[id]);

	/* 7. Release the PLL out of reset. */
	pllx_ctrl1.val = __raw_readl_dfc(mpmu_pll_ctrl1[id]);
	pllx_ctrl1.bit.pll_rst = 1;
	__raw_writel_dfc(pllx_ctrl1.val, mpmu_pll_ctrl1[id]);

	/* 8. Wait 50us (PLL stable time) */
	udelay(30);
	if (id <= 2) {
#ifdef REG_DRYRUN_DEBUG
		goto out;
#endif
		while ((!(__raw_readl_dfc(PMUM_POSR) & POSR_PLL_LOCK(id)))
				&& count)
			count--;
	} else {
#ifdef REG_DRYRUN_DEBUG
		goto out;
#endif
		while ((!(__raw_readl_dfc(PMUM_POSR2) & POSR2_PLL_LOCK(id)))
				&& count)
			count--;
	}
#ifdef REG_DRYRUN_DEBUG
out:
#endif
	BUG_ON(!count);

	/* 9. Release PLL clock gating */
	val = __raw_readl_dfc(PMUA_AP_PLL_CTRL);
	val &= ~(3 << PLL_GATING_CTRL_SHIFT(id));
	__raw_writel_dfc(val, PMUA_AP_PLL_CTRL);

	debug("%s, pll%d_vco\n", __func__, id);
	debug("PLL status1:%x\n", __raw_readl_dfc(PMUM_POSR));
	debug("PLL status2:%x\n", __raw_readl_dfc(PMUM_POSR2));

	return 0;
}
static void __clk_pll_vco_disable(int id)
{
	union pll_cr pllx_cr;
	union pll_ctrl1 pllx_ctrl1;
	u32 val;

	/* 1. Gate PLL. */
	val = __raw_readl_dfc(PMUA_AP_PLL_CTRL);
	val |= 0x3 << PLL_GATING_CTRL_SHIFT(id);
	__raw_writel_dfc(val, PMUA_AP_PLL_CTRL);

	/* 2. Reset PLL. */
	pllx_ctrl1.val = __raw_readl_dfc(mpmu_pll_ctrl1[id]);
	pllx_ctrl1.bit.pll_rst = 0;
	__raw_writel_dfc(pllx_ctrl1.val, mpmu_pll_ctrl1[id]);

	/* 3. Disable PLL. */
	pllx_cr.val = __raw_readl_dfc(mpmu_pll_cr[id]);
	pllx_cr.bit.sw_en = 0;
	__raw_writel_dfc(pllx_cr.val, mpmu_pll_cr[id]);

	debug("%s, pll%d_vco\n", __func__, id);
	debug("PLL status1:%x\n", __raw_readl_dfc(PMUM_POSR));
	debug("PLL status2:%x\n", __raw_readl_dfc(PMUM_POSR2));
}
struct pll_freq_tb *get_pll_freq_tb(int id, int sub_id)
{
	struct pll_freq_tb *pll_freq = NULL;

	switch (id) {
	case PLL_ID_PLL2:
		switch (sub_id) {
		case PLL_SUBID_1:
			pll_freq = &(pll2_freq_tb[0]);
			break;
		case PLL_SUBID_2:
			pll_freq = &(pll2_freq_tb[1]);
			break;
		case PLL_SUBID_3:
			pll_freq = &(pll2_freq_tb[2]);
			break;
		default:
			break;
		}
		break;
	case PLL_ID_PLL3:
		switch (sub_id) {
		case PLL_SUBID_1:
			pll_freq = &(pll3_freq_tb[0]);
			break;
		default:
			break;
		}
		break;
	case PLL_ID_PLL4:
		switch (sub_id) {
		case PLL_SUBID_1:
			pll_freq = &(pll4_freq_tb[0]);
			break;
		case PLL_SUBID_2:
			pll_freq = &(pll4_freq_tb[1]);
			break;
		case PLL_SUBID_3:
			pll_freq = &(pll4_freq_tb[2]);
			break;
		case PLL_SUBID_4:
			pll_freq = &(pll4_freq_tb[3]);
			break;
		default:
			break;
		}
		break;
	case PLL_ID_PLL5:
		switch (sub_id) {
		case PLL_SUBID_1:
			pll_freq = &(pll5_freq_tb[0]);
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}
	return pll_freq;
}

static u32 __pll_get_intpi(unsigned long fvco)
{
	if (fvco <= 2000000000UL)
		return 0x5;
	else if (fvco <= 2500000000UL)
		return 0x6;
	else if (fvco <= 3000000000UL)
		return 0x8;

	return 0x6;
}

static int __pll_vco_setrate(int id, int subid)
{
	union pll_cr pllx_cr;
	union pll_ctrl1 pllx_ctrl1;
	union pll_ctrl2 pllx_ctrl2;
	union pll_ctrl3 pllx_ctrl3;
	union pll_ctrl4 pllx_ctrl4;
	struct pll_freq_tb *pll_freq;
	pll_freq = get_pll_freq_tb(id, subid);
	u32 pi_loop_mode = 0, desired_modu_freq, desired_ssc_amp,
	    freq_offset_up, freq_offset_value, tmp;
	int offset;

	if (pll_freq->pll_refcnt)
		/*
		 * FIXME: if vco has been enabled, it would be disabled
		 * for updating and re-enable later.
		 */
		__clk_pll_vco_disable(id);

	/*
	 * 1~3 steps in function clk_pll_vco_disable.
	 * 4. Program REFDIV and FBDIV value. */
	pllx_cr.val = __raw_readl_dfc(mpmu_pll_cr[id]);
	pllx_cr.bit.refdiv = pll_freq->refdiv;
	pllx_cr.bit.fbdiv = pll_freq->fbdiv;
	__raw_writel_dfc(pllx_cr.val, mpmu_pll_cr[id]);

	/* 5. Program BYPASS_EN, VCODIV_SEL_SE, ICP, KVCO, BW_SEL,
	 * CLKOUT_SOURCE_SEL, PI_LOOP_MODE, freq_offset, SSC if enabled.
	 */
	pllx_ctrl1.val = __raw_readl_dfc(mpmu_pll_ctrl1[id]);
	pllx_ctrl1.bit.bypass_en = pll_freq->bypass_en;
	pllx_ctrl1.bit.bw_sel = pll_freq->bw_sel;
	pllx_ctrl1.bit.icp = pll_freq->icp;
	pllx_ctrl1.bit.kvco = pll_freq->kvco;
	pllx_ctrl1.bit.post_div_sel = pll_freq->post_div_sel;
	pllx_ctrl1.bit.ctrl1_pll_bit = pll_freq->mystical_bit;
	__raw_writel_dfc(pllx_ctrl1.val, mpmu_pll_ctrl1[id]);
	if (id == 4) {
		tmp = __raw_readl(PMUA_MC_CLK_RES_CTRL);
		tmp &= ~(0x3 << PLL4_VCODIV_SEL_SE_SHIFT);
		tmp |= pll_freq->post_div_sel << PLL4_VCODIV_SEL_SE_SHIFT;
		__raw_writel(tmp, PMUA_MC_CLK_RES_CTRL);
	}

	pllx_ctrl2.val = __raw_readl_dfc(mpmu_pll_ctrl2[id]);
	pllx_ctrl2.bit.src_sel = pll_freq->clk_src_sel;
	__raw_writel_dfc(pllx_ctrl2.val, mpmu_pll_ctrl2[id]);
	pllx_ctrl3.val = __raw_readl_dfc(mpmu_pll_ctrl3[id]);
	pllx_ctrl4.val = __raw_readl_dfc(mpmu_pll_ctrl4[id]);
	pllx_ctrl4.bit.diff_div_sel = pll_freq->diff_post_div_sel;

	if (pll_freq->ssc_en && pll_freq->fvco >= 1500000000UL) {
		/*
		 * enable freq offset
		 *    fvco = 4*refclk*fbdiv/refdiv/(1+offset_percent)
		 */

		/* (1) set ssc_clk_en = 0 */
		pllx_ctrl2.bit.ssc_clk_en = 0;
		__raw_writel_dfc(pllx_ctrl2.val, mpmu_pll_ctrl2[id]);

		/* (2) sampled by CK_DIV64_OUT */
		pllx_ctrl2.bit.freq_offset_mode = 1;

		/* (3) set freq_offset */
		offset = pll_freq->fvco - pll_freq->fvco_offseted;
		freq_offset_up = offset > 0 ? 0 : 0x10000;

		/*
		 * offset_percent = (fvco-fvco_offseted)/fvco_offseted
		 * freq_offset_value =
		 *     2^20*(abs(offset_percent)/(1+offset_percent)
		 *     2^20*(abs(offset)/fvco)
		 * offset_percent should be less than +-5%.
		 */
		if (abs(offset) * 100 / pll_freq->fvco_offseted > 5)
			freq_offset_value = 49932;
		else
			freq_offset_value = 0xffff &
				((2 << 20) * (abs(offset) / pll_freq->fvco));
		freq_offset_value |= freq_offset_up;
		pllx_ctrl2.bit.freq_offset_en = 1;
		pllx_ctrl2.bit.freq_offset = freq_offset_value;
		__raw_writel_dfc(pllx_ctrl2.val, mpmu_pll_ctrl2[id]);

		pi_loop_mode = 1;
	}

	if (pll_freq->ssc_en && pll_freq->fvco >= 1500000000UL) {
		/* select desired_modu_freq 30K as default */
		desired_modu_freq = 30000;
		/* select desired_ssc_amplitude 2.5% */
		desired_ssc_amp = 25;
		/* center mode */
		pllx_ctrl3.bit.ssc_mode = 0;
		/* ssc_freq_div = fvco/(4*4*desired_modu_freq) */
		pllx_ctrl3.bit.ssc_freq_div =
			(pll_freq->fvco >> 4) / desired_modu_freq;
		/* ssc_rnge = desired_ssc_amp/(ssc_freq_div*2^(-28)) */
		pllx_ctrl3.bit.ssc_rnge = desired_ssc_amp * ((1 << 28) /
			(1000 * pllx_ctrl3.bit.ssc_freq_div));
		__raw_writel_dfc(pllx_ctrl3.val, mpmu_pll_ctrl3[id]);

		pllx_ctrl2.bit.ssc_clk_en = 1;
		pllx_ctrl2.bit.pi_en = 1;
		pllx_ctrl2.bit.intpi = __pll_get_intpi(pll_freq->fvco);
		__raw_writel_dfc(pllx_ctrl2.val, mpmu_pll_ctrl2[id]);
		/* FIXME: wait >1us (SSC reset time) */
		udelay(5);
		pi_loop_mode = 1;
	}

	pllx_ctrl4.bit.pi_loop_mode = pi_loop_mode;
	__raw_writel_dfc(pllx_ctrl4.val, mpmu_pll_ctrl4[id]);
	if (pi_loop_mode)
		/* wait 5us (PI stable time) */
		udelay(10);

	debug("pll%d_cr:%x\n", id, pllx_cr.val);
	debug("pll%d_ctrl1:%x\n", id, pllx_ctrl1.val);
	debug("pll%d_ctrl2:%x\n", id, pllx_ctrl2.val);
	debug("pll%d_ctrl3:%x\n", id, pllx_ctrl3.val);
	debug("pll%d_ctrl4:%x\n", id, pllx_ctrl4.val);

	return 0;
}


void turn_off_pll(int id, int sub_id)
{
	struct pll_freq_tb *pll_freq;
	pll_freq = get_pll_freq_tb(id, sub_id);

	pll_freq->pll_refcnt--;
	if (pll_freq->pll_refcnt < 0)
		printf("unmatched pll_refcnt\n");
	if (pll_freq->pll_refcnt == 0) {
		__clk_pll_vco_disable(id);
		printf("Disable pll as it is not used!\n");
	}
}

/* must be called after __init_platform_opt */
void turn_on_pll(int id, int sub_id)
{
	struct pll_freq_tb *pll_freq;
	pll_freq = get_pll_freq_tb(id, sub_id);
	pll_freq->pll_refcnt++;
	if (pll_freq->pll_refcnt == 1) {
		__pll_vco_setrate(id, sub_id);
		__clk_pll_vco_enable(id);
	}
}

#if defined(CONFIG_PXA1928ZX) || defined(BIND_OP_SUPPORT)
/* reserve API to  read cpu profile from FUSE or API provided by trusted s/w */
static int get_profile(void)
{
	return 0;
}
#endif

static void  __init_platform_opt(enum ddr_type ddrtype)
{
	unsigned int i, chipid = 0;
	struct platform_opt *proc;
	unsigned int value;
	chipid = 0x0;
	/*
	 * ddr type is passed from OBM, FC code use this info
	 * to identify DDR OPs.
	 */
	for (i = 0; i < ARRAY_SIZE(platform_op_arrays); i++) {
		proc = platform_op_arrays + i;
		if ((proc->chipid == chipid) &&
			(proc->ddrtype == ddrtype))
			break;
	}
	BUG_ON(i == ARRAY_SIZE(platform_op_arrays));
	cur_platform_opt = proc;

	cur_platform_opt->max_pclk_rate = 1386;

	value = __raw_readl_dfc(PMUA_DVC_APSS);
	value &= (0x7 << 0);
	cur_platform_opt->d0_dvc_level = value;

}
/* Common condition here if you want to filter the core ops */
static unsigned int __is_cpu_op_invalid_com(struct pxa1928_core_opt *cop)
{
	unsigned int max_pclk_rate =
		cur_platform_opt->max_pclk_rate;

	/*
	 * If pclk > default support max core frequency, invalid it
	 */
	if (max_pclk_rate && (cop->pclk > max_pclk_rate))
		return 1;

	return 0;
};

/* plat extra condition here if you want to filter the core ops */
static unsigned int __is_cpu_op_invalid_plt(struct pxa1928_core_opt *cop)
{
	if (cur_platform_opt->is_core_op_invalid_plt)
		return cur_platform_opt->is_core_op_invalid_plt(cop);

	/* no rule no filter */
	return 0;
}

static void __init_core_opt(void)
{
	struct pxa1928_core_opt *core_opt, *cop;
	unsigned int core_opt_size = 0, i;

	core_opt = cur_platform_opt->core_opt;
	core_opt_size = cur_platform_opt->core_opt_size;

	for (i = 0; i < core_opt_size; i++) {
		cop = &core_opt[i];
		if (!cop->core_clk_src_rate)
			cop->core_clk_src_rate =
				clksel_to_clkrate(cop->core_clk_sel);
		BUG_ON(cop->core_clk_src_rate < 0);
		/* check the invalid condition of this op */
		if (__is_cpu_op_invalid_com(cop))
			continue;
		if (__is_cpu_op_invalid_plt(cop))
			continue;
		/* add it into core op list */
		list_add_tail(&cop->node, &core_op_list);
	}
}

static void __init_ddr_opt(void)
{
	struct pxa1928_ddr_opt *ddr_opt, *cop;
	unsigned int ddr_opt_size = 0, i;
	u32 reg;

	ddr_opt = cur_platform_opt->ddr_opt;
	ddr_opt_size = cur_platform_opt->ddr_opt_size;

	debug("dclk(src:sel,div) aclk(src:sel,div)\n");
	for (i = 0; i < ddr_opt_size; i++) {
		cop = &ddr_opt[i];
		BUG_ON((cop->ddr_clk_src_rate < 0));
	}

	reg = __raw_readl_dfc(PMUA_AP_DEBUG2);
	reg |= (0x1 << 20);
	__raw_writel_dfc(reg, PMUA_AP_DEBUG2);
	/* dummy read */
	reg = __raw_readl_dfc(PMUA_AP_DEBUG2);

}

static void __init_axi_opt(void)
{
	cur_axi_op = &pxa1928_axi_bootop;
}


static int pxa1928_vco_get_kvco(unsigned long rate)
{
	if (rate < 1350000000)
		return 0x8;
	else if (rate >= 1350000000 && rate < 1500000000)
		return 0x9;
	else if (rate >= 1500000000 && rate < 1750000000)
		return 0xa;
	else if (rate >= 1750000000 && rate < 2000000000)
		return 0xb;
	else if (rate >= 2000000000 && rate < 2200000000UL)
		return 0xc;
	else if (rate >= 2200000000UL && rate < 2400000000UL)
		return 0xd;
	else if (rate >= 2400000000UL && rate < 2600000000UL)
		return 0xe;
	else
		return 0xf;
}

#define VCO_PRATE_26M	(26)
#define VCO_REFDIV	(3)
#define VCO_MIN_PXA1928	(1200)
#define VCO_MAX_PXA1928	(3200)
#define TO_MHZ		(1000000)
static void pll_round_rate(unsigned long rate,
		struct pll_freq_tb *pll_freq)
{
	int div_sel, div_sel_max = 7, fbd, fbd_min, fbd_max;
	unsigned long pre_vco, vco_rate, offset, offset_min, prate;
	uint64_t div_result;

	pre_vco = VCO_PRATE_26M * 4 / VCO_REFDIV;
	fbd_min = (VCO_MIN_PXA1928 + pre_vco / 2) / pre_vco;
	fbd_max = VCO_MAX_PXA1928 / (pre_vco + 1);

	rate /= TO_MHZ;
	if (rate > (pre_vco * fbd_max / 2)) {
		rate = pre_vco * fbd_max / 2;
		printf("rate was too high (<=%luM)\n", rate);
	} else if (rate < ((pre_vco * fbd_min) >> div_sel_max)) {
		rate = pre_vco * fbd_min >> div_sel_max;
		printf("rate was too low (>=%luM)\n", rate);
	}

	offset_min = pre_vco * fbd_max / 2;
	prate = VCO_PRATE_26M * 4 * fbd_min / VCO_REFDIV;
	pll_freq->pll_clkout = prate >> div_sel_max;
	pll_freq->post_div_sel = div_sel_max;
	for (fbd = fbd_min; fbd <= fbd_max; fbd++) {
		vco_rate = VCO_PRATE_26M * 4 * fbd / VCO_REFDIV;
		for (div_sel = 1; div_sel <= div_sel_max; div_sel++) {
			if ((vco_rate >> div_sel) <= rate)
				break;
		}
		if (div_sel == div_sel_max + 1)
			break;
		offset = rate - (vco_rate >> div_sel);
		if (offset < offset_min) {
			offset_min = offset;
			prate = vco_rate;
			pll_freq->pll_clkout = prate >> div_sel;
			pll_freq->post_div_sel = div_sel;
		}
	}

	/* round vco */
	div_result = prate * pll_freq->refdiv + (VCO_PRATE_26M << 1);
	do_div(div_result, VCO_PRATE_26M << 2);
	pll_freq->fbdiv = div_result;
	div_result = (VCO_PRATE_26M << 2) * pll_freq->fbdiv;
	do_div(div_result, pll_freq->refdiv);
	pll_freq->fvco = div_result * TO_MHZ;
	pll_freq->kvco = pxa1928_vco_get_kvco(pll_freq->fvco);
	pll_freq->ssc_en = 1;
}

void pxa1928_pll3_set_rate(unsigned long rate)
{
	pll_round_rate(rate, &pll3_freq_tb[0]);
	turn_on_pll(PLL_ID_PLL3, PLL_SUBID_1);
}

void pxa1928_pll5_set_rate(unsigned long rate)
{
	pll_round_rate(rate, &pll5_freq_tb[0]);
	turn_on_pll(PLL_ID_PLL5, PLL_SUBID_1);
}

#ifndef CONFIG_PXA1928_FPGA
/* DVC map table
 * It is used in board config file to set voltage value in pmic.
 * example: (mV)
 * DVC	BUCK1(vcore)	BUCK3(vgc)	BUCK5(vmain)
 *   0		 950	       950		 950
 *   1		 950	       950		1050
 *   2		1150	      1050		1100
 *   3		1200	      1200		1200
 */
unsigned int pxa1928_dvc_map_table[][3][4] = {
	/* profile 0 */
	[0] = {
		[0] = {1200,	1200,	1200,	1200},	/* Buck 1 */
		[1] = {1200,	1200,	1200,	1200},	/* Buck 3 */
		[2] = {1200,	1200,	1200,	1200},	/* Buck 5 */
	},
};

static void __init_dvc_vol(void)
{
#ifndef CONFIG_PXA1928ZX
	set_dvc_level(DVC_ID_MP0_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_MP1_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_MP2L2ON_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_MP2L2OFF_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_D0_VL, DVC_VL_0011);
	set_dvc_level(DVC_ID_D0CG_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_D1_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_D2_VL, DVC_VL_0000);
#else
	int value, i;
	unsigned int *map;
	int profile = get_profile();

	map = pxa1928_dvc_map_table[profile][0];
	for (i = 0; i < 4; i++) {
		value = choose_buck_voltage(1, map[i] * 1000);
		pm8xx_power_write(PM822_BUCK1 + i, value);
	}

	map = pxa1928_dvc_map_table[profile][1];
	for (i = 0; i < 4; i++) {
		value = choose_buck_voltage(3, map[i] * 1000);
		pm8xx_power_write(PM822_BUCK3 + i, value);
	}

	map = pxa1928_dvc_map_table[profile][2];
	for (i = 0; i < 4; i++) {
		value = choose_buck_voltage(5, map[i] * 1000);
		pm8xx_power_write(PM822_BUCK5 + i, value);
	}

	/* Set DVC counters */
	/* FIXME: Just write the max value for temp use */
	writel(0xFFFFFFFF, PMUM_DVC_STB1);
	writel(0xFFFFFFFF, PMUM_DVC_STB2);
	writel(0xFFFFFFFF, PMUM_DVC_STB3);
	writel(0xFFFFFFFF, PMUM_DVC_STB4);
	writel(0x00000000, PMUM_DVC_DEBUG);

	set_dvc_level(DVC_ID_MP0_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_MP1_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_MP2L2ON_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_MP2L2OFF_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_D0_VL, DVC_VL_0011);
	set_dvc_level(DVC_ID_D0CG_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_D1_VL, DVC_VL_0000);
	set_dvc_level(DVC_ID_D2_VL, DVC_VL_0000);
	return;
#endif
}
#endif

static int _set_dvc_level(unsigned int *dvc_addr, enum dvc_id dvc_id, int dvc_level)
{
	unsigned int value, tmp;

#ifndef CONFIG_PXA1928ZX
	/* clear MSK_VL_MAIN_AP bit */
	value = __raw_readl_dfc(PMUA_DVC_CTRL);
	value &= ~(0x1 << 24);
	__raw_writel_dfc(value, PMUA_DVC_CTRL);
#endif

	if ((dvc_id == DVC_ID_MP0_VL) || (dvc_id == DVC_ID_D0_VL)) {
		/* clear irq bit */
		value = __raw_readl_dfc(PMUA_DVC_APSS);
		value |= (1 << 29);
		__raw_writel_dfc(value, PMUA_DVC_APSS);

		/* enable VC interrupt */
		value = __raw_readl_dfc(PMUA_DVC_APSS);
		value &= ~(1 << 30);
		__raw_writel_dfc(value, PMUA_DVC_APSS);
#ifdef CONFIG_PXA1928ZX
		value = __raw_readl_dfc(PMUA_AP_DEBUG1);
		value |= (1 << 24);
		__raw_writel_dfc(value, PMUA_AP_DEBUG1);
#endif
	}

	value = __raw_readl_dfc(dvc_addr);

#ifndef CONFIG_PXA1928ZX
	value &= ~(0xf << dvc_offset[dvc_id]);
#else
	value &= ~(0x7 << dvc_offset[dvc_id]);
#endif
	value |= dvc_level << dvc_offset[dvc_id];
	__raw_writel_dfc(value, dvc_addr);

	if ((dvc_id == DVC_ID_MP0_VL) || (dvc_id == DVC_ID_D0_VL)) {
#ifndef CONFIG_PXA1928ZX
		value = __raw_readl_dfc(PMUA_DVC_APSS);
		value |= (1 << 24);
		__raw_writel_dfc(value, PMUA_DVC_APSS);
#else
		value = __raw_readl_dfc(PMUA_AP_DEBUG1);
		value &= ~(1 << 24);
		__raw_writel_dfc(value, PMUA_AP_DEBUG1);
#endif
		for (tmp = 0; tmp < 1000000; tmp++) {
			if (__raw_readl_dfc(PMUA_DVC_APSS) & (1 << 31))
				break;
		}
		if (tmp == 1000000) {
			printf("no ack irq from dvc!\n");
			return -1;
		} else {
			/* clear irq bit */
			value = __raw_readl_dfc(PMUA_DVC_APSS);
			value |= (1 << 29);
			__raw_writel_dfc(value, PMUA_DVC_APSS);
			return 0;
		}
	}
	return 0;
}

static int set_dvc_level(enum dvc_id dvc_id, unsigned int dvc_level)
{
	unsigned int value;
	int ret;

#ifndef CONFIG_PXA1928ZX
	if (dvc_level > 15) {
#else

	if (dvc_level > 7) {
#endif
		printf("exceed the dvc value limitation!\n");
		return -1;
	}

	switch (dvc_id) {
	case DVC_ID_MP0_VL:
	case DVC_ID_MP1_VL:
	case DVC_ID_MP2L2ON_VL:
	case DVC_ID_MP2L2OFF_VL:
		value = __raw_readl_dfc(PMUA_DVC_APPCORESS);
		if (((value >>  dvc_offset[dvc_id]) & 0x7) == dvc_level)
			return 0;
		else
			ret = _set_dvc_level((u32 *)PMUA_DVC_APPCORESS, dvc_id, dvc_level);
		break;
	case DVC_ID_D0_VL:
	case DVC_ID_D0CG_VL:
	case DVC_ID_D1_VL:
	case DVC_ID_D2_VL:
		value = __raw_readl_dfc(PMUA_DVC_APSS);
		if (((value >>  dvc_offset[dvc_id]) & 0x7) == dvc_level)
			return 0;
		else
			ret = _set_dvc_level((u32 *)PMUA_DVC_APSS, dvc_id, dvc_level);
		break;
	default:
		return -1;
	}

	if ((dvc_id == DVC_ID_D0_VL) && !ret)
		cur_platform_opt->d0_dvc_level = dvc_level;

	return ret;
}

static void __init_fc_setting(void)
{
	return;
}

static struct pxa1928_core_opt *pclk_to_coreop
	(unsigned int rate, unsigned int *index)
{
	unsigned int idx = 0;
	struct pxa1928_core_opt *cop;

	list_for_each_entry(cop, &core_op_list, node) {
		if ((cop->pclk >= rate) ||
			list_is_last(&cop->node, &core_op_list))
			break;
		idx++;
	}

	*index = idx;
	return cop;
}

static unsigned int dclk_to_ddrop(unsigned int rate)
{
	unsigned int index;
	struct pxa1928_ddr_opt *op_array =
		cur_platform_opt->ddr_opt;
	unsigned int op_array_size =
		cur_platform_opt->ddr_opt_size;

	if (unlikely(rate > op_array[op_array_size - 1].dclk))
		return op_array_size - 1;

	for (index = 0; index < op_array_size; index++)
		if (op_array[index].dclk >= rate)
			break;

	return index;
}

static unsigned int clksel_to_clkrate(enum clk_sel src_clk_sel)
{
	unsigned int clk_rate = 0;
	switch (src_clk_sel) {
	case CLK_SRC_PLL1_312:
		clk_rate = 312;
		break;
	case CLK_SRC_PLL1_416:
		clk_rate = 416;
		break;
	case CLK_SRC_PLL1_624:
		clk_rate = 624;
		break;
	case CLK_SRC_PLL2_CLKOUT_1:
		clk_rate = 1386;
		break;
	case CLK_SRC_PLL2_CLKOUTP_1:
		clk_rate = 693;
		break;
	case CLK_SRC_PLL2_CLKOUT_2:
		clk_rate = 1057;
		break;
	case CLK_SRC_PLL2_CLKOUTP_2:
		clk_rate = 528;
		break;
	case CLK_SRC_PLL2_CLKOUT_3:
		clk_rate = 797;
		break;
	case CLK_SRC_PLL2_CLKOUTP_3:
		clk_rate = 531;
		break;
	case CLK_SRC_PLL4_CLKOUT_1:
		clk_rate = 1057;
		break;
	case CLK_SRC_PLL4_CLKOUTP_1:
		clk_rate = 528;
		break;
	case CLK_SRC_PLL4_CLKOUT_2:
		clk_rate = 1196;
		break;
	case CLK_SRC_PLL4_CLKOUTP_2:
		clk_rate = 598;
		break;
	case CLK_SRC_PLL5_CLKOUT_1:
		clk_rate = 528;
		break;
	case CLK_SRC_PLL5_CLKOUTP_1:
		clk_rate = 1057;
		break;
	default:
		break;
	}
	return clk_rate;
}

void clksel_to_pllid(enum clk_sel src_clk_sel, enum pll_id *pllid,
		enum pll_subid *pllsubid)
{
	switch (src_clk_sel) {
	case CLK_SRC_PLL1_312:
		*pllid = PLL_ID_PLL1;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL1_416:
		*pllid = PLL_ID_PLL1;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL1_624:
		*pllid = PLL_ID_PLL1;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL2_CLKOUT_1:
		*pllid = PLL_ID_PLL2;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL2_CLKOUTP_1:
		*pllid = PLL_ID_PLL2;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL2_CLKOUT_2:
		*pllid = PLL_ID_PLL2;
		*pllsubid = PLL_SUBID_2;
		break;
	case CLK_SRC_PLL2_CLKOUTP_2:
		*pllid = PLL_ID_PLL2;
		*pllsubid = PLL_SUBID_2;
		break;
	case CLK_SRC_PLL2_CLKOUT_3:
		*pllid = PLL_ID_PLL2;
		*pllsubid = PLL_SUBID_3;
		break;
	case CLK_SRC_PLL2_CLKOUTP_3:
		*pllid = PLL_ID_PLL2;
		*pllsubid = PLL_SUBID_3;
		break;
	case CLK_SRC_PLL4_CLKOUT_1:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL4_CLKOUTP_1:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL4_CLKOUT_2:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_2;
		break;
	case CLK_SRC_PLL4_CLKOUTP_2:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_2;
		break;
	case CLK_SRC_PLL4_CLKOUT_3:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_3;
		break;
	case CLK_SRC_PLL4_CLKOUTP_3:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_3;
		break;
	case CLK_SRC_PLL4_CLKOUT_4:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_4;
		break;
	case CLK_SRC_PLL4_CLKOUTP_4:
		*pllid = PLL_ID_PLL4;
		*pllsubid = PLL_SUBID_4;
		break;

	case CLK_SRC_PLL5_CLKOUT_1:
		*pllid = PLL_ID_PLL5;
		*pllsubid = PLL_SUBID_1;
		break;
	case CLK_SRC_PLL5_CLKOUTP_1:
		*pllid = PLL_ID_PLL5;
		*pllsubid = PLL_SUBID_1;
		break;
	default:
		break;
	}
	return;
}


static unsigned int clk_enable(enum clk_sel src_clk_sel)
{
	switch (src_clk_sel) {
	case CLK_SRC_PLL1_312:
		break;
	case CLK_SRC_PLL1_416:
		break;
	case CLK_SRC_PLL1_624:
		break;
	case CLK_SRC_PLL2_CLKOUT_1:
		turn_on_pll(PLL_ID_PLL2, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL2_CLKOUTP_1:
		turn_on_pll(PLL_ID_PLL2, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL2_CLKOUT_2:
		turn_on_pll(PLL_ID_PLL2, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL2_CLKOUTP_2:
		turn_on_pll(PLL_ID_PLL2, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL2_CLKOUT_3:
		turn_on_pll(PLL_ID_PLL2, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL2_CLKOUTP_3:
		turn_on_pll(PLL_ID_PLL2, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL4_CLKOUT_1:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL4_CLKOUTP_1:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL4_CLKOUT_2:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL4_CLKOUTP_2:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL4_CLKOUT_3:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL4_CLKOUTP_3:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL4_CLKOUT_4:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_4);
		break;
	case CLK_SRC_PLL4_CLKOUTP_4:
		turn_on_pll(PLL_ID_PLL4, PLL_SUBID_4);
		break;
	case CLK_SRC_PLL5_CLKOUT_1:
		turn_on_pll(PLL_ID_PLL5, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL5_CLKOUTP_1:
		turn_on_pll(PLL_ID_PLL5, PLL_SUBID_1);
		break;
	default:
		break;
	}
	return 0;
}

static unsigned int clk_disable(enum clk_sel src_clk_sel)
{
	switch (src_clk_sel) {
	case CLK_SRC_PLL1_312:
		break;
	case CLK_SRC_PLL1_416:
		break;
	case CLK_SRC_PLL1_624:
		break;
	case CLK_SRC_PLL2_CLKOUT_1:
		turn_off_pll(PLL_ID_PLL2, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL2_CLKOUTP_1:
		turn_off_pll(PLL_ID_PLL2, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL2_CLKOUT_2:
		turn_off_pll(PLL_ID_PLL2, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL2_CLKOUTP_2:
		turn_off_pll(PLL_ID_PLL2, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL2_CLKOUT_3:
		turn_off_pll(PLL_ID_PLL2, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL2_CLKOUTP_3:
		turn_off_pll(PLL_ID_PLL2, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL4_CLKOUT_1:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL4_CLKOUTP_1:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL4_CLKOUT_2:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL4_CLKOUTP_2:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_2);
		break;
	case CLK_SRC_PLL4_CLKOUT_3:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL4_CLKOUTP_3:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_3);
		break;
	case CLK_SRC_PLL4_CLKOUT_4:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_4);
		break;
	case CLK_SRC_PLL4_CLKOUTP_4:
		turn_off_pll(PLL_ID_PLL4, PLL_SUBID_4);
		break;
	case CLK_SRC_PLL5_CLKOUT_1:
		turn_off_pll(PLL_ID_PLL5, PLL_SUBID_1);
		break;
	case CLK_SRC_PLL5_CLKOUTP_1:
		turn_off_pll(PLL_ID_PLL5, PLL_SUBID_1);
		break;
	default:
		break;
	}
	return 0;
}

static void get_cur_core_op(struct pxa1928_core_opt *cop)
{
	unsigned int src, div;
	unsigned int reg_val;
	union pll_cr pllx_cr;
	int i;

	reg_val = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	src = (reg_val >> 30) & 0x3;
	if (src == 0x0)	/* use PLL1_624 */
		cop->core_clk_src_rate = 624;
	else if (src == 0x01) {/* use PLL2 CLKOUT */
		pllx_cr.val = __raw_readl_dfc(mpmu_pll_cr[PLL_ID_PLL2]);
		for (i = 0; i < 3; i++) {
			if (pllx_cr.bit.fbdiv == pll2_freq_tb[i].fbdiv)
				break;
		}
		cop->core_clk_src_rate = pll2_freq_tb[i].pll_clkout/1000000;
	} else if (src == 0x3) {/* use PLL2 CLKOUTP */
		pllx_cr.val = __raw_readl_dfc(mpmu_pll_cr[PLL_ID_PLL2]);
		for (i = 0; i < 3; i++) {
			if (pllx_cr.bit.fbdiv == pll2_freq_tb[i].fbdiv)
				break;
		}

		cop->core_clk_src_rate = pll2_freq_tb[i].pll_clkoutp/1000000;
	}
	div = reg_val & 0x7;
	cop->pclk = cop->core_clk_src_rate/div;
}

static void get_cur_ddr_op(struct pxa1928_ddr_opt *cop)
{
	return;
}
static void core_fc_seq(struct pxa1928_core_opt *cop,
			    struct pxa1928_core_opt *top)
{
	u32 reg;
	u32 axim, axim_cur;
	/*
	 * add check here to avoid the case default boot up PP
	 * has different pdclk and paclk definition with PP used
	 * in production
	 */
	if ((cop->core_clk_src_rate == top->core_clk_src_rate)
	    && (cop->pclk == top->pclk)) {
		printf("current rate = target rate!\n");
		return;
	}

#ifndef CONFIG_PXA1928ZX
	/* let FC_EN always 0x1 */
	reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	reg |= (0x1 << 3);
	__raw_writel_dfc(reg, PMUA_COREAPSS_CLKCTRL);
	/* dummy read */
	reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
#endif
	axim = top->baclk_div_regval;
	reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	axim_cur = (reg >> 8) & 0x7;

	/* if axim is getting slower, we should
	 * change axim freq firtst, then core freq */
	if (axim > axim_cur) {
		/* set axi bus div */
		reg &= ~(0x7 << 8);
		reg |= (top->baclk_div_regval << 8);
		__raw_writel_dfc(reg, PMUA_COREAPSS_CLKCTRL);
		/* dummy read */
		reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	}
#ifndef CONFIG_PXA1928ZX
	/* set WTC/RTC
	 * if pp < 624Mhz, set RTC/WTC = 00/00
	 * if pp >= 624Mhz and pp <= 1500Mhz, set RTC/WTC = 01/01
	 * if pp > 1500Mhz, set RTC/WTC = 10/10 for all memory
	 * excep RF2P; set RTC/WTC = 10/01 for RF2P */
	if (top->pclk > cop->pclk) {
		reg = __raw_readl_dfc(PMUA_COREAPSS_CFG);
		reg &= ~(COREAPSS_CFG_WTCRTC_MSK);
		if (top->pclk > 1500000)
			reg |= (COREAPSS_CFG_WTCRTC_EHMIPS);
		else
			reg |= (COREAPSS_CFG_WTCRTC_HMIPS);
		__raw_writel_dfc(reg, PMUA_COREAPSS_CFG);

		reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
		/* set rtc/wtc sel */
		reg &= ~(0x1 << 27);
		reg |= (top->rtc_wtc_sel_regval << 27);
		__raw_writel_dfc(reg, PMUA_COREAPSS_CLKCTRL);
		/* dummy read */
		reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	}
#endif

#ifndef CONFIG_PXA1928ZX
	reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	/* set core clk src sel */
	reg &= ~(0x7 << 29);
	reg |= (top->core_clk_sel_regval << 29);
#else
	reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	/* set core clk src sel */
	reg &= ~(0x3 << 30);
	reg |= (top->core_clk_sel_regval << 30);
	/* set rtc/wtc sel */
	reg &= ~(0x1 << 27);
	reg |= (top->rtc_wtc_sel_regval << 27);
#endif
	/* set periphclk div */
	reg &= ~(0x7 << 12);
	reg |= (top->periphclk_div_regval << 12);

	/* set pclk div */
	reg &= ~(0x7 << 0);
	reg |= (top->pclk_div_regval << 0);


	/* if axim is getting faster, we should
	 * change core frequency firtst, then axim freq */
	if (axim < axim_cur) {
		__raw_writel_dfc(reg, PMUA_COREAPSS_CLKCTRL);
		/* dummy read */
		reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
		reg &= ~(0x7 << 8);
		reg |= (top->baclk_div_regval << 8);
	}

	__raw_writel_dfc(reg, PMUA_COREAPSS_CLKCTRL);
	/* dummy read */
	reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);

#ifndef CONFIG_PXA1928ZX
	/* set WTC/RTC
	 * if pp < 624Mhz, set RTC/WTC = 00/00
	 * if pp >= 624Mhz and pp <= 1500Mhz, set RTC/WTC = 01/01
	 * if pp > 1500Mhz, set RTC/WTC = 10/10 for all memory
	 * excep RF2P; set RTC/WTC = 10/01 for RF2P */
	if (top->pclk < cop->pclk) {
		reg = __raw_readl_dfc(PMUA_COREAPSS_CFG);
		reg &= ~(COREAPSS_CFG_WTCRTC_MSK);
		if (top->pclk > 1500000)
			reg |= (COREAPSS_CFG_WTCRTC_EHMIPS);
		else
			reg |= (COREAPSS_CFG_WTCRTC_HMIPS);
		__raw_writel_dfc(reg, PMUA_COREAPSS_CFG);

		reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
		/* set rtc/wtc sel */
		reg &= ~(0x1 << 27);
		reg |= (top->rtc_wtc_sel_regval << 27);
		__raw_writel_dfc(reg, PMUA_COREAPSS_CLKCTRL);
		/* dummy read */
		reg = __raw_readl_dfc(PMUA_COREAPSS_CLKCTRL);
	}
#endif
	return;
}

static int set_core_freq(struct pxa1928_core_opt *old, struct pxa1928_core_opt *new)
{
	struct pxa1928_core_opt cop;
	int ret = 0;

	printf("CORE FC start: %u -> %u\n",
		old->pclk, new->pclk);

	memcpy(&cop, old, sizeof(struct pxa1928_core_opt));
	/*
	 * We do NOT check it here as system boot up
	 * PP may be has different pdclk,paclk with
	 * PP table we defined.Only check the freq
	 * after every FC.
	 */
	get_cur_core_op(&cop);
	if (unlikely((cop.core_clk_src_rate != old->core_clk_src_rate) ||
		(cop.pclk != old->pclk) ||
		(cop.baclk != old->baclk) ||
		(cop.periphclk != old->periphclk))) {
		printf("psrc pclk baclk periphclk\n");
		printf("OLD %d %d %d %d\n", old->core_clk_src_rate,
		       old->pclk, old->baclk, old->periphclk);
		printf("CUR %d %d %d %d\n", cop.core_clk_src_rate,
		       cop.pclk, cop.baclk, cop.periphclk);
		printf("NEW %d %d %d %d\n", new->core_clk_src_rate,
		       new->pclk, new->baclk, new->periphclk);
	}
	clk_enable(new->core_clk_sel);
	core_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct pxa1928_core_opt));
	get_cur_core_op(&cop);
	if (unlikely((cop.core_clk_src_rate != new->core_clk_src_rate) ||
		(cop.pclk != new->pclk))) {
		printf("unsuccessful frequency change!\n");
		printf("psrc pclk\n");
		printf("CUR %d %d\n", cop.core_clk_src_rate,
		       cop.pclk);
		printf("NEW %d %d\n", new->core_clk_src_rate,
			new->pclk);
		ret = -EAGAIN;
		clk_disable(new->core_clk_sel);
		goto out;
	}

	clk_disable(old->core_clk_sel);
out:
	printf("CORE FC end: %u -> %u\n",
		old->pclk, new->pclk);
	return ret;
}

static int pxa1928_core_setrate(unsigned long rate)
{
	struct pxa1928_core_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	enum pll_id pllid_old, pllid_new;
	enum pll_subid pllsubid_old, pllsubid_new;
	struct pxa1928_core_opt *op_array =
		cur_platform_opt->core_opt;

	if (unlikely(!cur_core_op))
		cur_core_op = &pxa1928_core_bootop;
	md_new = pclk_to_coreop(rate, &index);
	md_old = cur_core_op;

	/*
	 * Switching pll1_1248 and pll3p may generate glitch
	 * step 1),2),3) is neccessary
	 */
	clksel_to_pllid(md_old->core_clk_sel, &pllid_old, &pllsubid_old);
	clksel_to_pllid(md_new->core_clk_sel, &pllid_new, &pllsubid_new);
	if ((pllid_old != pllid_new) && (pllid_old != PLL_ID_PLL1)
			&& (pllid_new != PLL_ID_PLL1)) {
		/* 1) use startup op(op0) as a bridge */
		ret = set_core_freq(md_old, &op_array[0]);
		if (ret)
			return ret;
		/* 2) switch to op which uses pll1_1248/pll3p */
		ret = set_core_freq(&op_array[0], md_new);
	} else {
		ret = set_core_freq(md_old, md_new);
	}

	if (ret)
		goto out;
	cur_core_op = md_new;
out:
	return ret;
}

static int wait4ddrfc_done(void)
{
	/* FIXME timeout is legacay for 988
	 * 1 do we need this timeout ?
	 * 2 is 5000000 a reasonable value ?
	 */
	int timeout = 5000000;
	int ret = 0;
#ifdef REG_DRYRUN_DEBUG
	return;
#endif

	while (((1 << 28) & __raw_readl_dfc(PMUA_MC_CLK_RES_CTRL)) && timeout)
		timeout--;
	if (timeout <= 0) {
		printf("DDR frequency change timeout!\n");
		ret = 1;
	}
	return ret;

}

static void wait4axifc_done(void)
{
	int timeout = 1000;
#ifdef REG_DRYRUN_DEBUG
	return;
#endif
	while (((1 << 30) & __raw_readl_dfc(PMUA_CC_PJ)) && timeout)
		timeout--;
	if (timeout <= 0)
		printf("AXI frequency change timeout!\n");
}

enum ddr_dfc_type {
	DDR_SW_DFC = 0,
	DDR_HW_DFC,
};
#ifndef CONFIG_PXA1928ZX
union ddrdfc_ctrl {
	struct {
		unsigned int dfc_hw_en:1;
		unsigned int ddr_type:1;
		unsigned int ddrpll_ctrl:1;
		unsigned int dfc_freq_tbl_en:1;
		unsigned int reserved0:4;
		unsigned int dfc_fp_idx:5;
		unsigned int dfc_pllpark_idx:5;
		unsigned int reserved1:2;
		unsigned int dfc_fp_index_cur:5;
		unsigned int reserved2:3;
		unsigned int dfc_in_progress:1;
		unsigned int dfc_int_clr:1;
		unsigned int dfc_int_msk:1;
		unsigned int dfc_int;
	} b;
	unsigned int v;
};
#endif


#ifdef CONFIG_DDR_HW_DFC

#define DFC_LEVEL(i)	(0xD4282700UL + i * 8)

union ddrdfc_fp_lwr {
	struct {
		unsigned int ddr_vl:4;
		unsigned int mc_reg_tbl_sel:5;
		unsigned int rtcwtc_sel:1;
		unsigned int sel_4xmode:1;
		unsigned int sel_ddrpll:1;
		unsigned int reserved:20;
	} b;
	unsigned int v;
};


union ddrdfc_fp_upr {
	struct {
		unsigned int fp_upr0:3;
		unsigned int fp_upr1:5;
		unsigned int fp_upr2:7;
		unsigned int fp_upr3:3;
		unsigned int fp_upr4:1;
		unsigned int fp_upr5:3;
		unsigned int reserved:10;
	} b;
	unsigned int v;
};
static void ddr_hw_dfc_init(void)
{
	int i;
	struct pxa1928_ddr_opt *cop;
	union ddrdfc_fp_upr upr;
	union ddrdfc_fp_lwr lwr;

	struct pxa1928_ddr_opt *op_array =
		cur_platform_opt->ddr_opt;
	unsigned int ddr_opt_size = 0;
	struct pll_freq_tb *pll_freq = NULL;

	clk_enable(CLK_SRC_PLL4_CLKOUT_1);
	ddr_opt_size = cur_platform_opt->ddr_opt_size;

	for (i = 0; i < ddr_opt_size; i++) {
		cop = &op_array[i];
		lwr.v = __raw_readl_dfc(DFC_LEVEL(i));
		upr.v = __raw_readl_dfc(DFC_LEVEL(i) + 4);
		lwr.b.ddr_vl = cop->ddr_vl;
		lwr.b.mc_reg_tbl_sel = cop->ddr_tbl_index * 2 + 1;
		lwr.b.rtcwtc_sel = cop->rtcwtc_lvl;
		/* we don't use 4x mode for now */
		lwr.b.sel_4xmode = 0;
		lwr.b.sel_ddrpll =
			(cop->mc_clk_src_sel_regval == 1) ? 1 : 0;
		if (cop->mc_clk_src_sel_regval == 1) {
			switch (cop->ddr_clk_sel) {
			case CLK_SRC_PLL4_CLKOUT_1:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_1);
				break;
			case CLK_SRC_PLL4_CLKOUTP_1:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_1);
				break;
			case CLK_SRC_PLL4_CLKOUT_2:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_2);
				break;
			case CLK_SRC_PLL4_CLKOUTP_2:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_2);
				break;
			case CLK_SRC_PLL4_CLKOUT_3:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_3);
				break;
			case CLK_SRC_PLL4_CLKOUTP_3:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_3);
				break;
			case CLK_SRC_PLL4_CLKOUT_4:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_4);
				break;
			case CLK_SRC_PLL4_CLKOUTP_4:
				pll_freq = get_pll_freq_tb(PLL_ID_PLL4, PLL_SUBID_4);
				break;
			default:
				break;
			}

			if (pll_freq == NULL) {
				printf("DDR HW DFC error\n");
				return;
			}

			upr.b.fp_upr5 = pll_freq->icp;
			upr.b.fp_upr4 = pll_freq->bw_sel;
			upr.b.fp_upr3 = pll_freq->post_div_sel;
			upr.b.fp_upr2 = pll_freq->fbdiv;
			upr.b.fp_upr1 = pll_freq->refdiv;
			upr.b.fp_upr0 = pll_freq->kvco;
		} else {
			/* power off pll4 */
			upr.b.fp_upr4 = 1;
			upr.b.fp_upr3 = cop->dclk_div_regval;
			upr.b.fp_upr0 = cop->pmu_ddr_clk_sel_regval;
			upr.b.reserved = 0;
		}

		__raw_writel_dfc(lwr.v, DFC_LEVEL(i));
		__raw_writel_dfc(upr.v, DFC_LEVEL(i) + 4);

		__raw_writel_dfc(cop->rtcwtc, PMUA_MC_MEM_CTRL);
	}
}

static int  wait4hwdfc_done(void)
{
	u32 timeout = 5000000;
	int ret = 0;

	/* busy wait for hwdfc done */
	while (!((1 << 31) & __raw_readl_dfc(PMUA_DDRDFC_CTRL)) &&
			timeout)
		timeout--;

	if (unlikely(timeout <= 0)) {
		printf("AP HW DDR frequency change timeout!\n");
		ret = 1;
	}
	return ret;
}


static int ddr_hwdfc_seq(unsigned int level)
{
	union ddrdfc_ctrl ctrl;
	int ret = 0;
	ctrl.v = __raw_readl_dfc(PMUA_DDRDFC_CTRL);
	/* enable hw-based DFC */
	ctrl.b.dfc_hw_en = 0x1;
	/* DDR PLL is controlled by hw-dfc FSM */
	ctrl.b.ddrpll_ctrl = 0x1;
	/* enable DFC_FREQ_TBL_EN */
	ctrl.b.dfc_freq_tbl_en = 0x1;
	/* init DFC_PLLPARK_IDX */
	ctrl.b.dfc_pllpark_idx = 0x0;
	__raw_writel_dfc(ctrl.v, PMUA_DDRDFC_CTRL);



	/* set DDR_TYPE */
	ctrl.v = __raw_readl_dfc(PMUA_DDRDFC_CTRL);
	if ((cur_platform_opt->ddrtype == LPDDR2_533M) ||
		(cur_platform_opt->ddrtype == LPDDR3_600M) ||
		(cur_platform_opt->ddrtype == LPDDR3_HYN2G) ||
		(cur_platform_opt->ddrtype == LPDDR3_ELP2G) ||
		(cur_platform_opt->ddrtype == LPDDR3_HYN2G_DIS)) {
		ctrl.b.ddr_type = 0x0;
	} else {
		ctrl.b.ddr_type = 0x1;
	}
	__raw_writel_dfc(ctrl.v, PMUA_DDRDFC_CTRL);



	/* 1. trigger AP HWDFC */
	ctrl.v = __raw_readl_dfc(PMUA_DDRDFC_CTRL);
	ctrl.b.dfc_fp_idx = level;
	__raw_writel_dfc(ctrl.v, PMUA_DDRDFC_CTRL);

	/* 2. wait for dfc completes */
	ret = wait4hwdfc_done();

	/* 3. clear dfc interrupt */
	ctrl.v = __raw_readl_dfc(PMUA_DDRDFC_CTRL);
	ctrl.b.dfc_int_clr = 1;
	__raw_writel_dfc(ctrl.v, PMUA_DDRDFC_CTRL);

	return ret;
}

static int set_ddr_hw_freq(struct pxa1928_ddr_opt *old,
	struct pxa1928_ddr_opt *new)
{
	struct pxa1928_ddr_opt cop;
	int ret = 0;

	printf("DDR HW FC start: DDR %u -> %u\n",
		old->dclk, new->dclk);

	memcpy(&cop, old, sizeof(struct pxa1928_ddr_opt));
	/*
	 * We do NOT check it here as system boot up
	 * PP may be has different dclk and aclk combination
	 * with PP table we defined.Only check the freq
	 * after every FC.
	 */
	get_cur_ddr_op(&cop);
	if ((cop.ddr_clk_src_rate == new->ddr_clk_src_rate) &&
		(cop.dclk == new->dclk)) {
		printf("ddr dfc return with same old & new op\n");
		return 0;
	}

	/* update the wtc/rtc */
	__raw_writel_dfc(new->rtcwtc, PMUA_MC_MEM_CTRL);

	ret = ddr_hwdfc_seq(new->ddr_freq_level);
	if (!ret)
		printf("DDR HW FC end: DDR %u -> %u\n", old->dclk, new->dclk);
	else
		printf("DDR HW FC fail: DDR %u -> %u\n", old->dclk, new->dclk);

	return ret;
}

#endif


int  ddr_fc_seq(struct pxa1928_ddr_opt *cop,
			    struct pxa1928_ddr_opt *top)
{
	int ret = 0;
	union mc_res_ctrl mc_res;
#ifdef CONFIG_PXA1928ZX
	union pmua_cc_pj cc;
	u32 tmp;
#else
	union ddrdfc_ctrl ctrl;
#endif


#ifndef CONFIG_PXA1928ZX
	ctrl.v = __raw_readl_dfc(PMUA_DDRDFC_CTRL);
	/* disable hw-based DFC */
	ctrl.b.dfc_hw_en = 0x0;
	/* DDR PLL is controlled by software */
	ctrl.b.ddrpll_ctrl = 0x0;
	__raw_writel_dfc(ctrl.v, PMUA_DDRDFC_CTRL);
#endif


	/* issue DDR FC */
	if ((cop->ddr_clk_src_rate != top->ddr_clk_src_rate) ||
	    (cop->dclk != top->dclk)) {

#ifdef CONFIG_PXA1928ZX
		/* 1 make sure cc_pj[27] and cc_sp[27] are set */
		cc.v = __raw_readl_dfc(PMUA_CC_PJ);
		if (cc.b.pj_ddrfc_vote == 0) {
			cc.b.pj_ddrfc_vote = 1;
			__raw_writel_dfc(cc.v, PMUA_CC_PJ);
		}

		tmp = __raw_readl_dfc(PMUA_CC_SP);
		if ((tmp & 1 << 27) == 0) {
			tmp |= 1 << 27;
			__raw_writel_dfc(tmp, PMUA_CC_SP);
		}

		/* 2 Igore SP status */
		tmp = readl(PMUA_DEBUG);
		tmp |= 0x3;
		__raw_writel_dfc(tmp, PMUA_DEBUG);
#endif

#ifndef CONFIG_PXA1928ZX
		__raw_writel(top->rtcwtc, PMUA_MC_MEM_CTRL);
#endif
		/* 3 Fill DFC table first */
		mc_res.v = __raw_readl_dfc(PMUA_MC_CLK_RES_CTRL);
		if (top->dclk < cur_ddr_op->dclk) {
			if ((cur_platform_opt->ddrtype == LPDDR2_533M) ||
				(cur_platform_opt->ddrtype == LPDDR3_600M) ||
				(cur_platform_opt->ddrtype == LPDDR3_ELP2G) ||
				(cur_platform_opt->ddrtype == LPDDR3_HYN2G) ||
				(cur_platform_opt->ddrtype == LPDDR3_HYN2G_DIS)) {
				mc_res.b.mc_dfc_type = 0;
			} else {
				mc_res.b.mc_dfc_type = 2;
			}
				mc_res.b.mc_reg_table = top->ddr_tbl_index * 2;
		} else {
			if ((cur_platform_opt->ddrtype == LPDDR2_533M) ||
				(cur_platform_opt->ddrtype == LPDDR3_600M) ||
				(cur_platform_opt->ddrtype == LPDDR3_ELP2G) ||
				(cur_platform_opt->ddrtype == LPDDR3_HYN2G) ||
				(cur_platform_opt->ddrtype == LPDDR3_HYN2G_DIS)) {
				mc_res.b.mc_dfc_type = 1;
			} else {
				mc_res.b.mc_dfc_type = 3;
			}
			mc_res.b.mc_dfc_type = 1;
			mc_res.b.mc_reg_table = top->ddr_tbl_index * 2 + 1;
		}
		mc_res.b.mc_reg_table_en = 1;
#ifndef CONFIG_PXA1928ZX
		mc_res.b.rtcwtc_sel = top->rtcwtc_lvl;
#endif
		__raw_writel_dfc(mc_res.v, PMUA_MC_CLK_RES_CTRL);

		/* 4 Program new clock setting */
		mc_res.v = __raw_readl_dfc(PMUA_MC_CLK_RES_CTRL);
		mc_res.b.mc_clk_src_sel = top->mc_clk_src_sel_regval;
		mc_res.b.pmu_ddr_sel = top->pmu_ddr_clk_sel_regval;
		mc_res.b.pmu_ddr_div = top->dclk_div_regval;
		__raw_writel_dfc(mc_res.v, PMUA_MC_CLK_RES_CTRL);

		/* 5 Make sure it is 2x mode */
		mc_res.v = readl(PMUA_MC_CLK_RES_CTRL);
		if (mc_res.b.mc_4x_mode == 1) {
			mc_res.b.mc_4x_mode = 0;
			writel(mc_res.v, PMUA_MC_CLK_RES_CTRL);
		}

		/* 6 trigger DDR DFC */
		mc_res.v = readl(PMUA_MC_CLK_RES_CTRL);
		mc_res.b.mc_clk_dfc = 1;
		__raw_writel_dfc(mc_res.v, PMUA_MC_CLK_RES_CTRL);

		/* 7 wait for ddr fc done */
		ret  = wait4ddrfc_done();
	}
	return ret;
}

static int set_ddr_freq(struct pxa1928_ddr_opt *old,
	struct pxa1928_ddr_opt *new)
{
	struct pxa1928_ddr_opt cop;
	int ret = 0;

	printf("DDR FC start: DDR %u -> %u\n",
		old->dclk, new->dclk);

	memcpy(&cop, old, sizeof(struct pxa1928_ddr_opt));
	/*
	 * We do NOT check it here as system boot up
	 * PP may be has different dclk and aclk combination
	 * with PP table we defined.Only check the freq
	 * after every FC.
	 */
	get_cur_ddr_op(&cop);
	if ((cop.ddr_clk_src_rate == new->ddr_clk_src_rate) &&
		(cop.dclk == new->dclk)) {
		printf("ddr dfc return with same old & new op\n");
		return 0;
	}

	clk_enable(new->ddr_clk_sel);
	ret = ddr_fc_seq(&cop, new);
	if (!ret) {
		clk_disable(old->ddr_clk_sel);
		printf("DDR FC end: DDR %u -> %u\n",
			old->dclk, new->dclk);
	} else {
		clk_disable(new->ddr_clk_sel);
	}
	return ret;
}

static int pxa1928_ddr_setrate(enum ddr_dfc_type dfc_type, unsigned long rate)
{
	struct pxa1928_ddr_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	struct pxa1928_ddr_opt *op_array =
		cur_platform_opt->ddr_opt;

	if (unlikely(!cur_ddr_op))
		cur_ddr_op = &pxa1928_ddr_bootop;
	index = dclk_to_ddrop(rate);
	md_new = &op_array[index];
	md_old = cur_ddr_op;
	if (dfc_type == DDR_HW_DFC) {
#ifdef CONFIG_DDR_HW_DFC
		ret = set_ddr_hw_freq(md_old, md_new);
#endif
	} else {
		ret = set_ddr_freq(md_old, md_new);
	}
	if (ret)
		goto out;
	cur_ddr_op = md_new;
out:
	return ret;
}

static unsigned int axi_rate2_op_index(unsigned long rate)
{
	struct pxa1928_axi_opt *op_array;
	unsigned int axi_pp_num;
	int i;

	axi_pp_num = cur_platform_opt->axi_opt_size;
	op_array = cur_platform_opt->axi_opt;

	if (rate > op_array[axi_pp_num - 1].aclk2)
		return axi_pp_num - 1;

	for (i = 0; i < axi_pp_num; i++)
		if (op_array[i].aclk2 >= rate)
			break;
	return i;
}


static void axi_fc_seq(struct pxa1928_axi_opt *top)
{
	union pmua_bus_clk_res_ctrl bus_res;
	union pmua_cc_pj cc;
	union pmua_cc2_pj cc2;

	/* 1 Program PLL source select */
	bus_res.v = __raw_readl_dfc(PMUA_BUS_CLK_RES_CTRL);
	bus_res.b.axiclksel = top->axi_clk_sel_regval;
	__raw_writel_dfc(bus_res.v, PMUA_BUS_CLK_RES_CTRL);
#ifndef CONFIG_PXA1928ZX
	/* 2 Program div */
	cc2.v = __raw_readl_dfc(PMUA_CC2_PJ);
	cc2.b.axi2_div = top->aclk2_div_regval;
	__raw_writel_dfc(cc2.v, PMUA_CC2_PJ);

#else
	/* 2 Program div */
	cc.v = __raw_readl_dfc(PMUA_CC_PJ);
	cc.b.axi_div = top->aclk1_div_regval;
	__raw_writel_dfc(cc.v, PMUA_CC_PJ);

	cc2.v = __raw_readl_dfc(PMUA_CC2_PJ);
	cc2.b.axi2_div = top->aclk2_div_regval;
	cc2.b.axi3_div = top->aclk3_div_regval;
	cc2.b.axi4_div = top->aclk4_div_regval;
	__raw_writel_dfc(cc2.v, PMUA_CC2_PJ);
#endif
	/* 3 trigger AXI DFC */
	cc.v = __raw_readl_dfc(PMUA_CC_PJ);
	cc.b.aclk_fc_req = 1;
	__raw_writel_dfc(cc.v, PMUA_CC_PJ);

	/* 4 wait for DFC done */
	wait4axifc_done();
}

static int set_axi_freq(struct pxa1928_axi_opt *old,
		struct pxa1928_axi_opt *new)
{
	int ret = 0;

#ifndef CONFIG_PXA1928ZX
	if ((old->aclk2 == new->aclk2)) {
		printf("axi dfc return with same old & new op\n");
		return 0;
	}
#else
	if ((old->aclk1 == new->aclk1) && (old->aclk2 == new->aclk2)
			&& (old->aclk3 == new->aclk3) &&
			(old->aclk4 == new->aclk4)) {
		printf("axi dfc return with same old & new op\n");
		return 0;
	}
#endif
	axi_fc_seq(new);
	debug("AXI set_freq end: old %u, new %u\n",
		old->aclk2, new->aclk2);
	return ret;
}

static int pxa1928_axi_setrate(unsigned long rate)
{
	struct pxa1928_axi_opt *new_op, *op_array;
	int index;
	int ret;
	if (unlikely(!cur_axi_op))
		cur_axi_op = &pxa1928_axi_bootop;

	index = axi_rate2_op_index(rate);
	op_array = cur_platform_opt->axi_opt;

	new_op = &op_array[index];
	if (new_op == cur_axi_op)
		return 0;

	ret = set_axi_freq(cur_axi_op, new_op);
	cur_axi_op = new_op;
	return ret;
}

/*
 * This function is called from board level or cmd to
 * issue the core, ddr frequency change.
 */
#ifdef	BIND_OP_SUPPORT
int setop(int opnum)
{
	unsigned int cur_volt_main, cur_volt_core, cur_volt_gc;
	int ret = 0;
	struct pxa1928_group_opt *op_array =
		cur_platform_opt->group_opt;
	unsigned int op_size = cur_platform_opt->group_opt_size;

	if (opnum >= op_size) {
		printf("opnum out of range!\n");
		return -EAGAIN;
	}
	cur_volt_main = get_volt(VCC_MAIN, cur_platform_opt->d0_dvc_level);
	cur_volt_core = get_volt(VCC_MAIN_CORE, cur_platform_opt->d0_dvc_level);
	cur_volt_gc = get_volt(VCC_MAIN_GC, cur_platform_opt->d0_dvc_level);
	if ((op_array[opnum].v_vcc_main > cur_volt_main) ||
		(op_array[opnum].v_vcc_main_core > cur_volt_core) ||
		(op_array[opnum].v_vcc_main_gc > cur_volt_gc)) {
		cur_volt_main = op_array[opnum].v_vcc_main;
		cur_volt_core = op_array[opnum].v_vcc_main_core;
		cur_volt_gc = op_array[opnum].v_vcc_main_gc;
		ret = set_group_volt(&cur_volt_main, &cur_volt_core,
				&cur_volt_gc, cur_platform_opt->d0_dvc_level);
	}
	if (ret) {
		printf("Increase volatge failed!\n");
		return -1;
	}


	/* set core frequency */
	pxa1928_core_setrate(op_array[opnum].pclk);

	/* set ddr frequency */
	pxa1928_ddr_setrate(DDR_SW_DFC, op_array[opnum].dclk);

	if ((op_array[opnum].v_vcc_main < cur_volt_main) ||
		(op_array[opnum].v_vcc_main_core < cur_volt_core) ||
		(op_array[opnum].v_vcc_main_gc < cur_volt_gc)) {
		cur_volt_main = op_array[opnum].v_vcc_main;
		cur_volt_core = op_array[opnum].v_vcc_main_core;
		cur_volt_gc = op_array[opnum].v_vcc_main_gc;
		ret = set_group_volt(&cur_volt_main, &cur_volt_core,
				&cur_volt_gc, cur_platform_opt->d0_dvc_level);
	}
	if (ret) {
		printf("Increase volatge failed!\n");
		return -1;
	}
	return 0;
}

void __init_group_opt(int profile)
{
	unsigned int i = 0, j = 0, pwr_pin = 0;
	int max_require;
	int *vm_millivolts_tmp;
	unsigned int (*freqs_cmb)[VL_MAX];
	struct pxa1928_group_opt *op_array =
		cur_platform_opt->group_opt;
	unsigned int op_size = cur_platform_opt->group_opt_size;

	for (pwr_pin = VCC_MAIN; pwr_pin < VCC_PIN_MAX; pwr_pin++) {
		if (pwr_pin == VCC_MAIN) {
			freqs_cmb = freqs_vcc_main_limit;
			vm_millivolts_tmp = volt_vcc_main_svc[profile];
		} else if (pwr_pin == VCC_MAIN_CORE) {
			freqs_cmb = freqs_vcc_main_core_limit;
			vm_millivolts_tmp = volt_vcc_main_core_svc[profile];
		} else if (pwr_pin == VCC_MAIN_GC) {
			freqs_cmb = freqs_vcc_main_gc_limit;
			vm_millivolts_tmp = volt_vcc_main_gc_svc[profile];
		}

		for (i = 0; i < op_size; i++) {
			for (j = 0; j < VL_MAX; j++) {
				if (op_array[i].pclk <= freqs_cmb[CORE][j])
					break;
			}
			max_require = vm_millivolts_tmp[j];

			for (j = 0; j < VL_MAX; j++) {
				if (op_array[i].dclk <= freqs_cmb[DDR][j])
					break;
			}
			max_require = max(max_require, vm_millivolts_tmp[j]);
			for (j = 0; j < VL_MAX; j++) {
				if (op_array[i].aclk <= freqs_cmb[AXI][j])
					break;
			}
			max_require = max(max_require, vm_millivolts_tmp[j]);
			for (j = 0; j < VL_MAX; j++) {
				if (op_array[i].gclk <= freqs_cmb[GC][j])
					break;
			}
			max_require = max(max_require, vm_millivolts_tmp[j]);
			if (pwr_pin == VCC_MAIN)
				op_array[i].v_vcc_main = max_require;
			else if (pwr_pin == VCC_MAIN_CORE)
				op_array[i].v_vcc_main_core = max_require;
			else if (pwr_pin == VCC_MAIN_GC)
				op_array[i].v_vcc_main_gc = max_require;
		}
	}
}
void show_op(void)
{
	struct pxa1928_core_opt cop;
	struct pxa1928_ddr_opt dop;
	unsigned int i;
	struct pxa1928_group_opt *op_array =
		cur_platform_opt->group_opt;
	unsigned int op_size = cur_platform_opt->group_opt_size;

	printf("Current OP:\n");
	memcpy(&cop, cur_core_op, sizeof(struct pxa1928_core_opt));

	printf("pll = %d; pclk = %d; baclk = %d; periphclk = %d\n",
		cop.core_clk_src_rate, cop.pclk, cop.baclk, cop.periphclk);

	memcpy(&dop, cur_ddr_op, sizeof(struct pxa1928_ddr_opt));
	printf("dclk(src:sel)\n");
	printf("ddr_clk_src_rate = %d; dclk = %d\n", dop.ddr_clk_src_rate, dop.dclk);

	printf("\nAll supported OP:\n");
	printf("OP\t Core(MHZ)\t DDR(MHZ)\t AXI(MHZ)\t Vcore\n");
	for (i = 0; i < op_size; i++) {
		if (cur_platform_opt->max_pclk_rate &&
			(op_array[i].pclk > cur_platform_opt->max_pclk_rate))
			continue;
		printf("%d\t %d\t %d\t %d\t %d\t\n", i, op_array[i].pclk,
					op_array[i].dclk, op_array[i].aclk,
					op_array[i].v_vcc_main);
	}
}

int do_op(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	ulong num;

	if (argc == 2) {
		num = simple_strtoul(argv[1], NULL, 0);
		if (setop(num) < 0)
			return -1;
	}
	if (argc <= 2)
		show_op();
	return 0;
}

U_BOOT_CMD(
	op,	2,	1,	do_op,
	"change operating point",
	"[ op number ]"
);
#endif

static void __init_rtc_wtc(void)
{
#ifndef CONFIG_PXA1928ZX
#else
	u32 reg_val;
	/* init RTC/WTC settings for clocks */
	reg_val = __raw_readl(PMUA_COREAPSS_CFG);
	reg_val &= ~(0xFFFFFF00);
	reg_val |= (0x60A0A000);
	reg_val = __raw_writel(reg_val, PMUA_COREAPSS_CFG);
	reg_val = __raw_readl(PMUA_APPS_MEM_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_APPS_MEM_CTRL);
	reg_val = __raw_readl(PMUA_MC_MEM_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_MC_MEM_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_SP_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_SP_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_USB_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_USB_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_ISP_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_ISP_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_AU_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_AU_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_LCD_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_LCD_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_VPU_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_VPU_CTRL);
	reg_val = __raw_readl(PMUA_ISLD_GC_CTRL);
	reg_val &= ~(0xFFFF0000);
	__raw_writel(reg_val, PMUA_ISLD_GC_CTRL);
	return;
#endif
}


/*
 * This function is called from board/CPU init level to initialize
 * the OP table and frequency change related setting
 */
void pxa1928_fc_init(int ddrtype)
{
#ifdef	BIND_OP_SUPPORT
	int iprofile = get_profile();
#endif

	__init_rtc_wtc();
	__init_platform_opt(ddrtype);
	__init_core_opt();
	__init_ddr_opt();
	__init_axi_opt();

#ifdef	BIND_OP_SUPPORT
	__init_group_opt(iprofile);
#endif
#ifndef CONFIG_PXA1928_FPGA
	__init_dvc_vol();
#endif

#ifdef CONFIG_DDR_REGTABLE
	if (ddrtype == LPDDR3_600M)
		__init_ddr_regtable(ARRAY_SIZE(lpddr3_samsung1g),
				ARRAY_SIZE(lpddr3_2x156mhz), ddrtype);
	else if (ddrtype == LPDDR2_533M)
		__init_ddr_regtable(ARRAY_SIZE(lpddr2_regtbl_array),
				ARRAY_SIZE(mt42l256m32d2lg18_2x156mhz), ddrtype);
	else if (ddrtype == LPDDR3_HYN2G)
		__init_ddr_regtable(ARRAY_SIZE(lpddr3_hynix2g_regtbl),
				ARRAY_SIZE(lpddr3_hynix2g_2x156mhz), ddrtype);
	else if (ddrtype == LPDDR3_ELP2G)
		__init_ddr_regtable(ARRAY_SIZE(lpddr3_elpida2g_regtbl),
				ARRAY_SIZE(lpddr3_elpida2g_2x156mhz), ddrtype);
	else if (ddrtype == LPDDR3_HYN2G_DIS)
		__init_ddr_regtable(ARRAY_SIZE(lpddr3_hynix2g_dis_regtbl),
				ARRAY_SIZE(lpddr3_hynix2g_dis_2x156mhz), ddrtype);
	else
		printf("ddr type is wrong !!!!!\n");

#else
	printf("ddr regtable is initialized in blf file\n");
#endif

	__init_fc_setting();
#ifdef CONFIG_DDR_HW_DFC
	ddr_hw_dfc_init();
#endif

	cur_core_op = &pxa1928_core_bootop;
	cur_ddr_op = &pxa1928_ddr_bootop;
	cur_axi_op = &pxa1928_axi_bootop;

}


#define PM822_SLAVE_ADDR		0x30

#define VOL_BASE_VCC_MAIN		600000
#define VOL_STEP_VCC_MAIN		12500
#define VOL_HIGH_VCC_MAIN		1400000
#define VOL_BASE_VCC_MAIN_CORE		600000
#define VOL_STEP_VCC_MAIN_CORE		12500
#define VOL_HIGH_VCC_MAIN_CORE		1400000
#define VOL_BASE_VCC_MAIN_GC		600000
#define VOL_STEP_VCC_MAIN_GC		12500
#define VOL_HIGH_VCC_MAIN_GC		1400000

#define PM822_DVC0			0
#define PM822_DVC1			1

#define PM822_BUCK1                     (0x3C)
#define PM822_BUCK2                     (0x40)
#define PM822_BUCK3                     (0x41)
#define PM822_BUCK4                     (0x45)
#define PM822_BUCK5                     (0x46)

#define VOL_BASE		600000
#define VOL_STEP		12500
#define VOL_HIGH		1400000
static int set_volt(enum pxa1928_power_pin power_pin, u32 dvc_level, u32 vol)
{
	int ret = 0;
	struct pmic *p_power;
	struct pmic_chip_desc *board_pmic_chip;
	u32 val;
	u32 adr = 0;
	int hex = 0;

	vol *= 1000;
	if ((vol < VOL_BASE) || (vol > VOL_HIGH)) {
		printf("out of range when set voltage!\n");
		return -1;
	}

	board_pmic_chip = get_marvell_pmic();
	if (!board_pmic_chip)
		return -1;

	p_power = pmic_get(board_pmic_chip->power_name);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;
	if (dvc_level == DVC_VL_SLEEP) {
		adr = 0x30;

		if (vol <= 1600000)
			hex = (vol - 600000) / 12500;
		else
			hex = 0x50 + (vol - 1600000) / 50000;

		if (hex < 0)
			return -1;

		ret = pmic_reg_read(p_power, adr, &val);
		if (ret)
			return ret;

		val &= ~MARVELL88PM_BUCK_VOLT_MASK;
		val |= hex;
		ret = pmic_reg_write(p_power, adr, val);
		if (ret)
			return ret;

		/* Enable buck sleep mode */
		ret = pmic_reg_write(p_power, 0x5a, 0xaa);
		if (ret)
			return ret;
		ret = pmic_reg_write(p_power, 0x5b, 0x2);
		return ret;

	} else {
		val = dvc_level / 4;
		pmic_reg_write(p_power, 0x4f, val);

		adr = 0x3c + dvc_level % 4;

		if (vol <= 1600000)
			hex = (vol - 600000) / 12500;
		else
			hex = 0x50 + (vol - 1600000) / 50000;

		if (hex < 0)
			return -1;

		ret = pmic_reg_read(p_power, adr, &val);
		if (ret)
			return ret;

		val &= ~MARVELL88PM_BUCK_VOLT_MASK;
		val |= hex;
		ret = pmic_reg_write(p_power, adr, val);

		return ret;
	}
}
static u32 get_volt(enum pxa1928_power_pin power_pin, u32 dvc_level)
{
	return 0;
}


#ifdef	BIND_OP_SUPPORT
static int set_group_volt(u32 *vol_vcc_main, u32 *vol_vcc_core, u32 *vol_vcc_gc, u32 dvc_level)
{
	int ret = 0;
	ret  = set_volt(VCC_MAIN, dvc_level,  *vol_vcc_main);
	if (ret) {
		printf("Increase vcc_main(dvc = %d) volatge %dmV failed!\n",
				dvc_level, *vol_vcc_main);
		return -1;
	}
	set_volt(VCC_MAIN_CORE, dvc_level, *vol_vcc_core);
	if (ret) {
		printf("Increase vcc_main_core(dvc = %d) volatge %dmV failed!\n",
				dvc_level, *vol_vcc_core);
		return -1;
	}
	set_volt(VCC_MAIN_GC, dvc_level, *vol_vcc_gc);
	if (ret) {
		printf("Increase vcc_main_gc(dvc = %d) volatge %dmV failed!\n",
				dvc_level, *vol_vcc_gc);
		return -1;
	}
	return 0;
}
#endif

int do_setvolt(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong vol;
	unsigned int cur_volt;
	enum pxa1928_power_pin power_pin = VCC_MAIN;
	unsigned int dvc_level = 3;
	int res = -1;
	if ((argc < 1) || (argc > 4))
		return -1;

	if (argc == 1) {
		printf("usage to change vcc_main: setvolt vcc_main dvcxx  xxxx(unit mV)\n"
			"for pxa1928, xxxx can be 600..1350, step 13 or 25\n"
			"usage to change vcc_main_core: setvolt vcc_main_core dvcxx xxxx(unit mV)\n"
			"for pxa1928, xxxx can be 600..1350, step 13 or 25\n"
			"usage to change vcc_main_gc: setvolt vcc_main_core dvcxx xxxx(unit mV)\n"
			"for pxa1928, xxxx can be 600..1350, step 13 or 25\n"

			);
		return 0;
	}
	if (argc == 3) {
		if (strcmp("vcc_main", argv[1]) == 0 ||
				strcmp("VCC_MAIN", argv[1]) == 0) {
			power_pin = VCC_MAIN;
		} else if (strcmp("vcc_main_core", argv[1]) == 0 ||
				strcmp("VCC_MAIN_CORE", argv[1]) == 0) {
			power_pin = VCC_MAIN_CORE;
		} else if (strcmp("vcc_main_gc", argv[1]) == 0 ||
				strcmp("VCC_MAIN_GC", argv[1]) == 0) {
			power_pin = VCC_MAIN_GC;
		}

		if (strcmp("dvc0000", argv[2]) == 0 ||
				strcmp("DVC0000", argv[2]) == 0) {
			dvc_level = DVC_VL_0000;
		} else if (strcmp("dvc0001", argv[2]) == 0 ||
				strcmp("DVC0001", argv[2]) == 0) {
			dvc_level = DVC_VL_0001;
		} else if (strcmp("dvc0010", argv[2]) == 0 ||
				strcmp("DVC0010", argv[2]) == 0) {
			dvc_level = DVC_VL_0010;
		} else if (strcmp("dvc0011", argv[2]) == 0 ||
				strcmp("DVC0011", argv[2]) == 0) {
			dvc_level = DVC_VL_0011;
		} else if (strcmp("dvc0100", argv[2]) == 0 ||
				strcmp("DVC0100", argv[2]) == 0) {
			dvc_level = DVC_VL_0100;
		} else if (strcmp("dvc0101", argv[2]) == 0 ||
				strcmp("DV0101", argv[2]) == 0) {
			dvc_level = DVC_VL_0101;
		} else if (strcmp("dvc0110", argv[2]) == 0 ||
				strcmp("DVC0110", argv[2]) == 0) {
			dvc_level = DVC_VL_0110;
		} else if (strcmp("dvc0111", argv[2]) == 0 ||
				strcmp("DVC0111", argv[2]) == 0) {
			dvc_level = DVC_VL_0111;
		} else if (strcmp("dvc1000", argv[2]) == 0 ||
				strcmp("DVC1000", argv[2]) == 0) {
			dvc_level = DVC_VL_1000;
		} else if (strcmp("dvc1001", argv[2]) == 0 ||
				strcmp("DV1001", argv[2]) == 0) {
			dvc_level = DVC_VL_1001;
		} else if (strcmp("dvc1010", argv[2]) == 0 ||
				strcmp("DVC1010", argv[2]) == 0) {
			dvc_level = DVC_VL_1010;
		} else if (strcmp("dvc1011", argv[2]) == 0 ||
				strcmp("DVC1011", argv[2]) == 0) {
			dvc_level = DVC_VL_1011;
		} else if (strcmp("dvc1100", argv[2]) == 0 ||
				strcmp("DVC1100", argv[2]) == 0) {
			dvc_level = DVC_VL_1100;
		} else if (strcmp("dvc1101", argv[2]) == 0 ||
				strcmp("DV1101", argv[2]) == 0) {
			dvc_level = DVC_VL_1101;
		} else if (strcmp("dvc1110", argv[2]) == 0 ||
				strcmp("DVC1110", argv[2]) == 0) {
			dvc_level = DVC_VL_1110;
		} else if (strcmp("dvc1111", argv[2]) == 0 ||
				strcmp("DVC1111", argv[2]) == 0) {
			dvc_level = DVC_VL_1111;
		}

		cur_volt = get_volt(power_pin, dvc_level);
		printf("voltage of %s:%s = %d mv\n", argv[1], argv[2], cur_volt);
	}

	if (argc == 4) {
		if (strcmp("vcc_main", argv[1]) == 0 ||
				strcmp("VCC_MAIN", argv[1]) == 0) {
			power_pin = VCC_MAIN;
		} else if (strcmp("vcc_main_core", argv[1]) == 0 ||
				strcmp("VCC_MAIN_CORE", argv[1]) == 0) {
			power_pin = VCC_MAIN_CORE;
		} else if (strcmp("vcc_main_gc", argv[1]) == 0 ||
				strcmp("VCC_MAIN_GC", argv[1]) == 0) {
			power_pin = VCC_MAIN_GC;
		}

		if (strcmp("dvc0000", argv[2]) == 0 ||
				strcmp("DVC0000", argv[2]) == 0) {
			dvc_level = DVC_VL_0000;
		} else if (strcmp("dvc0001", argv[2]) == 0 ||
				strcmp("DVC0001", argv[2]) == 0) {
			dvc_level = DVC_VL_0001;
		} else if (strcmp("dvc0010", argv[2]) == 0 ||
				strcmp("DVC0010", argv[2]) == 0) {
			dvc_level = DVC_VL_0010;
		} else if (strcmp("dvc0011", argv[2]) == 0 ||
				strcmp("DVC0011", argv[2]) == 0) {
			dvc_level = DVC_VL_0011;
		} else if (strcmp("dvc0100", argv[2]) == 0 ||
				strcmp("DVC0100", argv[2]) == 0) {
			dvc_level = DVC_VL_0100;
		} else if (strcmp("dvc0101", argv[2]) == 0 ||
				strcmp("DV0101", argv[2]) == 0) {
			dvc_level = DVC_VL_0101;
		} else if (strcmp("dvc0110", argv[2]) == 0 ||
				strcmp("DVC0110", argv[2]) == 0) {
			dvc_level = DVC_VL_0110;
		} else if (strcmp("dvc0111", argv[2]) == 0 ||
				strcmp("DVC0111", argv[2]) == 0) {
			dvc_level = DVC_VL_0111;
		} else if (strcmp("dvc1000", argv[2]) == 0 ||
				strcmp("DVC1000", argv[2]) == 0) {
			dvc_level = DVC_VL_1000;
		} else if (strcmp("dvc1001", argv[2]) == 0 ||
				strcmp("DV1001", argv[2]) == 0) {
			dvc_level = DVC_VL_1001;
		} else if (strcmp("dvc1010", argv[2]) == 0 ||
				strcmp("DVC1010", argv[2]) == 0) {
			dvc_level = DVC_VL_1010;
		} else if (strcmp("dvc1011", argv[2]) == 0 ||
				strcmp("DVC1011", argv[2]) == 0) {
			dvc_level = DVC_VL_1011;
		} else if (strcmp("dvc1100", argv[2]) == 0 ||
				strcmp("DVC1100", argv[2]) == 0) {
			dvc_level = DVC_VL_1100;
		} else if (strcmp("dvc1101", argv[2]) == 0 ||
				strcmp("DV1101", argv[2]) == 0) {
			dvc_level = DVC_VL_1101;
		} else if (strcmp("dvc1110", argv[2]) == 0 ||
				strcmp("DVC1110", argv[2]) == 0) {
			dvc_level = DVC_VL_1110;
		} else if (strcmp("dvc1111", argv[2]) == 0 ||
				strcmp("DVC1111", argv[2]) == 0) {
			dvc_level = DVC_VL_1111;
		} else if (strcmp("sleep", argv[2]) == 0 ||
				strcmp("SLEEP", argv[2]) == 0) {
			dvc_level = DVC_VL_SLEEP;
		}

		res = strict_strtoul(argv[3], 0, &vol);
		if (res == 0 && set_volt(power_pin, dvc_level, vol) == 0)
			printf("Voltage set(%s:%s) was successful\n",
					argv[1], argv[2]);
		else
			printf("Voltage set(%s:%s) was unsuccessful\n",
					argv[1], argv[2]);
	}
	return 0;
}
U_BOOT_CMD(
	setvolt, 6, 1, do_setvolt,
	"Setting voltages",
	""
);
int do_setcorerate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct pxa1928_core_opt *core_opt;
	unsigned int core_opt_size = 0, i;
	ulong freq;
	int res = -1;

	core_opt = cur_platform_opt->core_opt;
	core_opt_size = cur_platform_opt->core_opt_size;

	if (argc != 2) {
		printf("usage: set core frequency xxxx(unit Mhz)\n");
		printf("Supported Core frequency: ");
		for (i = 0; i < core_opt_size; i++)
			printf("%d\t", core_opt[i].pclk);
		printf("\n");
		return -1;
	}
	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (pxa1928_core_setrate(freq) == 0))
		printf("Core freq change was successful\n");
	else
		printf("Core freq change was unsuccessful\n");
	return 0;
}

U_BOOT_CMD(
	setcpurate, 6, 1, do_setcorerate,
	"Setting cpu rate",
	""
);

int do_setddrrate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct pxa1928_ddr_opt *ddr_opt;
	unsigned int ddr_opt_size = 0, i;
	ulong freq;
	int res = -1;

	ddr_opt = cur_platform_opt->ddr_opt;
	ddr_opt_size = cur_platform_opt->ddr_opt_size;

	if ((argc != 3) && (argc != 2)) {
		printf("usage: setddrrate dfc_type(hwdfc or swdfc)  freq(unit Mhz)\n");
		printf("Supported ddr frequency: ");
		for (i = 0; i < ddr_opt_size; i++)
			printf("%d\t", ddr_opt[i].dclk);
		printf("\n");
		return -1;
	}

	if (argc == 3) {
		res = strict_strtoul(argv[2], 0, &freq);
		if (strcmp("hwdfc", argv[1]) == 0 ||
				strcmp("HWDFC", argv[1]) == 0) {
			if ((res == 0) && (pxa1928_ddr_setrate(DDR_HW_DFC, freq) == 0))
				printf("DDR freq change was successful\n");
			else
				printf("DDR freq change fail\n");


		} else if (strcmp("swdfc", argv[1]) == 0 ||
				strcmp("SWDFC", argv[1]) == 0) {
			if ((res == 0) && (pxa1928_ddr_setrate(DDR_SW_DFC, freq) == 0))
				printf("DDR freq change was successful\n");
			else
				printf("DDR freq change fail\n");
		} else {
			printf("usage: setddrrate dfc_type(hwdfc or swdfc)  freq(unit Mhz)\n");
			return 0;
		}
	}
	if (argc == 2) {
		res = strict_strtoul(argv[1], 0, &freq);
		if ((res == 0) && (pxa1928_ddr_setrate(DDR_HW_DFC, freq) == 0))
			printf("DDR freq change was successful\n");
		else
			printf("DDR freq change fail\n");
	}
	return 0;
}

U_BOOT_CMD(
	setddrrate, 6, 1, do_setddrrate,
	"Setting ddr rate",
	""
);

int do_setaxirate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct pxa1928_axi_opt *axi_opt;
	unsigned int axi_opt_size = 0, i;
	ulong freq;
	int res = -1;

	axi_opt = cur_platform_opt->axi_opt;
	axi_opt_size = cur_platform_opt->axi_opt_size;

	if (argc != 2) {
		printf("usage: set axi frequency xxxx(unit Mhz)\n");
		printf("Supported axi frequency: ");
		for (i = 0; i < axi_opt_size; i++)
			printf("%d\t", axi_opt[i].aclk2);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (pxa1928_axi_setrate(freq) == 0))
		printf("axi freq change was successful\n");
	else
		printf("axi freq change was successful\n");

	return 0;
}

U_BOOT_CMD(
	setaxirate, 6, 1, do_setaxirate,
	"Setting axi rate",
	""
);

int do_setdvclevel(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enum dvc_id dvc_id = DVC_ID_D0_VL;
	unsigned int dvc_level;
	ulong temp;
	int res = -1;

	if (argc != 3) {
		printf("usage: setdvclevel dvc_id dev_level\n");
		return -1;
	}

	if (strcmp("mp0_vl", argv[1]) == 0 ||
			strcmp("MP0_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_MP0_VL;
	} else if (strcmp("mp1_vl", argv[1]) == 0 ||
			strcmp("MP1_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_MP1_VL;
	} else if (strcmp("mp2l2on_vl", argv[1]) == 0 ||
			strcmp("MP2L2ON_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_MP2L2ON_VL;
	} else if (strcmp("mp2l2off_vl", argv[1]) == 0 ||
			strcmp("MP2L2OFF_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_MP2L2OFF_VL;
	} else if (strcmp("d0_vl", argv[1]) == 0 ||
			strcmp("D0_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_D0_VL;
	} else if (strcmp("d0cg_vl", argv[1]) == 0 ||
			strcmp("D0CG_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_D0CG_VL;
	} else if (strcmp("d1_vl", argv[1]) == 0 ||
			strcmp("D1_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_D1_VL;
	} else if (strcmp("d2_vl", argv[1]) == 0 ||
			strcmp("D2_VL", argv[1]) == 0) {
		dvc_id = DVC_ID_D2_VL;
	}

	res = strict_strtoul(argv[2], 0, &temp);
	dvc_level = temp;
	if (res == 0 && set_dvc_level(dvc_id, dvc_level) == 0)
		printf("Set DVC level (%s:%s) was successful\n",
				argv[1], argv[2]);
	else
		printf("Set DVC level (%s:%s) was unsuccessful\n",
				argv[1], argv[2]);

	return 0;
}

U_BOOT_CMD(
	setdvclevel, 6, 1, do_setdvclevel,
	"Setting dvc level",
	""
);

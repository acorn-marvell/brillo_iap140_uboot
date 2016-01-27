/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by:
 * Zhoujie Wu <zjwu@marvell.com>
 * Qiming Wu <wuqm@marvell.com>
 * Lu Cao <lucao@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <errno.h>
#include <common.h>
#include <command.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <linux/list.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include "helanx_pll.h"
#include "helanx_hwdvc.h"
#include <mvmfp.h>
#include <asm/arch/mfp.h>

DECLARE_GLOBAL_DATA_PTR;

#define	APMU_BASE		0xD4282800
#define APMU_REG(x)		(APMU_BASE + x)
#define	MPMU_BASE		0xD4050000
#define MPMU_REG(x)		(MPMU_BASE + x)
#define APB_SPARE_BASE		0xD4090000
#define APB_SPARE_REG(x)	(APB_SPARE_BASE + x)
#define CIU_BASE		0xD4282C00
#define CIU_REG(x)		(CIU_BASE + x)
#define GEU_BASE		0xD4292800
#define GEU_REG(x)		(GEU_BASE + x)


/* FC */
#define APMU_CP_CCR		APMU_REG(0x0000)
#define APMU_CCR		APMU_REG(0x0004)
#define APMU_CCSR		APMU_REG(0x000c)

#define APMU_IMR		APMU_REG(0x0098)
#define APMU_ISR		APMU_REG(0x00a0)
#define APMU_PLL_SEL_STATUS	APMU_REG(0x00c4)
#define APMU_MC_HW_SLP_TYPE	APMU_REG(0x00b0)

#define APMU_CC2R		APMU_REG(0x0100)
#define APMU_CC2SR		APMU_REG(0x0104)

#define MPMU_FCCR		MPMU_REG(0x0008)
#define MPMU_FCAP		MPMU_REG(0x0054)
#define MPMU_FCCP		MPMU_REG(0x0058)
#define MPMU_FCDCLK		MPMU_REG(0x005c)
#define MPMU_FCACLK		MPMU_REG(0x0060)

/* HW DFC */
#define DVC_DFC_DEBUG		APMU_REG(0x140)
#define DFC_AP			APMU_REG(0x180)
#define DFC_CP			APMU_REG(0x184)
#define DFC_STATUS		APMU_REG(0x188)
#define DFC_LEVEL(i)		APMU_REG(0x190 + ((i) << 2))

/* clock gating */
#define MC_CONF		CIU_REG(0x40)
#define APMU_MCK4_CTRL		APMU_REG(0x0e8)

/* core WTC/RTC */
#define CIU_CA9_CPU_CONF_SRAM_0		CIU_REG(0x00c8)
#define CIU_CA9_CPU_CONF_SRAM_1		CIU_REG(0x00cc)

/* axi WTC/RTC */
#define CIU_TOP_MEM_RTC_WTC_SPD		CIU_REG(0x44)

/* FUSE */
#define APMU_GEU		APMU_REG(0x068)
#define MANU_PARA_31_00		GEU_REG(0x110)
#define MANU_PARA_63_32		GEU_REG(0x114)
#define MANU_PARA_95_64		GEU_REG(0x118)
#define BLOCK4_PARA_31_00	GEU_REG(0x2B4)
#define BLOCK0_224_255		GEU_REG(0x11c)
#define UID_H_32		GEU_REG(0x18c)
#define UID_L_32		GEU_REG(0x1a8)

/* ULC has different base */
#define ULC_GEU_REG(x)		(0xd4201000 + x)
#define ULC_MANU_PARA_31_00		ULC_GEU_REG(0x410)
#define ULC_MANU_PARA_63_32		ULC_GEU_REG(0x414)
#define ULC_MANU_PARA_95_64		ULC_GEU_REG(0x418)
#define ULC_BLOCK0_224_255		ULC_GEU_REG(0x420)
#define ULC_UID_H_32		ULC_GEU_REG(0x48c)
#define ULC_UID_L_32		ULC_GEU_REG(0x4a8)

/*
 * for ana_grp PU_CLK contro. Must be enabled if
 * PLL2 or PLL3 or USB are used
 */
#define UTMI_CTRL		0xD4207104

#define AP_SRC_SEL_MASK		0x7
#define MPMU_FCAP_MASK		0x7
#define MPMU_FCDCLK_MASK	0x7
#define MPMU_FCACLK_MASK	0x3
#define UNDEF_OP		-1
#define MHZ_TO_KHZ		(1000)

#define PROFILE_NUM	11

union pmum_fccr {
	struct {
		unsigned int pll1fbd:9;
		unsigned int pll1refd:5;
		unsigned int pll1cen:1;
		unsigned int mfc:1;
		unsigned int reserved0:12;
		unsigned int i2sclksel:1;
		unsigned int reserved1:3;
	} b;
	unsigned int v;
};

union pmua_pllsel {
	struct {
		unsigned int cpclksel:2;
		unsigned int apclksel1:2;
		unsigned int ddrclksel1:2;
		unsigned int axiclksel:2;
		unsigned int apclksel2:1;
		unsigned int ddrclksel2:1;
		unsigned int reserved0:22;
	} b;
	unsigned int v;
};

union pmua_cc {
	struct {
		unsigned int core_clk_div:3;
		unsigned int reserved0:9;
		unsigned int ddr_clk_div:3;
		unsigned int bus_clk_div:3;
		unsigned int async1:1;
		unsigned int async2:1;
		unsigned int async3:1;
		unsigned int async3_1:1;
		unsigned int async4:1;
		unsigned int async5:1;
		unsigned int core_freq_chg_req:1;
		unsigned int ddr_freq_chg_req:1;
		unsigned int bus_freq_chg_req:1;
		unsigned int core_allow_spd_chg:1;
		unsigned int core_dyn_fc:1;
		unsigned int dclk_dyn_fc:1;
		unsigned int aclk_dyn_fc:1;
		unsigned int core_rd_st_clear:1;
	} b;
	unsigned int v;
};

union pmua_cc_ap {
	struct {
		unsigned int core_clk_div:3;
		unsigned int bus_mc_clk_div:3;
		unsigned int biu_clk_div:3;
		unsigned int l2_clk_div:3;
		unsigned int ddr_clk_div:3;
		unsigned int bus_clk_div:3;
		unsigned int async1:1;
		unsigned int async2:1;
		unsigned int async3:1;
		unsigned int async3_1:1;
		unsigned int async4:1;
		unsigned int async5:1;
		unsigned int core_freq_chg_req:1;
		unsigned int ddr_freq_chg_req:1;
		unsigned int bus_freq_chg_req:1;
		unsigned int core_allow_spd_chg:1;
		unsigned int core_dyn_fc:1;
		unsigned int dclk_dyn_fc:1;
		unsigned int aclk_dyn_fc:1;
		unsigned int core_rd_st_clear:1;
	} b;
	unsigned int v;
};

union pmua_cc2 {
	struct {
		unsigned int peri_clk_div:3;
		unsigned int peri_clk_dis:1;
		unsigned int reserved0:12;
		unsigned int cpu0_core_rst:1;
		unsigned int reserved1:1;
		unsigned int cpu0_dbg_rst:1;
		unsigned int cpu0_wdt_rst:1;
		unsigned int cpu1_core_rst:1;
		unsigned int reserved2:1;
		unsigned int cpu1_dbg_rst:1;
		unsigned int cpu1_wdt_rst:1;
		unsigned int reserved3:8;
	} b;
	unsigned int v;
};

union pmua_dm_cc {
	struct {
		unsigned int core_clk_div:3;
		unsigned int bus_mc_clk_div:3;
		unsigned int biu_clk_div:3;
		unsigned int l2_clk_div:3;
		unsigned int ddr_clk_div:3;
		unsigned int bus_clk_div:3;
		unsigned int async1:1;
		unsigned int async2:1;
		unsigned int async3:1;
		unsigned int async3_1:1;
		unsigned int async4:1;
		unsigned int async5:1;
		unsigned int cp_rd_status:1;
		unsigned int ap_rd_status:1;
		unsigned int cp_fc_done:1;
		unsigned int ap_fc_done:1;
		unsigned int dclk_fc_done:1;
		unsigned int aclk_fc_done:1;
		unsigned int reserved:2;
	} b;
	unsigned int v;
};

union pmua_dm_cc2 {
	struct {
		unsigned int peri_clk_div:3;
		unsigned int reserved:29;
	} b;
	unsigned int v;
};

/* HW DFC register defination */
union dfc_level_reg {
	struct {
		unsigned int dclksel:3;
		unsigned int clk_mode:1;
		unsigned int ddr_clk_div:3;
		unsigned int mc_table_num:5;
		unsigned int volt_level:3;
		unsigned int reserved:17;
	} b;
	unsigned int v;
};

union dfc_ap {
	struct {
		unsigned int dfc_req:1;
		unsigned int freq_level:3;
		unsigned int reserved:28;
	} b;
	unsigned int v;
};

union dfc_status {
	struct {
		unsigned int dfc_status:1;
		unsigned int cfl:3;
		unsigned int tfl:3;
		unsigned int dfc_cause:2;
		unsigned int reserved:23;
	} b;
	unsigned int v;
};

/*
 * AP clock source:
 * 0x0 = PLL1 624 MHz
 * 0x1 = PLL1 1248 MHz  or PLL3_CLKOUT
 * (depending on FCAP[2])
 * 0x2 = PLL2_CLKOUT
 * 0x3 = PLL1 832 MHZ
 * 0x5 = PLL3_CLKOUTP
 */
enum ap_clk_sel {
	AP_CLK_SRC_PLL1_624 = 0x0,
	AP_CLK_SRC_PLL1_1248 = 0x1,
	AP_CLK_SRC_PLL2 = 0x2,
	AP_CLK_SRC_PLL1_832 = 0x3,
	AP_CLK_SRC_PLL3P = 0x5,
};

/*
 * DDR clock source:
 * 0x0 = PLL1 624 MHz
 * 0x1 = PLL1 832 MHz
 * 0x4 = PLL2_CLKOUT
 * 0x5 = PLL4_CLKOUT
 * 0x6 = PLL3_CLKOUTP
 */
enum ddr_clk_sel {
	DDR_CLK_SRC_PLL1_624 = 0x0,
	DDR_CLK_SRC_PLL1_832 = 0x1,
	DDR_CLK_SRC_PLL2 = 0x4,
	DDR_CLK_SRC_PLL4 = 0x5,
	DDR_CLK_SRC_PLL1_1248 = 0x6,
	DDR_CLK_SRC_PLL3P = 0x7,
};

/*
 * AXI clock source:
 * 0x0 = PLL1 416 MHz
 * 0x1 = PLL1 624 MHz
 * 0x2 = PLL2_CLKOUT
 * 0x3 = PLL2_CLKOUTP
 */
enum axi_clk_sel {
	AXI_CLK_SRC_PLL1_416 = 0x0,
	AXI_CLK_SRC_PLL1_624 = 0x1,
	AXI_CLK_SRC_PLL2 = 0x2,
	AXI_CLK_SRC_PLL2P = 0x3,
};

enum ddr_type {
	DDR_400M = 0,
	DDR_533M,
	DDR_667M,
	DDR_800M,
	DDR_TYPE_MAX,
};

/* RTC/WTC table used for solution change rtc/wtc on the fly */
struct cpu_rtcwtc {
	/* max rate could be used by this rtc/wtc */
	unsigned int max_pclk;
	unsigned int l1_rtc;
	unsigned int l2_rtc;
};

struct cpu_opt {
	unsigned int pclk;		/* core clock */
	unsigned int l2clk;		/* L2 cache interface clock */
	unsigned int pdclk;		/* DDR interface clock */
	unsigned int baclk;		/* bus interface clock */
	unsigned int periphclk;		/* PERIPHCLK */
	enum ap_clk_sel ap_clk_sel;	/* core src sel val */
	unsigned int ap_clk_src;	/* core src rate */
	unsigned int pclk_div;		/* core clk divider*/
	unsigned int l2clk_div;		/* L2 clock divider */
	unsigned int pdclk_div;		/* DDR interface clock divider */
	unsigned int baclk_div;		/* bus interface clock divider */
	unsigned int periphclk_div;	/* PERIPHCLK divider */
	unsigned int l1_rtc;		/* L1 cache RTC/WTC */
	unsigned int l2_rtc;		/* L2 cache RTC/WTC */
	struct list_head node;

	/* use to record voltage requirement(mV) */
	unsigned int volts;
};

struct ddr_opt {
	unsigned int dclk;		/* ddr clock */
	unsigned int ddr_tbl_index;	/* ddr FC table index */
	unsigned int ddr_freq_level;	/* ddr freq level(0~7) */
	enum ddr_clk_sel ddr_clk_sel;/* ddr src sel val */
	unsigned int ddr_clk_src;	/* ddr src rate */
	unsigned int dclk_div;		/* ddr clk divider */

	/* use to record voltage requirement(mV) */
	unsigned int volts;
};

/* RTC/WTC table used for AXI */
struct axi_rtcwtc {
	unsigned int max_aclk;	/* max rate could be used by this rtc/wtc */
	unsigned int xtc_val;
};

struct axi_opt {
	unsigned int aclk;		/* axi clock */
	enum axi_clk_sel axi_clk_sel;/* axi src sel val */
	unsigned int axi_clk_src;	/* axi src rate */
	unsigned int aclk_div;		/* axi clk divider */
	unsigned int xtc_val;		/* RTC/WTC value */

	/* use to record voltage requirement(mV) */
	unsigned int volts;
};

/* comp that voting for vcore in uboot */
enum vcore_comps {
	CORE = 0,
	DDR,
	AXI,
	VM_RAIL_MAX,
};

/* vcore levels */
enum vcore_lvls {
	VL0 = 0,
	VL1,
	VL2,
	VL3,
	VL_MAX,
};

/*
 * used to show the bind relationship of cpu,ddr,axi
 * only used when core,ddr,axi FC bind together
 */
struct operating_point {
	u32 pclk;
	u32 dclk;
	u32 aclk;
	u32 vcore;
};

struct platform_opt {
	unsigned int cpuid;
	unsigned int chipid;
	char *cpu_name;
	enum ddr_type ddrtype;
	struct operating_point *op_array;
	unsigned int com_op_size;
	struct cpu_opt *cpu_opt;
	unsigned int cpu_opt_size;
	struct ddr_opt *ddr_opt;
	unsigned int ddr_opt_size;
	struct axi_opt *axi_opt;
	unsigned int axi_opt_size;

	/* default pll freq used on this platform */
	struct pll_cfg pll_cfg[PLL_MAX];

	/* the default max cpu rate could be supported */
	unsigned int df_max_cpurate;
	unsigned int bridge_cpurate;
	/* the plat rule for filter core ops */
	unsigned int (*is_cpuop_invalid_plt)(struct cpu_opt *cop);

	/* dvfs related, voltage table and freq_cmb, filled dynamicly */
	int *vm_millivolts;	/*size VL_MAX */
	unsigned int (*freqs_cmb)[VL_MAX];	/* size VM_RAIL_MAX */
};

#define debug_wt_reg(val, reg)			\
do {						\
	/*printf(" %08x ==> [%x]\n", val, reg);*/	\
	__raw_writel(val, reg);			\
} while (0)


/* current platform OP struct */
static struct platform_opt *cur_platform_opt;

static LIST_HEAD(core_op_list);

/* current core OP */
static struct cpu_opt *cur_cpu_op;
static struct cpu_opt *bridge;

/* current DDR/AXI OP */
static struct ddr_opt *cur_ddr_op;
static struct axi_opt *cur_axi_op;

static unsigned int cpu_sel2_srcrate(enum ap_clk_sel ap_sel);
static unsigned int ddr_sel2_srcrate(enum ddr_clk_sel ddr_sel);
static unsigned int axi_sel2_srcrate(enum axi_clk_sel axi_sel);

static unsigned int cal_comp_volts_req(enum vcore_comps comp,
	unsigned int rate);


static int set_volt(u32 vol);
static u32 get_volt(void);

static int hwdfc_enable; /* enable HW-DFC */

/* fuse related */
static unsigned int __maybe_unused uidro;
static unsigned int __maybe_unused uiprofile;
static unsigned int __maybe_unused uisvtdro;


/*
 * used to record dummy boot up PP, define it here
 * as the default src and div value read from register
 * is NOT the real core and ddr,axi rate.
 * This default boot OP is from PP table.
 */
static struct cpu_opt helan2_cpu_bootop = {
	.pclk = 312,
	.pdclk = 156,
	.baclk = 78,
	.ap_clk_src = 624,
};

static struct ddr_opt helan2_ddr_bootop = {
	.dclk = 156,
	.ddr_clk_src = 624,
};

static struct axi_opt helan2_axi_bootop = {
	.aclk = 104,
	.axi_clk_src = 416,
};

/*
 * For HELAN2:
 * PCLK = AP_CLK_SRC / (CORE_CLK_DIV + 1)
 * BIU_CLK = PCLK / (BIU_CLK_DIV + 1)
 * MC_CLK = PCLK / (MC_CLK_DIV + 1)
 */
static struct cpu_opt helan2_op_array[] = {
	{
		.pclk = 624,
		.pdclk = 312,
		.baclk = 156,
		.ap_clk_sel = AP_CLK_SRC_PLL1_624,
	},
	{
		.pclk = 832,
		.pdclk = 416,
		.baclk = 208,
		.ap_clk_sel = AP_CLK_SRC_PLL1_832,
	},
	{
		.pclk = 1057,
		.pdclk = 528,
		.baclk = 264,
		.ap_clk_sel = AP_CLK_SRC_PLL2,
	},
	{
		.pclk = 1248,
		.pdclk = 624,
		.baclk = 312,
		.ap_clk_sel = AP_CLK_SRC_PLL1_1248,
	},
	{
		.pclk = 1526,
		.pdclk = 763,
		.baclk = 381,
		.ap_clk_sel = AP_CLK_SRC_PLL3P,
		.ap_clk_src = 1526,
	},
	{
		.pclk = 1803,
		.pdclk = 901,
		.baclk = 450,
		.ap_clk_sel = AP_CLK_SRC_PLL3P,
		.ap_clk_src = 1803,
	},
};

/*
 * 1) Please don't select ddr from pll1 but axi from pll2
 * 2) FIXME: high ddr request means high axi is NOT
 * very reasonable
 */
static struct ddr_opt lpddr800_ddr_oparray[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 2,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 4,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 416,
		.ddr_tbl_index = 6,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 528,
		.ddr_tbl_index = 8,
		.ddr_clk_sel = DDR_CLK_SRC_PLL2,
	},
	{
		.dclk = 624,
		.ddr_tbl_index = 10,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_1248,
	},
	{
		.dclk = 797,
		.ddr_tbl_index = 12,
		.ddr_clk_sel = DDR_CLK_SRC_PLL4,
	},
};

static struct ddr_opt lpddr667_ddr_oparray[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 2,
		.ddr_freq_level = 0,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 4,
		.ddr_freq_level = 1,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 416,
		.ddr_tbl_index = 6,
		.ddr_freq_level = 2,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 528,
		.ddr_tbl_index = 8,
		.ddr_freq_level = 3,
		.ddr_clk_sel = DDR_CLK_SRC_PLL2,
	},
	{
		.dclk = 624,
		.ddr_tbl_index = 10,
		.ddr_freq_level = 4,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_1248,
	},
	{
		.dclk = 667,
		.ddr_tbl_index = 12,
		.ddr_freq_level = 5,
		.ddr_clk_sel = DDR_CLK_SRC_PLL4,
	},
};

static struct ddr_opt lpddr533_ddr_oparray[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 2,
		.ddr_freq_level = 0,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 4,
		.ddr_freq_level = 1,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 416,
		.ddr_tbl_index = 6,
		.ddr_freq_level = 2,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 528,
		.ddr_tbl_index = 8,
		.ddr_freq_level = 3,
		.ddr_clk_sel = DDR_CLK_SRC_PLL2,
	},
};

static struct ddr_opt lpddr400_ddr_oparray[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 2,
		.ddr_freq_level = 0,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 4,
		.ddr_freq_level = 1,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 398,
		.ddr_tbl_index = 6,
		.ddr_freq_level = 2,
		.ddr_clk_sel = DDR_CLK_SRC_PLL2,
	},
};

static struct axi_opt axi_oparray[] = {
	{
		.aclk = 156,
		.axi_clk_sel = AXI_CLK_SRC_PLL1_624,
	},
	{
		.aclk = 208,
		.axi_clk_sel = AXI_CLK_SRC_PLL1_416,
	},
};

/*
 * core and ddr opt combination vary from platform to platform
 * LPDDR400 pll2vco 1600M, pll2 800M,  pll2p 400M
 *	    pll3vco 2358M, pll3 1179M, pll3p 1179M
 * LPDDR533 pll2vco 2132M, pll2 1066M, pll2p 533M
 *	    pll3vco 2358M, pll3 1179M, pll3p 1179M
 * LPDDR667 pll2vco 2132M, pll2 1066M, pll2p 533M
 *	    pll3vco 2358M, pll3 1179M, pll3p 1179M
 *	    pll4vco 2668M, pll4 1334M, pll4p 667M
 * LPDDR800 pll2vco 2132M, pll2 1066M, pll2p 533M
 *	    pll3vco 2358M, pll3 1179M, pll3p 1179M
 *	    pll4vco 1600M, pll4 1600M, pll4p 800M
 */
static struct platform_opt platform_op_arrays[] = {
	{
		.cpu_name = "HELAN2",
		.cpu_opt = helan2_op_array,
		.cpu_opt_size = ARRAY_SIZE(helan2_op_array),
		.ddrtype = DDR_400M,
		.ddr_opt = lpddr400_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr400_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_cpurate = 1248,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {1595, 797, 797},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1526, 1526, 1526},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {1595, 1595, 797},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
	},
	{
		.cpu_name = "HELAN2",
		.cpu_opt = helan2_op_array,
		.cpu_opt_size = ARRAY_SIZE(helan2_op_array),
		.ddrtype = DDR_533M,
		.ddr_opt = lpddr533_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr533_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_cpurate = 1248,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {2115, 1057, 528},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1526, 1526, 1526},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {1595, 1595, 797},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
	},
	{
		.cpu_name = "HELAN2",
		.cpu_opt = helan2_op_array,
		.cpu_opt_size = ARRAY_SIZE(helan2_op_array),
		.ddrtype = DDR_667M,
		.ddr_opt = lpddr667_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr667_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_cpurate = 1248,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {2115, 1057, 528},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1526, 1526, 1526},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {2670, 1335, 667},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
	},
	{
		.cpu_name = "HELAN2",
		.cpu_opt = helan2_op_array,
		.cpu_opt_size = ARRAY_SIZE(helan2_op_array),
		.ddrtype = DDR_800M,
		.ddr_opt = lpddr800_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr800_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_cpurate = 1248,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {2115, 1057, 528},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1526, 1526, 1526},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {1595, 1595, 797},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
	},
};

/* FIXME: For bring up only */
static int vm_mv_helan2_svc[][VL_MAX] = {
	{1100, 1200, 1250, 1300},
};

/* FIXME: For bring up only */
static unsigned int freqs_cmb_helan2[VM_RAIL_MAX][VL_MAX] = {
	{ 312, 624, 1248, 1526 },   /* CORE */
	{ 156, 398, 528, 800 },     /* DDR */
	{ 208, 312, 312, 312 },     /* AXI */
};

/* FIXME: must remove the table after has fuse info !!! */
struct svtrng {
	unsigned int min;
	unsigned int max;
	unsigned int profile;
};

static struct svtrng svtrngtb_z0[] = {
	{0, 295, 15},
	{296, 308, 14},
	{309, 320, 13},
	{321, 332, 12},
	{333, 344, 11},
	{345, 357, 10},
	{358, 370, 9},
	{371, 382, 8},
	{383, 394, 7},
	{395, 407, 6},
	{408, 420, 5},
	{421, 432, 4},
	{433, 444, 3},
	{445, 0xffffffff, 2},
	/* NOTE: rsved profile 1, by default use profile 0 */
};

static unsigned int __maybe_unused convert_svtdro2profile(unsigned int uisvtdro)
{
	unsigned int uiProfile = 0, idx;

	for (idx = 0; idx < ARRAY_SIZE(svtrngtb_z0); idx++) {
		if (uisvtdro >= svtrngtb_z0[idx].min &&
		    uisvtdro <= svtrngtb_z0[idx].max) {
			uiProfile = svtrngtb_z0[idx].profile;
			break;
		}
	}

	return uiProfile;
}

static void __init_read_ultinfo(void)
{
	/*
	 * Read out DRO value, need enable GEU clock, if already disable,
	 * need enable it firstly
	 */
#ifndef CONFIG_TZ_HYPERVISOR
	unsigned int uimanupara_31_00 = 0;
	unsigned int uimanupara_63_32 = 0;
	unsigned int uimanupara_95_64 = 0;
	unsigned int uiblk4para_95_64 = 0;
	unsigned int uiallocrev = 0;
	unsigned int uifab = 0;
	unsigned int uirun = 0;
	unsigned int uiwafer = 0;
	unsigned int uix = 0;
	unsigned int uiy = 0;
	unsigned int uiparity = 0;
	unsigned int __maybe_unused uigeustatus = 0;
	unsigned int uifuses = 0;
	u64 uluid = 0;
	uigeustatus = __raw_readl(APMU_GEU);
	if (!(uigeustatus & 0x30)) {
		__raw_writel((uigeustatus | 0x30), APMU_GEU);
		udelay(10);
	}
	if (cpu_is_pxa1U88()) {
		uimanupara_31_00 = __raw_readl(MANU_PARA_31_00);
		uimanupara_63_32 = __raw_readl(MANU_PARA_63_32);
		uimanupara_95_64 = __raw_readl(MANU_PARA_95_64);
		uiblk4para_95_64 = __raw_readl(BLOCK4_PARA_31_00);
		uifuses = __raw_readl(BLOCK0_224_255);
		uluid = __raw_readl(UID_H_32);
		uluid = (uluid << 32) | __raw_readl(UID_L_32);
	} else {
		/* FIXME: how to read fuse info on ULC? */
	}

	__raw_writel(uigeustatus, APMU_GEU);

	printf("  0x%x   0x%x   0x%x 0x%x\n",
	       uimanupara_31_00, uimanupara_63_32,
	       uimanupara_95_64, uiblk4para_95_64);

	uiallocrev	= uimanupara_31_00 & 0x7;
	uifab		= (uimanupara_31_00 >>  3) & 0x1f;
	uirun		= ((uimanupara_63_32 & 0x3) << 24)
			| ((uimanupara_31_00 >> 8) & 0xffffff);
	uiwafer		= (uimanupara_63_32 >>  2) & 0x1f;
	uix		= (uimanupara_63_32 >>  7) & 0xff;
	uiy		= (uimanupara_63_32 >> 15) & 0xff;
	uiparity	= (uimanupara_63_32 >> 23) & 0x1;
	uidro		= (uimanupara_95_64 >>  4) & 0x3ff;
	uisvtdro	= uiblk4para_95_64 & 0x3ff;

	/* bit 240 ~ 255 for Profile information */
	uifuses = (uifuses >> 16) & 0x0000FFFF;
	if (cpu_is_pxa1U88())
		uiprofile = convert_svtdro2profile(uisvtdro);

	printf(" ");
	printf("	*********************************\n");
	printf("	*	ULT: %08X%08X	*\n",
	       uimanupara_63_32, uimanupara_31_00);
	printf("	*********************************\n");
	printf("	 ULT decoded below\n");
	printf("		alloc_rev[2:0]	= %d\n", uiallocrev);
	printf("		fab [ 7: 3]	= %d\n", uifab);
	printf("		run [33: 8]	= %d (0x%X)\n", uirun, uirun);
	printf("		wafer [38:34]	= %d\n", uiwafer);
	printf("		x [46:39]	= %d\n", uix);
	printf("		y [54:47]	= %d\n", uiy);
	printf("		parity [55:55]	= %d\n", uiparity);
	printf("	*********************************\n");
	printf("	*********************************\n");
	printf("		LVTDRO [77:68]	= %d\n", uidro);
	printf("		SVTDRO [9:0]	= %d\n", uisvtdro);
	printf("		Profile	= %d\n", uiprofile);
	printf("		UID	= %llx\n", uluid);
	printf("	*********************************\n");
	printf("\n");
#else
	/* FIXME: add TZ_HYPERVISOR support here */
#endif
}

/* turn on pll at defrate */
static void turn_on_pll_defrate(enum pll pll)
{
	struct pll_cfg pll_cfg;

	BUG_ON(!cur_platform_opt);

	pll_cfg = cur_platform_opt->pll_cfg[pll];

	turn_on_pll(pll, pll_cfg);
}

static void  __init_platform_opt(int ddr_mode)
{
	unsigned int i, chipid = 0;
	struct platform_opt *proc;

	/*
	 * ddr type is passed from OBM, FC code use this info
	 * to identify DDR OPs.
	 */
	if (ddr_mode >= DDR_TYPE_MAX)
		ddr_mode = DDR_533M;

	for (i = 0; i < ARRAY_SIZE(platform_op_arrays); i++) {
		proc = platform_op_arrays + i;
		if ((proc->chipid == chipid) && (proc->ddrtype == ddr_mode))
			break;
	}
	BUG_ON(i == ARRAY_SIZE(platform_op_arrays));
	cur_platform_opt = proc;
}

static struct cpu_rtcwtc cpu_rtcwtc_1908umc[] = {
	{.max_pclk = 312, .l1_rtc = 0x11111111, .l2_rtc = 0x00001111,},
	{.max_pclk = 832, .l1_rtc = 0x55555555, .l2_rtc = 0x00005555,},
	{.max_pclk = 1057, .l1_rtc = 0x55555555, .l2_rtc = 0x0000555A,},
	{.max_pclk = 1526, .l1_rtc = 0xAAAAAAAA, .l2_rtc = 0x0000AAAA,},
};

static void __init_cpu_rtcwtc(struct cpu_opt *cpu_opt)
{
	struct cpu_rtcwtc *cpu_rtcwtc;
	unsigned int size, index;

	cpu_rtcwtc = cpu_rtcwtc_1908umc;
	size = ARRAY_SIZE(cpu_rtcwtc_1908umc);

	for (index = 0; index < size; index++)
		if (cpu_opt->pclk <= cpu_rtcwtc[index].max_pclk)
			break;
	if (index == size)
		index = size - 1;

	cpu_opt->l1_rtc = cpu_rtcwtc[index].l1_rtc;
	cpu_opt->l2_rtc = cpu_rtcwtc[index].l2_rtc;

	if (cpu_opt->pclk == helan2_cpu_bootop.pclk) {
		__raw_writel(cpu_opt->l1_rtc, CIU_CA9_CPU_CONF_SRAM_0);
		__raw_writel(cpu_opt->l2_rtc, CIU_CA9_CPU_CONF_SRAM_1);
	}

};

/* Common condition here if you want to filter the core ops */
static unsigned int __is_cpu_op_invalid_com(struct cpu_opt *cop)
{
	unsigned int df_max_cpurate =
		cur_platform_opt->df_max_cpurate;

	/* If pclk could not drive from src, invalid it */
	if (cop->ap_clk_src % cop->pclk)
		return 1;

	/*
	 * If pclk > default support max core frequency, invalid it
	 */
	if (df_max_cpurate && (cop->pclk > df_max_cpurate))
		return 1;

	return 0;
};

/* plat extra condition here if you want to filter the core ops */
static unsigned int __is_cpu_op_invalid_plt(struct cpu_opt *cop)
{
	if (cur_platform_opt->is_cpuop_invalid_plt)
		return cur_platform_opt->is_cpuop_invalid_plt(cop);

	/* no rule no filter */
	return 0;
}

static void __init_cpu_opt(void)
{
	struct cpu_opt *cpu_opt, *cop;
	unsigned int cpu_opt_size = 0, i, idx = 0;

	__init_cpu_rtcwtc(cur_cpu_op);

	cpu_opt = cur_platform_opt->cpu_opt;
	cpu_opt_size = cur_platform_opt->cpu_opt_size;

	debug("pclk(src:sel,div) l2clk(src,div)\t");
	debug("pdclk(src,div)\tbaclk(src,div)\t");
	debug("periphclk(src,div)\n");

	for (i = 0; i < cpu_opt_size; i++) {
		cop = &cpu_opt[i];
		if (!cop->ap_clk_src)
			cop->ap_clk_src =
				cpu_sel2_srcrate(cop->ap_clk_sel);
		BUG_ON(cop->ap_clk_src < 0);
		/* check the invalid condition of this op */
		if (__is_cpu_op_invalid_com(cop))
			continue;
		if (__is_cpu_op_invalid_plt(cop))
			continue;
		/* add it into core op list */
		list_add_tail(&cop->node, &core_op_list);
		idx++;
		if (cop->pclk == cur_platform_opt->bridge_cpurate)
			bridge = cop;
		/* fill the opt related setting */
		__init_cpu_rtcwtc(cop);
		cop->pclk_div =
			cop->ap_clk_src / cop->pclk - 1;
		if (cop->l2clk) {
			cop->l2clk_div =
				cop->pclk / cop->l2clk - 1;
			cop->pdclk_div =
				cop->l2clk / cop->pdclk - 1;
			cop->baclk_div =
				cop->l2clk / cop->baclk - 1;
		} else {
			cop->pdclk_div =
				cop->pclk / cop->pdclk - 1;
			cop->baclk_div =
				cop->pclk / cop->baclk - 1;
		}
		if (cop->periphclk)
			cop->periphclk_div =
				cop->pclk / (4 * cop->periphclk) - 1;

		debug("%d(%d:%d,%d)\t%d([%s],%d)\t%d([%s],%d)\t%d([%s],%d)\t%d([%s],%d)\n",
		      cop->pclk, cop->ap_clk_src,
		      cop->ap_clk_sel & AP_SRC_SEL_MASK,
		      cop->pclk_div,
		      cop->l2clk, cop->l2clk ? "pclk" : "NULL",
		      cop->l2clk_div,
		      cop->pdclk, cop->l2clk ? "l2clk" : "pclk",
		      cop->pdclk_div,
		      cop->baclk, cop->l2clk ? "l2clk" : "pclk",
		      cop->baclk_div,
		      cop->periphclk,
		      cop->periphclk ? "pclk" : "NULL",
		      cop->periphclk_div);

		/* calc this op's voltage req */
		cop->volts = cal_comp_volts_req(CORE, cop->pclk);
	}
	/* override cpu_opt_size with real support tbl */
	cur_platform_opt->cpu_opt_size = idx;
}

static struct axi_rtcwtc axi_rtcwtc_1908umc[] = {
	{.max_aclk = 312, .xtc_val = 0xEE006656,},
};

static void __init_axi_rtcwtc(struct axi_opt *axi_opt)
{
	struct axi_rtcwtc *axi_xtc;
	unsigned int size, index;

	axi_xtc = axi_rtcwtc_1908umc;
	size = ARRAY_SIZE(axi_rtcwtc_1908umc);

	for (index = 0; index < size; index++)
		if (axi_opt->aclk <= axi_xtc[index].max_aclk)
			break;
	if (index == size)
		index = size - 1;

	axi_opt->xtc_val = axi_xtc[index].xtc_val;

	if (axi_opt->aclk == helan2_axi_bootop.aclk)
		__raw_writel(axi_opt->xtc_val, CIU_TOP_MEM_RTC_WTC_SPD);

}

static void __init_ddr_axi_opt(void)
{
	struct ddr_opt *ddr_opt, *ddr_cop;
	struct axi_opt *axi_opt, *axi_cop;
	union dfc_level_reg value;
	unsigned int ddr_opt_size = 0, axi_opt_size = 0, i;

	__init_axi_rtcwtc(cur_axi_op);

	ddr_opt = cur_platform_opt->ddr_opt;
	ddr_opt_size = cur_platform_opt->ddr_opt_size;
	axi_opt = cur_platform_opt->axi_opt;
	axi_opt_size = cur_platform_opt->axi_opt_size;

	debug("dclk(src:sel,div)\n");
	for (i = 0; i < ddr_opt_size; i++) {
		ddr_cop = &ddr_opt[i];
		ddr_cop->ddr_clk_src = ddr_sel2_srcrate(ddr_cop->ddr_clk_sel);
		BUG_ON(ddr_cop->ddr_clk_src < 0);
		ddr_cop->dclk_div =
			ddr_cop->ddr_clk_src / (2 * ddr_cop->dclk) - 1;
		debug("%d(%d:%d,%d)\n",
		      ddr_cop->dclk, ddr_cop->ddr_clk_src,
		      ddr_cop->ddr_clk_sel, ddr_cop->dclk_div);

		if (hwdfc_enable) {
			value.v = __raw_readl((unsigned long)DFC_LEVEL(i));
			value.b.dclksel = ddr_cop->ddr_clk_sel;
			value.b.ddr_clk_div = ddr_cop->dclk_div;
			value.b.mc_table_num = ddr_cop->ddr_tbl_index;
			value.b.volt_level = 0;
			__raw_writel(value.v, (unsigned long)DFC_LEVEL(i));
		}

		/* calc this op's voltage req */
		ddr_cop->volts = cal_comp_volts_req(DDR, ddr_cop->dclk);
	}

	debug("aclk(src:sel,div)\n");
	for (i = 0; i < axi_opt_size; i++) {
		axi_cop = &axi_opt[i];
		axi_cop->axi_clk_src = axi_sel2_srcrate(axi_cop->axi_clk_sel);
		BUG_ON(axi_cop->axi_clk_src < 0);
		axi_cop->aclk_div =
			axi_cop->axi_clk_src / axi_cop->aclk - 1;
		debug("%d(%d:%d,%d)\n",
		      axi_cop->aclk, axi_cop->axi_clk_src,
		      axi_cop->axi_clk_sel, axi_cop->aclk_div);

		/* calc this op's voltage req */
		axi_cop->volts = cal_comp_volts_req(AXI, axi_cop->aclk);
	}
}

static void __init_fc_setting(void)
{
	unsigned int regval;
	union pmua_cc cc_cp;
	union pmua_cc_ap cc_ap;
	/*
	 * enable AP FC done interrupt for one step,
	 * while not use three interrupts by three steps
	 */
	__raw_writel((1 << 1), APMU_IMR);

	/* always vote for CP allow AP FC */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	cc_cp.b.core_allow_spd_chg = 1;
	__raw_writel(cc_cp.v, APMU_CP_CCR);

	/* CA9 doesn't support halt acknowledge, mask it */
	regval = __raw_readl(APMU_DEBUG);
	regval |= (1 << 1);
	/*
	 * Always set AP_WFI_FC and CP_WFI_FC, then PMU will
	 * automaticlly send out clk-off ack when core is WFI
	 */
	regval |= (1 << 21) | (1 << 22);
	/*
	 * mask CP clk-off ack and cp halt ack for DDR/AXI FC
	 * this bits should be unmasked after cp is released
	 */
	regval |= (1 << 0) | (1 << 3);
	__raw_writel(regval, APMU_DEBUG);

	/* always use async for DDR, AXI interface */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.async5 = 1;
	cc_ap.b.async4 = 1;
	cc_ap.b.async3_1 = 1;
	cc_ap.b.async3 = 1;
	cc_ap.b.async2 = 1;
	cc_ap.b.async1 = 1;
	__raw_writel(cc_ap.v, APMU_CCR);

	/* use to enable PU_CLK shared by USB, PLL2 and PLL3 */
	__raw_writel(__raw_readl(UTMI_CTRL) | (1 << 23), UTMI_CTRL);

	/*
	 * vpu, gc, isp and LTE_DMA aclk need to be enabled and then disabled
	 * so that MC internal gating logic can work.
	 */
	/* vpu */
	regval = __raw_readl(APMU_REG(0x0A4));
	__raw_writel(regval | (0x1 | 0x1 << 3), APMU_REG(0x0A4));
	regval = __raw_readl(APMU_REG(0x0A4));
	__raw_writel(regval & ~(0x1 | 0x1 << 3), APMU_REG(0x0A4));
	/* gc */
	regval = __raw_readl(APMU_REG(0x0F4));
	__raw_writel(regval | (0x1 | 0x1 << 3), APMU_REG(0x0F4));
	regval = __raw_readl(APMU_REG(0x0F4));
	__raw_writel(regval & ~(0x1 | 0x1 << 3), APMU_REG(0x0F4));
	/* isp */
	regval = __raw_readl(APMU_REG(0x038));
	__raw_writel(regval | (0x1 << 16 | 0x1 << 17), APMU_REG(0x038));
	regval = __raw_readl(APMU_REG(0x038));
	__raw_writel(regval & ~(0x1 << 16 | 0x1 << 17), APMU_REG(0x038));
	/* LTE_DMA */
	regval = __raw_readl(APMU_REG(0x048));
	__raw_writel(regval | (0x1 | 0x1 << 1), APMU_REG(0x048));
	regval = __raw_readl(APMU_REG(0x048));
	__raw_writel(regval & ~(0x1 | 0x1 << 1), APMU_REG(0x048));

	/* enable all MCK and AXI fabric dynamic clk gating */
	regval = __raw_readl(MC_CONF);
	/* enable dclk gating */
	regval &= ~(1 << 19);
	/* enable 1x2 fabric AXI clock dynamic gating */
	regval |= (0xff << 8) |	/* MCK5 P0~P7*/
		(1 << 16) |		/* CP 2x1 fabric*/
		(1 << 17) | (1 << 18) |	/* AP&CP */
		(1 << 20) | (1 << 21) |	/* SP&CSAP 2x1 fabric */
		(1 << 26) | (1 << 27) | /* Fabric 0/1 */
		(1 << 29) | (1 << 30);	/* CA7 2x1 fabric */
	__raw_writel(regval, MC_CONF);

	/* enable MCK4 AHB clock */
	__raw_writel(0x3, APMU_MCK4_CTRL);
}

static struct cpu_opt *cpu_rate2_op_ptr
	(unsigned int rate, unsigned int *index)
{
	unsigned int idx = 0;
	struct cpu_opt *cop;

	list_for_each_entry(cop, &core_op_list, node) {
		if ((cop->pclk >= rate) ||
		    list_is_last(&cop->node, &core_op_list))
			break;
		idx++;
	}

	*index = idx;
	return cop;
}

static unsigned int cpu_get_oprateandvl(unsigned int index,
	unsigned int *volts)
{
	unsigned int idx = 0;
	struct cpu_opt *cop;

	list_for_each_entry(cop, &core_op_list, node) {
		if ((idx == index) ||
		    list_is_last(&cop->node, &core_op_list))
			break;
		idx++;
	}
	*volts = cop->volts;
	return cop->pclk;
}

static unsigned int ddr_rate2_op_index(unsigned int rate)
{
	unsigned int index;
	struct ddr_opt *op_array =
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

static unsigned int ddr_get_oprateandvl(unsigned int index,
	unsigned int *volts)
{
	*volts = cur_platform_opt->ddr_opt[index].volts;
	return cur_platform_opt->ddr_opt[index].dclk;
}

static unsigned int axi_rate2_op_index(unsigned int rate)
{
	unsigned int index;
	struct axi_opt *op_array =
		cur_platform_opt->axi_opt;
	unsigned int op_array_size =
		cur_platform_opt->axi_opt_size;

	if (unlikely(rate > op_array[op_array_size - 1].aclk))
		return op_array_size - 1;

	for (index = 0; index < op_array_size; index++)
		if (op_array[index].aclk >= rate)
			break;

	return index;
}

static unsigned int axi_get_oprateandvl(unsigned int index,
	unsigned int *volts)
{
	*volts = cur_platform_opt->axi_opt[index].volts;
	return cur_platform_opt->axi_opt[index].aclk;
}

static unsigned int cpu_sel2_srcrate(enum ap_clk_sel ap_sel)
{
	if (ap_sel == AP_CLK_SRC_PLL1_624)
		return 624;
	else if (ap_sel == AP_CLK_SRC_PLL1_1248)
		return 1248;
	else if (ap_sel == AP_CLK_SRC_PLL2)
		return PLAT_PLL_RATE(PLL2, pll);
	else if (ap_sel == AP_CLK_SRC_PLL1_832)
		return 832;
	else if (ap_sel == AP_CLK_SRC_PLL3P)
		return PLAT_PLL_RATE(PLL3, pllp);

	printf("%s bad ap_clk_sel %d\n", __func__, ap_sel);
	return -ENOENT;
}

static unsigned int ddr_sel2_srcrate(enum ddr_clk_sel ddr_sel)
{
	if (ddr_sel == DDR_CLK_SRC_PLL1_624)
		return 624;
	else if (ddr_sel == DDR_CLK_SRC_PLL1_832)
		return 832;
	else if (ddr_sel == DDR_CLK_SRC_PLL1_1248)
		return 1248;
	else if (ddr_sel == DDR_CLK_SRC_PLL2)
		return PLAT_PLL_RATE(PLL2, pll);
	else if (ddr_sel == DDR_CLK_SRC_PLL4)
		return PLAT_PLL_RATE(PLL4, pll);
	else if (ddr_sel == DDR_CLK_SRC_PLL3P)
		return PLAT_PLL_RATE(PLL3, pllp);

	printf("%s bad ddr_sel %d\n", __func__, ddr_sel);
	return -ENOENT;
}

static unsigned int axi_sel2_srcrate(enum axi_clk_sel axi_sel)
{
	if (axi_sel == AXI_CLK_SRC_PLL1_416)
		return 416;
	else if (axi_sel == AXI_CLK_SRC_PLL1_624)
		return 624;
	else if (axi_sel == AXI_CLK_SRC_PLL2)
		return PLAT_PLL_RATE(PLL2, pll);
	else if (axi_sel == AXI_CLK_SRC_PLL2P)
		return PLAT_PLL_RATE(PLL3, pllp);

	printf("%s bad axi_sel %d\n", __func__, axi_sel);
	return -ENOENT;
}

static unsigned int ap_clk_enable(struct cpu_opt *new)
{
	enum ap_clk_sel ap_sel = new->ap_clk_sel;
	struct pll_cfg *pll_cfg;
	if (ap_sel == AP_CLK_SRC_PLL2)
		turn_on_pll_defrate(PLL2);
	else if (ap_sel == AP_CLK_SRC_PLL3P) {
		pll_cfg = &cur_platform_opt->pll_cfg[PLL3];
		pll_cfg->pll_rate.pll = new->ap_clk_src;
		pll_cfg->pll_rate.pllp = new->ap_clk_src;
		pll_cfg->pll_rate.pll_vco = new->ap_clk_src;
		turn_on_pll_defrate(PLL3);
	}
	return 0;
}

static unsigned int ap_clk_disable(enum ap_clk_sel ap_sel)
{
	if (ap_sel == AP_CLK_SRC_PLL2)
		turn_off_pll(PLL2);
	else if (ap_sel == AP_CLK_SRC_PLL3P)
		turn_off_pll(PLL3);
	return 0;
}

static unsigned int ddr_clk_enable(enum ddr_clk_sel ddr_sel)
{
	if (ddr_sel == DDR_CLK_SRC_PLL2)
		turn_on_pll_defrate(PLL2);
	else if (ddr_sel == DDR_CLK_SRC_PLL4)
		turn_on_pll_defrate(PLL4);
	else if (ddr_sel == DDR_CLK_SRC_PLL3P)
		turn_on_pll_defrate(PLL3);
	return 0;
}

static unsigned int ddr_clk_disable(enum ddr_clk_sel ddr_sel)
{
	if (ddr_sel == DDR_CLK_SRC_PLL2)
		turn_off_pll(PLL2);
	else if (ddr_sel == DDR_CLK_SRC_PLL4)
		turn_off_pll(PLL4);
	else if (ddr_sel == DDR_CLK_SRC_PLL3P)
		turn_off_pll(PLL3);
	return 0;
}

static unsigned int axi_clk_enable(enum axi_clk_sel axi_sel)
{
	if ((axi_sel == AXI_CLK_SRC_PLL2) || (axi_sel == AXI_CLK_SRC_PLL2P))
		turn_on_pll_defrate(PLL2);
	return 0;
}

static unsigned int axi_clk_disable(enum axi_clk_sel axi_sel)
{
	if ((axi_sel == AXI_CLK_SRC_PLL2) || (axi_sel == AXI_CLK_SRC_PLL2P))
		turn_off_pll(PLL2);
	return 0;
}

static int fc_lock_ref_cnt;
static void get_fc_lock(void)
{
	union pmua_dm_cc dm_cc_ap;

	fc_lock_ref_cnt++;

	if (fc_lock_ref_cnt == 1) {
		int timeout = 100000;

		/* AP-CP FC mutual exclusion */
		dm_cc_ap.v = __raw_readl(APMU_CCSR);
		while (dm_cc_ap.b.cp_rd_status && timeout) {
			dm_cc_ap.v = __raw_readl(APMU_CCSR);
			timeout--;
		}
		if (timeout <= 0) {
			printf("cp does not release its fc lock\n");
			BUG();
		}
	}
}

static void put_fc_lock(void)
{
	union pmua_cc cc_ap;

	fc_lock_ref_cnt--;

	if (fc_lock_ref_cnt < 0)
		printf("unmatched put_fc_lock\n");

	if (fc_lock_ref_cnt == 0) {
		/* write 1 to MOH_RD_ST_CLEAR to clear MOH_RD_STATUS */
		cc_ap.v = __raw_readl(APMU_CCR);
		cc_ap.b.core_rd_st_clear = 1;
		__raw_writel(cc_ap.v, APMU_CCR);
		cc_ap.b.core_rd_st_clear = 0;
		__raw_writel(cc_ap.v, APMU_CCR);
	}
}

static void get_cur_cpu_op(struct cpu_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;
	union pmua_dm_cc2 dm_cc2_ap;
	u32 pll_freq, pllp_freq;

	get_fc_lock();

	dm_cc_ap.v = __raw_readl(APMU_CCSR);
	dm_cc2_ap.v = __raw_readl(APMU_CC2SR);
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);

	/* PLL Clock Select Status Register
	 * when APMU_PLL_SEL_STATUS[4:5] = 0x1, ap clk source depand on
	 * APMU_PLL_SEL_STATUS[8] bit status. If APMU_PLL_SEL_STATUS[8]
	 * is 0x0 ap clk is divided from PLL1_1248M or ap clk use PLL3P
	 * as clk source.
	 */
	if ((pllsel.b.apclksel1 == 0x1) &&
	    (pllsel.b.apclksel2 == 0x1)) {
		get_pll_freq(PLL3, &pll_freq, &pllp_freq);
		cop->ap_clk_src = pllp_freq;
	} else {
		cop->ap_clk_src = cpu_sel2_srcrate(pllsel.b.apclksel1);
		cop->ap_clk_sel = pllsel.b.apclksel1;
	}
	cop->pclk = cop->ap_clk_src / (dm_cc_ap.b.core_clk_div + 1);
	if (cop->l2clk) {
		cop->l2clk = cop->pclk / (dm_cc_ap.b.l2_clk_div + 1);
		cop->pdclk = cop->l2clk / (dm_cc_ap.b.bus_mc_clk_div + 1);
		cop->baclk = cop->l2clk / (dm_cc_ap.b.biu_clk_div + 1);
	} else {
		cop->pdclk = cop->pclk / (dm_cc_ap.b.bus_mc_clk_div + 1);
		cop->baclk = cop->pclk / (dm_cc_ap.b.biu_clk_div + 1);
	}
	if (cop->periphclk)
		cop->periphclk =
			cop->pclk / (dm_cc2_ap.b.peri_clk_div + 1) / 4;

	put_fc_lock();
}

static void get_cur_ddr_op(struct ddr_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;
	enum ddr_clk_sel ddr_sel;
	get_fc_lock();

	dm_cc_ap.v = __raw_readl(APMU_CCSR);
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);
	ddr_sel = pllsel.b.ddrclksel1 | (pllsel.b.ddrclksel2 << 2);
	cop->ddr_clk_src = ddr_sel2_srcrate(ddr_sel);
	cop->ddr_clk_sel = ddr_sel;
	cop->dclk = cop->ddr_clk_src / (dm_cc_ap.b.ddr_clk_div + 1) / 2;

	put_fc_lock();
}

static void get_cur_axi_op(struct axi_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;

	get_fc_lock();

	dm_cc_ap.v = __raw_readl(APMU_CCSR);
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);

	cop->axi_clk_src = axi_sel2_srcrate(pllsel.b.axiclksel);
	cop->axi_clk_sel = pllsel.b.axiclksel;
	cop->aclk = cop->axi_clk_src / (dm_cc_ap.b.bus_clk_div + 1);

	put_fc_lock();
}

static void wait_for_fc_done(void)
{
	int timeout = 1000000;
	while (!((1 << 1) & __raw_readl(APMU_ISR)) && timeout)
		timeout--;
	if (timeout <= 0)
		printf("AP frequency change timeout! ISR = 0x%x\n",
		       __raw_readl(APMU_ISR));
	__raw_writel(0x0, APMU_ISR);
}

static void set_ap_clk_sel(struct cpu_opt *top)
{
	u32 fcap, value;
	fcap = top->ap_clk_sel & MPMU_FCAP_MASK;
	debug_wt_reg(fcap, MPMU_FCAP);
	value = __raw_readl(MPMU_FCAP);
	if (value != fcap)
		printf("CORE FCAP Write failure: target 0x%X, final value 0x%X\n",
		       fcap, value);
}

static void set_ddr_clk_sel(struct ddr_opt *top)
{
	u32 fcdclk, value;
	fcdclk = top->ddr_clk_sel & MPMU_FCDCLK_MASK;
	debug_wt_reg(fcdclk, MPMU_FCDCLK);
	value = __raw_readl(MPMU_FCDCLK);
	if (value != fcdclk)
		printf("DDR FCDCLK Write failure: target 0x%X, final value 0x%X\n",
		       fcdclk, value);
}

static void set_axi_clk_sel(struct axi_opt *top)
{
	u32 fcaclk, value;
	fcaclk = top->axi_clk_sel & MPMU_FCACLK_MASK;
	debug_wt_reg(fcaclk, MPMU_FCACLK);
	value = __raw_readl(MPMU_FCACLK);
	if (value != fcaclk)
		printf("AXI FCACLK Write failure: target 0x%X, final value 0x%X\n",
		       fcaclk, value);
}

static void set_periph_clk_div(struct cpu_opt *top)
{
	union pmua_cc2 cc_ap2;

	cc_ap2.v = __raw_readl(APMU_CC2R);
	cc_ap2.b.peri_clk_div = top->periphclk_div;
	debug_wt_reg(cc_ap2.v, APMU_CC2R);
}

static void set_ddr_tbl_index(unsigned int index)
{
	unsigned int regval;

	/* APMU_MC_HW_SLP_TYPE[3:7] store the table number for mc5 */
	index &= 0x1F;
	regval = __raw_readl(APMU_MC_HW_SLP_TYPE);
	regval &= ~(0x1 << 10);		/* enable tbl based FC */
	regval &= ~(0x1F << 3);		/* clear ddr tbl index */
	regval |= (index << 3);
	debug_wt_reg(regval, APMU_MC_HW_SLP_TYPE);
}

/*
 * Sequence of changing RTC on the fly
 * RTC_lowpp means RTC is better for lowPP
 * RTC_highpp means RTC is better for highPP
 *
 * lowPP -> highPP:
 * 1) lowPP(RTC_lowpp) works at Vnom_lowPP(RTC_lowpp)
 * 2) Voltage increases from Vnom_lowPP(RTC_lowpp) to
 * Vnom_highPP(RTC_highpp)
 * 3) RTC changes from RTC_lowpp to RTC_highpp, lowPP(RTC_highpp)
 * could work at Vnom_highpp(RTC_highpp) as Vnom_highpp(RTC_highpp)
 * >= Vnom_lowpp(RTC_highpp)
 * 4) Core freq-chg from lowPP(RTC_highpp) to highPP(RTC_highpp)
 *
 * highPP -> lowPP:
 * 1) highPP(RTC_highpp) works at Vnom_highPP(RTC_highpp)
 * 2) Core freq-chg from highPP(RTC_highpp) to lowPP(RTC_highpp),
 * voltage could meet requirement as Vnom_highPP(RTC_highpp) >=
 * Vnom_lowpp(RTC_highpp)
 * 3) RTC changes from RTC_highpp to RTC_lowpp. Vnom_lowpp(RTC_lowpp)
 * < Vnom_lowpp(RTC_highpp), the voltage is ok
 * 4) voltage decreases from Vnom_highPP(RTC_highpp) to
 * Vnom_lowPP(RTC_lowpp)
 */
static void core_fc_seq(struct cpu_opt *cop,
			    struct cpu_opt *top)
{
	union pmua_cc cc_cp;
	union pmua_cc_ap cc_ap;

	/* low -> high */
	if (cop->pclk < top->pclk) {
		__raw_writel(top->l1_rtc,
			     CIU_CA9_CPU_CONF_SRAM_0);
		__raw_writel(top->l2_rtc,
			     CIU_CA9_CPU_CONF_SRAM_1);
	}

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n",
		       __func__);
		cc_cp.b.core_allow_spd_chg = 1;
		__raw_writel(cc_cp.v, APMU_CP_CCR);
	}

	/* 1) Pre FC : AP votes allow FC */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 1;

	/* 2) issue core FC */
	/* 2.1) set pclk src */
	set_ap_clk_sel(top);
	/* 2.2) select div for pclk, l2clk, pdclk, baclk */
	cc_ap.b.core_clk_div = top->pclk_div;
	if (top->l2clk)
		cc_ap.b.l2_clk_div = top->l2clk_div;
	cc_ap.b.bus_mc_clk_div = top->pdclk_div;
	cc_ap.b.biu_clk_div = top->baclk_div;
	/* 2.3) set periphclk div */
	if (top->periphclk)
		set_periph_clk_div(top);

	cc_ap.b.core_freq_chg_req = 1;
	/* used only for core FC, will NOT trigger fc_sm */
	/* cc_ap.b.core_dyn_fc = 1; */

	/* 2.4) set div and FC req trigger core FC */
	debug_wt_reg(cc_ap.v, APMU_CCR);
	wait_for_fc_done();

	/* 3) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);

	/* high -> low */
	if (cop->pclk > top->pclk) {
		__raw_writel(top->l1_rtc,
			     CIU_CA9_CPU_CONF_SRAM_0);
		__raw_writel(top->l2_rtc,
			     CIU_CA9_CPU_CONF_SRAM_1);
	}
}

static int set_core_freq(struct cpu_opt *old, struct cpu_opt *new)
{
	struct cpu_opt cop;
	int ret = 0;

	/*
	 * add check here to avoid the case default boot up PP
	 * has different pdclk and paclk definition with PP used
	 * in production
	 */
	if ((old->ap_clk_src == new->ap_clk_src) &&
	    (old->pclk == new->pclk) &&
	    (old->pdclk == new->pdclk) &&
	    (old->baclk == new->baclk)) {
		printf("current rate = target rate!\n");
		return -EAGAIN;
	}

	printf("CORE FC start: %u -> %u\n",
	       old->pclk, new->pclk);
	get_fc_lock();

	memcpy(&cop, old, sizeof(struct cpu_opt));
#if 0
	/*
	 * We do NOT check it here as system boot up
	 * PP may be has different pdclk,paclk with
	 * PP table we defined.Only check the freq
	 * after every FC.
	 */
	get_cur_cpu_op(&cop);
	if (unlikely((cop.ap_clk_src != old->ap_clk_src) ||
		     (cop.pclk != old->pclk) ||
		(cop.l2clk != old->l2clk) ||
		(cop.pdclk != old->pdclk) ||
		(cop.baclk != old->baclk) ||
		(cop.periphclk != old->periphclk))) {
		printf("psrc pclk l2clk pdclk baclk periphclk\n");
		printf("OLD %d %d %d %d %d %d\n", old->ap_clk_src,
		       old->pclk, old->l2clk, old->pdclk, old->baclk,
		       old->periphclk);
		printf("CUR %d %d %d %d %d %d\n", cop.ap_clk_src,
		       cop.pclk, cop.l2clk, cop.pdclk, cop.baclk,
		       cop.periphclk);
		printf("NEW %d %d %d %d %d %d\n", new->ap_clk_src,
		       new->pclk, new->l2clk, new->pdclk, new->baclk,
		       new->periphclk);
	}
#endif
	ap_clk_enable(new);
	core_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct cpu_opt));
	get_cur_cpu_op(&cop);
	if (unlikely((cop.ap_clk_src != new->ap_clk_src) ||
		     (cop.pclk != new->pclk) ||
		(cop.l2clk != new->l2clk) ||
		(cop.pdclk != new->pdclk) ||
		(cop.baclk != new->baclk) ||
		(cop.periphclk != new->periphclk))) {
		printf("unsuccessful frequency change!\n");
		printf("psrc pclk l2clk pdclk baclk periphclk\n");
		printf("CUR %d %d %d %d %d %d\n", cop.ap_clk_src,
		       cop.pclk, cop.l2clk, cop.pdclk, cop.baclk,
		       cop.periphclk);
		printf("NEW %d %d %d %d %d %d\n", new->ap_clk_src,
		       new->pclk, new->l2clk, new->pdclk, new->baclk,
		       new->periphclk);
		ret = -EAGAIN;
		ap_clk_disable(new->ap_clk_sel);
		goto out;
	}

	ap_clk_disable(old->ap_clk_sel);
out:
	put_fc_lock();
	printf("CORE FC end: %u -> %u\n",
	       old->pclk, new->pclk);
	return ret;
}

static int cpu_setrate(unsigned long rate)
{
	struct cpu_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;

	if (unlikely(!cur_cpu_op))
		cur_cpu_op = &helan2_cpu_bootop;
	md_new = cpu_rate2_op_ptr(rate, &index);
	md_old = cur_cpu_op;

	if ((md_old->ap_clk_sel == md_new->ap_clk_sel) &&
		(md_new->ap_clk_src != md_old->ap_clk_src)) {
		if (!bridge)
			bridge = list_first_entry(&core_op_list,
					struct cpu_opt, node);
		printf("The same clk src is used by new op,");
		printf("change to %d as a bridge op\n", bridge->pclk);
		/* 1) set core to bridge op */
		ret = set_core_freq(md_old, bridge);
		if (ret) {
			printf("FC error occurred when change freq %d to %d\n",
			       md_old->pclk, bridge->pclk);
			goto out;
		}
		/* 2) change core to target op */
		ret = set_core_freq(bridge, md_new);
	} else {
		ret = set_core_freq(md_old, md_new);
	}
	if (ret)
		goto out;
	cur_cpu_op = md_new;
out:
	return ret;
}

static void ddr_fc_seq(struct ddr_opt *cop,
			    struct ddr_opt *top)
{
	union pmua_cc cc_cp;
	union pmua_cc_ap cc_ap;
	unsigned int ddr_fc = 0;

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n",
		       __func__);
		cc_cp.b.core_allow_spd_chg = 1;
		__raw_writel(cc_cp.v, APMU_CP_CCR);
	}

	/* 1) Pre FC : AP votes allow FC */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 1;

	/* 2) issue DDR FC */
	if ((cop->ddr_clk_src != top->ddr_clk_src) ||
	    (cop->dclk != top->dclk)) {
		/* 2.1) set dclk src */
		set_ddr_clk_sel(top);
		/* 2.2) enable tbl based FC and set DDR tbl num */
		set_ddr_tbl_index(top->ddr_tbl_index);
		/* 2.3) select div for dclk */
		cc_ap.b.ddr_clk_div = top->dclk_div;
		/* 2.4) select ddr FC req bit */
		cc_ap.b.ddr_freq_chg_req = 1;
		ddr_fc = 1;
	}

	/* 3) set div and FC req bit trigger DDR/AXI FC */
	if (ddr_fc) {
		debug_wt_reg(cc_ap.v, APMU_CCR);
		wait_for_fc_done();
	}

	/* 4) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);
}

static void axi_fc_seq(struct axi_opt *cop,
			    struct axi_opt *top)
{
	union pmua_cc cc_cp;
	union pmua_cc_ap cc_ap;
	unsigned int axi_fc = 0;

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n",
		       __func__);
		cc_cp.b.core_allow_spd_chg = 1;
		__raw_writel(cc_cp.v, APMU_CP_CCR);
	}

	/* 1) Pre FC : AP votes allow FC */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 1;

	/* 2) issue AXI FC */
	if ((cop->axi_clk_src != top->axi_clk_src) ||
	    (cop->aclk != top->aclk)) {
		/* 3.1) set aclk src */
		set_axi_clk_sel(top);
		/* 3.2) select div for aclk */
		cc_ap.b.bus_clk_div = top->aclk_div;
		/* 3.3) select axi FC req bit */
		cc_ap.b.bus_freq_chg_req = 1;
		axi_fc = 1;
	}

	/* 3) set div and FC req bit trigger DDR/AXI FC */
	if (axi_fc) {
		debug_wt_reg(cc_ap.v, APMU_CCR);
		wait_for_fc_done();
	}

	/* 4) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);
}

static void __ddr_hwdfc_seq(unsigned int level)
{
	union dfc_ap dfc_ap;
	union dfc_status status;
	dfc_ap.v = __raw_readl(DFC_AP);
	dfc_ap.b.freq_level = level;
	dfc_ap.b.dfc_req = 1;
	__raw_writel(dfc_ap.v, DFC_AP);
	wait_for_fc_done();
	status.v = __raw_readl(DFC_STATUS);
	if (unlikely(status.b.cfl != level)) {
		printf("HW-DFC failed! expect LV %d, actual LV %d\n",
		       level, status.b.cfl);
		printf("HW_AP: %x, HW_STATUS: %x\n",
		       __raw_readl(DFC_AP), status.v);
	}
}


static int first_ddr_fc;
static void ddr_hwdfc_seq(struct ddr_opt *cop,
		struct ddr_opt *top)
{
	/*
	 * HW thinks default DFL is 0, we have to make sure HW
	 * get the correct DFL by first change it to 0, then change
	 * it to current DFL
	 */
	if (unlikely(!first_ddr_fc)) {
		__ddr_hwdfc_seq(0);
		__ddr_hwdfc_seq(cop->ddr_freq_level);
		first_ddr_fc = 1;
	}

	__ddr_hwdfc_seq(top->ddr_freq_level);
}

static int set_ddr_freq(struct ddr_opt *old,
	struct ddr_opt *new)
{
	struct ddr_opt cop;
	int ret = 0, errflag = 0;

	printf("DDR FC start: DDR %u -> %u\n",
	       old->dclk, new->dclk);

	if (!hwdfc_enable)
		get_fc_lock();

	memcpy(&cop, old, sizeof(struct ddr_opt));

	ddr_clk_enable(new->ddr_clk_sel);

	if (hwdfc_enable)
		ddr_hwdfc_seq(&cop, new);
	else
		ddr_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct ddr_opt));
	get_cur_ddr_op(&cop);
	if (unlikely((cop.ddr_clk_src != new->ddr_clk_src) ||
		     (cop.dclk != new->dclk))) {
		ddr_clk_disable(new->ddr_clk_sel);
		errflag = 1;
	}
	if (unlikely(errflag)) {
		printf("DDR:unsuccessful frequency change!\n");
		printf(" dsrc dclk\n");
		printf("CUR %d %d\n", cop.ddr_clk_src, cop.dclk);
		printf("NEW %d %d\n", new->ddr_clk_src, new->dclk);
		ret = -EAGAIN;
		goto out;
	}

	ddr_clk_disable(old->ddr_clk_sel);
out:
	if (!hwdfc_enable)
		put_fc_lock();
	printf("DDR FC end: DDR %u -> %u\n", old->dclk, new->dclk);
	return ret;
}

static int set_axi_freq(struct axi_opt *old,
	struct axi_opt *new)
{
	struct axi_opt cop;
	int ret = 0, errflag = 0;

	printf("AXI FC start: AXI %u -> %u\n",
	       old->aclk, new->aclk);
	get_fc_lock();

	memcpy(&cop, old, sizeof(struct axi_opt));
	axi_clk_enable(new->axi_clk_sel);
	axi_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct axi_opt));
	get_cur_axi_op(&cop);
	if (unlikely((cop.axi_clk_src != new->axi_clk_src) ||
		     (cop.aclk != new->aclk))) {
		axi_clk_disable(new->axi_clk_sel);
		errflag = 1;
	}
	if (unlikely(errflag)) {
		printf("AXI:unsuccessful frequency change!\n");
		printf("asrc aclk\n");
		printf("CUR %d %d\n", cop.axi_clk_src, cop.aclk);
		printf("NEW %d %d\n", new->axi_clk_src, new->aclk);
		ret = -EAGAIN;
		goto out;
	}

	axi_clk_disable(old->axi_clk_sel);
out:
	put_fc_lock();
	printf("AXI FC end: AXI %u -> %u\n",
	       old->aclk, new->aclk);
	return ret;
}

static int ddr_setrate(unsigned long rate)
{
	struct ddr_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	struct ddr_opt *op_array =
		cur_platform_opt->ddr_opt;

	if (unlikely(!cur_ddr_op))
		cur_ddr_op = &helan2_ddr_bootop;

	index = ddr_rate2_op_index(rate);
	md_new = &op_array[index];
	md_old = cur_ddr_op;

	ret = set_ddr_freq(md_old, md_new);
	if (ret)
		goto out;
	cur_ddr_op = md_new;
out:
	return ret;
}

static int axi_setrate(unsigned long rate)
{
	struct axi_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	struct axi_opt *op_array =
		cur_platform_opt->axi_opt;

	if (unlikely(!cur_axi_op))
		cur_axi_op = &helan2_axi_bootop;

	index = axi_rate2_op_index(rate);
	md_new = &op_array[index];
	md_old = cur_axi_op;

	ret = set_axi_freq(md_old, md_new);
	if (ret)
		goto out;
	cur_axi_op = md_new;
out:
	return ret;
}

/*
 * This function is called from board level or cmd to
 * issue the core, ddr&axi frequency change.
 */
int setop(unsigned int pclk, unsigned int dclk, unsigned int aclk)
{
	u32 pidx, didx, aidx;
	u32 pvcore, dvcore, avcore, vcore, cur_volt;
	int ret = 0;

	cpu_rate2_op_ptr(pclk, &pidx);
	didx = ddr_rate2_op_index(dclk);
	aidx = axi_rate2_op_index(aclk);
	cpu_get_oprateandvl(pidx, &pvcore);
	ddr_get_oprateandvl(didx, &dvcore);
	axi_get_oprateandvl(aidx, &avcore);
	vcore = max(pvcore, dvcore);
	vcore = max(vcore, avcore);

	cur_volt = get_volt();
	printf("cur %umV, tgt %umV\n", cur_volt, vcore);
	if (vcore > cur_volt) {
		ret = set_volt(vcore);
		if (ret) {
			printf("Increase Vcore %umV->%umV failed!\n",
			       cur_volt, vcore);
			return -1;
		} else
			printf("Increase Vcore %umV->%umV done!\n",
			       cur_volt, vcore);
	}

	ret |= ddr_setrate(dclk);
	ret |= axi_setrate(aclk);
	ret |= cpu_setrate(pclk);
	/* any failed, return without voltage change */
	if (ret)
		return 0;

	if (vcore < cur_volt) {
		ret = set_volt(vcore);
		if (ret) {
			printf("Decrease Vcore %umV->%umV failed!\n",
			       cur_volt, vcore);
			return -1;
		} else
			printf("Decrease Vcore %umV->%umV done!\n",
			       cur_volt, vcore);
	}
	return 0;
}

/* use to cal core/ddr/axi VL request */
static unsigned int cal_comp_volts_req(enum vcore_comps comp,
	unsigned int rate)
{
	unsigned int idx = 0;
	unsigned int vlreq = VL_MAX - 1;

	if (!cur_platform_opt->freqs_cmb ||
	    !cur_platform_opt->vm_millivolts) {
		printf("Fill volts req info at first!\n");
		return 0;
	}

	if (comp > VM_RAIL_MAX) {
		printf("Bad paras! comp:%d\n", comp);
		goto out;
	}

	for (idx = 0; idx < VL_MAX; idx++)
		if (rate <= cur_platform_opt->freqs_cmb[comp][idx])
			break;

	if (idx == VL_MAX) {
		printf("Unsupported rate:%u, comp:%d\n", rate, comp);
		goto out;
	}
	vlreq = idx;
out:
	return cur_platform_opt->vm_millivolts[vlreq];
}

static void __init_volts_info(int profile)
{
	int *vm_millivolts;
	unsigned int (*freqs_cmb)[VL_MAX];

	/* FIXME: register platform info according to profile  */
	vm_millivolts = vm_mv_helan2_svc[profile];
	freqs_cmb = freqs_cmb_helan2;

	cur_platform_opt->vm_millivolts = vm_millivolts;
	cur_platform_opt->freqs_cmb = freqs_cmb;

#define SOC_DVC1		93
#define SOC_DVC2		94
#define SOC_DVC3		95
	/* always use DVC[n:0] = 0 in uboot */
	gpio_direction_output(SOC_DVC1, 0);
	gpio_direction_output(SOC_DVC2, 0);
	gpio_direction_output(SOC_DVC3, 0);
	/* set default DVC LV to 0, hw reset value is 0x4 */
	setapactivevl(0);

	return;
}

/*
 * This function is called from board/CPU init level to initialize
 * the OP table/dvfs and frequency change related setting
 */
void helan2_fc_init(int ddr_mode)
{
	cur_cpu_op = &helan2_cpu_bootop;
	get_cur_cpu_op(cur_cpu_op);

	cur_ddr_op = &helan2_ddr_bootop;
	get_cur_ddr_op(cur_ddr_op);

	cur_axi_op = &helan2_axi_bootop;
	get_cur_axi_op(cur_axi_op);

	/* notes that below sequence has dependency */
	__init_read_ultinfo();

	/* init platform op table related */
	__init_platform_opt(ddr_mode);

	/* init volt req info, FIXME: pass profile */
	__init_volts_info(0);

	/* init cpu/ddr/axi related setting */
	__init_cpu_opt();
	__init_ddr_axi_opt();

	/* init freq-chg related hw setting */
	__init_fc_setting();

}

void show_op(void)
{
	struct cpu_opt cop, *op;
	struct ddr_opt ddr_op, *ddr_opt;
	struct axi_opt axi_op, *axi_opt;
	unsigned int ddr_opt_size, axi_opt_size, i;

	ddr_opt = cur_platform_opt->ddr_opt;
	ddr_opt_size = cur_platform_opt->ddr_opt_size;
	axi_opt = cur_platform_opt->axi_opt;
	axi_opt_size = cur_platform_opt->axi_opt_size;

	printf("Current OP:\n");
	memcpy(&cop, cur_cpu_op, sizeof(struct cpu_opt));
	printf("pclk(src:sel) l2clk(src)\tpdclk(src)\tbaclk(src)\tperiphclk(src)\n");
	printf("%d(%d:%d)\t%d([%s])\t%d([%s])\t%d([%s])\t%d([%s])\n",
	       cop.pclk, cop.ap_clk_src,
		cop.ap_clk_sel & AP_SRC_SEL_MASK,
		cop.l2clk, cop.l2clk ? "pclk" : "NULL",
		cop.pdclk, cop.l2clk ? "l2clk" : "pclk",
		cop.baclk, cop.l2clk ? "l2clk" : "pclk",
		cop.periphclk, cop.periphclk ? "pclk" : "NULL");

	memcpy(&ddr_op, cur_ddr_op, sizeof(struct ddr_opt));
	printf("dclk(src:sel)\n");
	printf("%d(%d:%d)\n", ddr_op.dclk, ddr_op.ddr_clk_src,
	       ddr_op.ddr_clk_sel);

	memcpy(&axi_op, cur_axi_op, sizeof(struct axi_opt));
	printf("aclk(src:sel)\n");
	printf("%d(%d:%d)\n",
	       axi_op.aclk, axi_op.axi_clk_src, axi_op.axi_clk_sel);

	printf("\nAll supported OP:\n");
	printf("Core(MHZ@mV):\n");
	list_for_each_entry(op, &core_op_list, node)
		printf("%d@%d\t", op->pclk, op->volts);

	printf("\n");

	printf("DDR(MHZ@mV):\n");
	for (i = 0; i < ddr_opt_size; i++)
		printf("%d@%d\t", ddr_opt[i].dclk, ddr_opt[i].volts);
	printf("\n");

	printf("AXI(MHZ@mV):\n");
	for (i = 0; i < axi_opt_size; i++)
		printf("%d@%d\t", axi_opt[i].aclk, axi_opt[i].volts);
	printf("\n");
}

int do_op(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int pclk, dclk, aclk;

	if (argc == 4) {
		pclk = simple_strtoul(argv[1], NULL, 0);
		dclk = simple_strtoul(argv[2], NULL, 0);
		aclk = simple_strtoul(argv[3], NULL, 0);
		printf("op: pclk = %dMHZ dclk = %dMHZ aclk = %dMHZ\n",
		       pclk, dclk, aclk);

		setop(pclk, dclk, aclk);
	} else if (argc >= 2) {
		printf("usage: op pclk dclk aclk(unit Mhz)\n");
	} else {
		show_op();
	}
	return 0;
}

U_BOOT_CMD(
	op,	4,	1,	do_op,
	"change operating point",
	"[ op number ]"
);


#define VOL_BASE		600000
#define VOL_STEP		12500
#define VOL_HIGH		1400000

static int set_volt(u32 vol)
{
	int ret = 0;
	struct pmic *p_power;
	struct pmic_chip_desc *board_pmic_chip;

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
	ret = marvell88pm_set_buck_vol(p_power, 1, vol);
	printf("Set VBUCK1 to %4dmV\n", vol / 1000);

	return ret;
}

static u32 get_volt(void)
{
	u32 vol = 0;
	struct pmic *p_power;
	struct pmic_chip_desc *board_pmic_chip;

	board_pmic_chip = get_marvell_pmic();
	if (!board_pmic_chip)
		return -1;

	p_power = pmic_get(board_pmic_chip->power_name);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;

	vol = marvell88pm_get_buck_vol(p_power, 1);
	if (vol < 0)
		return -1;

	return vol / 1000;
}

int do_setvolt(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong vol;
	int res = -1;
	if ((argc < 1) || (argc > 2))
		return -1;

	if (argc == 1) {
		printf("usage: setvolt xxxx(unit mV)\n"
			"xxxx can be 600mV~1350mV, step 13mV\n"
			);
		return 0;
	}
	res = strict_strtoul(argv[1], 0, &vol);
	if (res == 0 && set_volt(vol) == 0)
		printf("Voltage change was successful\n");
	else
		printf("Voltage change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setvolt, 6, 1, do_setvolt,
	"Setting voltages",
	""
);

int do_setcpurate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct cpu_opt *cop;
	ulong freq;
	int res = -1;

	if (argc != 2) {
		printf("usage: set core frequency xxxx(unit Mhz)\n");
		printf("Supported Core frequency: ");
		list_for_each_entry(cop, &core_op_list, node)
			printf("%d\t", cop->pclk);
		printf("\n");
		return res;
	}
	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (cpu_setrate(freq) == 0))
		printf("Core freq change was successful\n");
	else
		printf("Core freq change was unsuccessful\n");
	return 0;
}

U_BOOT_CMD(
	setcpurate, 6, 1, do_setcpurate,
	"Setting core rate",
	""
);

int do_setddrrate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct ddr_opt *ddr_opt;
	unsigned int ddr_opt_size = 0, i;
	ulong freq;
	int res = -1;

	ddr_opt = cur_platform_opt->ddr_opt;
	ddr_opt_size = cur_platform_opt->ddr_opt_size;

	if (argc != 2) {
		printf("usage: set ddr frequency xxxx(unit Mhz)\n");
		printf("Supported ddr frequency: ");
		for (i = 0; i < ddr_opt_size; i++)
			printf("%d\t", ddr_opt[i].dclk);

		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (ddr_setrate(freq) == 0))
		printf("DDR freq change was successful\n");
	else
		printf("DDR freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setddrrate, 6, 1, do_setddrrate,
	"Setting ddr rate",
	""
);

int do_setaxirate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct axi_opt *axi_opt;
	unsigned int axi_opt_size = 0, i;
	ulong freq;
	int res = -1;

	axi_opt = cur_platform_opt->axi_opt;
	axi_opt_size = cur_platform_opt->axi_opt_size;

	if (argc != 2) {
		printf("usage: set axi frequency xxxx(unit Mhz)\n");
		printf("Supported axi frequency: ");
		for (i = 0; i < axi_opt_size; i++)
			printf("%d\t", axi_opt[i].aclk);

		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (axi_setrate(freq) == 0))
		printf("AXI freq change was successful\n");
	else
		printf("AXI freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setaxirate, 6, 1, do_setaxirate,
	"Setting axi rate",
	""
);

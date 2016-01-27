/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Jane Li <jiel@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <errno.h>
#include <common.h>
#include <power/pxa_ddr.h>
#include <asm/gpio.h>
#include <asm/arch/features.h>
#include <linux/list.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>

#ifdef CONFIG_TZ_HYPERVISOR
#include <pxa_tzlc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#define	APMU_BASE		0xD4282800
#define APMU_REG(x)		(APMU_BASE + x)
#define	MPMU_BASE		0xD4050000
#define MPMU_REG(x)		(MPMU_BASE + x)
#define APB_SPARE_BASE		0xD4090000
#define APB_SPARE_REG(x)	(APB_SPARE_BASE + x)
#define CIU_BASE		0xD4282C00
#define CIU_REG(x)		(CIU_BASE + x)
#define AXI_PHYS_BASE		0xd4200000
/* PLL */
#define MPMU_PLL2CR		MPMU_REG(0x0034)
#define MPMU_PLL3CR		MPMU_REG(0x001c)
#define APB_SPARE_PLL2CR	APB_SPARE_REG(0x104)
#define APB_SPARE_PLL3CR	APB_SPARE_REG(0x108)

#define APB_PLL2_PI_CTRL	APB_SPARE_REG(0x118)
#define APB_PLL2_SSC_CTRL	APB_SPARE_REG(0x11C)
#define APB_PLL2_FREQOFFSET_CTRL	APB_SPARE_REG(0x120)
#define APB_PLL3_PI_CTRL	APB_SPARE_REG(0x124)
#define APB_PLL3_SSC_CTRL	APB_SPARE_REG(0x128)
#define APB_PLL3_FREQOFFSET_CTRL	APB_SPARE_REG(0x12C)
#define MPMU_POSR		MPMU_REG(0x0010)

/* FC */
#define APMU_CP_CCR             APMU_REG(0x0000)
#define APMU_CCR                APMU_REG(0x0004)
#define APMU_CCSR               APMU_REG(0x000c)

#define APMU_IMR                APMU_REG(0x0098)
#define APMU_ISR                APMU_REG(0x00a0)
#define APMU_PLL_SEL_STATUS     APMU_REG(0x00c4)
#define APMU_MC_HW_SLP_TYPE     APMU_REG(0x00b0)

#define APMU_CC2R               APMU_REG(0x0100)
#define APMU_CC2SR              APMU_REG(0x0104)

#define	MPMU_FCCR		MPMU_REG(0x0008)

/* clock gating */
#define MC_CONF		CIU_REG(0x40)
#define APMU_MCK4_CTRL		APMU_REG(0x0e8)

/* core WTC/RTC */
#define CIU_CA9_CPU_CONF_SRAM_0		CIU_REG(0x00c8)
#define CIU_CA9_CPU_CONF_SRAM_1		CIU_REG(0x00cc)

/* FUSE */
#define UIMAINFUSE_95_64	(AXI_PHYS_BASE + 0x1418)
#define BLOCK0_224_255		(AXI_PHYS_BASE + 0x1420)

/*
 * for ana_grp PU_CLK contro. Must be enabled if
 * PLL2 or PLL3 or USB are used
 */
#define UTMI_CTRL		0xD4207104

#define AP_SRC_SEL_MASK		0x7
#define UNDEF_OP		-1
#define MHZ			(1000 * 1000)
#define MHZ_TO_KHZ		(1000)

#ifdef CONFIG_FINE_TUNED_SVC
#define PROFILE_NUM	11
#else
#define PROFILE_NUM	8
#endif

#define PLL3_VCO_MIN		(1200)
#define PLL3_VCO_MAX		(2500)

enum {
	CORE_1P18G = 1183,
	CORE_1P25G = 1248,
	CORE_1P5G = 1482,
};

union pmum_pll2cr {
	struct {
		unsigned int reserved0:6;
		unsigned int reserved1:2;
		unsigned int en:1;
		unsigned int ctrl:1;
		unsigned int pll2fbd:9;
		unsigned int pll2refd:5;
		unsigned int reserved2:8;
	} b;
	unsigned int v;
};

union pmum_pll3cr {
	struct {
		unsigned int pll3refd:5;
		unsigned int pll3fbd:9;
		unsigned int reserved0:4;
		unsigned int pclk_1248_sel:1;
		unsigned int pll3_pu:1;
		unsigned int reserved1:12;
	} b;
	unsigned int v;
};

union apb_spare_pllswcr {
	struct {
		unsigned int lineupen:1;
		unsigned int gatectl:1;
		unsigned int bypassen:1;
		unsigned int diffclken:1;
		unsigned int divselse:4;
		unsigned int divseldiff:4;
		unsigned int ctune:2;
		unsigned int vcovnrg:3;
		unsigned int kvco:4;
		unsigned int icp:3;
		unsigned int vreg_ivreg:2;
		unsigned int vddl:4;
		unsigned int vddm:2;
	} b;
	unsigned int v;
};

union pmum_posr {
	struct {
		unsigned int pll1fbd:9;
		unsigned int pll1refd:5;
		unsigned int pll2fbd:9;
		unsigned int pll2refd:5;
		unsigned int reserved:4;
	} b;
	unsigned int v;
};

union pmum_fccr {
	struct {
		unsigned int pll1fbd:9;
		unsigned int pll1refd:5;
		unsigned int pll1cen:1;
		unsigned int mfc:1;
		unsigned int reserved0:3;
		unsigned int axiclksel0:1;
		unsigned int reserved1:3;
		unsigned int ddrclksel:2;
		unsigned int axiclksel1:1;
		unsigned int seaclksel:2;
		unsigned int i2sclksel:1;
		unsigned int mohclksel:3;
	} b;
	unsigned int v;
};

union pmua_pllsel {
	struct {
		unsigned int cpclksel:2;
		unsigned int apclksel:2;
		unsigned int ddrclksel:2;
		unsigned int axiclksel:2;
		unsigned int reserved0:24;
	} b;
	unsigned int v;
};

union pmua_cc {
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

union apb_pllx_pi_ctrl {
	struct {
		unsigned int pi_en:1;
		unsigned int ssc_clk_en:1;
		unsigned int clk_det_en:1;
		unsigned int reset_ext:1;
		unsigned int intpi:4;
		unsigned int sel_vco_se:1;
		unsigned int sel_vco_diff:1;
		unsigned int reserved:22;
	} b;
	unsigned int v;
};

union apb_pllx_ssc_ctrl {
	struct {
		unsigned int ssc_en:1;
		unsigned int ssc_mode:1;
		unsigned int ssc_reset_ext:1;
		unsigned int ssc_rnge:11;
		unsigned int reserved:2;
		unsigned int ssc_freq_div:16;
	} b;
	unsigned int v;
};

union apb_pllx_freqoffset_ctrl {
	struct {
		unsigned int freq_offset_en:1;
		unsigned int freq_offset_valid:1;
		unsigned int freq_offset:17;
		unsigned int reserve_in:4;
		unsigned int reserved:9;
	} b;
	unsigned int v;
};

/*
 * AP clock source:
 * 0x0 = PLL1 624 MHz
 * 0x1 = PLL1 1248 MHz  or PLL3_CLKOUT
 * (depending on PLL3_CR[18])
 * 0x2 = PLL2_CLKOUT
 * 0x3 = PLL2_CLKOUTP
 */
enum ap_clk_sel {
	AP_CLK_SRC_PLL1_624 = 0x0,
	AP_CLK_SRC_PLL1_1248 = 0x1,
	AP_CLK_SRC_PLL2 = 0x2,
	AP_CLK_SRC_PLL2P = 0x3,
	AP_CLK_SRC_PLL3P = 0x11,
};

/*
 * DDR/AXI clock source:
 * 0x0 = PLL1 416 MHz
 * 0x1 = PLL1 624 MHz
 * 0x2 = PLL2_CLKOUT
 * 0x3 = PLL2_CLKOUTP
 */
enum ddr_axi_clk_sel {
	DDR_AXI_CLK_SRC_PLL1_416 = 0x0,
	DDR_AXI_CLK_SRC_PLL1_624 = 0x1,
	DDR_AXI_CLK_SRC_PLL2 = 0x2,
	DDR_AXI_CLK_SRC_PLL2P = 0x3,
};

enum ddr_type {
	LPDDR2_400M = 0,
	LPDDR2_533M,
	DDR3_533M,
};

enum ssc_mode {
	CENTER_SPREAD = 0x0,
	DOWN_SPREAD = 0x1,
};

struct pll_post_div {
	unsigned int div;	/* PLL divider value */
	unsigned int divselval;	/* PLL corresonding reg setting */
};

/* RTC/WTC table used for solution change rtc/wtc on the fly */
struct cpu_rtcwtc {
	/* max rate could be used by this rtc/wtc */
	unsigned int max_pclk;
	unsigned int l1_rtc;
	unsigned int l2_rtc;
};


struct pxa988_cpu_opt {
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
};

struct pxa988_ddr_axi_opt {
	unsigned int dclk;		/* ddr clock */
	unsigned int ddr_tbl_index;	/* ddr FC table index */
	unsigned int aclk;		/* axi clock */
	enum ddr_axi_clk_sel ddr_clk_sel;/* ddr src sel val */
	enum ddr_axi_clk_sel axi_clk_sel;/* axi src sel val */
	unsigned int ddr_clk_src;	/* ddr src rate */
	unsigned int axi_clk_src;	/* axi src rate */
	unsigned int dclk_div;		/* ddr clk divider */
	unsigned int aclk_div;		/* axi clk divider */
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
	enum ddr_type ddrtype;
	char *cpu_name;
	struct operating_point *op_array;
	unsigned int com_op_size;
	struct pxa988_cpu_opt *cpu_opt;
	unsigned int cpu_opt_size;
	struct pxa988_ddr_axi_opt *ddr_axi_opt;
	unsigned int ddr_axi_opt_size;
	/* pll2 & pll3 freq used on this platform */
	unsigned int pll2vcofreq;
	unsigned int pll2freq;
	unsigned int pll2pfreq;
	unsigned int pll3vcofreq;
	unsigned int pll3freq;
	unsigned int pll3pfreq;
	/* the default max cpu rate could be supported */
	unsigned int df_max_cpurate;
	/* the plat rule for filter core ops */
	unsigned int (*is_cpuop_invalid_plt)(struct pxa988_cpu_opt *cop);
};

#define OP(p, d, a)		\
	{			\
		.pclk	= p,		\
		.dclk	= d,		\
		.aclk	= a,		\
		.vcore	= 0,		\
	}


#define debug_wt_reg(val, reg)		__raw_writel(val, reg)

/* current platform OP struct */
static struct platform_opt *cur_platform_opt;

static LIST_HEAD(core_op_list);

/* current core OP */
static struct pxa988_cpu_opt *cur_cpu_op;

/* current DDR/AXI OP */
static struct pxa988_ddr_axi_opt *cur_ddr_op;
static struct pxa988_ddr_axi_opt *cur_axi_op;

static unsigned int cpu_sel2_srcrate(enum ap_clk_sel ap_sel);
static unsigned int ddr_axi_sel2_srcrate(enum ddr_axi_clk_sel ddr_axi_sel);

static int set_volt(u32 vol);
static int get_volt(void);

static int is_1p5G_chip(void);

static unsigned int core_selected_max;

/*
 * used to record dummy boot up PP, define it here
 * as the default src and div value read from register
 * is NOT the real core and ddr,axi rate.
 * This default boot OP is from PP table.
 */
static struct pxa988_cpu_opt pxa988_cpu_bootop = {
	.pclk = 312,
	.pdclk = 78,
	.baclk = 78,
	.ap_clk_src = 624,
};

static struct pxa988_ddr_axi_opt pxa988_ddraxi_bootop = {
	.dclk = 104,
	.aclk = 104,
	.ddr_clk_src = 416,
	.axi_clk_src = 416,
};

static struct pxa988_cpu_opt pxa1088_op_array[] = {
	{
		.pclk = 156,
		.pdclk = 78,
		.baclk = 78,
		.ap_clk_sel = AP_CLK_SRC_PLL1_624,
	},
	{
		.pclk = 312,
		.pdclk = 156,
		.baclk = 156,
		.ap_clk_sel = AP_CLK_SRC_PLL1_624,
	},
	{
		.pclk = 624,
		.pdclk = 312,
		.baclk = 156,
		.ap_clk_sel = AP_CLK_SRC_PLL1_624,
	},
	{
		.pclk = 800,
		.pdclk = 400,
		.baclk = 200,
		.ap_clk_sel = AP_CLK_SRC_PLL2P,
	},
	{
		.pclk = 1066,
		.pdclk = 533,
		.baclk = 266,
		.ap_clk_sel = AP_CLK_SRC_PLL2P,
	},
	{
		.pclk = 1101,
		.pdclk = 550,
		.baclk = 275,
		.ap_clk_sel = AP_CLK_SRC_PLL3P,
	},
	{
		.pclk = 1183,
		.pdclk = 591,
		.baclk = 295,
		.ap_clk_sel = AP_CLK_SRC_PLL3P,
	},
#ifdef	CONFIG_CORE_1248
	{
		.pclk = 1248,
		.pdclk = 624,
		.baclk = 312,
		.ap_clk_sel = AP_CLK_SRC_PLL1_1248,
	},
#endif
	{
		.pclk = 1283,
		.pdclk = 641,
		.baclk = 320,
		.ap_clk_sel = AP_CLK_SRC_PLL3P,
	},
	{
		.pclk = 1482,
		.pdclk = 741,
		.baclk = 370,
		.ap_clk_sel = AP_CLK_SRC_PLL3P,
	},
};

/*
 * 1) Please don't select ddr from pll1 but axi from pll2
 * 2) FIXME: high ddr request means high axi is NOT
 * very reasonable
 */
static struct pxa988_ddr_axi_opt lpddr400_axi_oparray_1088[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 1,
		.aclk = 78,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 3,
		.aclk = 156,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 400,
		.ddr_tbl_index = 5,
		.aclk = 200,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
	},
};

static struct pxa988_ddr_axi_opt lpddr533_axi_oparray_1088[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 1,
		.aclk = 78,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 3,
		.aclk = 156,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 533,
		.ddr_tbl_index = 5,
		.aclk = 266,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
	},
};

static struct pxa988_ddr_axi_opt lpddr400_axi_oparray_1920[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 2,
		.aclk = 78,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 3,
		.aclk = 156,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 400,
		.ddr_tbl_index = 4,
		.aclk = 200,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
	},
};

static struct pxa988_ddr_axi_opt lpddr533_axi_oparray_1920[] = {
	{
		.dclk = 156,
		.ddr_tbl_index = 2,
		.aclk = 78,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 312,
		.ddr_tbl_index = 3,
		.aclk = 156,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 533,
		.ddr_tbl_index = 4,
		.aclk = 266,
		.ddr_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
		.axi_clk_sel = DDR_AXI_CLK_SRC_PLL2P,
	},
};
/*
 * bind relationship bewteen core and ddr&axi
 * all possible combination can be showed here
 * op 0~4 formal safe op
 * From op5 test op
 */
struct operating_point op_array_1088_lpddr400[] = {
	/* pclk dclk aclk vcc_core */
	OP(156, 156, 78),	/* op0 */
	OP(312, 156, 78),	/* op1 */
	OP(624, 312, 156),	/* op2 */
	OP(800, 400, 200),	/* op3 */
#ifdef CONFIG_CORE_1248
	OP(1248, 400, 200),	/* op4 */
#else
	OP(1101, 400, 200),	/* op4 */
	OP(1183, 400, 200),	/* op5 */
	OP(1283, 400, 200),	/* op6 */
	OP(1482, 400, 200),	/* op7 */
	/* Add PP for testing here */
	OP(624, 400, 200),	/* op4 */
	OP(800, 312, 156),	/* op5 */
	OP(1101, 312, 156),	/* op6 */
	OP(1183, 312, 156),	/* op7 */
	OP(1283, 312, 156),	/* op8 */
	OP(1482, 312, 156),	/* op9 */
#endif
};

struct operating_point op_array_1088_lpddr533[] = {
	/* pclk dclk aclk vcc_core */
	OP(156, 156, 78),	/* op0 */
	OP(312, 156, 78),	/* op1 */
	OP(624, 312, 156),	/* op2 */
	OP(1066, 533, 266),	/* op3 */
#ifdef CONFIG_CORE_1248
	OP(1248, 533, 266),	/* op4 */
#else
	OP(1101, 533, 266),	/* op4 */
	OP(1183, 533, 266),	/* op5 */
	OP(1283, 533, 266),	/* op6 */
	OP(1482, 533, 266),	/* op7 */
	/* Add PP for testing here */
	OP(624, 533, 266),	/* op4 */
	OP(1066, 312, 156),	/* op5 */
	OP(1101, 312, 156),	/* op6 */
	OP(1183, 312, 156),	/* op7 */
	OP(1283, 312, 156),	/* op8 */
	OP(1482, 312, 156),	/* op9 */
#endif
};

/*
 * core and ddr opt combination vary from platform to platform
 * LPDDR400, pll2vco 1600M, pll2 800M(ddr,core), pll2p 533M(peripheral)
 * LPDDR533 pll2vco 2132M, pll2 1066M(ddr,core), pll2p 1066M(peripheral)
 * pll3vco 2000M, pll3 500M(dsi), pll3p 1000M(cpu)
 */
static struct platform_opt platform_op_arrays[] = {
	{
		.cpuid = 0x8000,
		.chipid = 0xa01088,
		.ddrtype = LPDDR2_400M,
		.cpu_name = "PXA1088",
		.op_array = op_array_1088_lpddr400,
		.com_op_size = ARRAY_SIZE(op_array_1088_lpddr400),
		.cpu_opt = pxa1088_op_array,
		.cpu_opt_size = ARRAY_SIZE(pxa1088_op_array),
		.ddr_axi_opt = lpddr400_axi_oparray_1088,
		.ddr_axi_opt_size = ARRAY_SIZE(lpddr400_axi_oparray_1088),
		.pll2vcofreq = 1600,
		.pll2freq = 800,
		.pll2pfreq = 800,
		.pll3vcofreq = 2366,
		.pll3freq = 1183,
		.pll3pfreq = 1183,
		.df_max_cpurate = 1183,
	},
	{
		.cpuid = 0x8000,
		.chipid = 0xa01088,
		.ddrtype = LPDDR2_533M,
		.cpu_name = "PXA1088",
		.op_array = op_array_1088_lpddr533,
		.com_op_size = ARRAY_SIZE(op_array_1088_lpddr533),
		.cpu_opt = pxa1088_op_array,
		.cpu_opt_size = ARRAY_SIZE(pxa1088_op_array),
		.ddr_axi_opt = lpddr533_axi_oparray_1088,
		.ddr_axi_opt_size = ARRAY_SIZE(lpddr533_axi_oparray_1088),
		.pll2vcofreq = 2132,
		.pll2freq = 1066,
		.pll2pfreq = 1066,
		.pll3vcofreq = 2366,
		.pll3freq = 1183,
		.pll3pfreq = 1183,
		.df_max_cpurate = 1183,
	},
	{
		.cpuid = 0x8000,
		.chipid = 0xf01188,
		.ddrtype = LPDDR2_400M,
		.cpu_name = "PXA1L88",
		.op_array = op_array_1088_lpddr400,
		.com_op_size = ARRAY_SIZE(op_array_1088_lpddr400),
		.cpu_opt = pxa1088_op_array,
		.cpu_opt_size = ARRAY_SIZE(pxa1088_op_array),
		.ddr_axi_opt = lpddr400_axi_oparray_1920,
		.ddr_axi_opt_size = ARRAY_SIZE(lpddr400_axi_oparray_1920),
		.pll2vcofreq = 1600,
		.pll2freq = 800,
		.pll2pfreq = 800,
		.pll3vcofreq = 2366,
		.pll3freq = 1183,
		.pll3pfreq = 1183,
		.df_max_cpurate = 1183,
	},
	{
		.cpuid = 0x8000,
		.chipid = 0xf01188,
		.ddrtype = LPDDR2_533M,
		.cpu_name = "PXA1L88",
		.op_array = op_array_1088_lpddr533,
		.com_op_size = ARRAY_SIZE(op_array_1088_lpddr533),
		.cpu_opt = pxa1088_op_array,
		.cpu_opt_size = ARRAY_SIZE(pxa1088_op_array),
		.ddr_axi_opt = lpddr533_axi_oparray_1920,
		.ddr_axi_opt_size = ARRAY_SIZE(lpddr533_axi_oparray_1920),
		.pll2vcofreq = 2132,
		.pll2freq = 710,
		.pll2pfreq = 1066,
		.pll3vcofreq = 2366,
		.pll3freq = 1183,
		.pll3pfreq = 1183,
		.df_max_cpurate = 1183,
	},
};

enum {
	VL0 = 0,
	VL1,
	VL2,
	VL3,
	VL4,
	VL_MAX,
};

enum {
	CORE = 0,
	DDR_AXI,
	VM_RAIL_MAX,
};

/* uboot vmin must support CP/MSA requirement */
static int vm_mv_1088a0_svc_1p2G[][VL_MAX] = {
	{1000, 1075, 1300, 1375},	/* profile 0 */
	{1000, 1075, 1088, 1200},	/* profile 1 */
	{1000, 1075, 1088, 1200},	/* profile 2 */
	{1000, 1075, 1125, 1238},	/* profile 3 */
	{1000, 1075, 1163, 1288},	/* profile 4 */
	{1000, 1075, 1200, 1325},	/* profile 5 */
	{1000, 1075, 1238, 1363},	/* profile 6 */
	{1000, 1075, 1275, 1375},	/* profile 7 */
	{1000, 1075, 1300, 1375},	/* profile 8 */
};

static int vm_mv_1088a1_svc_1p2G[][VL_MAX] = {
	{1000, 1075, 1250, 1375},	/* profile 0 */
	{1000, 1075, 1088, 1200},	/* profile 1 */
	{1000, 1075, 1088, 1200},	/* profile 2 */
	{1000, 1075, 1088, 1238},	/* profile 3 */
	{1000, 1075, 1125, 1288},	/* profile 4 */
	{1000, 1075, 1150, 1325},	/* profile 5 */
	{1000, 1075, 1188, 1363},	/* profile 6 */
	{1000, 1075, 1225, 1375},	/* profile 7 */
	{1000, 1075, 1250, 1375},	/* profile 8 */
};

static int vm_mv_1088a1_svc_1p25G[][VL_MAX] = {
	{1000, 1075, 1250, 1375},	/* profile 0 */
	{1000, 1075, 1088, 1225},	/* profile 1 */
	{1000, 1075, 1088, 1225},	/* profile 2 */
	{1000, 1075, 1088, 1263},	/* profile 3 */
	{1000, 1075, 1125, 1313},	/* profile 4 */
	{1000, 1075, 1150, 1350},	/* profile 5 */
	{1000, 1075, 1188, 1375},	/* profile 6 */
	{1000, 1075, 1225, 1375},	/* profile 7 */
	{1000, 1075, 1250, 1375},	/* profile 8 */
};

static int vm_mv_1L88a0_svc[][VL_MAX] = {
	{1113, 1225, 1300, 1375},	/* profile 0 */
	{1050, 1100, 1175, 1175},	/* profile 1 */
	{1050, 1100, 1175, 1175},	/* profile 2 */
	{1050, 1100, 1175, 1175},	/* profile 3 */
	{1050, 1100, 1175, 1175},	/* profile 4 */
	{1050, 1100, 1175, 1213},	/* profile 5 */
	{1050, 1100, 1175, 1250},	/* profile 6 */
	{1100, 1125, 1200, 1288},	/* profile 7 */
	{1100, 1150, 1225, 1325},	/* profile 8 */
	{1100, 1175, 1250, 1350},	/* profile 9 */
	{1100, 1200, 1275, 1375},	/* profile 10 */
	{1113, 1225, 1300, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0_svc_1p25G[][VL_MAX] = {
	{1113, 1225, 1300, 1375},	/* profile 0 */
	{1050, 1100, 1175, 1200},	/* profile 1 */
	{1050, 1100, 1175, 1200},	/* profile 2 */
	{1050, 1100, 1175, 1200},	/* profile 3 */
	{1050, 1100, 1175, 1200},	/* profile 4 */
	{1050, 1100, 1175, 1238},	/* profile 5 */
	{1050, 1100, 1175, 1275},	/* profile 6 */
	{1100, 1125, 1200, 1313},	/* profile 7 */
	{1100, 1150, 1225, 1350},	/* profile 8 */
	{1100, 1175, 1250, 1375},	/* profile 9 */
	{1100, 1200, 1275, 1375},	/* profile 10 */
	{1113, 1225, 1300, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0c_svc_1p2G[][VL_MAX] = {
	{1113, 1225, 1275, 1375},	/* profile 0 */
	{1050, 1100, 1175, 1175},	/* profile 1 */
	{1050, 1100, 1175, 1175},	/* profile 2 */
	{1050, 1100, 1175, 1175},	/* profile 3 */
	{1050, 1100, 1175, 1188},	/* profile 4 */
	{1050, 1100, 1175, 1200},	/* profile 5 */
	{1100, 1113, 1175, 1238},	/* profile 6 */
	{1100, 1138, 1188, 1275},	/* profile 7 */
	{1100, 1150, 1213, 1313},	/* profile 8 */
	{1100, 1175, 1225, 1350},	/* profile 9 */
	{1100, 1200, 1250, 1375},	/* profile 10 */
	{1113, 1225, 1275, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0c_umc_svc_1p2G[][VL_MAX] = {
	{1025, 1225, 1275, 1375},	/* profile 0 */
	{1025, 1100, 1175, 1175},	/* profile 1 */
	{1025, 1100, 1175, 1175},	/* profile 2 */
	{1025, 1100, 1175, 1175},	/* profile 3 */
	{1025, 1100, 1175, 1188},	/* profile 4 */
	{1025, 1100, 1175, 1213},	/* profile 5 */
	{1025, 1100, 1175, 1250},	/* profile 6 */
	{1025, 1138, 1188, 1275},	/* profile 7 */
	{1025, 1150, 1213, 1313},	/* profile 8 */
	{1025, 1175, 1225, 1350},	/* profile 9 */
	{1025, 1200, 1250, 1375},	/* profile 10 */
	{1025, 1225, 1275, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0c_svc_1p25G[][VL_MAX] = {
	{1113, 1225, 1275, 1375},	/* profile 0 */
	{1050, 1100, 1175, 1188},	/* profile 1 */
	{1050, 1100, 1175, 1188},	/* profile 2 */
	{1050, 1100, 1175, 1188},	/* profile 3 */
	{1050, 1100, 1175, 1213},	/* profile 4 */
	{1050, 1100, 1175, 1225},	/* profile 5 */
	{1100, 1113, 1175, 1263},	/* profile 6 */
	{1100, 1138, 1188, 1300},	/* profile 7 */
	{1100, 1150, 1213, 1338},	/* profile 8 */
	{1100, 1175, 1225, 1363},	/* profile 9 */
	{1100, 1200, 1250, 1375},	/* profile 10 */
	{1113, 1225, 1275, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0c_umc_svc_1p25G[][VL_MAX] = {
	{1025, 1225, 1275, 1375},	/* profile 0 */
	{1025, 1100, 1175, 1188},	/* profile 1 */
	{1025, 1100, 1175, 1188},	/* profile 2 */
	{1025, 1100, 1175, 1188},	/* profile 3 */
	{1025, 1100, 1175, 1213},	/* profile 4 */
	{1025, 1100, 1175, 1225},	/* profile 5 */
	{1025, 1100, 1175, 1263},	/* profile 6 */
	{1025, 1138, 1188, 1300},	/* profile 7 */
	{1025, 1150, 1213, 1338},	/* profile 8 */
	{1025, 1175, 1225, 1363},	/* profile 9 */
	{1025, 1200, 1250, 1375},	/* profile 10 */
	{1025, 1225, 1275, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0c_svc_1p5G[][VL_MAX] = {
	{1113, 1225, 1275, 1375},	/* profile 0 */
	{1050, 1100, 1175, 1275},	/* profile 1 */
	{1050, 1100, 1175, 1275},	/* profile 2 */
	{1050, 1100, 1175, 1300},	/* profile 3 */
	{1050, 1100, 1175, 1325},	/* profile 4 */
	{1050, 1100, 1175, 1350},	/* profile 5 */
	{1100, 1113, 1175, 1375},	/* profile 6 */
	{1100, 1138, 1188, 1375},	/* profile 7 */
	{1100, 1150, 1213, 1375},	/* profile 8 */
	{1100, 1175, 1225, 1375},	/* profile 9 */
	{1100, 1200, 1250, 1375},	/* profile 10 */
	{1113, 1225, 1275, 1375},	/* profile 11 */
};

static int vm_mv_1L88a0c_umc_svc_1p5G[][VL_MAX] = {
	{1025, 1225, 1275, 1375},	/* profile 0 */
	{1025, 1100, 1175, 1275},	/* profile 1 */
	{1025, 1100, 1175, 1300},	/* profile 2 */
	{1025, 1100, 1175, 1325},	/* profile 3 */
	{1025, 1100, 1175, 1350},	/* profile 4 */
	{1025, 1100, 1175, 1375},	/* profile 5 */
	{1025, 1100, 1175, 1375},	/* profile 6 */
	{1025, 1138, 1188, 1375},	/* profile 7 */
	{1025, 1150, 1213, 1375},	/* profile 8 */
	{1025, 1175, 1225, 1375},	/* profile 9 */
	{1025, 1200, 1250, 1375},	/* profile 10 */
	{1025, 1225, 1275, 1375},	/* profile 11 */
};

static unsigned int freqs_cmb_1088a0[VM_RAIL_MAX][VL_MAX] = {
	{ 312, 312, 800, 1300 },	/* CORE */
	{ 156, 156, 312, 533 },		/* DDR/AXI */
};

static unsigned int freqs_cmb_1088a1[VM_RAIL_MAX][VL_MAX] = {
	{ 312, 312, 800, 1482 },	/* CORE */
	{ 312, 312, 312, 533 },		/* DDR/AXI */
};

static unsigned int freqs_cmb_1L88[VM_RAIL_MAX][VL_MAX] = {
	{ 312, 624, 800, 1482 },	/* CORE */
	{ 312, 312, 312, 533 },	/* DDR/AXI */
};


/* PLL post divider table */
static struct pll_post_div pll_post_div_tbl[] = {
	/* divider, reg vaule */
	{1, 0},
	{2, 2},
	{3, 4},
	{4, 5},
	{6, 7},
	{8, 8},
};

static unsigned int __clk_pll_vco2intpi(unsigned long vco)
{
	unsigned int intpi;

	if (vco >= 2400 && vco <= 2500)
		intpi = 6;
	else if (vco >= 2100)
		intpi = 5;
	else if (vco >= 1800)
		intpi = 4;
	else if (vco >= 1500)
		intpi = 3;
	else if (vco >= 1200)
		intpi = 2;
	else
		BUG_ON("Unsupported vco frequency for INTPI!\n");

	return intpi;
}

/*
 * Real amplitude percentage = amplitude / base
*/
static void __clk_pll_get_divrng(enum ssc_mode mode, unsigned long rate,
				 unsigned int amplitude, unsigned int base,
				 unsigned long vco, unsigned int *div,
				 unsigned int *rng)
{
	if (amplitude > (25 * base / 1000))
		BUG_ON("Amplitude can't exceed 2.5\%\n");
	switch (mode) {
	case CENTER_SPREAD:
		*div = (vco / rate) >> 2;
		break;
	case DOWN_SPREAD:
		*div = (vco / rate) >> 1;
		break;
	default:
		printf("Unsupported SSC MODE!\n");
		return;
	}
	if (*div == 0)
		*div = 1;
	*rng = (1 << 29) / (*div * base / amplitude);
}

static void enable_pll2_ssc(enum ssc_mode mode, unsigned int div,
			    unsigned int rng)
{
	union apb_pllx_pi_ctrl pll2_pi_ctrl;
	union apb_pllx_ssc_ctrl pll2_ssc_ctrl;
	unsigned int intpi = __clk_pll_vco2intpi(cur_platform_opt->pll2vcofreq);
	pll2_pi_ctrl.v = __raw_readl(APB_PLL2_PI_CTRL);
	pll2_pi_ctrl.b.pi_en = 1;
	pll2_pi_ctrl.b.ssc_clk_en = 1;
	pll2_pi_ctrl.b.clk_det_en = 1;
	pll2_pi_ctrl.b.reset_ext = 1;
	pll2_pi_ctrl.b.intpi = intpi;
	pll2_pi_ctrl.b.sel_vco_diff = 0;
	pll2_pi_ctrl.b.sel_vco_se = 0;
	__raw_writel(pll2_pi_ctrl.v, APB_PLL2_PI_CTRL);
	udelay(1);
	pll2_pi_ctrl.b.reset_ext = 0;
	__raw_writel(pll2_pi_ctrl.v, APB_PLL2_PI_CTRL);

	pll2_ssc_ctrl.v = __raw_readl(APB_PLL2_SSC_CTRL);
	pll2_ssc_ctrl.b.ssc_en = 1;
	pll2_ssc_ctrl.b.ssc_mode = mode;
	pll2_ssc_ctrl.b.ssc_freq_div = div;
	pll2_ssc_ctrl.b.ssc_rnge = rng;
	__raw_writel(pll2_ssc_ctrl.v, APB_PLL2_SSC_CTRL);
}

static void disable_pll2_ssc(void)
{
	union apb_pllx_pi_ctrl pll2_pi_ctrl;
	union apb_pllx_ssc_ctrl pll2_ssc_ctrl;
	pll2_pi_ctrl.v = __raw_readl(APB_PLL2_PI_CTRL);
	pll2_pi_ctrl.b.pi_en = 0;
	pll2_pi_ctrl.b.ssc_clk_en = 0;
	pll2_pi_ctrl.b.clk_det_en = 0;
	pll2_pi_ctrl.b.sel_vco_diff = 1;
	pll2_pi_ctrl.b.sel_vco_se = 1;
	__raw_writel(pll2_pi_ctrl.v, APB_PLL2_PI_CTRL);
	pll2_ssc_ctrl.v = __raw_readl(APB_PLL2_SSC_CTRL);
	pll2_ssc_ctrl.b.ssc_en = 0;
	__raw_writel(pll2_ssc_ctrl.v, APB_PLL2_SSC_CTRL);
}

static void enable_pll3_ssc(enum ssc_mode mode, unsigned int div,
			    unsigned int rng)
{
	union apb_pllx_pi_ctrl pll3_pi_ctrl;
	union apb_pllx_ssc_ctrl pll3_ssc_ctrl;
	unsigned int intpi = __clk_pll_vco2intpi(cur_platform_opt->pll3vcofreq);

	pll3_pi_ctrl.v = __raw_readl(APB_PLL3_PI_CTRL);
	pll3_pi_ctrl.b.pi_en = 1;
	pll3_pi_ctrl.b.ssc_clk_en = 1;
	pll3_pi_ctrl.b.clk_det_en = 1;
	pll3_pi_ctrl.b.reset_ext = 1;
	pll3_pi_ctrl.b.intpi = intpi;
	pll3_pi_ctrl.b.sel_vco_diff = 0;
	pll3_pi_ctrl.b.sel_vco_se = 0;
	__raw_writel(pll3_pi_ctrl.v, APB_PLL3_PI_CTRL);
	udelay(1);
	pll3_pi_ctrl.b.reset_ext = 0;
	__raw_writel(pll3_pi_ctrl.v, APB_PLL3_PI_CTRL);

	pll3_ssc_ctrl.v = __raw_readl(APB_PLL3_SSC_CTRL);
	pll3_ssc_ctrl.b.ssc_en = 1;
	pll3_ssc_ctrl.b.ssc_mode = mode;
	pll3_ssc_ctrl.b.ssc_freq_div = div;
	pll3_ssc_ctrl.b.ssc_rnge = rng;
	__raw_writel(pll3_ssc_ctrl.v, APB_PLL3_SSC_CTRL);
}

static void disable_pll3_ssc(void)
{
	union apb_pllx_pi_ctrl pll3_pi_ctrl;
	union apb_pllx_ssc_ctrl pll3_ssc_ctrl;
	pll3_pi_ctrl.v = __raw_readl(APB_PLL3_PI_CTRL);
	pll3_pi_ctrl.b.pi_en = 0;
	pll3_pi_ctrl.b.ssc_clk_en = 0;
	pll3_pi_ctrl.b.clk_det_en = 0;
	pll3_pi_ctrl.b.sel_vco_diff = 1;
	pll3_pi_ctrl.b.sel_vco_se = 1;
	__raw_writel(pll3_pi_ctrl.v, APB_PLL3_PI_CTRL);
	pll3_ssc_ctrl.v = __raw_readl(APB_PLL3_SSC_CTRL);
	pll3_ssc_ctrl.b.ssc_en = 0;
	__raw_writel(pll3_ssc_ctrl.v, APB_PLL3_SSC_CTRL);
}

#ifdef CONFIG_FINE_TUNED_SVC
static unsigned int convert_fuses2profile(unsigned int ui_fuses)
{
	unsigned int ui_profile = 0;
	unsigned int ui_temp = 3, ui_temp2 = 1;
	unsigned int i;

	for (i = 0; i < PROFILE_NUM; i++) {
		ui_temp |= ui_temp2 << (i + 1);
		if (ui_temp == ui_fuses)
			ui_profile = i + 1;
	}

	return ui_profile;
}
#else
static unsigned int convert_fuses2profile(unsigned int ui_fuses)
{
	unsigned int ui_profile = 0;
	unsigned int ui_temp = 3, ui_temp2 = 3;
	unsigned int i;

	for (i = 0; i < PROFILE_NUM; i++) {
		ui_temp |= ui_temp2 << (i * 2);
		if (ui_temp == ui_fuses)
			ui_profile = i + 1;
	}

	return ui_profile;
}
#endif

#ifdef CONFIG_TZ_HYPERVISOR
static int chip_profile;
static unsigned int chip_foundry;
static int cpu_version;
static int chip_support_1p5g;
void read_fuse_info(void)
{
	int ret = -1;
	unsigned int ui_fuses;
	tzlc_cmd_desc cmd_desc;
	tzlc_handle tzlc_hdl;
	unsigned int ui_main_fuse_95_64 = 0;

	/*
	 * with TrustZone enabled, fuse related info should be read in
	 * secure world.
	 */
	tzlc_hdl = pxa_tzlc_create_handle();

	memset(&cmd_desc, 0, sizeof(tzlc_cmd_desc));
	cmd_desc.op = TZLC_CMD_READ_MANUFACTURING_BITS;
	ret = pxa_tzlc_cmd_op(tzlc_hdl, &cmd_desc);

	if (ret == 0) {
		ui_fuses = cmd_desc.args[0];
		ui_main_fuse_95_64 = cmd_desc.args[1];
	} else {
		printf("error: fail to read fuse - manu. bits\n");
		return;
	}

	pxa_tzlc_destroy_handle(tzlc_hdl);

	/* bit 237 ~ 239 for foundry information */
	chip_foundry = (ui_fuses >> (237 - 224)) & 0x7;
	ui_fuses = (ui_fuses >> 16) & 0x0000FFFF;
	if (ui_fuses)
		chip_profile =
			convert_fuses2profile(ui_fuses);
	printf("ui_chip_profile = %d\n", chip_profile);
	printf("ui_chip_foundry = %d\n", chip_foundry);

	ui_fuses = cmd_desc.args[0];
	cpu_version = ((ui_fuses >> 14) & 0x3) ? 1 : 0;
	if (cpu_version && (ui_main_fuse_95_64 & (0x1 << 3)))
		cpu_version = 2;

	if (cpu_is_pxa1L88() && ((ui_main_fuse_95_64 >> 26) & 0x3))
		chip_support_1p5g = 1;
}

/* read cpu profile from FUSE */
static int get_profile(void)
{
	return chip_profile;
}

/* read cpu foundry from FUSE */
static unsigned int get_foundry(void)
{
	return chip_foundry;
}

/* read cpu version from FUSE. 0: A0a, 1: A0b, 2: A0c */
int get_cpu_version(void)
{
	return cpu_version;
}

/* check 1.5G chip from FUSE. 0: 1.2G, 1: 1.5G */
static int is_1p5G_chip(void)
{
	return chip_support_1p5g;
}
#else
/* read cpu profile from FUSE */
static int get_profile(void)
{
	unsigned int ui_dro_avg, ui_chip_profile = 0;
	unsigned int ui_main_fuse_95_64 = 0;
	unsigned int ui_fuses = 0;
	ui_main_fuse_95_64 = __raw_readl(UIMAINFUSE_95_64);
	ui_dro_avg = (ui_main_fuse_95_64 >>  4) & 0x3ff;
	ui_fuses = __raw_readl(BLOCK0_224_255);
	/* bit 240 ~ 255 for Profile information */
	ui_fuses = (ui_fuses >> 16) & 0x0000FFFF;
	if (ui_fuses)
		ui_chip_profile =
			convert_fuses2profile(ui_fuses);
	printf("ui_dro_avg = %u\n", ui_dro_avg);
	printf("ui_chip_profile = %d\n", ui_chip_profile);
	return ui_chip_profile;
}

/* read cpu foundry from FUSE */
static unsigned int get_foundry(void)
{
	unsigned int ui_fuses = 0, ui_chip_foundry = 0;

	ui_fuses = __raw_readl(BLOCK0_224_255);
	/* bit 237 ~ 239 for foundry information */
	ui_chip_foundry = (ui_fuses >> (237 - 224)) & 0x7;
	return ui_chip_foundry;
}

/* read cpu version from FUSE. 0: A0a, 1: A0b, 2: A0c */
int get_cpu_version(void)
{
	unsigned int ui_main_fuse_95_64 = 0;
	unsigned int ui_fuses = 0;
	int cpu_version;
	ui_main_fuse_95_64 = __raw_readl(UIMAINFUSE_95_64);
	ui_fuses = __raw_readl(BLOCK0_224_255);
	cpu_version = ((ui_fuses >> 14) & 0x3) ? 1 : 0;
	if (cpu_version && (ui_main_fuse_95_64 & (0x1 << 3)))
		cpu_version = 2;
	return cpu_version;
}

/* check 1.5G chip from FUSE. 0: 1.2G, 1: 1.5G */
static int is_1p5G_chip(void)
{
	unsigned int ui_main_fuse_95_64 = 0;
	ui_main_fuse_95_64 = __raw_readl(UIMAINFUSE_95_64);
	if (cpu_is_pxa1L88() && ((ui_main_fuse_95_64 >> 26) & 0x3))
		return 1;
	return 0;
}
#endif

/* convert post div reg setting to divider val */
	static unsigned int
__attribute__ ((unused)) __pll_divsel2div(unsigned int divselval)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pll_post_div_tbl); i++) {
		if (divselval == pll_post_div_tbl[i].divselval)
			return pll_post_div_tbl[i].div;
	}
	BUG_ON(i == ARRAY_SIZE(pll_post_div_tbl));
	return 0;
}

/* PLL range 1.2G~2.5G, vco_vrng = kvco */
static void __clk_pll_rate2rng(unsigned long rate, unsigned int *kvco,
			       unsigned int *vco_rng)
{
	if (rate >= 2400 && rate <= 2500)
		*kvco = 7;
	else if (rate >= 2150)
		*kvco = 6;
	else if (rate >= 1950)
		*kvco = 5;
	else if (rate >= 1750)
		*kvco = 4;
	else if (rate >= 1550)
		*kvco = 3;
	else if (rate >= 1350)
		*kvco = 2;
	else if (rate >= 1200)
		*kvco = 1;
	else
		printf("%s rate %lu out of range!\n", __func__, rate);

	*vco_rng = *kvco;
}

/* rate unit Mhz */
static unsigned int __clk_pll_calc_div(unsigned long rate,
	unsigned long parent_rate, unsigned int *div)
{
	unsigned int i;
	*div = 0;

	for (i = 0; i < ARRAY_SIZE(pll_post_div_tbl); i++) {
		if (rate == (parent_rate / pll_post_div_tbl[i].div)) {
			*div = pll_post_div_tbl[i].div;
			return pll_post_div_tbl[i].divselval;
		}
	}
	BUG_ON(i == ARRAY_SIZE(pll_post_div_tbl));
	return 0;
}

static u32 pll2_is_enabled(void)
{
	union pmum_pll2cr pll2cr;
	pll2cr.v = __raw_readl(MPMU_PLL2CR);

	/* ctrl = 0(hw enable) or ctrl = 1&&en = 1(sw enable) */
	/* ctrl = 1&&en = 0(sw disable) */
	if (pll2cr.b.ctrl && (!pll2cr.b.en))
		return 0;
	else
		return 1;
}

/*
 * Pll2 has two frequency output, pll2 and pll2p
 * pll2 and pll2p is divided from pll2 vco
 * frequency unit Mhz
 */
static void set_pll2_freq(u32 pll2vco_freq, u32 pll2_freq, u32 pll2p_freq)
{
	union pmum_pll2cr pll2cr;
	union apb_spare_pllswcr pll2_sw_ctrl;
	union pmum_posr pll2posr;
	u32 kvco, vcovnrg;
	u32 divselse, divseldiff, pll2_div, pll2p_div;

	/* do nothing if pll2 is enabled  */
	if (pll2_is_enabled()) {
		printf("Disable Pll2 before change its rate\n");
		return;
	}

	/* calc vco rate range */
	__clk_pll_rate2rng(pll2vco_freq, &kvco, &vcovnrg);

	/* calc pll2 and pll2p divider */
	divselse = __clk_pll_calc_div(pll2_freq, pll2vco_freq, &pll2_div);
	divseldiff = __clk_pll_calc_div(pll2p_freq, pll2vco_freq, &pll2p_div);

	/* pll2 SW ctrl setting */
	pll2_sw_ctrl.v = __raw_readl(APB_SPARE_PLL2CR);
	pll2_sw_ctrl.b.vddm = 1;
	pll2_sw_ctrl.b.vddl = 9;
	pll2_sw_ctrl.b.vreg_ivreg = 2;
	pll2_sw_ctrl.b.icp = 4;
	pll2_sw_ctrl.b.ctune = 1;
	pll2_sw_ctrl.b.bypassen = 0;
	pll2_sw_ctrl.b.gatectl = 0;
	pll2_sw_ctrl.b.lineupen = 0;
	pll2_sw_ctrl.b.diffclken = 1;
	pll2_sw_ctrl.b.kvco = kvco;
	pll2_sw_ctrl.b.vcovnrg = vcovnrg;
	pll2_sw_ctrl.b.divselse = divselse;
	pll2_sw_ctrl.b.divseldiff = divseldiff;
	debug_wt_reg(pll2_sw_ctrl.v, APB_SPARE_PLL2CR);

	/* Refclk/Refdiv = pll2freq/Fbdiv, Refclk = 26M */
	pll2cr.v = 0;
	pll2cr.b.pll2refd = 3;
	pll2cr.b.pll2fbd = pll2vco_freq * pll2cr.b.pll2refd / 26;
	/* we must lock refd/fbd first before enabling PLL2 */
	pll2cr.b.ctrl = 1;
	debug_wt_reg(pll2cr.v, MPMU_PLL2CR);
	pll2cr.b.ctrl = 0;	/* Let HW control PLL2 */
	debug_wt_reg(pll2cr.v, MPMU_PLL2CR);

	udelay(100);
	pll2posr.v = __raw_readl(MPMU_POSR);
	printf("PLL2_VCO enable@%dMhz, PLL2@%dMhz, PLL2P@%dMhz\nSWCR[w%x,r%x] PLLCR[w%x,r%x] PLL2 shadow refd%d, fbd%d\n",
	       pll2vco_freq, pll2vco_freq / pll2_div,
	       pll2vco_freq / pll2p_div,
	       pll2_sw_ctrl.v, __raw_readl(APB_SPARE_PLL2CR),
	       pll2cr.v, __raw_readl(MPMU_PLL2CR),
	       pll2posr.b.pll2refd, pll2posr.b.pll2fbd);

	return;
}

/* frequency unit Mhz, return pll2 vco freq */
static u32 __attribute__ ((unused)) get_pll2_freq(u32 *pll2_freq,
						  u32 *pll2p_freq)
{
	union pmum_pll2cr pll2cr;
	union apb_spare_pllswcr pll2_sw_ctl;
	u32 pll2_vco, pll2_div, pll2p_div, pll2refd;

	/* return 0 if pll2 is disabled(ctrl = 1, en = 0) */
	if (!pll2_is_enabled()) {
		printf("%s PLL2 is not enabled!\n", __func__);
		*pll2_freq = 0;
		*pll2p_freq = 0;
		return 0;
	}

	pll2cr.v = __raw_readl(MPMU_PLL2CR);
	pll2refd = pll2cr.b.pll2refd;
	BUG_ON(pll2refd == 1);

	if (pll2refd == 0)
		pll2refd = 1;
	pll2_vco = 26 * pll2cr.b.pll2fbd / pll2refd;

	pll2_sw_ctl.v = __raw_readl(APB_SPARE_PLL2CR);
	pll2_div = __pll_divsel2div(pll2_sw_ctl.b.divselse);
	pll2p_div = __pll_divsel2div(pll2_sw_ctl.b.divseldiff);
	*pll2_freq = pll2_vco / pll2_div;
	*pll2p_freq = pll2_vco / pll2p_div;

	return pll2_vco;
}

/*
 * 1. Whenever PLL2 is enabled, ensure it's set as HW activation.
 * 2. When PLL2 is disabled (no one uses PLL2 as source),
 * set it as SW activation.
 */
static int pll2_refcnt;
void turn_off_pll2(void)
{
	union pmum_pll2cr pll2cr;

	pll2_refcnt--;
	if (pll2_refcnt < 0)
		printf("unmatched pll2_refcnt\n");
	if (pll2_refcnt == 0) {
		pll2cr.v = __raw_readl(MPMU_PLL2CR);
		pll2cr.b.ctrl = 1;	/* Let SW control PLL2 */
		pll2cr.b.en = 0;	/* disable PLL2 by en bit */
		__raw_writel(pll2cr.v, MPMU_PLL2CR);
		printf("Disable pll2 as it is not used!\n");
		if (has_feat_ssc())
			disable_pll2_ssc();
	}
}

/* must be called after __init_platform_opt */
void turn_on_pll2(void)
{
	u32 pll2vco, pll2, pll2p;
	unsigned int div, rng;

	BUG_ON(!cur_platform_opt ||
	       !cur_platform_opt->pll2vcofreq);

	pll2_refcnt++;
	if (pll2_refcnt == 1) {
		pll2vco = cur_platform_opt->pll2vcofreq;
		pll2 = cur_platform_opt->pll2freq;
		pll2p = cur_platform_opt->pll2pfreq;
		set_pll2_freq(pll2vco, pll2, pll2p);
		if (has_feat_ssc()) {
			__clk_pll_get_divrng(DOWN_SPREAD, 30000, 25, 1000,
					     pll2vco * MHZ, &div, &rng);
			enable_pll2_ssc(DOWN_SPREAD, div, rng);
		}
	}
}

static u32 pll3_is_enabled(void)
{
	union pmum_pll3cr pll3cr;
	pll3cr.v = __raw_readl(MPMU_PLL3CR);

	/*
	 * PLL3CR[19:18] = 0x1, 0x2, 0x3 means PLL3 is enabled.
	 * PLL3CR[19:18] = 0x0 means PLL3 is disabled
	 */
	if ((!pll3cr.b.pll3_pu) && (!pll3cr.b.pclk_1248_sel))
		return 0;
	else
		return 1;
}

/*
 * 1. Pll3 has two frequency output, pll3 and pll3p
 * 2. pll3 and pll3p is divided from pll3 vco, frequency unit Mhz
 * 3. SW always sets PLL3CR[19:18] = 0x3 to enable PLL3,
 * PLL3CR[19:18] = 0x0 to disable PLL3
 * 4. frequency unit Mhz
 */
void set_pll3_freq(u32 pll3vco_freq, u32 pll3_freq, u32 pll3p_freq)
{
	union pmum_pll3cr pll3cr;
	union apb_spare_pllswcr pll3_sw_ctrl;
	u32 kvco, vcovnrg;
	u32 divselse, divseldiff, pll3_div, pll3p_div;

	/* do nothing if pll3 is enabled  */
	if (pll3_is_enabled()) {
		printf("Disable Pll3 before change its rate\n");
		return;
	}

	/* calc vco rate range */
	__clk_pll_rate2rng(pll3vco_freq, &kvco, &vcovnrg);

	/* calc pll3 and pll3p divider */
	divselse = __clk_pll_calc_div(pll3_freq, pll3vco_freq, &pll3_div);
	divseldiff = __clk_pll_calc_div(pll3p_freq, pll3vco_freq, &pll3p_div);

	/* pll3 SW ctrl setting */
	pll3_sw_ctrl.v = __raw_readl(APB_SPARE_PLL3CR);
	pll3_sw_ctrl.b.vddm = 1;
	pll3_sw_ctrl.b.vddl = 9;
	pll3_sw_ctrl.b.vreg_ivreg = 2;
	pll3_sw_ctrl.b.icp = 4;
	pll3_sw_ctrl.b.ctune = 1;
	pll3_sw_ctrl.b.bypassen = 0;
	pll3_sw_ctrl.b.gatectl = 0;
	pll3_sw_ctrl.b.lineupen = 0;
	pll3_sw_ctrl.b.diffclken = 1;
	pll3_sw_ctrl.b.kvco = kvco;
	pll3_sw_ctrl.b.vcovnrg = vcovnrg;
	pll3_sw_ctrl.b.divselse = divselse;
	pll3_sw_ctrl.b.divseldiff = divseldiff;
	__raw_writel(pll3_sw_ctrl.v, APB_SPARE_PLL3CR);

	/* Refclk/Refdiv = pllvcofreq/Fbdiv, Refclk = 26M */
	pll3cr.v = __raw_readl(MPMU_PLL3CR);
	pll3cr.b.pll3refd = 3;
	pll3cr.b.pll3fbd = pll3vco_freq * pll3cr.b.pll3refd / 26;
	pll3cr.b.pll3_pu = 1;
	__raw_writel(pll3cr.v, MPMU_PLL3CR);

	udelay(100);
	printf("PLL3_VCO enable@%dMhz, PLL3@%dMhz, PLL3P@%dMhz\n"
	       "SWCR[w%x,r%x] PLLCR[w%x,r%x]\n",
	       pll3vco_freq, pll3vco_freq / pll3_div,
	       pll3vco_freq / pll3p_div,
	       pll3_sw_ctrl.v, __raw_readl(APB_SPARE_PLL3CR),
	       pll3cr.v, __raw_readl(MPMU_PLL3CR));
}

/* frequency unit Mhz, return pll3 vco freq */
static u32 __attribute__ ((unused)) get_pll3_freq(u32 *pll3_freq,
						  u32 *pll3p_freq)
{
	union pmum_pll3cr pll3cr;
	union apb_spare_pllswcr pll3_sw_ctl;
	u32 pll3_vco, pll3_div, pll3p_div, pll3refd;

	/* return 0 if pll3 is disabled */
	if (!pll3_is_enabled()) {
		printf("%s PLL3 is not enabled!\n", __func__);
		*pll3_freq = 0;
		*pll3p_freq = 0;
		return 0;
	}

	pll3cr.v = __raw_readl(MPMU_PLL3CR);
	pll3refd = pll3cr.b.pll3refd;
	BUG_ON(pll3refd == 1);
	if (pll3refd == 0)
		pll3refd = 1;
	pll3_vco = 26 * pll3cr.b.pll3fbd / pll3refd;

	pll3_sw_ctl.v = __raw_readl(APB_SPARE_PLL3CR);
	pll3_div = __pll_divsel2div(pll3_sw_ctl.b.divselse);
	pll3p_div = __pll_divsel2div(pll3_sw_ctl.b.divseldiff);
	*pll3_freq = pll3_vco / pll3_div;
	*pll3p_freq = pll3_vco / pll3p_div;

	return pll3_vco;
}

/* PLL3CR[19:18] = 0 shutdown */
static int pll3_refcnt;
void turn_off_pll3(void)
{
	union pmum_pll3cr pll3cr;

	pll3_refcnt--;
	if (pll3_refcnt < 0)
		printf("unmatched pll3_refcnt\n");
	if (pll3_refcnt == 0) {
		pll3cr.v = __raw_readl(MPMU_PLL3CR);
		pll3cr.b.pll3_pu = 0;
		pll3cr.b.pclk_1248_sel = 0;
		__raw_writel(pll3cr.v, MPMU_PLL3CR);
		printf("Disable pll3 as it is not used!\n");
		if (has_feat_ssc())
			disable_pll3_ssc();
	}
}

/* must be called after __init_platform_opt */
void turn_on_pll3(void)
{
	u32 pll3vco, pll3, pll3p;
	unsigned int div, rng;

	BUG_ON(!cur_platform_opt ||
	       !cur_platform_opt->pll3vcofreq);

	pll3_refcnt++;
	if (pll3_refcnt == 1) {
		pll3vco = cur_platform_opt->pll3vcofreq;
		pll3 = cur_platform_opt->pll3freq;
		pll3p = cur_platform_opt->pll3pfreq;
		set_pll3_freq(pll3vco, pll3, pll3p);
		if (has_feat_ssc()) {
			__clk_pll_get_divrng(CENTER_SPREAD, 30000, 25, 1000,
					     pll3vco * MHZ, &div, &rng);
			enable_pll3_ssc(CENTER_SPREAD, div, rng);
		}
	}
}

/* unit MHZ */
void set_plat_max_corefreq(u32 max_freq)
{
	core_selected_max = max_freq;
}

static void  __init_platform_opt(int ddr_mode)
{
	unsigned int i, chipid = 0;
	enum ddr_type ddrtype = LPDDR2_400M;
	struct platform_opt *proc;
	if (cpu_is_pxa1088())
		chipid = 0xa01088;
	else if (cpu_is_pxa1L88())
		chipid = 0xf01188;
	/*
	 * ddr type is passed from OBM, FC code use this info
	 * to identify DDR OPs.
	 */
	if (ddr_mode == 0)
		ddrtype = LPDDR2_400M;
	else if (ddr_mode == 1)
		ddrtype = LPDDR2_533M;

	for (i = 0; i < ARRAY_SIZE(platform_op_arrays); i++) {
		proc = platform_op_arrays + i;
		if ((proc->chipid == chipid) && (proc->ddrtype == ddrtype))
			break;
	}
	BUG_ON(i == ARRAY_SIZE(platform_op_arrays));
	cur_platform_opt = proc;

	if (cpu_is_pxa1088() || cpu_is_pxa1L88()) {
		if (!is_1p5G_chip())
#ifndef CONFIG_CORE_1248
			cur_platform_opt->df_max_cpurate = CORE_1P18G;
#else
			cur_platform_opt->df_max_cpurate = CORE_1P25G;
#endif
		else {
			if (core_selected_max >
			    cur_platform_opt->df_max_cpurate)
				cur_platform_opt->df_max_cpurate =
						core_selected_max;
		}
	}

	/*
	 * pll3p dedicate for core, and its rate is determined
	 * by df_max_cpurate
	 */
	if ((cur_platform_opt->df_max_cpurate >
		cur_platform_opt->pll3pfreq) &&
		(cur_platform_opt->df_max_cpurate != 1248)) {
		if (cur_platform_opt->df_max_cpurate <
			PLL3_VCO_MIN) {
			cur_platform_opt->pll3pfreq =
				cur_platform_opt->df_max_cpurate;
			cur_platform_opt->pll3freq =
				cur_platform_opt->df_max_cpurate;
			cur_platform_opt->pll3vcofreq =
				2 * cur_platform_opt->df_max_cpurate;
		} else {
			cur_platform_opt->pll3pfreq =
				cur_platform_opt->df_max_cpurate;
			cur_platform_opt->pll3freq =
				cur_platform_opt->df_max_cpurate;
			cur_platform_opt->pll3vcofreq =
				cur_platform_opt->df_max_cpurate;
		}
		BUG_ON((cur_platform_opt->pll3vcofreq < PLL3_VCO_MIN) ||
		       (cur_platform_opt->pll3vcofreq > PLL3_VCO_MAX));
		printf("Platform default max frequency: %dMHZ\n",
		       cur_platform_opt->df_max_cpurate);
	}
}

static struct cpu_rtcwtc cpu_rtcwtc_1088[] = {
	{.max_pclk = 312, .l1_rtc = 0x02222222, .l2_rtc = 0x00002221,},
	{.max_pclk = 800, .l1_rtc = 0x02666666, .l2_rtc = 0x00006265,},
	{.max_pclk = 1183, .l1_rtc = 0x2AAAAAA, .l2_rtc = 0x0000A2A9,},
	{.max_pclk = 1482, .l1_rtc = 0x02EEEEEE, .l2_rtc = 0x0000E2ED,},
};

static struct cpu_rtcwtc cpu_rtcwtc_tsmc_1L88[] = {
	{.max_pclk = 624, .l1_rtc = 0x02222222, .l2_rtc = 0x00002221,},
	{.max_pclk = 1066, .l1_rtc = 0x02666666, .l2_rtc = 0x00006265,},
	{.max_pclk = 1283, .l1_rtc = 0x2AAAAAA, .l2_rtc = 0x0000A2A9,},
	{.max_pclk = 1482, .l1_rtc = 0x02EEEEEE, .l2_rtc = 0x0000E2ED,},
};

static struct cpu_rtcwtc cpu_rtcwtc_umc_1L88_a0c[] = {
	{.max_pclk = 312, .l1_rtc = 0x02222222, .l2_rtc = 0x00002221,},
	{.max_pclk = 800, .l1_rtc = 0x02666666, .l2_rtc = 0x00006265,},
	{.max_pclk = 1066, .l1_rtc = 0x02AAAAAA, .l2_rtc = 0x0000A2A9,},
	{.max_pclk = 1482, .l1_rtc = 0x02EEEEEE, .l2_rtc = 0x0000E2ED,},
};

static void __init_cpu_rtcwtc(struct pxa988_cpu_opt *cpu_opt)
{
	struct cpu_rtcwtc *cpu_rtcwtc;
	unsigned int size, index, ui_foundry;

	if (cpu_is_pxa1088()) {
		cpu_rtcwtc = cpu_rtcwtc_1088;
		size = ARRAY_SIZE(cpu_rtcwtc_1088);
	} else if (cpu_is_pxa1L88()) {
		ui_foundry = get_foundry();
		if (cpu_is_pxa1L88_a0c() && (ui_foundry == 2)) {
			cpu_rtcwtc = cpu_rtcwtc_umc_1L88_a0c;
			size = ARRAY_SIZE(cpu_rtcwtc_umc_1L88_a0c);
		} else {
			cpu_rtcwtc = cpu_rtcwtc_tsmc_1L88;
			size = ARRAY_SIZE(cpu_rtcwtc_tsmc_1L88);
		}
	} else {
		panic("Non-Supported cpu type, No rtcwtc is set!\n");
	}

	for (index = 0; index < size; index++)
		if (cpu_opt->pclk <= cpu_rtcwtc[index].max_pclk)
			break;
	if (index == size)
		index = size - 1;

	cpu_opt->l1_rtc = cpu_rtcwtc[index].l1_rtc;
	cpu_opt->l2_rtc = cpu_rtcwtc[index].l2_rtc;
};

/* Common condition here if you want to filter the core ops */
static unsigned int __is_cpu_op_invalid_com(struct pxa988_cpu_opt *cop)
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
static unsigned int __is_cpu_op_invalid_plt(struct pxa988_cpu_opt *cop)
{
	if (cur_platform_opt->is_cpuop_invalid_plt)
		return cur_platform_opt->is_cpuop_invalid_plt(cop);

	/* no rule no filter */
	return 0;
}

static void __init_cpu_opt(void)
{
	struct pxa988_cpu_opt *cpu_opt, *cop;
	unsigned int cpu_opt_size = 0, i;

	cpu_opt = cur_platform_opt->cpu_opt;
	cpu_opt_size = cur_platform_opt->cpu_opt_size;

	debug("pclk(src:sel,div) l2clk(src,div)\tpdclk(src,div)\tbaclk(src,div)\tperiphclk(src,div)\n");

	printf("ui_chip_foundry = %u\n", get_foundry());

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
		if (cop->periphclk) {
			if (has_feat_periclk_mult2())
				cop->periphclk_div =
					cop->pclk / (2 * cop->periphclk) - 1;
			else
				cop->periphclk_div =
					cop->pclk / (4 * cop->periphclk) - 1;
		}


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
	}
}

static void __init_ddr_axi_opt(void)
{
	struct pxa988_ddr_axi_opt *ddr_axi_opt, *cop;
	unsigned int ddr_axi_opt_size = 0, i;

	ddr_axi_opt = cur_platform_opt->ddr_axi_opt;
	ddr_axi_opt_size = cur_platform_opt->ddr_axi_opt_size;

	debug("dclk(src:sel,div) aclk(src:sel,div)\n");
	for (i = 0; i < ddr_axi_opt_size; i++) {
		cop = &ddr_axi_opt[i];
		cop->ddr_clk_src = ddr_axi_sel2_srcrate(cop->ddr_clk_sel);
		cop->axi_clk_src = ddr_axi_sel2_srcrate(cop->axi_clk_sel);
		BUG_ON((cop->ddr_clk_src < 0) || (cop->axi_clk_src < 0));
		cop->dclk_div =
			cop->ddr_clk_src / (2 * cop->dclk) - 1;
		cop->aclk_div =
			cop->axi_clk_src / cop->aclk - 1;

		debug("%d(%d:%d,%d)\t%d(%d:%d,%d)\n",
		      cop->dclk, cop->ddr_clk_src,
		      cop->ddr_clk_sel, cop->dclk_div,
		      cop->aclk, cop->axi_clk_src,
		      cop->axi_clk_sel, cop->aclk_div);
	}
}

static void __init_fc_setting(void)
{
	unsigned int regval;
	union pmua_cc cc_ap, cc_cp;
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
}

static struct pxa988_cpu_opt *cpu_rate2_op_ptr
	(unsigned int rate, unsigned int *index)
{
	unsigned int idx = 0;
	struct pxa988_cpu_opt *cop;

	list_for_each_entry(cop, &core_op_list, node) {
		if ((cop->pclk >= rate) ||
		    list_is_last(&cop->node, &core_op_list))
			break;
		idx++;
	}

	*index = idx;
	return cop;
}

static unsigned int ddr_rate2_op_index(unsigned int rate)
{
	unsigned int index;
	struct pxa988_ddr_axi_opt *op_array =
		cur_platform_opt->ddr_axi_opt;
	unsigned int op_array_size =
		cur_platform_opt->ddr_axi_opt_size;

	if (unlikely(rate > op_array[op_array_size - 1].dclk))
		return op_array_size - 1;

	for (index = 0; index < op_array_size; index++)
		if (op_array[index].dclk >= rate)
			break;

	return index;
}

static unsigned int axi_rate2_op_index(unsigned int rate)
{
	unsigned int index;
	struct pxa988_ddr_axi_opt *op_array =
		cur_platform_opt->ddr_axi_opt;
	unsigned int op_array_size =
		cur_platform_opt->ddr_axi_opt_size;

	if (unlikely(rate > op_array[op_array_size - 1].aclk))
		return op_array_size - 1;

	for (index = 0; index < op_array_size; index++)
		if (op_array[index].aclk >= rate)
			break;

	return index;
}

static unsigned int cpu_sel2_srcrate(enum ap_clk_sel ap_sel)
{
	if (ap_sel == AP_CLK_SRC_PLL1_624)
		return 624;
	else if (ap_sel == AP_CLK_SRC_PLL1_1248)
		return 1248;
	else if (ap_sel == AP_CLK_SRC_PLL2)
		return cur_platform_opt->pll2freq;
	else if (ap_sel == AP_CLK_SRC_PLL2P)
		return cur_platform_opt->pll2pfreq;
	else if (ap_sel == AP_CLK_SRC_PLL3P)
		return cur_platform_opt->pll3pfreq;
	printf("%s bad ap_clk_sel %d\n", __func__, ap_sel);
	return -ENOENT;
}

static unsigned int ddr_axi_sel2_srcrate(enum ddr_axi_clk_sel ddr_axi_sel)
{
	if (ddr_axi_sel == DDR_AXI_CLK_SRC_PLL1_416)
		return 416;
	else if (ddr_axi_sel == DDR_AXI_CLK_SRC_PLL1_624)
		return 624;
	else if (ddr_axi_sel == DDR_AXI_CLK_SRC_PLL2)
		return cur_platform_opt->pll2freq;
	else if (ddr_axi_sel == DDR_AXI_CLK_SRC_PLL2P)
		return cur_platform_opt->pll2pfreq;
	printf("%s bad ddr_axi_sel %d\n", __func__, ddr_axi_sel);
	return -ENOENT;
}

static unsigned int clk_enable(unsigned int src)
{
	/*
	 * The value is the same at case:
	 * DDR_AXI_CLK_SRC_PLL2 & AP_CLK_SRC_PLL2
	 * DDR_AXI_CLK_SRC_PLL2P& AP_CLK_SRC_PLL2P
	 */
	switch (src) {
	case AP_CLK_SRC_PLL2:
	case AP_CLK_SRC_PLL2P:
		turn_on_pll2();
		break;
	case AP_CLK_SRC_PLL3P:
		turn_on_pll3();
		break;
	default:
		break;
	}
	return 0;
}

static unsigned int clk_disable(unsigned int src)
{
	switch (src) {
	case AP_CLK_SRC_PLL2:
	case AP_CLK_SRC_PLL2P:
		turn_off_pll2();
		break;
	case AP_CLK_SRC_PLL3P:
		turn_off_pll3();
		break;
	default:
		break;
	}
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

static void get_cur_cpu_op(struct pxa988_cpu_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;
	union pmua_cc cc_cp;
	union pmua_dm_cc2 dm_cc2_ap;
	unsigned int pll1_pll3_sel;

	get_fc_lock();

	dm_cc_ap.v = __raw_readl(APMU_CCSR);
	dm_cc2_ap.v = __raw_readl(APMU_CC2SR);
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	cc_cp.b.core_rd_st_clear = 1;
	__raw_writel(cc_cp.v, APMU_CP_CCR);
	cc_cp.b.core_rd_st_clear = 0;
	__raw_writel(cc_cp.v, APMU_CP_CCR);
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);
	pll1_pll3_sel = __raw_readl(MPMU_PLL3CR);

	cop->ap_clk_src = cpu_sel2_srcrate(pllsel.b.apclksel);
	cop->ap_clk_sel = pllsel.b.apclksel;
	if ((pllsel.b.apclksel == 0x1) && (pll1_pll3_sel & (1 << 18))) {
		cop->ap_clk_src = cpu_sel2_srcrate(AP_CLK_SRC_PLL3P);
		cop->ap_clk_sel = AP_CLK_SRC_PLL3P;
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
	if (cop->periphclk) {
		if (has_feat_periclk_mult2())
			cop->periphclk =
				cop->pclk / (dm_cc2_ap.b.peri_clk_div + 1) / 2;
		else
			cop->periphclk =
				cop->pclk / (dm_cc2_ap.b.peri_clk_div + 1) / 4;
	}

	put_fc_lock();
}

static void get_cur_ddr_axi_op(struct pxa988_ddr_axi_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;
	union pmua_cc cc_cp;

	get_fc_lock();

	dm_cc_ap.v = __raw_readl(APMU_CCSR);
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	cc_cp.b.core_rd_st_clear = 1;
	__raw_writel(cc_cp.v, APMU_CP_CCR);
	cc_cp.b.core_rd_st_clear = 0;
	__raw_writel(cc_cp.v, APMU_CP_CCR);
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);

	cop->ddr_clk_src = ddr_axi_sel2_srcrate(pllsel.b.ddrclksel);
	cop->ddr_clk_sel = pllsel.b.ddrclksel;
	cop->axi_clk_src = ddr_axi_sel2_srcrate(pllsel.b.axiclksel);
	cop->axi_clk_sel = pllsel.b.axiclksel;
	cop->dclk = cop->ddr_clk_src / (dm_cc_ap.b.ddr_clk_div + 1) / 2;
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

static void pll1_pll3_switch(enum ap_clk_sel sel)
{
	unsigned int regval;

	if ((sel != AP_CLK_SRC_PLL3P) && (sel != AP_CLK_SRC_PLL1_1248))
		return;

	regval = __raw_readl(MPMU_PLL3CR);
	if (sel == AP_CLK_SRC_PLL1_1248)
		regval &= ~(1 << 18);
	else
		regval |= (1 << 18);
	debug_wt_reg(regval, MPMU_PLL3CR);
}

static void set_ap_clk_sel(struct pxa988_cpu_opt *top)
{
	union pmum_fccr fccr;
	unsigned int value;

	/* sel pll1 and pll3 switch bit */
	pll1_pll3_switch(top->ap_clk_sel);

	fccr.v = __raw_readl(MPMU_FCCR);
	fccr.b.mohclksel =
		top->ap_clk_sel & AP_SRC_SEL_MASK;
	debug_wt_reg(fccr.v, MPMU_FCCR);
	value = __raw_readl(MPMU_FCCR);
	if (value != fccr.v)
		printf("CORE FCCR Write failure: target 0x%X, final value 0x%X\n",
		       fccr.v, value);
}

static void set_periph_clk_div(struct pxa988_cpu_opt *top)
{
	union pmua_cc2 cc_ap2;

	cc_ap2.v = __raw_readl(APMU_CC2R);
	cc_ap2.b.peri_clk_div = top->periphclk_div;
	debug_wt_reg(cc_ap2.v, APMU_CC2R);
}

static void set_ddr_clk_sel(struct pxa988_ddr_axi_opt *top)
{
	union pmum_fccr fccr;
	unsigned int value;

	fccr.v = __raw_readl(MPMU_FCCR);
	fccr.b.ddrclksel = top->ddr_clk_sel;
	debug_wt_reg(fccr.v, MPMU_FCCR);
	value = __raw_readl(MPMU_FCCR);
	if (value != fccr.v)
		printf("DDR FCCR Write failure: target 0x%x, final value 0x%X\n",
		       fccr.v, value);
}

static void set_axi_clk_sel(struct pxa988_ddr_axi_opt *top)
{
	union pmum_fccr fccr;
	unsigned int value;

	fccr.v = __raw_readl(MPMU_FCCR);
	fccr.b.axiclksel0 = top->axi_clk_sel & 0x1;
	fccr.b.axiclksel1 = (top->axi_clk_sel & 0x2) >> 1;
	debug_wt_reg(fccr.v, MPMU_FCCR);
	value = __raw_readl(MPMU_FCCR);
	if (value != fccr.v)
		printf("AXI FCCR Write failure: target 0x%x, final value 0x%X\n",
		       fccr.v, value);
}

static void set_ddr_tbl_index(unsigned int index)
{
	unsigned int regval;

	index = (index > 0x7) ? 0x7 : index;
	regval = __raw_readl(APMU_MC_HW_SLP_TYPE);
	regval &= ~(0x1 << 6);		/* enable tbl based FC */
	regval &= ~(0x7 << 3);		/* clear ddr tbl index */
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
static void core_fc_seq(struct pxa988_cpu_opt *cop,
			    struct pxa988_cpu_opt *top)
{
	union pmua_cc cc_ap, cc_cp;

	/*
	 * add check here to avoid the case default boot up PP
	 * has different pdclk and paclk definition with PP used
	 * in production
	 */
	if ((cop->ap_clk_src == top->ap_clk_src) &&
	    (cop->pclk == top->pclk) &&
	    (cop->pdclk == top->pdclk) &&
	    (cop->baclk == top->baclk)) {
		printf("current rate = target rate!\n");
		return;
	}

	/* low -> high */
	if ((cop->pclk < top->pclk) && (top->l1_rtc != cop->l1_rtc)) {
		__raw_writel(top->l1_rtc, CIU_CA9_CPU_CONF_SRAM_0);
		__raw_writel(top->l2_rtc, CIU_CA9_CPU_CONF_SRAM_1);
	}

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n", __func__);
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
	if ((cop->pclk > top->pclk) && (top->l1_rtc != cop->l1_rtc)) {
		__raw_writel(top->l1_rtc, CIU_CA9_CPU_CONF_SRAM_0);
		__raw_writel(top->l2_rtc, CIU_CA9_CPU_CONF_SRAM_1);
	}
}

static int set_core_freq(struct pxa988_cpu_opt *old, struct pxa988_cpu_opt *new)
{
	struct pxa988_cpu_opt cop;
	int ret = 0;

	printf("CORE FC start: %u -> %u\n", old->pclk, new->pclk);
	get_fc_lock();

	memcpy(&cop, old, sizeof(struct pxa988_cpu_opt));
	clk_enable(new->ap_clk_sel);
	core_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct pxa988_cpu_opt));
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
		clk_disable(new->ap_clk_sel);
		goto out;
	}

	clk_disable(old->ap_clk_sel);
out:
	put_fc_lock();
	printf("CORE FC end: %u -> %u\n", old->pclk, new->pclk);
	return ret;
}

static int pxa988_cpu_setrate(unsigned long rate)
{
	struct pxa988_cpu_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	struct pxa988_cpu_opt *op_array =
		cur_platform_opt->cpu_opt;

	if (unlikely(!cur_cpu_op))
		cur_cpu_op = &pxa988_cpu_bootop;
	md_new = cpu_rate2_op_ptr(rate, &index);
	md_old = cur_cpu_op;

	/*
	 * Switching pll1_1248 and pll3p may generate glitch
	 * step 1),2),3) is neccessary
	 */
	if (((md_old->ap_clk_sel == AP_CLK_SRC_PLL3P) &&
	     (md_new->ap_clk_sel == AP_CLK_SRC_PLL1_1248)) ||
	    ((md_old->ap_clk_sel == AP_CLK_SRC_PLL1_1248) &&
	     (md_new->ap_clk_sel == AP_CLK_SRC_PLL3P))) {
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
	cur_cpu_op = md_new;
out:
	return ret;
}

static void ddr_fc_seq(struct pxa988_ddr_axi_opt *cop,
			    struct pxa988_ddr_axi_opt *top)
{
	union pmua_cc cc_ap, cc_cp;
	unsigned int ddr_axi_fc = 0;

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n", __func__);
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
		ddr_axi_fc = 1;
	}

	/* 3) set div and FC req bit trigger DDR/AXI FC */
	if (ddr_axi_fc) {
		debug_wt_reg(cc_ap.v, APMU_CCR);
		wait_for_fc_done();
	}

	/* 4) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);
}

static void axi_fc_seq(struct pxa988_ddr_axi_opt *cop,
			    struct pxa988_ddr_axi_opt *top)
{
	union pmua_cc cc_ap, cc_cp;
	unsigned int ddr_axi_fc = 0;

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n", __func__);
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
		ddr_axi_fc = 1;
	}

	/* 3) set div and FC req bit trigger DDR/AXI FC */
	if (ddr_axi_fc) {
		debug_wt_reg(cc_ap.v, APMU_CCR);
		wait_for_fc_done();
	}

	/* 4) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.core_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);
}

static int set_ddr_freq(struct pxa988_ddr_axi_opt *old,
	struct pxa988_ddr_axi_opt *new)
{
	struct pxa988_ddr_axi_opt cop;
	int ret = 0, errflag = 0;

	printf("DDR FC start: DDR %u -> %u\n", old->dclk, new->dclk);
	get_fc_lock();

	memcpy(&cop, old, sizeof(struct pxa988_ddr_axi_opt));

	clk_enable(new->ddr_clk_sel);
	ddr_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct pxa988_ddr_axi_opt));
	get_cur_ddr_axi_op(&cop);
	if (unlikely((cop.ddr_clk_src != new->ddr_clk_src) ||
		     (cop.dclk != new->dclk))) {
		clk_disable(new->ddr_clk_sel);
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

	clk_disable(old->ddr_clk_sel);
out:
	put_fc_lock();
	printf("DDR FC end: DDR %u -> %u\n", old->dclk, new->dclk);
	return ret;
}

static int set_axi_freq(struct pxa988_ddr_axi_opt *old,
	struct pxa988_ddr_axi_opt *new)
{
	struct pxa988_ddr_axi_opt cop;
	int ret = 0, errflag = 0;

	printf("AXI FC start: AXI %u -> %u\n", old->aclk, new->aclk);
	get_fc_lock();

	memcpy(&cop, old, sizeof(struct pxa988_ddr_axi_opt));
	clk_enable(new->axi_clk_sel);
	axi_fc_seq(&cop, new);

	memcpy(&cop, new, sizeof(struct pxa988_ddr_axi_opt));
	get_cur_ddr_axi_op(&cop);
	if (unlikely((cop.axi_clk_src != new->axi_clk_src) ||
		     (cop.aclk != new->aclk))) {
		clk_disable(new->axi_clk_sel);
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

	clk_disable(old->axi_clk_sel);
out:
	put_fc_lock();
	printf("AXI FC end: AXI %u -> %u\n", old->aclk, new->aclk);
	return ret;
}

static int pxa988_ddr_setrate(unsigned long rate)
{
	struct pxa988_ddr_axi_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	struct pxa988_ddr_axi_opt *op_array =
		cur_platform_opt->ddr_axi_opt;

	if (unlikely(!cur_ddr_op))
		cur_ddr_op = &pxa988_ddraxi_bootop;
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

static int pxa988_axi_setrate(unsigned long rate)
{
	struct pxa988_ddr_axi_opt *md_new, *md_old;
	unsigned int index;
	int ret = 0;
	struct pxa988_ddr_axi_opt *op_array =
		cur_platform_opt->ddr_axi_opt;

	if (unlikely(!cur_axi_op))
		cur_axi_op = &pxa988_ddraxi_bootop;
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

/* frequency unit Mhz*/
int getopindex(unsigned int corefreq, unsigned int ddrfreq)
{
	struct operating_point *op_array =
		cur_platform_opt->op_array;
	unsigned int op_size = cur_platform_opt->com_op_size, i;

	for (i = 0; i < op_size; i++)
		if ((op_array[i].pclk == corefreq) &&
		    (op_array[i].dclk == ddrfreq))
			return i;
	return op_size - 1;
}

unsigned int get_max_cpurate(void)
{
	if (cur_platform_opt)
		return cur_platform_opt->df_max_cpurate;
	return 0;
}

/*
 * This function is called from board level or cmd to
 * issue the core, ddr&axi frequency change.
 */
int setop(int opnum)
{
	int cur_volt;
	int ret = 0;
	struct operating_point *op_array =
		cur_platform_opt->op_array;
	unsigned int op_size = cur_platform_opt->com_op_size;

	if (opnum >= op_size) {
		printf("opnum out of range!\n");
		return -EAGAIN;
	}
	cur_volt = get_volt();
	if (cur_volt <= 0) {
		printf("Fail to get voltage!\n");
		return -1;
	}
	if (op_array[opnum].vcore > cur_volt)
		ret = set_volt(op_array[opnum].vcore);
	if (ret) {
		printf("Increase volatge %dmV failed!\n",
		       op_array[opnum].vcore);
		return -1;
	}

	/* set core frequency */
	pxa988_cpu_setrate(op_array[opnum].pclk);

	/* set ddr and axi frequency */
	pxa988_ddr_setrate(op_array[opnum].dclk);
	pxa988_axi_setrate(op_array[opnum].aclk);

	if (op_array[opnum].vcore < cur_volt)
		ret = set_volt(op_array[opnum].vcore);
	if (ret) {
		printf("Decrease volatge %dmV failed!\n",
		       op_array[opnum].vcore);
		return -1;
	}
	return 0;
}

void __init_op_array(int profile)
{
	unsigned int i = 0, j = 0;
	int max_require;
	int *vm_millivolts_tmp;
	unsigned int (*freqs_cmb)[VL_MAX];
	struct operating_point *op_array =
		cur_platform_opt->op_array;
	unsigned int op_size = cur_platform_opt->com_op_size;

	if (cpu_is_pxa1088_a0()) {
		vm_millivolts_tmp = vm_mv_1088a0_svc_1p2G[profile];
		freqs_cmb = freqs_cmb_1088a0;
	} else if (cpu_is_pxa1088_a1()) {
		if (cur_platform_opt->df_max_cpurate > CORE_1P18G)
			vm_millivolts_tmp = vm_mv_1088a1_svc_1p25G[profile];
		else
			vm_millivolts_tmp = vm_mv_1088a1_svc_1p2G[profile];
		freqs_cmb = freqs_cmb_1088a1;
	} else if (cpu_is_pxa1L88_a0()) {
		if (cur_platform_opt->df_max_cpurate > CORE_1P18G)
			vm_millivolts_tmp = vm_mv_1L88a0_svc_1p25G[profile];
		else
			vm_millivolts_tmp = vm_mv_1L88a0_svc[profile];
		freqs_cmb = freqs_cmb_1L88;
	} else if (cpu_is_pxa1L88_a0c()) {
		if (cur_platform_opt->df_max_cpurate > CORE_1P25G)
			if (get_foundry() == 2)
				vm_millivolts_tmp =
					vm_mv_1L88a0c_umc_svc_1p5G[profile];
			else
				vm_millivolts_tmp =
					vm_mv_1L88a0c_svc_1p5G[profile];
		else if (cur_platform_opt->df_max_cpurate > CORE_1P18G)
			if (get_foundry() == 2)
				vm_millivolts_tmp =
					vm_mv_1L88a0c_umc_svc_1p25G[profile];
			else
				vm_millivolts_tmp =
					vm_mv_1L88a0c_svc_1p25G[profile];
		else
			if (get_foundry() == 2)
				vm_millivolts_tmp =
					vm_mv_1L88a0c_umc_svc_1p2G[profile];
			else
				vm_millivolts_tmp =
					vm_mv_1L88a0c_svc_1p2G[profile];
		freqs_cmb = freqs_cmb_1L88;
	} else {
		vm_millivolts_tmp = vm_mv_1088a0_svc_1p2G[profile];
		freqs_cmb = freqs_cmb_1088a0;
	}

	for (i = 0; i < op_size; i++) {
		for (j = 0; j < VL_MAX; j++) {
			if (op_array[i].pclk <= freqs_cmb[CORE][j])
				break;
		}
		max_require = vm_millivolts_tmp[j];

		for (j = 0; j < VL_MAX; j++) {
			if (op_array[i].dclk <= freqs_cmb[DDR_AXI][j])
				break;
		}
		op_array[i].vcore = max(max_require, vm_millivolts_tmp[j]);
	}
}

/*
 * This function is called from board/CPU init level to initialize
 * the OP table and frequency change related setting
 */
void pxa988_fc_init(int ddr_mode)
{
	unsigned int dcg_regval = 0, mc_hw_slp;
	int iprofile = get_profile();

	__init_platform_opt(ddr_mode);
	__init_cpu_opt();
	__init_ddr_axi_opt();
	__init_fc_setting();

	/* enable MCK4 and AXI fabric dynamic clk gating */
	dcg_regval = __raw_readl(MC_CONF);
	/* disable cp fabric clk gating */
	dcg_regval &= ~(1 << 16);
	/* enable dclk gating */
	dcg_regval &= ~(1 << 19);
	if (!has_feat_mck4_axi_clock_gate()) {
		dcg_regval |= (1 << 9) | (1 << 18) | /* Seagull */
			(1 << 12) | (1 << 27) |  /* Fabric #2 */
			(1 << 15) | (1 << 20) | (1 << 21) | /* VPU*/
			(1 << 17) | (1 << 26); /* Fabric#1 CA9 */
	} else {
		dcg_regval |= (0xff << 8) | /* MCK4 P0~P7*/
			(1 << 17) | (1 << 18) | /* Fabric 0 */
			(1 << 20) | (1 << 21) |	/* VPU fabric */
			(1 << 26) | (1 << 27);  /* Fabric 0/1 */
	}
	__raw_writel(dcg_regval, MC_CONF);
	/* enable MCK4 AHB clock */
	__raw_writel(0x3, APMU_MCK4_CTRL);

	/* Disable dvc dll auto update */
	if (has_feat_dvc_auto_dll_update()) {
		mc_hw_slp = __raw_readl(APMU_MC_HW_SLP_TYPE);
		mc_hw_slp |= (1 << 9);
		__raw_writel(mc_hw_slp, APMU_MC_HW_SLP_TYPE);
	}
	/*
	 * Some Z2 chips need higher voltage to make system stable.
	 * And based on current result, 986 Z2 need higher voltage
	 * than 988 Z2 chip. So, update volt into op_array accordingly.
	 */
	__init_op_array(iprofile);
	cur_cpu_op = &pxa988_cpu_bootop;
	cur_ddr_op = &pxa988_ddraxi_bootop;
	cur_axi_op = &pxa988_ddraxi_bootop;
}

void show_op(void)
{
	struct pxa988_cpu_opt cop;
	struct pxa988_ddr_axi_opt daop;
	unsigned int i;
	struct operating_point *op_array =
		cur_platform_opt->op_array;
	unsigned int op_size = cur_platform_opt->com_op_size;

	printf("Current OP:\n");
	memcpy(&cop, cur_cpu_op, sizeof(struct pxa988_cpu_opt));
	printf("pclk(src:sel) l2clk(src)\tpdclk(src)\tbaclk(src)\tperiphclk(src)\n");
	printf("%d(%d:%d)\t%d([%s])\t%d([%s])\t%d([%s])\t%d([%s])\n",
	       cop.pclk, cop.ap_clk_src,
	       cop.ap_clk_sel & AP_SRC_SEL_MASK,
	       cop.l2clk, cop.l2clk ? "pclk" : "NULL",
	       cop.pdclk, cop.l2clk ? "l2clk" : "pclk",
	       cop.baclk, cop.l2clk ? "l2clk" : "pclk",
	       cop.periphclk, cop.periphclk ? "pclk" : "NULL");
	memcpy(&daop, cur_ddr_op, sizeof(struct pxa988_ddr_axi_opt));
	printf("dclk(src:sel)\n");
	printf("%d(%d:%d)\n", daop.dclk, daop.ddr_clk_src, daop.ddr_clk_sel);

	memcpy(&daop, cur_axi_op, sizeof(struct pxa988_ddr_axi_opt));
	printf("aclk(src:sel)\n");
	printf("%d(%d:%d)\n", daop.aclk, daop.axi_clk_src, daop.axi_clk_sel);

	printf("\nAll supported OP:\n");
	printf("OP\t Core(MHZ)\t DDR(MHZ)\t AXI(MHZ)\t Vcore\n");
	for (i = 0; i < op_size; i++) {
		if (cur_platform_opt->df_max_cpurate &&
		    (op_array[i].pclk > cur_platform_opt->df_max_cpurate))
			continue;
		printf("%d\t %d\t %d\t %d\t %d\t\n", i,
		       op_array[i].pclk, op_array[i].dclk,
		       op_array[i].aclk, op_array[i].vcore);
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

#define PM812_SLAVE_ADDR	0x31
#define PM812_VBUCK1_SET0_REG	0x3c	/* dvc[1:0] = 0x0 */

#define VOL_BASE		600000
#define VOL_STEP		12500
#define VOL_HIGH		1400000

#define PM812_DVC1		43
#define PM812_DVC2		44

static int set_volt(u32 vol)
{
	int ret = 0;
	struct pmic *p_power;

	vol *= 1000;
	if ((vol < VOL_BASE) || (vol > VOL_HIGH)) {
		printf("out of range when set voltage!\n");
		return -1;
	}

	p_power = pmic_get(MARVELL_PMIC_POWER);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;
	ret = marvell88pm_set_buck_vol(p_power, 1, vol);
	printf("Set VBUCK1 to %4dmV\n", vol / 1000);

	/* always use DVC[1:0] = 0 in uboot */
	/* TODO : make sure DVC pins' function is GPIO */
	gpio_direction_output(PM812_DVC1, 0);
	gpio_direction_output(PM812_DVC2, 0);
	return ret;
}

static int get_volt(void)
{
	u32 vol = 0;
	struct pmic *p_power;

	p_power = pmic_get(MARVELL_PMIC_POWER);
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
			"for Emei 988, xxxx can be 600..1350, step 13 or 25\n"
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

int do_pllops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong pllx, enable, vco = 0, pll = 0, pllp = 0;
	int res = -1;

	if (argc != 6) {
		printf("usage:\n");
		printf("pll 2/3 1 pll2/3vcofreq pll2/3freq pll2p/3pfreq enable pll2/3 and set to XXX MHZ\n");
		printf("pll 2/3 1 0 0 0 enable pll2/3 at default frequency\n");
		printf("pll 2/3 0 0 0 0 disable pll2/3\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &pllx);
	if (res < 0) {
		printf("Failed to get pll num\n");
		return -res;
	}

	res = strict_strtoul(argv[2], 0, &enable);
	if (res < 0) {
		printf("Failed to get enable/disable flag\n");
		return -res;
	}

	res = strict_strtoul(argv[3], 0, &vco);
	if (res < 0) {
		printf("Failed to get enable/disable flag\n");
		return -res;
	}

	res = strict_strtoul(argv[4], 0, &pll);
	if (res < 0) {
		printf("Failed to get enable/disable flag\n");
		return -res;
	}

	res = strict_strtoul(argv[5], 0, &pllp);
	if (res < 0) {
		printf("Failed to get enable/disable flag\n");
		return -res;
	}

	if (enable) {
		if (vco && (pllx == 2))
			set_pll2_freq(vco, pll, pllp);
		else if (!vco && (pllx == 2))
			turn_on_pll2();
		else if (vco && (pllx == 3))
			set_pll3_freq(vco, pll, pllp);
		else if (!vco && (pllx == 3))
			turn_on_pll3();
	} else {
		if (pllx == 2)
			turn_off_pll2();
		else if (pllx == 3)
			turn_off_pll3();
	}
	return 0;
}

U_BOOT_CMD(
	pllops, 6, 1, do_pllops,
	"Set PLL2/3 output",
	""
);

int do_setcpurate(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	struct pxa988_cpu_opt *cpu_opt;
	unsigned int cpu_opt_size = 0, i;
	ulong freq;
	int res = -1;

	cpu_opt = cur_platform_opt->cpu_opt;
	cpu_opt_size = cur_platform_opt->cpu_opt_size;

	if (argc != 2) {
		printf("usage: set core frequency xxxx(unit Mhz)\n");
		printf("Supported Core frequency: ");
		for (i = 0; i < cpu_opt_size; i++)
			printf("%d\t", cpu_opt[i].pclk);
		printf("\n");
		return -1;
	}
	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (pxa988_cpu_setrate(freq) == 0))
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
	struct pxa988_ddr_axi_opt *ddr_axi_opt;
	unsigned int ddr_axi_opt_size = 0, i;
	ulong freq;
	int res = -1;

	ddr_axi_opt = cur_platform_opt->ddr_axi_opt;
	ddr_axi_opt_size = cur_platform_opt->ddr_axi_opt_size;

	if (argc != 2) {
		printf("usage: set ddr/axi frequency xxxx(unit Mhz)\n");
		printf("Supported ddr/axi frequency: ");
		for (i = 0; i < ddr_axi_opt_size; i++)
			printf("%d/%d\t",
			       ddr_axi_opt[i].dclk, ddr_axi_opt[i].aclk);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (pxa988_ddr_setrate(freq) == 0))
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
	struct pxa988_ddr_axi_opt *ddr_axi_opt;
	unsigned int ddr_axi_opt_size = 0, i;
	ulong freq;
	int res = -1;

	ddr_axi_opt = cur_platform_opt->ddr_axi_opt;
	ddr_axi_opt_size = cur_platform_opt->ddr_axi_opt_size;

	if (argc != 2) {
		printf("usage: set axi frequency xxxx(unit Mhz)\n");
		printf("Supported axi frequency: ");
		for (i = 0; i < ddr_axi_opt_size; i++)
			printf("%d\t", ddr_axi_opt[i].aclk);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if ((res == 0) && (pxa988_axi_setrate(freq) == 0))
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

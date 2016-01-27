/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Bill Zhou <zhoub@marvell.com>
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
#define DCIU_BASE		0xD1DF0000
#define DCIU_REG(x)		(DCIU_BASE + x)
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
#define APMU_FCLSR		APMU_REG(0x0334)

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

/* FUSE */
#define APMU_GEU               APMU_REG(0x068)
#define GEU_FUSE_MANU_PARA_0 GEU_REG(0x110)
#define GEU_FUSE_MANU_PARA_1 GEU_REG(0x114)
#define GEU_FUSE_MANU_PARA_2 GEU_REG(0x118)
#define GEU_AP_CP_MP_ECC GEU_REG(0x11C)
#define BLOCK0_RESERVED_1 GEU_REG(0x120)
#define BLOCK4_MANU_PARA1_0  GEU_REG(0x2b4)
#define BLOCK4_MANU_PARA1_1  GEU_REG(0x2b8)
#define NUM_PROFILES 16

/* RTC, WTC */
#define DCIU_CLST_L1(x)		DCIU_REG(0x14 + (x * 0x40))
#define DCIU_CLST_L2(x)		DCIU_REG(0x18 + (x * 0x40))
#define CIU_TOP_MEM_XTC		CIU_REG(0x0044)

/*
 * for ana_grp PU_CLK contro. Must be enabled if
 * PLL2 or PLL3 or USB are used
 */
#define UTMI_CTRL		0xD4207104

#define AP_SRC_SEL_MASK		0x7
#define MPMU_FCAP_MASK		0x77
#define MPMU_FCDCLK_MASK	0x7
#define MPMU_FCACLK_MASK	0x3
#define UNDEF_OP		-1
#define MHZ_TO_KHZ		(1000)

#define PROFILE_NUM	11
#define CONFIG_SVC_TSMC	1

union pmua_pllsel {
	struct {
		unsigned int cpclksel:3;
		unsigned int ddrclksel:3;
		unsigned int axiclksel:2;
		unsigned int apc0clksel:3;
		unsigned int apc1clksel:3;
		unsigned int reserved0:18;
	} b;
	unsigned int v;
};

union pmua_cc {
	struct {
		unsigned int core_clk_div:3;
		unsigned int reserved1:9;
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
		unsigned int apc0_pclk_div:3;
		unsigned int apc0_aclk_div:3;
		unsigned int apc1_pclk_div:3;
		unsigned int apc1_aclk_div:3;
		unsigned int ddr_clk_div:3;
		unsigned int bus_clk_div:3;
		unsigned int ap_allow_spd_chg:1;
		unsigned int reserved0:1;
		unsigned int apc0_freq_chg_req:1;
		unsigned int apc1_freq_chg_req:1;
		unsigned int ddr_freq_chg_req:1;
		unsigned int bus_freq_chg_req:1;
		unsigned int apc0_dyn_fc:1;
		unsigned int apc1_dyn_fc:1;
		unsigned int dclk_dyn_fc:1;
		unsigned int aclk_dyn_fc:1;
		unsigned int reserved1:3;
		unsigned int core_rd_st_clear:1;
	} b;
	unsigned int v;
};

union pmua_dm_cc {
	struct {
		unsigned int apc0_pclk_div:3;
		unsigned int apc0_aclk_div:3;
		unsigned int apc1_pclk_div:3;
		unsigned int apc1_aclk_div:3;
		unsigned int ddr_clk_div:3;
		unsigned int bus_clk_div:3;
		unsigned int reserved0:6;	/* FIXME: async bits? */
		unsigned int cp_rd_status:1;
		unsigned int ap_rd_status:1;
		unsigned int cp_fc_done:1;
		unsigned int apc0_fc_done:1;
		unsigned int dclk_fc_done:1;
		unsigned int aclk_fc_done:1;
		unsigned int apc1_fc_done:1;
		unsigned int reserved:1;
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
		unsigned int mclpmtblnum:2;
		unsigned int reserved:15;
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

/* core,ddr,axi clk src sel register description */
union pmum_fcapclk {
	struct {
		unsigned int apc0clksel:3;
		unsigned int reserved:1;
		unsigned int apc1clksel:3;
		unsigned int reserved1:25;
	} b;
	unsigned int v;
};

union pmum_fcdclk {
	struct {
		unsigned int ddrclksel:3;
		unsigned int reserved:29;
	} b;
	unsigned int v;
};

union pmum_fcaclk {
	struct {
		unsigned int axiclksel:2;
		unsigned int reserved:30;
	} b;
	unsigned int v;
};

/*
 * Clust0 clock source:
 * 0x0 = PLL1 624 MHz
 * 0x1 = PLL1 1248 MHz
 * 0x2 = PLL2_CLKOUT
 * 0x3 = PLL1 832 MHZ
 * 0x5 = PLL3_CLKOUTP
 */
enum c0_clk_sel {
	C0_CLK_SRC_PLL1_624 = 0x0,
	C0_CLK_SRC_PLL1_1248 = 0x1,
	C0_CLK_SRC_PLL2 = 0x2,
	C0_CLK_SRC_PLL1_832 = 0x3,
	C0_CLK_SRC_PLL3P = 0x5,
};

/*
 * Clust0 clock source:
 * 0x0 = PLL1 624 MHz
 * 0x1 = PLL1 1248 MHz
 * 0x2 = PLL2_CLKOUT
 * 0x3 = PLL1 832 MHZ
 * 0x5 = PLL3_CLKOUTP
 */
enum c1_clk_sel {
	C1_CLK_SRC_PLL1_832 = 0x0,
	C1_CLK_SRC_PLL1_1248 = 0x1,
	C1_CLK_SRC_PLL2 = 0x2,
	C1_CLK_SRC_PLL3P = 0x3,
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
	DDR_800M_2X,
	DDR_TYPE_MAX,
};

/* RTC/WTC table used for solution change rtc/wtc on the fly */
struct cpu_rtcwtc {
	/* max rate could be used by this rtc/wtc */
	unsigned int max_pclk;
	unsigned int l1_xtc;
	unsigned int l2_xtc;
};

struct cpu_opt {
	int clst_index;	/* distinguish cluster 0 or 1 */
	unsigned int pclk;	/* core clock */
	unsigned int core_aclk;	/* core AXI interface clock */
	unsigned int ap_clk_sel;	/* core src sel val */
	struct clk *parent;	/* core clk parent node */
	unsigned int ap_clk_src;	/* core src rate */
	unsigned int pclk_div;	/* core clk divider */
	unsigned int core_aclk_div;	/* core AXI interface clock divider */
	unsigned int l1_xtc;		/* L1 cache RTC/WTC */
	unsigned int l2_xtc;		/* L2 cache RTC/WTC */
	unsigned long l1_xtc_addr;
	unsigned long l2_xtc_addr;
	struct list_head node;

	/* use to record voltage requirement(mV) */
	unsigned int volts;
};

struct ddr_opt {
	unsigned int dclk;		/* ddr clock */
	unsigned int mode_4x_en;        /* enable dclk 4x mode */
	unsigned int ddr_tbl_index;	/* ddr FC table index */
	unsigned int ddr_freq_level;	/* ddr freq level(0~7) */
	int ddr_clk_sel;/* ddr src sel val */
	unsigned int ddr_clk_src;	/* ddr src rate */
	unsigned int dclk_div;		/* ddr clk divider */

	/* use to record voltage requirement(mV) */
	unsigned int volts;
};

/* RTC/WTC table used for solution change rtc/wtc on the fly */
struct axi_rtcwtc {
	unsigned int max_aclk;	/* max rate could be used by this rtc/wtc */
	unsigned int xtc_val;
};

struct axi_opt {
	unsigned int aclk;		/* axi clock */
	int axi_clk_sel;/* axi src sel val */
	unsigned int axi_clk_src;	/* axi src rate */
	unsigned int aclk_div;		/* axi clk divider */

	unsigned int xtc_val;		/* RTC/WTC value */
	unsigned long xtc_addr;	/* RTC/WTC address */

	/* use to record voltage requirement(mV) */
	unsigned int volts;
};

/* comp that voting for vcore in uboot */
enum vcore_comps {
	CLST0 = 0,
	CLST1,
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
	VL4,
	VL5,
	VL6,
	VL7,
	VL_MAX,
};

enum {
	CORE_1p0G = 1057,
	CORE_1p2G = 1183,
	CORE_1p25G = 1248,
	CORE_1p5G = 1526,
	CORE_1p6G = 1595,
	CORE_1p8G = 1803,
	CORE_2p0G = 2000,
};

enum fab {
	TSMC = 0,
	SEC,
	FAB_MAX,
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
	int ddrtype;
	struct operating_point *op_array;
	unsigned int com_op_size;
	struct cpu_opt *clst0_opt;
	unsigned int clst0_opt_size;
	struct cpu_opt *clst1_opt;
	unsigned int clst1_opt_size;
	struct ddr_opt *ddr_opt;
	unsigned int ddr_opt_size;
	struct axi_opt *axi_opt;
	unsigned int axi_opt_size;

	/* default pll freq used on this platform */
	struct pll_cfg pll_cfg[PLL_MAX];

	/* the default max cpu rate could be supported */
	unsigned int df_max_clst0_rate;
	unsigned int df_max_clst1_rate;
	unsigned int bridge_cpurate;
	/* the plat rule for filter core ops */
	unsigned int (*is_cpuop_invalid_plt)(struct cpu_opt *cop);

	/* dvfs related, voltage table and freq_cmb, filled dynamicly */
	int *vm_millivolts;	/*size VL_MAX */
	unsigned int (*freqs_cmb)[VL_MAX];	/* size VM_RAIL_MAX */
};

#define debug_wt_reg(val, reg)			\
do {						\
	printf(" %08x ==> [%x]\n", val, reg);	\
	__raw_writel(val, reg);			\
} while (0)


/* current platform OP struct */
static struct platform_opt *cur_platform_opt;

static LIST_HEAD(clst0_op_list);
static LIST_HEAD(clst1_op_list);

/* current core OP */
static struct cpu_opt *cur_clst0_op;
static struct cpu_opt *cur_clst1_op;
static struct cpu_opt *clst0_bridge_op;
static struct cpu_opt *clst1_bridge_op;

/* current DDR/AXI OP */
static struct ddr_opt *cur_ddr_op;
static struct axi_opt *cur_axi_op;

static unsigned int clst0_sel2_srcrate(int ap_sel);
static unsigned int clst1_sel2_srcrate(int ap_sel);
static unsigned int ddr_sel2_srcrate(int ddr_sel);
static unsigned int axi_sel2_srcrate(int axi_sel);

static unsigned int cal_comp_volts_req(int comp,
	unsigned int rate);


static int set_volt(u32 vol);
static u32 get_volt(void);

static int hwdfc_enable; /* enable HW-DFC */

/* fuse related */
enum svc_versions {
	SEC_SVC_1_01 = 0,
	SVC_1_11,
	SVC_TSMC_1p8G,
	SVC_TSMC_B0,
	NO_SUPPORT,
};

enum chip_stepping_id {
	TSMC_28LP = 0,
	TSMC_28LP_B020A,
	TSMC_28LP_B121A,
	SEC_28LP_A1,
	SEC_28LP_A1D1A,
	SEC_28LP_A2D2A,
};

static unsigned int __maybe_unused uidro;
static unsigned int __maybe_unused uisvtdro;
static enum chip_stepping_id chip_stepping;
static unsigned int uiprofile;
static unsigned int helan3_maxfreq;
static unsigned int svc_version;
static unsigned int fab_rev;
unsigned int uiYldTableEn;
unsigned int (*freqs_cmb)[VL_MAX];
int *millivolts;
static int min_lv0_voltage;
static int min_cp_voltage;
static int min_gc_voltage;
unsigned int helan3_ddr_mode;
static char dvfs_comp_name[VM_RAIL_MAX][20] = {
	"CORE", "CORE1", "DDR", "AXI",};
int ddr_800M_tsmc_svc[] = {1050, 950, 950, 950, 950, 950, 950, 950, 963,
963, 963, 975, 1000, 1012, 1025, 1050};
int ddr_800M_sec_svc[] = {1075, 975, 975, 975, 975, 975, 975, 975, 975,
975, 975, 975, 1000, 1025, 1050, 1075};


/*
 * used to record dummy boot up PP, define it here
 * as the default src and div value read from register
 * is NOT the real core and ddr,axi rate.
 * This default boot OP is from PP table.
 */
static struct cpu_opt helan3_clst0_bootop = {
	.clst_index = CLST0,
	.pclk = 312,
	.core_aclk = 156,
	.ap_clk_src = 624,
};

static struct cpu_opt helan3_clst1_bootop = {
	.clst_index = CLST1,
	.pclk = 312,
	.core_aclk = 156,
	.ap_clk_src = 1248,
};

static struct ddr_opt helan3_ddr_bootop = {
	.dclk = 156,
	.ddr_clk_src = 624,
};

static struct axi_opt helan3_axi_bootop = {
	.aclk = 104,
	.axi_clk_src = 416,
};

/*
 * For HELAN3:
 * PCLK = AP_CLK_SRC / (CORE_CLK_DIV + 1)
 * CORE_ACLK = PCLK / (BUS_CLK_DIV + 1)
 */
static struct cpu_opt helan3_clst0_op_array[] = {
	{
		.clst_index = CLST0,
		.pclk = 312,
		.core_aclk = 156,
		.ap_clk_sel = C0_CLK_SRC_PLL1_624,
	},
	{
		.clst_index = CLST0,
		.pclk = 416,
		.core_aclk = 208,
		.ap_clk_sel = C0_CLK_SRC_PLL1_832,
	},
	{
		.clst_index = CLST0,
		.pclk = 624,
		.core_aclk = 312,
		.ap_clk_sel = C0_CLK_SRC_PLL1_624,
	},
	{
		.clst_index = CLST0,
		.pclk = 832,
		.core_aclk = 416,
		.ap_clk_sel = C0_CLK_SRC_PLL1_832,
	},
	{
		.clst_index = CLST0,
		.pclk = 1057,
		.core_aclk = 528,
		.ap_clk_sel = C0_CLK_SRC_PLL2,
	},
	{
		.clst_index = CLST0,
		.pclk = 1248,
		.core_aclk = 624,
		.ap_clk_sel = C0_CLK_SRC_PLL1_1248,
	},
};

static struct cpu_opt helan3_clst1_op_array[] = {
	{
		.clst_index = CLST1,
		.pclk = 312,
		.core_aclk = 156,
		.ap_clk_sel = C1_CLK_SRC_PLL1_1248,
	},
	{
		.clst_index = CLST1,
		.pclk = 416,
		.core_aclk = 208,
		.ap_clk_sel = C1_CLK_SRC_PLL1_832,
	},
	{
		.clst_index = CLST1,
		.pclk = 624,
		.core_aclk = 312,
		.ap_clk_sel = C1_CLK_SRC_PLL1_1248,
	},
	{
		.clst_index = CLST1,
		.pclk = 832,
		.core_aclk = 416,
		.ap_clk_sel = C1_CLK_SRC_PLL1_832,
	},
	{
		.clst_index = CLST1,
		.pclk = 1248,
		.core_aclk = 624,
		.ap_clk_sel = C1_CLK_SRC_PLL1_1248,
	},
	{
		.clst_index = CLST1,
		.pclk = 1491,
		.core_aclk = 745,
		.ap_clk_sel = C1_CLK_SRC_PLL3P,
		.ap_clk_src = 1491,
	},
	{
		.clst_index = CLST1,
		.pclk = 1595,
		.core_aclk = 797,
		.ap_clk_sel = C1_CLK_SRC_PLL3P,
		.ap_clk_src = 1595,
	},
	{
		.clst_index = CLST1,
		.pclk = 1803,
		.core_aclk = 901,
		.ap_clk_sel = C1_CLK_SRC_PLL3P,
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
		.dclk = 208,
		.mode_4x_en = 1,
		.ddr_tbl_index = 2,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 312,
		.mode_4x_en = 1,
		.ddr_tbl_index = 4,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 416,
		.mode_4x_en = 1,
		.ddr_tbl_index = 6,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 624,
		.mode_4x_en = 1,
		.ddr_tbl_index = 10,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_1248,
	},
	{
		.dclk = 797,
		.mode_4x_en = 1,
		.ddr_tbl_index = 12,
		.ddr_clk_sel = DDR_CLK_SRC_PLL4,
	},
};

static struct ddr_opt lpddr667_ddr_oparray[] = {
	{
		.dclk = 208,
		.mode_4x_en = 1,
		.ddr_tbl_index = 2,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 312,
		.mode_4x_en = 1,
		.ddr_tbl_index = 4,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_624,
	},
	{
		.dclk = 416,
		.mode_4x_en = 1,
		.ddr_tbl_index = 6,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_832,
	},
	{
		.dclk = 624,
		.mode_4x_en = 1,
		.ddr_tbl_index = 10,
		.ddr_clk_sel = DDR_CLK_SRC_PLL1_1248,
	},
	{
		.dclk = 667,
		.mode_4x_en = 0,
		.ddr_tbl_index = 12,
		.ddr_clk_sel = DDR_CLK_SRC_PLL4,
	},
};

static struct ddr_opt lpddr533_ddr_oparray[] = {
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
};

static struct ddr_opt lpddr400_ddr_oparray[] = {
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
		.dclk = 398,
		.ddr_tbl_index = 6,
		.ddr_clk_sel = DDR_CLK_SRC_PLL2,
	},
};

static struct axi_rtcwtc axi_rtcwtc_tbl[] = {
	{.max_aclk = 208, .xtc_val = 0xEE006656, },
	{.max_aclk = 312, .xtc_val = 0xEE116656, },
};

static struct axi_opt axi_oparray[] = {
	{
		.aclk = 104,
		.axi_clk_sel = AXI_CLK_SRC_PLL1_416,
	},
	{
		.aclk = 156,
		.axi_clk_sel = AXI_CLK_SRC_PLL1_624,
	},
	{
		.aclk = 208,
		.axi_clk_sel = AXI_CLK_SRC_PLL1_416,
	},
	{
		.aclk = 312,
		.axi_clk_sel = AXI_CLK_SRC_PLL1_624,
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
		.cpu_name = "HELAN3",
		.clst0_opt = helan3_clst0_op_array,
		.clst0_opt_size = ARRAY_SIZE(helan3_clst0_op_array),
		.clst1_opt = helan3_clst1_op_array,
		.clst1_opt_size = ARRAY_SIZE(helan3_clst1_op_array),
		.ddrtype = DDR_400M,
		.ddr_opt = lpddr400_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr400_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_clst0_rate = 1248,
		.df_max_clst1_rate = 1803,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {1595, 797, 797},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1491, 1491, 1491},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {1595, 1595, 797},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
	},
	{
		.cpu_name = "HELAN3",
		.clst0_opt = helan3_clst0_op_array,
		.clst0_opt_size = ARRAY_SIZE(helan3_clst0_op_array),
		.clst1_opt = helan3_clst1_op_array,
		.clst1_opt_size = ARRAY_SIZE(helan3_clst1_op_array),
		.ddrtype = DDR_533M,
		.ddr_opt = lpddr533_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr533_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_clst0_rate = 1248,
		.df_max_clst1_rate = 1803,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {2115, 1057, 528},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1491, 1491, 1491},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {1595, 1595, 797},
			.pll_ssc = {0, CENTER_SPREAD, 1000, 25, 100000},
		},
	},
	{
		.cpu_name = "HELAN3",
		.clst0_opt = helan3_clst0_op_array,
		.clst0_opt_size = ARRAY_SIZE(helan3_clst0_op_array),
		.clst1_opt = helan3_clst1_op_array,
		.clst1_opt_size = ARRAY_SIZE(helan3_clst1_op_array),
		.ddrtype = DDR_667M,
		.ddr_opt = lpddr667_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr667_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_clst0_rate = 1248,
		.df_max_clst1_rate = 1803,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {2115, 1057, 528},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1491, 1491, 1491},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {2670, 1335, 667},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
	},
	{
		.cpu_name = "HELAN3",
		.clst0_opt = helan3_clst0_op_array,
		.clst0_opt_size = ARRAY_SIZE(helan3_clst0_op_array),
		.clst1_opt = helan3_clst1_op_array,
		.clst1_opt_size = ARRAY_SIZE(helan3_clst1_op_array),
		.ddrtype = DDR_800M,
		.ddr_opt = lpddr800_ddr_oparray,
		.ddr_opt_size = ARRAY_SIZE(lpddr800_ddr_oparray),
		.axi_opt = axi_oparray,
		.axi_opt_size = ARRAY_SIZE(axi_oparray),
		.df_max_clst0_rate = 1248,
		.df_max_clst1_rate = 1803,
		.bridge_cpurate = 1248,
		.pll_cfg[PLL2] = {
			.pll_rate = {2115, 1057, 528},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL3] = {
			.pll_rate = {1491, 1491, 1491},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
		.pll_cfg[PLL4] = {
			.pll_rate = {1595, 1595, 797},
			.pll_ssc = {0, DOWN_SPREAD, 1000, 25, 100000},
		},
	},
};


static int vm_mv_helan3_svc_tsmc[][VL_MAX] = {
	/*LV0,  LV1,  LV2,  LV3,  LV4,  LV5,  LV6,  LV7 */
	{1000, 1038, 1063, 1113, 1125, 1188, 1263, 1300},/* Profile0 */
	{950, 975, 975, 1000, 1000, 1038, 1025, 1150},/* Profile1 */
	{950, 975, 975, 1000, 1000, 1038, 1038, 1163},/* Profile2 */
	{950, 975, 975, 1000, 1000, 1050, 1050, 1175},/* Profile3 */
	{950, 975, 975, 1000, 1000, 1050, 1050, 1188},/* Profile4 */
	{950, 975, 975, 1000, 1013, 1063, 1063, 1200},/* Profile5 */
	{950, 975, 975, 1000, 1013, 1063, 1075, 1213},/* Profile6 */
	{950, 975, 988, 1013, 1025, 1075, 1088, 1225},/* Profile7 */
	{950, 975, 988, 1013, 1025, 1088, 1100, 1238},/* Profile8 */
	{950, 975, 988, 1025, 1038, 1088, 1113, 1250},/* Profile9 */
	{950, 975, 1000, 1038, 1050, 1113, 1138, 1275},/* Profile10 */
	{950, 988, 1013, 1050, 1063, 1125, 1163, 1288},/* Profile11 */
	{963, 1000, 1025, 1063, 1075, 1150, 1188, 1300},/* Profile12 */
	{975, 1000, 1038, 1075, 1088, 1163, 1213, 1275},/* Profile13 */
	{988, 1013, 1050, 1100, 1100, 1175, 1238, 1275},/* Profile14 */
	{1000, 1038, 1063, 1113, 1125, 1188, 1263, 1300},/* Profile15 */
};

static unsigned int freqs_cmb_helan3_tsmc[][VM_RAIL_MAX][VL_MAX] = {
	[0] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[1] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 624000, 624000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 667000, 667000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[2] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 624000, 624000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[3] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[4] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 832000, 832000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[5] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[6] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[7] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[8] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[9] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[10] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[11] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[12] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[13] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[14] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[15] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */

	},
};

/* 8 VLs PMIC setting */
/* FIXME: adjust according to SVC */
static int vm_mv_helan3_svc_sec[][VL_MAX] = {
	/*LV0,  LV1,  LV2,  LV3,  LV4,  LV5,  LV6,  LV7 */
	{975, 1050, 1063, 1125, 1125, 1200, 1288, 1300},/* Profile0 */
	{975, 988, 988, 1000, 1025, 1150, 1150, 1225},/* Profile1 */
	{975, 988, 988, 1000, 1038, 1150, 1150, 1225},/* Profile2 */
	{975, 988, 988, 1000, 1038, 1150, 1163, 1238},/* Profile3 */
	{975, 988, 988, 1000, 1038, 1150, 1163, 1250},/* Profile4 */
	{975, 988, 988, 1013, 1038, 1150, 1175, 1263},/* Profile5 */
	{975, 988, 988, 1013, 1038, 1150, 1175, 1263},/* Profile6 */
	{975, 988, 988, 1013, 1038, 1150, 1188, 1275},/* Profile7 */
	{975, 988, 988, 1025, 1050, 1150, 1188, 1288},/* Profile8 */
	{975, 988, 988, 1025, 1050, 1150, 1200, 1288},/* Profile9 */
	{975, 988, 1000, 1050, 1063, 1150, 1213, 1300},/* Profile10 */
	{975, 988, 1013, 1050, 1075, 1150, 1225, 1238},/* Profile11 */
	{975, 988, 1025, 1063, 1075, 1150, 1238, 1250},/* Profile12 */
	{975, 1000, 1038, 1075, 1088, 1163, 1250, 1275},/* Profile13 */
	{975, 1013, 1050, 1100, 1100, 1175, 1263, 1300},/* Profile14 */
	{975, 1025, 1063, 1125, 1125, 1200, 1288, 1288},/* Profile15 */
};

static unsigned int freqs_cmb_helan3_sec[][VM_RAIL_MAX][VL_MAX] = {
	[0] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 312000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 1057000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[1] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 624000, 624000, 624000, 624000, 1057000, 1057000, 1248000}, /* CLUSTER0 */
		{832000, 832000, 832000, 832000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[2] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 624000, 624000, 624000, 624000, 1057000, 1057000, 1248000}, /* CLUSTER0 */
		{832000, 832000, 832000, 832000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[3] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 1057000, 1057000, 1248000}, /* CLUSTER0 */
		{832000, 832000, 832000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[4] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 832000, 1057000, 1248000}, /* CLUSTER0 */
		{832000, 832000, 832000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[5] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 832000, 1057000, 1248000}, /* CLUSTER0 */
		{624000, 832000, 832000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[6] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 832000, 1057000, 1248000}, /* CLUSTER0 */
		{624000, 832000, 832000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[7] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 832000, 1057000, 1248000}, /* CLUSTER0 */
		{624000, 624000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 208000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[8] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{624000, 624000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[9] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{416000, 416000, 416000, 624000, 624000, 832000, 1057000, 1248000}, /* CLUSTER0 */
		{416000, 624000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 208000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[10] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 416000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[11] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 312000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 416000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 208000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[12] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 312000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 312000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 208000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[13] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 312000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 312000, 624000, 832000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[14] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 312000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 312000, 624000, 1057000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[15] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 312000, 416000, 624000, 624000, 1248000, 1248000, 1248000}, /* CLUSTER0 */
		{312000, 312000, 624000, 1057000, 1057000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 667000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
};


static unsigned int freqs_cmb_helan3_tsmc_b0[][VM_RAIL_MAX][VL_MAX] = {
	[0] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[1] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 624000, 624000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 667000, 667000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[2] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 624000, 624000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[3] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[4] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 832000, 832000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[5] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 1057000, 1491000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[6] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[7] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[8] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[9] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[10] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[11] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[12] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[13] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[14] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[15] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1491000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
};


static int vm_mv_helan3_svc_tsmc_b0[][VL_MAX] = {
	/*LV0,  LV1,  LV2,  LV3,  LV4,  LV5,  LV6,  LV7 */
	{1000, 1038, 1063, 1113, 1125, 1188, 1263, 1300},/* Profile0 */
	{950, 975, 975, 1000, 1000, 1038, 1025, 1150},/* Profile1 */
	{950, 975, 975, 1000, 1000, 1038, 1038, 1163},/* Profile2 */
	{950, 975, 975, 1000, 1000, 1050, 1050, 1175},/* Profile3 */
	{950, 975, 975, 1000, 1000, 1050, 1050, 1188},/* Profile4 */
	{950, 975, 975, 1000, 1013, 1063, 1063, 1200},/* Profile5 */
	{950, 975, 975, 1000, 1013, 1063, 1075, 1213},/* Profile6 */
	{950, 975, 988, 1013, 1025, 1075, 1088, 1225},/* Profile7 */
	{950, 975, 988, 1013, 1025, 1088, 1100, 1238},/* Profile8 */
	{950, 975, 988, 1025, 1038, 1088, 1113, 1250},/* Profile9 */
	{950, 975, 1000, 1038, 1050, 1113, 1138, 1275},/* Profile10 */
	{950, 988, 1013, 1050, 1063, 1125, 1163, 1288},/* Profile11 */
	{963, 1000, 1025, 1063, 1075, 1150, 1188, 1300},/* Profile12 */
	{975, 1000, 1038, 1075, 1088, 1163, 1213, 1275},/* Profile13 */
	{988, 1013, 1050, 1100, 1100, 1175, 1238, 1275},/* Profile14 */
	{1000, 1038, 1063, 1113, 1125, 1188, 1263, 1300},/* Profile15 */
};


static int vm_mv_helan3_svc_tsmc_1p8G[][VL_MAX] = {
	/*LV0,  LV1,  LV2,  LV3,  LV4,  LV5,  LV6,  LV7 */
	{1000, 1038, 1063, 1113, 1125, 1188, 1263, 1300},/* Profile0 */
	{950, 975, 975, 1000, 1000, 1038, 1025, 1175},/* Profile1 */
	{950, 975, 975, 1000, 1000, 1038, 1038, 1188},/* Profile2 */
	{950, 975, 975, 1000, 1000, 1050, 1050, 1200},/* Profile3 */
	{950, 975, 975, 1000, 1000, 1050, 1050, 1213},/* Profile4 */
	{950, 975, 975, 1000, 1013, 1063, 1063, 1225},/* Profile5 */
	{950, 975, 975, 1000, 1013, 1063, 1075, 1238},/* Profile6 */
	{950, 975, 988, 1013, 1025, 1075, 1088, 1250},/* Profile7 */
	{950, 975, 988, 1013, 1025, 1088, 1100, 1250},/* Profile8 */
	{950, 975, 988, 1025, 1038, 1088, 1113, 1263},/* Profile9 */
	{950, 975, 1000, 1038, 1050, 1113, 1138, 1288},/* Profile10 */
	{950, 988, 1013, 1050, 1063, 1125, 1163, 1300},/* Profile11 */
	{963, 1000, 1025, 1063, 1075, 1150, 1188, 1300},/* Profile12 */
	{975, 1000, 1038, 1075, 1088, 1163, 1213, 1275},/* Profile13 */
	{988, 1013, 1050, 1100, 1100, 1175, 1238, 1288},/* Profile14 */
	{1000, 1038, 1063, 1113, 1125, 1188, 1263, 1300},/* Profile15 */
};


static unsigned int freqs_cmb_1936_tsmc_1p8G[][VM_RAIL_MAX][VL_MAX] = {
	[0] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[1] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 624000, 624000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 667000, 667000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[2] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 624000, 624000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[3] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 1057000, 1057000, 1491000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[4] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 832000, 832000, 832000, 832000, 1491000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 312000, 312000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[5] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 1057000, 1491000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[6] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[7] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[8] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[9] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[10] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[11] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[12] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{0, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 667000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[13] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[14] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */
	},
	[15] = {
		/* LV0,    LV1,    LV2,    LV3,    LV4,    LV5,    LV6,    LV7 */
		{312000, 416000, 416000, 624000, 624000, 832000, 832000, 1248000}, /* CLUSTER0 */
		{312000, 624000, 624000, 832000, 832000, 1248000, 1491000, 1803000}, /* CLUSTER1 */
		{624000, 624000, 624000, 624000, 624000, 624000, 667000, 797000}, /* DDR */
		{208000, 208000, 208000, 312000, 312000, 312000, 312000, 312000}, /* AXI */

	},
};

/* FIXME: must remove the table after has fuse info !!! */
struct svtrng {
	unsigned int min;
	unsigned int max;
	unsigned int profile;
};

static struct svtrng svtrngtb[] = {
	{290, 310, 15},
	{311, 322, 14},
	{323, 335, 13},
	{336, 348, 12},
	{349, 360, 11},
	{361, 373, 10},
	{374, 379, 9},
	{380, 386, 8},
	{387, 392, 7},
	{393, 398, 6},
	{399, 405, 5},
	{406, 411, 4},
	{412, 417, 3},
	{418, 424, 2},
	{425, 440, 1},
};

void convert_max_freq(unsigned int uiCpuFreq)
{
	switch (uiCpuFreq) {
	case 0x0:
	case 0x5:
	case 0x6:
		printf("%s Part SKU is 1.5GHz; FuseBank0[179:174] = 0x%X\n", __func__, uiCpuFreq);
		helan3_maxfreq = CORE_1p5G;
		break;
	case 0x1:
	case 0xA:
		printf("%s Part SKU is 1.8GHz; FuseBank0[179:174] = 0x%X\n", __func__, uiCpuFreq);
		helan3_maxfreq = CORE_1p8G;
		break;
	case 0x2:
	case 0x3:
		printf("%s Part SKU is 2GHz; FuseBank0[179:174] = 0x%X\n", __func__, uiCpuFreq);
		helan3_maxfreq = CORE_2p0G;
		break;
	default:
		printf("%s ERROR: Fuse value (0x%X) not supported,default max freq 1.5G\n",
		__func__, uiCpuFreq);
		helan3_maxfreq = CORE_1p5G;
		break;
	}
	return;
}

unsigned int get_helan3_max_freq(void)
{
	return helan3_maxfreq;
}

unsigned int convert_svc_voltage(unsigned int uiGc3d_Vlevel,
	unsigned int uiLPP_LoPP_Profile, unsigned int uiPCPP_Profile)
{
	switch (uiLPP_LoPP_Profile) {
	case 0:
		min_lv0_voltage = 950;
		break;
	case 1:
		min_lv0_voltage = 975;
		break;
	case 3:
		min_lv0_voltage = 1025;
		break;
	default:
		min_lv0_voltage = 950;
		break;
	}
	switch (uiPCPP_Profile) {
	case 0:
		min_cp_voltage = 963;
		break;
	case 1:
		min_cp_voltage = 988;
		break;
	case 3:
		min_cp_voltage = 1038;
		break;
	default:
		min_cp_voltage = 963;
		break;
	}
	switch (uiGc3d_Vlevel) {
	case 0:
		min_gc_voltage = 950;
		break;
	case 1:
		min_gc_voltage = 975;
		break;
	case 3:
		min_gc_voltage = 1050;
		break;
	default:
		min_gc_voltage = 950;
		break;
	}

	return 0;
}

unsigned int convert_svc_version(unsigned int uiSVCRev, unsigned int uiFabRev)
{
	if (uiSVCRev == 0) {
		if (!uiYldTableEn) {
			svc_version = SEC_SVC_1_01;
		} else {
			if (uiFabRev == 0)
				svc_version = SVC_1_11;
			else if (uiFabRev == 1)
				svc_version = SEC_SVC_1_01;
			}
	} else if (uiSVCRev == 1) {
		if (uiFabRev == 0)
			svc_version = NO_SUPPORT;
		else if (uiFabRev == 1)
			svc_version = SVC_1_11;
	} else {
			svc_version = NO_SUPPORT;
	}

	if (helan3_maxfreq == CORE_1p8G)
		svc_version = SVC_TSMC_1p8G;

	if ((chip_stepping == TSMC_28LP_B020A) ||
		(chip_stepping == TSMC_28LP_B121A))
		svc_version = SVC_TSMC_B0;

	return svc_version;
}

char *convert_step_revision(unsigned int step_id)
{
	if (fab_rev == TSMC) {
		if (step_id == 0x0) {
			chip_stepping = TSMC_28LP;
			return "TSMC 28LP";
		} else if (step_id == 0x1) {
			chip_stepping = TSMC_28LP_B020A;
			return "TSMC 28LP_B020A";
		} else if (step_id == 0x2) {
			chip_stepping = TSMC_28LP_B121A;
			return "TSMC 28LP_B121A";
		}
	} else if (fab_rev == SEC) {
		if (step_id == 0x0) {
			chip_stepping = SEC_28LP_A1;
			return "SEC 28LP_A1";
		} else if (step_id == 0x1) {
			chip_stepping = SEC_28LP_A1D1A;
			return "SEC 28LP_A1D1A";
		} else if (step_id == 0x2) {
			chip_stepping = SEC_28LP_A2D2A;
			return "SEC 28LP_A2D2A";
		}
	}
	return "not SEC and TSMC chip";
}


static u32 convert_svtdro2profile(unsigned int uisvtdro)
{
	unsigned int ui_profile = 0, idx;

	if (uisvtdro >= 290 && uisvtdro <= 440) {
		for (idx = 0; idx < ARRAY_SIZE(svtrngtb); idx++) {
			if (uisvtdro >= svtrngtb[idx].min &&
				uisvtdro <= svtrngtb[idx].max) {
				ui_profile = svtrngtb[idx].profile;
				break;
			}
		}
	} else {
		ui_profile = 0;
		printf("SVTDRO is either not programmed or outside of the SVC spec range: %d",
			uisvtdro);
	}

	printf("%s uisvtdro[%d]->profile[%d]\n",
		__func__, uisvtdro, ui_profile);
	return ui_profile;
}

unsigned int convertFusesToProfile_helan3(unsigned int uifuses)
{
	unsigned int ui_profile = 0;
	unsigned int uitemp = 1, uitemp2 = 1;
	int i;

	for (i = 1; i < NUM_PROFILES; i++) {
		if (uitemp == uifuses)
			ui_profile = i;
		uitemp |= uitemp2 << (i);
	}

	printf("%s uiFuses[0x%x]->profile[%d]\n",
		__func__, uifuses, ui_profile);
	return ui_profile;
}

unsigned int convert_fab_revision(unsigned int fab_revision)
{
	unsigned int ui_fab = TSMC;
	if (fab_revision == 0)
		ui_fab = TSMC;
	else if (fab_revision == 1)
		ui_fab = SEC;

	return ui_fab;
}

static void __init_read_ultinfo(void)
{
	/*
	 * Read out DRO value, need enable GEU clock, if already disable,
	 * need enable it firstly
	 */
	unsigned int __maybe_unused uigeustatus = 0;
	unsigned int uiProfileFuses, uiSVCRev, uiFabRev, guiProfile;
	unsigned int uiBlock0_GEU_FUSE_MANU_PARA_0, uiBlock0_GEU_FUSE_MANU_PARA_1;
	unsigned int uiBlock0_GEU_FUSE_MANU_PARA_2, uiBlock0_BLOCK0_RESERVED_1;
	unsigned int uiBlock4_MANU_PARA1_0, uiBlock4_MANU_PARA1_1;
	unsigned int uiAllocRev, uiFab, uiRun, uiWafer, uiX, uiY, uiParity;
	unsigned int uiLVTDRO_Avg, uiSVTDRO_Avg, uiSIDD1p05 = 0, uiSIDD1p30 = 0;
	unsigned int uiCpuFreq;
	unsigned int uiskusetting = 0, uiGc3d_Vlevel, ui_step_id;
	unsigned int uiLPP_LoPP_Profile, uiPCPP_Profile, uiNegEdge;
	unsigned int smc_ret = 0;
	struct fuse_info arg;

	uigeustatus = __raw_readl(APMU_GEU);
	if (!(uigeustatus & 0x30)) {
		__raw_writel((uigeustatus | 0x30), APMU_GEU);
		udelay(10);
	}

	smc_ret = smc_get_fuse_info(0xc2003000, (void *)&arg);
	if (smc_ret == 0) {
		/* GEU_FUSE_MANU_PARA_0	0x110	Bank 0 [127: 96] */
		uiBlock0_GEU_FUSE_MANU_PARA_0 = arg.arg0;
		/* GEU_FUSE_MANU_PARA_1	0x114	Bank 0 [159:128] */
		uiBlock0_GEU_FUSE_MANU_PARA_1 = arg.arg1;
		/* GEU_FUSE_MANU_PARA_2	0x118	Bank 0 [191:160] */
		uiBlock0_GEU_FUSE_MANU_PARA_2 = arg.arg2;
		/* GEU_AP_CP_MP_ECC		0x11C	Bank 0 [223:192] */
		/* BLOCK0_RESERVED_1 0x120	Bank 0 [255:224] */
		uiBlock0_BLOCK0_RESERVED_1 = arg.arg4;
		/* Fuse Block 4 191:160 */
		uiBlock4_MANU_PARA1_0 = arg.arg5;
		/* Fuse Block 4 255:192 */
		uiBlock4_MANU_PARA1_1  = arg.arg6;
	}

	else {
		uiBlock0_GEU_FUSE_MANU_PARA_0 = __raw_readl(GEU_FUSE_MANU_PARA_0);
		uiBlock0_GEU_FUSE_MANU_PARA_1 = __raw_readl(GEU_FUSE_MANU_PARA_1);
		uiBlock0_GEU_FUSE_MANU_PARA_2 = __raw_readl(GEU_FUSE_MANU_PARA_2);
		uiBlock0_BLOCK0_RESERVED_1    = __raw_readl(BLOCK0_RESERVED_1);
		uiBlock4_MANU_PARA1_0         = __raw_readl(BLOCK4_MANU_PARA1_0);
		uiBlock4_MANU_PARA1_1         = __raw_readl(BLOCK4_MANU_PARA1_1);
	}
	__raw_writel(uigeustatus, APMU_GEU);

	uiAllocRev = uiBlock0_GEU_FUSE_MANU_PARA_0 & 0x7;
	uiFab = (uiBlock0_GEU_FUSE_MANU_PARA_0 >>  3) & 0x1f;
	uiRun = ((uiBlock0_GEU_FUSE_MANU_PARA_1 & 0x3) << 24) |
		((uiBlock0_GEU_FUSE_MANU_PARA_0 >> 8) & 0xffffff);
	uiWafer = (uiBlock0_GEU_FUSE_MANU_PARA_1 >>  2) & 0x1f;
	uiX = (uiBlock0_GEU_FUSE_MANU_PARA_1 >>  7) & 0xff;
	uiY = (uiBlock0_GEU_FUSE_MANU_PARA_1 >> 15) & 0xff;
	uiParity = (uiBlock0_GEU_FUSE_MANU_PARA_1 >> 23) & 0x1;
	uiSVCRev = (uiBlock0_BLOCK0_RESERVED_1    >> 13) & 0x3;
	uiFabRev = (uiBlock0_BLOCK0_RESERVED_1    >> 11) & 0x3;
	uiSVTDRO_Avg = ((uiBlock0_GEU_FUSE_MANU_PARA_2 & 0x3) << 8) |
			((uiBlock0_GEU_FUSE_MANU_PARA_1 >> 24) & 0xff);
	uiLVTDRO_Avg = (uiBlock0_GEU_FUSE_MANU_PARA_2 >>  4) & 0x3ff;
	uiProfileFuses = (uiBlock0_BLOCK0_RESERVED_1    >> 16) & 0xffff;
	uiSIDD1p05 = uiBlock4_MANU_PARA1_0 & 0x3ff;
	uiSIDD1p30 = ((uiBlock4_MANU_PARA1_1 & 0x3) << 8) |
		((uiBlock4_MANU_PARA1_0 >> 24) & 0xff);

	uiCpuFreq = (uiBlock0_GEU_FUSE_MANU_PARA_2 >>  14) & 0x3f;
	uiFabRev = (uiBlock4_MANU_PARA1_1 >> 4) & 0x3;
	fab_rev = convert_fab_revision(uiFabRev);
	/*bit 201 ~ 202 for UDR voltage*/
	uiskusetting = (uiBlock4_MANU_PARA1_1 >> 9) & 0x3;
	ui_step_id = (uiBlock0_GEU_FUSE_MANU_PARA_2 >> 2) & 0x3;
	uiSVCRev = (uiBlock4_MANU_PARA1_1 >> 6) & 0x3;
	uiYldTableEn = (uiBlock0_BLOCK0_RESERVED_1 >> 9) & 0x1;
	convert_max_freq(uiCpuFreq);
	convert_step_revision(ui_step_id);
	convert_svc_version(uiSVCRev, uiFabRev);

	if (uiYldTableEn == 1) {
		uiGc3d_Vlevel = (uiBlock0_BLOCK0_RESERVED_1 >> 10) & 0x3;
		uiLPP_LoPP_Profile = (uiBlock0_BLOCK0_RESERVED_1 >> 12) & 0x3;
		uiPCPP_Profile = (uiBlock0_BLOCK0_RESERVED_1 >> 14) & 0x3;
		convert_svc_voltage(uiGc3d_Vlevel, uiLPP_LoPP_Profile,
			uiPCPP_Profile);

		uiNegEdge = (uiBlock0_BLOCK0_RESERVED_1 >> 31) & 0x1;
		/* bit 0 = 700mv,bit 1 = 800mv, opposite*/
		uiskusetting = !uiNegEdge;
	}

	guiProfile = convertFusesToProfile_helan3(uiProfileFuses);

	if (guiProfile == 0)
		guiProfile = convert_svtdro2profile(uiSVTDRO_Avg);

	printf(" \n");
	printf("     *************************** \n");
	printf("     *  ULT: %08X%08X  * \n", uiBlock0_GEU_FUSE_MANU_PARA_1,
		uiBlock0_GEU_FUSE_MANU_PARA_0);
	printf("     *************************** \n");
	printf("     ULT decoded below \n");
	printf("     alloc_rev = %d\n", uiAllocRev);
	printf("           fab = %d\n",      uiFab);
	printf("           run = %d (0x%07X)\n", uiRun, uiRun);
	printf("         wafer = %d\n",    uiWafer);
	printf("             x = %d\n",        uiX);
	printf("             y = %d\n",        uiY);
	printf("        parity = %d\n",   uiParity);
	printf("        UDR voltage bit = %d\n", uiskusetting);
	printf("     *************************** \n");
	if (0 == uiFabRev)
		printf("     *  Fab   = TSMC 28LP (%d)\n",    uiFabRev);
	else if (1 == uiFabRev)
		printf("     *  Fab   = SEC 28LP (%d)\n",    uiFabRev);
	else
		printf("     *  FabRev (%d) not currently supported\n",    uiFabRev);
	printf("     *  ui_step_id is %d\n", ui_step_id);
	printf("     *  chip_stepping is %d\n", chip_stepping);
	printf("     *  chip step is %s\n", convert_step_revision(ui_step_id));
	printf("     *  wafer = %d\n", uiWafer);
	printf("     *  x     = %d\n",     uiX);
	printf("     *  y     = %d\n",     uiY);
	printf("     *************************** \n");
	printf("     *  Iddq @ 1.05V = %dmA\n",   uiSIDD1p05);
	printf("     *  Iddq @ 1.30V = %dmA\n",   uiSIDD1p30);
	printf("     *************************** \n");
	printf("     *  LVTDRO = %d\n",   uiLVTDRO_Avg);
	printf("     *  SVTDRO = %d\n",   uiSVTDRO_Avg);
	printf("     *  SVC Revision = %2d\n", uiSVCRev);
	printf("     *  SVC Profile  = %2d\n", guiProfile);
	printf("     *  SVC Table Version  = %2d\n", svc_version);
	printf("     *************************** \n");
	printf(" \n");
	uiprofile = guiProfile;

}

int get_ddr_800M_4x(void)
{
	int i;
	if (helan3_ddr_mode == DDR_800M) {
		/* change DDR800 4x mode by default to 2x mode */
		i =  ARRAY_SIZE(lpddr800_ddr_oparray) - 1;
		if ((lpddr800_ddr_oparray[i].dclk == 797) &&
			(lpddr800_ddr_oparray[i].mode_4x_en == 1))
			return 1;
	}
	return 0;
}


void adjust_ddr_svc(void)
{
	int i, ddr_voltage  = 0;
	if (svc_version == SVC_1_11 || svc_version == SVC_TSMC_B0
		|| svc_version == SVC_TSMC_1p8G)
		ddr_voltage = ddr_800M_tsmc_svc[uiprofile];
	else if (svc_version == SEC_SVC_1_01)
		ddr_voltage = ddr_800M_sec_svc[uiprofile];
	if (get_ddr_800M_4x())
		for (i = 0; i < VL_MAX; i++)
			if ((freqs_cmb[DDR][i] != 0) &&
			(millivolts[i] >= ddr_voltage))
				freqs_cmb[DDR][i] = 797000;

	return;
}


void dump_dvc_table(void)
{
	int i, j;
	printf(
	"=====dump svc voltage and freq table===\nsvc table voltage:\n");
	for (i = 0; i < VL_MAX; i++)
		printf("%d ", millivolts[i]);
	printf(
	"\n========================================\nsvc table freq table:\n");
	for (i = 0; i < VM_RAIL_MAX; i++) {
		printf("%s: ", dvfs_comp_name[i]);
		for (j = 0; j < VL_MAX; j++)
			printf("%u ", freqs_cmb[i][j]);
		printf("\n");
	}
	printf("========================================\n");

	return;
}


int handle_svc_table(void)
{
	int i, j, lv0_change_index = 0;

	if (svc_version == SVC_1_11) {
		millivolts = vm_mv_helan3_svc_tsmc[uiprofile];
		freqs_cmb = freqs_cmb_helan3_tsmc[uiprofile];
	} else if (svc_version == SEC_SVC_1_01) {
		millivolts = vm_mv_helan3_svc_sec[uiprofile];
		freqs_cmb = freqs_cmb_helan3_sec[uiprofile];
	} else if (svc_version == SVC_TSMC_1p8G) {
		millivolts = vm_mv_helan3_svc_tsmc_1p8G[uiprofile];
		freqs_cmb = freqs_cmb_1936_tsmc_1p8G[uiprofile];
	} else if (svc_version == SVC_TSMC_B0) {
		millivolts = vm_mv_helan3_svc_tsmc_b0[uiprofile];
		freqs_cmb = freqs_cmb_helan3_tsmc_b0[uiprofile];
	}

	if (uiYldTableEn == 1) {
		/* Change LV0 voltage according to fuse value */
		if (min_lv0_voltage >= millivolts[0]) {
			for (i = 0; i < VL_MAX; i++)
				if (min_lv0_voltage >= millivolts[i])
					millivolts[i] = min_lv0_voltage;
				else
					break;

			lv0_change_index = i - 1;
		}

		/*	Change other components(without SDH) frequency
		 *	combination table if LV0 is changed.
		 */
		if ((lv0_change_index) != 0) {
			for (j = 0; j < lv0_change_index; j++)
				for (i = 0; i < VM_RAIL_MAX; i++)
					freqs_cmb[i][j] =
					freqs_cmb[i][lv0_change_index];
		}
	}

	adjust_ddr_svc();

	dump_dvc_table();

	return 0;
}


/* turn on pll at defrate */
static void turn_on_pll_defrate(int pll)
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
	helan3_ddr_mode = ddr_mode;
	if (ddr_mode >= DDR_TYPE_MAX)
		ddr_mode = DDR_533M;
	if (ddr_mode == DDR_800M_2X) {
		/* change DDR800 4x mode by default to 2x mode */
		i =  ARRAY_SIZE(lpddr800_ddr_oparray) - 1;
		if (lpddr800_ddr_oparray[i].dclk == 797)
			lpddr800_ddr_oparray[i].mode_4x_en = 0;
		/*
		 * 800Mhz 2x/4x share one op table,
		 * so need update ddrtype for 800Mhz 2x manually here.
		 */
		proc = platform_op_arrays + 3;
		proc->ddrtype = DDR_800M_2X;
	}

	for (i = 0; i < ARRAY_SIZE(platform_op_arrays); i++) {
		proc = platform_op_arrays + i;
		if ((proc->chipid == chipid) && (proc->ddrtype == ddr_mode))
			break;
	}
	BUG_ON(i == ARRAY_SIZE(platform_op_arrays));
	cur_platform_opt = proc;
	if (svc_version == SVC_1_11 || svc_version == SVC_TSMC_B0) {
		if ((uiprofile >= 13) && (ddr_mode == DDR_800M_2X) && (helan3_maxfreq == CORE_1p5G))
			panic("1.5GHz SKU chip Don't support DDR 800 mode when profile >= 13 , will panic.\n");

		if ((uiprofile >= 13) && (helan3_maxfreq == CORE_1p5G)) {
			cur_platform_opt->df_max_clst0_rate = CORE_1p0G;
			printf("1.5GHz SKU chip clst0 support max freq is 1057M when profile >= 13\n");
		}

		if ((uiprofile >= 12) && (helan3_maxfreq == CORE_1p5G)) {
			cur_platform_opt->df_max_clst1_rate = CORE_1p6G;
			printf("1.5GHz SKU chip clst1 support max freq is 1595M when profile >= 12\n");
		}
	} else if (svc_version == SEC_SVC_1_01) {
		if ((uiprofile >= 4) && (ddr_mode == DDR_800M_2X) && (helan3_maxfreq == CORE_1p5G))
			panic("1.5GHz SKU chip Don't support DDR 800 mode when profile >= 4 , will panic.\n");

		if ((uiprofile >= 11) && (helan3_maxfreq == CORE_1p5G)) {
			cur_platform_opt->df_max_clst0_rate = CORE_1p0G;
			printf("1.5GHz SKU chip clst0 support max freq is 1057M when profile >= 11\n");
		}
		if ((uiprofile >= 13) && (helan3_maxfreq == CORE_1p5G)) {
			cur_platform_opt->df_max_clst1_rate = CORE_1p5G;
			printf("1.5GHz SKU chip clst1 support max freq is 1526M when profile >= 13\n");
		} else if (helan3_maxfreq == CORE_1p5G) {
			cur_platform_opt->df_max_clst1_rate = CORE_1p6G;
			printf("1.5GHz SKU chip clst1 support max freq is 1595M\n");
		}
	}
}

static struct cpu_rtcwtc clst0_cpu_rtcwtc_tbl[] = {
	{.max_pclk = 416, .l1_xtc = 0x11111111, .l2_xtc = 0x00001111, },
	{.max_pclk = 832, .l1_xtc = 0x55555555, .l2_xtc = 0x00005555, },
	{.max_pclk = 1057, .l1_xtc = 0x55555555, .l2_xtc = 0x0000555A, },
	{.max_pclk = 1248, .l1_xtc = 0xAAAAAAAA, .l2_xtc = 0x0000AAAA, },
};

static struct cpu_rtcwtc clst1_cpu_rtcwtc_tbl[] = {
	{.max_pclk = 416, .l1_xtc = 0x11111111, .l2_xtc = 0x00001111, },
	{.max_pclk = 832, .l1_xtc = 0x55555555, .l2_xtc = 0x00005555, },
	{.max_pclk = 1057, .l1_xtc = 0x55555555, .l2_xtc = 0x0000555A, },
	{.max_pclk = 1803, .l1_xtc = 0xAAAAAAAA, .l2_xtc = 0x0000AAAA, },
};

/* param - 'write' = 1 means write rtc/wtc to reg for this op. */
static void __init_cpu_rtcwtc(struct cpu_opt *cpu_opt, int write)
{
	struct cpu_rtcwtc *cpu_rtcwtc;
	unsigned int size, index;

	if (cpu_opt->clst_index == CLST0) {
		cpu_rtcwtc = clst0_cpu_rtcwtc_tbl;
		size = ARRAY_SIZE(clst0_cpu_rtcwtc_tbl);
	} else {
		cpu_rtcwtc = clst1_cpu_rtcwtc_tbl;
		size = ARRAY_SIZE(clst1_cpu_rtcwtc_tbl);
	}

	for (index = 0; index < size; index++)
		if (cpu_opt->pclk <= cpu_rtcwtc[index].max_pclk)
			break;
	if (index == size)
		index = size - 1;

	cpu_opt->l1_xtc = cpu_rtcwtc[index].l1_xtc;
	cpu_opt->l2_xtc = cpu_rtcwtc[index].l2_xtc;
	cpu_opt->l1_xtc_addr = DCIU_CLST_L1(cpu_opt->clst_index);
	cpu_opt->l2_xtc_addr = DCIU_CLST_L2(cpu_opt->clst_index);

	if (write) {
		__raw_writel(cpu_opt->l1_xtc, cpu_opt->l1_xtc_addr);
		__raw_writel(cpu_opt->l2_xtc, cpu_opt->l2_xtc_addr);
	}
};

/* Common condition here if you want to filter the core ops */
static unsigned int __is_cpu_op_invalid_com(struct cpu_opt *cop)
{
	unsigned int df_max_cpurate = 0;

	if (cop->clst_index == CLST0)
		df_max_cpurate = cur_platform_opt->df_max_clst0_rate;
	else if (cop->clst_index == CLST1)
		df_max_cpurate = cur_platform_opt->df_max_clst1_rate;

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

static void __init_cpu_opt(int clst_index)
{
	struct cpu_opt *cpu_opt = NULL, *cop = NULL;
	unsigned int cpu_opt_size = 0, i, idx = 0;

	if (clst_index == CLST0) {
		cpu_opt = cur_platform_opt->clst0_opt;
		cpu_opt_size = cur_platform_opt->clst0_opt_size;
	} else if (clst_index == CLST1) {
		cpu_opt = cur_platform_opt->clst1_opt;
		cpu_opt_size = cur_platform_opt->clst1_opt_size;
	}

	debug("pclk(src:sel,div) core_aclk(div)\n");

	for (i = 0; i < cpu_opt_size; i++) {
		cop = &cpu_opt[i];
		if ((clst_index == CLST0) && (!cop->ap_clk_src))
			cop->ap_clk_src = clst0_sel2_srcrate(cop->ap_clk_sel);
		else if ((clst_index == CLST1) && (!cop->ap_clk_src))
			cop->ap_clk_src = clst1_sel2_srcrate(cop->ap_clk_sel);

		BUG_ON(cop->ap_clk_src < 0);
		/* check the invalid condition of this op */
		if (__is_cpu_op_invalid_com(cop))
			continue;
		if (__is_cpu_op_invalid_plt(cop))
			continue;
		/* add it into core op list */
		if (cop->clst_index == CLST0) {
			list_add_tail(&cop->node, &clst0_op_list);
			if (cop->pclk == cur_platform_opt->bridge_cpurate)
				clst0_bridge_op = cop;
		} else if (cop->clst_index == CLST1) {
			list_add_tail(&cop->node, &clst1_op_list);
			if (cop->pclk == cur_platform_opt->bridge_cpurate)
				clst1_bridge_op = cop;
		}
		idx++;

		/* fill the opt related setting */
		__init_cpu_rtcwtc(cop, 0);
		cop->pclk_div = cop->ap_clk_src / cop->pclk - 1;
		cop->core_aclk_div = cop->pclk / cop->core_aclk - 1;

		debug("%d(%d:%d,%d)\t%d(%d)\n",
		      cop->pclk, cop->ap_clk_src,
		      cop->ap_clk_sel & AP_SRC_SEL_MASK,
		      cop->pclk_div,
		      cop->core_aclk,
		      cop->core_aclk_div);

		/* calc this op's voltage req */
		cop->volts = cal_comp_volts_req(cop->clst_index, cop->pclk);
	}
	/* override cpu_opt_size with real support tbl */
	if (cop->clst_index == CLST0)
		cur_platform_opt->clst0_opt_size = idx;
	else if (cop->clst_index == CLST1)
		cur_platform_opt->clst1_opt_size = idx;
}

/* param - 'write' = 1 means write rtc/wtc to reg for this op. */
static void __init_axi_rtcwtc(struct axi_opt *axi_opt, int write)
{
	struct axi_rtcwtc *axi_rtcwtc_table;
	unsigned int size, index;

	axi_rtcwtc_table = axi_rtcwtc_tbl;
	size = ARRAY_SIZE(axi_rtcwtc_tbl);

	for (index = 0; index < size; index++)
		if (axi_opt->aclk <= axi_rtcwtc_table[index].max_aclk)
			break;
	if (index == size)
		index = size - 1;

	axi_opt->xtc_val = axi_rtcwtc_table[index].xtc_val;
	axi_opt->xtc_addr = (unsigned long)CIU_TOP_MEM_XTC;

	if (write)
		__raw_writel(axi_opt->xtc_val, axi_opt->xtc_addr);
}

static void __init_ddr_axi_opt(void)
{
	struct ddr_opt *ddr_opt, *ddr_cop;
	struct axi_opt *axi_opt, *axi_cop;
	union dfc_level_reg value;
	unsigned int ddr_opt_size = 0, axi_opt_size = 0, i;

	ddr_opt = cur_platform_opt->ddr_opt;
	ddr_opt_size = cur_platform_opt->ddr_opt_size;
	axi_opt = cur_platform_opt->axi_opt;
	axi_opt_size = cur_platform_opt->axi_opt_size;

	debug("dclk(src:sel,div)\n");
	for (i = 0; i < ddr_opt_size; i++) {
		ddr_cop = &ddr_opt[i];
		ddr_cop->ddr_freq_level = i;
		ddr_cop->ddr_clk_src = ddr_sel2_srcrate(ddr_cop->ddr_clk_sel);
		BUG_ON(ddr_cop->ddr_clk_src < 0);
		ddr_cop->dclk_div =
			ddr_cop->ddr_clk_src / (2 * ddr_cop->dclk) - 1;
		debug("%d(%d:%d,%d)\n",
		      ddr_cop->dclk, ddr_cop->ddr_clk_src,
		      ddr_cop->ddr_clk_sel, ddr_cop->dclk_div);

		if (hwdfc_enable) {
			value.v = __raw_readl((unsigned long)DFC_LEVEL(i));
			value.b.clk_mode = ddr_cop->mode_4x_en;
			value.b.dclksel = ddr_cop->ddr_clk_sel;
			value.b.ddr_clk_div = ddr_cop->dclk_div;
			value.b.mc_table_num = ddr_cop->ddr_tbl_index;
			/* FIXME: fill DVC level in DFC_LEVEL, refer to hwdfc_initvl in kernel */
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
		__init_axi_rtcwtc(axi_cop, 0);
		/* calc this op's voltage req */
		axi_cop->volts = cal_comp_volts_req(AXI, axi_cop->aclk);
	}
}

static void __init_fc_setting(void)
{
	unsigned int regval;
	union pmua_cc cc_cp;
	/*
	 * enable AP FC done interrupt for one step,
	 * while not use three interrupts by three steps
	 */
	__raw_writel((1 << 1), APMU_IMR);

	/* always vote for CP allow AP FC */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	cc_cp.b.core_allow_spd_chg = 1;
	__raw_writel(cc_cp.v, APMU_CP_CCR);

	/* DE: Should be set to 1 as AP will NOT send out ACK */
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

	/*
	 * enable CCI400 clock generator working clock
	 * automatic gating
	 */
	regval = __raw_readl(APMU_REG(0x300));
	regval |= (1 << 13);
	__raw_writel(regval, APMU_REG(0x300));
}

static struct cpu_opt *cpu_rate2_op_ptr
	(unsigned int rate, unsigned int *index, struct list_head *op_list)
{
	unsigned int idx = 0;
	struct cpu_opt *cop;

	list_for_each_entry(cop, op_list, node) {
		if ((cop->pclk >= rate) ||
		    list_is_last(&cop->node, op_list))
			break;
		idx++;
	}

	*index = idx;
	return cop;
}

static unsigned int cpu_get_oprateandvl(unsigned int index,
	unsigned int *volts, struct list_head *op_list)
{
	unsigned int idx = 0;
	struct cpu_opt *cop;

	list_for_each_entry(cop, op_list, node) {
		if ((idx == index) ||
		    list_is_last(&cop->node, op_list))
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

static unsigned int clst0_sel2_srcrate(int ap_sel)
{
	switch (ap_sel) {
	case C0_CLK_SRC_PLL1_624:
		return 624;
	case C0_CLK_SRC_PLL1_832:
		return 832;
	case C0_CLK_SRC_PLL1_1248:
		return 1248;
	case C0_CLK_SRC_PLL2:
		return PLAT_PLL_RATE(PLL2, pll);
	case C0_CLK_SRC_PLL3P:
		return PLAT_PLL_RATE(PLL3, pllp);
	default:
		printf("%s bad ap_clk_sel %d\n", __func__, ap_sel);
		return -ENOENT;
	}
}

static unsigned int clst1_sel2_srcrate(int ap_sel)
{
	switch (ap_sel) {
	case C1_CLK_SRC_PLL1_832:
		return 832;
	case C1_CLK_SRC_PLL1_1248:
		return 1248;
	case C1_CLK_SRC_PLL2:
		return PLAT_PLL_RATE(PLL2, pll);
	case C1_CLK_SRC_PLL3P:
		return PLAT_PLL_RATE(PLL3, pllp);
	default:
		printf("%s bad ap_clk_sel %d\n", __func__, ap_sel);
		return -ENOENT;
	}
}

static unsigned int ddr_sel2_srcrate(int ddr_sel)
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

static unsigned int axi_sel2_srcrate(int axi_sel)
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
	int ap_sel = new->ap_clk_sel;
	struct pll_cfg *pll_cfg;

	if (new->clst_index == CLST0) {
		if (ap_sel == C0_CLK_SRC_PLL2) {
			turn_on_pll_defrate(PLL2);
		} else if (ap_sel == C0_CLK_SRC_PLL3P) {
			pll_cfg = &cur_platform_opt->pll_cfg[PLL3];
			pll_cfg->pll_rate.pll = new->ap_clk_src;
			pll_cfg->pll_rate.pllp = new->ap_clk_src;
			pll_cfg->pll_rate.pll_vco = new->ap_clk_src;
			turn_on_pll_defrate(PLL3);
		}
	} else if (new->clst_index == CLST1) {
		if (ap_sel == C1_CLK_SRC_PLL2) {
			turn_on_pll_defrate(PLL2);
		} else if (ap_sel == C1_CLK_SRC_PLL3P) {
			pll_cfg = &cur_platform_opt->pll_cfg[PLL3];
			pll_cfg->pll_rate.pll = new->ap_clk_src;
			pll_cfg->pll_rate.pllp = new->ap_clk_src;
			pll_cfg->pll_rate.pll_vco = new->ap_clk_src;
			turn_on_pll_defrate(PLL3);
		}
	}
	return 0;
}

static unsigned int ap_clk_disable(struct cpu_opt *opt)
{
	int ap_sel = opt->ap_clk_sel;

	if (opt->clst_index == CLST0) {
		if (ap_sel == C0_CLK_SRC_PLL2)
			turn_off_pll(PLL2);
		else if (ap_sel == C0_CLK_SRC_PLL3P)
			turn_off_pll(PLL3);
	} else if (opt->clst_index == CLST1) {
		if (ap_sel == C1_CLK_SRC_PLL2)
			turn_off_pll(PLL2);
		else if (ap_sel == C1_CLK_SRC_PLL3P)
			turn_off_pll(PLL3);
	}
	return 0;
}

static unsigned int ddr_clk_enable(int ddr_sel)
{
	if (ddr_sel == DDR_CLK_SRC_PLL2)
		turn_on_pll_defrate(PLL2);
	else if (ddr_sel == DDR_CLK_SRC_PLL4)
		turn_on_pll_defrate(PLL4);
	else if (ddr_sel == DDR_CLK_SRC_PLL3P)
		turn_on_pll_defrate(PLL3);
	return 0;
}

static unsigned int ddr_clk_disable(int ddr_sel)
{
	if (ddr_sel == DDR_CLK_SRC_PLL2)
		turn_off_pll(PLL2);
	else if (ddr_sel == DDR_CLK_SRC_PLL4)
		turn_off_pll(PLL4);
	else if (ddr_sel == DDR_CLK_SRC_PLL3P)
		turn_off_pll(PLL3);
	return 0;
}

static unsigned int axi_clk_enable(int axi_sel)
{
	if ((axi_sel == AXI_CLK_SRC_PLL2) || (axi_sel == AXI_CLK_SRC_PLL2P))
		turn_on_pll_defrate(PLL2);
	return 0;
}

static unsigned int axi_clk_disable(int axi_sel)
{
	if ((axi_sel == AXI_CLK_SRC_PLL2) || (axi_sel == AXI_CLK_SRC_PLL2P))
		turn_off_pll(PLL2);
	return 0;
}

static void clr_aprd_status(void)
{
	union pmua_cc cc_ap;

	/* write 1 to MOH_RD_ST_CLEAR to clear MOH_RD_STATUS */
	cc_ap.v = readl(APMU_CCR);
	cc_ap.b.core_rd_st_clear = 1;
	writel(cc_ap.v, APMU_CCR);
	cc_ap.b.core_rd_st_clear = 0;
	writel(cc_ap.v, APMU_CCR);
}

static unsigned int get_dm_cc_ap(void)
{
	unsigned int value;
	value = __raw_readl(APMU_CCSR);
	clr_aprd_status();
	return value;
}

static int fc_lock_ref_cnt;
static void get_fc_lock(void)
{
	u32 reg_val;

	fc_lock_ref_cnt++;

	if (fc_lock_ref_cnt == 1) {
		int timeout = 100000;

		do {
			/* Read dm_cc_ap to compete the FC lock */
			__raw_readl(APMU_CCSR);
			/* Read FC lock status register to query the lock status */
			reg_val = __raw_readl(APMU_FCLSR);
			/* AP got FC lock */
			if ((reg_val & 0x3) == 1)
				break;
		} while (timeout--);

		if (timeout <= 0)
			printf("ap failed to get fc lock\n");
	} else {
		printf("get_fc_lock: unmatched fc_lock_ref_cnt\n");
	}
}

static void put_fc_lock(void)
{
	union pmua_cc cc_ap;

	fc_lock_ref_cnt--;

	if (fc_lock_ref_cnt == 0) {
		/* write 1 to MOH_RD_ST_CLEAR to clear MOH_RD_STATUS */
		cc_ap.v = __raw_readl(APMU_CCR);
		cc_ap.b.core_rd_st_clear = 1;
		__raw_writel(cc_ap.v, APMU_CCR);
		cc_ap.b.core_rd_st_clear = 0;
		__raw_writel(cc_ap.v, APMU_CCR);
	} else {
		printf("put_fc_lock: unmatched fc_lock_ref_cnt\n");
	}
}

static void get_cur_cpu_op(struct cpu_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;

	dm_cc_ap.v = get_dm_cc_ap();
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);

	if (cop->clst_index == CLST0) {
		cop->ap_clk_src = clst0_sel2_srcrate(pllsel.b.apc0clksel);
		cop->ap_clk_sel = pllsel.b.apc0clksel;
		cop->pclk = cop->ap_clk_src / (dm_cc_ap.b.apc0_pclk_div + 1);
		cop->core_aclk = cop->pclk / (dm_cc_ap.b.apc0_aclk_div + 1);
	} else if (cop->clst_index == CLST1) {
		cop->ap_clk_src = clst1_sel2_srcrate(pllsel.b.apc1clksel);
		cop->ap_clk_sel = pllsel.b.apc1clksel;
		cop->pclk = cop->ap_clk_src / (dm_cc_ap.b.apc1_pclk_div + 1);
		cop->core_aclk = cop->pclk / (dm_cc_ap.b.apc1_aclk_div + 1);
	}
}

static void get_cur_ddr_op(struct ddr_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;

	dm_cc_ap.v = get_dm_cc_ap();
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);
	cop->ddr_clk_src = ddr_sel2_srcrate(pllsel.b.ddrclksel);
	cop->ddr_clk_sel = pllsel.b.ddrclksel;
	cop->dclk = cop->ddr_clk_src / (dm_cc_ap.b.ddr_clk_div + 1) / 2;
}

static void get_cur_axi_op(struct axi_opt *cop)
{
	union pmua_pllsel pllsel;
	union pmua_dm_cc dm_cc_ap;

	dm_cc_ap.v = get_dm_cc_ap();
	pllsel.v = __raw_readl(APMU_PLL_SEL_STATUS);

	cop->axi_clk_src = axi_sel2_srcrate(pllsel.b.axiclksel);
	cop->axi_clk_sel = pllsel.b.axiclksel;
	cop->aclk = cop->axi_clk_src / (dm_cc_ap.b.bus_clk_div + 1);
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
	union pmum_fcapclk fcap;

	fcap.v = __raw_readl(MPMU_FCAP);
	if (top->clst_index == CLST0)
		fcap.b.apc0clksel = top->ap_clk_sel;
	else if (top->clst_index == CLST1)
		fcap.b.apc1clksel = top->ap_clk_sel;
	__raw_writel(fcap.v, MPMU_FCAP);
}

static void set_ddr_clk_sel(struct ddr_opt *top)
{
	union pmum_fcdclk fcdclk;

	fcdclk.v = __raw_readl(MPMU_FCDCLK);
	fcdclk.b.ddrclksel = top->ddr_clk_sel;
	__raw_writel(fcdclk.v, MPMU_FCDCLK);
}

static void set_axi_clk_sel(struct axi_opt *top)
{
	union pmum_fcaclk fcaclk;

	fcaclk.v = __raw_readl(MPMU_FCACLK);
	fcaclk.b.axiclksel = top->axi_clk_sel;
	__raw_writel(fcaclk.v, MPMU_FCACLK);
}

static void set_ddr_tbl_index(struct ddr_opt *top)
{
	unsigned int regval;

	/* APMU_MC_HW_SLP_TYPE[3:7] store the table number for mc5 */
	regval = __raw_readl(APMU_MC_HW_SLP_TYPE);
	regval &= ~(0x1 << 11);
	regval |= (top->mode_4x_en << 11);
	regval &= ~(0x1 << 10);		/* enable tbl based FC */
	regval &= ~(0x1F << 3);		/* clear ddr tbl index */
	top->ddr_tbl_index &= 0x1F;
	regval |= (top->ddr_tbl_index << 3);
	__raw_writel(regval, APMU_MC_HW_SLP_TYPE);
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
	if ((cop->pclk < top->pclk) && (top->l1_xtc != cop->l1_xtc))
		__raw_writel(top->l1_xtc, top->l1_xtc_addr);
        if ((cop->pclk < top->pclk) && (top->l2_xtc != cop->l2_xtc))
                __raw_writel(top->l2_xtc, top->l2_xtc_addr);

	/* 0) Pre FC : check CP allow AP FC voting */
	cc_cp.v = __raw_readl(APMU_CP_CCR);
	if (unlikely(!cc_cp.b.core_allow_spd_chg)) {
		printf("%s CP doesn't allow AP FC!\n",
		       __func__);
		cc_cp.b.core_allow_spd_chg = 1;
		__raw_writel(cc_cp.v, APMU_CP_CCR);
	}

	/* Pre FC : AP votes allow FC */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.ap_allow_spd_chg = 1;
	__raw_writel(cc_ap.v, APMU_CCR);

	/* set pclk src */
	set_ap_clk_sel(top);

	/* select div for pclk, core aclk and issue core FC */
	if (top->clst_index == CLST0) {
		cc_ap.b.apc0_pclk_div = top->pclk_div;
		cc_ap.b.apc0_aclk_div = top->core_aclk_div;

		cc_ap.b.apc0_freq_chg_req = 1;
		__raw_writel(cc_ap.v, APMU_CCR);
	} else if (top->clst_index == CLST1) {
		cc_ap.b.apc1_pclk_div = top->pclk_div;
		cc_ap.b.apc1_aclk_div = top->core_aclk_div;

		cc_ap.b.apc1_freq_chg_req = 1;
		__raw_writel(cc_ap.v, APMU_CCR);
	}

	wait_for_fc_done();

	/* Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.ap_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);

	/* high -> low */
	if ((cop->pclk > top->pclk) && (top->l1_xtc != cop->l1_xtc))
		__raw_writel(top->l1_xtc, top->l1_xtc_addr);
        if ((cop->pclk > top->pclk) && (top->l2_xtc != cop->l2_xtc))
                __raw_writel(top->l2_xtc, top->l2_xtc_addr);
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
	    (old->core_aclk == new->core_aclk)) {
		printf("current rate = target rate!\n");
		return -EAGAIN;
	}

	printf("CORE FC start: %u -> %u\n",
	       old->pclk, new->pclk);

	get_fc_lock();
	memcpy(&cop, old, sizeof(struct cpu_opt));
	ap_clk_enable(new);
	core_fc_seq(&cop, new);
	memcpy(&cop, new, sizeof(struct cpu_opt));
	get_cur_cpu_op(&cop);

	if (unlikely((cop.ap_clk_src != new->ap_clk_src) ||
		     (cop.pclk != new->pclk) ||
		(cop.core_aclk != new->core_aclk))) {
		printf("unsuccessful frequency change!\n");
		printf("psrc pclk core_aclk\n");
		printf("CUR %d %d %d\n", cop.ap_clk_src,
		       cop.pclk, cop.core_aclk);
		printf("NEW %d %d %d\n", new->ap_clk_src,
		       new->pclk, new->core_aclk);
		ret = -EAGAIN;
		ap_clk_disable(new);
		goto out;
	}

	ap_clk_disable(old);
out:
	put_fc_lock();
	printf("CORE FC end: %u -> %u\n",
	       old->pclk, new->pclk);
	return ret;
}

static int cpu_setrate(int clst_index, unsigned long rate)
{
	struct cpu_opt *md_new = NULL, *md_old = NULL, *bridge = NULL;
	unsigned int index;
	int ret = 0;

	if (clst_index == CLST0) {
		if (unlikely(!cur_clst0_op))
			cur_clst0_op = &helan3_clst0_bootop;

		md_new = cpu_rate2_op_ptr(rate, &index, &clst0_op_list);
		md_old = cur_clst0_op;
		if (!clst0_bridge_op)
			clst0_bridge_op = list_first_entry(&clst0_op_list,
					struct cpu_opt, node);

		bridge = clst0_bridge_op;
	} else if (clst_index == CLST1) {
		if (unlikely(!cur_clst1_op))
			cur_clst1_op = &helan3_clst1_bootop;

		md_new = cpu_rate2_op_ptr(rate, &index, &clst1_op_list);
		md_old = cur_clst1_op;

		if (!clst1_bridge_op)
			clst1_bridge_op = list_first_entry(&clst1_op_list,
					struct cpu_opt, node);
		bridge = clst1_bridge_op;
	}

	if ((md_old->ap_clk_sel == md_new->ap_clk_sel) &&
	    (md_new->ap_clk_src != md_old->ap_clk_src)) {
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

	if (clst_index == CLST0)
		cur_clst0_op = md_new;
	else if (clst_index == CLST1)
		cur_clst1_op = md_new;

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
	cc_ap.b.ap_allow_spd_chg = 1;

	/* 2) issue DDR FC */
	if ((cop->ddr_clk_src != top->ddr_clk_src) ||
	    (cop->dclk != top->dclk)) {
		/* 2.1) set dclk src */
		set_ddr_clk_sel(top);
		/* 2.2) enable tbl based FC and set DDR tbl num */
		set_ddr_tbl_index(top);
		/* 2.3) select div for dclk */
		cc_ap.b.ddr_clk_div = top->dclk_div;
		/* 2.4) select ddr FC req bit */
		cc_ap.b.ddr_freq_chg_req = 1;
		ddr_fc = 1;
	}

	/* 3) set div and FC req bit trigger DDR/AXI FC */
	if (ddr_fc) {
		__raw_writel(cc_ap.v, APMU_CCR);
		wait_for_fc_done();
	}

	/* 4) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.ap_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);
}

static void axi_fc_seq(struct axi_opt *cop,
			    struct axi_opt *top)
{
	union pmua_cc cc_cp;
	union pmua_cc_ap cc_ap;
	unsigned int axi_fc = 0;

	/* update rtc/wtc if neccessary, PP low -> high */
	if ((cop->aclk < top->aclk) && (top->xtc_val != cop->xtc_val))
		__raw_writel(top->xtc_val, top->xtc_addr);

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
	cc_ap.b.ap_allow_spd_chg = 1;

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
		__raw_writel(cc_ap.v, APMU_CCR);
		wait_for_fc_done();
	}

	/* 4) Post FC : AP clear allow FC voting */
	cc_ap.v = __raw_readl(APMU_CCR);
	cc_ap.b.ap_allow_spd_chg = 0;
	__raw_writel(cc_ap.v, APMU_CCR);

	/*  update rtc/wtc if neccessary, high -> low */
	if ((cop->aclk > top->aclk) && (top->xtc_val != cop->xtc_val))
		__raw_writel(top->xtc_val, top->xtc_addr);
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
		cur_ddr_op = &helan3_ddr_bootop;

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
		cur_axi_op = &helan3_axi_bootop;

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
int setop(unsigned int c0pclk, unsigned int c1pclk,
	unsigned int dclk, unsigned int aclk)
{
	u32 c0pidx, c1pidx, didx, aidx;
	u32 c0vcore, c1vcore, dvcore, avcore, vcore, cur_volt;
	int ret = 0;

	cpu_rate2_op_ptr(c0pclk, &c0pidx, &clst0_op_list);
	cpu_rate2_op_ptr(c1pclk, &c1pidx, &clst1_op_list);
	didx = ddr_rate2_op_index(dclk);
	aidx = axi_rate2_op_index(aclk);
	cpu_get_oprateandvl(c0pidx, &c0vcore, &clst0_op_list);
	cpu_get_oprateandvl(c1pidx, &c1vcore, &clst1_op_list);
	ddr_get_oprateandvl(didx, &dvcore);
	axi_get_oprateandvl(aidx, &avcore);
	vcore = max(c0vcore, c1vcore);
	vcore = max(vcore, dvcore);
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
	ret |= cpu_setrate(CLST0, c0pclk);
	ret |= cpu_setrate(CLST1, c1pclk);

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
static unsigned int cal_comp_volts_req(int comp,
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
		if ((rate * 1000) <= cur_platform_opt->freqs_cmb[comp][idx])
			break;

	if (idx == VL_MAX) {
		printf("Unsupported rate:%u, comp:%d\n", rate, comp);
		goto out;
	}
	vlreq = idx;
out:
	return cur_platform_opt->vm_millivolts[vlreq];
}

#define SOC_DVC1		93
#define SOC_DVC2		94
#define SOC_DVC3		95

static void __init_volts_info(int profile)
{

	handle_svc_table();

	cur_platform_opt->vm_millivolts = millivolts;
	cur_platform_opt->freqs_cmb = freqs_cmb;

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
void helan3_fc_init(int ddr_mode)
{
	/* notes that below sequence has dependency */
	__init_read_ultinfo();

	/* init platform op table related */
	__init_platform_opt(ddr_mode);

	/* init volt req info, FIXME: pass profile */
	__init_volts_info(uiprofile);

	/* init cpu/ddr/axi related setting */
	__init_cpu_opt(CLST0);
	__init_cpu_opt(CLST1);
	__init_ddr_axi_opt();

	/* init freq-chg related hw setting */
	__init_fc_setting();

	cur_clst0_op = &helan3_clst0_bootop;
	get_cur_cpu_op(cur_clst0_op);
	/* set rtc/wtc for cluster0 boot pp */
	__init_cpu_rtcwtc(cur_clst0_op, 1);
	ap_clk_enable(cur_clst0_op);

	cur_clst1_op = &helan3_clst1_bootop;
	get_cur_cpu_op(cur_clst1_op);
	/* set rtc/wtc for cluster1 boot pp */
	__init_cpu_rtcwtc(cur_clst1_op, 1);
	ap_clk_enable(cur_clst1_op);

	cur_ddr_op = &helan3_ddr_bootop;
	get_cur_ddr_op(cur_ddr_op);
	ddr_clk_enable(cur_ddr_op->ddr_clk_sel);

	cur_axi_op = &helan3_axi_bootop;
	get_cur_axi_op(cur_axi_op);
	/* set rtc/wtc for axi boot pp */
	__init_axi_rtcwtc(cur_axi_op, 1);
	axi_clk_enable(cur_axi_op->axi_clk_sel);
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

	printf("Current clst0 OP:\n");
	memcpy(&cop, cur_clst0_op, sizeof(struct cpu_opt));
	printf("pclk(src:sel) core_aclk\n");
	printf("%d(%d:%d) %d\n", cop.pclk, cop.ap_clk_src, cop.ap_clk_sel, cop.core_aclk);

	printf("Current clst1 OP:\n");
	memcpy(&cop, cur_clst1_op, sizeof(struct cpu_opt));
	printf("pclk(src:sel) core_aclk\n");
	printf("%d(%d:%d) %d\n", cop.pclk, cop.ap_clk_src, cop.ap_clk_sel, cop.core_aclk);

	memcpy(&ddr_op, cur_ddr_op, sizeof(struct ddr_opt));
	printf("dclk(src:sel)\n");
	printf("%d(%d:%d)\n", ddr_op.dclk, ddr_op.ddr_clk_src, ddr_op.ddr_clk_sel);

	memcpy(&axi_op, cur_axi_op, sizeof(struct axi_opt));
	printf("aclk(src:sel)\n");
	printf("%d(%d:%d)\n", axi_op.aclk, axi_op.axi_clk_src, axi_op.axi_clk_sel);

	printf("\nAll supported OP:\n");
	printf("Clst0 core(MHZ@mV):\n");
	list_for_each_entry(op, &clst0_op_list, node)
		printf("%d@%d\t", op->pclk, op->volts);

	printf("\nAll supported OP:\n");
	printf("Clst1 core(MHZ@mV):\n");
	list_for_each_entry(op, &clst1_op_list, node)
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
	unsigned int c0pclk, c1pclk, dclk, aclk;

	if (argc == 5) {
		c0pclk = simple_strtoul(argv[1], NULL, 0);
		c1pclk = simple_strtoul(argv[2], NULL, 0);
		dclk = simple_strtoul(argv[3], NULL, 0);
		aclk = simple_strtoul(argv[4], NULL, 0);
		printf("op: c0pclk = %dMHZ c1pclk = %dMHZ dclk = %dMHZ aclk = %dMHZ\n", c0pclk, c1pclk, dclk, aclk);

		setop(c0pclk, c1pclk, dclk, aclk);
	} else if (argc >= 2) {
		printf("usage: op c0pclk c1pclk dclk aclk(unit Mhz)\n");
	} else {
		show_op();
	}
	return 0;
}

U_BOOT_CMD(
	op,	5,	1,	do_op,
	"change operating point",
	"[ op number ]"
);


#define VOL_BASE		600000
#define VOL_STEP		12500
#define VOL_HIGH		1400000

/* FIXME: we need update PMIC code according to the first dkb version */
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

	pmic_reg_write(p_power, 0x4f, 0);

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

	pmic_reg_write(p_power, 0x4f, 0);

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
	ulong freq, clst_index;
	int res = -1;

	if (argc != 3) {
		printf("usage: setcpurate clst_index rate (unit Mhz)\n");
		printf("Supported Core frequency on clst-0:\n");
		list_for_each_entry(cop, &clst0_op_list, node)
			printf("%d\t", cop->pclk);
		printf("\n");
		printf("Supported Core frequency on clst-1:\n");
		list_for_each_entry(cop, &clst1_op_list, node)
			printf("%d\t", cop->pclk);
		printf("\n");
		return res;
	}
	res = strict_strtoul(argv[1], 0, &clst_index);
	res = strict_strtoul(argv[2], 0, &freq);

	if ((res == 0) && (cpu_setrate(clst_index, freq) == 0))
		printf("Clst-%lu freq change was successful\n", clst_index);
	else
		printf("Core-%lu freq change was unsuccessful\n", clst_index);
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

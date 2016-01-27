/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by:
 * Zhoujie Wu <zjwu@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <errno.h>
#include <common.h>
#include <command.h>
#include <asm/arch/cpu.h>
#include <asm/arch/features.h>

#include "helanx_pll.h"


/* PLL */
#define APMU_REG(x)		(0xD4282800 + x)
#define MPMU_REG(x)		(0xD4050000 + x)
#define APB_SPARE_REG(x)	(0xD4090000 + x)

#define MPMU_PLL2CR		MPMU_REG(0x0034)
#define MPMU_PLL3CR		MPMU_REG(0x001c)
#define MPMU_PLL4CR		MPMU_REG(0x0050)
#define APB_SPARE_PLL2CR	APB_SPARE_REG(0x104)
#define APB_SPARE_PLL3CR	APB_SPARE_REG(0x108)
#define APB_SPARE_PLL4CR	APB_SPARE_REG(0x124)
#define APB_PLL2_SSC_CTRL	APB_SPARE_REG(0x130)
#define APB_PLL2_SSC_CONF	APB_SPARE_REG(0x134)
#define APB_PLL2_FREQOFFSET_CTRL	APB_SPARE_REG(0x138)
#define APB_PLL3_SSC_CTRL	APB_SPARE_REG(0x13c)
#define APB_PLL3_SSC_CONF	APB_SPARE_REG(0x140)
#define APB_PLL3_FREQOFFSET_CTRL	APB_SPARE_REG(0x144)
#define APB_PLL4_SSC_CTRL	APB_SPARE_REG(0x148)
#define APB_PLL4_SSC_CONF	APB_SPARE_REG(0x14c)
#define APB_PLL4_FREQOFFSET_CTRL	APB_SPARE_REG(0x150)
#define MPMU_POSR		MPMU_REG(0x0010)
#define POSR_PLL2_LOCK		(1 << 29)
#define POSR_PLL3_LOCK		(1 << 30)
#define POSR_PLL4_LOCK		(1 << 31)

#define PLL_VCO_MIN			(1200)
#define PLL_VCO_SSC_MIN		(1500)
#define PLL_VCO_MAX			(3000)
#define PLL_SSC_ENABLED		(1)
#define PLL_SSC_DISABLED	(0)

#define PLL_CONFIGABLE		(0xffffffff)
#define PLL_UNCONFIGABLE	(0)

union posr {
	struct {
		unsigned int pll1fbd:9;
		unsigned int pll1refd:5;
		unsigned int pll2fbd:9;
		unsigned int pll2refd:5;
		unsigned int pll1_lock:1;
		unsigned int pll2_lock:1;
		unsigned int pll3_lock:1;
		unsigned int pll4_lock:1;
	} b;
	unsigned int v;
};

union pll2cr {
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

union pllx_cr {
	struct {
		unsigned int pllrefd:5;
		unsigned int pllfbd:9;
		unsigned int reserved0:5;
		unsigned int pll_pu:1;
		unsigned int reserved1:12;
	} b;
	unsigned int v;
};

union pllx_swcr {
	struct {
		unsigned int avvd1815_sel:1;
		unsigned int vddm:2;
		unsigned int vddl:3;
		unsigned int icp:4;
		unsigned int pll_bw_sel:1;
		unsigned int kvco:4;
		unsigned int ctune:2;
		unsigned int divseldiff:3;
		unsigned int divselse:3;
		unsigned int diffclken:1;
		unsigned int bypassen:1;
		unsigned int gatectl:1;
		unsigned int fd:3;
		unsigned int reserved:3;
	} b;
	unsigned int v;
};

union pllx_ssc_ctrl {
	struct {
		unsigned int pi_en:1;
		unsigned int reset_pi:1;
		unsigned int ssc_mode:1;
		unsigned int ssc_clk_en:1;
		unsigned int reset_ssc:1;
		unsigned int pi_loop_mode:1;
		unsigned int clk_det_en:1;
		unsigned int reserved:9;
		unsigned int intpi:4;
		unsigned int intpr:3;
		unsigned int reserven_in:9;
	} b;
	unsigned int v;
};

union pllx_ssc_conf {
	struct {
		unsigned int ssc_rnge:11;
		unsigned int reserved:5;
		unsigned int ssc_freq_div:16;
	} b;
	unsigned int v;
};

union pllx_freqoffset_ctrl {
	struct {
		unsigned int freq_offset_en:1;
		unsigned int freq_offset_valid:1;
		unsigned int freq_offset:17;
		unsigned int reserve_in:4;
		unsigned int reserved:9;
	} b;
	unsigned int v;
};

struct pll_post_div {
	unsigned int div;	/* PLL divider value */
	unsigned int divselval;	/* PLL corresonding reg setting */
};

struct intpi_range {
	int vco_min;
	int vco_max;
	u8 value;
};

struct kvco_range {
	int vco_min;
	int vco_max;
	u8 kvco;
};

/* pll related */
static int pll_refcnt[PLL_MAX];

static unsigned int pllx_config_status[PLL_MAX] = {
	PLL_UNCONFIGABLE, PLL_UNCONFIGABLE,/* unconfigurable */
		PLL_CONFIGABLE, PLL_CONFIGABLE, PLL_CONFIGABLE,
};

static unsigned int *pllx_cr_reg[PLL_MAX] = {
	NULL, NULL,
	(u32 *)MPMU_PLL2CR, (u32 *)MPMU_PLL3CR, (u32 *)MPMU_PLL4CR,
};

static unsigned int *pllx_sw_reg[PLL_MAX] = {
	NULL, NULL,
	(u32 *)APB_SPARE_PLL2CR, (u32 *)APB_SPARE_PLL3CR, (u32 *)APB_SPARE_PLL4CR,
};

static unsigned int pllx_lockbit[PLL_MAX] = {
	0, 0, POSR_PLL2_LOCK, POSR_PLL3_LOCK, POSR_PLL4_LOCK,
};

static unsigned int *pllx_ssc_ctrl_reg[PLL_MAX] = {
	NULL, NULL,
	(u32 *)APB_PLL2_SSC_CTRL, (u32 *)APB_PLL3_SSC_CTRL, (u32 *)APB_PLL4_SSC_CTRL,
};

static unsigned int *pllx_ssc_conf_reg[PLL_MAX] = {
	NULL, NULL,
	(u32 *)APB_PLL2_SSC_CONF, (u32 *)APB_PLL3_SSC_CONF, (u32 *)APB_PLL4_SSC_CONF,
};

static unsigned int pllx_ssc_status[PLL_MAX];

/* vco rate to intpi value table */
static struct intpi_range pll_intpi_tbl[] = {
	{2500, 3000, 8},
	{2000, 2500, 6},
	{1500, 2000, 5},
};

/* PLL post divider table */
static struct pll_post_div pll_post_div_tbl[] = {
	/* divider, reg vaule */
	{1, 0x0},
	{2, 0x1},
	{4, 0x2},
	{8, 0x3},
	{16, 0x4},
	{32, 0x5},
	{64, 0x6},
	{128, 0x7},
};

/* kvco range value table */
static struct kvco_range kvco_rng_tbl[] = {
	{2600, 3000, 0xf},
	{2400, 2600, 0xe},
	{2200, 2400, 0xd},
	{2000, 2200, 0xc},
	{1750, 2000, 0xb},
	{1500, 1750, 0xa},
	{1350, 1500, 0x9},
	{1200, 1350, 0x8},
};

static void enable_pllx_ssc(enum pll pll,
	struct pll_ssc_cfg *ssc_cfg, unsigned long vcofreq);
static void disable_pllx_ssc(enum pll pll);


/* convert post div reg setting to divider val */
static unsigned int __pll_divsel2div(unsigned int divselval)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pll_post_div_tbl); i++) {
		if (divselval == pll_post_div_tbl[i].divselval)
			return pll_post_div_tbl[i].div;
	}
	BUG_ON(i == ARRAY_SIZE(pll_post_div_tbl));
	return 0;
}

static void __clk_vco_rate2rng(unsigned long rate,
			       unsigned int *kvco)
{
	int i, size = ARRAY_SIZE(kvco_rng_tbl);

	for (i = 0; i < size; i++) {
		if (rate >= kvco_rng_tbl[i].vco_min &&
		    rate <= kvco_rng_tbl[i].vco_max) {
			*kvco = kvco_rng_tbl[i].kvco;
			return;
		}
	}
	BUG_ON(i == size);
	return;
}

/* rate unit Mhz */
static unsigned int __clk_pll_calc_div(unsigned long rate,
	unsigned long parent_rate, unsigned int *div)
{
	unsigned int i, expectdiv, divsel, tblmaxidx;

	tblmaxidx = ARRAY_SIZE(pll_post_div_tbl) - 1;
	*div = pll_post_div_tbl[tblmaxidx].div;
	divsel = pll_post_div_tbl[tblmaxidx].divselval;

	expectdiv = parent_rate / rate;
	for (i = 0; i < ARRAY_SIZE(pll_post_div_tbl); i++) {
		if (expectdiv <= pll_post_div_tbl[i].div) {
			*div = pll_post_div_tbl[i].div;
			divsel = pll_post_div_tbl[i].divselval;
			break;
		}
	}
	if (expectdiv != *div)
		printf("%s no proper div %d, use div %d!\n", __func__, expectdiv, *div);
	return divsel;
}

static unsigned int __clk_pll_check_lock(enum pll pllid)
{
	unsigned int delaytime = 12;

	/* 28nm PLL lock time is about 47us */
	udelay(45);
	while ((!(__raw_readl(MPMU_POSR) & pllx_lockbit[pllid])) && delaytime) {
		udelay(5);
		delaytime--;
	}

	if (unlikely(!delaytime)) {
		printf("PLL%d is NOT locked after 100us enable!\n", pllid);
		return 0;
	}

	return 1;
}

static unsigned int __clk_pll_check_paras(enum pll pllid)
{
	if (pllid >= PLL_MAX) {
		printf("PLL%d out of range paras!\n", pllid);
		return -EINVAL;
	}

	if (pllx_config_status[pllid] != PLL_CONFIGABLE) {
		printf("PLL%d is not configurable!\n", pllid);
		return -EINVAL;
	}

	return 0;
}

/* FIXME: later PLL HW ctrl interface aligned, pll2_xxx functions won't be used */

/*
 * 1. Whenever PLL2 is enabled, ensure it's set as HW activation.
 * 2. When PLL2 is disabled (no one uses PLL2 as source),
 * set it as SW activation.
 */
static u32 pll2_is_enabled(void)
{
	union pll2cr pll2cr;
	pll2cr.v = __raw_readl(MPMU_PLL2CR);

	/* ctrl = 0(hw enable) or ctrl = 1&&en = 1(sw enable) */
	/* ctrl = 1&&en = 0(sw disable) */
	if (pll2cr.b.ctrl && (!pll2cr.b.en))
		return 0;
	else
		return 1;
}

static unsigned int pll2_power_ctrl(u32 endis)
{
	union pll2cr pllcr;
	unsigned int ret = 0;

	pllcr.v = __raw_readl(MPMU_PLL2CR);
	if (endis) {
		/* we must lock refd/fbd first before enabling PLL2 */
		pllcr.b.ctrl = 1;
		__raw_writel(pllcr.v, MPMU_PLL2CR);
		pllcr.b.ctrl = 0;/* Let HW control PLL2 */
		__raw_writel(pllcr.v, MPMU_PLL2CR);
		ret = __clk_pll_check_lock(PLL2);
	} else {
		printf("Disable PLL2 as not used!\n");
		pllcr.b.ctrl = 1;/* Let SW control PLL2 */
		pllcr.b.en = 0;	 /* disable PLL2 by en bit */
		__raw_writel(pllcr.v, MPMU_PLL2CR);
	}
	return ret;
}

static void pll2_fill_vco_div(u32 refdiv, u32 fbdiv)
{
	union pll2cr pll2cr;

	pll2cr.v = __raw_readl(MPMU_PLL2CR);
	pll2cr.b.pll2refd = refdiv;
	pll2cr.b.pll2fbd = fbdiv;
	__raw_writel(pll2cr.v, MPMU_PLL2CR);
}

static void pll2_get_vco_div(u32 *refdiv, u32 *fbdiv)
{
	union posr pllposr;

	pllposr.v = __raw_readl(MPMU_POSR);
	*refdiv = pllposr.b.pll2refd;
	*fbdiv = pllposr.b.pll2fbd;
}

static u32 pll_is_enabled(enum pll pllid)
{
	union pllx_cr pllxcr;
	u32 *pll_cr_reg;

	if ((pllid == PLL2) && has_feat_sepa_pll2interf())
		return pll2_is_enabled();

	pll_cr_reg = pllx_cr_reg[pllid];
	pllxcr.v = __raw_readl(pll_cr_reg);
	/*
	 * PLLXCR[19] = 0x0 means PLLX is disabled
	 */
	if (!pllxcr.b.pll_pu)
		return 0;
	else
		return 1;
}

static u32 pll_power_ctrl(enum pll pllid, u32 endis)
{
	union pllx_cr pllxcr;
	u32 *pll_cr_reg, ret = 0;

	if ((pllid == PLL2) && has_feat_sepa_pll2interf())
		return pll2_power_ctrl(endis);

	if (!endis)
		printf("Disable PLL%d as not used!\n", pllid);

	pll_cr_reg = pllx_cr_reg[pllid];
	pllxcr.v = __raw_readl(pll_cr_reg);
	pllxcr.b.pll_pu = !!endis;
	__raw_writel(pllxcr.v, pll_cr_reg);
	if (endis)
		ret = __clk_pll_check_lock(pllid);

	return ret;
}

/* rate unit Mhz, must be called when pll is disabled */
/* set refdiv, fbdiv, (Refclk * 4 * Fbdiv) / Refdiv = pllxfreq, Refclk = 26M */
static void pll_vco_setrate(enum pll pllid, u32 rate)
{
	union pllx_cr pllcr;
	u32 pllrefd, pllfbd;
	u32 *pll_cr_reg;

	/* refdiv 1/2/3 is avaliable, select 3 to align with SV */
	pllrefd = 3;
	pllfbd = rate * pllrefd / (26 * 4);

	if ((pllid == PLL2) && has_feat_sepa_pll2interf()) {
		pll2_fill_vco_div(pllrefd, pllfbd);
	} else {
		pll_cr_reg = pllx_cr_reg[pllid];
		pllcr.v = __raw_readl(pll_cr_reg);
		pllcr.b.pllrefd = pllrefd;
		pllcr.b.pllfbd = pllfbd;
		__raw_writel(pllcr.v, pll_cr_reg);
	}
	printf("PLL%d refd%d, fbd%d\n", pllid, pllrefd, pllfbd);
}

static u32 pll_vco_getrate(enum pll pllid)
{
	union pllx_cr pllcr;
	u32 pllrefd, pllfbd;
	u32 *pll_cr_reg;

	if ((pllid == PLL2) && has_feat_sepa_pll2interf()) {
		pll2_get_vco_div(&pllrefd, &pllfbd);
	} else {
		pll_cr_reg = pllx_cr_reg[pllid];
		pllcr.v = __raw_readl(pll_cr_reg);
		pllrefd = pllcr.b.pllrefd;
		pllfbd = pllcr.b.pllfbd;
	}

	if (unlikely(pllrefd > 3))
		printf("Unsupported pll refdiv%d, pll%d", pllrefd, pllid);

	return DIV_ROUND_UP(26 * 4 * pllfbd, pllrefd);
}


/* frequency unit Mhz, return pll vco freq */
u32 get_pll_freq(enum pll pll, u32 *pll_freq, u32 *pllp_freq)
{
	union pllx_swcr pll_sw_ctl;
	u32 pll_vco, pll_div, pllp_div;
	u32 *pll_sw_reg;

	/* return 0 if pll is disabled */
	if (!pll_is_enabled(pll)) {
		printf("%s PLL%d is not enabled!\n", __func__, pll);
		*pll_freq = 0;
		*pllp_freq = 0;
		return 0;
	}

	/* get vco rate */
	pll_vco = pll_vco_getrate(pll);

	/* get se and diff out rate */
	pll_sw_reg = pllx_sw_reg[pll];
	pll_sw_ctl.v = __raw_readl(pll_sw_reg);
	pll_div = __pll_divsel2div(pll_sw_ctl.b.divselse);
	pllp_div = __pll_divsel2div(pll_sw_ctl.b.divseldiff);
	*pll_freq = pll_vco / pll_div;
	*pllp_freq = pll_vco / pllp_div;
	return pll_vco;
}

void set_pll_freq(enum pll pll, u32 pllvco_freq,
		u32 pll_freq, u32 pllp_freq)
{
	union pllx_swcr pll_sw_ctrl;
	u32 kvco = 0, divselse, divseldiff, pll_div, pllp_div;
	u32 *pll_cr_reg, *pll_sw_reg;
	u32 pll_vco_out, pll_out, pllp_out;

	if (__clk_pll_check_paras(pll))
		return;

	/* do nothing if pll is enabled  */
	if (pll_is_enabled(pll)) {
		printf("Pll%d is already enabled!\n", pll);
		return;
	}

	/* calc vco rate range */
	__clk_vco_rate2rng(pllvco_freq, &kvco);

	/* calc pll and pllp divider */
	divselse = __clk_pll_calc_div(pll_freq, pllvco_freq, &pll_div);
	divseldiff = __clk_pll_calc_div(pllp_freq, pllvco_freq, &pllp_div);

	/* pll SW ctrl setting */
	pll_sw_reg = pllx_sw_reg[pll];
	pll_sw_ctrl.v = __raw_readl(pll_sw_reg);
	pll_sw_ctrl.b.avvd1815_sel = 1;
	pll_sw_ctrl.b.vddm = 1;
	pll_sw_ctrl.b.vddl = 4;
	pll_sw_ctrl.b.icp = 3;
	pll_sw_ctrl.b.pll_bw_sel = 0;
	pll_sw_ctrl.b.kvco = kvco;
	pll_sw_ctrl.b.ctune = 1;
	pll_sw_ctrl.b.divseldiff = divseldiff;
	pll_sw_ctrl.b.divselse = divselse;
	pll_sw_ctrl.b.diffclken = 1;
	pll_sw_ctrl.b.bypassen = 0;
	pll_sw_ctrl.b.gatectl = 0;
	pll_sw_ctrl.b.fd = 4;
	__raw_writel(pll_sw_ctrl.v, pll_sw_reg);

	/* set pllx rate */
	pll_vco_setrate(pll, pllvco_freq);

	/* power on pllx */
	pll_power_ctrl(pll, 1);

	/* get real freq */
	pll_vco_out = get_pll_freq(pll, &pll_out, &pllp_out);

	pll_cr_reg = pllx_cr_reg[pll];
	printf("PLL%d_VCO enable@%dMhz, PLL%d@%dMhz, PLL%dP@%dMhz, PLL%d_DIV3@%dMhz\n"
		"SWCR[w%x,r%x] PLL%dCR[r%x]\n",
		pll, pll_vco_out, pll, pll_out, pll, pllp_out,
		pll, pll_vco_out / 3,
		pll_sw_ctrl.v, __raw_readl(pll_sw_reg),
		pll, __raw_readl(pll_cr_reg));

	return;
}

void turn_off_pll(enum pll pll)
{
	if (__clk_pll_check_paras(pll))
		return;

	pll_refcnt[pll]--;
	if (pll_refcnt[pll] < 0)
		printf("unmatched pll%d_refcnt\n", pll);

	if (pll_refcnt[pll] == 0) {
		pll_power_ctrl(pll, 0);

		if (pllx_ssc_status[pll]) {
			disable_pllx_ssc(pll);
			pllx_ssc_status[pll] = 0;
		}
	}
}

/* turn on PLL at specific rate and enable SSC */
void turn_on_pll(enum pll pll, struct pll_cfg pll_cfg)
{
	struct pll_rate_cfg pll_rate = pll_cfg.pll_rate;
	struct pll_ssc_cfg pll_ssc = pll_cfg.pll_ssc;

	if (__clk_pll_check_paras(pll))
		return;

	pll_refcnt[pll]++;
	if (pll_refcnt[pll] == 1) {
		set_pll_freq(pll, pll_rate.pll_vco, pll_rate.pll, pll_rate.pllp);

		if (pll_ssc.ssc_enflg && !pllx_ssc_status[pll]) {
			enable_pllx_ssc(pll, &pll_ssc, pll_rate.pll_vco * MHZ);
			pllx_ssc_status[pll] = 1;
		}
	}
}

static unsigned int __clk_pll_vco2intpi(unsigned long vco_freq)
{
	unsigned int intpi, vco_rate, i, itp_tbl_size;
	struct intpi_range *itp_tbl;
	vco_rate = vco_freq / MHZ;
	intpi = 6;

	itp_tbl = pll_intpi_tbl;
	itp_tbl_size = ARRAY_SIZE(pll_intpi_tbl);
	for (i = 0; i < itp_tbl_size; i++) {
		if ((vco_rate >= itp_tbl[i].vco_min) &&
		    (vco_rate <= itp_tbl[i].vco_max)) {
			intpi = itp_tbl[i].value;
			break;
		}
	}
	if (i == itp_tbl_size)
		BUG_ON("Unsupported vco frequency for INTPI!\n");

	return intpi;
}

/*
 * Real amplitude percentage = amplitude / base
*/
static void __clk_get_sscdivrng(enum ssc_mode mode,
				unsigned long rate,
				unsigned int amplitude, unsigned int base,
				unsigned long vco, unsigned int *div,
				unsigned int *rng)
{
	unsigned int vco_avg;
	if (amplitude > (50 * base / 1000))
		BUG_ON("Amplitude can't exceed 5\%\n");
	switch (mode) {
	case CENTER_SPREAD:
		/* VCO_CLK_AVG = REFCLK * (4*N /M) */
		vco_avg = vco;
		/* SSC_FREQ_DIV = VCO_CLK _AVG /
		   (4*4 * Desired_Modulation_Frequency) */
		*div = (vco_avg / rate) >> 4;
		break;
	case DOWN_SPREAD:
		/* VCO_CLK_AVG= REFCLK * (4*N /M)*(1-Desired_SSC_Amplitude/2) */
		vco_avg = vco - (vco >> 1) / base * amplitude;
		/* SSC_FREQ_DIV = VCO_CLK_AVG /
		   (4*2 * Desired_Modulation_Frequency) */
		*div = (vco_avg / rate) >> 3;
		break;
	default:
		printf("Unsupported SSC MODE!\n");
		return;
	}
	if (*div == 0)
		*div = 1;
	/* SSC_RNGE = Desired_SSC_Amplitude / (SSC_FREQ_DIV * 2^-26) */
	*rng = (1 << 26) / (*div * base / amplitude);
}

static void enable_pllx_ssc(enum pll pll,
	struct pll_ssc_cfg *ssc_cfg, unsigned long vcofreq)
{
	union pllx_ssc_conf ssc_conf;
	union pllx_ssc_ctrl ssc_ctrl;
	u32 *pll_ssc_cnf_reg = pllx_ssc_conf_reg[pll];
	u32 *pll_ssc_ctl_reg = pllx_ssc_ctrl_reg[pll];
	u32 div = 0, rng = 0;

	__clk_get_sscdivrng(ssc_cfg->ssc_mode,
			    ssc_cfg->desired_mod_freq,
			    ssc_cfg->amplitude,
			    ssc_cfg->base, vcofreq, &div, &rng);

	ssc_conf.v = __raw_readl(pll_ssc_cnf_reg);
	ssc_conf.b.ssc_freq_div = div & 0xfff0;
	ssc_conf.b.ssc_rnge = rng;
	__raw_writel(ssc_conf.v, pll_ssc_cnf_reg);

	ssc_ctrl.v = __raw_readl(pll_ssc_ctl_reg);
	ssc_ctrl.b.intpi = __clk_pll_vco2intpi(vcofreq);
	ssc_ctrl.b.intpr = 4;
	ssc_ctrl.b.ssc_mode = ssc_cfg->ssc_mode;
	ssc_ctrl.b.pi_en = 1;
	ssc_ctrl.b.clk_det_en = 1;
	ssc_ctrl.b.reset_pi = 1;
	ssc_ctrl.b.reset_ssc = 1;
	ssc_ctrl.b.pi_loop_mode = 0;
	ssc_ctrl.b.ssc_clk_en = 0;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);

	udelay(2);
	ssc_ctrl.b.reset_ssc = 0;
	ssc_ctrl.b.reset_pi = 0;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);

	udelay(2);
	ssc_ctrl.b.pi_loop_mode = 1;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);

	udelay(2);
	ssc_ctrl.b.ssc_clk_en = 1;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);

	printf("enable ssc for pll%d ssc_conf [sw:%x] [hw:%x]\n"
		"ssc_ctrl [sw:%x] [hw:%x]\n",
		pll, ssc_conf.v, __raw_readl(pll_ssc_cnf_reg),
		ssc_ctrl.v, __raw_readl(pll_ssc_ctl_reg));
}

static void disable_pllx_ssc(enum pll pll)
{
	union pllx_ssc_ctrl ssc_ctrl;
	u32 *pll_ssc_ctl_reg = pllx_ssc_ctrl_reg[pll];

	ssc_ctrl.v = __raw_readl(pll_ssc_ctl_reg);
	ssc_ctrl.b.ssc_clk_en = 0;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);
	udelay(100);

	ssc_ctrl.b.pi_loop_mode = 0;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);
	udelay(2);

	ssc_ctrl.b.pi_en = 0;
	__raw_writel(ssc_ctrl.v, pll_ssc_ctl_reg);

	printf("disable ssc for pll%d ssc_ctrl [sw:%x] [hw:%x]\n",
	       pll, ssc_ctrl.v, __raw_readl(pll_ssc_ctl_reg));
}

int do_pllops(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong pllx, enable, vco = 0, pll = 0, pllp = 0;
	int res = -1;

	if (argc != 6) {
		printf("usage:\n");
		printf("pll 2/3/4 1 pllvcofreq pllfreq pllpfreq,");
		printf(" enable pllx and set to XXX MHZ\n");
		printf("pll 2/3/4 0 0 0 0, disable pll\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &pllx);
	if (res < 0 || __clk_pll_check_paras(pllx)) {
		printf("Failed to get pll num\n");
		return -res;
	}

	if (pll_refcnt[pllx] != 0) {
		printf("system is using PLL%lu(refcnt:%u),", pllx, pll_refcnt[pllx]);
		printf("please switch to other PLL at first,ignore cfg!\n");
		return -1;
	}

	res = strict_strtoul(argv[2], 0, &enable);
	if (res < 0) {
		printf("Failed to get enable/disable flag\n");
		return -res;
	}

	res = strict_strtoul(argv[3], 0, &vco);
	if (res < 0 || (enable && !vco)) {
		printf("Failed to get pll vco rate\n");
		return -res;
	}

	res = strict_strtoul(argv[4], 0, &pll);
	if (res < 0 || (enable && !pll)) {
		printf("Failed to get pll rate\n");
		return -res;
	}

	res = strict_strtoul(argv[5], 0, &pllp);
	if (res < 0 || (enable && !pllp)) {
		printf("Failed to get pllp rate\n");
		return -res;
	}

	if (enable)
		set_pll_freq(pllx, vco, pll, pllp);
	else
		pll_power_ctrl(pllx, 0);
	return 0;
}

U_BOOT_CMD(
	pllops, 6, 1, do_pllops,
	"Set PLL output",
	""
);

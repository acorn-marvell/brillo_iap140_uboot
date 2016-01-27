/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa1928.h>

#define AXI_PHYS_BASE		(0xD4200000ul)
#define APMU_BASE		(AXI_PHYS_BASE + 0x82800)
#define APMU_REG(x)		(APMU_BASE + x)
#define CIU_BASE		(AXI_PHYS_BASE + 0x82c00)
#define CIU_REG(x)		(CIU_BASE + x)

#define GC3D_BASE	(AXI_PHYS_BASE + 0xd000)
#define GC2D_BASE	(AXI_PHYS_BASE + 0xf000)
#define REG_GCPULSEEATER (GC3D_BASE + 0x10c)
#define AQHICLKCTR_OFF	(0x000ul)
#define POWERCTR_OFF	(0x100ul)
#define FSCALE_VALMASK	(0x1fcul)

/* VPU pwr function */
#define APMU_ISLD_VPU_CTRL	APMU_REG(0x1b0)
#define APMU_VPU_RSTCTRL	APMU_REG(0x1f0)
#define APMU_VPU_CLKCTRL	APMU_REG(0x1f4)
#define APMU_ISLD_VPU_PWRCTRL	APMU_REG(0x208)
#define CIU_VPU_CKGT		CIU_REG(0x6c)

#define REG_WIDTH_1BIT	1
#define REG_WIDTH_2BIT	2
#define REG_WIDTH_3BIT	3
#define REG_WIDTH_4BIT	4
#define REG_WIDTH_5BIT	5


#define HWMODE_EN	(1u << 0)
#define PWRUP		(1u << 1)
#define PWR_STATUS	(1u << 4)
#define REDUN_STATUS	(1u << 5)
#define INT_CLR		(1u << 6)
#define INT_MASK	(1u << 7)
#define INT_STATUS	(1u << 8)
#define CMEM_DMMYCLK_EN		(1u << 4)

#define VPU_ACLK_RST	(1u << 0)
#define VPU_DEC_CLK_RST	(1u << 1)
#define VPU_ENC_CLK_RST	(1u << 2)
#define VPU_HCLK_RST	(1u << 3)
#define VPU_PWRON_RST	(1u << 7)
#define VPU_FC_EN	(1u << 9)

#define VPU_ACLK_DIV_MASK	(7 << 0)
#define VPU_ACLK_DIV_SHIFT	0

#define VPU_ACLKSRC_SEL_MASK	(7 << 4)
#define VPU_ACLKSRC_SEL_SHIFT	4

#define VPU_DCLK_DIV_MASK	(7 << 8)
#define	VPU_DCLK_DIV_SHIFT	8

#define	VPU_DCLKSRC_SEL_MASK	(7 << 12)
#define VPU_DCLKSRC_SEL_SHIFT	12

#define	VPU_ECLK_DIV_MASK	(7 << 16)
#define VPU_ECLK_DIV_SHIFT	16

#define VPU_ECLKSRC_SEL_MASK	(7 << 20)
#define VPU_ECLKSRC_SEL_SHIFT	20

/* GC 3D/2D pwr function */
#define APMU_ISLD_GC3D_PWRCTRL	APMU_REG(0x20c)
#define APMU_ISLD_GC_CTRL	APMU_REG(0x1b4)
#define APMU_ISLD_GC2D_CTRL	APMU_REG(0x1b8)
#define APMU_GC3D_CLKCTRL	APMU_REG(0x174)
#define APMU_GC3D_RSTCTRL	APMU_REG(0x170)
#define CIU_GC3D_CKGT		CIU_REG(0x3c)

#define APMU_ISLD_GC2D_PWRCTRL	APMU_REG(0x210)
#define APMU_GC2D_CLKCTRL	APMU_REG(0x17c)
#define APMU_GC2D_RSTCTRL	APMU_REG(0x178)
#define APMU_AP_DEBUG3		APMU_REG(0x394)
#define CIU_FABRIC1_CKGT	CIU_REG(0x64)
#define GC2D_ACLK_EN_A0			(1u << 0)

#define HWMODE_EN	(1u << 0)
#define PWRUP		(1u << 1)
#define PWR_STATUS	(1u << 4)
#define REDUN_STATUS	(1u << 5)
#define INT_CLR		(1u << 6)
#define INT_MASK	(1u << 7)
#define INT_STATUS	(1u << 8)

#define GC2D_ACLK_EN	(1u << 0)
#define GC2D_CLK1X_DIV_MASK	(7 << 8)
#define GC2D_CLK1X_DIV_SHIFT	8

#define GC2D_CLK1X_CLKSRC_SEL_MASK	(7 << 12)
#define GC2D_CLK1X_CLKSRC_SEL_SHIFT	12
#define	GC2D_HCLK_EN	(1u << 24)
#define	GC2D_UPDATE_RTCWTC	(1u << 31)
#define GC2D_CLK1X_EN_SHIFT     15
#define GC2D_CLK1X_EN_MASK      (1 << 15)
#define GC2D_ACLK_RST	(1u << 0)
#define GC2D_CLK1X_RST	(1u << 1)
#define GC2D_HCLK_RST	(1u << 2)
#define GC2D_PWRON_RST	(1u << 7)
#define GC2D_FC_EN	(1u << 9)

#define X2H_CKGT_DISABLE	(1u << 0)

#define GC3D_ACLK_RST	(1u << 0)
#define GC3D_CLK1X_RST	(1u << 1)
#define GC3D_HCLK_RST	(1u << 2)
#define GC3D_PWRON_RST	(1u << 7)
#define GC3D_FC_EN	(1u << 9)

#define GC3D_ACLK_DIV_MASK	(7 << 0)
#define GC3D_ACLK_DIV_SHIFT	0
#define GC3D_ACLKSRC_SEL_MASK	(7 << 4)
#define GC3D_ACLKSRC_SEL_SHIFT	4

#define GC3D_CLK1X_DIV_MASK	(7 << 8)
#define GC3D_CLK1X_DIV_SHIFT	8
#define GC3D_CLK1X_CLKSRC_SEL_MASK	(7 << 12)
#define GC3D_CLK1X_CLKSRC_SEL_SHIFT	12
#define GC3D_CLK1X_EN_SHIFT     15
#define GC3D_CLK1X_EN_MASK      (1 << 15)

#define GC3D_CLKSH_DIV_MASK	(7 << 16)
#define GC3D_CLKSH_DIV_SHIFT	16
#define	GC3D_CLKSH_CLKSRC_SEL_MASK	(7 << 20)
#define GC3D_CLKSH_CLKSRC_SEL_SHIFT	20

#define GC3D_HCLK_EN	(1u << 24)
#define GC3D_UPDATE_RTCWTC	(1u << 31)

#define GC3D_FAB_CKGT_DISABLE	(1u << 1)
#define MC_P4_CKGT_DISABLE	(1u << 0)

#define CMEM_DMMYCLK_EN		(1u << 4)
#define GC3D_CLK_EN			(1u << 3)
#define GC3D_ACLK_EN			(1u << 2)
#define GC3D_ACLK_EN_SHIFT      7
#define GC3D_ACLK_EN_MASK       (1 << 7)
#define GC3D_CLKSH_EN_SHIFT     23
#define GC3D_CLKSH_EN_MASK      (1 << 23)

#define DEFAULT_GC3D_MAX_FREQ	797333333
#define DEFAULT_GC2D_MAX_FREQ	416000000
#define KEEP_ACLK11_ON          (1u << 26)

#define PMUA_AUDIO_PWRCTRL		APMU_REG(0x010c)
#define PMUA_AUDIO_CLK_RES_CTRL		APMU_REG(0x0164)
#define PMUA_SRAM_PWRCTRL		APMU_REG(0x0240)
#define PMUA_ISLD_AUDIO_CTRL		APMU_REG(0x01A8)

#define MASK(n) ((1 << (n)) - 1)
#define MHZ	(1000 * 1000)

#define CLK_SET_BITS(addr, set, clear) do {	\
	unsigned int tmp;			\
	tmp = __raw_readl(addr);		\
	tmp &= ~(clear);			\
	tmp |= (set);				\
	__raw_writel(tmp, addr);		\
} while (0)						\

/* GC internal scaling and clock gating feature support */
enum GC_COMP {
	GC3D_CTR,
	GC2D_CTR,
};

u32 gcctr_base[] = {
	GC3D_BASE, GC2D_BASE
};

/* record reference count on hantro power on/off */
static int hantro_pwr_refcnt;
/* record reference count on gc2d/gc3d power on/off */
static int gc2d_pwr_refcnt;
static int gc3d_pwr_refcnt;

struct peri_reg_info {
	void		*clkctl_reg;
	void		*rstctl_reg;
	void		*rtcwtc_reg;
	/* peripheral device source select */
	u32		src_sel_shift;
	u32		src_sel_width;
	u32		src_sel_mask;
	/* peripheral device divider set */
	u32		div_shift;
	u32		div_width;
	u32		div_mask;
	/* peripheral device freq change trigger */
	u32		fcreq_shift;
	u32		fcreq_width;
	u32		fcreq_mask;
	u32		cutoff_freq;
	u32		rtcwtc_shift;
	u32		enable_val;
	u32		disable_val;
};

struct periph_clk_tbl {
	unsigned long fclk_rate;	/* fclk rate */
	const char *fclk_parent;	/* fclk parent */
	u32 fclk_mul;			/* fclk src select */
	u32 fclk_div;			/* fclk src divider */
	unsigned long aclk_rate;	/* aclk rate */
	const char *aclk_parent;	/* aclk parent */
	u32 aclk_mul;			/* aclk src select */
	u32 aclk_div;			/* aclk src divider */
};

struct periph_parents {
	const char  *name;
	unsigned long rate;
	unsigned long hw_sel;
};

struct xpu_rtcwtc {
	unsigned long max_rate;
	unsigned int rtcwtc;
};

static struct xpu_rtcwtc vpu_rtcwtc_a0[] = {
	{.max_rate = 208000000, .rtcwtc = 0x55550000,},
	{.max_rate = 624000000, .rtcwtc = 0xa9aa0000,},
};

static struct xpu_rtcwtc vpu_rtcwtc_b0[] = {
	{.max_rate = 624000000, .rtcwtc = 0xa9aa0000,},
};

static struct xpu_rtcwtc gc_rtcwtc_a0[] = {
	{.max_rate = 416000000, .rtcwtc = 0x550000,},
	{.max_rate = 797333333, .rtcwtc = 0x55550000,},
};

static struct xpu_rtcwtc gc_rtcwtc_b0[] = {
	{.max_rate = 208000000, .rtcwtc = 0x0,},
	{.max_rate = 797333333, .rtcwtc = 0x55550000,},
};

static struct xpu_rtcwtc gc2d_rtcwtc_a0[] = {
	{.max_rate = 416000000, .rtcwtc = 0x0,},
};

static struct xpu_rtcwtc gc2d_rtcwtc_b0[] = {
	{.max_rate = 208000000, .rtcwtc = 0x009a0000,},
	{.max_rate = 416000000, .rtcwtc = 0xa99a0000,},
};

static struct peri_reg_info vpu_aclk_reg = {
	.clkctl_reg = (void *)APMU_VPU_CLKCTRL,
	.rstctl_reg = (void *)APMU_VPU_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_VPU_CTRL,
	.src_sel_shift = 4,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 0,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.cutoff_freq = 208000000,
	.rtcwtc_shift = 31,
};

static struct peri_reg_info vpu_eclk_reg = {
	.clkctl_reg = (void *)APMU_VPU_CLKCTRL,
	.rstctl_reg = (void *)APMU_VPU_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_VPU_CTRL,
	.src_sel_shift = 20,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 16,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.cutoff_freq = 208000000,
	.rtcwtc_shift = 31,
};

static struct peri_reg_info vpu_dclk_reg = {
	.clkctl_reg = (void *)APMU_VPU_CLKCTRL,
	.rstctl_reg = (void *)APMU_VPU_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_VPU_CTRL,
	.src_sel_shift = 12,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 8,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.cutoff_freq = 208000000,
	.rtcwtc_shift = 31,
};

static struct periph_parents vpu_periph_parents[] = {
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll5p",
		.rate = 531555555,
		.hw_sel = 0x1,
	},
	{
		.name = "pll1_416",
		.rate = 416000000,
		.hw_sel = 0x2,
	},

};

static struct periph_clk_tbl vpu_eclk_tbl_1928[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 531555555,
		.fclk_parent = "pll5p",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},

};

static struct periph_clk_tbl vpu_eclk_tbl_1926[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};


static struct periph_clk_tbl vpu_dclk_tbl[] = {
	{
		.fclk_rate = 104000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 104000000,
		.aclk_parent = "pll1_624",
	},

	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

enum fc_flag {
	NO_CHANGE = 0,
	LOW2HIGH,
	HIGH2LOW,
};

static int check_is_pxa1926(void)
{
	int is_pxa1926 = PXA1926_2L_DISCRETE;
	is_pxa1926 = readl(PXA_CHIP_TYPE_REG);
	if (is_pxa1926 == PXA1926_2L_DISCRETE)
		is_pxa1926 = 1;
	else
		is_pxa1926 = 0;
	return is_pxa1926;
}

static void hantro_on(void)
{
	unsigned int regval, divval;
	unsigned int timeout;

	/* 1. Assert AXI Reset. */
	CLK_SET_BITS(APMU_VPU_RSTCTRL, 0, VPU_ACLK_RST);

	/* 2. Enable HWMODE to power up the island and clear interrupt mask. */
	CLK_SET_BITS(APMU_ISLD_VPU_PWRCTRL, HWMODE_EN, 0);

	CLK_SET_BITS(APMU_ISLD_VPU_PWRCTRL, 0, INT_MASK);

	/* 3. Power up the island. */
	CLK_SET_BITS(APMU_ISLD_VPU_PWRCTRL, PWRUP, 0);
	/*
	 * 4. Wait for active interrupt pending, indicating
	 * completion of island power up sequence.
	 * */
	timeout = 1000;
	do {
		if (--timeout == 0) {
			printf("VPU: active interrupt pending!\n");
			return;
		}
		udelay(10);
		regval = readl(APMU_ISLD_VPU_PWRCTRL);
	} while (!(regval & INT_STATUS));

	/*
	 * 5. The island is now powered up. Clear the interrupt and
	 *    set the interrupt masks.
	 */
	CLK_SET_BITS(APMU_ISLD_VPU_PWRCTRL, INT_CLR | INT_MASK, 0);

	/* 6. Enable Dummy Clocks to SRAMs. */
	CLK_SET_BITS(APMU_ISLD_VPU_CTRL, CMEM_DMMYCLK_EN, 0);

	/* 7. Wait for 500ns. */
	udelay(1);

	/* 8. Disable Dummy Clocks to SRAMs. */
	CLK_SET_BITS(APMU_ISLD_VPU_PWRCTRL, 0, CMEM_DMMYCLK_EN);

	/* 9. Disable VPU fabric Dynamic Clock gating. */
	CLK_SET_BITS(CIU_VPU_CKGT, 0x1f, 0);

	/* 10. De-assert VPU HCLK Reset. */
	CLK_SET_BITS(APMU_VPU_RSTCTRL, VPU_HCLK_RST, 0);

	/* 11. Set PMUA_VPU_RSTCTRL[9] = 0x0. */
	CLK_SET_BITS(APMU_VPU_RSTCTRL, 0, VPU_FC_EN);

	/* 12. program PMUA_VPU_CLKCTRL to enable clks*/
	/* Enable VPU AXI Clock. */
	regval = readl(APMU_VPU_CLKCTRL);
	divval = (regval & VPU_ACLK_DIV_MASK) >> VPU_ACLK_DIV_SHIFT;
	if (!divval) {
		regval &= ~VPU_ACLK_DIV_MASK;
		regval |= (2 << VPU_ACLK_DIV_SHIFT);
		writel(regval, APMU_VPU_CLKCTRL);
	}

	/* Enable Encoder Clock. */
	regval = readl(APMU_VPU_CLKCTRL);
	divval = (regval & VPU_ECLK_DIV_MASK) >> VPU_ECLK_DIV_SHIFT;
	if (!divval) {
		regval &= ~VPU_ECLK_DIV_MASK;
		regval |= (2 << VPU_ECLK_DIV_SHIFT);
		writel(regval, APMU_VPU_CLKCTRL);
	}

	/* Enable Decoder Clock. */
	regval = readl(APMU_VPU_CLKCTRL);
	divval = (regval & VPU_DCLK_DIV_MASK) >> VPU_DCLK_DIV_SHIFT;
	if (!divval) {
		regval &= ~VPU_DCLK_DIV_MASK;
		regval |= (2 << VPU_DCLK_DIV_SHIFT);
		writel(regval, APMU_VPU_CLKCTRL);
	}

	/* 13. Enable VPU Frequency Change. */
	CLK_SET_BITS(APMU_VPU_RSTCTRL, VPU_FC_EN, 0);

	/* 14. De-assert VPU ACLK/DCLK/ECLK Reset. */
	CLK_SET_BITS(APMU_VPU_RSTCTRL, VPU_ACLK_RST | VPU_DEC_CLK_RST
			| VPU_ENC_CLK_RST, 0);

	/* 15. Enable VPU fabric Dynamic Clock gating. */
	CLK_SET_BITS(CIU_VPU_CKGT, 0, 0x1f);

	/* Disable Peripheral Clock. */
	CLK_SET_BITS(APMU_VPU_CLKCTRL, 0, VPU_DCLK_DIV_MASK
			| VPU_ECLK_DIV_MASK);
}

static void hantro_off(void)
{

	/* 1. Assert Bus Reset and Perpheral Reset. */
	CLK_SET_BITS(APMU_VPU_RSTCTRL, 0, VPU_DEC_CLK_RST | VPU_ENC_CLK_RST
			| VPU_HCLK_RST);

	/* 2. Disable Bus and Peripheral Clock. */
	CLK_SET_BITS(APMU_VPU_CLKCTRL, 0, VPU_ACLK_DIV_MASK | VPU_DCLK_DIV_MASK
			| VPU_ECLK_DIV_MASK);
	/* 3. Power down the island. */
	CLK_SET_BITS(APMU_ISLD_VPU_PWRCTRL, 0, PWRUP);
}

static void hantro_power_switch(unsigned int power_on)
{
	if (power_on) {
		if (hantro_pwr_refcnt == 0)
			hantro_on();
		hantro_pwr_refcnt++;
	} else {
		/*
		 * If try to power off hantro, but hantro_pwr_refcnt is 0,
		 * print warning and return.
		 */
		if ((hantro_pwr_refcnt == 0) || (--hantro_pwr_refcnt > 0))
			printf("can't power off now!\n");
		else
			hantro_off();
	}
}

static int do_vpupwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long onoff;
	int res = -1;
	if (argc != 2) {
		printf("usage: vpu_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		hantro_power_switch(onoff);

	return res;
}

U_BOOT_CMD(
	vpu_pwr, 6, 1, do_vpupwr,
	"VPU power on/off",
	""
);

static void enable_vpuenc(unsigned int on)
{
	unsigned int regval, divval;
	if (on) {
		/* Enable Encoder Clock. */
		regval = readl(APMU_VPU_CLKCTRL);
		divval = (regval & VPU_ECLK_DIV_MASK) >> VPU_ECLK_DIV_SHIFT;
		if (!divval) {
			regval &= ~VPU_ECLK_DIV_MASK;
			regval |= (2 << VPU_ECLK_DIV_SHIFT);
			writel(regval, APMU_VPU_CLKCTRL);
		}
	} else {
		/* disable Encoder Clock. */
		regval = readl(APMU_VPU_CLKCTRL);
		regval &= ~VPU_ECLK_DIV_MASK;
		writel(regval, APMU_VPU_CLKCTRL);
	}
}

static int do_enable_vpuenc(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	unsigned long on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable vpu clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	enable_vpuenc(on);

	return res;
}

U_BOOT_CMD(
	enable_vpuenc, 2, 1, do_enable_vpuenc,
	"enable vpu encode clk",
	"enable_vpuenc 1/0"
);

static void enable_vpudec(unsigned int on)
{
	unsigned int regval, divval;
	if (on) {
		/*enable decoder clk*/
		regval = readl(APMU_VPU_CLKCTRL);
		divval = (regval & VPU_DCLK_DIV_MASK) >> VPU_DCLK_DIV_SHIFT;
		if (!divval) {
			regval &= ~VPU_DCLK_DIV_MASK;
			regval |= (2 << VPU_DCLK_DIV_SHIFT);
			writel(regval, APMU_VPU_CLKCTRL);
		}

	} else {
		/* disable decoder Clock. */
		CLK_SET_BITS(APMU_VPU_CLKCTRL, 0, VPU_DCLK_DIV_MASK);
	}
}

static int do_enable_vpudec(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	unsigned long on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable vpu clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	enable_vpudec(on);

	return res;
}

U_BOOT_CMD(
	enable_vpudec, 2, 1, do_enable_vpudec,
	"enable vpu decode clk",
	"enable_vpudec 1/0"
);


static void gc2d_on(void)
{
	unsigned int regval, divval;
	unsigned int timeout;

	/* 1. Disable fabric1 x2h dynamic clock gating. */
	CLK_SET_BITS(CIU_FABRIC1_CKGT, X2H_CKGT_DISABLE, 0);

	/* 2. Assert GC2D ACLK reset */
	CLK_SET_BITS(APMU_GC2D_RSTCTRL, 0, GC2D_ACLK_RST);

	/* 3. Enable HWMODE to power up the island and clear interrupt mask. */
	CLK_SET_BITS(APMU_ISLD_GC2D_PWRCTRL, HWMODE_EN, 0);

	CLK_SET_BITS(APMU_ISLD_GC2D_PWRCTRL, 0, INT_MASK);

	/* 4. Power up the island. */
	CLK_SET_BITS(APMU_ISLD_GC2D_PWRCTRL, PWRUP, 0);
	/*
	 * 5. Wait for active interrupt pending, indicating
	 * completion of island power up sequence.
	 * */
	timeout = 1000;
	do {
		if (--timeout == 0) {
			printf("GC2D: active interrupt pending!\n");
			return;
		}
		udelay(10);
		regval = readl(APMU_ISLD_GC2D_PWRCTRL);
	} while (!(regval & INT_STATUS));

	/*
	 * 6. The island is now powered up. Clear the interrupt and
	 *    set the interrupt masks.
	 */
	CLK_SET_BITS(APMU_ISLD_GC2D_PWRCTRL, INT_CLR | INT_MASK, 0);

	/* 7. Enable Dummy Clocks to SRAMs. */
	CLK_SET_BITS(APMU_ISLD_GC2D_CTRL, CMEM_DMMYCLK_EN, 0);

	/* 8. Wait for 500ns. */
	udelay(1);

	/* 9. Disable Dummy Clocks to SRAMs. */
	CLK_SET_BITS(APMU_ISLD_GC2D_CTRL, 0, CMEM_DMMYCLK_EN);

	/* 10. set FC_EN to 0. */
	CLK_SET_BITS(APMU_GC2D_RSTCTRL, 0, GC2D_FC_EN);

	/* 11. Program PMUA_GC2D_CLKCTRL to enable clocks. */
	/* Enable GC2D HCLK clock. */
	CLK_SET_BITS(APMU_GC2D_CLKCTRL, GC2D_HCLK_EN, 0);

	/* Enalbe GC2D CLK1X clock. */
	if (cpu_is_pxa1928_a0()) {
		regval = readl(APMU_GC2D_CLKCTRL);
		divval = (regval & GC2D_CLK1X_DIV_MASK) >> GC2D_CLK1X_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC2D_CLK1X_DIV_MASK;
			regval |= (2 << GC2D_CLK1X_DIV_SHIFT);
			writel(regval, APMU_GC2D_CLKCTRL);
		}
	} else {
		CLK_SET_BITS(APMU_GC2D_CLKCTRL, GC2D_CLK1X_EN_MASK, 0);
	}
	/* Enable GC2D ACLK clock. */
	CLK_SET_BITS(APMU_GC2D_CLKCTRL, GC2D_ACLK_EN, 0);

	/* 12. Enable Frequency change. */
	CLK_SET_BITS(APMU_GC2D_RSTCTRL, GC2D_FC_EN, 0);

	/* 13. Wait 32 cycles of the slowest clock(HCLK, clk 1x, ACLK. */
	udelay(5);

	/* 14. De-assert GC2D ACLK Reset, CLK1X Reset and HCLK Reset. */
	CLK_SET_BITS(APMU_GC2D_RSTCTRL, GC2D_ACLK_RST | GC2D_CLK1X_RST
			| GC2D_HCLK_RST, 0);

	/* 15. Wait 128 cycles of the slowest clock(HCLK, clk 1x, ACLK. */
	udelay(10);

	/* 16. Enable fabric1 x2h dynamic clock gating. */
	CLK_SET_BITS(CIU_FABRIC1_CKGT, 0, X2H_CKGT_DISABLE);
	/* set gc2d's rtc/wtc values to zero */
	if (cpu_is_pxa1928_a0())
		CLK_SET_BITS(APMU_ISLD_GC2D_CTRL, 0, 0xffff0000);
	/* keep ACLK11 running in D0CG mode */
	CLK_SET_BITS(APMU_AP_DEBUG3, KEEP_ACLK11_ON, 0);

}

static void gc2d_off(void)
{
	/* 1. Assert GC2D CLK1X/HCLK Reset. */
	CLK_SET_BITS(APMU_GC2D_RSTCTRL, 0, GC2D_CLK1X_RST | GC2D_HCLK_RST);

	/* 2. Disable all clocks. */
	/* Disable GC2D HCLK clock. */
	CLK_SET_BITS(APMU_GC2D_CLKCTRL, 0, GC2D_HCLK_EN);

	/* Disable GC2D CLK1X clock. */
	if (cpu_is_pxa1928_a0())
		CLK_SET_BITS(APMU_GC2D_CLKCTRL, 0, GC2D_CLK1X_DIV_MASK);
	else
		CLK_SET_BITS(APMU_GC2D_CLKCTRL, 0, GC2D_CLK1X_EN_MASK);
	/* Disable GC2D AXI clock. */
	CLK_SET_BITS(APMU_GC2D_CLKCTRL, 0, GC2D_ACLK_EN);

	/* 3. Power down the island. */
	CLK_SET_BITS(APMU_ISLD_GC2D_PWRCTRL, 0, PWRUP);
}

static void gc2d_pwr(unsigned int power_on)
{
	if (power_on) {
		if (gc2d_pwr_refcnt == 0)
			gc2d_on();
		gc2d_pwr_refcnt++;
	} else {
		/*
		 * If try to power off gc2d, but gc2d_pwr_refcnt is 0,
		 * print warning and return.
		 */
		if ((gc2d_pwr_refcnt == 0) || (--gc2d_pwr_refcnt > 0))
			printf("can't power off now!\n");
		else
			gc2d_off();
	}
}

static int do_gc2dpwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long onoff;
	int res = -1;
	if (argc != 2) {
		printf("usage: gc2d_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		gc2d_pwr(onoff);

	return res;
}

U_BOOT_CMD(
	gc2d_pwr, 6, 1, do_gc2dpwr,
	"Gc2D power on/off",
	""
);

static void gc3d_on(void)
{
	unsigned int regval, divval;
	unsigned int timeout;

	/* 1. Disable GC3D fabric dynamic clock gating. */
	CLK_SET_BITS(CIU_GC3D_CKGT, GC3D_FAB_CKGT_DISABLE
			| MC_P4_CKGT_DISABLE, 0);
	/* 2. Assert GC3D ACLK reset. */
	CLK_SET_BITS(APMU_GC3D_RSTCTRL, 0, GC3D_ACLK_RST);

	/* 3. Enable HWMODE to power up the island and clear interrupt mask */
	CLK_SET_BITS(APMU_ISLD_GC3D_PWRCTRL, HWMODE_EN, 0);
	CLK_SET_BITS(APMU_ISLD_GC3D_PWRCTRL, 0, INT_MASK);

	/* 4. Power up the island. */
	CLK_SET_BITS(APMU_ISLD_GC3D_PWRCTRL, PWRUP, 0);

	/*
	 * 5. Wait for active interrupt pending, indicating
	 * completion of island power up sequence.
	 * */
	timeout = 1000;
	do {
		if (--timeout == 0) {
			printf("GC3D: active interrupt pending!\n");
			return;
		}
		udelay(10);
		regval = readl(APMU_ISLD_GC3D_PWRCTRL);
	} while (!(regval & INT_STATUS));

	/*
	 * 6. The island is now powered up. Clear the interrupt and
	 *    set the interrupt masks.
	 */
	CLK_SET_BITS(APMU_ISLD_GC3D_PWRCTRL, INT_CLR | INT_MASK, 0);

	/* 7. Enable Dummy Clocks to SRAMs. */
	CLK_SET_BITS(APMU_ISLD_GC_CTRL, CMEM_DMMYCLK_EN, 0);

	/* 8. Wait for 500ns. */
	udelay(1);

	/* 9. Disable Dummy Clocks to SRAMs. */
	CLK_SET_BITS(APMU_ISLD_GC_CTRL, 0, CMEM_DMMYCLK_EN);

	/* 10. set FC_EN to 0. */
	CLK_SET_BITS(APMU_GC3D_RSTCTRL, 0, GC3D_FC_EN);

	/* 11. Program PMUA_GC3D_CLKCTRL to enable clocks. */
	/* Enable GC3D AXI clock. */
	if (cpu_is_pxa1928_a0()) {
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_ACLK_DIV_MASK) >> GC3D_ACLK_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_ACLK_DIV_MASK;
			regval |= (2 << GC3D_ACLK_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}

		/* Enalbe GC3D CLK1X clock. */
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_CLK1X_DIV_MASK) >> GC3D_CLK1X_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_CLK1X_DIV_MASK;
			regval |= (2 << GC3D_CLK1X_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}

		CLK_SET_BITS(APMU_GC3D_CLKCTRL, GC3D_CLK1X_EN_MASK, 0);
		/* Enable GC3D CLKSH clock. */
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_CLKSH_DIV_MASK) >> GC3D_CLKSH_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_CLKSH_DIV_MASK;
			regval |= (2 << GC3D_CLKSH_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}
	} else {
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, GC3D_ACLK_EN_MASK |
				GC3D_CLK1X_EN_MASK | GC3D_CLKSH_EN_MASK, 0);
	}

	/* Enable GC3D HCLK clock. */
	CLK_SET_BITS(APMU_GC3D_CLKCTRL, GC3D_HCLK_EN, 0);

	/* 12. Enable Frequency change. */
	CLK_SET_BITS(APMU_GC3D_RSTCTRL, GC3D_FC_EN, 0);

	/* 13. Wait 32 cycles of the slowest clock. */
	udelay(1);

	/* 14. De-assert GC3D PWRON Reset. */
	CLK_SET_BITS(APMU_GC3D_RSTCTRL, GC3D_PWRON_RST, 0);

	/* 15. De-assert GC3D ACLK Reset, CLK1X Reset and HCLK Reset. */
	CLK_SET_BITS(APMU_GC3D_RSTCTRL, GC3D_ACLK_RST | GC3D_CLK1X_RST
			| GC3D_HCLK_RST, 0);

	/* 16. Wait for 1000ns. */
	udelay(1);

	/* 16. Enable GC3D fabric dynamic clock gating. */
	CLK_SET_BITS(CIU_GC3D_CKGT, 0, GC3D_FAB_CKGT_DISABLE
			| MC_P4_CKGT_DISABLE);
}

static void gc3d_off(void)
{
	/* 1. Disable all clocks. */
	/* Disable GC3D HCLK clock. */
	CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_HCLK_EN);

	if (cpu_is_pxa1928_a0()) {
		/* Disable GC3D SHADER clock */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_CLKSH_DIV_MASK);

		/* Disable GC3D CLK1X clock. */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_CLK1X_DIV_MASK);

		/* Disable GC3D AXI clock. */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_ACLK_DIV_MASK);
	} else {
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_ACLK_EN_MASK |
				GC3D_CLK1X_EN_MASK | GC3D_CLKSH_EN_MASK);
	}

	/* 2. Assert HCLK and CLK1X resets. */
	CLK_SET_BITS(APMU_GC3D_RSTCTRL, 0, GC3D_CLK1X_RST | GC3D_HCLK_RST);

	/* 3. Power down the island. */
	CLK_SET_BITS(APMU_ISLD_GC3D_PWRCTRL, 0, PWRUP);
}

static void gc3d_pwr(unsigned int power_on)
{
	if (power_on) {
		if (gc3d_pwr_refcnt == 0)
			gc3d_on();
		gc3d_pwr_refcnt++;
	} else {
		/*
		 * If try to power off gc3d, but gc3d_pwr_refcnt is 0,
		 * print warning and return.
		 */
		if ((gc3d_pwr_refcnt == 0) || (--gc3d_pwr_refcnt > 0))
			printf("can't power off now!\n");
		else
			gc3d_off();
	}
}

static int do_gcpwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long onoff;
	int res = -1;
	if (argc != 2) {
		printf("usage: gcpwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
if (res == 0)
		gc3d_pwr(onoff);

	return res;
}

U_BOOT_CMD(
	gc_pwr, 6, 1, do_gcpwr,
	"Gc3D power on/off",
	""
);

static void enable_2dgc(unsigned int on)
{
	unsigned int regval, divval;

	if (on) {
		/* Enalbe GC2D CLK1X clock. */
		regval = readl(APMU_GC2D_CLKCTRL);
		divval = (regval & GC2D_CLK1X_DIV_MASK) >> GC2D_CLK1X_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC2D_CLK1X_DIV_MASK;
			regval |= (2 << GC2D_CLK1X_DIV_SHIFT);
			writel(regval, APMU_GC2D_CLKCTRL);
		}

		/* Enable GC2D ACLK clock. */
		CLK_SET_BITS(APMU_GC2D_CLKCTRL, GC2D_ACLK_EN, 0);

	} else {
		/* Disable GC2D CLK1X clock. */
		CLK_SET_BITS(APMU_GC2D_CLKCTRL, 0, GC2D_CLK1X_DIV_MASK);
		/* Disable GC2D AXI clock. */
		CLK_SET_BITS(APMU_GC2D_CLKCTRL, 0, GC2D_ACLK_EN);
	}
}

static int do_enable_2dgc(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	unsigned long on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable 2dgc clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	if (res == 0)
		enable_2dgc(on);

	return 0;
}

U_BOOT_CMD(
	enable_2dgc, 2, 1, do_enable_2dgc,
	"enable 2dgc clk",
	"enable_2dgc 1/0"
);

static void enable_3dgc(unsigned int on)
{
	unsigned int regval, divval;

	if (on) {
		/* Enable GC3D AXI clock. */
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_ACLK_DIV_MASK) >> GC3D_ACLK_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_ACLK_DIV_MASK;
			regval |= (2 << GC3D_ACLK_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}

		/* Enalbe GC3D CLK1X clock. */
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_CLK1X_DIV_MASK) >> GC3D_CLK1X_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_CLK1X_DIV_MASK;
			regval |= (2 << GC3D_CLK1X_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}
	} else {
		/* Disable GC3D CLK1X clock. */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_CLK1X_DIV_MASK);
		/* Disable GC3D AXI clock. */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_ACLK_DIV_MASK);
	}
}

static int do_enable_3dgc(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	unsigned long on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable 3dgc clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	if (res == 0)
		enable_3dgc(on);

	return 0;
}

U_BOOT_CMD(
	enable_3dgc, 2, 1, do_enable_3dgc,
	"enable 3dgc clk",
	"enable_3dgc 1/0"
);

static void enable_gcsh(unsigned int on)
{
	unsigned int regval, divval;

	if (on) {
		/* Enable GC3D AXI clock. */
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_ACLK_DIV_MASK) >> GC3D_ACLK_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_ACLK_DIV_MASK;
			regval |= (2 << GC3D_ACLK_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}

		/* Enable GC3D CLKSH clock. */
		regval = readl(APMU_GC3D_CLKCTRL);
		divval = (regval & GC3D_CLKSH_DIV_MASK) >> GC3D_CLKSH_DIV_SHIFT;
		if (!divval) {
			regval &= ~GC3D_CLKSH_DIV_MASK;
			regval |= (2 << GC3D_CLKSH_DIV_SHIFT);
			writel(regval, APMU_GC3D_CLKCTRL);
		}

	} else {
		/* Disable GC3D SHADER clock */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_CLKSH_DIV_MASK);
		/* Disable GC3D AXI clock. */
		CLK_SET_BITS(APMU_GC3D_CLKCTRL, 0, GC3D_ACLK_DIV_MASK);
	}
}

static int do_enable_gcsh(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	unsigned long on;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable 3dgc clk\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &on);
	if (res == 0)
		enable_gcsh(on);

	return 0;
}

U_BOOT_CMD(
	enable_gc_sh, 2, 1, do_enable_gcsh,
	"enable gc shader clk",
	"enable_gc_sh 1/0"
);

static unsigned int peri_get_parent_index(const char *parent,
	struct periph_parents *table, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		if (!strcmp(parent, table[i].name))
			break;
	}
	if (i == size) {
		printf("no supported source parent found\n");
		return 0;
	}
	return i;
}

static int peri_set_mux_div(struct peri_reg_info *reg_info,
		unsigned long mux, unsigned long div, unsigned int fc_flag,
		unsigned int rtcwtc)
{
	unsigned int regval, newval;
	unsigned int muxmask, divmask;
	unsigned int muxval, divval, fcreqmsk;

	muxval = mux & reg_info->src_sel_mask;

	divval = div & reg_info->div_mask;
	muxval = muxval << reg_info->src_sel_shift;
	divval = divval << reg_info->div_shift;

	muxmask = reg_info->src_sel_mask << reg_info->src_sel_shift;
	divmask = reg_info->div_mask << reg_info->div_shift;
	regval = __raw_readl(reg_info->clkctl_reg);
	regval &= ~(muxmask | divmask);
	newval = regval | (muxval | divval);
	/*
	 * A 3-step progaming is recommended for changing clock freq.
	 * 1) set FC_EN field to 0 to block frequency changes.
	 * 2) configure clk source and clk divider.
	 * 3) set FC_EN field to 1 to allow frequency changes
	 */
	fcreqmsk = reg_info->fcreq_mask << reg_info->fcreq_shift;
	regval = __raw_readl(reg_info->rstctl_reg);
	regval &= ~fcreqmsk;
	__raw_writel(regval, reg_info->rstctl_reg);

	regval = __raw_readl(reg_info->rtcwtc_reg);
	regval &= 0xffff;
	regval |= rtcwtc;
	__raw_writel(regval, reg_info->rtcwtc_reg);
	/* update rtc/wtc */
	regval = __raw_readl(reg_info->clkctl_reg);
	regval |= 1 << reg_info->rtcwtc_shift;
	__raw_writel(regval, reg_info->clkctl_reg);
	/* change frequency */
	__raw_writel(newval, reg_info->clkctl_reg);
	if (reg_info->fcreq_mask) {
		regval = __raw_readl(reg_info->rstctl_reg);
		regval |= fcreqmsk;
		__raw_writel(regval, reg_info->rstctl_reg);
	}
	regval = __raw_readl(reg_info->clkctl_reg);
	printf("regval[0x%p] = %x\n", reg_info->clkctl_reg, regval);
	return 0;
}

static int cal_fc_dir(struct peri_reg_info *reg_info, unsigned long rate,
		struct periph_parents *pare_src, unsigned int pare_tb_size)
{
	unsigned int i, regval,  src_val, div_val;
	unsigned int old_rate = 0;
	unsigned int fc_flag = NO_CHANGE;

	regval = readl(reg_info->clkctl_reg);
	src_val = (regval >> reg_info->src_sel_shift) & reg_info->src_sel_mask;
	div_val =  (regval >> reg_info->div_shift) & reg_info->div_mask;
	for (i = 0; i < pare_tb_size; i++) {
		if (src_val == pare_src[i].hw_sel) {
			old_rate = pare_src[i].rate/(div_val + 1);
			break;
		}
	}
	if (old_rate == rate)
		fc_flag = NO_CHANGE;
	else if ((old_rate <= reg_info->cutoff_freq) &&
			(rate > reg_info->cutoff_freq))
		fc_flag = LOW2HIGH;
	else if ((old_rate > reg_info->cutoff_freq) &&
			(rate <= reg_info->cutoff_freq))
		fc_flag = HIGH2LOW;
	return fc_flag;
}

static int cal_rtcwtc(struct peri_reg_info *reg_info, unsigned int fc_flag,
		struct xpu_rtcwtc *xpu_rtcwtc, unsigned int rtcwtc_tb_size)
{
	unsigned int i;
	unsigned int rtcwtc = 0;

	for (i = 0; i < rtcwtc_tb_size; i++) {
		if ((fc_flag == HIGH2LOW) && (xpu_rtcwtc[i].max_rate
					== reg_info->cutoff_freq)){
			rtcwtc = xpu_rtcwtc[i].rtcwtc;
			break;
		} else if (fc_flag == LOW2HIGH) {
			if (xpu_rtcwtc[i].max_rate > reg_info->cutoff_freq) {
				rtcwtc = xpu_rtcwtc[i].rtcwtc;
				break;
			}
		}
	}
	return rtcwtc;
}

static int peri_set_clk(unsigned long freq, struct periph_clk_tbl *tbl,
	int tbl_size, struct peri_reg_info *clk_reg_info,
	struct periph_parents *periph_parents, int parent_size)
{
	unsigned long rate, mux, div;
	unsigned int parent, i, ret, fc_flag, rtcwtc;
	struct xpu_rtcwtc *xpu_rtcwtc;
	unsigned int rtcwtc_size;

	if (cpu_is_pxa1928_a0()) {
		if (clk_reg_info->clkctl_reg == (void *)APMU_GC2D_CLKCTRL) {
			xpu_rtcwtc = gc2d_rtcwtc_a0;
			rtcwtc_size = ARRAY_SIZE(gc2d_rtcwtc_a0);
		} else {
			xpu_rtcwtc = gc_rtcwtc_a0;
			rtcwtc_size = ARRAY_SIZE(gc_rtcwtc_a0);
		}

	} else {
		if (clk_reg_info->clkctl_reg == (void *)APMU_GC2D_CLKCTRL) {
			xpu_rtcwtc = gc2d_rtcwtc_b0;
			rtcwtc_size = ARRAY_SIZE(gc2d_rtcwtc_b0);
		} else {
			xpu_rtcwtc = gc_rtcwtc_b0;
			rtcwtc_size = ARRAY_SIZE(gc_rtcwtc_b0);
		}
	}

	rate = freq * MHZ;

	for (i = 0; i < tbl_size; i++) {
		if (rate <= tbl[i].fclk_rate) {
			printf("i = %d\n", i);
			printf("set clk rate @ %lu\n", tbl[i].fclk_rate);
			break;
		}
	}
	if (i == tbl_size) {
		printf("no supported rate\n");
		return -1;
	}

	parent = peri_get_parent_index(tbl[i].fclk_parent, periph_parents,
			parent_size);
	mux = periph_parents[parent].hw_sel;
	div = (periph_parents[parent].rate / tbl[i].fclk_rate);
	fc_flag = cal_fc_dir(clk_reg_info, rate,
			periph_parents, parent_size);
	rtcwtc = cal_rtcwtc(clk_reg_info, fc_flag, xpu_rtcwtc, rtcwtc_size);
	ret = peri_set_mux_div(clk_reg_info, mux, div, fc_flag, rtcwtc);
	return ret;
}

static int perip_setrate(unsigned long freq, struct periph_clk_tbl *tbl,
	int tbl_size, struct peri_reg_info *aclk_reg_info,
	struct peri_reg_info *fclk_reg_info,
	struct periph_parents *periph_aparents, int aparent_size,
	struct periph_parents *periph_fparents, int fparent_size,
	struct xpu_rtcwtc *xpu_rtcwtc, unsigned int rtcwtc_size)
{
	unsigned long rate, fmux, amux, fdiv, adiv;
	unsigned int fparent, aparent, i, fc_flag;
	unsigned int ret, rtcwtc;

	rate = freq * MHZ;
	fc_flag = cal_fc_dir(aclk_reg_info, rate,
			periph_aparents, aparent_size);
	for (i = 0; i < tbl_size; i++) {
		if (rate <= tbl[i].fclk_rate) {
			printf("i = %d\n", i);
			printf("set fclk rate @ %lu aclk rate @ %lu\n",
			       tbl[i].fclk_rate, tbl[i].aclk_rate);
			break;
		}
	}
	if (i == tbl_size) {
		printf("no supported rate\n");
		return -1;
	}

	aparent = peri_get_parent_index(tbl[i].aclk_parent, periph_aparents,
					aparent_size);
	amux = periph_aparents[aparent].hw_sel;
	adiv = (periph_aparents[aparent].rate / tbl[i].aclk_rate);
	rtcwtc = cal_rtcwtc(aclk_reg_info, fc_flag, xpu_rtcwtc, rtcwtc_size);
	ret = peri_set_mux_div(aclk_reg_info, amux, adiv, fc_flag, rtcwtc);
	fc_flag = cal_fc_dir(fclk_reg_info, rate,
			periph_fparents, fparent_size);
	fparent = peri_get_parent_index(tbl[i].fclk_parent, periph_fparents,
					fparent_size);
	fmux = periph_fparents[fparent].hw_sel;
	fdiv = (periph_fparents[fparent].rate / tbl[i].fclk_rate);
	rtcwtc = cal_rtcwtc(fclk_reg_info, fc_flag, xpu_rtcwtc, rtcwtc_size);
	ret = peri_set_mux_div(fclk_reg_info, fmux, fdiv, fc_flag, rtcwtc);
	return ret;
}

static int do_setvpuencrate(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	struct periph_clk_tbl *tbl;
	unsigned int tbl_size, i;
	struct periph_parents *parents_tbl = vpu_periph_parents;
	unsigned int size = ARRAY_SIZE(vpu_periph_parents);
	struct xpu_rtcwtc *xpu_rtcwtc;
	unsigned int rtcwtc_size;
	ulong freq;
	int res = -1;

	if (check_is_pxa1926()) {
		tbl = vpu_eclk_tbl_1926;
		tbl_size = ARRAY_SIZE(vpu_eclk_tbl_1926);
	} else {
		tbl = vpu_eclk_tbl_1928;
		tbl_size = ARRAY_SIZE(vpu_eclk_tbl_1928);
	}
	if (cpu_is_pxa1928_a0()) {
		xpu_rtcwtc = vpu_rtcwtc_a0;
		rtcwtc_size = ARRAY_SIZE(vpu_rtcwtc_a0);
	} else {
		xpu_rtcwtc = vpu_rtcwtc_b0;
		rtcwtc_size = ARRAY_SIZE(vpu_rtcwtc_b0);
	}
	if (argc != 2) {
		printf("usage: set vpu encode frequency xxxx(unit Mhz)\n");
		printf("Supported vpu encode frequency:\n");
		printf("vpu fclk:\t vpu aclk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = perip_setrate(freq, tbl, tbl_size, &vpu_aclk_reg,
			&vpu_eclk_reg, parents_tbl, size, parents_tbl, size,
			xpu_rtcwtc, rtcwtc_size);
	if (res == 0)
		printf("VPU encode freq change was successful\n");
	else
		printf("VPU encode freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setvpuencrate, 2, 1, do_setvpuencrate,
	"Setting vpu encode rate",
	"setvpuencrate xxx(Mhz)"
);

static int do_setvpudecrate(cmd_tbl_t *cmdtp, int flag, int argc,
			 char * const argv[])
{
	struct periph_clk_tbl *tbl = vpu_dclk_tbl;
	unsigned int tbl_size = ARRAY_SIZE(vpu_dclk_tbl), i;
	struct periph_parents *parents_tbl = vpu_periph_parents;
	unsigned int size = ARRAY_SIZE(vpu_periph_parents);
	struct xpu_rtcwtc *xpu_rtcwtc;
	unsigned int rtcwtc_size;
	ulong freq;
	int res = -1;

	if (cpu_is_pxa1928_a0()) {
		xpu_rtcwtc = vpu_rtcwtc_a0;
		rtcwtc_size = ARRAY_SIZE(vpu_rtcwtc_a0);
	} else {
		xpu_rtcwtc = vpu_rtcwtc_b0;
		rtcwtc_size = ARRAY_SIZE(vpu_rtcwtc_b0);
	}
	if (argc != 2) {
		printf("usage: set vpu encode frequency xxxx(unit Mhz)\n");
		printf("Supported vpu encode frequency:\n");
		printf("vpu fclk:\t vpu aclk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = perip_setrate(freq, tbl, tbl_size, &vpu_aclk_reg,
			&vpu_dclk_reg, parents_tbl, size, parents_tbl, size,
			xpu_rtcwtc, rtcwtc_size);
	if (res == 0)
		printf("VPU decode freq change was successful\n");
	else
		printf("VPU decode freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setvpudecrate, 2, 1, do_setvpudecrate,
	"Setting vpu decode rate",
	"setvpudecrate xxx(Mhz)"
);

static struct periph_parents gc2d_periph_parents[] = {
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll1_416",
		.rate = 416000000,
		.hw_sel = 0x2,
	},

};

static struct peri_reg_info gc2d_fclk_reg = {
	.clkctl_reg = (void *)APMU_GC2D_CLKCTRL,
	.rstctl_reg = (void *)APMU_GC2D_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_GC2D_CTRL,
	.src_sel_shift = 12,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 8,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.rtcwtc_shift = 31,
};


static struct periph_clk_tbl gc2d_clk_tbl_1928[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll1_416",
	},
};

static struct periph_clk_tbl gc2d_clk_tbl_1926[] = {
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
	},
};

static int do_set2dgcrate(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct periph_clk_tbl *tbl;
	unsigned int tbl_size, i;
	struct periph_parents *parents_tbl = gc2d_periph_parents;
	unsigned int size = ARRAY_SIZE(gc2d_periph_parents);

	ulong freq;
	int res = -1;

	if (check_is_pxa1926()) {
		tbl = gc2d_clk_tbl_1926;
		tbl_size = ARRAY_SIZE(gc2d_clk_tbl_1926);
	} else {
		tbl = gc2d_clk_tbl_1928;
		tbl_size = ARRAY_SIZE(gc2d_clk_tbl_1928);
	}
	if (argc != 2) {
		printf("usage: set 2dgc frequency xxxx(unit Mhz)\n");
		printf("Supported 2dgc frequency:\n");
		printf("gc fclk:\t gc aclk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_set_clk(freq, tbl, tbl_size, &gc2d_fclk_reg,
				   parents_tbl, size);
	if (res == 0)
		printf("2D GC freq change was successful\n");
	else
		printf("2D GC freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	set2dgcrate, 2, 1, do_set2dgcrate,
	"Setting 2dgc rate",
	"set2dgcrate xxx(Mhz)"
);

static struct periph_parents gc3d_fclk_parents[] = {
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll5",
		.rate = 1248000000,
		.hw_sel = 0x2,
	},
	{
		.name = "pll5p",
		.rate = 531555555,
		.hw_sel = 0x6,
	},
};

static struct periph_parents gc3d_aclk_parents[] = {
	{
		.name = "pll1_624",
		.rate = 624000000,
		.hw_sel = 0x0,
	},
	{
		.name = "pll1_416",
		.rate = 416000000,
		.hw_sel = 0x2,
	},
};

static struct periph_clk_tbl gc3d_clk_tbl_1928[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 156000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 531555555,
		.fclk_parent = "pll5p",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
	{
		.fclk_rate = 797333333,
		.fclk_parent = "pll5",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

static struct periph_clk_tbl gc3d_clk_tbl_1926[] = {
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 208000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll5",
		.aclk_rate = 312000000,
		.aclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll5",
		.aclk_rate = 416000000,
		.aclk_parent = "pll1_416",
	},
};

static struct peri_reg_info gc3d_aclk_reg = {
	.clkctl_reg = (void *)APMU_GC3D_CLKCTRL,
	.rstctl_reg = (void *)APMU_GC3D_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_GC_CTRL,
	.src_sel_shift = 4,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 0,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.cutoff_freq = 416000000,
	.rtcwtc_shift = 31,
};

static struct peri_reg_info gc3d_fclk_reg = {
	.clkctl_reg = (void *)APMU_GC3D_CLKCTRL,
	.rstctl_reg = (void *)APMU_GC3D_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_GC_CTRL,
	.src_sel_shift = 12,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 8,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.cutoff_freq = 416000000,
	.rtcwtc_shift = 31,
};


static int do_set3dgcrate(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct periph_clk_tbl *tbl;
	unsigned int tbl_size, i;
	struct periph_parents *fparents_tbl = gc3d_fclk_parents;
	unsigned int fp_size = ARRAY_SIZE(gc3d_fclk_parents);
	struct periph_parents *aparents_tbl = gc3d_aclk_parents;
	unsigned int ap_size = ARRAY_SIZE(gc3d_aclk_parents);
	ulong freq;
	struct xpu_rtcwtc *xpu_rtcwtc;
	unsigned int rtcwtc_size;
	int res = -1;

	if (check_is_pxa1926()) {
		pxa1928_pll5_set_rate(1248000000);
		tbl = gc3d_clk_tbl_1926;
		tbl_size = ARRAY_SIZE(gc3d_clk_tbl_1926);
	} else {
		tbl = gc3d_clk_tbl_1928;
		tbl_size = ARRAY_SIZE(gc3d_clk_tbl_1928);
	}
	if (cpu_is_pxa1928_a0()) {
		xpu_rtcwtc = gc_rtcwtc_a0;
		rtcwtc_size = ARRAY_SIZE(gc_rtcwtc_a0);
	} else {
		xpu_rtcwtc = gc_rtcwtc_b0;
		rtcwtc_size = ARRAY_SIZE(gc_rtcwtc_b0);
	}

	if (argc != 2) {
		printf("usage: set 3dgc frequency xxxx(unit Mhz)\n");
		printf("Supported 3dgc frequency:\n");
		printf("gc fclk:\t gc aclk:\n");
		for (i = 0; i < tbl_size; i++) {
			printf("%lu\t%lu\n", tbl[i].fclk_rate,
			       tbl[i].aclk_rate);
		}
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = perip_setrate(freq, tbl, tbl_size, &gc3d_aclk_reg,
			&gc3d_fclk_reg, aparents_tbl, ap_size,
			fparents_tbl, fp_size, xpu_rtcwtc, rtcwtc_size);

	if (res == 0)
		printf("3D GC freq change was successful\n");
	else
		printf("3D GC freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	set3dgcrate, 2, 1, do_set3dgcrate,
	"Setting 3dgc rate",
	"set3dgcrate xxx(Mhz)"
);

static struct peri_reg_info gc_shader_reg = {
	.clkctl_reg = (void *)APMU_GC3D_CLKCTRL,
	.rstctl_reg = (void *)APMU_GC3D_RSTCTRL,
	.rtcwtc_reg = (void *)APMU_ISLD_GC_CTRL,
	.src_sel_shift = 20,
	.src_sel_mask = MASK(REG_WIDTH_3BIT),
	.div_shift = 16,
	.div_mask = MASK(REG_WIDTH_3BIT),
	.fcreq_shift = 9,
	.fcreq_mask = MASK(REG_WIDTH_1BIT),
	.cutoff_freq = 416000000,
	.rtcwtc_shift = 31,
};

static struct periph_clk_tbl gc_shader_clk_tbl_1928[] = {
	{
		.fclk_rate = 156000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_624",
	},

	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 531555555,
		.fclk_parent = "pll5p",
	},
	{
		.fclk_rate = 624000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 797333333,
		.fclk_parent = "pll5",
	},
};

static struct periph_clk_tbl gc_shader_clk_tbl_1926[] = {
	{
		.fclk_rate = 208000000,
		.fclk_parent = "pll1_624",
	},

	{
		.fclk_rate = 312000000,
		.fclk_parent = "pll1_624",
	},
	{
		.fclk_rate = 416000000,
		.fclk_parent = "pll5",
	},
};

static int do_setgcshrate(cmd_tbl_t *cmdtp, int flag, int argc,
			  char * const argv[])
{
	struct periph_clk_tbl *tbl;
	unsigned int tbl_size, i;
	struct periph_parents *parents_tbl = gc3d_fclk_parents;
	unsigned int size = ARRAY_SIZE(gc3d_fclk_parents);
	ulong freq;
	int res = -1;

	if (check_is_pxa1926()) {
		pxa1928_pll5_set_rate(1248000000);
		tbl = gc_shader_clk_tbl_1926;
		tbl_size = ARRAY_SIZE(gc_shader_clk_tbl_1926);
	} else {
		tbl = gc_shader_clk_tbl_1928;
		tbl_size = ARRAY_SIZE(gc_shader_clk_tbl_1928);
	}

	if (argc != 2) {
		printf("usage: set gc shader frequency xxxx(unit Mhz)\n");
		printf("Supported gc shader frequency:\n");
		printf("gc shader clk:\n");
		for (i = 0; i < tbl_size; i++)
			printf("%lu\n", tbl[i].fclk_rate);
		printf("\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &freq);
	if (res == 0)
		res = peri_set_clk(freq, tbl, tbl_size, &gc_shader_reg,
				   parents_tbl, size);

	if (res == 0)
		printf("GC Shader freq change was successful\n");
	else
		printf("GC Shader freq change was unsuccessful\n");

	return 0;
}

U_BOOT_CMD(
	setgcshrate, 2, 1, do_setgcshrate,
	"Setting gc shader rate",
	"setgcshrate xxx(Mhz)"
);
void pxa1928_audio_subsystem_poweron(int pwr_on)
{
	unsigned int pmua_audio_pwrctrl, pmua_audio_clk_res_ctrl;
	unsigned int pmua_isld_audio_ctrl, pmua_sram_pwrctrl;

	if (pwr_on) {
		/* Audio island power on */
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		if ((pmua_audio_pwrctrl & (0x3 << 9)) == 3) {
			printf("%s audio domain has poewed on\n", __func__);
			return;
		}
		pmua_sram_pwrctrl = readl(PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl |= (1 << 2);
		writel(pmua_sram_pwrctrl, PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl = readl(PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl |= (1 << 0);
		writel(pmua_sram_pwrctrl, PMUA_SRAM_PWRCTRL);
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		pmua_audio_pwrctrl |= (1 << 9);
		writel(pmua_audio_pwrctrl, PMUA_AUDIO_PWRCTRL);
		udelay(10);
		pmua_sram_pwrctrl = readl(PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl |= (1 << 3);
		writel(pmua_sram_pwrctrl, PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl = readl(PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl |= (1 << 1);
		writel(pmua_sram_pwrctrl, PMUA_SRAM_PWRCTRL);
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		pmua_audio_pwrctrl |= (1 << 10);
		writel(pmua_audio_pwrctrl, PMUA_AUDIO_PWRCTRL);
		udelay(10);

		/* disable sram FW */
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl &= ~(3 << 1);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl &= ~(1 << 0);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);

		/* Enable Dummy clocks to srams*/
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl |= (1 << 4);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);
		udelay(1);

		/* disbale dummy clock to srams*/
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl &= ~(1 << 4);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);

		/*
		 * program audio sram & apb clocks
		 * to source from 26MHz vctcxo
		 */
		pmua_audio_clk_res_ctrl = readl(PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl |= (1 << 3);
		writel(pmua_audio_clk_res_ctrl, PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl = readl(PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl |= (1 << 1);
		writel(pmua_audio_clk_res_ctrl, PMUA_AUDIO_CLK_RES_CTRL);

		/* disable island fw*/
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		pmua_audio_pwrctrl |= (1 << 8);
		writel(pmua_audio_pwrctrl, PMUA_AUDIO_PWRCTRL);

		/* start sram repairt */
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		pmua_audio_pwrctrl |= (1 << 2);
		writel(pmua_audio_pwrctrl, PMUA_AUDIO_PWRCTRL);

		/* wait for sram repair completion*/
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		while (pmua_audio_pwrctrl & (1 << 2))
			pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);

		/* release sram and apb reset*/
		pmua_audio_clk_res_ctrl = readl(PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl |= (1 << 2);
		writel(pmua_audio_clk_res_ctrl, PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl = readl(PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl |= (1 << 0);
		writel(pmua_audio_clk_res_ctrl, PMUA_AUDIO_CLK_RES_CTRL);
		udelay(1);
	} else {
		/* Audio island power down */
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		/* Check if audio already power off, if yes then return */
		if ((pmua_audio_pwrctrl & ((0x3 << 9) | (0x1 << 8))) == 0x0) {
			/* Audio is powered off, return */
			return;
		}

		/* Enable Dummy clocks to srams*/
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl |= (1 << 4);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);
		udelay(1);

		/* disbale dummy clock to srams*/
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl &= ~(1 << 4);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);

		/* enable sram fw */
		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl &= ~(3 << 1);
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);

		pmua_isld_audio_ctrl = readl(PMUA_ISLD_AUDIO_CTRL);
		pmua_isld_audio_ctrl |= 1 << 0;
		writel(pmua_isld_audio_ctrl, PMUA_ISLD_AUDIO_CTRL);

		/* enable audio fw */
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		pmua_audio_pwrctrl &= ~(1 << 8);
		writel(pmua_audio_pwrctrl, PMUA_AUDIO_PWRCTRL);

		/* assert audio sram and apb reset */
		pmua_audio_clk_res_ctrl = readl(PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl &= ~(1 << 2);
		writel(pmua_audio_clk_res_ctrl, PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl = readl(PMUA_AUDIO_CLK_RES_CTRL);
		pmua_audio_clk_res_ctrl &= ~(1 << 0);
		writel(pmua_audio_clk_res_ctrl, PMUA_AUDIO_CLK_RES_CTRL);

		/* power off the island */
		pmua_audio_pwrctrl = readl(PMUA_AUDIO_PWRCTRL);
		pmua_audio_pwrctrl &= ~(3 << 9);
		writel(pmua_audio_pwrctrl, PMUA_AUDIO_PWRCTRL);

		/* if needed, power off the audio buffer sram */
		pmua_sram_pwrctrl = readl(PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl &= ~(3 << 2);
		writel(pmua_sram_pwrctrl, PMUA_SRAM_PWRCTRL);
		/*
		 * Fixme: if needed, power off the audio map code sram
		 * but, we keep it here due to we don't want to
		 * load map firmware again */
		pmua_sram_pwrctrl = readl(PMUA_SRAM_PWRCTRL);
		pmua_sram_pwrctrl &= ~(3 << 0);
		writel(pmua_sram_pwrctrl, PMUA_SRAM_PWRCTRL);
	}
}

static int do_audiopwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long onoff;
	int res = -1;
	if (argc != 2) {
		printf("usage: audio_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		pxa1928_audio_subsystem_poweron(onoff);

	return res;
}

U_BOOT_CMD(
	audio_pwr, 6, 1, do_audiopwr,
	"AUDIO power on/off",
	""
);

static void gc_fs_on(enum GC_COMP comp)
{
	u32 val;
	/* switch to manual mode, only 3D has this extra step */
	if (GC3D_CTR == comp) {
		val = __raw_readl(REG_GCPULSEEATER);
		val &= ~(1 << 16);
		val |= (1 << 17);
		__raw_writel(val, REG_GCPULSEEATER);
	}

	val = __raw_readl(gcctr_base[comp] + AQHICLKCTR_OFF);
	val &= ~(FSCALE_VALMASK);
	/* set 1/64 scaling divider */
	val |= (1 << 2) | (1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
	/* make scaling work */
	val &= ~(1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
}

static void gc_fs_off(enum GC_COMP comp)
{
	u32 val;

	val = __raw_readl(gcctr_base[comp] + AQHICLKCTR_OFF);
	val &= ~(FSCALE_VALMASK);
	/* set 64/64 scaling divider */
	val |= (64 << 2) | (1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
	/* make scaling work */
	val &= ~(1 << 9);
	__raw_writel(val, gcctr_base[comp] + AQHICLKCTR_OFF);
}

static void gc_cg_ctrl(enum GC_COMP comp, u32 onoff)
{
	u32 val;

	val = __raw_readl(gcctr_base[comp] + POWERCTR_OFF);
	val &= ~(1 << 0);
	val |= (!!onoff);
	__raw_writel(val, gcctr_base[comp] + POWERCTR_OFF);
}

static int do_enable_3dgc_fs(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_3dgc_fs 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0) {
		if (onoff)
			gc_fs_on(GC3D_CTR);
		else
			gc_fs_off(GC3D_CTR);
	}
	return res;
}

U_BOOT_CMD(
	enable_3dgc_fs, 6, 1, do_enable_3dgc_fs,
	"Enable GC3D frequency scaling",
	""
);

static int do_enable_3dgc_cg(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_3dgc_cg 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		gc_cg_ctrl(GC3D_CTR, onoff);
	return res;
}

U_BOOT_CMD(
	enable_3dgc_cg, 6, 1, do_enable_3dgc_cg,
	"Enable GC3D clock gating",
	""
);

static int do_enable_2dgc_fs(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_2dgc_fs 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0) {
		if (onoff)
			gc_fs_on(GC2D_CTR);
		else
			gc_fs_off(GC2D_CTR);
	}
	return res;
}

U_BOOT_CMD(
	enable_2dgc_fs, 6, 1, do_enable_2dgc_fs,
	"Enable GC2D frequency scaling",
	""
);

static int do_enable_2dgc_cg(cmd_tbl_t *cmdtp, int flag, int argc,
		char * const argv[])
{
	ulong onoff;
	int res = -1;

	if (argc != 2) {
		printf("usage: enable_2dgc_cg 1/0\n");
		return res;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		gc_cg_ctrl(GC2D_CTR, onoff);
	return res;
}

U_BOOT_CMD(
	enable_2dgc_cg, 6, 1, do_enable_2dgc_cg,
	"Enable GC2D clock gating",
	""
);


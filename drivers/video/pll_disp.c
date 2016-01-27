/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 */
#include <config.h>
#include <common.h>
#include <malloc.h>
#include <mmp_disp.h>
#include <linux/types.h>
#include <div64.h>
#include <asm/io.h>

#include <power/pxa988_freq.h>
#ifndef CONFIG_PXA1U88
static unsigned int pll3_vco;

struct dsi_op_info {
	unsigned int fps_max;
	unsigned int fps_min;
	unsigned int op_num;
	unsigned int op_divider_num;
	unsigned int pll3_vco_min;
	unsigned int pll3_vco_max;
	unsigned int pll3_divider_num;
	unsigned int pll3_divider_table[6];
	unsigned int pll3_op_index;
	unsigned int op_table[3];
};

static struct dsi_op_info dsi_op_info_data = {
	.fps_max = 70,
	.fps_min = 50,
	.op_num = 3,
	.op_divider_num = 16,
	.pll3_vco_min = 1200,
	.pll3_vco_max = 2500,
	.pll3_divider_num = 6,
	.pll3_divider_table = {
		8, 6, 4, 3, 2, 1
	},
	.pll3_op_index = 2,
	.op_table = {
		416, 624, 0
	},
};

void calculate_dsi_clk(struct mmp_disp_plat_info *mi)
{
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	struct lcd_videomode *modes = &mi->modes[0];
	struct dsi_op_info *op_info;
	u32 total_w, total_h, byteclk, bitclk, bitclk_max,
		byteclk_max, bitclk_min, byteclk_min;
	u32 op_num, dsi_divider_num, i, j;
	u32 diff, diff_min, diff_count;
	u32 fps_max, fps_min;
	u32 def_op, temp_op;
	char *cmdline;

	if (!di)
		return;

	op_info = &dsi_op_info_data;
	def_op = get_max_cpurate();
	if (!def_op)
		return;

	/* Check current max cpu rate */
	for (i = 0; i < op_info->pll3_divider_num; i++) {
		if ((def_op * op_info->pll3_divider_table[i])
				> op_info->pll3_vco_max)
			continue;
		if ((def_op * op_info->pll3_divider_table[i])
				< op_info->pll3_vco_min) {
			if ((def_op * op_info->pll3_divider_table[i+1])
					< op_info->pll3_vco_max) {
				def_op *= op_info->pll3_divider_table[i+1];
				break;
			}
		}
	}

	/* Set shared cpu rate */
	op_info->op_table[op_info->pll3_op_index] = def_op;

	fps_max = op_info->fps_max;
	fps_min = op_info->fps_min;

	total_w = modes->xres + modes->left_margin +
		modes->right_margin + modes->hsync_len;
	total_h = modes->yres + modes->upper_margin +
		modes->lower_margin + modes->vsync_len;

	byteclk = ((total_w * (di->bpp >> 3)) * total_h *
		modes->refresh) / di->lanes;
	bitclk = byteclk << 3;
	bitclk /= 1000000;

	byteclk_max = ((total_w * (di->bpp >> 3)) * total_h *
		fps_max) / di->lanes;
	bitclk_max = byteclk_max << 3;
	bitclk_max /= 1000000;

	byteclk_min = ((total_w * (di->bpp >> 3)) * total_h *
		fps_min) / di->lanes;
	bitclk_min = byteclk_min << 3;
	bitclk_min /= 1000000;

	op_num = op_info->op_num;
	dsi_divider_num = op_info->op_divider_num;

	diff_min = bitclk;
	diff_count = 0xff;

	/* calculate bit clk for all ops */
	for (j = 0; j < op_num; j++) {
		for (i = 1; i < dsi_divider_num; i++) {
			temp_op = op_info->op_table[j] / i;
			if (temp_op > bitclk_min && temp_op < bitclk_max) {
				diff = (temp_op > bitclk) ? (temp_op - bitclk) :
					(bitclk - temp_op);
				if (diff < diff_min) {
					diff_min = diff;
					diff_count = j;
				}
			}
		}
	}

	/* If can't find the suitable bit clk, select pll3 directly */
	if (diff_count == 0xff) {
		/* Set the pll3 op only for dsi */
		for (j = 1; j < dsi_divider_num; j++) {
			if ((bitclk * j > op_info->pll3_vco_min) &&
			    (bitclk * j < op_info->pll3_vco_max)) {
				pll3_vco = bitclk * j;
				break;
			}
		}
	}

	/* If find the suitable bit clk is shared pll3 op, exit */
	if (diff_count == op_info->pll3_op_index) {
			pll3_vco = op_info->op_table[op_info->pll3_op_index];
			return;
	}

#ifdef CONFIG_CORE_1248
	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	sprintf(cmdline + strlen(cmdline), " pll3_vco=%d", PLL3_VCO_DEF);
	setenv("bootargs", cmdline);
	free(cmdline);
#else
	if (pll3_vco) {
		cmdline = malloc(COMMAND_LINE_SIZE);
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		sprintf(cmdline + strlen(cmdline), " pll3_vco=%d", pll3_vco);
		setenv("bootargs", cmdline);
		free(cmdline);
	}
#endif
}
#else
#define LCD_PN_SCLK			(0xd420b1a8)
#define PIXEL_SRC_DISP1			(0x0 << 29)
#define PIXEL_SRC_DSI1_PLL		(0x3 << 29)
#define PIXEL_SRC_SEL_MASK		(0x3 << 29)
#define DSI1_BIT_CLK_SRC_DISP1		(0x0 << 12)
#define DSI1_BIT_CLK_SRC_DSI1_PLL	(0x3 << 12)
#define DSI1_BIT_CLK_SRC_SEL_MASK	(0x3 << 12)
#define DSI1_BITCLK_DIV(div)		((div) << 8)
#define DSI1_BITCLK_DIV_MASK		((0xf) << 8)
#define PIXEL_CLK_DIV(div)		(div)
#define PIXEL_CLK_DIV_MASK		(0xff)

#define APMU_DISP_CLKCTRL		(0xd428284c)
#define DISP1_CLK_SRC_SEL_PLL1_624	(0 << 9)
#define DISP1_CLK_SRC_SEL_PLL1_832	(1 << 9)
#define DISP1_CLK_SRC_SEL_PLL1_499	(2 << 9)
#define DISP1_CLK_SRC_SEL_MASK		(3 << 9)
#define DISP1_CLK_SRC_EN		(1 << 5)
#define DSIPLL_CLK_SRC_EN		(1 << 8)
#define DSIPLL_CLK_SRC_SEL_MASK		(1 << 14)
#define DSIPLL_CLK_SRC_SEL_PLL4		(0 << 14)
#define DSIPLL_CLK_SRC_SEL_PLL4_DIV3	(1 << 14)

#define MHZ_TO_HZ	(1000000)
#define PLL1_832M	(832000000)
#define PLL1_624M	(624000000)
#define PLL1_499M	(499000000)
static int parent_rate_fixed_num = 3;
static int parent_rate_fixed_tbl[3] = {
	624,
	832,
	499,
};

unsigned int pll4_vco;	/* vco rate Mhz */
unsigned int pll4;	/* pll se rate(pll) Mhz */
unsigned int pll4p;
#define VCO_PRATE_26M	(26)
#define VCO_REFDIV	(3)
#define VCO_MIN_PXA1908	(1200)
#define VCO_MAX_PXA1908	(3000)
#define TO_MHZ		(1000000)
static void pll4_round_rate(unsigned long rate)
{
	int div_sel, div_sel_max = 7, fbd, fbd_min, fbd_max;
	unsigned long pre_vco, vco_rate, offset, offset_min, new_rate;
	int post_div_sel;
	uint64_t div_result;

	pre_vco = VCO_PRATE_26M * 4 / VCO_REFDIV;
	fbd_min = (VCO_MIN_PXA1908 + pre_vco / 2) / pre_vco;
	fbd_max = VCO_MAX_PXA1908 / (pre_vco + 1);

	rate /= TO_MHZ;
	if (rate > (pre_vco * fbd_max / 2)) {
		rate = pre_vco * fbd_max / 2;
		printf("rate was too high (<=%luM)\n", rate);
	} else if (rate < ((pre_vco * fbd_min) >> div_sel_max)) {
		rate = pre_vco * fbd_min >> div_sel_max;
		printf("rate was too low (>=%luM)\n", rate);
	}

	offset_min = pre_vco * fbd_max / 2;
	pll4_vco = VCO_PRATE_26M * 4 * fbd_min / VCO_REFDIV;
	pll4 = pll4_vco >> div_sel_max;
	post_div_sel = div_sel_max;
	for (fbd = fbd_min; fbd <= fbd_max; fbd++) {
		vco_rate = VCO_PRATE_26M * 4 * fbd / VCO_REFDIV;
		for (div_sel = 1; div_sel <= div_sel_max; div_sel++) {
			new_rate = vco_rate >> div_sel;
			offset = (new_rate > rate) ? (new_rate - rate) : (rate - new_rate);
			if (offset < offset_min) {
				offset_min = offset;
				pll4_vco = vco_rate;
				pll4 = pll4_vco >> div_sel;
				post_div_sel = div_sel;
			}
		}
	}

	/* round vco */
	div_result = pll4_vco * VCO_REFDIV + (VCO_PRATE_26M << 1);
	do_div(div_result, VCO_PRATE_26M << 2);
	div_result = (VCO_PRATE_26M << 2) * div_result;
	div_result += VCO_REFDIV - 1;
	do_div(div_result, VCO_REFDIV);
	pll4_vco = div_result;
	pll4 = div_result >> post_div_sel;
	pll4p = pll4;
}

void lcd_calculate_sclk(struct mmp_disp_info *fb,
		int dsiclk, int pathclk)
{
	u32 sclk, apmu, bclk_div = 1, pclk_div, tmp;
	int sclk_src, parent_rate, div_tmp;
	int offset, offset_min = dsiclk, i;
	char *cmdline;

	sclk_src = fb->mi->sclk_src;
	for (i = 0; i < parent_rate_fixed_num; i++) {
		parent_rate = parent_rate_fixed_tbl[i];
		div_tmp = (parent_rate + dsiclk / 2) / dsiclk;
		if (!div_tmp)
			div_tmp = 1;

		offset = abs(parent_rate - dsiclk * div_tmp);

		if (offset < offset_min) {
			offset_min = offset;
			bclk_div = div_tmp;
			sclk_src = parent_rate;
		}
	}

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	if (offset_min > 20) {
		printf("LCD choose PLL4 as clk source,DDR max freq is constraint to 624M!!\n");
#ifndef CONFIG_MMP_FPGA
		sprintf(cmdline + strlen(cmdline), " ddr_max=%d", DDR_MAX_FREQ);
#endif
		/* out of fixed parent rate +-20M */
		fb->mi->sclk_src = dsiclk;
		bclk_div = 1;
	} else {
		printf("LCD choose PLL1 as clk source!\n");
		fb->mi->sclk_src = sclk_src;
	}

	setenv("bootargs", cmdline);
	free(cmdline);
	sclk = readl(LCD_PN_SCLK);
	sclk &= ~(PIXEL_SRC_SEL_MASK | DSI1_BIT_CLK_SRC_SEL_MASK |
			DSI1_BITCLK_DIV_MASK | PIXEL_CLK_DIV_MASK);
	pclk_div = (fb->mi->sclk_src - pathclk / 2) / pathclk;
	pclk_div = pclk_div / 2;
	pclk_div *= 2;

	sclk = DSI1_BITCLK_DIV(bclk_div) | PIXEL_CLK_DIV(pclk_div);

	fb->mi->sclk_src *= MHZ_TO_HZ;
	apmu = readl(APMU_DISP_CLKCTRL);
	apmu &= ~(DISP1_CLK_SRC_SEL_MASK | DSIPLL_CLK_SRC_SEL_MASK);
	if (fb->mi->sclk_src == PLL1_832M) {
		apmu |= DISP1_CLK_SRC_SEL_PLL1_832;
		sclk |= PIXEL_SRC_DISP1 | DSI1_BIT_CLK_SRC_DISP1;
	} else if (fb->mi->sclk_src == PLL1_624M) {
		apmu |= DISP1_CLK_SRC_SEL_PLL1_624;
		sclk |= PIXEL_SRC_DISP1 | DSI1_BIT_CLK_SRC_DISP1;
	} else if (fb->mi->sclk_src == PLL1_499M) {
		apmu |= DISP1_CLK_SRC_SEL_PLL1_499;
		sclk |= PIXEL_SRC_DISP1 | DSI1_BIT_CLK_SRC_DISP1;
		/*
		 * For ULC and helan3 and projects in future maybe,
		 * PLL1_499M's enable bit need to be set when choose
		 * PLL1_499M as clk source.
		 */
		tmp = readl(APB_SPARE1_REG);
		tmp |= 1 << 31;
		writel(tmp, APB_SPARE1_REG);
	} else {
		pll4_round_rate(fb->mi->sclk_src);
		printf("pll4_vco: %d, pll4: %d, pll4p: %d\n", pll4_vco, pll4, pll4p);
#ifndef CONFIG_MMP_FPGA
		set_pll_freq(PLL4, pll4_vco, pll4, pll4p);
#endif
		apmu |= DSIPLL_CLK_SRC_SEL_PLL4 | DSIPLL_CLK_SRC_EN;
		sclk |= PIXEL_SRC_DSI1_PLL | DSI1_BIT_CLK_SRC_DSI1_PLL;
	}

	writel(apmu, APMU_DISP_CLKCTRL);
	fb->mi->sclk_div = sclk;
}
#endif

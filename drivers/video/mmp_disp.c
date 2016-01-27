/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by:	Green Wan <gwan@marvell.com>
 *		Jun Nie <njun@marvell.com>
 *		Kevin Liu <kliu5@marvell.com>
 *
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <asm/io.h>
#include <i2c.h>
#include <asm/gpio.h>
#include <mmp_disp.h>
#include <common.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif

struct mmp_disp_info *mmp_disp_fbi;
int g_panel_id;
unsigned long g_disp_start_addr;
unsigned long g_disp_buf_size;

struct lcd_regs *get_regs(struct mmp_disp_info *fbi)
{
	struct lcd_regs *regs = (struct lcd_regs *)((uintptr_t)fbi->reg_base);

	if (fbi->id == 0)
		regs = (struct lcd_regs *)((uintptr_t)fbi->reg_base + 0xc0);
	if (fbi->id == 2)
		regs = (struct lcd_regs *)((uintptr_t)fbi->reg_base + 0x200);

	return regs;
}

u32 dma_ctrl_read(struct mmp_disp_info *fbi, int ctrl1)
{
	u32 reg = (uintptr_t)fbi->reg_base + dma_ctrl(ctrl1, fbi->id);
	return __raw_readl((uintptr_t)reg);
}

void dma_ctrl_write(struct mmp_disp_info *fbi, int ctrl1, u32 value)
{
	u32 reg = (u32)(uintptr_t)fbi->reg_base + dma_ctrl(ctrl1, fbi->id);
	__raw_writel(value, (uintptr_t)reg);
}

void dma_ctrl_set(struct mmp_disp_info *fbi, int ctrl1, u32 mask, u32 value)
{
	u32 reg = (u32)(uintptr_t)fbi->reg_base + dma_ctrl(ctrl1, fbi->id);
	u32 tmp1, tmp2;

	tmp1 = __raw_readl((uintptr_t)reg);
	tmp2 = tmp1;
	tmp2 &= ~mask;
	tmp2 |= value;
	if (tmp1 != tmp2)
		__raw_writel(tmp2, (uintptr_t)reg);
}

static void set_pix_fmt(struct mmp_disp_plat_info *mi, int pix_fmt)
{
	switch (pix_fmt) {
	case PIX_FMT_RGB565:
		mi->bits_per_pixel = 16;
		break;
	case PIX_FMT_BGR565:
		mi->bits_per_pixel = 16;
		break;
	case PIX_FMT_RGB1555:
		mi->bits_per_pixel = 16;
		break;
	case PIX_FMT_BGR1555:
		mi->bits_per_pixel = 16;
		break;
	case PIX_FMT_RGB888PACK:
		mi->bits_per_pixel = 24;
		break;
	case PIX_FMT_BGR888PACK:
		mi->bits_per_pixel = 24;
		break;
	case PIX_FMT_RGB888UNPACK:
		mi->bits_per_pixel = 32;
		break;
	case PIX_FMT_BGR888UNPACK:
		mi->bits_per_pixel = 32;
		break;
	case PIX_FMT_RGBA888:
		mi->bits_per_pixel = 32;
		break;
	case PIX_FMT_BGRA888:
		mi->bits_per_pixel = 32;
		break;
	}
}

/*
 * The hardware clock divider has an integer and a fractional
 * stage:
 *
 *	clk2 = clk_in / integer_divider
 *	clk_out = clk2 * (1 - (fractional_divider >> 12))
 *
 * Calculate integer and fractional divider for given clk_in
 * and clk_out.
 */
static void set_clock_divider(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	struct dsi_info *di = (struct dsi_info *)mi->phy_info;
	u32 val = mi->sclk_div, reg;

	/* for lcd controller */
	writel(val & (~0xf00), fbi->reg_base + clk_div(fbi->id));
	/* for dsi clock */
	if (mi->phy_type & (DSI | DSI2DPI)) {
		if (di->id == 1) {
			reg = readl(fbi->reg_base + clk_div(0));
			reg &= ~0xf00;
			reg |= val & 0xf00;
			writel(reg, fbi->reg_base + clk_div(0));
		} else if (di->id == 2) {
			reg = readl(fbi->reg_base + clk_div(1));
			reg &= ~0xf00;
			reg |= val & 0xf00;
			writel(reg, fbi->reg_base + clk_div(1));
			writel(val & (~0xf00), fbi->reg_base + clk_div(2));
		}
	}
}

static u32 dma_ctrl0_update(int active, struct mmp_disp_plat_info *mi,
		u32 x, u32 pix_fmt)
{
	int tmp, shift1, shift2;

	tmp = 0x100;
	shift1 = 16;
	shift2 = 12;

	if (active)
		x |= tmp;
	else
		x &= ~tmp;

	/* If we are in a pseudo-color mode, we need to enable
	 * palette lookup  */
	if (pix_fmt == PIX_FMT_PSEUDOCOLOR)
		x |= 0x10000000;

	/* Configure hardware pixel format */
	x &= ~(0xF << shift1);
	x |= (pix_fmt >> 1) << shift1;

	/* Check YUV422PACK */
	x &= ~((1 << 9) | (1 << 11) | (1 << 10) | (1 << 12));
	if (((pix_fmt >> 1) == 5) || (pix_fmt & 0x1000)) {
		x |= 1 << 9;
		x |= (mi->panel_rbswap) << 12;
		if (pix_fmt == 11)
			x |= 1 << 11;
		if (pix_fmt & 0x1000)
			x |= 1 << 10;
	} else {
		/* Check red and blue pixel swap.
		 * 1. source data swap. BGR[M:L] rather than RGB[M:L] is
		 * stored in memeory as source format.
		 * 2. panel output data swap
		 */
		x |= (((pix_fmt & 1) ^ 1) ^ (mi->panel_rbswap)) << shift2;
	}
	/* enable horizontal smooth filter for both graphic and video layers */
	x |= CFG_GRA_HSMOOTH(1) | CFG_DMA_HSMOOTH(1);

	return x;
}

static void set_dma_control0(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi;
	u32 x = 0, active, pix_fmt = fbi->pix_fmt;

	/* Set bit to enable graphics DMA */
	dma_ctrl_set(fbi, 0, CFG_ARBFAST_ENA(1), CFG_ARBFAST_ENA(1));

	mi = fbi->mi;
	active = fbi->active;
	x = dma_ctrl_read(fbi, 0);
	active = 1;
	x = dma_ctrl0_update(active, mi, x, pix_fmt);
	dma_ctrl_write(fbi, 0, x);
	/* enable multiple burst request in DMA AXI bus arbiter for
	 * faster read
	 */
	x = readl(fbi->reg_base + LCD_SPU_DMA_CTRL0);
	x |= CFG_ARBFAST_ENA(1);
	writel(x, fbi->reg_base + LCD_SPU_DMA_CTRL0);
}

static void set_graphics_start(struct mmp_disp_info *fbi, int xoffset,
		int yoffset)
{
	int pixel_offset;
	unsigned int phys_addr;
	static int debugcount;
#ifdef CONFIG_VDMA
	struct mmp_vdma_reg *vdma_reg = (struct mmp_vdma_reg *)fbi->vdma_reg_base;
	struct mmp_vdma_ch_reg *ch_reg = &vdma_reg->ch0_reg;
#else
	struct lcd_regs *regs = get_regs(fbi);
#endif
	if (debugcount < 10)
		debugcount++;

	pixel_offset = (yoffset * ALIGN(fbi->mi->modes->xres, 16)) + xoffset;
	phys_addr = (unsigned int)(uintptr_t)fbi->fb_start +
		(pixel_offset * (fbi->mi->bits_per_pixel >> 3));


#ifdef CONFIG_VDMA
	/* Set VDMA Source address */
	writel(phys_addr, &ch_reg->src_addr);
#else
	writel(phys_addr, &regs->g_0);
#endif
}

static void set_screen(struct mmp_disp_info *fbi, struct mmp_disp_plat_info *mi)
{
	struct lcd_regs *regs = get_regs(fbi);
	struct dsi_info *di = NULL;
	u32 x, h_porch, vsync_ctrl, vec = 10;
	u32 xres = mi->modes->xres, yres = mi->modes->yres;
	u32 xres_z = mi->modes->xres, yres_z = mi->modes->yres;
	u32 bits_per_pixel = mi->bits_per_pixel;

	if (mi->phy_type & (DSI2DPI | DSI)) {
		di = (struct dsi_info *)mi->phy_info;
		if (!DISP_GEN4(fbi->mi->version))
			vec = ((di->lanes <= 2) ? 1 : 2)
				* 10 * di->bpp / 8 / di->lanes;
	}
	/* resolution, active */
	writel((mi->modes->yres << 16) | mi->modes->xres, &regs->screen_active);
	/* pitch, pixels per line */
	x = readl(&regs->g_pitch);
	x = (x & ~0xFFFF) | ((ALIGN(xres, 16) * bits_per_pixel) >> 3);
	writel(x, &regs->g_pitch);
	/* resolution, src size */
	writel((yres << 16) | xres, &regs->g_size);

	/* resolution, dst size */
	if (DISP_GEN4(fbi->mi->version)) {
		if (xres != xres_z || yres != yres_z)
			printf("resize not support for graphic layer of GEN4\n");
	} else {
		writel((yres_z << 16) | xres_z,	&regs->g_size_z);
	}

	/* h porch, left/right margin */
	if (mi->phy_type & (DSI2DPI | DSI)) {
		h_porch = (mi->modes->xres + mi->modes->right_margin) * vec / 10
			- mi->modes->xres;
		h_porch = (mi->modes->left_margin * vec / 10) << 16 | h_porch;
	} else {
		h_porch = (mi->modes->left_margin) << 16 | mi->modes->right_margin;
	}
	writel(h_porch, &regs->screen_h_porch);
	/* v porch, upper/lower margin */
	writel((mi->modes->upper_margin << 16) | mi->modes->lower_margin,
	       &regs->screen_v_porch);

	/* vsync ctrl */
	if (mi->phy_type & (DSI2DPI | DSI)) {
		vsync_ctrl = (((mi->modes->xres + mi->modes->right_margin) * vec / 10) << 16)
				| ((mi->modes->xres + mi->modes->right_margin) * vec / 10);
	} else {
		vsync_ctrl = ((mi->modes->xres + mi->modes->right_margin) << 16)
				| (mi->modes->xres + mi->modes->right_margin);
	}
	writel(vsync_ctrl, &regs->vsync_ctrl);  /* FIXME */

	/* blank color */
	writel(0x00000000, &regs->blank_color);
}

static void set_dumb_panel_control(struct mmp_disp_info *fbi,
		struct mmp_disp_plat_info *mi)
{
	u32 x;

	x = readl(fbi->reg_base + intf_ctrl(fbi->id)) & 0x00000001;
	x |= (fbi->is_blanked ? 0x7 : mi->dumb_mode) << 28;
	x |= (mi->modes->sync & 2) ? 0 : 0x00000008;
	x |= (mi->modes->sync & 1) ? 0 : 0x00000004;

	/*
	 * DC4_LITE didn't support below bits
	 */
	if (!DISP_GEN4_LITE(fbi->mi->version)) {
		x |= (mi->modes->sync & 8) ? 0 : 0x00000020;
		x |= mi->gpio_output_data << 20;
		x |= mi->gpio_output_mask << 12;
		x |= mi->panel_rgb_reverse_lanes ? 0x00000080 : 0;
	} else {
		/*
		 * For DC4_LITE, GATED_ENABLE and VSYNC_INV bits are in DUMB_CTRL;
		 * For DC4 and older version, they are in LCD_PN_CTRL1;
		 */
		x |= (1 << 16) | (1 << 18);
	}
	x |= mi->invert_pix_val_ena ? 0x00000010 : 0;
	x |= mi->invert_pixclock ? 0x00000002 : 0;

	writel(x, fbi->reg_base + intf_ctrl(fbi->id));  /* FIXME */
}

static void set_dumb_screen_dimensions(struct mmp_disp_info *fbi,
		struct mmp_disp_plat_info *mi)
{
	struct lcd_regs *regs = get_regs(fbi);
	struct dsi_info *di = NULL;
	int x;
	int y;
	int vec = 10;

	/* FIXME - need to double check (*3) and (*2) */
	if (mi->phy_type & (DSI2DPI | DSI)) {
		di = (struct dsi_info *)mi->phy_info;
		vec = ((di->lanes <= 2) ? 1 : 2) * 10 * di->bpp / 8 / di->lanes;
		if (di->master_mode) {
			if (DISP_GEN4(fbi->mi->version)) {
				/*
				 * If it is DC4_LITE, it has only one layer and one dsi,
				 * its master_control setting is also diffrent from DC4;
				 */
				if (DISP_GEN4_LITE(fbi->mi->version))
					writel(timing_master_config_dc4_lite(fbi->id),
					       fbi->reg_base +
					       TIMING_MASTER_CONTROL_DC4_LITE);
				else
				       writel(timing_master_config_dc4(fbi->id,
								       di->id - 1, di->id - 1),
					      fbi->reg_base + TIMING_MASTER_CONTROL_DC4);
			} else
				writel(timing_master_config(fbi->id,
							    di->id - 1, di->id - 1),
				       fbi->reg_base + TIMING_MASTER_CONTROL);
		}
	}

	x = mi->modes->xres + mi->modes->right_margin +
		mi->modes->hsync_len + mi->modes->left_margin;
	x = x * vec / 10;
	y = mi->modes->yres + mi->modes->lower_margin +
		mi->modes->vsync_len + mi->modes->upper_margin;

	writel((y << 16) | x, &regs->screen_size);
}

static int mmp_disp_set_par(struct mmp_disp_info *fb,
		struct mmp_disp_plat_info *mi)
{
	struct mmp_disp_info *fbi = fb;
	int pix_fmt;
	u32 x;

	/* Determine which pixel format we're going to use */
	pix_fmt = mi->pix_fmt;    /* choose one */

	if (pix_fmt < 0)
		return pix_fmt;
	fbi->pix_fmt = pix_fmt;

	set_pix_fmt(mi, pix_fmt);

	/* Configure graphics DMA parameters.
	 * Configure global panel parameters.
	 */
	set_screen(fbi, mi);
	/* Configure dumb panel ctrl regs & timings */
	set_dumb_panel_control(fbi, mi);
	set_dumb_screen_dimensions(fbi, mi);
	x = readl(fbi->reg_base + intf_ctrl(fbi->id));
	if (!(DISP_GEN4_LITE(fbi->mi->version) || DISP_GEN4_PLUS(fbi->mi->version))
			&& ((x & 1) == 0))
		writel(x | 1, fbi->reg_base + intf_ctrl(fbi->id));

	set_graphics_start(fbi, 0, 0);
	set_dma_control0(fbi);
	return 0;
}

static void mmp_disp_set_default(struct mmp_disp_info *fbi,
		struct mmp_disp_plat_info *mi)
{
	struct lcd_regs *regs = get_regs(fbi);
	u32 dma_ctrl1 = 0x20320081;
	u32 burst_length = (mi->burst_len == 16) ?
		CFG_CYC_BURST_LEN16 : CFG_CYC_BURST_LEN8;

	/* LCD_TOP_CTRL control reg */
	writel(TOP_CTRL_DEFAULT, fbi->reg_base + LCD_TOP_CTRL);

	/* DC4 and DC4_LITE don't have IOPAD register */
	if (!DISP_GEN4(fbi->mi->version)) {
		/* Configure default register values */
		writel(mi->io_pin_allocation_mode | burst_length,
		       fbi->reg_base + SPU_IOPAD_CONTROL);
	}
	/* enable 16 cycle burst length to get better formance */
	writel(0x00000000, &regs->blank_color);
	writel(0x00000000, &regs->g_1);
	writel(0x00000000, &regs->g_start);

	/* DC4_LITE don't have LCD_PN_CTRL1 */
	if (!DISP_GEN4_LITE(fbi->mi->version)) {
		/*
		 * vsync in LCD internal controller is always positive,
		 * we default configure dma trigger @vsync falling edge,
		 * so that DMA idle time between DMA frame done and
		 *  next DMA transfer begin can be as large as possible
		 */
		dma_ctrl1 |= CFG_VSYNC_INV_MASK;
		dma_ctrl_write(fbi, 1, dma_ctrl1);
	}
}

static void rect_fill(void *addr, int left, int up, int right,
		      int down, int width, unsigned int color, int pix_fmt)
{
	int i, j;
	for (j = up; j < down; j++)
		for (i = left; i < right; i++)
			if (pix_fmt == PIX_FMT_RGB565)
				*((unsigned short *)addr + j * width + i) = color;
			else if (pix_fmt == PIX_FMT_RGBA888)
				*((unsigned int *)addr + j * width + i) = color;
}

void test_panel(struct mmp_disp_info *fbi)
{
	struct mmp_disp_plat_info *mi = fbi->mi;
	int w = mi->modes->xres, h = mi->modes->yres, pix_fmt = fbi->pix_fmt;
	int x = w / 8, y = h / 8, bpp = 2;
	void *start = fbi->fb_start;
	int color_blue = 0x1f, color_green = 0x7e0, color_red = 0xf800;

	printf("panel test: white background, test RGB color\r\n");

	if (pix_fmt == PIX_FMT_RGB565) {
		color_blue = 0x1f;
		color_green = 0x7e0;
		color_red = 0xf800;
		bpp = 2;
	} else if (pix_fmt == PIX_FMT_RGBA888) {
		color_blue = 0xff0000ff;
		color_green = 0xff00ff00;
		color_red = 0xffff0000;
		bpp = 4;
	}

	memset(start, 0xff, w * h * bpp);
	udelay(50 * 1000);
	rect_fill(start, x, y, w - x, h - y, w, color_blue, pix_fmt);
	udelay(50 * 1000);
	rect_fill(start, x * 2, y * 2, w - x * 2, h - y * 2, w, color_green, pix_fmt);
	udelay(50 * 1000);
	rect_fill(start, x * 3, y * 3, w - x * 3, h - y * 3, w, color_red, pix_fmt);
	udelay(50 * 1000);

	flush_cache((unsigned long)start, (unsigned long)fbi->fb_size);
}

#ifdef CONFIG_VDMA
static void vdma_enable(struct mmp_disp_info *fbi)
{
	struct lcd_regs *regs = get_regs(fbi);
	struct mmp_vdma_reg *vdma_reg =
		(struct mmp_vdma_reg *)(fbi->vdma_reg_base);
	struct mmp_vdma_ch_reg *ch_reg = &vdma_reg->ch0_reg;
	u32 pitch, size, src_sz, height;
	u32 tmp, tmp1, tmp2, mask, set;

	if (!(DISP_GEN4_LITE(fbi->mi->version) ||
		DISP_GEN4_PLUS(fbi->mi->version))) {
		/*
		 * For DC4_LITE, hardware has fix vdma0 to graphic layer;
		 * For DC4:
		 * Select VDMA Channel 0: VMDA1
		 * LCD_VDMA_SEL_CTRL bit21:20->0 : PN graphic path use VMDA1
		 * LCD_VDMA_SEL_CTRL bit2:0->0 : VDMA1 map to PN graphic path
		 */
		tmp = readl(fbi->reg_base + LCD_VDMA_SEL_CTRL);
		tmp &= ~((0x3 << 20) | 0x7);
		writel(tmp, fbi->reg_base + LCD_VDMA_SEL_CTRL);
	}

	if (!DISP_GEN4(fbi->mi->version)) {
		/* Select PN Graphic SQU Line Buffer @0x2ac bit27:26 ->0 */
		tmp = readl(fbi->reg_base + LCD_SCALING_SQU_CTL_INDEX);
		tmp &= ~(0x3 << 26);
		writel(tmp, fbi->reg_base + LCD_SCALING_SQU_CTL_INDEX);
	}

	pitch = ((ALIGN(fbi->mi->modes->xres, 16)
				* fbi->mi->bits_per_pixel) >> 3) & 0xffff;

	/*
	 * Set source address in SQU of LCD Controller
	 * and destination address in SQU of VDMA
	 */
	if (DISP_GEN4(fbi->mi->version)) {
		writel(0x00000000, &regs->g_squln);
		writel(0x00000000, &ch_reg->dst_addr);
	} else {
		if (pitch * VDMA_SRAM_LINES > 64*1024) {
			printf("error: requested vdma size exceed 64KB!\n");
			hang();
		}
		/* The sram bank size is 64k and use the tail for vdma */
		writel(CONFIG_SRAM_BASE + 64 * 1024 - pitch * VDMA_SRAM_LINES,
		       fbi->reg_base + LCD_PN_SQULN_CTRL);
		writel(CONFIG_SRAM_BASE + 64 * 1024 - pitch * VDMA_SRAM_LINES,
		       &ch_reg->dst_addr);
	}

	/* Set and Enable Panel Graphic Path SQU Line Buffers */
	tmp2 = readl(fbi->reg_base + LCD_PN_SQULN_CTRL);
	tmp1 = tmp2;
	tmp1 &= ~0x3f;
	if (DISP_GEN4_LITE(fbi->mi->version))
		tmp1 |= ((VDMA_SRAM_LINES - 1) << 1) | 0x1;
	else
		tmp1 |= ((VDMA_SRAM_LINES / 2 - 1) << 1) | 0x1;
	if (tmp1 != tmp2)
		writel(tmp1, fbi->reg_base + LCD_PN_SQULN_CTRL);

	height = fbi->mi->modes->yres & 0xffff;
	src_sz = pitch * height;
	size = height << 16 | pitch;
	/* FIXME: set dst and src with the same pitch */
	pitch |= pitch << 16;
	writel(pitch, &ch_reg->pitch);
	writel(src_sz, &ch_reg->src_size);
	writel(size, &ch_reg->dst_size);

	tmp = readl(&ch_reg->ctrl) & (~(0xff << 8));
	if (DISP_GEN4_LITE(fbi->mi->version))
		/* Allocated memory in SQU line size is SQU_LN_SZ + 1 */
		tmp |= (VDMA_SRAM_LINES - 1) << 8;
	else
		tmp |= VDMA_SRAM_LINES << 8;
	writel(tmp, &ch_reg->ctrl);

	/* Enable VDMA Channel 0 */
	if (DISP_GEN4(fbi->mi->version)) {
		mask = AXI_RD_CNT_MAX(0x1f) | CH_ENA(1) | DC_ENA(1);
		set = AXI_RD_CNT_MAX(0x10) | CH_ENA(1);

		/*
		 * dec_ctrl and clk_ctrl are removed in DC4_LITE.
		 */
		if (!(DISP_GEN4_LITE(fbi->mi->version) ||
			DISP_GEN4_PLUS(fbi->mi->version))) {
			/* FIXME: VDMA_DEC_CTRL could only be touched before
			 * VDMA channel enable, otherwise it may cause VDMA AXI crash,
			 * so by default, we enable decompression ctrl for VDMA
			 * channel0, only dynamically configure HDR_ENA in
			 * VDMA_CH_CTRL*/
			tmp = readl((uintptr_t)&vdma_reg->dec_ctrl) | (1 << 4) | 1;
			writel(tmp, &vdma_reg->dec_ctrl);
			writel(VDMA_CLK_CTRL, &vdma_reg->clk_ctrl);
		}
	} else {
		mask = (0x3 << 4) | (0x3 << 6) | 0x3;
		set =  (0x2 << 4) | (0x2 << 6) | 0x1;
	}
	tmp2 = readl(&ch_reg->ctrl);
	tmp1 = tmp2;
	tmp1 &= ~mask;
	tmp1 |= set;
	if (tmp1 != tmp2)
		writel(tmp1, &ch_reg->ctrl);

	/*Load All VDMA Path 0 Configure Registers */
	tmp = readl(&vdma_reg->main_ctrl) | (1 << 24);
	writel(tmp, &vdma_reg->main_ctrl);
}
#endif

static u32 mmp_disp_calc_buf_size(struct mmp_disp_plat_info *mi)
{
	u32 size, pitch, hdr_size, temp;

	if (DISP_GEN4(mi->version) && !DISP_GEN4_LITE(mi->version)) {
		/* for DC4 which support decompression:
		 * decompress mode buffer mapping:
		 * _____________________ ->fb_start_dma
		 * |                   |
		 * |___________________| ->hdr_addr_start
		 * |___________________|
		 *
		 * start address of source image in DDR should be 256 bytes align,
		 * source pitch should be 256bytes aligned when decompression is enabled;
		 * src_size = 256_align(xres * bytes_per_pixel) * yres *3
		 *
		 * hdr_size for decompression mode
		 * 256 bytes per unit, and 4 bits(1/2 byte) for one unit status.
		 * the buffer to save header should be pitch aligned for next fb start
		 * address calculation: pitch_align(256_align(xres * bytes_per_pixel) * yres /512)
		 */
		pitch = ALIGN(mi->modes->xres * 4 , 256);
		/* one source image size */
		size = pitch *  ALIGN(mi->modes->yres, 4);
		/* one decoder-header size aligned to pitch */
		temp = size / 512;
		if (temp % pitch)
			hdr_size = (temp / pitch + 1) * pitch;
		else
			hdr_size = temp;
		size += hdr_size;
	} else {
		/* currently kernel use RGBA888/BGRA888 format which is 4bytes per pixel */
		size = ALIGN(mi->modes->xres, 16) *
			ALIGN(mi->modes->yres, 4) * 4;
	}

	return size;
}

static void mmp_disp_init_info(struct mmp_disp_info *fbi,
		struct mmp_disp_plat_info *mi)
{
	struct dsi_info *di = NULL;

	memset(fbi, 0, sizeof(struct mmp_disp_info));
	/* Initialize private data */
	fbi->panel_rbswap = mi->panel_rbswap;
	fbi->id = mi->index;
	mmp_disp_fbi = fbi;
	fbi->mi = mi;
	fbi->active = mi->active;

	/* Map LCD controller registers */
	fbi->reg_base = (void *)DISPLAY_CONTROLLER_BASE;
#ifdef CONFIG_VDMA
	/* Map VDMA Controller Registers */
	fbi->vdma_reg_base = (void *)VDMA_CONTROLLER_BASE;
#endif

	if (mi->phy_type & (DSI2DPI | DSI)) {
		di = (struct dsi_info *)mi->phy_info;
		di->regs = DSI1_REG_BASE;     /* DSI 1 */
	}

	/* Get DISP Subsystem Verison */
	fbi->mi->version = readl(fbi->reg_base + LCD_VERSION);
}

#define ESC_CLK 52000000
static void mmp_disp_set_clk(struct mmp_disp_info *fbi)
{
	struct dsi_info *di = (struct dsi_info *)fbi->mi->phy_info;
	struct lcd_videomode *modes = &fbi->mi->modes[0];
	u32 total_w, total_h, bitclk, x, path_rate;

	total_w = modes->xres + modes->left_margin +
		modes->right_margin + modes->hsync_len;
	total_h = modes->yres + modes->upper_margin +
		modes->lower_margin + modes->vsync_len;

	path_rate = total_w  * total_h * modes->refresh;
	bitclk = path_rate * di->bpp / di->lanes;

	if (!DISP_GEN4(fbi->mi->version)) {
		x = (di->lanes > 2) ? 2 : 3;
		path_rate = (bitclk >> x);
	} else if (path_rate < ESC_CLK)
		path_rate = ESC_CLK;

	path_rate /= 1000000;
	bitclk /= 1000000;
	if (fbi->mi->calculate_sclk)
		fbi->mi->calculate_sclk(fbi, bitclk, path_rate);

	set_clock_divider(fbi);
}

void *mmp_disp_init(struct mmp_disp_plat_info *mi)
{
	struct mmp_disp_info *fbi = malloc(sizeof(struct mmp_disp_info));
#if defined(DEBUG_MMP_DISP)
	u32 i;
#endif
	u32 x;
	struct dsi_info *di = NULL;

	mmp_disp_init_info(fbi, mi);
	set_clock_divider(fbi);

	if (mi->phy_type & (DSI2DPI | DSI)) {
		di = (struct dsi_info *)mi->phy_info;
		/* phy interface init */
		mmp_disp_dsi_init_dphy(fbi);
		/* if phy_info changed, we need to update agian */
		di = (struct dsi_info *)mi->phy_info;
		/* phy_info maybe updated for some platform, so need to set dsi_info again */
		if (!di->regs)
			di->regs = DSI1_REG_BASE;     /* DSI 1 */
	}

	fbi->fb_size = mmp_disp_calc_buf_size(mi);
	/* use the second buffer of framebuffer reserved for display */
	fbi->fb_start = (void*)(long)(FRAME_BUFFER_ADDR + fbi->fb_size);

	g_disp_start_addr = (unsigned long)fbi->fb_start;
	g_disp_buf_size = (unsigned long)fbi->fb_size;

	/* clear the frame buffer to 0x0 */
	memset(fbi->fb_start, 0x00, fbi->fb_size);
	flush_cache(g_disp_start_addr, g_disp_buf_size);

	mmp_disp_set_clk(fbi);
	mmp_disp_set_default(fbi, mi);
	mmp_disp_set_par(fbi, mi);

#ifdef CONFIG_VDMA
	vdma_enable(fbi);
#endif

	if (DISP_GEN4_LITE(fbi->mi->version) || DISP_GEN4_PLUS(fbi->mi->version)) {
		x = readl(fbi->reg_base + intf_ctrl(fbi->id));
		writel(x | 1, fbi->reg_base + intf_ctrl(fbi->id));
	}

	/* Load All Configure Registers with Shadow Registers */
	if (DISP_GEN4(fbi->mi->version) &&
		!(DISP_GEN4_LITE(fbi->mi->version) || DISP_GEN4_PLUS(fbi->mi->version)))
		writel(1 << 30, fbi->reg_base + LCD_SHADOW_CTRL);

	if (mi->phy_type & (DSI2DPI | DSI))
		/* panel interface init */
		mmp_disp_dsi_init_panel(fbi);

	/* dump all lcd and dsi registers for debug purpose */
#if defined(DEBUG_MMP_DISP)
	printf("lcd regs:\n");
	for (i = 0; i < 0x300; i += 4) {
		if (!(i % 16) && i)
			printf("\n0x%3x: ", i);
		printf(" %8x", readl(fbi->reg_base + i));
	}
	if (mi->phy_type & (DSI2DPI | DSI)) {
		printf("\n dsi regs:");
		for (i = 0x0; i < 0x200; i += 4) {
			if (!(i % 16))
				printf("\n0x%3x: ", i);
			printf(" %8x", readl(di->regs + i));
		}
	}
	printf("\n");
	test_panel(fbi);
#endif

	return (void *)fbi;
}

#ifdef CONFIG_OF_LIBFDT
/* check the display reserve mem in dtb,  modify or add it if neccessary */
int handle_disp_rsvmem(struct mmp_disp_plat_info *mi)
{
	uint64_t addr = 0, size = 0;
	u32 disp_reserve_size;
	int total = fdt_num_mem_rsv(working_fdt);
	char cmd[128];
	int j, err = 0;

	for (j = 0; j < total; j++) {
		err = fdt_get_mem_rsv(working_fdt, j, &addr, &size);
		if (err < 0) {
			printf("libfdt fdt_get_mem_rsv():  %s\n", fdt_strerror(err));
			goto out;
		}
		if (addr == FRAME_BUFFER_ADDR)
			break;
	}

	disp_reserve_size = mmp_disp_calc_buf_size(mi);
	/* totally reserve 3 framebuffer size, align to pagesize, also add one guard page */
	disp_reserve_size = ALIGN(disp_reserve_size * 3, PAGE_SIZE) + PAGE_SIZE;

	if (addr != FRAME_BUFFER_ADDR) {
		err = -1;
		printf("Error!! Didn't find framebuffer @0x%08x reserved in dtb,\n"
			"still try to add the rsvmem, but can't make sure it will succeed!\n",
			FRAME_BUFFER_ADDR);
		goto fdt_rsvmem_add;
	}

	if (size != disp_reserve_size) {
		printf("change the disp rsvmem @0x%08x from size 0x%08x to 0x%08x\n",
				(u32)(addr & 0xffffffff),
				(u32)(size & 0xffffffff),
				disp_reserve_size);

		sprintf(cmd, "fdt rsvmem delete 0x%x", j);
		run_command(cmd, 0);
	} else
		goto out;

fdt_rsvmem_add:
	sprintf(cmd, "fdt rsvmem add 0x%x 0x%x", FRAME_BUFFER_ADDR, disp_reserve_size);
	run_command(cmd, 0);

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "fdt set /soc/axi/fbbase marvell,fb-mem <0x%x>", FRAME_BUFFER_ADDR);
	run_command(cmd, 0);
out:
	return err;
}
#endif

void show_debugmode_logo(struct mmp_disp_plat_info *mi, const unsigned char *picture)
{
	char cmd[100];
	struct lcd_videomode *fb_mode = mi->modes;
	int xres = fb_mode->xres, yres = fb_mode->yres;
	int wide = 446, high = 148;

	memset((void *)g_disp_start_addr, 0x0, g_disp_buf_size);
	sprintf(cmd, "bmp display %p %d %d",
		picture, (xres - wide) / 2, (yres - high) / 2);
	run_command(cmd, 0);
	flush_cache(g_disp_start_addr, g_disp_buf_size);
}

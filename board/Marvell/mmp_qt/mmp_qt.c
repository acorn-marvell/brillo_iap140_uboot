/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <mvmfp.h>
#include <i2c.h>
#include <asm/arch/mfp.h>
#include <asm/gpio.h>
#ifdef CONFIG_GENERIC_MMC
#include <sdhci.h>
#include <mv_sdhci.h>
#endif

#if defined(CONFIG_MMP_DISP)
#include <mmp_disp.h>
#include <video_fb.h>
#include "../common/panel.h"
#include "../common/logo_pxa988.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

#define PMIC_I2C_BUS 2

int board_early_init_f(void)
{
#if 0
	u32 mfp_cfg[] = {
		/* Keypad */
		GPIO016_KP_DKIN1,
		GPIO017_KP_DKIN2,

		/* AP UART */
		GPIO047_UART1_RXD,
		GPIO048_UART1_TXD,

		/* LCD Backlight */
		GPIO032_GPIO_32,
		/* LCD Reset */
		GPIO004_GPIO_4,

#if 0
		/* Sensor hub */
		GPIO073_GPS_I2C_SCL,
		GPIO074_GPS_I2C_SDA,
#endif
		/* 0xd401_3800: for AP */
		GPIO073_CI2C_SCL_3,
		GPIO074_CI2C_SDA_3,

		/* I2C */
		GPIO079_CI2C_SCL,
		GPIO080_CI2C_SDA,
		GPIO087_CI2C_SCL_2,
		GPIO088_CI2C_SDA_2,

		/* Rear raw camera sensor */
		GPIO053_OVT_SCL0,
		GPIO054_OVT_SDA0,

		MFP_EOC		/*End of configureation*/
	};

	mfp_config(mfp_cfg);
#endif
	return 0;
}

void reset_cpu(unsigned long ignored)
{
	printf("rebooting...\n");
}

#ifdef CONFIG_GENERIC_MMC
static u32 mfp_mmc_cfg[] = {
	/* MMC1 */
	MMC1_DAT3_MMC1_DAT3,
	MMC1_DAT2_MMC1_DAT2,
	MMC1_DAT1_MMC1_DAT1,
	MMC1_DAT0_MMC1_DAT0,
	MMC1_CLK_MMC1_CLK,
	MMC1_CMD_MMC1_CMD,
	MMC1_CD_MMC1_CD,
	MMC1_WP_MMC1_WP,

	/* MMC3 */
	ND_IO7_MMC3_DAT7,
	ND_IO6_MMC3_DAT6,
	ND_IO5_MMC3_DAT5,
	ND_IO4_MMC3_DAT4,
	ND_IO3_MMC3_DAT3,
	ND_IO2_MMC3_DAT2,
	ND_IO1_MMC3_DAT1,
	ND_IO0_MMC3_DAT0,
	ND_CLE_SM_OEN_MMC3_CMD,
	SM_SCLK_MMC3_CLK,

	MFP_EOC		/*End of configureation*/
};

int board_mmc_init(bd_t *bd)
{
	ulong mmc_base_address[CONFIG_SYS_MMC_NUM] = CONFIG_SYS_MMC_BASE;
	u8 i;

	/* configure MFP's */
	mfp_config(mfp_mmc_cfg);
	for (i = 0; i < CONFIG_SYS_MMC_NUM; i++) {
		if (mv_sdh_init(mmc_base_address[i], 104*1000000, 0,
				SDHCI_QUIRK_32BIT_DMA_ADDR))
			return 1;
	}

	return 0;
}
#endif

int board_init(void)
{
	return 0;
}


#if defined(CONFIG_MMP_DISP)
#define MMP_FPGA_FB_XRES			800
#define MMP_FPGA_FB_YRES			480

static struct fb_videomode video_modes[] = {
	[0] = {
		/* for old Jasper(Bonnel) panel */
		.pixclock = 62500,
		.refresh = 60,
		.xres = MMP_FPGA_FB_XRES,
		.yres = MMP_FPGA_FB_YRES,
		.hsync_len = 4,
		.left_margin = 212,
		.right_margin = 40,
		.vsync_len = 4,
		.upper_margin = 31,
		.lower_margin = 10,
		.sync = FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
};

static struct mmp_disp_plat_info mmp_mipi_lcd_info = {
	.id                     = "GFX Layer",
	.sclk_src               = 500000000,
	.sclk_div               = 0xe0001210,
	.num_modes              = ARRAY_SIZE(video_modes),
	.modes                  = video_modes,
	.pix_fmt                = PIX_FMT_RGBA888,
	.burst_len              = 16,
	/*
	 * don't care about io_pin_allocation_mode and dumb_mode
	 * since the panel is hard connected with lcd panel path
	 * and dsi1 output
	 */
	.dumb_mode               = 6,
	.panel_rgb_reverse_lanes = 0,
	.invert_composite_blank = 0,
	.invert_pix_val_ena     = 0,
	.invert_pixclock        = 0,
	.panel_rbswap           = 0,
	.active                 = 1,
	.enable_lcd             = 1,
	.max_fb_size            = MMP_FPGA_FB_XRES *
					MMP_FPGA_FB_YRES * 8 + 4096,
};

#define LCD_RST_GPIO1	4
#define BOOST_EN_5V	6

static GraphicDevice ctfb;
void *lcd_init(void)
{
	void *ret;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;

	ret = (void *)mmp_disp_init(fb);
	return ret;
}

struct mmp_disp_info *fbi;
void *video_hw_init(void)
{
	struct mmp_disp_plat_info *mi;
	unsigned long t1, hsynch, vsynch;

	fbi = lcd_init();
	mi = fbi->mi;

	ctfb.winSizeX = ALIGN(mi->modes->xres, 16);
	ctfb.winSizeY = mi->modes->yres;

	/* calculate hsynch and vsynch freq (info only) */
	t1 = (mi->modes->left_margin + mi->modes->xres +
	      mi->modes->right_margin + mi->modes->hsync_len) / 8;
	t1 *= 8;
	t1 *= mi->modes->pixclock;
	t1 /= 1000;
	hsynch = 1000000000L / t1;
	t1 *= (mi->modes->upper_margin + mi->modes->yres +
	       mi->modes->lower_margin + mi->modes->vsync_len);
	vsynch = 1000000000L / t1;

	/* fill in Graphic device struct */
	sprintf(ctfb.modeIdent, "%dx%dx%d %ldkHz %ldHz", ctfb.winSizeX,
		ctfb.winSizeY, mi->bits_per_pixel, (hsynch / 1000),
		vsynch);

	ctfb.frameAdrs = (unsigned int)(uintptr_t) fbi->fb_start;
	ctfb.plnSizeX = ctfb.winSizeX;
	ctfb.plnSizeY = ctfb.winSizeY;
	ctfb.gdfBytesPP = 4;
	ctfb.gdfIndex = GDF_32BIT_X888RGB;

	ctfb.isaBase = 0x9000000;
	ctfb.pciBase = (unsigned int)(uintptr_t) fbi->fb_start;
	ctfb.memSize = fbi->fb_size;

	/* Cursor Start Address */
	ctfb.dprBase = (unsigned int)(uintptr_t) fbi->fb_start + (ctfb.winSizeX
			* ctfb.winSizeY * ctfb.gdfBytesPP);
	if ((ctfb.dprBase & 0x0fff) != 0) {
		/* allign it */
		ctfb.dprBase &= 0xfffff000;
		ctfb.dprBase += 0x00001000;
	}
	ctfb.vprBase = (unsigned int)(uintptr_t) fbi->fb_start;
	ctfb.cprBase = (unsigned int)(uintptr_t) fbi->fb_start;

	return &ctfb;
}

void show_logo(void)
{
	char cmd[100];
	int wide, high;

	wide = 330;
	high = 236;
	sprintf(cmd, "bmp display %p %d %d", MARVELL_PXA988,
		(MMP_FPGA_FB_XRES - wide) / 2, (MMP_FPGA_FB_YRES - high) / 2);
	run_command(cmd, 0);
	flush_cache(DEFAULT_FB_BASE, DEFAULT_FB_SIZE);
}

void lcd_blank(void)
{
	memset((unsigned short *)DEFAULT_FB_BASE, 0x0, ctfb.memSize);
}
#endif

#ifdef CONFIG_MISC_INIT_R

int misc_init_r(void)
{
#if defined(CONFIG_MMP_DISP)
	show_logo();
#endif

#if defined(CONFIG_HELAN3_POWER)
	/* init DDR533 mode by default */
	helan3_fc_init(1);
#endif

	return 0;
}
#endif /* CONFIG_MISC_INIT_R */

#if defined(CONFIG_CMD_NET)
int board_eth_init(bd_t *bis)
{
	int res = -1;
#if defined(CONFIG_MV_UDC)
	if (usb_eth_initialize(bis) >= 0)
		res = 0;
#endif
	return res;
}
#endif

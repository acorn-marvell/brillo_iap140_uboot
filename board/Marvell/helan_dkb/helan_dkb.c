/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <malloc.h>
#include <pxa_amp.h>
#include <mvmfp.h>
#include <i2c.h>
#include <asm/sizes.h>
#include <asm/arch/mfp.h>
#include <asm/gpio.h>
#include <power/pxa988_freq.h>
#include <power/mmp_idle.h>
#include <emmd_rsv.h>
#ifdef CONFIG_GENERIC_MMC
#include <sdhci.h>
#include <mv_sdhci.h>
#endif
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <asm/arch/features.h>

#if defined(CONFIG_MMP_DISP)
#include <mmp_disp.h>
#include <video_fb.h>
#include "../common/panel.h"
#include "../common/logo_pxa988.h"
#include "../common/logo_ramdump.h"
#endif

#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#include "../common/obm2osl.h"
#include "../common/cmdline.h"
#include <mv_recovery.h>

#define PMIC_I2C_BUS 2

#ifdef CONFIG_RAMDUMP
#include "../common/ramdump.h"
/*
 * Platform memory map definition for ramdump, see ramdump.h
 */
static struct rd_mem_desc rd_mems[] = {
	{ 0, 0},	/* skip ion area */
	{ 0, 0 },	/* left memory */
};

struct rd_desc ramdump_desc = {
	.rdc_addr = 0x8140400,
	.mems = &rd_mems[0],
	.nmems = sizeof(rd_mems)/sizeof(rd_mems[0]),
};
#endif

#define DKB_10 0x10
#define DKB_11 0x11
#define DKB_T7V1 0x71
#define DKB_T7V2 0x72

DECLARE_GLOBAL_DATA_PTR;

static u8 dkb_version;
static u8 pwr_chg_enable;
static u8 ddr_mode;
static int fastboot;

#define RAMDUMP_USB_DUMP 1
#define RAMDUMP_SD_DUMP 2

int board_early_init_f(void)
{
	u32 mfp_cfg[] = {
		/* AP UART*/
		GPIO047_UART1_RXD,
		GPIO048_UART1_TXD,

		/* KeyPad */
		GPIO000_KP_MKIN0,
		GPIO001_KP_MKOUT0,
		GPIO002_KP_MKIN1,
		GPIO003_KP_MKOUT1,
		GPIO004_KP_MKIN2,
		GPIO005_KP_MKOUT2,
		GPIO006_KP_MKIN3,

		/* LCD BL PWM */
		GPIO032_GPIO_32,
		/* LCD RESET */
		GPIO004_GPIO_4,

		/* I2C */
		GPIO053_CI2C_SCL,
		GPIO054_CI2C_SDA,
		GPIO087_CI2C_SCL_2,
		GPIO088_CI2C_SDA_2,

		MFP_EOC		/*End of configureation*/
	};

	mfp_config(mfp_cfg);
	pxa_amp_init();
	return 0;
}

void reset_cpu(unsigned long ignored)
{
#ifdef CONFIG_POWER_88PM800
	printf("helan dkb board rebooting...\n");
	pmic_reset_bd();
#endif
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

#define NULL_TP		0x0
#define SYNAPT_TP	0x1
#define GOODIX_TP	0x2
#define FT5306_TP	0x3

static int touch_type;
static void helandkb_touch_detect(void)
{
	u8 data;
	int ret;
	char slave_addr[] = {0x22, 0x14, 0x5d, 0x39, 0xff};
	int index = 0;

	i2c_set_bus_num(2);

	/*
	 * touch_type
	 * 0x1: synaptics touch
	 * 0x2: goodix touch
		 * note: in uboot, goodix touch slave addr is 0x14
		 *       in kernel, its addr is 0x5d
		 * determined by RST and INT pins' time sequence
		 * should read two addr incase reboot from kernel
	 * 0x3: ft5306 touch
	 */
	while (slave_addr[index] != 0xff) {
		ret = i2c_read(slave_addr[index], 0x0, 1, &data, 1);
		if (!ret)
			break;
		index++;
	}

	switch (index) {
	case 0:
		touch_type = SYNAPT_TP;
		break;
	case 1:
	case 2:
		touch_type = GOODIX_TP;
		break;
	case 3:
		touch_type = FT5306_TP;
		break;
	default:
		touch_type = NULL_TP;
		break;
	}
}

/* 0 - 400MHz; 1 - 533Mhz*/
static void get_ddr_mode(void)
{
	struct OBM2OSL *params;
	params = (struct OBM2OSL *)(*(u32 *)CONFIG_OBM_PARAM_ADDR);

	if (!params || (params->ddr_mode >= 2)) {
		printf("WARNING: no available ddr mode !!!\n");
		/* use 400Mhz by default*/
		ddr_mode = 0;
	} else {
		ddr_mode = params->ddr_mode;
	}
}

int board_init(void)
{
	emmd_page = (struct emmd_page *)CRASH_BASE_ADDR;
#ifdef CONFIG_TZ_HYPERVISOR
	read_fuse_info();
#endif

	get_ddr_mode();
	if (cpu_is_pxa1088_a0() && ddr_mode)
		panic("Please burn A0 chip with DDR400 BLF, Don't use DDR533 BLF!\n");
	if (emmd_page->indicator == 0x454d4d44) {
		/* clear this indicator for prevent the re-entry */
		printf("Detect RAMDUMP signature!!\n");
		emmd_page->indicator = 0;
		flush_cache((unsigned long)(&emmd_page->indicator),
			    (unsigned long)sizeof(emmd_page->indicator));
		fastboot = 1;
	}
	return 0;
}

#ifdef CONFIG_POWER_88PM800
void board_pmic_power_fixup(struct pmic *p_power)
{
	u32 val;
	/* Set VCC_main(buck1) as 1.2v*/
	val = 0x30;
	pmic_reg_write(p_power, 0x3c, val);
	pmic_reg_write(p_power, 0x3d, val);
	pmic_reg_write(p_power, 0x3e, val);
	pmic_reg_write(p_power, 0x3f, val);

	/* Set buck4 as 1.8V */
	val = 0x54;
	pmic_reg_write(p_power, 0x42, val);
	pmic_reg_write(p_power, 0x43, val);
	pmic_reg_write(p_power, 0x44, val);
	pmic_reg_write(p_power, 0x45, val);


	/* Set buck1 ramp up speed to 12.5mV/us */
	pmic_reg_read(p_power, 0x78, &val);
	val &= ~(7 << 3);
	val |= (0x6 << 3);
	pmic_reg_write(p_power, 0x78, val);

	/* Set buck1 audio mode as 0.8V */
	val = 0x10;
	pmic_reg_write(p_power, 0x38, val);

	/* enable buck1 sleep modewhen SLEEpn = '0' */
	pmic_reg_read(p_power, 0x5a, &val);
	val &= 0xfc;
	val |= 0x2;
	pmic_reg_write(p_power, 0x5a, val);

	/* Set LDO8 as 3.1V for LCD  */
	marvell88pm_set_ldo_vol(p_power, 8, 3100000);

	/* Set LDO12 as 3.0V for SD IO/bus */
	marvell88pm_set_ldo_vol(p_power, 12, 3000000);

	/* Set LDO13 as 3.0V for SD Card */
	marvell88pm_set_ldo_vol(p_power, 13, 3000000);

	/* Set LDO14 as 3.0V for eMMC */
	marvell88pm_set_ldo_vol(p_power, 14, 3000000);

	/* Set LDO15 as 1.8V for LCD  */
	marvell88pm_set_ldo_vol(p_power, 15, 1800000);

	/* Set vbuck1 sleep voltage is 0.8V */
	val = 0x10;
	pmic_reg_write(p_power, 0x30, val);
}

void board_pmic_gpadc_fixup(struct pmic *p_gpadc)
{
	u32 val, buf0, buf1;

	/*
	 * enable gpadc1 to detect battery, by default
	 * enable gpadc3 to measure board version
	 * enable vbat to measure battery voltage, by default
	 */
	pmic_reg_read(p_gpadc, 0x2, &val);
	val |= 0x22;
	pmic_reg_write(p_gpadc, 0x2, val);
	/* set bias current for gpadc1: 11uA */
	pmic_reg_read(p_gpadc, 0xc, &val);
	val |= 0x2;
	pmic_reg_write(p_gpadc, 0xc, val);
	/* enable gpadc1 bias */
	pmic_reg_read(p_gpadc, 0x14, &val);
	val |= 0x22;
	pmic_reg_write(p_gpadc, 0x14, val);
	/* enable gpadc1 battery detection */
	pmic_reg_read(p_gpadc, 0x8, &val);
	val |= 0x60;
	pmic_reg_write(p_gpadc, 0x8, val);

	mdelay(1);
	/* get dkb version */
	pmic_reg_read(p_gpadc, 0xa6, &buf0);
	pmic_reg_read(p_gpadc, 0xa7, &buf1);

	val = (buf0 << 4) | (buf1 & 0xf);
	val = ((val & 0xfff) * 7 * 100) >> 11;
	printf("board id voltage value = %d\n", val);

	/*
	 * board id:
	 * helandkb1.0->DKB_10     0.9V
	 * helandkb1.1->DKB_11     1.2V
	 * t7 tablet1.1->DKB_T7V1   0.6V
	 * t7 tablet1.2->DKB_T7V1   0.3V
	 */
	if ((val < 1050) && (val > 750))
		dkb_version = DKB_10;
	else if ((val < 1500) && (val > 1050))
		dkb_version = DKB_11;
	else if ((val < 750) && (val > 450))
		dkb_version = DKB_T7V1;
	else
		dkb_version = DKB_T7V2;
}

int power_init_board(void)
{
	if (pmic_init(PMIC_I2C_BUS))
		return -1;
	return power_init_common();
}
#endif

#if defined(CONFIG_MMP_DISP)
#define HELAN_QHD_FB_XRES		540
#define HELAN_QHD_FB_YRES		960
#define T7_FB_XRES		1024
#define T7_FB_YRES		600

#define PWM3_BASE		0xD401AC00
#define APB_CLK_REG_BASE	0xD4015000
#define APMU_CLK_REG_BASE	0xD4282800

#define APBC_PWM2_CLK_RST	(APB_CLK_REG_BASE + 0x14)
#define APBC_PWM3_CLK_RST	(APB_CLK_REG_BASE + 0x18)
#define APMU_LCD_DSI_CLK_RES_CTRL	(APMU_CLK_REG_BASE + 0x4c)
#define PWM_CR3		(PWM3_BASE + 0x00)
#define PWM_DCR		(PWM3_BASE + 0x04)
#define PWM_PCR		(PWM3_BASE + 0x08)

#define EMEI_EMMC_BMP_BASE	(0xE00000/512)
#define EMEI_EMMC_BMP_SIZE	(0x100000/512)

static struct mmp_disp_plat_info mmp_mipi_info_saved;

static struct dsi_info helan_qhd_dsi = {
	.id = 1,
	.lanes = 2,
	.bpp = 24,
	.burst_mode = DSI_BURST_MODE_BURST,
	.rgb_mode = DSI_LCD_INPUT_DATA_RGB_MODE_888,
	.hbp_en = 1,
	.master_mode = 1,
};

/* t7: use DSI2DPI bridge, and 4 lanes */
static struct dsi_info t7_dsiinfo = {
	.id = 1,
	.lanes = 4,
	.bpp = 24,
	.rgb_mode = DSI_LCD_INPUT_DATA_RGB_MODE_888,
	.burst_mode = DSI_BURST_MODE_BURST,
	.hbp_en = 1,
	.hfp_en = 1,
};

static struct lcd_videomode video_qhd_modes[] = {
	[0] = {
		/* we should align the follwoing settings with kernel driver */
		.pixclock = 19230, /* 1/52MHz */
		.refresh = 50,
		.xres = HELAN_QHD_FB_XRES,
		.yres = HELAN_QHD_FB_YRES,
		.hsync_len = 2,
		.left_margin = 10,
		.right_margin = 68,
		.vsync_len = 2,
		.upper_margin = 6,
		.lower_margin = 6,
		.sync = FB_SYNC_VERT_HIGH_ACT
			| FB_SYNC_HOR_HIGH_ACT,
	},
};

#define HELAN_720P_FB_XRES		720
#define HELAN_720P_FB_YRES		1280

static struct dsi_info helan_720p_dsi = {
	.id = 1,
	.lanes = 4,
	.bpp = 24,
	.burst_mode = DSI_BURST_MODE_BURST,
	.rgb_mode = DSI_LCD_INPUT_DATA_RGB_MODE_888,
	.hbp_en = 1,
	.master_mode = 1,
};

static struct lcd_videomode video_720p_modes[] = {
	[0] = {
		/* we should align the follwoing settings with kernel driver */
		.refresh = 60,
		.xres = HELAN_720P_FB_XRES,
		.yres = HELAN_720P_FB_YRES,
		.hsync_len = 2,
		.left_margin = 40,
		.right_margin = 150,
		.vsync_len = 2,
		.upper_margin = 8,
		.lower_margin = 10,
		.sync = FB_SYNC_VERT_HIGH_ACT
			| FB_SYNC_HOR_HIGH_ACT,
	},
};

static struct lcd_videomode t7_video_modes[] = {
	[0] = {
		.refresh = 60,
		.xres = T7_FB_XRES,
		.yres = T7_FB_YRES,
		.hsync_len = 20,
		.left_margin = 150,
		.right_margin = 150,
		.vsync_len = 3,
		.upper_margin = 16,
		.lower_margin = 16,
		.sync = FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
};


static struct mmp_disp_plat_info mmp_mipi_lcd_info = {
	.index = 0,
	.id = "GFX Layer",
	/* FIXME */
	.sclk_src = 416000000,
	.sclk_div = 0x40001104,
	.num_modes = ARRAY_SIZE(video_720p_modes),
	.modes = video_720p_modes,
	.pix_fmt = PIX_FMT_RGBA888,
	.burst_len = 16,
	/*
	 * don't care about io_pin_allocation_mode and dumb_mode
	 * since the panel is hard connected with lcd panel path
	 * and dsi1 output
	 */
	.panel_rgb_reverse_lanes = 0,
	.invert_composite_blank = 0,
	.invert_pix_val_ena = 0,
	.invert_pixclock = 0,
	.panel_rbswap = 0,
	.active = 1,
	.enable_lcd = 1,
	.phy_type = DSI,
	.phy_info = &helan_720p_dsi,
	.dsi_panel_config = lg_720p_init_config,
	.dynamic_detect_panel = 0,
	.panel_manufacturer_id = 0,
};


#define HELAN_1080P_FB_XRES		1080
#define HELAN_1080P_FB_YRES		1920

static struct dsi_info helan_1080p_dsi = {
	.id = 1,
	.lanes = 4,
	.bpp = 24,
	.burst_mode = DSI_BURST_MODE_BURST,
	.rgb_mode = DSI_LCD_INPUT_DATA_RGB_MODE_888,
	.hbp_en = 1,
	.master_mode = 1,
};

static struct lcd_videomode video_1080p_modes[] = {
	[0] = {
		/* we should align the follwoing settings with kernel driver */
		.refresh = 54,
		.xres = HELAN_1080P_FB_XRES,
		.yres = HELAN_1080P_FB_YRES,
		.hsync_len = 2,
		.left_margin = 40,
		.right_margin = 132,
		.vsync_len = 2,
		.upper_margin = 2,
		.lower_margin = 30,
		.sync = FB_SYNC_VERT_HIGH_ACT
			| FB_SYNC_HOR_HIGH_ACT,
	},
};

#define LCD_RST_GPIO1	1
#define BOOST_EN_5V	107
static void lcd_power_en(int on)
{
	if (on) {
		gpio_direction_output(LCD_RST_GPIO1, 0);
		udelay(1000);
		gpio_direction_output(BOOST_EN_5V, 1);
		udelay(1000);
		gpio_direction_output(LCD_RST_GPIO1, 1);
		udelay(10000);
	} else {
		gpio_direction_output(LCD_RST_GPIO1, 0);
		gpio_direction_output(BOOST_EN_5V, 0);
	}
}

static int t7_tc358765_reset(void)
{
	int gpio = 19;

	gpio_direction_output(gpio, 0);
	udelay(10000);
	gpio_direction_output(gpio, 1);
	udelay(4000);

	return 0;
}

static void t7_lcd_power_en(int on)
{
	int lcd_rst_n = 1, lcd_power_en = 108, tc3_en = 31, tc3_rst = 19;

	if (dkb_version == DKB_T7V2)
		lcd_rst_n = 49;

	if (on) {
		gpio_direction_output(lcd_rst_n, 1);
		gpio_direction_output(tc3_en, 1);
		udelay(1000); /* > 500us */

		/* AVDD ... enable */
		gpio_direction_output(lcd_power_en, 1);
		gpio_direction_output(lcd_rst_n, 0);
	} else {
		gpio_direction_output(lcd_power_en, 0);
		udelay(50 * 1000);
		gpio_direction_output(lcd_rst_n, 1);
		gpio_direction_output(tc3_en, 0);
		gpio_direction_output(tc3_rst, 0);
	}
}

static void t7_lcd_reset(void)
{
	int lcd_rst_n = 1;

	if (dkb_version == DKB_T7V2)
		lcd_rst_n = 49;

	/* second reset */
	gpio_direction_output(lcd_rst_n, 1);
	udelay(5 * 1000);
	gpio_direction_output(lcd_rst_n, 0);
	udelay(120 * 1000);
}

#define LCD_PWM_GPIO32	32
static void turn_off_backlight(void)
{
	gpio_direction_input(LCD_PWM_GPIO32);
}

static void turn_on_backlight(void)
{
	int bl_level, p_num, intensity = 40;
	if ((g_panel_id == SHARP_1080P_ID) ||
	    (g_panel_id == SHARP_QHD_ID)) {
		/* Reset backlight chip to the hightest level*/
		gpio_direction_output(LCD_PWM_GPIO32, 0);
		udelay(1000);
		gpio_direction_output(LCD_PWM_GPIO32, 1);

		/*
		 * Brightness is controlled by a series of pulses
		 * generated by gpio. It has 32 leves and level 1
		 * is the brightest. Pull low for 3ms makes
		 * backlight shutdown
		 */
		bl_level = (100 - intensity) * 32 / 100 + 1;
		p_num = bl_level-1;
		while (p_num--) {
			gpio_direction_output(LCD_PWM_GPIO32, 0);
			udelay(1);
			gpio_direction_output(LCD_PWM_GPIO32, 1);
			udelay(1);
		}
	} else
		/* Change control backlight to panel's PWM, so far, can't
		 * disconnect GPIO32 with LCD_PWM, so have to set it to
		 * input, avoid to disturb PWM from panel.
		 */
		gpio_direction_input(LCD_PWM_GPIO32);
}

void set_hx8394a_720p_info(struct mmp_disp_plat_info *fb)
{
	fb->phy_info = &helan_720p_dsi;
	fb->sclk_div = 0x40001104;
	fb->num_modes = ARRAY_SIZE(video_720p_modes);
	fb->modes = video_720p_modes;
	fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
		fb->modes->yres * 8 + 4096;

	fb->dsi_panel_config = hx8394a_init_config;
	fb->panel_manufacturer_id = HX8394A_720P_ID;
}

void set_1080p_info(struct mmp_disp_plat_info *fb)
{
	fb->phy_info = &helan_1080p_dsi;
	fb->sclk_div = 0x40001104;
	fb->num_modes = ARRAY_SIZE(video_1080p_modes);
	fb->modes = video_1080p_modes;
	fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
		fb->modes->yres * 8 + 4096;

	fb->dsi_panel_config = panel_1080p_init_config;
	fb->panel_manufacturer_id = SHARP_1080P_ID;
}

void set_qHD_info(struct mmp_disp_plat_info *fb)
{
	fb->phy_info = &helan_qhd_dsi;
	fb->sclk_div = 0x40001108;
	fb->num_modes = ARRAY_SIZE(video_qhd_modes);
	fb->modes = video_qhd_modes;
	fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
		fb->modes->yres * 8 + 4096;

	fb->dsi_panel_config = panel_qhd_init_config;
	fb->panel_manufacturer_id = SHARP_QHD_ID;
}

static void set_pwm_en(int en)
{
	unsigned long data;

	if (en) {
		data = __raw_readl(APBC_PWM3_CLK_RST) & ~(0x7 << 4);
		data |= 1 << 1;
		__raw_writel(data, APBC_PWM3_CLK_RST);

		/* delay two cycles of the solwest clock between the APB bus
		 * clock and the functional module clock. */
		udelay(10);

		/* release from reset */
		data &= ~(1 << 2);
		__raw_writel(data, APBC_PWM3_CLK_RST);

		/* enable APB bus clk */
		/* Note: pwm0/1 and pwm2/3 share apb bus clk  */
		data = __raw_readl(APBC_PWM2_CLK_RST) & ~(0x7 << 4);
		data |= 1;
		__raw_writel(data, APBC_PWM2_CLK_RST);
		udelay(10);

		/* release from reset */
		data &= ~(1 << 2);
		__raw_writel(data, APBC_PWM2_CLK_RST);

	} else {
		data = __raw_readl(APBC_PWM3_CLK_RST) & ~(0x1 << 1 | 0x7 << 4);
		__raw_writel(data, APBC_PWM3_CLK_RST);
		udelay(1000);

		data |= 1 << 2;
		__raw_writel(data, APBC_PWM3_CLK_RST);

		/* disable APB bus clk */
		/* Note: pwm0/1 and pwm2/3 share apb bus clk  */
		data &= ~(1);
		__raw_writel(data, APBC_PWM3_CLK_RST);
		udelay(1000);
	}
}


static void t7_turn_off_backlight(void)
{
	__raw_writel(0, PWM_CR3);
	__raw_writel(0, PWM_DCR);

	/* disable pwm */
	set_pwm_en(0);
}

static void t7_turn_on_backlight(void)
{
	int duty_ns = 1000000, period_ns = 2000000;
	unsigned long period_cycles, prescale, pv, dc;

	/* enable pwm */
	set_pwm_en(1);

	period_cycles = 26000;
	if (period_cycles < 1)
		period_cycles = 1;

	prescale = (period_cycles - 1) / 1024;
	pv = period_cycles / (prescale + 1) - 1;

	if (prescale > 63)
		return;

	if (duty_ns == period_ns)
		dc = (1 << 10);
	else
		dc = (pv + 1) * duty_ns / period_ns;

	__raw_writel(prescale | (1 << 6), PWM_CR3);
	__raw_writel(dc, PWM_DCR);
	__raw_writel(pv, PWM_PCR);
}

static GraphicDevice ctfb;

static void update_panel_info(struct mmp_disp_plat_info *fb)
{
	switch (g_panel_id) {
	case SHARP_1080P_ID:
		set_1080p_info(fb);
		break;
	case SHARP_QHD_ID:
		set_qHD_info(fb);
		break;
	case HX8394A_720P_ID:
		set_hx8394a_720p_info(fb);
	break;
	/*TODO: new panel(720p etc) info can be set here*/
	default:
		break;
	}
}

void *lcd_init(void)
{
	void *ret;
	u32 val;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	if (dkb_version == DKB_T7V1 || dkb_version == DKB_T7V2) {
		t7_turn_off_backlight();
		t7_lcd_power_en(1);

		fb->num_modes = ARRAY_SIZE(t7_video_modes);
		fb->modes = t7_video_modes;
		fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
			fb->modes->yres * 8 + 4096;

		memset(fb->id, 0, 16);
		strcpy(fb->id, "GFX Layer T7");
		/*
		 * T7 has 4 dsi line, pclk 312M, pdiv is 4.
		 * choose 624M clk src and post div =2 to get 312M.
		 */
		val = readl(APMU_LCD_DSI_CLK_RES_CTRL);
		val &= ~(1<<6);
		val |= (1<<9) | (0x2<<10);
		writel(val, APMU_LCD_DSI_CLK_RES_CTRL);

		fb->sclk_src = 312000000,
		fb->sclk_div = 0x40001104,

		fb->phy_type = DSI2DPI;
		fb->xcvr_reset = t7_tc358765_reset;
		fb->twsi_id = 0;

		fb->phy_info = (void *)&t7_dsiinfo;
		fb->dsi_panel_config = NULL;
	} else {
		turn_off_backlight();
		lcd_power_en(1);
		fb->dynamic_detect_panel = 1;
		fb->update_panel_info = update_panel_info;
	}


	ret = (void *)mmp_disp_init(fb);
	/*
	 * if ret==NULL, it means that panel is not the default one.
	 * we need to reset panel related info according the detected
	 * id and execute pxa168fb_init again.
	 */
	if (dkb_version == DKB_T7V1 || dkb_version == DKB_T7V2) {
		/* wait lvds signal stable, then do the second lcd reset */
		udelay(35 * 1000);
		t7_lcd_reset();
		t7_turn_on_backlight();
	} else {
		memcpy(&mmp_mipi_info_saved, fb, sizeof(struct mmp_disp_plat_info));
		turn_on_backlight();
	}
	return ret;
}

struct mmp_disp_info *fbi;
void *video_hw_init(void)
{
	struct mmp_disp_plat_info *mi;
	unsigned long t1, hsynch, vsynch;

	if (*(u32 *)CONFIG_WARM_RESET)
		return NULL;

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

	ctfb.frameAdrs = (unsigned int) fbi->fb_start;
	ctfb.plnSizeX = ctfb.winSizeX;
	ctfb.plnSizeY = ctfb.winSizeY;
	ctfb.gdfBytesPP = 4;
	ctfb.gdfIndex = GDF_32BIT_X888RGB;

	ctfb.isaBase = 0x9000000;
	ctfb.pciBase = (unsigned int) fbi->fb_start;
	ctfb.memSize = fbi->fb_size;

	/* Cursor Start Address */
	ctfb.dprBase = (unsigned int) fbi->fb_start + (ctfb.winSizeX
			* ctfb.winSizeY * ctfb.gdfBytesPP);
	if ((ctfb.dprBase & 0x0fff) != 0) {
		/* allign it */
		ctfb.dprBase &= 0xfffff000;
		ctfb.dprBase += 0x00001000;
	}
	ctfb.vprBase = (unsigned int) fbi->fb_start;
	ctfb.cprBase = (unsigned int) fbi->fb_start;

	return &ctfb;
}
static int boot_logo;
void show_logo(void)
{
	char cmd[100];
	char *cmdline;
	int wide, high, fb_size;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct lcd_videomode *fb_mode = fb->modes;
	int xres = fb_mode->xres, yres = fb_mode->yres;

	/* 4bytes for each pixel(32bpp), reserve 3 buffers */
	fb_size = ALIGN(xres, 16) * ALIGN(yres, 4) * 4 * 3;
	/* use MB aligned buffer size */
	fb_size = ALIGN(fb_size, 0x100000);
	fb_size = fb_size >> 20;

	if (dkb_version == DKB_T7V1 || dkb_version == DKB_T7V2) {
		xres = T7_FB_XRES;
		yres = T7_FB_YRES;
	}

	/* Show marvell logo */
	wide = 330;
	high = 236;
	sprintf(cmd, "bmp display %p %d %d",
		MARVELL_PXA988, (xres - wide) / 2,
		(yres - high) / 2);
	run_command(cmd, 0);

	flush_cache(DEFAULT_FB_BASE, DEFAULT_FB_SIZE);
	boot_logo = 1;
	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	remove_cmdline_param(cmdline, "androidboot.lcd=");
	remove_cmdline_param(cmdline, "fhd_lcd=");

	if (xres == HELAN_1080P_FB_XRES) {
		sprintf(cmdline + strlen(cmdline),
			" fhd_lcd=1 androidboot.lcd=1080p");
	} else {
		sprintf(cmdline + strlen(cmdline),
			" androidboot.lcd=720p");
	}
	remove_cmdline_param(cmdline, "fbmem=");
	sprintf(cmdline + strlen(cmdline), " fbmem=%dM@0x%x",
		fb_size, DEFAULT_FB_BASE);
	setenv("bootargs", cmdline);
	free(cmdline);
}

void lcd_blank(void)
{
	memset((unsigned short *)DEFAULT_FB_BASE, 0x0, ctfb.memSize);
}

void show_ramdump_logo(int type)
{
	char cmd[100];
	int wide, high;
	const unsigned char *logo;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct lcd_videomode *fb_mode = fb->modes;

	if (type == RAMDUMP_USB_DUMP)
		logo = RAMDUMP_USB_LOGO;
	else if (type == RAMDUMP_SD_DUMP)
		logo = RAMDUMP_SD_LOGO;
	else
		return;

	/* Show RAMDUMP dump logo */
	wide = 446;
	high = 148;
	sprintf(cmd, "bmp display %p %d %d", logo,
		(fb_mode->xres - wide) / 2, (fb_mode->yres - high) / 2);
	run_command(cmd, 0);

	flush_cache(DEFAULT_FB_BASE, DEFAULT_FB_SIZE);
}
#else
void show_ramdump_logo(int type)
{
}
#endif

#ifdef CONFIG_OF_LIBFDT
void handle_dtb(struct fdt_header *devtree)
{
	char cmd[128];
#if defined(CONFIG_MMP_DISP)
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct lcd_videomode *fb_mode = fb->modes;
	int xres = fb_mode->xres;
#endif

	/* set dtb addr */
	sprintf(cmd, "fdt addr 0x%x", (unsigned int)devtree);
	run_command(cmd, 0);

#if defined(CONFIG_MMP_DISP)
	/* detele unused touch device node */
	if (touch_type == SYNAPT_TP) {
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/ft5306@39", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/gt913@720p", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/gt913@1080p", 0);
		if (xres == HELAN_1080P_FB_XRES)
			run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/s3202@720p", 0);
		else
			run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/s3202@1080p", 0);
	} else if (touch_type == GOODIX_TP) {
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/ft5306@39", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/s3202@720p", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/s3202@1080p", 0);
		if (xres == HELAN_1080P_FB_XRES)
			run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/gt913@720p", 0);
		else
			run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/gt913@1080p", 0);
	} else {
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/s3202@720p", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/s3202@1080p", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/gt913@720p", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4037000/gt913@1080p", 0);
	}

	if (fb->panel_manufacturer_id == SHARP_1080P_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /hx8394", 0);
		run_command("fdt rm /nt35565", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <4>", 0);
	} else if (fb->panel_manufacturer_id == HX8394A_720P_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt rm /nt35565", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <4>", 0);
	} else if (fb->panel_manufacturer_id == SHARP_QHD_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /hx8394", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <2>", 0);
	} else {
		run_command("fdt rm /hx8394", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt rm /nt35565", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <4>", 0);
	}
#endif
}
#endif

#ifdef CONFIG_MISC_INIT_R
static unsigned char helandkb_recovery_reg_read(void)
{
	u32 value = 0;
	struct pmic *pmic_base;

	/* Get the magic number from RTC register */
	pmic_base = pmic_get(MARVELL_PMIC_BASE);
	if (!pmic_base || pmic_probe(pmic_base)
	    || pmic_reg_read(pmic_base, 0xef, &value)) {
		printf("read rtc register error\n");
		value = 0;
	}

	return (unsigned char)value;
}

static unsigned char helandkb_recovery_reg_write(unsigned char value)
{
	struct pmic *pmic_base;

	pmic_base = pmic_get(MARVELL_PMIC_BASE);
	if (!pmic_base || pmic_probe(pmic_base)
	    || pmic_reg_write(pmic_base, 0xef, value))
		printf("write rtc register error\n");

	return 0;
}

static struct recovery_reg_funcs helandkb_recovery_reg_funcs = {
	.recovery_reg_read = helandkb_recovery_reg_read,
	.recovery_reg_write = helandkb_recovery_reg_write,
};
int misc_init_r(void)
{
	char *cmdline, *p1;
	int i, ddr_size = 0, cpu;
	u32 base, size;
	u32 max_preferred_freq = 0, max_freq = 0, maxfreqflg = 0;
#ifdef CONFIG_RAMDUMP
	char *s;
	int ramdump;
#endif

	if (pxa_is_warm_reset())
		setenv("bootdelay", "-1");

#if defined(CONFIG_MMP_DISP)
	if (!fastboot)
		show_logo();
#endif

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	remove_cmdline_param(cmdline, "ddr_mode=");
	sprintf(cmdline + strlen(cmdline), " ddr_mode=%d", ddr_mode);

	/* set the cma range, we can change it according to
	 * system configuration */
	remove_cmdline_param(cmdline, "cma=");
	remove_cmdline_param(cmdline, "cgourp_disable=");
	remove_cmdline_param(cmdline, "androidboot.low_ram=");
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (gd->bd->bi_dram[i].size == 0)
			break;
		ddr_size += gd->bd->bi_dram[i].size;
	}

	/* ION reserved here for uboot running for EMMD protection */
	if (ddr_size > SZ_512M + SZ_256M) {
		sprintf(cmdline + strlen(cmdline), " cma=112M");
		/*
		 * cgroup will only enabled for low_ram case,
		 * for high ram case disable cgroup function
		 */
		sprintf(cmdline + strlen(cmdline), " cgroup_disable=memory");
	} else
		sprintf(cmdline + strlen(cmdline), " cma=64M androidboot.low_ram=true");

	remove_cmdline_param(cmdline, "ioncarv=");
	base = CONFIG_SYS_TEXT_BASE;
	size = SZ_16M;
	if (((base + size) > ddr_size))
		printf("ERROR: wrong ION setting!!");

	sprintf(cmdline + strlen(cmdline), " ioncarv=%dM@0x%08x", size >> 20, base);

#ifdef CONFIG_RAMDUMP
	rd_mems[0].start = CONFIG_TZ_HYPERVISOR_SIZE;
	rd_mems[0].end = base;
	rd_mems[1].start = base + size;
	rd_mems[1].end = ddr_size;
#endif

	/* if there's new maxfreq request, add it to bootargs */
	max_preferred_freq = get_sku_max_freq();
	if (max_preferred_freq) {
		if (max_preferred_freq <= 1183)
			max_preferred_freq = 1183;
		else
			max_preferred_freq = 1248;
		remove_cmdline_param(cmdline, "max_freq=");
		sprintf(cmdline + strlen(cmdline),
			" max_freq=%u", max_preferred_freq);
		max_freq = max_preferred_freq;
		maxfreqflg = 1;
	} else {
		/* check whether there is sku setting for max_freq previously */
		p1 = strstr(cmdline, "max_freq=");
		if (p1) {
			p1 = strchr(p1, '=');
			if (p1) {
				p1++;
				max_freq = simple_strtoul(p1, NULL, 10);
			}
			if (max_freq)
				maxfreqflg = 1;
		}
	}

	setenv("bootargs", cmdline);
	/* if there's new maxfreq request, save env to later boots */
	if (max_preferred_freq)
		saveenv();

	free(cmdline);

	p_recovery_reg_funcs = &helandkb_recovery_reg_funcs;

	/* set op according to setting by selected, unless by default */
	if (maxfreqflg) {
		set_plat_max_corefreq(max_freq);
		printf("max_freq is selected as %u\n", max_freq);
	}
	pxa988_fc_init(ddr_mode);
	if (pwr_chg_enable)
		setop(1);
	else
		setop(3);

#if defined(CONFIG_MMP_DISP)
	calculate_dsi_clk(&mmp_mipi_info_saved);
#endif
	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		set_idle_count(cpu, 0);

	helandkb_touch_detect();

	/*
	 * bus 2 is used by pmic, set here for debug with
	 * "i2c probe", this should be the called just before exit,
	 * in case the default bus number is changed
	 */
	i2c_set_bus_num(2);

	/* for fastboot */
	setenv("fbenv", "mmc2");

#ifdef CONFIG_RAMDUMP
	if (fastboot) {
		s = getenv("ramdump");
		if (!s) {
			setenv("ramdump", "0");
			s = getenv("ramdump");
		}
		ramdump = (int)simple_strtol(s, NULL, 10);
		if (!ramdump && (0x1 != emmd_page->dump_style)) {
#ifdef CONFIG_RAMDUMP_TFFS
			show_ramdump_logo(RAMDUMP_SD_DUMP);
			if (run_command("ramdump 0 0 0 2", 0)) {
				printf("SD dump fail, enter fastboot mode\n");
				show_ramdump_logo(RAMDUMP_USB_DUMP);
				run_command("fb", 0);
			} else {
				run_command("reset", 0);
			}
#endif
		} else {
			printf("ready to enter fastboot mode\n");
			show_ramdump_logo(RAMDUMP_USB_DUMP);
			run_command("fb", 0);
		}
	}
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

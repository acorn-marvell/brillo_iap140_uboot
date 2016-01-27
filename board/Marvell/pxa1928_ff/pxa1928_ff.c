/*
e* (C) Copyright 2011
 * Marvell Semiconductors Ltd. <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa1928.h>
#ifdef CONFIG_SDHCI
#include <sdhci.h>
#endif
#include <mvmfp.h>
#include <mv_recovery.h>
#include <pxa_amp.h>
#include <asm/arch/mfp.h>
#include <asm/gpio.h>
#include <malloc.h>
#include <eeprom_34xx02.h>
#include <emmd_rsv.h>
#include <power/pmic.h>
#include <power/88pm830.h>
#include <power/marvell88pm_pmic.h>
#include <power/pxa1928_freq.h>
#include <asm/gpio.h>
#include <asm/armv8/adtfile.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#include "../common/cmdline.h"
#if defined(CONFIG_MMP_DISP)
#include <stdio_dev.h>
#include <mmp_disp.h>
#include <leds-lm3532.h>
#include <leds-88pm828x.h>
#include <video_fb.h>
#include "../common/panel.h"
#include "../common/marvell.h"
#include "../common/logo_pxa988.h"
#include "../common/logo_debugmode.h"
#endif
#include "../common/obm2osl.h"
#include "../common/mrd_flag.h"
#include "../common/mv_cp.h"

#include "../common/charge.h"

static struct pmic_chip_desc *board_pmic_chip;
DECLARE_GLOBAL_DATA_PTR;

#define MACH_TYPE_PXA1928		3897
#define DVC_CONTROL_REG	0x4F
/*two dvc control register bits can support 4 control level
4(control level) * 4 (DVC level registers) can support 16level DVC*/
#define DVC_SET_ADDR1	(1 << 0)
#define DVC_SET_ADDR2	(1 << 1)
#define DVC_CTRl_LVL	4

#define PMIC_I2C_BUS 0
#define CHG_I2C_BUS 0
#define FG_I2C_BUS 0

#define RTC_CLK                 (0xD4015000)
#define RTC_BRN0                (0xD4010014)
#define FB_MAGIC_NUM          ('f'<<24 | 'a'<<16 | 's'<<8 | 't')
#define PD_MAGIC_NUM          ('p'<<24 | 'r'<<16 | 'o'<<8 | 'd')

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
	.rdc_addr = EMMD_INDICATOR + EMMD_RDC_OFFSET,
	.phys_start = CONFIG_TZ_HYPERVISOR_SIZE,
	.mems = &rd_mems[0],
	.nmems = sizeof(rd_mems)/sizeof(rd_mems[0]),
};
#endif
static int fastboot;

#define PXA1928_FORM_FACTOR_SHARP 0x9
#define PXA1928_FORM_FACTOR_TRULY 0xA

#ifdef CONFIG_REVISION_TAG
static u8 board_type, board_rev;
static u8 board_eco[16];
static uchar board_info[64];
static uchar board_sn[33];
static u32 ddr_speed;
u32 get_board_rev(void)
{
	return (u32)board_rev;
}
u32 get_board_type(void)
{
	return (u32)board_type;
}
u32 prox_sensor_is_off(void)
{
	#define PROXIMITY_ECN_INDEX	(0)
	#define PROXIMITY_DISABLE_VAL	(0)
	return (u32) (board_eco[PROXIMITY_ECN_INDEX] == PROXIMITY_DISABLE_VAL);
}
#endif

unsigned int mv_profile = 0xFF;
/* Define CPU/DDR default max frequency
   CPU: 1300MHz
   DDR: 528MHz
   GC3D: 624MHz
   GC2D: 312MHz
*/
#define CPU_MAX_FREQ_DEFAULT	1300
#define DDR_MAX_FREQ_DEFAULT	528
#define GC3D_MAX_FREQ_DEFAULT	624
#define GC2D_MAX_FREQ_DEFAULT	312
/* Define CPU/DDR max frequency
   CPU: 1508MHz
   GC3D: 797MHz
   GC2D: 416MHz
*/
#define CPU_MAX_FREQ		1508
#define DDR_MAX_FREQ		667
#define DDR_MAX_FREQ_POP	800
#define GC3D_MAX_FREQ		797
#define GC2D_MAX_FREQ		416

#ifdef CONFIG_CHECK_DISCRETE
static void unlock_aib_regs(void)
{
	struct pxa1928apbc_registers *apbc =
		(struct pxa1928apbc_registers *)PXA1928_APBC_BASE;

	writel(FIRST_ACCESS_KEY, &apbc->asfar);
	writel(SECOND_ACCESS_KEY, &apbc->assar);
}
#endif

/*
 * when boot kernel: UBOOT take Volume Up key (GPIO15/GPIO160) for recovery magic key
 * when power up: OBM take Volume Up key (GPIO15/GPIO160) for swdownloader
 */
static unsigned __attribute__((unused)) recovery_key = 15;
/* Take Volume Down key (GPIO17 for pop board/GPIO162 for discrete board) for fastboot */
static int fastboot_key = 17;
static int pxa1928_discrete = 0xff;
static int check_discrete(void)
{
#ifdef CONFIG_CHECK_DISCRETE
	struct pxa1928aib_registers *aib =
		(struct pxa1928aib_registers *)PXA1928_AIB_BASE;

	/* unlock AIB registers */
	unlock_aib_regs();

	/* get discrete or pop info */
	pxa1928_discrete = (readl(&aib->nand) & (1 << 4)) ? 1 : 0;
#endif
	printf("PKG:   %s\n", (pxa1928_discrete == 0xff) ?
		"Discrete/PoP not checked on Zx chips" :
		(pxa1928_discrete ? "Discrete" : "PoP"));
	return 0;
}

static int chip_type = PXA1926_2L_DISCRETE;
static void check_chip_type(void)
{
	if (cpu_is_pxa1928_a0()) {
		if (1 == pxa1928_discrete)
			printf("Chip is pxa1928 A0 Discrete\n");
		else
			printf("Chip is pxa1928 A0 POP\n");
		return;
	}
	/* for pxa1928 B0 */
	chip_type = readl(PXA_CHIP_TYPE_REG);
	switch (chip_type) {
	case PXA1926_2L_DISCRETE:
		printf("Chip is PXA1926 B0 2L Discrete\n");
		break;

	case PXA1928_POP:
		printf("Chip is PXA1928 B0 PoP\n");
		break;

	case PXA1928_4L:
		printf("Chip is PXA1928 B0 4L\n");
		break;

	case PXA1928_2L:
		printf("Chip is PXA1928 B0 2L\n");
		break;

	default:
		chip_type = PXA1926_2L_DISCRETE;
		printf("Unknow chip type, set to PXA1926 B0 2L Discrete as default\n");
		break;
	}
	return;
}

#if defined(CONFIG_MMP_DISP)
#define LCD_RST_N		121 /* FIXME */
#define BOOST_5V_EN_BOARD_REV1	125
#define BOOST_5V_EN_BOARD_REV2	10
#define LCD_BACKLIGHT_EN	6
#define BOOST_5V_EN_BOARD_DIS	155
#define LCD_BACKLIGHT_EN_DIS	151
#define BL_KDT3102		0xB3

#define PWM2_BASE		0xD401A400
#define APB_CLK_REG_BASE	0xD4015000
#define APBC_PWM1_CLK_RST	(APB_CLK_REG_BASE + 0x3C)
#define APBC_PWM2_CLK_RST	(APB_CLK_REG_BASE + 0x40)
#define APBC_FNCLKSEL_MASK	(7 << 4)
#define PWM_CR2		(PWM2_BASE + 0x00)
#define PWM_DCR		(PWM2_BASE + 0x04)
#define PWM_PCR		(PWM2_BASE + 0x08)
#define PWMCR_SD	(1 << 6)
#define PWMDCR_FD	(1 << 10)

#define PXA1928_FB_XRES		720
#define PXA1928_FB_YRES		1280

#define PXA1928_1080P_FB_XRES		1080
#define PXA1928_1080P_FB_YRES		1920

static u8 bl_chip_id;

static struct dsi_info concord_dsi = {
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
		.xres = PXA1928_FB_XRES,
		.yres = PXA1928_FB_YRES,
		/* FIXME: timing parameter need to confirm on real board */
		.hsync_len = 2,
		.left_margin = 20,
		.right_margin = 138,
		.vsync_len = 2,
		.upper_margin = 10,
		.lower_margin = 20,
		.sync = FB_SYNC_VERT_HIGH_ACT \
				  | FB_SYNC_HOR_HIGH_ACT,
	},
};

#if 0
static struct lcd_videomode lg_720p_modes[] = {
	[0] = {
		.refresh = 60,
		.xres = PXA1928_FB_XRES,
		.yres = PXA1928_FB_YRES,
		.hsync_len = 2,
		.left_margin = 40,   /* left_margin should >=20 */
		.right_margin = 150, /* right_margin should >=100 */
		.vsync_len = 2,
		.upper_margin = 8,  /* upper_margin should >= 4 */
		.lower_margin = 10,  /* lower_margin should >= 4 */
		.sync = FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
};
#endif

static struct lcd_videomode video_1080p_modes[] = {
	[0] = {
		/* we should align the follwoing settings with kernel driver */
		.pixclock = 19230, /* 1/52MHz */
		.refresh = 28,
		.xres = PXA1928_1080P_FB_XRES,
		.yres = PXA1928_1080P_FB_YRES,
		.hsync_len = 2,
		.left_margin = 40,
		.right_margin = 132,
		.vsync_len = 2,
		.upper_margin = 2,
		.lower_margin = 30,
		.sync = FB_SYNC_VERT_HIGH_ACT \
				  | FB_SYNC_HOR_HIGH_ACT,
	},
};

static void update_panel_info(struct mmp_disp_plat_info *fb);
static void lcd_calculate_sclk(struct mmp_disp_info *fb,
		int dsiclk, int pathclk);
static struct mmp_disp_plat_info mmp_mipi_lcd_info = {
	.index = 0,
	.id = "GFX Layer",
	/* FIXME */
	.sclk_src = 416000000,
	.sclk_div = 0x20001106,
	.num_modes = ARRAY_SIZE(video_1080p_modes),
	.modes = video_1080p_modes,
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
	.phy_info = &concord_dsi,
	.dsi_panel_config = panel_1080p_init_config,
	.update_panel_info = update_panel_info,
	.calculate_sclk = lcd_calculate_sclk,
	.dynamic_detect_panel = 1,
	/* We use sharp 1080p panel as default panel */
	.panel_manufacturer_id = SHARP_1080P_ID,
};

static void lcd_power_en(int on)
{
	unsigned boost_5v_en = BOOST_5V_EN_BOARD_REV1;
#ifdef CONFIG_REVISION_TAG
	if (board_rev == 2)
		boost_5v_en = BOOST_5V_EN_BOARD_REV2;
#endif
	if (1 == pxa1928_discrete)
		boost_5v_en = BOOST_5V_EN_BOARD_DIS;
	if (on) {
		gpio_direction_output(LCD_RST_N, 0);
		udelay(1000);
		gpio_direction_output(boost_5v_en, 1);
		udelay(1000);
		gpio_direction_output(LCD_RST_N, 1);
		udelay(10000);
	} else {
		gpio_direction_output(LCD_RST_N, 0);
		gpio_direction_output(boost_5v_en, 0);
	}
}

static void set_pwm_en(int en)
{
	unsigned long data;

	/*
	 * A dependence exists between pwm1 and pwm2.pwm1 can controll its
	 * apb bus clk independently, while pwm2 apb bus clk is controlled
	 * by pwm1's. The same relationship exists between pwm3 and pwm4.
	 */
	if (en) {
		__raw_writel(APBC_APBCLK, APBC_PWM1_CLK_RST);
		__raw_writel(APBC_FNCLK, APBC_PWM2_CLK_RST);
	} else {
		data = __raw_readl(APBC_PWM2_CLK_RST) &
			~(APBC_FNCLK | APBC_FNCLKSEL_MASK);
		__raw_writel(data, APBC_PWM2_CLK_RST);
		udelay(10);

		data &= ~APBC_APBCLK;
		__raw_writel(data, APBC_PWM2_CLK_RST);
		udelay(10);

		data |= APBC_RST;
		__raw_writel(data, APBC_PWM2_CLK_RST);

		data = __raw_readl(APBC_PWM1_CLK_RST) &
			~(APBC_FNCLK | APBC_FNCLKSEL_MASK);
		__raw_writel(data, APBC_PWM1_CLK_RST);
		udelay(10);

		data &= ~APBC_APBCLK;
		__raw_writel(data, APBC_PWM1_CLK_RST);
		udelay(10);

		data |= APBC_RST;
		__raw_writel(data, APBC_PWM1_CLK_RST);

	}
}

static void turn_off_backlight(void)
{
	if (1 == pxa1928_discrete)
		gpio_direction_output(LCD_BACKLIGHT_EN_DIS, 0);
	else
		gpio_direction_output(LCD_BACKLIGHT_EN, 0);

	if (bl_chip_id == BL_KDT3102)
		return;

	__raw_writel(0, PWM_CR2);
	__raw_writel(0, PWM_DCR);

	/* disable pwm */
	set_pwm_en(0);
}

struct lm3532_data lm3532_pdata = {
	.addr = LM3532_I2C_ADDR,
	.i2c_bus_num = LM3532_I2C_NUM,
	.ramp_time = 0, /* Ramp time in milliseconds */
	.ctrl_a_fs_current = LM3532_20p2mA_FS_CURRENT,
	.ctrl_a_mapping_mode = LM3532_LINEAR_MAPPING,
	.ctrl_a_pwm = 0x6,
	.output_cfg_val = 0x0,
	.feedback_en_val = 0x3,
};

struct pm828x_data pm828x_pdata = {
       .addr = PM828X_I2C_ADDR,
       .ramp_mode = PM828X_RAMP_MODE_NON_LINEAR,
       .idac_current = PM828X_IDAC_CURRENT_20MA,
       .ramp_clk = PM828X_RAMP_CLK_8K,
       .str_config = PM828X_SINGLE_STR_CONFIG,
       .i2c_num = 4,
};

static void turn_on_backlight(void)
{
	int duty_ns = 50000, period_ns = 100000;
	unsigned long period_cycles, prescale, pv, dc;

	if (1 == pxa1928_discrete)
		gpio_direction_output(LCD_BACKLIGHT_EN_DIS, 1);
	else
		gpio_direction_output(LCD_BACKLIGHT_EN, 1);

	pm828x_read_id(&pm828x_pdata, &bl_chip_id);
	if (bl_chip_id == PM828X_ID)
		pm828x_init(&pm828x_pdata);
	else
		lm3532_init(&lm3532_pdata);

	/* enable pwm */
	set_pwm_en(1);

	period_cycles = 2600;
	if (period_cycles < 1)
		period_cycles = 1;

	prescale = (period_cycles - 1) / 1024;
	pv = period_cycles / (prescale + 1) - 1;

	if (prescale > 63)
		return;

	if (duty_ns == period_ns)
		dc = PWMDCR_FD;
	else
		dc = (pv + 1) * duty_ns / period_ns;

	__raw_writel(prescale | PWMCR_SD, PWM_CR2);
	__raw_writel(dc, PWM_DCR);
	__raw_writel(pv, PWM_PCR);
}

void set_720p_info(struct mmp_disp_plat_info *fb)
{
	fb->num_modes = ARRAY_SIZE(video_720p_modes);
	fb->modes = video_720p_modes;
	fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
		fb->modes->yres * 8 + 4096;

	fb->dsi_panel_config = panel_720p_init_config;
	fb->panel_manufacturer_id = TRULY_720P_ID;
}

#if 0
void set_lg_720p_info(struct mmp_disp_plat_info *fb)
{
	fb->num_modes = ARRAY_SIZE(lg_720p_modes);
	fb->modes = lg_720p_modes;
	fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
		fb->modes->yres * 8 + 4096;

	fb->dsi_panel_config = lg_720p_init_config;
}
#endif

static void set_1080p_info(struct mmp_disp_plat_info *fb)
{
	fb->num_modes = ARRAY_SIZE(video_1080p_modes);
	fb->modes = video_1080p_modes;
	fb->max_fb_size = ALIGN(fb->modes->xres, 16) *
		fb->modes->yres * 8 + 4096;

	if (get_board_type() == PXA1928_FORM_FACTOR_TRULY) {
		fb->dsi_panel_config = panel_1080p_tft_init_config;
		fb->panel_manufacturer_id = SHARP_QHD_ID;
	} else if (get_board_type() == PXA1928_FORM_FACTOR_SHARP) {
		fb->dsi_panel_config = panel_1080p_init_config;
		fb->panel_manufacturer_id = SHARP_1080P_ID;
	} else {
		fb->dsi_panel_config = panel_1080p_init_config;
		fb->panel_manufacturer_id = SHARP_1080P_ID;
	}
}

static GraphicDevice ctfb;
static enum panel_res_type {
	PANEL_720P = 0,
	PANEL_1080P,
} panel_res;

static void update_panel_info(struct mmp_disp_plat_info *fb)
{
	/* Set only 1080p option:
	* 1. This is the only supported screen for the FF
	* 2. for testing cases when no screen attached the kernel display driver
	*	sets 1080p buffer size. setting buffer as 1080p prevents memory over write
	*/
	set_1080p_info(fb);
}

#define LCD_PN_SCLK			(0xd420b1a8)
#define PIXEL_SRC_DISP1			(0x1 << 29)
#define PIXEL_SRC_DSI1_PLL		(0x3 << 29)
#define PIXEL_SRC_SEL_MASK		(0x7 << 29)
#define DSI1_BIT_CLK_SRC_DISP1		(0x1 << 12)
#define DSI1_BIT_CLK_SRC_DSI1_PLL	(0x3 << 12)
#define DSI1_BIT_CLK_SRC_SEL_MASK	(0x3 << 12)
#define DSI1_BITCLK_DIV(div)		((div) << 8)
#define DSI1_BITCLK_DIV_MASK		((0xf) << 8)
#define PIXEL_CLK_DIV(div)		(div)
#define PIXEL_CLK_DIV_MASK		(0xff)

#define APMU_DISP_CLKCTRL		(0xd4282984)
#define DISP1_CLK_SRC_SEL_PLL1624	(0 << 12)
#define DISP1_CLK_SRC_SEL_PLL1416	(1 << 12)
#define DISP1_CLK_SRC_SEL_MASK		(7 << 12)
#define DISP1_CLK_DIV(div)		((div) << 8)
#define DISP1_CLK_DIV_MASK		(7 << 8)

#define MHZ_TO_HZ	(1000000)
#define PLL1_416M	(416000000)
#define PLL1_624M	(624000000)
static int parent_rate_fixed_num = 2;
static int parent_rate_fixed_tbl[2] = {
	416,
	624,
};

static void lcd_calculate_sclk(struct mmp_disp_info *fb,
		int dsiclk, int pathclk)
{
	u32 sclk, apmu, bclk_div = 1, pclk_div;
	int sclk_src, parent_rate, div_tmp;
	int offset, offset_min = dsiclk, i;

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

	if (offset_min > 20) {
		/* out of fixed parent rate +-20M */
		fb->mi->sclk_src = dsiclk;
		bclk_div = 1;
	} else {
		fb->mi->sclk_src = sclk_src;
	}

	sclk = readl(LCD_PN_SCLK);
	sclk &= ~(PIXEL_SRC_SEL_MASK | DSI1_BIT_CLK_SRC_SEL_MASK |
			DSI1_BITCLK_DIV_MASK | PIXEL_CLK_DIV_MASK);
	pclk_div = (fb->mi->sclk_src + pathclk / 2) / pathclk;

	sclk = DSI1_BITCLK_DIV(bclk_div) | PIXEL_CLK_DIV(pclk_div);

	fb->mi->sclk_src *= MHZ_TO_HZ;
	apmu = readl(APMU_DISP_CLKCTRL);
	apmu &= ~(DISP1_CLK_SRC_SEL_MASK | DISP1_CLK_DIV_MASK);
	apmu |= DISP1_CLK_DIV(1);
	if (fb->mi->sclk_src == PLL1_416M) {
		apmu |= DISP1_CLK_SRC_SEL_PLL1416;
		sclk |= PIXEL_SRC_DISP1 | DSI1_BIT_CLK_SRC_DISP1;
	} else if (fb->mi->sclk_src == PLL1_624M) {
		apmu |= DISP1_CLK_SRC_SEL_PLL1624;
		sclk |= PIXEL_SRC_DISP1 | DSI1_BIT_CLK_SRC_DISP1;
	} else {
		pxa1928_pll3_set_rate(fb->mi->sclk_src);
		sclk |= PIXEL_SRC_DSI1_PLL | DSI1_BIT_CLK_SRC_DSI1_PLL;
	}

	writel(apmu, APMU_DISP_CLKCTRL);
	fb->mi->sclk_div = sclk;
}

void *lcd_init(void)
{
	void *ret;
	char *cmdline;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;

	turn_off_backlight();
	lcd_power_en(1);

	ret = (void *)mmp_disp_init(fb);
	if (ret) {
		/* update panel info */
		cmdline = malloc(COMMAND_LINE_SIZE);
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		remove_cmdline_param(cmdline, "androidboot.lcd=");
		if (g_panel_id == SHARP_1080P_ID ||
				g_panel_id == SHARP_QHD_ID) {
			sprintf(cmdline + strlen(cmdline),
				" androidboot.lcd=%s", "1080_50");
			panel_res = PANEL_1080P;
		} else {
			sprintf(cmdline + strlen(cmdline),
				" androidboot.lcd=%s", "720_45");
			panel_res = PANEL_720P;
		}
		setenv("bootargs", cmdline);
		free(cmdline);
	}

	turn_on_backlight();
	return ret;
}

struct mmp_disp_info *fbi;
void *video_hw_init(void)
{
	struct mmp_disp_plat_info *mi;
	unsigned long t1, hsynch, vsynch;

	if (pxa_is_warm_reset())
		return NULL;

	fbi = lcd_init();
	if (fbi == NULL)
		return NULL;

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
	ctfb.dprBase = (unsigned int) (uintptr_t)fbi->fb_start + (ctfb.winSizeX \
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

static void show_logo(void)
{
	char cmd[100];
	char *cmdline;
	int wide, high;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct lcd_videomode *fb_mode = fb->modes;
	int xres = fb_mode->xres, yres = fb_mode->yres;

	/* Show marvell logo */
	wide = 330;
	high = 236;
	sprintf(cmd, "bmp display %p %d %d", MARVELL_PXA988, \
		(xres - wide) / 2, (yres - high) / 2);
	run_command(cmd, 0);
	flush_cache(g_disp_start_addr, g_disp_buf_size);

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	setenv("bootargs", cmdline);
	free(cmdline);
}

#else	/* end of CONFIG_MMP_DISP */
static void show_logo(void) {}
#endif

int board_early_init_f(void)
{
#ifdef CONFIG_CMD_MFP
	u32 mfp_cfg[] = {
		/* UART3 */
		UART3_RXD_MMC2_DAT7_MFP33,
		UART3_TXD_MMC2_DAT6_MFP34,
		/* TWSI */
		PWR_SCL_MFP67,
		PWR_SDA_MFP68,
		TWSI6_SCL_MMC2_DAT5_MFP35,
		TWSI6_SDA_MMC2_DAT4_MFP36,
		/* eMMC */
		MMC3_DAT0_ND_IO8_MFP87,
		MMC3_DAT1_ND_IO9_MFP86,
		MMC3_DAT2_ND_IO10_MFP85,
		MMC3_DAT3_ND_IO11_MFP84,
		MMC3_DAT4_ND_IO12_MFP83,
		MMC3_DAT5_ND_IO13_MFP82,
		MMC3_DAT6_ND_IO14_MFP81,
		MMC3_DAT7_ND_IO15_MFP80,
		MMC3_CLK_SM_ADVMUX_MFP88,
		MMC3_CMD_SM_RDY_MFP89,
		MMC3_RST_ND_CLE_MFP90,
		/* SD */
		MMC1_DAT0_MFP62,
		MMC1_DAT1_MFP61,
		MMC1_DAT2_MFP60,
		MMC1_DAT3_MFP59,
		MMC1_DAT4_MFP58,
		MMC1_DAT5_MFP57,
		MMC1_DAT6_MFP56,
		MMC1_DAT7_MFP55,
		MMC1_CLK_MFP64,
		MMC1_CMD_MFP63,
		MMC1_CD_N_MFP65,
		MMC1_WP_MFP66,
		/* DVC pin */
		DVC_PIN0_MFP107,
		DVC_PIN1_MFP108,
		DVC_PIN2_MFP99,
		DVC_PIN3_MFP103,
		/* Main Camera */
		GPIO12_MFP12,
		GPIO113_MFP113,
		/* Secondary Camera */
		GPIO13_MFP13,
		GPIO111_MFP111,
		/* GPS */
		GPIO53_MFP53,
		/*End of configureation*/
		MFP_EOC
	};
	u32 mfp_cfg_pop[] = {
		/* TWSI */
		TWSI2_SCL_MFP43,
		TWSI2_SDA_MFP44,
		TWSI3_SCL_MFP18,
		TWSI3_SDA_MFP19,
		TWSI4_SCL_MFP46,
		TWSI4_SDA_MFP45,
		TWSI5_SCL_MFP29,
		TWSI5_SDA_MFP30,
		/* Back light PWM2 */
		BACKLIGHT_PWM2_MFP51,
		BOOST_5V_EN_MFP10,
		LCD_RESET_MFP121,
		LCD_BACKLIGHT_EN_MFP6,
		/* Volume Down key for fastboot */
		GPIO17_MFP17,
		/* Volume Up key for recovery */
		GPIO15_MFP15,
		/*End of configureation*/
		MFP_EOC
	};
	u32 mfp_cfg_discrete[] = {
		/*GPIO 00:54*/
		GPIO0_MFP0,
		GPIO1_MFP1,
		GPIO2_MFP2,
		GPIO3_MFP3,
		GPIO4_MFP4,
		GPIO5_MFP5,
		GPIO6_MFP6,
		GPIO7_MFP7,
		GPIO8_MFP8,
		GPIO9_MFP9,
		GPIO10_MFP10,
		GPIO11_MFP11,
		GPIO14_MFP14,
		GPIO15_MFP15,
		GPIO16_MFP16,
		GPIO17_MFP17,
		GPIO18_MFP18,
		GPIO19_MFP19,
		GPIO20_MFP20,
		GPIO21_MFP21,
		GPIO22_MFP22,
		GPIO23_MFP23,
		GPIO24_MFP24,
		GPIO25_MFP25,
		GPIO26_MFP26,
		GPIO27_MFP27,
		GPIO28_MFP28,
		GPIO29_MFP29,
		GPIO30_MFP30,
		GPIO31_MFP31,
		GPIO32_MFP32,
		GPIO43_MFP43,
		GPIO44_MFP44,
		GPIO45_MFP45,
		GPIO46_MFP46,
		GPIO47_MFP47,
		GPIO48_MFP48,
		GPIO49_MFP49,
		GPIO50_MFP50,
		GPIO51_MFP51,
		GPIO52_MFP52,
		GPIO54_MFP54,
		GPIO136_MFP136,
		GPIO137_MFP137,
		GPIO138_MFP138,
		GPIO140_MFP140,
		GPIO141_MFP141,
		GPIO142_MFP142,
		GPIO143_MFP143,
		GPIO144_MFP144,
		/* USIM */
		IC_USB_P_DIS_MFP193,
		IC_USB_N_DIS_MFP194,
		USIM1_UCLK_DIS_MFP190,
		USIM1_UIO_DIS_MFP191,
		USIM1_NURST_DIS_MFP192,
		USIM2_UCLK_DIS_MFP195,
		USIM2_UIO_DIS_MFP196,
		USIM2_NURST_DIS_MFP197,
		/* MMC1_CD */
		MMC1_CD_ND_NCS1_MFP100,
		/* GPIO */
		GPIO65_MFP65,
		/* TWSI */
		TWSI2_SCL_MFP178,
		TWSI2_SDA_MFP179,
		TWSI3_SCL_MFP176,
		TWSI3_SDA_MFP177,
		TWSI4_SCL_MFP181,
		TWSI4_SDA_MFP180,
		TWSI5_SCL_MFP174,
		TWSI5_SDA_MFP175,
		/* Back light PWM2 */
		BACKLIGHT_PWM2_MFP186,
		BOOST_5V_EN_MFP155,
		LCD_RESET_MFP121,
		LCD_BACKLIGHT_EN_MFP151,
		/* Volume Down key for fastboot */
		GPIO162_MFP162,
		/* Volume Up key for recovery */
		GPIO160_MFP160,
		MFP_EOC
	};

	mfp_config(mfp_cfg);
	check_discrete();
	if (1 == pxa1928_discrete) {
		recovery_key = 160;
		fastboot_key = 162;
		mfp_config(mfp_cfg_discrete);
	} else {
		mfp_config(mfp_cfg_pop);
	}
#endif
	pxa_amp_init();
	return 0;
}

int board_init(void)
{
	int ret;
	emmd_page = (struct emmd_page *)CRASH_BASE_ADDR;
	if (emmd_page->indicator == 0x454d4d44) {
		/* clear this indicator for prevent the re-entry */
		printf("Detect RAMDUMP signature!!\n");
		emmd_page->indicator = 0;
		flush_cache((unsigned long)(&emmd_page->indicator),
			    (unsigned long)sizeof(emmd_page->indicator));

		adtf_dump_all_trace_buffer();
		/* Register etb ddr buffer as RDI object if ramdump indicator is set*/
		ret = ramdump_attach_pbuffer("etbadtf", CONFIG_MV_ETB_DMP_ADDR, 0x10000);
		printf("add etbadtf to ramdump - %s\n", ret ? "fail" : "success");
		ret = ramdump_attach_pbuffer("powerud", (uintptr_t)emmd_page->pmic_power_reason,
						sizeof(emmd_page->pmic_power_reason));
		printf("add powerud to ramdump - %s\n", ret ? "fail" : "success");

		fastboot = 1;
	}
	check_chip_type();

#ifdef CONFIG_REVISION_TAG
	/* Get board info from eeprom */
	struct eeprom_data eeprom_data;
	eeprom_data.index = 1;
	eeprom_data.i2c_num = 0;
	printf("\nBoard info from EEPROM No.1:\n");
	eeprom_get_board_rev(&eeprom_data, &board_rev);
	/* Set to rev2 if the board is not rev1 */
	if (board_rev != 1) {
		board_rev = 2;
		eeprom_get_board_name(&eeprom_data, board_info);
		eeprom_get_board_type(&eeprom_data, &board_type);
		eeprom_get_board_manu_y_m(&eeprom_data);

		printf("\nBoard info from EEPROM No.2:\n");
		eeprom_data.index = 2;
		eeprom_data.i2c_num = 4;
		eeprom_get_board_category(&eeprom_data, board_info);
		if (eeprom_get_board_sn(&eeprom_data, board_sn)) {
			*board_sn = 0;
			printf("Board serial number is invalid\n");
		}
		eeprom_get_chip_name(&eeprom_data, board_info);
		eeprom_get_chip_stepping(&eeprom_data, board_info);
		eeprom_get_board_reg_date(&eeprom_data);
		eeprom_get_board_state(&eeprom_data);
		eeprom_get_user_team(&eeprom_data);
		eeprom_get_current_user(&eeprom_data);
		eeprom_get_board_eco(&eeprom_data, board_eco);
		eeprom_get_lcd_resolution(&eeprom_data, board_info);
		eeprom_get_lcd_screen_size(&eeprom_data, board_info);
		eeprom_get_ddr_type(&eeprom_data, board_info);
		eeprom_get_ddr_size_speed(&eeprom_data);
		eeprom_get_emmc_type(&eeprom_data, board_info);
		eeprom_get_emmc_size(&eeprom_data, board_info);
		eeprom_get_rf_name_ver(&eeprom_data);
		eeprom_get_rf_type(&eeprom_data, board_info);
		printf("\n");
	} else {
		eeprom_get_board_name(&eeprom_data, board_info);
		eeprom_get_board_type(&eeprom_data, &board_type);
		if (eeprom_get_board_sn(&eeprom_data, board_sn)) {
			*board_sn = 0;
			printf("Board serial number is invalid\n");
		}
		eeprom_get_board_eco(&eeprom_data, board_eco);
		eeprom_get_board_manu_y_m(&eeprom_data);
	}
#endif
	gd->bd->bi_arch_number = MACH_TYPE_PXA1928;
	gd->bd->bi_boot_params = 0x9a00000;

#ifdef CONFIG_CMD_GPIO
	gpio_direction_input(recovery_key);
#endif

	printf("run board_init\n");
	return 0;
}

static void charging_indication(bool on)
{
	u32 data = 0;
	struct pmic *p_base;

	p_base = pmic_get(board_pmic_chip->base_name);
	pmic_reg_read(p_base, LED_PWM_CONTROL7, &data);

	/* Blink red led to indicate charging to the user */
	if (on)
		data |= LED_EN | R_LED_EN | LED_BLK_MODE;
	else
		data &= ~(LED_EN | R_LED_EN | LED_BLK_MODE);

	pmic_reg_write(p_base, LED_PWM_CONTROL7, data);
}

#ifdef CONFIG_GENERIC_MMC
#ifdef CONFIG_POWER_88PM860
static unsigned char pxa1928_recovery_reg_read(void)
{
	u32 data = 0;
	struct pmic *p_base;

	p_base = pmic_get(board_pmic_chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return -1;
	}
	/* Get the magic number from RTC register */
	pmic_reg_read(p_base, 0xef, &data);
	return (unsigned char)data;
}
static unsigned char pxa1928_recovery_reg_write(unsigned char value)
{
	struct pmic *p_base;

	p_base = pmic_get(board_pmic_chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return -1;
	}
	/* Set the magic number from RTC register */
	pmic_reg_write(p_base, 0xef, value);
	return 0;
}
#else
static unsigned char pxa1928_recovery_reg_read(void)
{
	return 0;
}
static unsigned char pxa1928_recovery_reg_write(unsigned char value)
{
	return 0;
}
#endif
static struct recovery_reg_funcs pxa1928_recovery_reg_funcs = {
	.recovery_reg_read = pxa1928_recovery_reg_read,
	.recovery_reg_write = pxa1928_recovery_reg_write,
};

int recovery_key_detect(void)
{
	return !gpio_get_value(recovery_key);
}
#endif

#ifdef CONFIG_OF_LIBFDT
unsigned int dtb_offset(void)
{
	unsigned int offset;

	switch (board_rev) {
	case 2:
		offset = pxa1928_discrete ? 1 : 0;
		break;
	case 1:
		if (pxa1928_discrete == 0xff)
			/* non-arvm8 boards */
			offset = 0;
		else
			offset = pxa1928_discrete ? 3 : 2;
			break;
		case 0x9:
		case 0xA:
			offset = 0;
			printf("%s board_rev %x is known as pxa1928ff\n",
				__func__, board_rev);
		break;
	default:
		printf("%s board_rev %x unknown\n",
				__func__, board_rev);
		offset = 0;
		break;
	}

	return offset * DTB_SIZE;
}

void handle_dtb(struct fdt_header *devtree)
{
	char cmd[128]; int ret;

	/* set dtb addr */
	sprintf(cmd, "fdt addr 0x%p", devtree);
	run_command(cmd, 0);

#ifdef CONFIG_MMP_DISP
	if (bl_chip_id == BL_KDT3102) {
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/lm3532@38", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/pm828x@10", 0);
		run_command("fdt rm /pwm-bl", 0);
	} else {
		run_command("fdt rm /r63311 bl_gpio", 0);
		if (bl_chip_id == PM828X_ID)
			run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/lm3532@38", 0);
		else
			run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/pm828x@10", 0);
	}

	/*
	 * Some pxa1928ff boards have a  shuttered proximity sensor.
	 * In case the board owner would like to disable the
	 * proximity sensor, one can write 0x0 to board_eco[0] at EEPROM.
	 * In order to enable it back please write 0xff to board_eco[0].
	 */
	if (prox_sensor_is_off())
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033000/apds990x@39", 0);

	if (g_panel_id == SHARP_1080P_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /otm1281", 0);
		run_command("fdt rm /tft-10801920-1e", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/gt913@5d", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/s3202@22", 0);
		run_command("fdt set /soc/axi/disp/path1/pn_sclk_clksrc clksrc pll3", 0);
	} else if (g_panel_id == SHARP_QHD_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /otm1281", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/gt913@5d", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/s3202@22", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/s3202@20/ synaptics,swap_axes", 0);
		run_command("fdt set /soc/axi/disp/path1/pn_sclk_clksrc clksrc pll3", 0);

	} else {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /otm1281", 0);
		ret = run_command("fdt rm /tft-10801920-1e", 0);
		printf("ret = %d line = %d\n",ret, __LINE__);
		ret = run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/gt913@5d", 0);
		printf("ret = %d line = %d\n",ret, __LINE__);
		ret = run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/s3202@22", 0);
		printf("ret = %d line = %d\n",ret, __LINE__);
		run_command("fdt set /soc/axi/disp/path1/pn_sclk_clksrc clksrc pll3", 0);
	}

	if (handle_disp_rsvmem(&mmp_mipi_lcd_info))
		printf("Attention, framebuffer maybe not reserved correctly!\n");
#endif /* end of CONFIG_MMP_DISP */
	/*reserve mem for emmd*/
	run_command("fdt rsvmem add 0x8140000 0x1000", 0);
	/* pass profile number */
	sprintf(cmd, "fdt set /profile marvell,profile-number <%d>\n", mv_profile);
	run_command(cmd, 0);

	if (chip_type != PXA1926_2L_DISCRETE || (chip_type == PXA1926_2L_DISCRETE &&
				1 != pxa1928_discrete))
		run_command("fdt set /pp_version version pxa1928", 0);

	if (cpu_is_pxa1928_a0()) {
		run_command("fdt mknode / chip_version", 0);
		run_command("fdt set /chip_version version a0", 0);
		run_command("fdt set /clock-controller/peri_clock/gc2d_clk lpm-qos <3>", 0);
		run_command("fdt rm /soc/apb@d4000000/map@c3000000/ marvell,b0_fix", 0);
		run_command("fdt set /soc/apb@d4000000/thermal@d403b000 marvell,version-flag <3>",
				 0);
		run_command("fdt rm /soc/apb@d4000000/map@c3000000/ marvell,b0_fix", 0);
		/* overwrite emmc rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4217000 marvell,sdh-dtr-data "
			"<0 26000000 104000000 0 0 0 0 0 2 52000000 104000000 0 0 0 0 1 "
			"7 52000000 104000000 0 120 3 1 1 9 0xffffffff 104000000 0 0 0 0 0>", 0);
		/* overwrite sd card rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280000 marvell,sdh-dtr-data "
			"<0 26000000 104000000 0 0 0 0 0 1 26000000 104000000 0 0 0 0 1 "
			"3 52000000 104000000 0 0 0 0 1 4 52000000 104000000 0 0 0 0 1 "
			"5 52000000 104000000 0 0 0 0 1 7 52000000 104000000 0 0 0 0 1 "
			"9 0xffffffff 104000000 0 0 0 0 0>", 0);
		/* overwrite sdio rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280800 marvell,sdh-dtr-data "
			"<0 26000000 104000000 0 0 0 0 0 1 26000000 104000000 0 0 0 0 1 "
			"3 52000000 104000000 0 0 0 0 1 4 52000000 104000000 0 0 0 0 1 "
			"5 52000000 104000000 0 0 0 0 1 7 52000000 104000000 0 0 0 0 1 "
			"9 0xffffffff 104000000 0 0 0 0 0>", 0);
		/* disable emmc hs200 mode */
		run_command("fdt set /soc/axi@d4200000/sdh@d4217000 marvell,sdh-host-caps2-disable <0x20>", 0);
		/* disable sd card sdr104 mode */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280000 marvell,sdh-host-caps-disable <0x40000>", 0);
		/* disable sdio sdr104 mode */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280800 marvell,sdh-host-caps-disable <0x40000>", 0);

	} else {
		run_command("fdt mknode / chip_type", 0);
		switch (chip_type) {
		case PXA1926_2L_DISCRETE:
			run_command("fdt set /chip_type type <0>", 0);
			break;
		case PXA1928_POP:
			run_command("fdt set /chip_type type <1>", 0);
			break;
		case PXA1928_4L:
			run_command("fdt set /chip_type type <2>", 0);
			break;
		default:
			run_command("fdt set /chip_type type <0>", 0);
			break;
		}

		if (chip_type == PXA1926_2L_DISCRETE && 1 != pxa1928_discrete)
			run_command("fdt set /chip_type type <1>", 0);

		/* update dtb so as not to enable ICU for B0 stepping */
		run_command("fdt set /pxa1928_apmu_ver version bx", 0);
		run_command("fdt rm /soc/axi/wakeupgen@d4284000", 0);
		/* set patch flag of uart break for B0 stepping */
		run_command("fdt set /soc/apb@d4000000/uart@d4018000 break-abnormal <1>", 0);
	}
}
#endif

#ifndef CONFIG_GLB_SECURE_EN
#define SOC_POWER_POINT_ADDR0	0xD4292AAC
#define SOC_POWER_POINT_ADDR1	0xD4292AB0
static int svt_profile_map_table[] = {
	999, 445, 433, 421, 408, 395, 383, 371,
	358, 358, 333, 333, 309, 309, 1,
};
void show_dro(void)
{
	u32 val0, val1;
	int lvt, nlvt, nsvt, svt;
	int i;

	val0 = readl(SOC_POWER_POINT_ADDR0);
	val1 = readl(SOC_POWER_POINT_ADDR1);
	lvt = val0 & 0x7FF;
	nlvt = (val0 & 0x3FF800) >> 11;
	nsvt = ((val1 & 0x1) << 10) | ((val0 & 0xFFC00000) >> 22);
	svt = (val1 & 0xFFE) >> 1;
	printf("----show dro----\n");
	printf("LVT NUMBER: %d\n", lvt);
	printf("NLVT NUMBER: %d\n", nlvt);
	printf("NSVT NUMBER: %d\n", nsvt);
	printf("SVT NUMBER: %d\n", svt);
	printf("----------------\n");

	for (i = 1; i < 15; i++) {
		if (svt >= svt_profile_map_table[i] &&
			svt < svt_profile_map_table[i - 1])
			break;
	}
	mv_profile = i + 1;
	if (mv_profile >= 15 || mv_profile < 0)
		mv_profile = 0;
	printf("SoC Profile Number: %d\n", mv_profile);
}

int do_dro_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	show_dro();
	return 0;
}

U_BOOT_CMD(
	dro_read,	1,	0,	do_dro_read,
	"dro_read", ""
);
#endif

static int do_sethighperf(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmdline;
	char *ep;
	unsigned int new_set;
	static unsigned int old_set;
	u32 cpu_max = CPU_MAX_FREQ_DEFAULT;
	u32 ddr_max = DDR_MAX_FREQ_DEFAULT;
	u32 gc3d_max = GC3D_MAX_FREQ_DEFAULT;
	u32 gc2d_max = GC2D_MAX_FREQ_DEFAULT;

	if (argc != 2) {
		printf("usage: sethighperf 0 or 1 to enable low or high performance\n");
		return -1;
	}
	new_set = simple_strtoul((const char *)argv[1], &ep, 10);
	if (new_set != 0 && new_set != 1) {
		printf("usage: sethighperf 0 or 1 to enable low or high performance\n");
		return -1;
	}
	if (((cpu_is_pxa1928_a0() && (1 != pxa1928_discrete)) || (chip_type != PXA1926_2L_DISCRETE)) &&
		old_set != new_set) {
		old_set = new_set;
		cmdline = malloc(COMMAND_LINE_SIZE);
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		remove_cmdline_param(cmdline, "cpu_max=");
		remove_cmdline_param(cmdline, "ddr_max=");
		remove_cmdline_param(cmdline, "gc3d_max=");
		remove_cmdline_param(cmdline, "gc2d_max=");
		if (1 == new_set) {
			cpu_max = CPU_MAX_FREQ;
			gc3d_max = GC3D_MAX_FREQ;
			gc2d_max = GC2D_MAX_FREQ;
			if (ddr_speed != 0)
				ddr_max = ddr_speed;
		} else {
			if (ddr_speed != 0 && ddr_speed < DDR_MAX_FREQ_DEFAULT)
				ddr_max = ddr_speed;
		}
		sprintf(cmdline + strlen(cmdline), " cpu_max=%u000", cpu_max);
		sprintf(cmdline + strlen(cmdline), " ddr_max=%u000", ddr_max);
		sprintf(cmdline + strlen(cmdline), " gc3d_max=%u000", gc3d_max);
		sprintf(cmdline + strlen(cmdline), " gc2d_max=%u000", gc2d_max);
		setenv("bootargs", cmdline);
		free(cmdline);
	}
	printf("sethighperf success\n");
	return 0;
}

U_BOOT_CMD(
	sethighperf, 3, 0, do_sethighperf,
	"Setting cpu, ddr frequence for high performance or low performance",
	""
);

#ifdef CONFIG_PXA1928_CONCORD_BRINGUP
static void set_limit_max_frequency(unsigned int type, u32 max)
{
	return;
}
#else
static void set_limit_max_frequency(unsigned int type, u32 max)
{
	char *cmdline;
	char *rem;
	const char *add;
	u32 max_preferred;

	switch (type) {
	case SKU_CPU_MAX_PREFER:
		rem = "cpu_max=";
		add = " cpu_max=%u000";
		break;

	case SKU_DDR_MAX_PREFER:
		rem = "ddr_max=";
		add = " ddr_max=%u000";
		break;

	case SKU_GC3D_MAX_PREFER:
		rem = "gc3d_max=";
		add = " gc3d_max=%u000";
		break;

	case SKU_GC2D_MAX_PREFER:
		rem = "gc2d_max=";
		add = " gc2d_max=%u000";
		break;

	default:
		return;
	}

	max_preferred = get_sku_max_setting(type);
	if (0 == max_preferred)
		max_preferred = max;
	else
		max_preferred = min(max_preferred, max);

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, rem);
	sprintf(cmdline + strlen(cmdline), add, max_preferred);
	setenv("bootargs", cmdline);
	free(cmdline);
}
#endif
#if defined(CONFIG_PXA1928_DFC)
static u32 get_ddr_type(void)
{
	if (((readl(0xd0000300) >> 4) & 0xf) == 0xa) /* lpddr3 */
		if (((readl(0xd0000200) >> 16) & 0x1f) == 0xd) /* 1GB */
			return 1;
		else if (((readl(0xd0000210) >> 8) & 0xf) == 0x5)/* Hynix 2GB */
			return readl(0xD4282C98) ? 4 : 2; /* 4 for dis, 2 for pop */
		else /* Elpida 2GB */
			return 3;
	else /* lpddr2 */
		return 0;
}
#endif

#ifdef CONFIG_OBM_PARAM_ADDR
static void parse_cp_ddr_range(void)
{
	char *cmdline;
	struct OBM2OSL *obm2osl = (struct OBM2OSL *)
		(*(unsigned long *)CONFIG_OBM_PARAM_ADDR);
	unsigned int start = 0;
	unsigned int size = 0;

	if (get_cp_ddr_range(obm2osl, &start, &size) < 0)
		return;

	cmdline = malloc(COMMAND_LINE_SIZE);
	if (!cmdline) {
		printf("%s: malloc cmdline error\n", __func__);
		return;
	}

	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	remove_cmdline_param(cmdline, "cpmem=");
	if (size >> 20)
		sprintf(cmdline + strlen(cmdline), " cpmem=%uM@0x%08x",
			size >> 20, start);
	else
		sprintf(cmdline + strlen(cmdline), " cpmem=%uK@0x%08x",
			size >> 10, start);

	setenv("bootargs", cmdline);
	free(cmdline);
}
#else
static void parse_cp_ddr_range(void)
{
}
#endif

/* CONFIG_POWER_OFF_CHARGE=y */
__weak void power_off_charge(u32 *emmd_pmic, u32 lo_uv, u32 hi_uv,
		void (*charging_indication)(bool)) {}
__weak int get_powerup_mode(void) {return INVALID_MODE; }
int misc_init_r(void)
{
	unsigned long i, ddr_size = 0;
	char *cmdline;
	ulong base, size;
	unsigned int rtc_mode;
	int powermode;
	int power_up_mode;
	__attribute__((unused)) enum reset_reason rr;
#if defined(CONFIG_PXA1928_DFC)
	u32 ddr_type;
	ddr_type = get_ddr_type();
#endif
	u32 cpu_max, ddr_max, gc3d_max, gc2d_max;

	power_up_mode = get_powerup_mode();
	if (power_up_mode == BATTERY_MODE) {
		/*
		 * handle charge in uboot, the reason we do it here is because
		 * 1) we need to wait GD_FLG_ENV_READY to setenv()
		 * 2) we need to show uboot charging indication
		 */
		power_off_charge(&emmd_page->pmic_power_status, 0, 3600000,
				charging_indication);
	}

	if (pxa_is_warm_reset())
		setenv("bootdelay", "-1");

	if (!fastboot)
		show_logo();

	cmdline = malloc(COMMAND_LINE_SIZE);
	if (!cmdline) {
		printf("misc_init_r: can not allocate memory for cmdline\n");
		return -1;
	}
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (gd->bd->bi_dram[i].size == 0)
			break;
		ddr_size += gd->bd->bi_dram[i].size;
	}

	/* set ddr size in bootargs */
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, "mem=");
	sprintf(cmdline + strlen(cmdline), " mem=%ldM", ddr_size>>20);

	/* set cma size to 25MB */
	remove_cmdline_param(cmdline, "cma=");
	sprintf(cmdline + strlen(cmdline), " cma=51M");

	/* Set ioncarv on the command line */
	remove_cmdline_param(cmdline, "ioncarv=");
	base = CONFIG_SYS_TEXT_BASE;
	size = 0x9000000;
	if (!size || (base + size) > ddr_size)
		printf("ERROR: wrong ION setting!!");
	sprintf(cmdline + strlen(cmdline), " ioncarv=%ldM@0x%08lx",
		size>>20, base);
#ifdef CONFIG_RAMDUMP
	remove_cmdline_param(cmdline, "RDCA=");
	sprintf(cmdline + strlen(cmdline), " RDCA=%08lx",
		ramdump_desc.rdc_addr);
	/*
	 * Exclude the 2MB OBM area for now.
	 * Crash seem to set the phys_offset to the
	 * first p-header physical address.
	 */
	rd_mems[0].start = CONFIG_TZ_HYPERVISOR_SIZE;
	rd_mems[0].end = base;
	/*
	 * Dump the ION area which contains important data, e.g. ETB.
	 * Only skip the u-boot area to prevent gzip errors.
	 */
	rd_mems[1].start = CONFIG_SYS_RELOC_END;
	rd_mems[1].end = ddr_size;
#endif
	setenv("bootargs", cmdline);

	powermode = get_powermode();
	printf("flag of powermode is %d\n", powermode);

#ifdef CONFIG_OBM_PARAM_ADDR
	struct OBM2OSL *params = 0;
	int keypress = 0;

	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	params = (struct OBM2OSL *)(uintptr_t)(*(u32 *)CONFIG_OBM_PARAM_ADDR);
	if (params && params->signature == OBM2OSL_IDENTIFIER)
		keypress = params->booting_mode;

	/*adb reboot product*/
	/*enable RTC CLK*/
	rtc_mode = *(unsigned int *)(RTC_CLK);
	rtc_mode = (rtc_mode | APBC_APBCLK) & (~APBC_RST);
	writel(rtc_mode, RTC_CLK);
	udelay(10);

	rtc_mode = *(unsigned int *)(RTC_BRN0);

	if (rtc_mode == PD_MAGIC_NUM) {
		*(volatile unsigned int *)(RTC_BRN0) = 0;
		keypress = params->booting_mode = PRODUCT_USB_MODE;
		printf("[reboot product]product usb mode %d\n",keypress);
	}

	if (keypress == PRODUCT_UART_MODE)
		powermode = POWER_MODE_DIAG_OVER_UART;
	else if (keypress == PRODUCT_USB_MODE)
		powermode = POWER_MODE_DIAG_OVER_USB;

	switch (powermode) {
	case POWER_MODE_DIAG_OVER_UART:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		remove_cmdline_param(cmdline, "console=");
		remove_cmdline_param(cmdline, "earlyprintk=");
		sprintf(cmdline + strlen(cmdline), " androidboot.bsp=2");
		memset((void*) g_disp_start_addr, 0x0, g_disp_buf_size);
		show_debugmode_logo(&mmp_mipi_lcd_info, PRODUCT_MODE_LOGO_UART);
		break;

	case POWER_MODE_DIAG_OVER_USB:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		sprintf(cmdline + strlen(cmdline),
			" androidboot.console=ttyS0 androidboot.bsp=1");
		memset((void*) g_disp_start_addr, 0x0, g_disp_buf_size);
		show_debugmode_logo(&mmp_mipi_lcd_info, PRODUCT_MODE_LOGO_USB);
		break;

	default:
		break;
	}
	setenv("bootargs", cmdline);
#endif
	/* set cpu, gc3d, gc2d  max frequency */
	if (chip_type == PXA1926_2L_DISCRETE && 1 == pxa1928_discrete) {
		cpu_max = CPU_MAX_FREQ_DEFAULT;
		ddr_max = DDR_MAX_FREQ_DEFAULT;
		gc3d_max = GC3D_MAX_FREQ_DEFAULT;
		gc2d_max = GC2D_MAX_FREQ_DEFAULT;
	} else {
		cpu_max = CPU_MAX_FREQ;
		ddr_max = DDR_MAX_FREQ;
		gc3d_max = GC3D_MAX_FREQ;
		gc2d_max = GC2D_MAX_FREQ;
	}

	if (chip_type == PXA1928_POP)
		ddr_max = DDR_MAX_FREQ_POP;

	set_limit_max_frequency(SKU_CPU_MAX_PREFER, cpu_max);
	set_limit_max_frequency(SKU_DDR_MAX_PREFER, ddr_max);
	set_limit_max_frequency(SKU_GC3D_MAX_PREFER, gc3d_max);
	set_limit_max_frequency(SKU_GC2D_MAX_PREFER, gc2d_max);
#ifdef CONFIG_REVISION_TAG
	if (2 == board_rev) {
		struct eeprom_data eeprom_data;
		eeprom_data.index = 2;
		eeprom_data.i2c_num = 4;
		char *ep;
		u8 ddr_speed_info[8] = {0};
		if (eeprom_get_ddr_speed(&eeprom_data, ddr_speed_info))
			ddr_speed = 0;
		else
			ddr_speed = simple_strtoul((const char *)ddr_speed_info, &ep, 10);
		if (0 == ddr_speed)
			printf("Ddr speed info is invalid\n");
		if (ddr_speed != 0 && ddr_speed < ddr_max)
			set_limit_max_frequency(SKU_DDR_MAX_PREFER, ddr_speed);
	}
	if (*board_sn != 0) {
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		remove_cmdline_param(cmdline, "androidboot.serialno=");
		sprintf(cmdline + strlen(cmdline), " androidboot.serialno=%s", board_sn);
		setenv("bootargs", cmdline);

		/* this sn is also used as fastboot serial no. */
		setenv("fb_serial", (char *)board_sn);
	}
#endif

#ifdef CONFIG_CMD_FASTBOOT
	setenv("fbenv", "mmc0");
#endif

#ifndef CONFIG_GLB_SECURE_EN
	show_dro();
#endif
	/*
	 * bus 0 is used by pmic, set here for debug with
	 * "i2c probe", this should be the called just before exit,
	 * in case the default bus number is changed
	 */
	i2c_set_bus_num(0);
#if defined(CONFIG_PXA1928_DFC)
	if (!pxa_is_warm_reset()) {
		pxa1928_fc_init(ddr_type);

		run_command("setcpurate 624", 0);
		run_command("setddrrate hwdfc 312", 0);
		run_command("setaxirate 156", 0);
	}
#endif

#ifdef CONFIG_RAMDUMP
	/* query MPMU reset reason (also saves the info to DDR for RAMDUMP) */
	rr = get_reset_reason();
	if (fastboot) {
		const char *s;
		int ramdump;
		const char *dump_mode_var = "ramdump";

		setenv("fbenv", "mmc0");
		s = getenv(dump_mode_var);
		if (!s) {
			setenv(dump_mode_var, "0");
			s = getenv(dump_mode_var);
		}
		ramdump = simple_strtol(s, NULL, 10);
		if (!ramdump && (0x1 != emmd_page->dump_style)) {
			show_debugmode_logo(&mmp_mipi_lcd_info, RAMDUMP_SD_LOGO);
			/*
			 * Use force=1: EMMD signature has been detected, so
			 * need to dump even if RDC signature is missing,
			 * e.g. because RAMDUMP is not enabled in kernel.
			 */
			if (run_command("ramdump 0 0 1 2", 0)) {
				printf("SD dump fail, enter fastboot mode\n");
				show_debugmode_logo(&mmp_mipi_lcd_info, RAMDUMP_USB_LOGO);
				run_command("fb", 0);
			} else {
				run_command("reset", 0);
			}
		} else {
			printf("ready to enter fastboot mode\n");
			show_debugmode_logo(&mmp_mipi_lcd_info, RAMDUMP_USB_LOGO);
			run_command("fb", 0);
		}
	}
	/*
	 * Zero the entire DDR excluding the lower area already populated.
	 * Do this on power-on only. The reason is to eliminate the random
	 * contents DDR powers up with: this significantly improves the
	 * ramdump compression rate and reduces the dump size and time.
	 */
#ifndef BUILD_VARIANT_USER
	if (rr == rr_poweron)
		ddr_zero(0x10000000, ddr_size);
#endif
#endif
	/*adb reboot fastboot*/
	if (rtc_mode == FB_MAGIC_NUM) {
		*(volatile unsigned int *)(RTC_BRN0) = 0;
		printf("[reboot fastboot]fastboot mode");
		run_command("fb",0);
	}

	*(u32 *)(CONFIG_CORE_BUSY_ADDR) = 0x0;

	/* get CP ddr range */
	parse_cp_ddr_range();

	free(cmdline);
	return 0;
}

#ifdef CONFIG_MV_SDHCI
 /* eMMC: BUCK2/VCC_IO_NAND(1.8v)->eMMC(vmmcq), LDO4/V3P3_NAND(2.8v)->eMMC(vmmc) (default on)
 * SD: LDO6/VCC_IO_MMC1(3.3v)->SD(vmmcq), LDO10/V_MMC_CARD(3.3v)->SD(vmmc) (default off)
 */
int board_mmc_init(bd_t *bd)
{
	ulong mmc_base_address[CONFIG_SYS_MMC_NUM] = CONFIG_SYS_MMC_BASE;
	u8 i;
	u32 val;

	for (i = 0; i < CONFIG_SYS_MMC_NUM; i++) {
		if (mv_sdh_init(mmc_base_address[i], 104000000, 0,
				SDHCI_QUIRK_32BIT_DMA_ADDR))
			return 1;
		/*
		 * use default hardware clock gating
		 * by default, SD_FIFO_PARM = 0x70005
		 */
		if (i == 0) {
			/*
			 * emmc need to tune RX/TX under HS50
			 * RX need to set proper delays cycles.
			 * TX can work just invert the internal clock (TX_CFG_REG[30])
			 * but also set the delay cycles here for safety.
			 */
			writel(TX_MUX_DLL | TX_HOLD_DELAY0(0x16A),
						mmc_base_address[i] + TX_CFG_REG);
			writel(SDCLK_DELAY(0xA0) | SDCLK_SEL1(0x1),
						mmc_base_address[i] + RX_CFG_REG);
		} else {
			/*
			 * sd card can work under HS50 by default.
			 * but also invert TX internal clock (TX_CFG_REG[30]) here for safety.
			 */
			val = readl(mmc_base_address[i] + TX_CFG_REG);
			val |= TX_INT_CLK_INV;
			writel(val, mmc_base_address[i] + TX_CFG_REG);
		}
	}

	p_recovery_reg_funcs = &pxa1928_recovery_reg_funcs;
	return 0;
}
#endif

void reset_cpu(ulong ignored)
{
#ifdef CONFIG_POWER_88PM860
	printf("pxa1928 board rebooting...\n");
	pmic_reset_bd(board_pmic_chip);
#endif
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	u32 val;

	val = smp_hw_cpuid();
	if (val >= 0 && val <= 3)
		printf("Boot Core: Core %d\n", val + 1);
	else
		printf("Boot Core is unknown.");

	val = smp_config();
	printf("Available Cores: %s %s %s %s\n"
			, (val & 0x1) ? ("Core 1") : ("")
			, (val & 0x2) ? ("Core 2") : ("")
			, (val & 0x4) ? ("Core 3") : ("")
			, (val & 0x8) ? ("Core 4") : ("")
	);

	return 0;
}
#endif
#ifdef CONFIG_POWER_88PM860
void board_pmic_power_fixup(struct pmic *p_power)
{
	u32 val;
	unsigned int mask, dvc_ctrl;

	/* enable buck1 dual phase mode */
	pmic_reg_read(p_power, 0x8e, &val);
	val |= (1 << 2);
	pmic_reg_write(p_power, 0x8e, val);

	/* set buck1 all the DVC register 16 levels all at 1.2v
	 * dvc_ctrl is the value of the two dvc control bits
	 */
	for (dvc_ctrl = 0; dvc_ctrl < DVC_CTRl_LVL; dvc_ctrl++) {
		pmic_reg_read(p_power, DVC_CONTROL_REG, &val);
		mask = (DVC_SET_ADDR1 | DVC_SET_ADDR2);
		val &= ~mask;
		val |= dvc_ctrl & mask;
		pmic_reg_write(p_power, DVC_CONTROL_REG, val);

		val = 0x30;
		pmic_reg_write(p_power, 0x3c, val);
		pmic_reg_write(p_power, 0x3d, val);
		pmic_reg_write(p_power, 0x3e, val);
		pmic_reg_write(p_power, 0x3f, val);

		pmic_reg_write(p_power, 0x4b, val);
		pmic_reg_write(p_power, 0x4c, val);
		pmic_reg_write(p_power, 0x4d, val);
		pmic_reg_write(p_power, 0x4e, val);

		/* set buck3 all the DVC register at 1.2v */
		val = 0x30;
		pmic_reg_write(p_power, 0x41, val);
		pmic_reg_write(p_power, 0x42, val);
		pmic_reg_write(p_power, 0x43, val);
		pmic_reg_write(p_power, 0x44, val);

		/* set buck5 all the DVC register at 3.3v for WIB_SYS */
		val = 0x72;
		pmic_reg_write(p_power, 0x46, val);
		pmic_reg_write(p_power, 0x47, val);
		pmic_reg_write(p_power, 0x48, val);
		pmic_reg_write(p_power, 0x49, val);
	}
	/* set ldo5 at 2.8v for emmc */
	marvell88pm_set_ldo_vol(p_power, 5, 2800000);

#ifdef CONFIG_MMP_DISP
#ifdef CONFIG_REVISION_TAG
		/* set ldo3(avdd_dsi) to 1.2v and enable it */
		marvell88pm_set_ldo_vol(p_power, 3, 1200000);

		/* set buck2(vcc_io_gpio1) for 5v_en to 1.8v and enable it */
		marvell88pm_set_buck_vol(p_power, 2, 1800000);

		/* set ldo12 to 2.8v and enable it */
		marvell88pm_set_ldo_vol(p_power, 12, 2800000);

		/* set ldo6 to 1.8v and enable it */
		marvell88pm_set_ldo_vol(p_power, 6, 1800000);

		/* set ldo4 to 2.8v and enable it */
		marvell88pm_set_ldo_vol(p_power, 4, 2800000);

#endif
#endif
	/* set ldo17 at 2.8v */
	marvell88pm_set_ldo_vol(p_power, 17, 2800000);
}
#endif

__weak struct pmic_chip_desc *get_marvell_pmic(void)
{
	return board_pmic_chip;
}

int power_init_board(void)
{
	int ret;

	board_pmic_chip = calloc(sizeof(struct pmic_chip_desc), 1);
	if (!board_pmic_chip) {
		printf("%s: No available memory for allocation!\n", __func__);
		return -ENOMEM;
	}

	/* init PMIC */
	if (pmic_marvell_init(PMIC_I2C_BUS, board_pmic_chip))
		return -1;

	ret = batt_marvell_init(board_pmic_chip,
				PMIC_I2C_BUS,
				CHG_I2C_BUS,
				FG_I2C_BUS);
	if (ret < 0)
		return ret;

	return 0;
}

#ifdef CONFIG_CMD_NET
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

#ifdef CONFIG_MMP_DISP
int pxa1928_lcd_power_control(int on)
{
	u32 val, count = 0;
	__attribute__((unused))	struct pxa1928apmu_registers *apmu =
		(struct pxa1928apmu_registers *) PXA1928_APMU_BASE;

	/*  HW mode power on/off*/
	if (on) {
		powerup_display_controller(apmu);
		stdio_init();
		show_logo();
		printf("lcd on for power test\n");
	} else {
		turn_off_backlight();
		lcd_power_en(0);
		val = readl(&apmu->disp_clkctrl2);
		val &= ~DISP_CLKCTRL2_ACLK_DIV;
		writel(val, &apmu->disp_clkctrl2);

		val = readl(&apmu->disp_clkctrl);
		val &= ~(DISP_CLKCTRL_ESC_CLK_EN | DISP_CLKCTRL_CLK1_DIV |
				DISP_CLKCTRL_CLK2_DIV | DISP_CLKCTRL_VDMA_DIV);
		writel(val, &apmu->disp_clkctrl);

		val = readl(&apmu->disp_rstctrl);
		val &= ~(DISP_RSTCTRL_ACLK_RSTN | DISP_RSTCTRL_ACLK_PORSTN |
				DISP_RSTCTRL_VDMA_PORSTN | DISP_RSTCTRL_HCLK_RSTN |
				DISP_RSTCTRL_VDMA_CLK_RSTN | DISP_RSTCTRL_ESC_CLK_RSTN);
		writel(val, &apmu->disp_rstctrl);

		val = readl(&apmu->isld_lcd_pwrctrl);
		val &= ISLD_LCD_PWR_HWMODE_EN;
		writel(val, &apmu->isld_lcd_pwrctrl);

		val = readl(&apmu->isld_lcd_pwrctrl);
		val &= ~(ISLD_LCD_PWR_UP);
		writel(val, &apmu->isld_lcd_pwrctrl);

		while (readl(&apmu->isld_lcd_pwrctrl) & ISLD_LCD_PWR_STATUS) {
			udelay(1000);
			count++;
			if (count > 20) {
				printf("Timeout for polling disp active interrupt\n");
				break;
			}
		}
		printf("lcd off for power test\n");
	}
	return 0;
}
int do_lcdpwr(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong onoff;
	int res = -1;
	if (argc != 2) {
		printf("usage: lcd_pwr 1/0\n");
		return -1;
	}

	res = strict_strtoul(argv[1], 0, &onoff);
	if (res == 0)
		pxa1928_lcd_power_control(onoff);

	return 0;
}

U_BOOT_CMD(
	lcd_pwr, 6, 1, do_lcdpwr,
	"LCD power on/off",
	""
);
#endif

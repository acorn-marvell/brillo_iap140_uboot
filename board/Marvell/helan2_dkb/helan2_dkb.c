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
#include <emmd_rsv.h>
#include <asm/arch/mfp.h>
#include <asm/gpio.h>
#include <asm/sizes.h>
#include <eeprom_34xx02.h>
#ifdef CONFIG_GENERIC_MMC
#include <sdhci.h>
#include <mv_sdhci.h>
#endif
#include <power/88pm830.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <power/pxa988_freq.h>
#include <power/mmp_idle.h>

#define PMIC_I2C_BUS 2
#define CHG_I2C_BUS 2
#define FG_I2C_BUS 2

#include "../common/charge.h"

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

DECLARE_GLOBAL_DATA_PTR;

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

static u8 ddr_mode;
static int fastboot;
#define RAMDUMP_USB_DUMP 1
#define RAMDUMP_SD_DUMP 2

/* 0 - 400MHz; 1 - 533Mhz, 2 -- 667Mhz, 3 -- 800Mhz */
static void helan2_get_ddr_mode(void)
{
	struct OBM2OSL *params;
	params = (struct OBM2OSL *)(*(u32 *)CONFIG_OBM_PARAM_ADDR);

	if (!params || (params->ddr_mode >= 4)) {
		printf("WARNING: no available ddr mode !!!\n");
		/* use 400Mhz by default*/
		ddr_mode = 0;
	} else {
		ddr_mode = params->ddr_mode;
	}
}

/*
 * helan2 uses direct key dk_pin1 and dk_pin2
 * So the param of helan2_key_detect should be
 * 1 or 2. For example, helan2_key_detect(1);
 */
static int __attribute__ ((unused)) helan2_key_detect(int dk_pin)
{
	u32 kpc_dk;
	udelay(500);
	kpc_dk = __raw_readl(KPC_DK);

	if (!(kpc_dk & (1u << dk_pin)))
		return 1;
	else
		return 0;
}

static void __attribute__ ((unused)) helan2dkb_keypad_init(void)
{
	u32 cnt = 5, kpc_pc = 0x00000042;
	/* enable keypad clock */
	__raw_writel(0x3, PXA988_APBC_KEYPAD);
	/* initialize keypad control register */
	__raw_writel(kpc_pc, KPC_PC);

	while (cnt--) {
		if (kpc_pc == __raw_readl(KPC_PC))
			break;
	}
}

int board_early_init_f(void)
{
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
	pxa_amp_init();
	return 0;
}

void reset_cpu(unsigned long ignored)
{
#ifdef CONFIG_POWER_88PM820
	printf("helan2 board rebooting...\n");
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

int board_init(void)
{
	emmd_page = (struct emmd_page *)CRASH_BASE_ADDR;
	/* arch number of Board */
	gd->bd->bi_arch_number = MACH_TYPE_HELAN2_DKB;
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_TZ_HYPERVISOR_SIZE + 0x00100000;

	helan2_get_ddr_mode();

#ifdef CONFIG_SQU_DEBUG_SUPPORT
	show_squdbg_regval();
#endif

	if (emmd_page->indicator == 0x454d4d44) {
		/* clear this indicator for prevent the re-entry */
		printf("Detect RAMDUMP signature!!\n");
		emmd_page->indicator = 0;
		flush_cache((unsigned long)(&emmd_page->indicator),
			    (unsigned long)sizeof(emmd_page->indicator));
		fastboot = 1;

#ifdef CONFIG_SQU_DEBUG_SUPPORT
		save_squdbg_regval2ddr();
#endif
	}

	return 0;
}

#ifdef CONFIG_POWER_88PM820
void board_pmic_power_fixup(struct pmic *p_power)
{
	u32 val;
	/* Set buck3(for DDR) at 1.2V */
	val = 0x30;
	pmic_reg_write(p_power, 0x41, val);
	pmic_reg_write(p_power, 0x42, val);
	pmic_reg_write(p_power, 0x43, val);
	pmic_reg_write(p_power, 0x44, val);

	/* Set buck5(for RF) at 1.2V */
	val = 0x30;
	pmic_reg_write(p_power, 0x46, val);
	pmic_reg_write(p_power, 0x47, val);
	pmic_reg_write(p_power, 0x48, val);
	pmic_reg_write(p_power, 0x49, val);

	/* Set vbuck1 sleep voltage as 0.7V */
	val = 0x8;
	pmic_reg_write(p_power, 0x30, val);
	/* enable buck1 sleep mode*/
	pmic_reg_read(p_power, 0x5a, &val);
	val &= 0xfc;
	val |= 0x2;
	pmic_reg_write(p_power, 0x5a, val);

	/* Set LDO10 as 3.1V for LCD  */
	marvell88pm_set_ldo_vol(p_power, 10, 3100000);

	/* Set LDO4 as 3.3V for SD VDD */
	marvell88pm_set_ldo_vol(p_power, 4, 3300000);
	/* Set LDO5 as 3.3V for SD signaling voltage */
	marvell88pm_set_ldo_vol(p_power, 5, 3300000);
}
#endif

__weak int power_init_common(void) {return -1; }
__weak int pmic_init(unsigned char bus) {return -1; }
int power_init_board(void)
{
	/* init PMIC  */
	if (pmic_init(PMIC_I2C_BUS))
		return -1;
	if (power_init_common()) {
		printf("%s: init pmic fails.\n", __func__);
		return -1;
	}

	return 0;
}

#if defined(CONFIG_MMP_DISP)
static struct mmp_disp_plat_info mmp_mipi_info_saved;
#define EMEI_EMMC_BMP_BASE	(0xE00000/512)
#define EMEI_EMMC_BMP_SIZE	(0x100000/512)

#define HELAN_720P_FB_XRES		720
#define HELAN_720P_FB_YRES		1280

static struct dsi_info helan2_720p_dsi = {
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
		.left_margin = 30,
		.right_margin = 136,
		.vsync_len = 2,
		.upper_margin = 8,
		.lower_margin = 10,
		.sync = FB_SYNC_VERT_HIGH_ACT
			| FB_SYNC_HOR_HIGH_ACT,
	},
};


static void update_panel_info(struct mmp_disp_plat_info *fb);
static struct mmp_disp_plat_info mmp_mipi_lcd_info = {
	.index = 0,
	.id = "GFX Layer",
	/* FIXME */
	.sclk_src = 416000000,
	.sclk_div = 0x00000208,
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
	.phy_info = &helan2_720p_dsi,
	.dsi_panel_config = lg_720p_init_config,
	.dynamic_detect_panel = 1,
	.update_panel_info = update_panel_info,
	.panel_manufacturer_id = 0,
};

#define HELAN_1080P_FB_XRES		1080
#define HELAN_1080P_FB_YRES		1920

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

#define LCD_RST_GPIO1	4
#define BOOST_EN_5V	6
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

#define LCD_PWM_GPIO32	32
static void turn_off_backlight(void)
{
	gpio_direction_input(LCD_PWM_GPIO32);
}

static void turn_on_backlight(void)
{
	int bl_level, p_num, intensity = 50;
	if (g_panel_id == SHARP_1080P_ID) {
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
	fb->sclk_div = 0x00000208,
	fb->num_modes = ARRAY_SIZE(video_720p_modes);
	fb->modes = video_720p_modes;

	/* align with android format and vres_virtual pitch */
	fb->dsi_panel_config = hx8394a_init_config;
	fb->panel_manufacturer_id = HX8394A_720P_ID;
}

void set_1080p_info(struct mmp_disp_plat_info *fb)
{
	fb->sclk_div = 0x00000104;
	fb->num_modes = ARRAY_SIZE(video_1080p_modes);
	fb->modes = video_1080p_modes;

	fb->dsi_panel_config = panel_1080p_init_config;
	fb->panel_manufacturer_id = SHARP_1080P_ID;
}

static GraphicDevice ctfb;
static void update_panel_info(struct mmp_disp_plat_info *fb)
{
	switch (g_panel_id) {
	case SHARP_1080P_ID:
		set_1080p_info(fb);
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
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;

	turn_off_backlight();
	lcd_power_en(1);

	ret = (void *)mmp_disp_init(fb);

	memcpy(&mmp_mipi_info_saved, fb, sizeof(struct mmp_disp_plat_info));
	turn_on_backlight();
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

static int use_cm3(u8 i2c_bus, u8 addr)
{
	unsigned int old_bus;
	int ret;

	old_bus = i2c_get_bus_num();
	i2c_set_bus_num(i2c_bus);
	ret = i2c_probe(addr);
	i2c_set_bus_num(old_bus);

	/* detected: use AP, else use cm3 */
	return !!ret;
}

/* this i2c bus has been controlled by AP in dts file by default */
void handle_cm3(u8 i2c_bus, u8 addr)
{
	if (use_cm3(i2c_bus, addr)) {
		/* delete this i2c bus node */
		run_command("fdt rm /soc/apb@d4000000/i2c@d4013800", 0);
		/* delete the apsenhb node */
		run_command("fdt rm /apsenhb", 0);
		printf("use cm3...\n");
	} else {
		/* delete the cm3senhb node */
		run_command("fdt rm /cm3senhb", 0);
		printf("not use cm3...\n");
	}
}

/* attention for the bus number and the checked i2c addr, it's changable */
#define SENSOR_HUB_I2C_BUS 3
#define CANARY_ADDR 0x69

#ifdef CONFIG_OF_LIBFDT
void handle_dtb(struct fdt_header *devtree)
{
	char cmd[128];

#if defined(CONFIG_MMP_DISP)
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
#endif

	/* set dtb addr */
	sprintf(cmd, "fdt addr 0x%p", devtree);
	run_command(cmd, 0);

#if defined(CONFIG_MMP_DISP)
	/* detele unused panel & touch device node */
	if (fb->panel_manufacturer_id == SHARP_1080P_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /hx8394", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/s3202@720p", 0);
	} else if (fb->panel_manufacturer_id == HX8394A_720P_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/s3202@1080p", 0);
	} else {
		run_command("fdt rm /hxr394", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/s3202@1080p", 0);
	}
#endif
	handle_cm3(SENSOR_HUB_I2C_BUS, CANARY_ADDR);
}
#endif

#ifdef CONFIG_MISC_INIT_R
static unsigned char helan2dkb_recovery_reg_read(void)
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

static unsigned char helan2dkb_recovery_reg_write(unsigned char value)
{
	struct pmic *pmic_base;

	pmic_base = pmic_get(MARVELL_PMIC_BASE);
	if (!pmic_base || pmic_probe(pmic_base)
	    || pmic_reg_write(pmic_base, 0xef, value))
		printf("write rtc register error\n");

	return 0;
}

static struct recovery_reg_funcs helan2dkb_recovery_reg_funcs = {
	.recovery_reg_read = helan2dkb_recovery_reg_read,
	.recovery_reg_write = helan2dkb_recovery_reg_write,
};

#define EEPROM_BOARD_SN_LEN	32

static ulong m3shm_area_size = 0x00100000;
static ulong m3shm_area_addr = 0x0a000000;

static ulong m3acq_area_size = 0x01000000;
static ulong m3acq_area_addr = 0x0b000000;

#define LOW_RAM_DDR_SIZE 0x30000000

/* CONFIG_POWER_OFF_CHARGE=y */
__weak void power_off_charge(u32 *emmd_pmic, u8 pmic_i2, u8 chg_i2c, u8 fg_i2c,
			     u32 lo_uv, u32 hi_uv, void (*charging_indication)(bool)) {}
int misc_init_r(void)
{
	char *cmdline;
	char *s;
	int keypress = 0, cpu, i;
	u32 base, size;
	u32 ddr_size = 0;
	u32 stored_ddr_mode = 0;
	struct OBM2OSL *params;
	char *board_sn;
	struct eeprom_data eeprom_data;
#ifdef CONFIG_RAMDUMP
	int ramdump;
#endif
	/*
	 * handle charge in uboot, the reason we do it here is because
	 * 1) we need to wait GD_FLG_ENV_READY to setenv()
	 * 2) we need to show uboot charging indication
	 */
	power_off_charge(&emmd_page->pmic_power_status,
			 PMIC_I2C_BUS, CHG_I2C_BUS, FG_I2C_BUS, 0, 3600000, NULL);

	if (pxa_is_warm_reset())
		setenv("bootdelay", "-1");

	/* Put CP and MSA in sleep as default for Helan2 */
	cp_msa_sleep();

#if defined(CONFIG_MMP_DISP)
	if (!fastboot)
		show_logo();
#endif

	p_recovery_reg_funcs = &helan2dkb_recovery_reg_funcs;

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	params = (struct OBM2OSL *)(*(u32 *)CONFIG_OBM_PARAM_ADDR);
	if (params && params->signature == OBM2OSL_IDENTIFIER)
		keypress = params->booting_mode;

	switch (keypress) {
	case PRODUCT_UART_MODE:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		/* END key pressed */
		remove_cmdline_param(cmdline, "console=");
		sprintf(cmdline + strlen(cmdline), " androidboot.bsp=2");
		break;
	case PRODUCT_USB_MODE:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		/* OK key pressed */
		sprintf(cmdline + strlen(cmdline),
			" androidboot.console=ttyS1 androidboot.bsp=1");
		break;
	}

	/* set the cma range, we can change it according to
	 * system configuration */
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (gd->bd->bi_dram[i].size == 0)
			break;
		ddr_size += gd->bd->bi_dram[i].size;
	}

	remove_cmdline_param(cmdline, "cma=");
	remove_cmdline_param(cmdline, "androidboot.low_ram=");
	if (ddr_size > LOW_RAM_DDR_SIZE)
		sprintf(cmdline + strlen(cmdline), " cma=112M");
	else
		sprintf(cmdline + strlen(cmdline), " cma=64M androidboot.low_ram=true");

	s = get_cmdline_param(cmdline, "ddr_mode=");
	if (s) {
		stored_ddr_mode = (u32)simple_strtoul(s, NULL, 10);
		if (stored_ddr_mode != ddr_mode) {
			printf("Warning: ddr_mode=%d from %s is overwritten by ddr_mode=%d from %s\n",
				stored_ddr_mode > ddr_mode ? stored_ddr_mode : ddr_mode,
				stored_ddr_mode > ddr_mode ? "flash" : "OBM",
				stored_ddr_mode > ddr_mode ? ddr_mode : stored_ddr_mode,
				stored_ddr_mode > ddr_mode ? "OBM" : "flash");
		}
		ddr_mode = min(ddr_mode, stored_ddr_mode);
	}
	remove_cmdline_param(cmdline, "ddr_mode=");
	sprintf(cmdline + strlen(cmdline), " ddr_mode=%d", ddr_mode);

	remove_cmdline_param(cmdline, "m3shmmem=");
	sprintf(cmdline + strlen(cmdline), " m3shmmem=%luM@0x%08lx",
		m3shm_area_size>>20, m3shm_area_addr);

	remove_cmdline_param(cmdline, "m3acqmem=");
	sprintf(cmdline + strlen(cmdline), " m3acqmem=%luM@0x%08lx",
		m3acq_area_size>>20, m3acq_area_addr);

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

	/* allocate 1 byte more to store the last '0' */
	board_sn = malloc(EEPROM_BOARD_SN_LEN + 1);
	board_sn[0] = 0;
	eeprom_data.index = 2;
	eeprom_data.i2c_num = 1;
	eeprom_get_board_sn(&eeprom_data, (u8 *)board_sn);

	/* check board_sn and make sure it is valid */
	for (i = 0; i < EEPROM_BOARD_SN_LEN; i++) {
		/* 32~126 are valid printable ASIIC */
		if ((board_sn[i] < 32) || (board_sn[i] > 126)) {
			board_sn[i] = 0;
			break;
		}
	}
	/* at least there is a '0' at the end of the string */
	board_sn[EEPROM_BOARD_SN_LEN] = 0;

	/* if board_sn[0] == 0, it means didn't get valid sn from eeprom */
	if (board_sn[0] != 0) {
		remove_cmdline_param(cmdline, "androidboot.serialno=");
		sprintf(cmdline + strlen(cmdline),
			" androidboot.serialno=%s", board_sn);

		/* this sn is also used as fastboot serial no. */
		setenv("fb_serial", board_sn);
	}

	free(board_sn);

	setenv("bootargs", cmdline);
	free(cmdline);

#ifdef CONFIG_HELAN2_POWER
	/* increase freq before emmd dump */
	helan2_fc_init(ddr_mode);
	setop(832, 312, 156);
#endif

	for (cpu = 0; cpu < CONFIG_NR_CPUS; cpu++)
		set_idle_count(cpu, 0);

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

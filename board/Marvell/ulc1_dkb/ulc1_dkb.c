/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Dongjiu Geng <djgeng@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <pxa_amp.h>
#include <mvmfp.h>
#include <i2c.h>
#include <emmd_rsv.h>
#include <asm/arch/mfp.h>
#include <asm/gpio.h>
#include <asm/sizes.h>
#include <asm/armv8/adtfile.h>
#include <eeprom_34xx02.h>
#ifdef CONFIG_GENERIC_MMC
#include <sdhci.h>
#include <mv_sdhci.h>
#endif
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <power/88pm886.h>
#include <power/pxa988_freq.h>
#include <power/mmp_idle.h>
#include <emmd_rsv.h>
#include <linux/ctype.h>
#if defined(CONFIG_MMP_DISP)
#include <mmp_disp.h>
#include <video_fb.h>
#include "../common/panel.h"
#include "../common/logo_pxa988.h"
#include "../common/logo_debugmode.h"
#endif

#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif

#include <mmc.h>

#include "../common/obm2osl.h"
#include "../common/cmdline.h"
#include <mv_recovery.h>
#include "../common/mrd_flag.h"
#include "../common/mv_cp.h"
#include "../common/charge.h"

#define PMIC_I2C_BUS 2
#define CHG_I2C_BUS 2
#define FG_I2C_BUS 2

static struct pmic_chip_desc *board_pmic_chip;

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
	/* rdc header address */
	.rdc_addr = 0x8140400,
	/* dump phys start address in DDR */
	.phys_start = CONFIG_TZ_HYPERVISOR_SIZE,
	.mems = &rd_mems[0],
	.nmems = sizeof(rd_mems)/sizeof(rd_mems[0]),
};
#endif
static int fastboot;
static int fastboot_key = 0;

static u8 ddr_mode;

/* 1 - 533Mhz, 2 -- 667Mhz */
static void ulc1_get_ddr_mode(void)
{
	struct OBM2OSL *params;
	params = (struct OBM2OSL *)(*(unsigned long *)CONFIG_OBM_PARAM_ADDR);

	if (!params || (params->ddr_mode >= 4)) {
		printf("WARNING: no available ddr mode !!!\n");
		/* use 533Mhz by default */
		ddr_mode = 1;
	} else {
		ddr_mode = params->ddr_mode;
	}
}

/*
 * ulc1 uses direct key dk_pin1 and dk_pin2
 * So the param of ulc1_key_detect should be
 * 1 or 2. For example, ulc1_key_detect(1);
 */
static int __attribute__ ((unused)) ulc1_key_detect(int dk_pin)
{
	u32 kpc_dk;
	udelay(500);
	kpc_dk = __raw_readl(KPC_DK);

	if (!(kpc_dk & (1u << dk_pin)))
		return 1;
	else
		return 0;
}

static void __attribute__ ((unused)) ulc1dkb_keypad_init(void)
{
	u32 cnt = 5, kpc_pc = 0x00000042;
	/* enable keypad clock */
	__raw_writel(0x3, PXA1908_APBC_KEYPAD);
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

		/* Wi-Fi PDn */
		GPIO007_GPIO_7,

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

		/* Fastboot key */
		GPIO000_KP_MKIN0,
		GPIO001_KP_MKIN0,
		GPIO002_KP_MKIN0,
		GPIO003_KP_MKIN0,
		GPIO000_GPIO_0_2,
		GPIO001_GPIO_1_2,
		GPIO002_GPIO_2_2,
		GPIO003_GPIO_3_2,

		/* User LED */
		GPIO005_GPIO_5, /* USER LED 0 */
		GPIO006_GPIO_6, /* USER LED 1 */
		GPIO008_GPIO_8, /* USER LED 2 */
		GPIO015_GPIO_15, /* USER LED 3 */

		GPIO030_GPIO_30, /* WIFI LED 3 */
		GPIO029_GPIO_29, /* BT LED 3 */
		GPIO010_GPIO_10, /* ZIGBEE LED 3 */

		MFP_EOC		/*End of configureation*/
	};

	mfp_config(mfp_cfg);
	pxa_amp_init();
	return 0;
}

void reset_cpu(unsigned long ignored)
{
	printf("ulc1 board rebooting...\n");
	pmic_reset_bd(board_pmic_chip);
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
		//if (mv_sdh_init(mmc_base_address[i], 104*1000000, 0,
		//		SDHCI_QUIRK_32BIT_DMA_ADDR))
		if (mv_sdh_init(mmc_base_address[i], 104*1000000, 0,
				0))
			return 1;
	}

	return 0;
}
#endif

int fastboot_key_detect(void)
{
	return !gpio_get_value(fastboot_key);
}

void fastboot_key_set_led(void)
{
	gpio_set_value(5, 1);
	gpio_set_value(6, 0);
	gpio_set_value(8, 0);
	gpio_set_value(15, 0);
}

void powerup_indicator(void)
{
	gpio_set_value(5, 1);
	gpio_set_value(6, 1);
	gpio_set_value(8, 1);
	gpio_set_value(15, 1);
}

int board_init(void)
{
	emmd_page = (struct emmd_page *)CRASH_BASE_ADDR;
	/* arch number of Board */
	gd->bd->bi_arch_number = MACH_TYPE_HELAN2_DKB;
	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x9a00000;

	ulc1_get_ddr_mode();

#ifdef CONFIG_SQU_DEBUG_SUPPORT
	show_squdbg_regval();
#endif

#ifdef CONFIG_RAMDUMP
	if (emmd_page->indicator == 0x454d4d44) {
		/* clear this indicator for prevent the re-entry */
		printf("Detect RAMDUMP signature!!\n");
		emmd_page->indicator = 0;
		flush_cache((unsigned long)(&emmd_page->indicator),
			    (unsigned long)sizeof(emmd_page->indicator));
		adtf_dump_all_trace_buffer();
		/* Register etb ddr buffer as RDI object */
		ramdump_attach_pbuffer("etbadtf",
				       CONFIG_MV_ETB_DMP_ADDR + 8, 0xfff8);
		fastboot = 1;

#ifdef CONFIG_SQU_DEBUG_SUPPORT
		save_squdbg_regval2ddr();
#endif
	}
#endif

	gpio_direction_input(fastboot_key);
	/* Light  LEDs for boot indicator, user LED0 is used as fastboot indicator */
	gpio_direction_output(5, 0);
	gpio_direction_output(6, 0);
	gpio_direction_output(8, 0); 
	gpio_direction_output(15, 0); 

	/* Wi-Fi PDn */
	gpio_direction_output(7, 0);
	printf("Wi-Fi power down by pulling down GPIO_7\n");

	/*
	gpio_direction_output(30, 0); 
	gpio_direction_output(29, 0); 
	gpio_direction_output(10, 0); 
	*/
	
	powerup_indicator();

	return 0;
}

#if defined(CONFIG_MMP_DISP)
static struct mmp_disp_plat_info mmp_mipi_info_saved;
#define HELAN_720P_FB_XRES		720
#define HELAN_720P_FB_YRES		1280
#define HELAN_FWVGA_FB_XRES		480
#define HELAN_FWVGA_FB_YRES		854

static struct dsi_info ulc1_720p_dsi = {
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
		.lower_margin = 30,
		.sync = FB_SYNC_VERT_HIGH_ACT
			| FB_SYNC_HOR_HIGH_ACT,
	},
};

static struct dsi_info ulc1_fwvga_dsi = {
	.id = 1,
	.lanes = 2,
	.bpp = 24,
	.burst_mode = DSI_BURST_MODE_BURST,
	.rgb_mode = DSI_LCD_INPUT_DATA_RGB_MODE_888,
	.hbp_en = 1,
	.master_mode = 1,
};
static struct lcd_videomode video_fwvga_modes[] = {
	[0] = {
		/* we should align the follwoing settings with kernel driver */
		.refresh = 60,
		.xres = HELAN_FWVGA_FB_XRES,
		.yres = HELAN_FWVGA_FB_YRES,
		.hsync_len = 4,
		.left_margin = 82,
		.right_margin = 86,
		.vsync_len = 4,
		.upper_margin = 10,
		.lower_margin = 30,
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
	.sclk_div = 0x0000020c,
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
	.phy_info = &ulc1_720p_dsi,
	.dsi_panel_config = hx8394a_init_config,
	.dynamic_detect_panel = 1,
	.update_panel_info = update_panel_info,
	.calculate_sclk = lcd_calculate_sclk,
	.panel_manufacturer_id = HX8394A_720P_ID,
};

#define LCD_RST_GPIO1	4
#define BOOST_EN_5V	6
static void lcd_power_en(int on)
{
	if (on) {
		gpio_direction_output(LCD_RST_GPIO1, 0);
		udelay(10000);
		gpio_direction_output(LCD_RST_GPIO1, 1);
		udelay(200000);
	} else
		gpio_direction_output(LCD_RST_GPIO1, 0);
}

#define LCD_PWM_GPIO32	32
static void turn_off_backlight(void)
{
	gpio_direction_input(LCD_PWM_GPIO32);
}

static void turn_on_backlight(void *fbi)
{
	pwm_turn_on_backlight((struct mmp_disp_info *)fbi);
	/* Change control backlight to panel's PWM, so far, can't
	 * disconnect GPIO32 with LCD_PWM, so have to set it to
	 * input, avoid to disturb PWM from panel.
	 */
	gpio_direction_input(LCD_PWM_GPIO32);
}

void set_fwvga_info(struct mmp_disp_plat_info *fb)
{

	fb->sclk_div = 0x00000210;
	fb->num_modes = ARRAY_SIZE(video_fwvga_modes);
	fb->modes = video_fwvga_modes;
	fb->phy_info = &ulc1_fwvga_dsi;

	fb->dsi_panel_config = otm8018b_init_config;
	fb->panel_manufacturer_id = OTM8018B_ID;

}

void set_otm1283a_info(struct mmp_disp_plat_info *fb)
{
	fb->dsi_panel_config = otm1283a_init_config;
	fb->panel_manufacturer_id = OTM1283A_ID;

}

static GraphicDevice ctfb;
static void update_panel_info(struct mmp_disp_plat_info *fb)
{
	switch (g_panel_id) {
	case HX8394A_720P_ID:
		printf("LCD: it is hx8394 panel\n");
		break;
	case OTM8018B_ID:
		set_fwvga_info(fb);
		printf("LCD: it is otm8018b panel\n");
		break;
	case OTM1283A_ID:
		set_otm1283a_info(fb);
		printf("LCD: it is otm1283a panel\n");
		break;
	/*TODO: new panel(720p etc) info can be set here*/
	default:
		printf("LCD warning: not hx8394 or otm8018b panel, panel id is 0x%x\n",
				g_panel_id);
		break;
	}
}

void *lcd_init(void)
{
	void *ret;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct pmic *pmic_power;

	pmic_power = pmic_get(board_pmic_chip->power_name);
	if (!pmic_power || pmic_probe(pmic_power)) {
		printf("probe pmic power page error.\n");
		return NULL;
	}

	/* set the ldo11 as 2.8V and enable it*/
	marvell88pm_set_ldo_vol(pmic_power, 11, 2800000);

	udelay(12000);
	turn_off_backlight();
	lcd_power_en(1);

	ret = (void *)mmp_disp_init(fb);
	memcpy(&mmp_mipi_info_saved, fb, sizeof(struct mmp_disp_plat_info));
	turn_on_backlight(ret);
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

static int boot_logo;
void show_logo(void)
{
	char cmd[100];
	char *cmdline;
	int wide, high, root_mode = 0;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct lcd_videomode *fb_mode = fb->modes;
	int xres = fb_mode->xres, yres = fb_mode->yres;

	/* Show marvell logo */
	wide = 330;
	high = 236;

	root_mode = root_detection();
	if (root_mode)
		printf("%s: system is in root mode\n", __func__);

	sprintf(cmd, "bmp display %p %d %d",
		MARVELL_PXA988, (xres - wide) / 2,
		(yres - high) / 2);
	run_command(cmd, 0);
	flush_cache(g_disp_start_addr, g_disp_buf_size);
	boot_logo = 1;
	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	remove_cmdline_param(cmdline, "androidboot.lcd=");
	remove_cmdline_param(cmdline, "fhd_lcd=");

	if (xres == HELAN_720P_FB_XRES) {
		sprintf(cmdline + strlen(cmdline),
			" androidboot.lcd=720p");
	} else
		sprintf(cmdline + strlen(cmdline),
			" androidboot.lcd=fwvga");

	setenv("bootargs", cmdline);
	free(cmdline);
}

void lcd_blank(void)
{
	memset((void *)g_disp_start_addr, 0x0, ctfb.memSize);
}
#endif

#define GPIO10_AP_CM3_SEL 10
/* this i2c bus has been controlled by AP in dts file by default */
void handle_cm3(u8 i2c_bus, u8 addr)
{
	int gpio10_val;
	/*
	 * if GPIO10 high(default state), it's for CM3;
	 * if it's low(need HW ECO), then for AP.
	 */
	gpio_direction_input(GPIO10_AP_CM3_SEL);
	gpio10_val = gpio_get_value(GPIO10_AP_CM3_SEL);
	if (gpio10_val) {
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
	if (fb->panel_manufacturer_id == HX8394A_720P_ID) {
		run_command("fdt rm /otm8018b", 0);
		run_command("fdt rm /otm1283a", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <4>", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/msg2133@26", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/88ms100@720p", 0);
	} else if (fb->panel_manufacturer_id == OTM8018B_ID) {
		run_command("fdt rm /hx8394", 0);
		run_command("fdt rm /otm1283a", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <2>", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/s3202@720p", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/88ms100@720p", 0);
	} else if (fb->panel_manufacturer_id == OTM1283A_ID) {
		run_command("fdt rm /hx8394", 0);
		run_command("fdt rm /otm8018b", 0);
		run_command("fdt set /soc/axi/dsi marvell,dsi-lanes <4>", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/msg2133@26", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/s3202@720p", 0);
	}
#endif

	handle_cm3(SENSOR_HUB_I2C_BUS, CANARY_ADDR);

#if defined(CONFIG_MMP_DISP)
	if (handle_disp_rsvmem(&mmp_mipi_lcd_info))
		printf("Attention, framebuffer maybe not reserved correctly!\n");
#endif
}
#endif
#ifdef CONFIG_MISC_INIT_R

static unsigned char ulc1dkb_recovery_reg_read(void)
{
	u32 value = 0;
	struct pmic *pmic_base;

	/* Get the magic number from RTC register */
	pmic_base = pmic_get(board_pmic_chip->base_name);
	if (!pmic_base || pmic_probe(pmic_base)
	    || pmic_reg_read(pmic_base, 0xef, &value)) {
		printf("read rtc register error\n");
		value = 0;
	}

	return (unsigned char)value;
}

static unsigned char ulc1dkb_recovery_reg_write(unsigned char value)
{
	struct pmic *pmic_base;

	pmic_base = pmic_get(board_pmic_chip->base_name);
	if (!pmic_base || pmic_probe(pmic_base)
	    || pmic_reg_write(pmic_base, 0xef, value))
		printf("write rtc register error\n");

	return 0;
}

static struct recovery_reg_funcs ulc1dkb_recovery_reg_funcs = {
	.recovery_reg_read = ulc1dkb_recovery_reg_read,
	.recovery_reg_write = ulc1dkb_recovery_reg_write,
};


#if 0
static void parse_cp_ddr_range(void)
{
	char *cmdline;
	struct OBM2OSL *obm2osl = (struct OBM2OSL *)
		(*(unsigned long *)CONFIG_OBM_PARAM_ADDR);
	unsigned int start = 0;
	unsigned int size = 0;
	unsigned int exist_cp_prop = 0;

	if (get_cp_ddr_range2(obm2osl, &exist_cp_prop, &start, &size) < 0)
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

	/* set exist cp prop */
	printf("exist cp prop: %u(0x%08x)\n", exist_cp_prop, exist_cp_prop);

	remove_cmdline_param(cmdline, "androidboot.exist.cp=");
	sprintf(cmdline + strlen(cmdline), " androidboot.exist.cp=%d", exist_cp_prop);

	setenv("bootargs", cmdline);
	free(cmdline);
}

#define RF_NAME_LEN 8
#define RF_INFO_LEN 16
#define RF_INFO_FIELDS 4
/*
 * RF Info field in eeprom are as follows:
 * Address 0xE8: rf_name - 8 bytes, ASCII, not null terminated
 * Address 0xF0: rf_info 16 bytes as follows:
 * 0x00 - 0x07: RF_VERSION, ASCII
 * 0x08 - 0x09: RF_GEOGRAPHY, ASCII (EU,NA,CN,LA,JA...)
 * 0x0A - 0x0B: RF_BAND, TBD
 * 0x0C - 0x0F: future use, TBD
*/
/* copy rf name & info, only copy printable chars */
static int strncpy_printable(char *dst, char *src, int n)
{
	int len = 0;

	if (!dst || !src)
		return 0;

	/* *src should be printable character */
	while (isascii(*src) && isprint(*src) && len < n) {
		*dst++ = *src++;
		len++;
	}
	*dst = 0;
	return len;
}

/* the current rf info data in eeprom is "LTE_V15"
 * the RF image name is Skylark_LTG_V15, so the LTE_v15
 * can not pass the verification, so ignore the the
 * prefix "LTE_" */
#define IGNOREKEY "LTE_"

static void set_rf_info_patterns(void)
{
	struct eeprom_data eeprom_data;
	char pattern[RF_NAME_LEN + RF_INFO_LEN * 2] = {0};
	char rf_name[RF_NAME_LEN + 1] = {0};
	char rf_info[RF_INFO_LEN + 1] = {0};
	char temp[RF_INFO_LEN + 1] = {0};
	char *ptemp;

	char *cmdline = NULL;

	int cplen;

	int length[RF_INFO_FIELDS] = {8, 2, 2, 4};
	int offset;
	int i;
	int ret;

	eeprom_data.index = 2;
	eeprom_data.i2c_num = 1;

	/* rf_name */
	ret = eeprom_get_rf_name(&eeprom_data, (u8 *)rf_name);
	if (ret) {
		printf("RF Name: get eeprom data error\n");
		return;
	}
	cplen = strncpy_printable(temp, rf_name, RF_NAME_LEN);
	if (!cplen) {
		printf("RF: RF name is not present in eeprom - skip rf info\n");
		return;
	}

	strcat(pattern, temp);

	/* rf_info */
	ret = eeprom_get_rf_info(&eeprom_data, (u8 *)rf_info);
	if (ret) {
		printf("RF Info: get eeprom data error\n");
		return;
	}
	for (i = 0; i < RF_INFO_FIELDS; i++) {
		offset = (i == 0) ? 0 : (offset + length[i - 1]);
		cplen = strncpy_printable(temp, &rf_info[offset], length[i]);
		ptemp = temp;
		if (!strncasecmp(temp, IGNOREKEY, strlen(IGNOREKEY)))
			ptemp += strlen(IGNOREKEY);
		if (cplen) {
			strcat(pattern, ",");
			strcat(pattern, ptemp);
		}
	}

	printf("RF info pattern: [%s]\n", pattern);

	/* cat pattern to cmdline */
	cmdline = malloc(COMMAND_LINE_SIZE);
	if (!cmdline) {
		printf("RF: malloc cmdline buffer error\n");
		return;
	}
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, "androidboot.rfpattern=");
	sprintf(cmdline + strlen(cmdline), " androidboot.rfpattern=%s", pattern);
	setenv("bootargs", cmdline);
	free(cmdline);
}
#endif

#define EEPROM_BOARD_SN_LEN	32

#define LOW_RAM_DDR_SIZE 0x30000000

/* CONFIG_POWER_OFF_CHARGE=y */
__weak void power_off_charge(u32 *emmd_pmic, u32 lo_uv, u32 hi_uv,
		void (*charging_indication)(bool)) {}
__weak int get_powerup_mode(void) {return INVALID_MODE; }

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
void mmc_decode_cid(struct mmc *card, struct mmc_cid *cid)
{	
	uint *resp = card->cid;

	memset(cid, 0, sizeof(struct mmc_cid));

	/*
	 * SD doesn't currently have a version field so we will
	 * have to assume we can parse this.
	 */
	/* todo: */
	#if 1
	cid->manfid		= UNSTUFF_BITS(resp, 120, 8);
	cid->oemid			= UNSTUFF_BITS(resp, 104, 16);
	cid->prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
	cid->prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
	cid->prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
	cid->prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
	cid->prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
	cid->prod_name[5]		= UNSTUFF_BITS(resp, 56, 8);
	cid->hwrev			= UNSTUFF_BITS(resp, 52, 4);
	cid->fwrev			= UNSTUFF_BITS(resp, 48, 4);
	cid->serial		= UNSTUFF_BITS(resp, 16, 32);
	cid->year			= UNSTUFF_BITS(resp, 12, 4);
	cid->month			= UNSTUFF_BITS(resp, 8, 4);
	cid->year += 1997; /* SD cards year offset */
	#else
	cid->manfid		= UNSTUFF_BITS(resp, 120, 8);
	cid->oemid			= UNSTUFF_BITS(resp, 104, 16);
	cid->prod_name[0]		= UNSTUFF_BITS(resp, 96, 8);
	cid->prod_name[1]		= UNSTUFF_BITS(resp, 88, 8);
	cid->prod_name[2]		= UNSTUFF_BITS(resp, 80, 8);
	cid->prod_name[3]		= UNSTUFF_BITS(resp, 72, 8);
	cid->prod_name[4]		= UNSTUFF_BITS(resp, 64, 8);
	cid->hwrev			= UNSTUFF_BITS(resp, 60, 4);
	cid->fwrev			= UNSTUFF_BITS(resp, 56, 4);
	cid->serial		= UNSTUFF_BITS(resp, 24, 32);
	cid->year			= UNSTUFF_BITS(resp, 12, 8);
	cid->month			= UNSTUFF_BITS(resp, 8, 4);
	cid->year += 2000; /* SD cards year offset */
	#endif

#if 0
	printf("manfid:%x\n", cid->manfid);
	printf("oemid:%x\n", cid->oemid);
	printf("prod_name:%s\n", cid->prod_name);
	printf("hwrev:%x\n", cid->hwrev);
	printf("fwrev:%x\n", cid->fwrev);
	printf("serial:%x\n", cid->serial);
	printf("year:%x\n", cid->year);
	printf("month:%x\n", cid->month);
#endif

	
}

int emmc_get_board_sn_from_cid(char *data)
{
	struct mmc_cid cid;
	
	struct mmc *card = find_mmc_device(CONFIG_FASTBOOT_FLASH_MMC_DEV);
	if (!card ){
		return -1;
	}else{
		mmc_decode_cid(card, &cid);
	}

	sprintf(data, "%08x", cid.serial);

	return 0;
}

#include "persistdata.h"

int misc_init_r(void)
{
	char *cmdline;
	char *s;
	int keypress = 0, i;
	u32 ddr_size = 0;
	u32 stored_ddr_mode = 0;
	struct OBM2OSL *params;
	char *board_sn;
#if 0
	struct eeprom_data eeprom_data;
#endif
	unsigned int rtc_mode, rtc_mode_save;
	int powermode;
	int power_up_mode;

#ifdef CONFIG_RAMDUMP
	u32 base;
	int ramdump;
#endif

	power_up_mode = get_powerup_mode();

	switch (power_up_mode) {
	case BATTERY_MODE:
		/*
		 * handle charge in uboot, the reason we do it here is because
		 * we need to wait GD_FLG_ENV_READY to setenv()
		 */
		power_off_charge(&emmd_page->pmic_power_status, 0, 3400000, NULL);
		break;
	case POWER_SUPPLY_MODE:
		/* continue boot normaly */
		break;
	default:
		printf("invalid power mode! going to power down\n");
		pmic_enter_powerdown(board_pmic_chip);
	}

	if (pxa_is_warm_reset())
		setenv("bootdelay", "-1");

#if defined(CONFIG_MMP_DISP)
	if (!fastboot)
		show_logo();
#endif
	p_recovery_reg_funcs = &ulc1dkb_recovery_reg_funcs;

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	powermode = get_powermode();
	printf("flag of powermode is %d\n", powermode);

	params = (struct OBM2OSL *)(*(unsigned long *)CONFIG_OBM_PARAM_ADDR);
	if (params && params->signature == OBM2OSL_IDENTIFIER)
		keypress = params->booting_mode;

	/* adb reboot product */
	/* enable RTC CLK */
	rtc_mode = *(unsigned int *)(RTC_CLK);
	rtc_mode_save = rtc_mode;
	rtc_mode = (rtc_mode | APBC_APBCLK | APBC_POWER) & (~APBC_RST);
	writel(rtc_mode, RTC_CLK);
	udelay(10);
	rtc_mode = *(unsigned int *)(RTC_BRN0);
	*(unsigned int *)(RTC_BRN0) = 0;
	/* restore */
	writel(rtc_mode_save, RTC_CLK);

	if (rtc_mode == PD_MAGIC_NUM) {
		keypress = PRODUCT_USB_MODE;
		printf("[reboot product]product usb mode %d\n", keypress);
	}

	if (keypress == PRODUCT_UART_MODE)
		powermode = POWER_MODE_DIAG_OVER_UART;
	else if (keypress == PRODUCT_USB_MODE)
		powermode = POWER_MODE_DIAG_OVER_USB;

	switch (powermode) {
	case POWER_MODE_DIAG_OVER_UART:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		/* END key pressed */
		remove_cmdline_param(cmdline, "console=");
		sprintf(cmdline + strlen(cmdline), " androidboot.bsp=2");
#ifdef CONFIG_MMP_DISP
		show_debugmode_logo(&mmp_mipi_lcd_info, PRODUCT_MODE_LOGO_UART);
#endif

		break;

	case POWER_MODE_DIAG_OVER_USB:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		/* OK key pressed */
		sprintf(cmdline + strlen(cmdline),
			" androidboot.console=ttyS1 androidboot.bsp=1");
#ifdef CONFIG_MMP_DISP
		show_debugmode_logo(&mmp_mipi_lcd_info, PRODUCT_MODE_LOGO_USB);
#endif
		break;
	}

	/* set the cma range, we can change it according to
	 * system configuration */
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (gd->bd->bi_dram[i].size == 0)
			break;
		ddr_size += gd->bd->bi_dram[i].size;
	}

	/* set cma size to 20MB */
	remove_cmdline_param(cmdline, "cma=");
	sprintf(cmdline + strlen(cmdline), " cma=20M");

	remove_cmdline_param(cmdline, "cgroup_disable=");
	sprintf(cmdline + strlen(cmdline), " cgroup_disable=memory");

	remove_cmdline_param(cmdline, "androidboot.low_ram=");
	if (ddr_size <= LOW_RAM_DDR_SIZE)
		sprintf(cmdline + strlen(cmdline), " androidboot.low_ram=true");

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
#if 0
	sprintf(cmdline + strlen(cmdline), " ddr_mode=%d", ddr_mode);
#else
	ddr_mode = 2;
	sprintf(cmdline + strlen(cmdline), " ddr_mode=%d", ddr_mode);
#endif

#ifdef CONFIG_RAMDUMP
	base = CONFIG_SYS_TEXT_BASE;
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
	rd_mems[1].start = CONFIG_SYS_RELOC_END;
	rd_mems[1].end = gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS - 1].start +
		gd->bd->bi_dram[CONFIG_NR_DRAM_BANKS - 1].size;
#endif

	/* allocate 1 byte more to store the last '0' */
	board_sn = malloc(EEPROM_BOARD_SN_LEN + 1);
	board_sn[0] = 0;
	char board_wifi_mac[18] = {0};
	char board_bluetooth_mac[18] = {0};
	char board_zigbee_mac[24] = {0};
	#if 0
	eeprom_data.index = 2;
	eeprom_data.i2c_num = 1;
	eeprom_get_board_sn(&eeprom_data, (u8 *)board_sn);
	#else

#ifdef CONFIG_PERSIST_DATA
	if(emmc_get_persist_data() == 0){

		PDATA_BLOCK_t *pdb;
		if(mem_get_persist_data_block(PD_TYPE_SN, &pdb)){
			strncpy(board_sn, pdb->data, MIN(pdb->length, EEPROM_BOARD_SN_LEN));
		}else{
			emmc_get_board_sn_from_cid(board_sn);
		}

		if(mem_get_persist_data_block(PD_TYPE_WIFIMAC, &pdb)){
			strncpy(board_wifi_mac, pdb->data, MIN(pdb->length, 17));

			remove_cmdline_param(cmdline, "androidboot.wifi.mac=");
			sprintf(cmdline + strlen(cmdline),
				" androidboot.wifi.mac=%s", board_wifi_mac);
		}

		if(mem_get_persist_data_block(PD_TYPE_BTMAC, &pdb)){
			strncpy(board_bluetooth_mac, pdb->data, MIN(pdb->length, 17));

			remove_cmdline_param(cmdline, "androidboot.bluetooth.mac=");
			sprintf(cmdline + strlen(cmdline),
				" androidboot.bluetooth.mac=%s", board_bluetooth_mac);
		}

		if(mem_get_persist_data_block(PD_TYPE_ZIGBMAC, &pdb)){
			strncpy(board_zigbee_mac, pdb->data, MIN(pdb->length, 23));

			remove_cmdline_param(cmdline, "androidboot.zigbee.mac=");
			sprintf(cmdline + strlen(cmdline),
				" androidboot.zigbee.mac=%s", board_zigbee_mac);
		}
	}else{
		emmc_get_board_sn_from_cid(board_sn);
		printf("Force fastboot mode.\n");
		run_command("fb", 0);
		run_command("reset", 0);
	}
#endif

	#endif

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
			show_debugmode_logo(&mmp_mipi_lcd_info, RAMDUMP_SD_LOGO);
			if (run_command("ramdump 0 0 1 2", 0)) {
				printf("SD dump fail, enter fastboot mode\n");
				show_debugmode_logo(&mmp_mipi_lcd_info, RAMDUMP_USB_LOGO);
				run_command("fb", 0);
			} else {
				run_command("reset", 0);
			}
#endif
		} else {
			printf("ready to enter fastboot mode\n");
			show_debugmode_logo(&mmp_mipi_lcd_info, RAMDUMP_USB_LOGO);
			run_command("fb", 0);
		}
	}
#endif

	/* adb reboot fastboot */
	if (rtc_mode == FB_MAGIC_NUM) {
		printf("[reboot fastboot]fastboot mode\n");
		run_command("fb", 0);
	}

	/* adb reboot bootloader */
	if (rtc_mode == FB_MAGIC_NUM_ALT) {
		printf("[reboot bootloader]fastboot mode\n");
		run_command("fb", 0);
	}

        /* fastboot reboot bootloader */
        char *s1 = getenv("reboot-bootloader");
        if(s1 != NULL)
        {
               if(!(strcmp(s1, "yes")))
               {
                   setenv("reboot-bootloader", "no");
                   saveenv();
                   run_command("fb", 0);
               }
        }

	/* get CP ddr range */
	//parse_cp_ddr_range();

	/* set rf patterns for dynamic rf config */
	//set_rf_info_patterns();

	return 0;

}
#endif /* CONFIG_MISC_INIT_R */

#ifdef CONFIG_POWER_88PM886
void board_pmic_power_fixup(struct pmic *p_power)
{
	u32 val;

	/* set buck1 8 DVC levels as 1.25v */
	val = 0x34;
	pmic_update_bits(p_power, 0xa5, 0x7f, val);
	pmic_update_bits(p_power, 0xa6, 0x7f, val);
	pmic_update_bits(p_power, 0xa7, 0x7f, val);
	pmic_update_bits(p_power, 0xa8, 0x7f, val);
	val |= 0x80;
	pmic_update_bits(p_power, 0x9a, 0xff, val);
	pmic_update_bits(p_power, 0x9b, 0xff, val);
	pmic_update_bits(p_power, 0x9c, 0xff, val);
	pmic_update_bits(p_power, 0x9d, 0xff, val);

	val = 0x0f;
	pmic_update_bits(p_power, 0xa1, 0x0f, val);

}
#endif

/* used to do some board related config */
static int board_charger_config(void)
{
	u32 data;
	struct pmic *p_gpadc;
	p_gpadc = pmic_get("88pm88x_gpadc");
	if (!p_gpadc || pmic_probe(p_gpadc)) {
		printf("%s: Can't get pmic - battery\n", __func__);
		return -1;
	}

	/*
	 * TODO: since we already enable GPADC3 in battery driver, so we don't
	 * need to enable it again here, but if we need to use GPADC1 as battery
	 * detection pin, please enable GPADC1 here.
	 */

	/* use GPADC3 as battery detection pin and enable bias current */
	data = PM886_BD_GP_SEL_3 | PM886_BD_PREBIAS;
	pmic_update_bits(p_gpadc, PM886_GPADC_CONFIG8, data, data);
	return 0;
}

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

	board_charger_config();

	return 0;
}

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

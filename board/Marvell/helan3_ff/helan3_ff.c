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
#include <eeprom_34xx02.h>
#ifdef CONFIG_GENERIC_MMC
#include <sdhci.h>
#include <mv_sdhci.h>
#endif
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <power/88pm880.h>
#include <power/pxa988_freq.h>
#include <power/mmp_idle.h>
#include <emmd_rsv.h>
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

#include "../common/obm2osl.h"
#include "../common/cmdline.h"
#include <mv_recovery.h>
#include "../common/mrd_flag.h"
#include "../common/mv_cp.h"

#include <leds-lm3532.h>

/* dvc control register bit make DVC can support 8 level voltages */
#define DVC_SET_ADDR1	(1 << 0)
#define DVC_SET_ADDR2	(1 << 1)
#define DVC_CTRl_LVL	2

#define PMIC_I2C_BUS 2
#define CHG_I2C_BUS 2
#define FG_I2C_BUS 2

#define APBC_RTC_CLK_RST	(0xD4015028)
#define RTC_BRN0		(0xD4010014)
#define FB_MAGIC_NUM		('f'<<24 | 'a'<<16 | 's'<<8 | 't')
#define PD_MAGIC_NUM		('p'<<24 | 'r'<<16 | 'o'<<8 | 'd')

#define BOARD_ECO_LEN						0x10
/* define the index to check that light sensor exist and is valid */
#define BOARD_ECO_LIGHT_SENSOR_VALID_INDEX	0
#define BOARD_ECO_LIGHT_SENSOR_INVALID_VAL	'P'

struct pmic_chip_desc *board_pmic_chip;
u8 board_eco_data[BOARD_ECO_LEN] = {0xff};

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
static u8 ddr_mode;

/* 1 - 533Mhz, 2 -- 667Mhz */
static void helan3_get_ddr_mode(void)
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

int board_early_init_f(void)
{
	u32 mfp_cfg[] = {
		/* Keypad */
		GPIO016_KP_DKIN0,
		GPIO017_KP_DKIN1,

		/* AP UART */
		GPIO047_UART2_RXD,
		GPIO048_UART2_TXD,

		/* LCD Backlight */
		GPIO032_GPIO_32,
		GPIO079_CI2C_SCL,
		GPIO080_CI2C_SDA,

		/* LCD Reset */
		GPIO098_GPIO_98,

		/* 5V_BOOST_EN */
		GPIO096_GPIO_96,

		/*
		 * enable sensor hub
		 * in order to enable as regular I2C use instead:
		 * GPIO073_CI2C_SCL_3
		 * GPIO074_CI2C_SDA_3
		 */
		/* to enable sensors using sensor hub use insted:
		* GPIO073_GPS_I2C_SCL,
		* GPIO074_GPS_I2C_SDA,
		*/
		GPIO073_CI2C_SCL_3,
		GPIO074_CI2C_SDA_3,
		/* I2C */
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
	printf("helan3 board rebooting...\n");
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
		if (mv_sdh_init(mmc_base_address[i], 104*1000000, 0,
				SDHCI_QUIRK_32BIT_DMA_ADDR))
			return 1;
	}

	return 0;
}
#endif

int board_init(void)
{
	int ret;
	emmd_page = (struct emmd_page *)CRASH_BASE_ADDR;
	/* arch number of Board */
	gd->bd->bi_arch_number = MACH_TYPE_HELAN2_DKB;
	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x9a00000;

	helan3_get_ddr_mode();

#ifdef CONFIG_SQU_DEBUG_SUPPORT
	show_squdbg_regval();
#endif
	if (emmd_page->indicator == 0x454d4d44) {
		/* clear this indicator for prevent the re-entry */
		printf("Detect RAMDUMP signature!!\n");
		emmd_page->indicator = 0;
		flush_cache((unsigned long)(&emmd_page->indicator),
			    (unsigned long)sizeof(emmd_page->indicator));

		ret = ramdump_attach_pbuffer("powerud", (uintptr_t)emmd_page->pmic_power_reason,
					sizeof(emmd_page->pmic_power_reason));
		printf("add powerud to ramdump - %s\n", ret ? "fail" : "success");

		fastboot = 1;

#ifdef CONFIG_SQU_DEBUG_SUPPORT
		save_squdbg_regval2ddr();
#endif
	}

	return 0;
}

#if defined(CONFIG_MMP_DISP)
static struct mmp_disp_plat_info mmp_mipi_info_saved;
static struct dsi_info helan3_dsi = {
	.id = 1,
	.lanes = 4,
	.bpp = 24,
	.burst_mode = DSI_BURST_MODE_BURST,
	.rgb_mode = DSI_LCD_INPUT_DATA_RGB_MODE_888,
	.hbp_en = 1,
	.master_mode = 1,
};

#define HELAN_1080P_FB_XRES		1080
#define HELAN_1080P_FB_YRES		1920

static struct lcd_videomode video_1080p_modes[] = {
	[0] = {
		/* we should align the follwoing settings with kernel driver */
		.refresh = 57,
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

static void update_panel_info(struct mmp_disp_plat_info *fb);
static struct mmp_disp_plat_info mmp_mipi_lcd_info = {
	.index = 0,
	.id = "GFX Layer",
	/* FIXME */
	.sclk_src = 832000000,
	.sclk_div = 0x00000104,
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
	.phy_info = &helan3_dsi,
	.dsi_panel_config = panel_1080p_init_config,
	.dynamic_detect_panel = 1,
	.update_panel_info = update_panel_info,
	.calculate_sclk = lcd_calculate_sclk,
	.panel_manufacturer_id = SHARP_1080P_ID,
};

#define LCD_RST_GPIO1	98
#define BOOST_EN_5V		96

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

static void turn_off_backlight(void)
{
	gpio_direction_input(LCD_PWM_GPIO32);
}

struct lm3532_data lm3532_pdata = {
	.addr = LM3532_I2C_ADDR,
	.i2c_bus_num = LM3532_I2C_NUM,
	.ramp_time = 0, /* Ramp time in milliseconds */
	.ctrl_a_fs_current = LM3532_19p4mA_FS_CURRENT,
	.ctrl_a_mapping_mode = LM3532_LINEAR_MAPPING,
	.ctrl_a_pwm = 0x6,
	.output_cfg_val = 0x0,
	.feedback_en_val = 0x3,
};

static void turn_on_backlight(void *fbi)
{
	gpio_direction_output(LCD_BACKLIGHT_EN, 1);
	lm3532_init(&lm3532_pdata);
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
	if (g_panel_id == fb->panel_manufacturer_id) {
		printf("LCD: it is the default panel setting in uboot, the panel id is 0x%x\n",
			g_panel_id);
		return;
	}

	switch (g_panel_id) {
	case SHARP_1080P_ID:
		set_1080p_info(fb);
		printf("LCD: it is sharp1080P panel\n");
		break;
	default:
		printf("LCD warning: not hx8394 or sharp1080P panel, panel id is 0x%x\n",
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

	/* set the ldo5 as 1.8V */
	pmic_update_bits(pmic_power, 0x38, 0x0f, 0x03);
	/* enable ldo5 */
	pmic_update_bits(pmic_power, 0x09, 1 << 4, 1 << 4);

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
	int wide, high;
	struct mmp_disp_plat_info *fb = &mmp_mipi_lcd_info;
	struct lcd_videomode *fb_mode = fb->modes;
	int xres = fb_mode->xres, yres = fb_mode->yres;

	/* Show marvell logo */
	wide = 330;
	high = 236;
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

	if (xres == HELAN_1080P_FB_XRES) {
		sprintf(cmdline + strlen(cmdline),
			" fhd_lcd=1 androidboot.lcd=1080p");
	}

	setenv("bootargs", cmdline);
	free(cmdline);
}

void lcd_blank(void)
{
	memset((void *)g_disp_start_addr, 0x0, ctfb.memSize);
}
#endif

/* this i2c bus has been controlled by AP in dts file by default */
void handle_cm3(u8 i2c_bus, u8 addr)
{
#ifdef USE_CM3_SENSORS
	/* USE AP path sensors */
	/* delete this i2c bus node */
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800", 0);
	/* delete the apsenhb node */
	run_command("fdt rm /apsenhb", 0);
	printf("use cm3...\n");
#else
	/* delete the cm3senhb node */
	run_command("fdt rm /cm3senhb", 0);
	printf("not use cm3...\n");
#endif
}

/* attention for the bus number and the checked i2c addr, it's changable */
#define SENSOR_HUB_I2C_BUS 3
#define CANARY_ADDR 0x69

#ifdef CONFIG_OF_LIBFDT
void handle_dtb(struct fdt_header *devtree)
{
	char cmd[128];

	/* set dtb addr */
	sprintf(cmd, "fdt addr 0x%p", devtree);
	run_command(cmd, 0);

#if defined(CONFIG_MMP_DISP)
	/*
	 * detele unused panel & touch device node
	 * in helan3_ff we use only 1080p
	 */
	run_command("fdt rm /lg4591", 0);
	run_command("fdt rm /hx8394", 0);
	run_command("fdt rm /soc/apb@d4000000/i2c@d4010800/s3202@720p", 0);
#endif

	/* Check if light sensor should be removed  */
	if (BOARD_ECO_LIGHT_SENSOR_INVALID_VAL ==
			board_eco_data[BOARD_ECO_LIGHT_SENSOR_VALID_INDEX]) {
		run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/apds990x@39", 0);
		printf("APDS990x light sensor disabled\n");
	}

	/* remove unused sensors on ff */
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/apds9930@39", 0);
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/bmi160@68", 0);
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/bme280@76", 0);
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/icm20628@69", 0);
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/mxc400x@15", 0);
	run_command("fdt rm /soc/apb@d4000000/i2c@d4013800/mmc3524@30", 0);

	handle_cm3(SENSOR_HUB_I2C_BUS, CANARY_ADDR);

	/* remove common vibrator */
	run_command("fdt rm /vibrator", 0);

#if defined(CONFIG_MMP_DISP)
	if (handle_disp_rsvmem(&mmp_mipi_lcd_info))
		printf("Attention, framebuffer maybe not reserved correctly!\n");
#endif
}
#endif
#ifdef CONFIG_MISC_INIT_R

static unsigned char helan3ff_recovery_reg_read(void)
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

static unsigned char helan3ff_recovery_reg_write(unsigned char value)
{
	struct pmic *pmic_base;

	pmic_base = pmic_get(board_pmic_chip->base_name);
	if (!pmic_base || pmic_probe(pmic_base)
	    || pmic_reg_write(pmic_base, 0xef, value))
		printf("write rtc register error\n");

	return 0;
}

static struct recovery_reg_funcs helan3ff_recovery_reg_funcs = {
	.recovery_reg_read = helan3ff_recovery_reg_read,
	.recovery_reg_write = helan3ff_recovery_reg_write,
};

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

#define EEPROM_BOARD_SN_LEN	32

#define LOW_RAM_DDR_SIZE 0x30000000

static void charging_indication(bool on)
{
	u32 mask;
	struct pmic *p_base;

	p_base = pmic_get(board_pmic_chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("probe pmic base page error.\n");
		return;
	}

	if (on) {
		/* turn off backlight to save power */
		turn_off_backlight();
		/* enable RGB clock */
		pmic_update_bits(p_base, PM880_REG_CLK_CTRL_1,
				 PM880_RGB_CLK_EN, PM880_RGB_CLK_EN);
		/* blink red led to indicate charging to the user */
		mask = LED_EN | R_LED_EN | LED_BLK_MODE;
		pmic_update_bits(p_base, LED_PWM_CONTROL7, mask, mask);
	} else {
		/* disable RGB led */
		mask = LED_EN | R_LED_EN | LED_BLK_MODE;
		pmic_update_bits(p_base, LED_PWM_CONTROL7, mask, 0);
		/* disable RGB clock */
		pmic_update_bits(p_base, PM880_REG_CLK_CTRL_1,
				 PM880_RGB_CLK_EN, 0);
		/* restore backlight */
		turn_on_backlight(NULL);
	}
}

/* CONFIG_POWER_OFF_CHARGE=y */
__weak void power_off_charge(u32 *emmd_pmic, u32 lo_uv, u32 hi_uv,
		void (*charging_indication)(bool)) {}
int misc_init_r(void)
{
	char *cmdline;
	char *s;
	int keypress = 0, i;
	u32 ddr_size = 0;
	u32 stored_ddr_mode = 0;
	struct OBM2OSL *params;
	char *board_sn;
	struct eeprom_data eeprom_data;
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
		power_off_charge(&emmd_page->pmic_power_status, 0, 3400000, charging_indication);
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
	p_recovery_reg_funcs = &helan3ff_recovery_reg_funcs;

	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);

	powermode = get_powermode();
	printf("flag of powermode is %d\n", powermode);

	params = (struct OBM2OSL *)(*(unsigned long *)CONFIG_OBM_PARAM_ADDR);
	if (params && params->signature == OBM2OSL_IDENTIFIER)
		keypress = params->booting_mode;

	/* adb reboot product */
	/* enable RTC CLK */
	rtc_mode = *(unsigned int *)(APBC_RTC_CLK_RST);
	rtc_mode_save = rtc_mode;
	rtc_mode = (rtc_mode | APBC_APBCLK | PM_POWER_SENSOR_BIT) & (~APBC_RST);
	writel(rtc_mode, APBC_RTC_CLK_RST);
	udelay(10);
	rtc_mode = *(unsigned int *)(RTC_BRN0);

	if (rtc_mode == PD_MAGIC_NUM) {
		*(volatile unsigned int *)(RTC_BRN0) = 0;
		keypress = params->booting_mode = PRODUCT_USB_MODE;
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
		show_debugmode_logo(&mmp_mipi_lcd_info, PRODUCT_MODE_LOGO_UART);
		break;
	case POWER_MODE_DIAG_OVER_USB:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		/* OK key pressed */
		sprintf(cmdline + strlen(cmdline),
			" androidboot.console=ttyS1 androidboot.bsp=1");
		show_debugmode_logo(&mmp_mipi_lcd_info, PRODUCT_MODE_LOGO_USB);
		break;
	}

	/* set the cma range, we can change it according to
	 * system configuration */
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (gd->bd->bi_dram[i].size == 0)
			break;
		ddr_size += gd->bd->bi_dram[i].size;
	}

	remove_cmdline_param(cmdline, "cgroup_disable=");
	sprintf(cmdline + strlen(cmdline), " cgroup_disable=memory");

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

	sprintf(cmdline + strlen(cmdline), " resolution.lcd=720x1280");
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
	eeprom_data.index = 2;
	eeprom_data.i2c_num = 0;
	eeprom_get_board_sn(&eeprom_data, (u8 *)board_sn);
	eeprom_get_board_eco(&eeprom_data, (u8 *)board_eco_data);


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

#ifdef CONFIG_HELAN3_POWER
	/* increase freq before emmd dump */
	helan3_fc_init(ddr_mode);
	setop(832, 1248, 624, 208);
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
		*(volatile unsigned int *)(RTC_BRN0) = 0;
		printf("[reboot fastboot]fastboot mode");
		run_command("fb", 0);
	}
	writel(rtc_mode_save, APBC_RTC_CLK_RST);	/* restore  RTC_clock state */

	/* get CP ddr range */
	parse_cp_ddr_range();

	return 0;

}
#endif /* CONFIG_MISC_INIT_R */

void board_pmic_base_fixup(struct pmic *p_base)
{
	/* set dvc4 and dvc5 to input since they are not connected */
	pmic_update_bits(p_base, PM880_REG_GPIO_CTRL4, 0xF0, 0x0);
	pmic_update_bits(p_base, PM880_REG_GPIO_CTRL5, 0xF, 0x0);
}


void board_pmic_buck_fixup(struct pmic *p_buck)
{
	unsigned int addr, lvl;

	/* enable buck1 dual phase mode */
	pmic_update_bits(p_buck, PM880_BUCK1_DUAL,
				PM880_BK1A_DUAL_SEL, PM880_BK1A_DUAL_SEL);

	/* set buck1 all the DVC level voltages at 1.25V */
	addr = PM880_ID_BUCK1;
	for (lvl = 0; lvl < 16; lvl++) {
		pmic_update_bits(p_buck, addr + lvl, 0x7f, 0x34);
		pmic_update_bits(p_buck, addr + lvl + 0x18, 0x7f, 0x34);
	}

	/* set buck2 to 1.8v and enable it */
	marvell88pm_set_buck_vol(p_buck, 2, 1800000);

	/* set buck7 all the DVC level voltage at 2.1V */
	addr = PM880_ID_BUCK7;
	for (lvl = 0; lvl < 4; lvl++)
		pmic_update_bits(p_buck, addr + lvl, 0x7f, 0x78);
}

void board_pmic_ldo_fixup(struct pmic *p_ldo)
{
	/* set ldo1(USB) to 3.1v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 1, 3100000);
	/* set ldo3(GPS) to 1.8v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 3, 1800000);
	/* set ldo9(RF) to 2.8v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 9, 2800000);
	/* set ldo10(SDIO) to 3v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 10, 3000000);
	/* set ldo11(Vibrator) to 2.8v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 11, 2800000);
	/* set ldo14(SD) to 3.1v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 14, 3100000);
	/* set ldo15(EMMC) to 3v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 15, 3000000);
	/* set ldo16(sensors) to 2.8v and enable it */
	marvell88pm_set_ldo_vol(p_ldo, 16, 2800000);
}

/* TODO: do some board related config */
static int board_charger_config(void)
{
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

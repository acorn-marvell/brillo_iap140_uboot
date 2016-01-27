/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <asm/gpio.h>
#include    "panel.h"
#include    "tft_1080p_cmd.h"

static u8 exit_sleep[] = {0x11};
static u8 display_on[] = {0x29};
static u8 sleep_in[] = {0x10};
static u8 display_off[] = {0x28};

#define NT35521_SLEEP_OUT_DELAY 200
#define NT35521_SLEEP_IN_DELAY 200
#define NT35521_DISP_ON_DELAY	0
#define NT35521_DISP_OFF_DELAY	0

static struct dsi_cmd_desc video_display_off_cmds[] = {
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(display_off),
		display_off},
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(sleep_in),
		sleep_in},
};

void panel_720p_nt35521_exit_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, video_display_off_cmds, ARRAY_SIZE(video_display_off_cmds));
}

#define NT35565_SLEEP_OUT_DELAY 200
#define NT35565_SLEEP_IN_DELAY 200
#define NT35565_DISP_ON_DELAY	0
#define NT35565_DISP_OFF_DELAY	0

static struct dsi_cmd_desc nt35565_video_display_on_cmds[] = {
	{DSI_DI_DCS_SWRITE, 1, NT35565_SLEEP_OUT_DELAY, sizeof(exit_sleep),
		exit_sleep},
	{DSI_DI_DCS_SWRITE, 1, NT35565_DISP_ON_DELAY, sizeof(display_on),
		display_on},
};

void panel_qhd_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, nt35565_video_display_on_cmds,
			 ARRAY_SIZE(nt35565_video_display_on_cmds));
}

void panel_1080p_tft_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, tft1080p_video_display_on_cmds,
			ARRAY_SIZE(tft1080p_video_display_on_cmds));
}

void panel_exit_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, video_display_off_cmds,
			 ARRAY_SIZE(video_display_off_cmds));
}

static u8 manufacturer_cmd_access_protect[] = {0xB0, 0x04};
static u8 backlight_ctrl[] = {0xCE, 0x00, 0x01, 0x88, 0xC1, 0x00, 0x1E, 0x04};
static u8 nop[] = {0x0};
static u8 seq_test_ctrl[] = {0xD6, 0x01};
static u8 write_display_brightness[] = {0x51, 0x0F, 0xFF};
static u8 write_ctrl_display[] = {0x53, 0x24};

static struct dsi_cmd_desc r63311_video_display_on_cmds[] = {
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(manufacturer_cmd_access_protect),
		manufacturer_cmd_access_protect},
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(nop),
		nop},
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(nop),
		nop},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(backlight_ctrl),
		backlight_ctrl},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(seq_test_ctrl),
		seq_test_ctrl},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(write_display_brightness),
		write_display_brightness},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(write_ctrl_display),
		write_ctrl_display},
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(display_on),
		display_on},
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(exit_sleep),
		exit_sleep},
};


void panel_1080p_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, r63311_video_display_on_cmds,
			 ARRAY_SIZE(r63311_video_display_on_cmds));
}

#define OTM1281_SLEEP_OUT_DELAY 100
#define OTM1281_DISP_ON_DELAY   100

static u8 reg_shift_00[] = {0x0, 0x0};
static u8 enable_cmd2[] = {0xff, 0x12, 0x80, 0x1};
static u8 reg_shift_80[] = {0x0, 0x80};
static u8 reg_shift_81[] = {0x0, 0x81};
static u8 reg_shift_82[] = {0x0, 0x82};
static u8 reg_shift_b0[] = {0x0, 0xb0};
/*
 * increase frame rate to 70 Hz
 * it should be larger than host output(60Hz)
 */
static u8 reg_osc_adj[] = {0xc1, 0x9};
static u8 enable_orise[] = {0xff, 0x12, 0x80};
static u8 reg_shift_b8[] = {0x0, 0xb8};
static u8 f5_set[] = {0xf5, 0x0c, 0x12};
static u8 reg_shift_90[] = {0x0, 0x90};
static u8 pwr_ctrl3[] = {0xc5, 0x10, 0x6F, 0x02, 0x88, 0x1D, 0x15, 0x00,
	0x04};
static u8 reg_shift_a0[] = {0x0, 0xa0};
static u8 pwr_ctrl4[] = {0xc5, 0x10, 0x6F, 0x02, 0x88, 0x1D, 0x15, 0x00,
	0x04};
static u8 pwr_ctrl12[] = {0xc5, 0x20, 0x01, 0x00, 0xb0, 0xb0, 0x00, 0x04,
	0x00};
static u8 pwr_vdd[] = {0xd8, 0x58, 0x00, 0x58, 0x00};
static u8 reg_test[] = {0xb0, 0x20};
static u8 set_vcomdc[] = {0xd9, 0x94};
static u8 disable_orise[] = {0xff, 0x00, 0x00};
static u8 disable_cmd2[] = {0xff, 0x00, 0x00, 0x00};
#define SWRESET_DELAY   120
static u8 swreset[] = {0x1};

static struct dsi_cmd_desc otm1281_video_display_on_cmds[] = {
	{DSI_DI_DCS_SWRITE, 1, SWRESET_DELAY, sizeof(swreset), swreset},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(enable_cmd2), enable_cmd2},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_80), reg_shift_80},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(enable_orise), enable_orise},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_81), reg_shift_81},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(reg_osc_adj), reg_osc_adj},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_82), reg_shift_82},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(reg_osc_adj), reg_osc_adj},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_b8), reg_shift_b8},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(f5_set), f5_set},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_b0), reg_shift_b0},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(reg_test), reg_test},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_90), reg_shift_90},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pwr_ctrl3), pwr_ctrl3},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_a0), reg_shift_a0},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pwr_ctrl4), pwr_ctrl4},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_80), reg_shift_80},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pwr_ctrl12), pwr_ctrl12},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_00), reg_shift_00},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pwr_vdd), pwr_vdd},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_00), reg_shift_00},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(set_vcomdc), set_vcomdc},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_80), reg_shift_80},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(disable_orise), disable_orise},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(reg_shift_80), reg_shift_80},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(disable_cmd2), disable_cmd2},

	{DSI_DI_DCS_SWRITE, 1, OTM1281_SLEEP_OUT_DELAY, sizeof(exit_sleep),
		exit_sleep},
	{DSI_DI_DCS_SWRITE, 1, OTM1281_DISP_ON_DELAY, sizeof(display_on),
		display_on},
};

void panel_720p_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, otm1281_video_display_on_cmds,
			 ARRAY_SIZE(otm1281_video_display_on_cmds));
}

static u8 dsi_config[] = {0xe0, 0x43, 0x00, 0x00, 0x00, 0x00};
static u8 dsi_ctr1[] = {0xB5, 0x34, 0x20, 0x40, 0x00, 0x20};
static u8 dsi_ctr2[] = {0xB6, 0x04, 0x74, 0x0f, 0x16, 0x13};
static u8 osc_setting[] = {0xc0, 0x01, 0x08};
static u8 power_ctr1[] = {0xc1, 0x00};
static u8 power_ctr3[] = {0xc3, 0x00, 0x09, 0x10, 0x02,
	0x00, 0x66, 0x20, 0x33, 0x00};
static u8 power_ctr4[] = {0xc4, 0x23, 0x24, 0x17, 0x17, 0x59};
static u8 pos_gamma_red[] = {0xd0, 0x21, 0x13, 0x67, 0x37, 0x0c,
	0x06, 0x62, 0x23, 0x03};
static u8 neg_gamma_red[] = {0xd1, 0x32, 0x13, 0x66, 0x37, 0x02,
	0x06, 0x62, 0x23, 0x03};
static u8 pos_gamma_green[] = {0xd2, 0x41, 0x14, 0x56, 0x37, 0x0c,
	0x06, 0x62, 0x23, 0x03};
static u8 neg_gamma_green[] = {0xd3, 0x52, 0x14, 0x55, 0x37, 0x02,
	0x06, 0x62, 0x23, 0x03};
static u8 pos_gamma_blue[] = {0xd4, 0x41, 0x14, 0x56, 0x37, 0x0c,
	0x06, 0x62, 0x23, 0x03};
static u8 neg_gamma_blue[] = {0xd5, 0x52, 0x14, 0x55, 0x37, 0x02,
	0x06, 0x62, 0x23, 0x03};
static u8 set_addr_mode[] = {0x36, 0x08};
static u8 opt2[] = {0xf9, 0x00};
static u8 power_ctr2_1[] = {0xc2, 0x02};
static u8 power_ctr2_2[] = {0xc2, 0x06};
static u8 power_ctr2_3[] = {0xc2, 0x4e};
static char brightness_ctrl[] = {0x51, 0x00};
static char display_ctrl[] = {0x53, 0x24};
static char turn_on_backlight[] = {0x51, 0x2d};

static struct dsi_cmd_desc lg720p_video_display_on_cmds[] = {
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(dsi_config), dsi_config},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(dsi_ctr1), dsi_ctr1},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(dsi_ctr2), dsi_ctr2},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(osc_setting), osc_setting},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(power_ctr1), power_ctr1},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(power_ctr3), power_ctr3},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(power_ctr4), power_ctr4},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pos_gamma_red), pos_gamma_red},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(neg_gamma_red), neg_gamma_red},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pos_gamma_green), pos_gamma_green},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(neg_gamma_green), neg_gamma_green},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(pos_gamma_blue), pos_gamma_blue},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(neg_gamma_blue), neg_gamma_blue},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(set_addr_mode), set_addr_mode},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(opt2), opt2},
	{DSI_DI_GENERIC_LWRITE, 1, 10, sizeof(power_ctr2_1), power_ctr2_1},
	{DSI_DI_GENERIC_LWRITE, 1, 10, sizeof(power_ctr2_2), power_ctr2_2},
	{DSI_DI_GENERIC_LWRITE, 1, 80, sizeof(power_ctr2_3), power_ctr2_3},
	{DSI_DI_DCS_SWRITE, 1, 10, sizeof(exit_sleep), exit_sleep},
	{DSI_DI_GENERIC_LWRITE, 1, 10, sizeof(opt2), opt2},
	{DSI_DI_DCS_SWRITE, 1, 0, sizeof(display_on), display_on},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(display_ctrl), (u8 *)display_ctrl},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(brightness_ctrl),
		(u8 *)brightness_ctrl},
};

void lg_720p_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, lg720p_video_display_on_cmds,
			 ARRAY_SIZE(lg720p_video_display_on_cmds));
}

static u8 set_password[] = {0xb9, 0xff, 0x83, 0x94};
static u8 set_lane[] = {0xba, 0x13};
static u8 set_power[] = {0xb1, 0x01, 0x00, 0x07, 0x87, 0x01, 0x11, 0x11, 0x2a, 0x30, 0x3f,
	0x3f, 0x47, 0x12, 0x01, 0xe6, 0xe2};
static u8 set_cyc[] = {0xb4, 0x80, 0x06, 0x32, 0x10, 0x03, 0x32, 0x15, 0x08, 0x32, 0x10, 0x08,
	0x33, 0x04, 0x43, 0x05, 0x37, 0x04, 0x3F, 0x06, 0x61, 0x61, 0x06};
static u8 set_rtl_display_reg[] = {0xb2, 0x00, 0xc8, 0x08, 0x04, 0x00, 0x22};
static u8 set_gip[] = {0xD5, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0xCC, 0x00, 0x00,
	0x00, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x01, 0x67, 0x45, 0x23,
	0x01, 0x23, 0x88, 0x88, 0x88, 0x88};
static u8 set_tcon[] = {0xC7, 0x00, 0x10, 0x00, 0x10};
static u8 set_init0[] = {0xBF, 0x06, 0x00, 0x10, 0x04};
static u8 set_panel[] = {0xCC, 0x09};
static u8 set_gamma[] = {0xE0, 0x00, 0x04, 0x06, 0x2B, 0x33, 0x3F, 0x11, 0x34, 0x0A, 0x0E,
	0x0D, 0x11, 0x13, 0x11, 0x13, 0x10, 0x17, 0x00, 0x04, 0x06, 0x2B, 0x33, 0x3F, 0x11, 0x34,
	0x0A, 0x0E, 0x0D, 0x11, 0x13, 0x11, 0x13, 0x10, 0x17, 0x0B, 0x17, 0x07, 0x11, 0x0B, 0x17,
	0x07, 0x11};
static u8 set_dgc[] = {0xC1, 0x01, 0x00, 0x07, 0x0E, 0x15, 0x1D, 0x25, 0x2D, 0x34, 0x3C, 0x42,
	0x49, 0x51, 0x58, 0x5F, 0x67, 0x6F, 0x77, 0x80, 0x87, 0x8F, 0x98, 0x9F, 0xA7, 0xAF, 0xB7,
	0xC1, 0xCB, 0xD3, 0xDD, 0xE6, 0xEF, 0xF6, 0xFF, 0x16, 0x25, 0x7C, 0x62, 0xCA, 0x3A, 0xC2,
	0x1F, 0xC0, 0x00, 0x07, 0x0E, 0x15, 0x1D, 0x25, 0x2D, 0x34, 0x3C, 0x42, 0x49, 0x51, 0x58,
	0x5F, 0x67, 0x6F, 0x77, 0x80, 0x87, 0x8F, 0x98, 0x9F, 0xA7, 0xAF, 0xB7, 0xC1, 0xCB, 0xD3,
	0xDD, 0xE6, 0xEF, 0xF6, 0xFF, 0x16, 0x25, 0x7C, 0x62, 0xCA, 0x3A, 0xC2, 0x1F, 0xC0, 0x00,
	0x07, 0x0E, 0x15, 0x1D, 0x25, 0x2D, 0x34, 0x3C, 0x42, 0x49, 0x51, 0x58, 0x5F, 0x67, 0x6F,
	0x77, 0x80, 0x87, 0x8F, 0x98, 0x9F, 0xA7, 0xAF, 0xB7, 0xC1, 0xCB, 0xD3, 0xDD, 0xE6, 0xEF,
	0xF6, 0xFF, 0x16, 0x25, 0x7C, 0x62, 0xCA, 0x3A, 0xC2, 0x1F, 0xC0};
static u8 set_vcom[] = {0xB6, 0x0C};
static u8 set_init1[] = {0xD4, 0x32};
static u8 set_cabc[] = {0xC9, 0x0F, 0x2, 0x1E, 0x1E, 0x00, 0x00, 0x00, 0x01, 0x3E, 0x00, 0x00};

static struct dsi_cmd_desc hx8394a_display_on_cmds[] = {
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_password), set_password},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_lane), set_lane},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_power), set_power},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_cyc), set_cyc},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_rtl_display_reg), set_rtl_display_reg},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_gip), set_gip},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_tcon), set_tcon},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_init0), set_init0},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_panel), set_panel},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_gamma), set_gamma},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_dgc), set_dgc},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_vcom), set_vcom},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_init1), set_init1},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(set_cabc), set_cabc},
	{DSI_DI_DCS_SWRITE, 1, 200, sizeof(exit_sleep), exit_sleep},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(display_ctrl), (u8 *)display_ctrl},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(brightness_ctrl), (u8 *)brightness_ctrl},
	{DSI_DI_DCS_SWRITE, 1, 50, sizeof(display_on), display_on},
	};

void hx8394a_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, hx8394a_display_on_cmds,
			 ARRAY_SIZE(hx8394a_display_on_cmds));
}

static u8 otm8018b_dijin_exit_sleep[] = {0x11};
static u8 otm8018b_dijin_display_on[] = {0x29};

#define OTM8019_SLEEP_OUT_DELAY 100
#define OTM8019_DISP_ON_DELAY   100
static u8 otm8018b_dijin_reg_cmd_01[] = {0x00,0x00};
static u8 otm8018b_dijin_reg_cmd_02[] = {0xFF,0x80,0x09,0x01};

static u8 otm8018b_dijin_reg_cmd_03[] = {0x00,0x80};
static u8 otm8018b_dijin_reg_cmd_04[] = {0xFF,0x80,0x09};

static u8 otm8018b_dijin_reg_cmd_05[] = {0x00,0x80};
static u8 otm8018b_dijin_reg_cmd_06[] = {0xC4,0x30};

static u8 otm8018b_dijin_reg_cmd_07[] = {0x00,0x8A};
static u8 otm8018b_dijin_reg_cmd_08[] = {0xC4,0x40};

static u8 otm8018b_dijin_reg_cmd_09[] = {0x00,0xB4};
static u8 otm8018b_dijin_reg_cmd_10[] = {0xC0,0x02};

static u8 otm8018b_dijin_reg_cmd_11[] = {0x00,0x82};
static u8 otm8018b_dijin_reg_cmd_12[] = {0xC5,0xA3};

static u8 otm8018b_dijin_reg_cmd_13[] = {0x00,0x90};
static u8 otm8018b_dijin_reg_cmd_14[] = {0xC5,0x96,0x87};

static u8 otm8018b_dijin_reg_cmd_15[] = {0x00,0x00};
static u8 otm8018b_dijin_reg_cmd_16[] = {0xD8,0x87,0x87};

static u8 otm8018b_dijin_reg_cmd_17[] = {0x00,0x00};
static u8 otm8018b_dijin_reg_cmd_18[] = {0xD9,0x52};

static u8 otm8018b_dijin_reg_cmd_19[] = {0x00,0x81};
static u8 otm8018b_dijin_reg_cmd_20[] = {0xC1,0x66};

static u8 otm8018b_dijin_reg_cmd_21[] = {0x00,0xA1};
static u8 otm8018b_dijin_reg_cmd_22[] = {0xC1,0x08};

static u8 otm8018b_dijin_reg_cmd_23[] = {0x00,0x89};
static u8 otm8018b_dijin_reg_cmd_24[] = {0xC4,0x08};

static u8 otm8018b_dijin_reg_cmd_25[] = {0x00,0xA3};
static u8 otm8018b_dijin_reg_cmd_26[] = {0xC0,0x00};

static u8 otm8018b_dijin_reg_cmd_27[] = {0x00,0x81};
static u8 otm8018b_dijin_reg_cmd_28[] = {0xC4,0x83};

static u8 otm8018b_dijin_reg_cmd_29[] = {0x00,0x92};
static u8 otm8018b_dijin_reg_cmd_30[] = {0xC5,0x01};

static u8 otm8018b_dijin_reg_cmd_31[] = {0x00,0xB1};
static u8 otm8018b_dijin_reg_cmd_32[] = {0xC5,0xA9};

static u8 otm8018b_dijin_reg_cmd_33[] = {0x00,0xC7};
static u8 otm8018b_dijin_reg_cmd_34[] = {0xCF,0x02};

static u8 otm8018b_dijin_reg_cmd_35[] = {0x00,0x90};
static u8 otm8018b_dijin_reg_cmd_36[] = {0xB3,0x02};

static u8 otm8018b_dijin_reg_cmd_37[] = {0x00,0x92};
static u8 otm8018b_dijin_reg_cmd_38[] = {0xB3,0x45};

static u8 otm8018b_dijin_reg_cmd_39[] = {0x00,0x80};
static u8 otm8018b_dijin_reg_cmd_40[] = {0xC0,0x00,0x58,0x00,0x15,0x15,0x00,0x58,0x15,0x15};

static u8 otm8018b_dijin_reg_cmd_41[] = {0x00,0x90};
static u8 otm8018b_dijin_reg_cmd_42[] = {0xC0,0x00,0x44,0x00,0x00,0x00,0x03};

static u8 otm8018b_dijin_reg_cmd_43[] = {0x00,0x80};
static u8 otm8018b_dijin_reg_cmd_44[] = {0xCE,0x87,0x03,0x00,0x86,0x03,0x00,0x85,0x03,0x00,0x84,0x03,0x00};

static u8 otm8018b_dijin_reg_cmd_45[] = {0x00,0x90};
static u8 otm8018b_dijin_reg_cmd_46[] = {0xCE,0x33,0x52,0x00,0x33,0x53,0x00,0x33,0x54,0x00,0x33,0x55,0x00,0x00,0x00};

static u8 otm8018b_dijin_reg_cmd_47[] = {0x00,0xA0};
static u8 otm8018b_dijin_reg_cmd_48[] = {0xCE,0x38,0x05,0x03,0x56,0x00,0x00,0x00,0x38,0x04,0x03,0x57,0x00,0x00,0x00};

static u8 otm8018b_dijin_reg_cmd_49[] = {0x00,0xB0};
static u8 otm8018b_dijin_reg_cmd_50[] = {0xCE,0x38,0x03,0x03,0x58,0x00,0x00,0x00,0x38,0x02,0x03,0x59,0x00,0x00,0x00};

static u8 otm8018b_dijin_reg_cmd_51[] = {0x00,0xC0};
static u8 otm8018b_dijin_reg_cmd_52[] = {0xCE,0x38,0x01,0x03,0x5A,0x00,0x00,0x00,0x38,0x00,0x03,0x5C,0x00,0x00,0x00};

static u8 otm8018b_dijin_reg_cmd_53[] = {0x00,0xD0};
static u8 otm8018b_dijin_reg_cmd_54[] = {0xCE,0x30,0x00,0x03,0x5C,0x00,0x00,0x00,0x30,0x01,0x03,0x5D,0x00,0x00,0x00};

static u8 otm8018b_dijin_reg_cmd_55[] = {0x00,0xC3};
static u8 otm8018b_dijin_reg_cmd_56[] = {0xCB,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04};

static u8 otm8018b_dijin_reg_cmd_57[] = {0x00,0xD8};
static u8 otm8018b_dijin_reg_cmd_58[] = {0xCB,0x04,0x04,0x04,0x04,0x04,0x04,0x04};

static u8 otm8018b_dijin_reg_cmd_59[] = {0x00,0xE0};
static u8 otm8018b_dijin_reg_cmd_60[] = {0xCB,0x04};

static u8 otm8018b_dijin_reg_cmd_61[] = {0x00,0x83};
static u8 otm8018b_dijin_reg_cmd_62[] = {0xCC,0x03,0x01,0x09,0x0B,0x0D,0x0F,0x05};

static u8 otm8018b_dijin_reg_cmd_63[] = {0x00,0x90};
static u8 otm8018b_dijin_reg_cmd_64[] = {0xCC,0x07};

static u8 otm8018b_dijin_reg_cmd_65[] = {0x00,0x9D};
static u8 otm8018b_dijin_reg_cmd_66[] = {0xCC,0x04,0x02};

static u8 otm8018b_dijin_reg_cmd_67[] = {0x00,0xA0};
static u8 otm8018b_dijin_reg_cmd_68[] = {0xCC,0x0A,0x0C,0x0E,0x10,0x06,0x08};

static u8 otm8018b_dijin_reg_cmd_69[] = {0x00,0xB3};
static u8 otm8018b_dijin_reg_cmd_70[] = {0xCC,0x06,0x08,0x0A,0x10,0x0E,0x0C,0x04};

static u8 otm8018b_dijin_reg_cmd_71[] = {0x00,0xC0};
static u8 otm8018b_dijin_reg_cmd_72[] = {0xCC,0x02};

static u8 otm8018b_dijin_reg_cmd_73[] = {0x00,0xCD};
static u8 otm8018b_dijin_reg_cmd_74[] = {0xCC,0x05,0x07};

static u8 otm8018b_dijin_reg_cmd_75[] = {0x00,0xD0};
static u8 otm8018b_dijin_reg_cmd_76[] = {0xCC,0x09,0x0F,0x0D,0x0B,0x03,0x01};

static u8 otm8018b_dijin_reg_cmd_77[] = {0x00,0x00};
static u8 otm8018b_dijin_reg_cmd_78[] = {0xE1,0x04,0x0D,0x12,0x0F,0x09,0x1C,0x0E,0x0E,0x00,0x05,0x02,0x06,0x0E,0x1D,0x1A,0x12};

static u8 otm8018b_dijin_reg_cmd_79[] = {0x00,0x00};
static u8 otm8018b_dijin_reg_cmd_80[] = {0xE2,0x04,0x0D,0x12,0x0E,0x08,0x1C,0x0E,0x0E,0x00,0x04,0x03,0x07,0x0E,0x1E,0x1B,0x12};

static u8 otm8018b_dijin_reg_cmd_81[] = {0x00,0xA0};
static u8 otm8018b_dijin_reg_cmd_82[] = {0xC1,0xEA};

static u8 otm8018b_dijin_reg_cmd_83[] = {0x00,0xA6};
static u8 otm8018b_dijin_reg_cmd_84[] = {0xC1,0x01,0x00,0x00};

static u8 otm8018b_dijin_reg_cmd_85[] = {0x00,0xC6};
static u8 otm8018b_dijin_reg_cmd_86[] = {0xB0,0x03};

static u8 otm8018b_dijin_reg_cmd_87[] = {0x00,0x81};
static u8 otm8018b_dijin_reg_cmd_88[] = {0xC5,0x66};

static u8 otm8018b_dijin_reg_cmd_89[] = {0x00,0xB6};
static u8 otm8018b_dijin_reg_cmd_90[] = {0xF5,0x06};

static u8 otm8018b_dijin_reg_cmd_91[] = {0x00,0x8B};
static u8 otm8018b_dijin_reg_cmd_92[] = {0xB0,0x40};

static u8 otm8018b_dijin_reg_cmd_93[] = {0x00,0xB1};
static u8 otm8018b_dijin_reg_cmd_94[] = {0xB0,0x80};

static u8 otm8018b_dijin_reg_cmd_95[] = {0x00,0x00};
static u8 otm8018b_dijin_reg_cmd_96[] = {0xFF,0xFF,0xFF,0xFF};

static u8 otm8018b_dijin_reg_cmd_97[] = {0x35,0x00};
static u8 otm8018b_dijin_reg_cmd_98[] = {0x51, 0x00};

static u8 otm8018b_dijin_reg_cmd_99[] = {0x53, 0x24};

static struct dsi_cmd_desc otm8018b_display_on_cmds[] = {
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_01), otm8018b_dijin_reg_cmd_01},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_02), otm8018b_dijin_reg_cmd_02},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_03), otm8018b_dijin_reg_cmd_03},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_04), otm8018b_dijin_reg_cmd_04},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_05), otm8018b_dijin_reg_cmd_05},
	{DSI_DI_GENERIC_LWRITE,   1,10,sizeof(otm8018b_dijin_reg_cmd_06), otm8018b_dijin_reg_cmd_06},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_07), otm8018b_dijin_reg_cmd_07},
	{DSI_DI_GENERIC_LWRITE,   1,10,sizeof(otm8018b_dijin_reg_cmd_08), otm8018b_dijin_reg_cmd_08},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_09), otm8018b_dijin_reg_cmd_09},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_10), otm8018b_dijin_reg_cmd_10},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_11), otm8018b_dijin_reg_cmd_11},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_12), otm8018b_dijin_reg_cmd_12},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_13), otm8018b_dijin_reg_cmd_13},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_14), otm8018b_dijin_reg_cmd_14},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_15), otm8018b_dijin_reg_cmd_15},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_16), otm8018b_dijin_reg_cmd_16},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_17), otm8018b_dijin_reg_cmd_17},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_18), otm8018b_dijin_reg_cmd_18},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_19), otm8018b_dijin_reg_cmd_19},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_20), otm8018b_dijin_reg_cmd_20},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_21), otm8018b_dijin_reg_cmd_21},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_22), otm8018b_dijin_reg_cmd_22},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_23), otm8018b_dijin_reg_cmd_23},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_24), otm8018b_dijin_reg_cmd_24},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_25), otm8018b_dijin_reg_cmd_25},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_26), otm8018b_dijin_reg_cmd_26},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_27), otm8018b_dijin_reg_cmd_27},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_28), otm8018b_dijin_reg_cmd_28},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_29), otm8018b_dijin_reg_cmd_29},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_30), otm8018b_dijin_reg_cmd_30},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_31), otm8018b_dijin_reg_cmd_31},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_32), otm8018b_dijin_reg_cmd_32},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_33), otm8018b_dijin_reg_cmd_33},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_34), otm8018b_dijin_reg_cmd_34},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_35), otm8018b_dijin_reg_cmd_35},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_36), otm8018b_dijin_reg_cmd_36},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_37), otm8018b_dijin_reg_cmd_37},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_38), otm8018b_dijin_reg_cmd_38},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_39), otm8018b_dijin_reg_cmd_39},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_40), otm8018b_dijin_reg_cmd_40},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_41), otm8018b_dijin_reg_cmd_41},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_42), otm8018b_dijin_reg_cmd_42},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_43), otm8018b_dijin_reg_cmd_43},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_44), otm8018b_dijin_reg_cmd_44},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_45), otm8018b_dijin_reg_cmd_45},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_46), otm8018b_dijin_reg_cmd_46},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_47), otm8018b_dijin_reg_cmd_47},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_48), otm8018b_dijin_reg_cmd_48},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_49), otm8018b_dijin_reg_cmd_49},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_50), otm8018b_dijin_reg_cmd_50},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_51), otm8018b_dijin_reg_cmd_51},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_52), otm8018b_dijin_reg_cmd_52},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_53), otm8018b_dijin_reg_cmd_53},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_54), otm8018b_dijin_reg_cmd_54},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_55), otm8018b_dijin_reg_cmd_55},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_56), otm8018b_dijin_reg_cmd_56},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_57), otm8018b_dijin_reg_cmd_57},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_58), otm8018b_dijin_reg_cmd_58},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_59), otm8018b_dijin_reg_cmd_59},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_60), otm8018b_dijin_reg_cmd_60},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_61), otm8018b_dijin_reg_cmd_61},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_62), otm8018b_dijin_reg_cmd_62},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_63), otm8018b_dijin_reg_cmd_63},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_64), otm8018b_dijin_reg_cmd_64},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_65), otm8018b_dijin_reg_cmd_65},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_66), otm8018b_dijin_reg_cmd_66},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_67), otm8018b_dijin_reg_cmd_67},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_68), otm8018b_dijin_reg_cmd_68},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_69), otm8018b_dijin_reg_cmd_69},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_70), otm8018b_dijin_reg_cmd_70},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_71), otm8018b_dijin_reg_cmd_71},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_72), otm8018b_dijin_reg_cmd_72},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_73), otm8018b_dijin_reg_cmd_73},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_74), otm8018b_dijin_reg_cmd_74},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_75), otm8018b_dijin_reg_cmd_75},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_76), otm8018b_dijin_reg_cmd_76},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_77), otm8018b_dijin_reg_cmd_77},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_78), otm8018b_dijin_reg_cmd_78},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_79), otm8018b_dijin_reg_cmd_79},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_80), otm8018b_dijin_reg_cmd_80},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_81), otm8018b_dijin_reg_cmd_81},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_82), otm8018b_dijin_reg_cmd_82},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_83), otm8018b_dijin_reg_cmd_83},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_84), otm8018b_dijin_reg_cmd_84},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_85), otm8018b_dijin_reg_cmd_85},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_86), otm8018b_dijin_reg_cmd_86},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_87), otm8018b_dijin_reg_cmd_87},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_88), otm8018b_dijin_reg_cmd_88},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_89), otm8018b_dijin_reg_cmd_89},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_90), otm8018b_dijin_reg_cmd_90},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_91), otm8018b_dijin_reg_cmd_91},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_92), otm8018b_dijin_reg_cmd_92},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_93), otm8018b_dijin_reg_cmd_93},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_94), otm8018b_dijin_reg_cmd_94},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_95), otm8018b_dijin_reg_cmd_95},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_96), otm8018b_dijin_reg_cmd_96},
	{DSI_DI_GENERIC_LWRITE,   1,0,sizeof(otm8018b_dijin_reg_cmd_97), otm8018b_dijin_reg_cmd_97},

	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(otm8018b_dijin_reg_cmd_98), otm8018b_dijin_reg_cmd_98},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(otm8018b_dijin_reg_cmd_99), otm8018b_dijin_reg_cmd_99},

	{DSI_DI_DCS_SWRITE, 1, OTM8019_SLEEP_OUT_DELAY, sizeof(otm8018b_dijin_exit_sleep),
		otm8018b_dijin_exit_sleep},
	{DSI_DI_DCS_SWRITE, 1, OTM8019_DISP_ON_DELAY, sizeof(otm8018b_dijin_display_on),
		otm8018b_dijin_display_on},
};

void otm8018b_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, otm8018b_display_on_cmds,
			 ARRAY_SIZE(otm8018b_display_on_cmds));
}

static struct dsi_cmd_desc trun_on_backlight_cmds[] = {
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(turn_on_backlight), (u8 *)turn_on_backlight},
	};

void pwm_turn_on_backlight(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, trun_on_backlight_cmds,
			ARRAY_SIZE(trun_on_backlight_cmds));
}
static u8 otm1283a_cmi_exit_sleep[] = {0x11, 0x00};
static u8 otm1283a_cmi_display_on[] = {0x29, 0x00};

#define OTM1283_SLEEP_OUT_DELAY 120
#define OTM1283_DISP_ON_DELAY   20
static u8 otm1283a_cmi_reg_cmd_01[] = {0x00, 0x00};
static u8 otm1283a_cmi_reg_cmd_02[] = {0xFF, 0x12, 0x83, 0x01};

static u8 otm1283a_cmi_reg_cmd_03[] = {0x00, 0x80};
static u8 otm1283a_cmi_reg_cmd_04[] = {0xFF, 0x12, 0x83};

static u8 otm1283a_cmi_reg_cmd_05[] = {0x00, 0xB9};
static u8 otm1283a_cmi_reg_cmd_06[] = {0xB0, 0x51};

static u8 otm1283a_cmi_reg_cmd_07[] = {0x00, 0xC6};
static u8 otm1283a_cmi_reg_cmd_08[] = {0xB0, 0x03};

static u8 otm1283a_cmi_reg_cmd_09[] = {0x00, 0x80};
static u8 otm1283a_cmi_reg_cmd_10[] = {0xC0, 0x00, 0x64, 0x00, 0x10, 0x10, 0x00, 0x64, 0x10, 0x10};

static u8 otm1283a_cmi_reg_cmd_11[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_12[] = {0xC0, 0x00, 0x5c, 0x00, 0x01, 0x00, 0x04};

static u8 otm1283a_cmi_reg_cmd_13[] = {0x00, 0xB3};
static u8 otm1283a_cmi_reg_cmd_14[] = {0xC0, 0x00, 0x50};

static u8 otm1283a_cmi_reg_cmd_15[] = {0x00, 0x81};
static u8 otm1283a_cmi_reg_cmd_16[] = {0xC1, 0x55};

static u8 otm1283a_cmi_reg_cmd_17[] = {0x00, 0x82};
static u8 otm1283a_cmi_reg_cmd_18[] = {0xC4, 0x02};

static u8 otm1283a_cmi_reg_cmd_19[] = {0x00, 0x8B};
static u8 otm1283a_cmi_reg_cmd_20[] = {0xC4, 0x40};

static u8 otm1283a_cmi_reg_cmd_21[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_22[] = {0xC4, 0x49};

static u8 otm1283a_cmi_reg_cmd_23[] = {0x00, 0xA0};
static u8 otm1283a_cmi_reg_cmd_24[] = {0xC4, 0x05, 0x10, 0x04, 0x02, 0x05, 0x15, 0x11, 0x05, 0x10, 0x07, 0x02, 0x05, 0x15, 0x11};

static u8 otm1283a_cmi_reg_cmd_25[] = {0x00, 0xB0};
static u8 otm1283a_cmi_reg_cmd_26[] = {0xC4, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_27[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_28[] = {0xC5, 0x50, 0xA6, 0xD0, 0x66};

static u8 otm1283a_cmi_reg_cmd_29[] = {0x00, 0xB0};
static u8 otm1283a_cmi_reg_cmd_30[] = {0xC5, 0x04, 0x38};

static u8 otm1283a_cmi_reg_cmd_31[] = {0x00, 0xB4};
static u8 otm1283a_cmi_reg_cmd_32[] = {0xC5, 0xC0};

static u8 otm1283a_cmi_reg_cmd_33[] = {0x00, 0xb5};
static u8 otm1283a_cmi_reg_cmd_34[] = {0xc5, 0x0b, 0x95, 0xff, 0x0b, 0x95, 0xff};

static u8 otm1283a_cmi_reg_cmd_35[] = {0x00, 0xbb};
static u8 otm1283a_cmi_reg_cmd_36[] = {0xc5, 0x80};

static u8 otm1283a_cmi_reg_cmd_37[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_38[] = {0xf5, 0x02, 0x11, 0x02, 0x11};

static u8 otm1283a_cmi_reg_cmd_39[] = {0x00, 0x94};
static u8 otm1283a_cmi_reg_cmd_40[] = {0xF5, 0x02};

static u8 otm1283a_cmi_reg_cmd_41[] = {0x00, 0xb2};
static u8 otm1283a_cmi_reg_cmd_42[] = {0xf5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_43[] = {0x00, 0xBA};
static u8 otm1283a_cmi_reg_cmd_44[] = {0xF5, 0x03};

static u8 otm1283a_cmi_reg_cmd_45[] = {0x00, 0x80};
static u8 otm1283a_cmi_reg_cmd_46[] = {0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_47[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_48[] = {0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_49[] = {0x00, 0xa0};
static u8 otm1283a_cmi_reg_cmd_50[] = {0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_51[] = {0x00, 0xb0};
static u8 otm1283a_cmi_reg_cmd_52[] = {0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_53[] = {0x00, 0xc0};
static u8 otm1283a_cmi_reg_cmd_54[] = {0xcb, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_55[] = {0x00, 0xd0};
static u8 otm1283a_cmi_reg_cmd_56[] = {0xcb, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};

static u8 otm1283a_cmi_reg_cmd_57[] = {0x00, 0xe0};
static u8 otm1283a_cmi_reg_cmd_58[] = {0xcb, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_59[] = {0x00, 0xf0};
static u8 otm1283a_cmi_reg_cmd_60[] = {0xcb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static u8 otm1283a_cmi_reg_cmd_61[] = {0x00, 0x80};
static u8 otm1283a_cmi_reg_cmd_62[] = {0xcc, 0x29, 0x2a, 0x0a, 0x0c, 0x0e, 0x10, 0x12, 0x14, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_63[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_64[] = {0xcc, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x29, 0x2a, 0x09, 0x0b, 0x0d, 0x0f, 0x11, 0x13};

static u8 otm1283a_cmi_reg_cmd_65[] = {0x00, 0xa0};
static u8 otm1283a_cmi_reg_cmd_66[] = {0xcc, 0x05, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_67[] = {0x00, 0xb0};
static u8 otm1283a_cmi_reg_cmd_68[] = {0xcc, 0x29, 0x2a, 0x13, 0x11, 0x0f, 0x0d, 0x0b, 0x09, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_69[] = {0x00, 0xc0};
static u8 otm1283a_cmi_reg_cmd_70[] = {0xcc, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x29, 0x2a, 0x14, 0x12, 0x10, 0x0e, 0x0c, 0x0a};

static u8 otm1283a_cmi_reg_cmd_71[] = {0x00, 0xd0};
static u8 otm1283a_cmi_reg_cmd_72[] = {0xcc, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_73[] = {0x00, 0x80};
static u8 otm1283a_cmi_reg_cmd_74[] = {0xce, 0x89, 0x05, 0x10, 0x88, 0x05, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_75[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_76[] = {0xce, 0x54, 0xFD, 0x10, 0x54, 0xFE, 0x10, 0x55, 0x01, 0x10, 0x55, 0x02, 0x10, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_77[] = {0x00, 0xa0};
static u8 otm1283a_cmi_reg_cmd_78[] = {0xce, 0x58, 0x07, 0x04, 0xFD, 0x00, 0x10, 0x00, 0x58, 0x06, 0x04, 0xFE, 0x00, 0x10, 0x00};

static u8 otm1283a_cmi_reg_cmd_79[] = {0x00, 0xb0};
static u8 otm1283a_cmi_reg_cmd_80[] = {0xce, 0x58, 0x05, 0x04, 0xFF, 0x00, 0x10, 0x00, 0x58, 0x04, 0x05, 0x00, 0x00, 0x10, 0x00};

static u8 otm1283a_cmi_reg_cmd_81[] = {0x00, 0xc0};
static u8 otm1283a_cmi_reg_cmd_82[] = {0xce, 0x58, 0x03, 0x05, 0x01, 0x00, 0x10, 0x00, 0x58, 0x02, 0x05, 0x02, 0x00, 0x10, 0x00};

static u8 otm1283a_cmi_reg_cmd_83[] = {0x00, 0xd0};
static u8 otm1283a_cmi_reg_cmd_84[] = {0xce, 0x58, 0x01, 0x05, 0x03, 0x00, 0x10, 0x00, 0x58, 0x00, 0x05, 0x04, 0x00, 0x10, 0x00};

static u8 otm1283a_cmi_reg_cmd_85[] = {0x00, 0x80};
static u8 otm1283a_cmi_reg_cmd_86[] = {0xcf, 0x50, 0x00, 0x05, 0x05, 0x00, 0x10, 0x00, 0x50, 0x01, 0x05, 0x06, 0x00, 0x10, 0x00};

static u8 otm1283a_cmi_reg_cmd_87[] = {0x00, 0x90};
static u8 otm1283a_cmi_reg_cmd_88[] = {0xcf, 0x50, 0x02, 0x05, 0x07, 0x00, 0x10, 0x00, 0x50, 0x03, 0x05, 0x08, 0x00, 0x10, 0x00};

static u8 otm1283a_cmi_reg_cmd_89[] = {0x00, 0xc0};
static u8 otm1283a_cmi_reg_cmd_90[] = {0xcf, 0x39, 0x39, 0x20, 0x20, 0x00, 0x00, 0x01, 0x01, 0x20, 0x00, 0x00};

static u8 otm1283a_cmi_reg_cmd_91[] = {0x00, 0x00};
static u8 otm1283a_cmi_reg_cmd_92[] = {0xd8, 0xBE, 0xBE};

static u8 otm1283a_cmi_reg_cmd_93[] = {0x00, 0x00};
static u8 otm1283a_cmi_reg_cmd_94[] = {0xd9, 0x77}; /* VCOM */

static u8 otm1283a_cmi_reg_cmd_95[] = {0x00, 0x00};
static u8 otm1283a_cmi_reg_cmd_96[] = {0xE1, 0x02, 0x12, 0x18, 0x0E, 0x07, 0x0F, 0x0B, 0x09, 0x04, 0x07, 0x0E, 0x08, 0x0F, 0x12, 0x0C, 0x08};

static u8 otm1283a_cmi_reg_cmd_97[] = {0x00, 0x00};
static u8 otm1283a_cmi_reg_cmd_98[] = {0xE2, 0x02, 0x12, 0x18, 0x0E, 0x07, 0x10, 0x0B, 0x0A, 0x04, 0x08, 0x0E, 0x08, 0x0F, 0x12, 0x0C, 0x08};

static u8 otm1283a_cmi_reg_cmd_99[] = {0x00, 0x00};
static u8 otm1283a_cmi_reg_cmd_100[] = {0xff, 0xff, 0xff, 0xff};

static u8 otm1283a_cmi_reg_cmd_101[] = {0x35, 0x00}; /* TE on */

static u8 otm1283a_cmi_reg_cmd_102[] = {0x51, 0x00}; /* Backlight off */
static u8 otm1283a_cmi_reg_cmd_103[] = {0x53, 0x24};

static struct dsi_cmd_desc otm1283a_display_on_cmds[] = {
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_01), otm1283a_cmi_reg_cmd_01},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_02), otm1283a_cmi_reg_cmd_02},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_03), otm1283a_cmi_reg_cmd_03},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_04), otm1283a_cmi_reg_cmd_04},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_05), otm1283a_cmi_reg_cmd_05},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_06), otm1283a_cmi_reg_cmd_06},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_07), otm1283a_cmi_reg_cmd_07},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_08), otm1283a_cmi_reg_cmd_08},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_09), otm1283a_cmi_reg_cmd_09},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_10), otm1283a_cmi_reg_cmd_10},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_11), otm1283a_cmi_reg_cmd_11},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_12), otm1283a_cmi_reg_cmd_12},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_13), otm1283a_cmi_reg_cmd_13},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_14), otm1283a_cmi_reg_cmd_14},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_15), otm1283a_cmi_reg_cmd_15},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_16), otm1283a_cmi_reg_cmd_16},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_17), otm1283a_cmi_reg_cmd_17},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_18), otm1283a_cmi_reg_cmd_18},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_19), otm1283a_cmi_reg_cmd_19},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_20), otm1283a_cmi_reg_cmd_20},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_21), otm1283a_cmi_reg_cmd_21},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_22), otm1283a_cmi_reg_cmd_22},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_23), otm1283a_cmi_reg_cmd_23},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_24), otm1283a_cmi_reg_cmd_24},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_25), otm1283a_cmi_reg_cmd_25},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_26), otm1283a_cmi_reg_cmd_26},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_27), otm1283a_cmi_reg_cmd_27},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_28), otm1283a_cmi_reg_cmd_28},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_29), otm1283a_cmi_reg_cmd_29},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_30), otm1283a_cmi_reg_cmd_30},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_31), otm1283a_cmi_reg_cmd_31},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_32), otm1283a_cmi_reg_cmd_32},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_33), otm1283a_cmi_reg_cmd_33},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_34), otm1283a_cmi_reg_cmd_34},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_35), otm1283a_cmi_reg_cmd_35},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_36), otm1283a_cmi_reg_cmd_36},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_37), otm1283a_cmi_reg_cmd_37},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_38), otm1283a_cmi_reg_cmd_38},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_39), otm1283a_cmi_reg_cmd_39},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_40), otm1283a_cmi_reg_cmd_40},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_41), otm1283a_cmi_reg_cmd_41},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_42), otm1283a_cmi_reg_cmd_42},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_43), otm1283a_cmi_reg_cmd_43},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_44), otm1283a_cmi_reg_cmd_44},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_45), otm1283a_cmi_reg_cmd_45},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_46), otm1283a_cmi_reg_cmd_46},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_47), otm1283a_cmi_reg_cmd_47},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_48), otm1283a_cmi_reg_cmd_48},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_49), otm1283a_cmi_reg_cmd_49},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_50), otm1283a_cmi_reg_cmd_50},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_51), otm1283a_cmi_reg_cmd_51},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_52), otm1283a_cmi_reg_cmd_52},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_53), otm1283a_cmi_reg_cmd_53},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_54), otm1283a_cmi_reg_cmd_54},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_55), otm1283a_cmi_reg_cmd_55},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_56), otm1283a_cmi_reg_cmd_56},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_57), otm1283a_cmi_reg_cmd_57},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_58), otm1283a_cmi_reg_cmd_58},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_59), otm1283a_cmi_reg_cmd_59},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_60), otm1283a_cmi_reg_cmd_60},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_61), otm1283a_cmi_reg_cmd_61},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_62), otm1283a_cmi_reg_cmd_62},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_63), otm1283a_cmi_reg_cmd_63},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_64), otm1283a_cmi_reg_cmd_64},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_65), otm1283a_cmi_reg_cmd_65},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_66), otm1283a_cmi_reg_cmd_66},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_67), otm1283a_cmi_reg_cmd_67},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_68), otm1283a_cmi_reg_cmd_68},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_69), otm1283a_cmi_reg_cmd_69},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_70), otm1283a_cmi_reg_cmd_70},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_71), otm1283a_cmi_reg_cmd_71},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_72), otm1283a_cmi_reg_cmd_72},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_73), otm1283a_cmi_reg_cmd_73},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_74), otm1283a_cmi_reg_cmd_74},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_75), otm1283a_cmi_reg_cmd_75},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_76), otm1283a_cmi_reg_cmd_76},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_77), otm1283a_cmi_reg_cmd_77},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_78), otm1283a_cmi_reg_cmd_78},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_79), otm1283a_cmi_reg_cmd_79},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_80), otm1283a_cmi_reg_cmd_80},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_81), otm1283a_cmi_reg_cmd_81},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_82), otm1283a_cmi_reg_cmd_82},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_83), otm1283a_cmi_reg_cmd_83},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_84), otm1283a_cmi_reg_cmd_84},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_85), otm1283a_cmi_reg_cmd_85},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_86), otm1283a_cmi_reg_cmd_86},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_87), otm1283a_cmi_reg_cmd_87},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_88), otm1283a_cmi_reg_cmd_88},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_89), otm1283a_cmi_reg_cmd_89},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_90), otm1283a_cmi_reg_cmd_90},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_91), otm1283a_cmi_reg_cmd_91},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_92), otm1283a_cmi_reg_cmd_92},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_93), otm1283a_cmi_reg_cmd_93},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_94), otm1283a_cmi_reg_cmd_94},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_95), otm1283a_cmi_reg_cmd_95},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_96), otm1283a_cmi_reg_cmd_96},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_97), otm1283a_cmi_reg_cmd_97},

	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_98), otm1283a_cmi_reg_cmd_98},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_99), otm1283a_cmi_reg_cmd_99},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_100), otm1283a_cmi_reg_cmd_100},
	{DSI_DI_GENERIC_LWRITE, 1, 0, sizeof(otm1283a_cmi_reg_cmd_101), otm1283a_cmi_reg_cmd_101},

	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(otm1283a_cmi_reg_cmd_102), otm1283a_cmi_reg_cmd_102},
	{DSI_DI_DCS_SWRITE1, 1, 0, sizeof(otm1283a_cmi_reg_cmd_103), otm1283a_cmi_reg_cmd_103},

	{DSI_DI_DCS_SWRITE, 1, OTM1283_SLEEP_OUT_DELAY, sizeof(otm1283a_cmi_exit_sleep),
		otm1283a_cmi_exit_sleep},
	{DSI_DI_DCS_SWRITE, 1, OTM1283_DISP_ON_DELAY, sizeof(otm1283a_cmi_display_on),
		otm1283a_cmi_display_on},
};

void otm1283a_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, otm1283a_display_on_cmds,
			 ARRAY_SIZE(otm1283a_display_on_cmds));
}

#define FL10802_SLEEP_OUT_DELAY	100
#define FL10802_DISP_ON_DELAY	100
#define LP_MODE			1
/******************* huayu  cpt *****************/
static u8 fl10802_cpt_fwvga_video_on_cmd00[] = {0xB9, 0xF1, 0x08, 0x01};
static u8 fl10802_cpt_fwvga_video_on_cmd01[] = {0xB1, 0x22, 0x1A, 0x1A, 0x87};
static u8 fl10802_cpt_fwvga_video_on_cmd02[] = {0xB2, 0x22};
static u8 fl10802_cpt_fwvga_video_on_cmd03[] = {
	0xB3, 0x01, 0x00, 0x06, 0x06, 0x18, 0x13, 0x39, 0x35
};
static u8 fl10802_cpt_fwvga_video_on_cmd04[] = {
	0xBA, 0x31, 0x00, 0x44, 0x25,
	0x91, 0x0A, 0x00, 0x00, 0xC1,
	0x00, 0x00, 0x00, 0x0D, 0x02,
	0x4F, 0xB9, 0xEE
};
static u8 fl10802_cpt_fwvga_video_on_cmd05[] = {
	0xE3, 0x09, 0x09, 0x03, 0x03, 0x00
};
static u8 fl10802_cpt_fwvga_video_on_cmd06[] = {0xB4, 0x02};
static u8 fl10802_cpt_fwvga_video_on_cmd07[] = {0xB5, 0x07, 0x07};
static u8 fl10802_cpt_fwvga_video_on_cmd08[] = {0xB6, 0x20, 0x20};
static u8 fl10802_cpt_fwvga_video_on_cmd09[] = {0xB8, 0x64, 0x28};
static u8 fl10802_cpt_fwvga_video_on_cmd10[] = {0xCC, 0x00};
static u8 fl10802_cpt_fwvga_video_on_cmd11[] = {0xBC, 0x47};
static u8 fl10802_cpt_fwvga_video_on_cmd12[] = {
	0xE9, 0x00, 0x00, 0x0F, 0x03,
	0x6C, 0x0A, 0x8A, 0x10, 0x01,
	0x00, 0x37, 0x13, 0x0A, 0x76,
	0x37, 0x00, 0x00, 0x18, 0x00,
	0x00, 0x00, 0x25, 0x09, 0x80,
	0x40, 0x00, 0x42, 0x60, 0x00,
	0x00, 0x00, 0x09, 0x81, 0x50,
	0x01, 0x53, 0x70, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00
};
static u8 fl10802_cpt_fwvga_video_on_cmd13[] = {
	0xEA, 0x94, 0x00, 0x00, 0x00,
	0x08, 0x95, 0x10, 0x07, 0x35,
	0x10, 0x00, 0x00, 0x00, 0x08,
	0x94, 0x00, 0x06, 0x24, 0x00,
	0x00, 0x00, 0x00
};
static u8 fl10802_cpt_fwvga_video_on_cmd14[] = {
	0xE0, 0x00, 0x00, 0x08, 0x10,
	0x19, 0x2A, 0x25, 0x37, 0x09,
	0x10, 0x11, 0x15, 0x18, 0x16,
	0x16, 0x16, 0x1B, 0x00, 0x00,
	0x08, 0x10, 0x19, 0x2A, 0x25,
	0x37, 0x09, 0x10, 0x11, 0x15,
	0x18, 0x16, 0x16, 0x16, 0x1B
};
static u8 fl10802_cpt_fwvga_video_on_cmd15[] = {0x11, 0x00};
static u8 fl10802_cpt_fwvga_video_on_cmd16[] = {0x29, 0x00};

static struct dsi_cmd_desc fl10802_cpt_video_display_on_cmds[] = {
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd00),
		fl10802_cpt_fwvga_video_on_cmd00},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd01),
		fl10802_cpt_fwvga_video_on_cmd01},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd02),
		fl10802_cpt_fwvga_video_on_cmd02},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd03),
		fl10802_cpt_fwvga_video_on_cmd03},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd04),
		fl10802_cpt_fwvga_video_on_cmd04},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd05),
		fl10802_cpt_fwvga_video_on_cmd05},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd07),
		fl10802_cpt_fwvga_video_on_cmd07},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd08),
		fl10802_cpt_fwvga_video_on_cmd08},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd09),
		fl10802_cpt_fwvga_video_on_cmd09},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd06),
		fl10802_cpt_fwvga_video_on_cmd06},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd10),
		fl10802_cpt_fwvga_video_on_cmd10},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd11),
		fl10802_cpt_fwvga_video_on_cmd11},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd12),
		fl10802_cpt_fwvga_video_on_cmd12},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd13),
		fl10802_cpt_fwvga_video_on_cmd13},
	{DSI_DI_GENERIC_LWRITE, LP_MODE, 0,
		sizeof(fl10802_cpt_fwvga_video_on_cmd14),
		fl10802_cpt_fwvga_video_on_cmd14},

	{DSI_DI_DCS_SWRITE, LP_MODE, 150,
		sizeof(fl10802_cpt_fwvga_video_on_cmd15),
		fl10802_cpt_fwvga_video_on_cmd15},
	{DSI_DI_DCS_SWRITE, LP_MODE, 100,
		sizeof(fl10802_cpt_fwvga_video_on_cmd16),
		fl10802_cpt_fwvga_video_on_cmd16}
};

void fl10802_init_config(struct mmp_disp_info *fbi)
{
	dsi_cmd_array_tx(fbi, fl10802_cpt_video_display_on_cmds,
			 ARRAY_SIZE(fl10802_cpt_video_display_on_cmds));
}

static u8 hx8394dd_720p_video_on_cmd_list0[] = {0xB9, 0xFF, 0x83, 0x94};
static u8 hx8394dd_720p_video_on_cmd_list1[] = {0xBA, 0x73, 0x83};
static u8 hx8394dd_720p_video_on_cmd_list2[] = {
	0xB1, 0x6c, 0x12, 0x12, 0x26,
	0x04, 0x11, 0xF1, 0x81, 0x3a,
	0x54, 0x23, 0x80, 0xC0, 0xD2,
	0x58
};
static u8 hx8394dd_720p_video_on_cmd_list3[] = {
	0xB2, 0x00, 0x64, 0x0e, 0x0d,
	0x22, 0x1C, 0x08, 0x08, 0x1C,
	0x4D, 0x00
};
static u8 hx8394dd_720p_video_on_cmd_list4[] = {
	0xB4, 0x00, 0xFF, 0x51, 0x5A,
	0x59, 0x5A, 0x03, 0x5A, 0x01,
	0x70, 0x20, 0x70
};
static u8 hx8394dd_720p_video_on_cmd_list5[] = {0xBC, 0x07};
static u8 hx8394dd_720p_video_on_cmd_list6[] = {0xBF, 0x41, 0x0E, 0x01};
/* static u8 hx8394dd_720p_video_on_cmd_list7[] ={0xD2,0x55}; */
static u8 hx8394dd_720p_video_on_cmd_list7[] = {0xB6, 0x6B, 0x6B};
static u8 hx8394dd_720p_video_on_cmd_list8[] = {
	0xD3, 0x00, 0x0F, 0x00, 0x40,
	0x07, 0x10, 0x00, 0x08, 0x10,
	0x08, 0x00, 0x08, 0x54, 0x15,
	0x0e, 0x05, 0x0e, 0x02, 0x15,
	0x06, 0x05, 0x06, 0x47, 0x44,
	0x0a, 0x0a, 0x4b, 0x10, 0x07,
	0x07
};
static u8 hx8394dd_720p_video_on_cmd_list9[] = {
	0xD5, 0x1a, 0x1a, 0x1b, 0x1b,
	0x00, 0x01, 0x02, 0x03, 0x04,
	0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x24, 0x25, 0x18,
	0x18, 0x26, 0x27, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x20,
	0x21, 0x18, 0x18, 0x18, 0x18
};
static u8 hx8394dd_720p_video_on_cmd_list10[] = {
	0xD6, 0x1a, 0x1a, 0x1b, 0x1b,
	0x0B, 0x0a, 0x09, 0x08, 0x07,
	0x06, 0x05, 0x04, 0x03, 0x02,
	0x01, 0x00, 0x21, 0x20, 0x58,
	0x58, 0x27, 0x26, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x25,
	0x24, 0x18, 0x18, 0x18, 0x18
};
/* static u8 hx8394dd_720p_video_on_cmd_list11[] ={0xB6,0x82,0x82}; */
/* Gammer = 2.6.. */
#if 0
static u8 hx8394dd_720p_video_on_cmd_list12[] = {
	0xE0, 0x00, 0x1b, 0x1f, 0x3a,
	0x3f, 0x3F, 0x28, 0x41, 0x09,
	0x0b, 0x0C, 0x17, 0x0D, 0x10,
	0x13, 0x11, 0x10, 0x02, 0x0f,
	0x06, 0x11, 0x00, 0x1b, 0x20,
	0x3b, 0x3f, 0x3F, 0x28, 0x47,
	0x08, 0x0b, 0x0C, 0x17, 0x0c,
	0x10, 0x11, 0x10, 0x12, 0x04,
	0x0c, 0x0d, 0x0a
};
#else /* Gammer = 2.03 */
static u8 hx8394dd_720p_video_on_cmd_list12[] = {
	0xE0, 0x00, 0x0b, 0x10, 0x2c,
	0x32, 0x3F, 0x1d, 0x39, 0x06,
	0x0b, 0x0d, 0x17, 0x0e, 0x11,
	0x14, 0x11, 0x13, 0x08, 0x13,
	0x14, 0x16, 0x00, 0x0b, 0x10,
	0x2c, 0x32, 0x3F, 0x1d, 0x39,
	0x06, 0x0b, 0x0d, 0x17, 0x0e,
	0x11, 0x14, 0x11, 0x13, 0x08,
	0x13, 0x14, 0x16
};
#endif
static u8 hx8394dd_720p_video_on_cmd_list13[] = {0xC0, 0x30, 0x14};
static u8 hx8394dd_720p_video_on_cmd_list14[] = {0xC7, 0x00, 0xC0, 0x40, 0xC0};
static u8 hx8394dd_720p_video_on_cmd_list15[] = {0xCC, 0x09};
/* static u8 hx8394dd_720p_video_on_cmd_list16[] = {0xDF, 0x88}; */
static u8 hx8394dd_720p_video_on_cmd_list17[] = {0x11};
static u8 hx8394dd_720p_video_on_cmd_list18[] = {0x29};
static struct dsi_cmd_desc hx8394d_jt_video_display_on_cmds[] = {
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list0),
		hx8394dd_720p_video_on_cmd_list0},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list1),
		hx8394dd_720p_video_on_cmd_list1},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list2),
		hx8394dd_720p_video_on_cmd_list2},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list3),
		hx8394dd_720p_video_on_cmd_list3},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list4),
		hx8394dd_720p_video_on_cmd_list4},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list5),
		hx8394dd_720p_video_on_cmd_list5},

	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list7),
		hx8394dd_720p_video_on_cmd_list7},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list8),
		hx8394dd_720p_video_on_cmd_list8},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list9),
		hx8394dd_720p_video_on_cmd_list9},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list6),
		hx8394dd_720p_video_on_cmd_list6},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list10),
		hx8394dd_720p_video_on_cmd_list10},
	/* {DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list11),
	   hx8394dd_720p_video_on_cmd_list11}, */
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list12),
		hx8394dd_720p_video_on_cmd_list12},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list13),
		hx8394dd_720p_video_on_cmd_list13},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list14),
		hx8394dd_720p_video_on_cmd_list14},
	{DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list15),
		hx8394dd_720p_video_on_cmd_list15},
	/* {DSI_DI_DCS_LWRITE, 1, 0, sizeof(hx8394dd_720p_video_on_cmd_list16),
	   hx8394dd_720p_video_on_cmd_list16}, */
	{DSI_DI_DCS_LWRITE, 1, 120, sizeof(hx8394dd_720p_video_on_cmd_list17),
		hx8394dd_720p_video_on_cmd_list17},
	{DSI_DI_DCS_LWRITE, 1, 200, sizeof(hx8394dd_720p_video_on_cmd_list18),
		hx8394dd_720p_video_on_cmd_list18}
};

void hx8394d_jt_init_config(struct mmp_disp_info *fbi)
{
	gpio_direction_output(20, 0);
	udelay(10000);
	gpio_direction_output(20, 1);
	udelay(20000);
	dsi_cmd_array_tx(fbi, hx8394d_jt_video_display_on_cmds,
			ARRAY_SIZE(hx8394d_jt_video_display_on_cmds));
}


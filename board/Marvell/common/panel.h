/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __PANEL_H__
#define __PANEL_H__
#include <common.h>
#include <malloc.h>
#include <mmp_disp.h>

#define TRULY_720P_ID 0x400000
#define SHARP_1080P_ID 0x38
#define SHARP_QHD_ID 0x8000
#define HX8394A_720P_ID 0x83941A
#define HX8394D_JT_720P_ID 0x83940D
#define OTM8018B_ID 0x8009
#define OTM1283A_ID 0x1283
#define FL10802_ID 0x307
#define TFT_1080P_ID 0x96

void panel_1080p_init_config(struct mmp_disp_info *fbi);
void panel_720p_init_config(struct mmp_disp_info *fbi);
void lg_720p_init_config(struct mmp_disp_info *fbi);
void panel_qhd_init_config(struct mmp_disp_info *fbi);
void panel_exit_config(struct mmp_disp_info *fbi);
void hx8394a_init_config(struct mmp_disp_info *fbi);
void otm8018b_init_config(struct mmp_disp_info *fbi);
void panel_1080p_tft_init_config(struct mmp_disp_info *fbi);
void pwm_turn_on_backlight(struct mmp_disp_info *fbi);
void otm1283a_init_config(struct mmp_disp_info *fbi);
void fl10802_init_config(struct mmp_disp_info *fbi);
void hx8394d_jt_init_config(struct mmp_disp_info *fbi);

#endif /*__PANEL_H__*/

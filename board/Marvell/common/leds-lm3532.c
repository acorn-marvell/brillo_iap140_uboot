 /*
 * Copyright (C) 2008-2009 Motorola, Inc.
 * Author: Alina Yakovleva <qvdh43@motorola.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include    <common.h>
#include    <i2c.h>
#include    <asm/errno.h>
#include    <leds-lm3532.h>

struct lm3532_reg {
	const char *name;
	uint8_t reg;
} lm3532_r0_regs[] = {
	{"OUTPUT_CFG_REG",    LM3532_OUTPUT_CFG_REG},
	{"START_UP_RAMP_REG", LM3532_START_UP_RAMP_REG},
	{"RUN_TIME_RAMP_REG", LM3532_RUN_TIME_RAMP_REG},
	{"CTRL_A_PWM_REG",    LM3532_CTRL_A_PWM_REG},
	{"CTRL_B_PWM_REG",    LM3532_CTRL_B_PWM_REG},
	{"CTRL_C_PWM_REG",    LM3532_CTRL_C_PWM_REG},
	{"CTRL_A_BR_CFG_REG", LM3532_CTRL_A_BR_CFG_REG},
	{"CTRL_B_BR_CFG_REG", LM3532_R0_CTRL_B_BR_CFG_REG},
	{"CTRL_C_BR_CFG_REG", LM3532_R0_CTRL_C_BR_CFG_REG},
	{"ENABLE_REG",        LM3532_R0_ENABLE_REG},
	{"ALS1_RES_SEL_REG",  LM3532_ALS1_RES_SEL_REG},
	{"ALS2_RES_SEL_REG",  LM3532_ALS2_RES_SEL_REG},
	{"ALS_CFG_REG",       LM3532_R0_ALS_CFG_REG},
	{"ALS_ZONE_REG",      LM3532_R0_ALS_ZONE_REG},
	{"ALS_BR_ZONE_REG",   LM3532_R0_ALS_BR_ZONE_REG},
	{"ALS_UP_ZONE_REG",   LM3532_R0_ALS_UP_ZONE_REG},
	{"IND_BLINK1_REG",    LM3532_R0_IND_BLINK1_REG},
	{"IND_BLINK2_REG",    LM3532_R0_IND_BLINK2_REG},
	{"ZB1_REG",           LM3532_ZB1_REG},
	{"ZB2_REG",           LM3532_ZB2_REG},
	{"ZB3_REG",           LM3532_ZB3_REG},
	{"ZB4_REG",           LM3532_ZB4_REG},
	{"CTRL_A_ZT1_REG",    LM3532_CTRL_A_ZT1_REG},
	{"CTRL_A_ZT2_REG",    LM3532_CTRL_A_ZT2_REG},
	{"CTRL_A_ZT3_REG",    LM3532_CTRL_A_ZT3_REG},
	{"CTRL_A_ZT4_REG",    LM3532_CTRL_A_ZT4_REG},
	{"CTRL_A_ZT5_REG",    LM3532_CTRL_A_ZT5_REG},
	{"CTRL_B_ZT1_REG",    LM3532_CTRL_B_ZT1_REG},
	{"CTRL_B_ZT2_REG",    LM3532_CTRL_B_ZT2_REG},
	{"CTRL_B_ZT3_REG",    LM3532_CTRL_B_ZT3_REG},
	{"CTRL_B_ZT4_REG",    LM3532_CTRL_B_ZT4_REG},
	{"CTRL_B_ZT5_REG",    LM3532_CTRL_B_ZT5_REG},
	{"CTRL_C_ZT1_REG",    LM3532_CTRL_C_ZT1_REG},
	{"CTRL_C_ZT2_REG",    LM3532_CTRL_C_ZT2_REG},
	{"CTRL_C_ZT3_REG",    LM3532_CTRL_C_ZT3_REG},
	{"CTRL_C_ZT4_REG",    LM3532_CTRL_C_ZT4_REG},
	{"CTRL_C_ZT5_REG",    LM3532_CTRL_C_ZT5_REG},
	{"VERSION_REG",       LM3532_VERSION_REG},
};

struct lm3532_reg lm3532_regs[] = {
	{"OUTPUT_CFG_REG",      LM3532_OUTPUT_CFG_REG},
	{"START_UP_RAMP_REG",   LM3532_START_UP_RAMP_REG},
	{"RUN_TIME_RAMP_REG",   LM3532_RUN_TIME_RAMP_REG},
	{"CTRL_A_PWM_REG",      LM3532_CTRL_A_PWM_REG},
	{"CTRL_B_PWM_REG",      LM3532_CTRL_B_PWM_REG},
	{"CTRL_C_PWM_REG",      LM3532_CTRL_C_PWM_REG},
	{"CTRL_A_BR_CFG_REG",   LM3532_CTRL_A_BR_CFG_REG},
	{"CTRL_A_FS_CURR_REG",  LM3532_CTRL_A_FS_CURR_REG},
	{"CTRL_B_BR_CFG_REG",   LM3532_CTRL_B_BR_CFG_REG},
	{"CTRL_B_FS_CURR_REG",  LM3532_CTRL_B_FS_CURR_REG},
	{"CTRL_C_BR_CFG_REG",   LM3532_CTRL_C_BR_CFG_REG},
	{"CTRL_C_FS_CURR_REG",  LM3532_CTRL_C_FS_CURR_REG},
	{"ENABLE_REG",          LM3532_ENABLE_REG},
	{"FEEDBACK_ENABLE_REG", LM3532_FEEDBACK_ENABLE_REG},
	{"ALS1_RES_SEL_REG",    LM3532_ALS1_RES_SEL_REG},
	{"ALS2_RES_SEL_REG",    LM3532_ALS2_RES_SEL_REG},
	{"ALS_CFG_REG",         LM3532_ALS_CFG_REG},
	{"ALS_ZONE_REG",        LM3532_ALS_ZONE_REG},
	{"ALS_BR_ZONE_REG",     LM3532_ALS_BR_ZONE_REG},
	{"ALS_UP_ZONE_REG",     LM3532_ALS_UP_ZONE_REG},
	{"ZB1_REG",             LM3532_ZB1_REG},
	{"ZB2_REG",             LM3532_ZB2_REG},
	{"ZB3_REG",             LM3532_ZB3_REG},
	{"ZB4_REG",             LM3532_ZB4_REG},
	{"CTRL_A_ZT1_REG",      LM3532_CTRL_A_ZT1_REG},
	{"CTRL_A_ZT2_REG",      LM3532_CTRL_A_ZT2_REG},
	{"CTRL_A_ZT3_REG",      LM3532_CTRL_A_ZT3_REG},
	{"CTRL_A_ZT4_REG",      LM3532_CTRL_A_ZT4_REG},
	{"CTRL_A_ZT5_REG",      LM3532_CTRL_A_ZT5_REG},
	{"CTRL_B_ZT1_REG",      LM3532_CTRL_B_ZT1_REG},
	{"CTRL_B_ZT2_REG",      LM3532_CTRL_B_ZT2_REG},
	{"CTRL_B_ZT3_REG",      LM3532_CTRL_B_ZT3_REG},
	{"CTRL_B_ZT4_REG",      LM3532_CTRL_B_ZT4_REG},
	{"CTRL_B_ZT5_REG",      LM3532_CTRL_B_ZT5_REG},
	{"CTRL_C_ZT1_REG",      LM3532_CTRL_C_ZT1_REG},
	{"CTRL_C_ZT2_REG",      LM3532_CTRL_C_ZT2_REG},
	{"CTRL_C_ZT3_REG",      LM3532_CTRL_C_ZT3_REG},
	{"CTRL_C_ZT4_REG",      LM3532_CTRL_C_ZT4_REG},
	{"CTRL_C_ZT5_REG",      LM3532_CTRL_C_ZT5_REG},
	{"VERSION_REG",         LM3532_VERSION_REG},
};

#ifdef LM3532_DEBUG
static const char *lm3532_reg_name(struct lm3532_data *pdata, int reg)
{
	unsigned reg_count;
	int i;

	if (pdata->revision == 0xF6) {
		reg_count = sizeof(lm3532_r0_regs) / sizeof(lm3532_r0_regs[0]);
		for (i = 0; i < reg_count; i++)
			if (reg == lm3532_r0_regs[i].reg)
				return lm3532_r0_regs[i].name;
	} else {
		reg_count = sizeof(lm3532_regs) / sizeof(lm3532_regs[0]);
		for (i = 0; i < reg_count; i++)
			if (reg == lm3532_regs[i].reg)
				return lm3532_regs[i].name;
	}
	return "UNKNOWN";
}
#endif

static int lm3532_read_reg(struct lm3532_data *pdata,
		unsigned reg, uint8_t *value)
{
	uint8_t buf[1];
	int ret = 0;
	unsigned int bus = i2c_get_bus_num();

	if (!value)
		return -EINVAL;
	i2c_set_bus_num(pdata->i2c_bus_num);
	buf[0] = reg;
	ret = i2c_read(pdata->addr, reg, 1, buf, 1);
	*value = buf[0];
	i2c_set_bus_num(bus);

	return ret;
}

static int lm3532_write_reg(struct lm3532_data *pdata,
		unsigned reg, uint8_t value, const char *caller)
{
	int ret = 0;
	unsigned int bus = i2c_get_bus_num();

	i2c_set_bus_num(pdata->i2c_bus_num);
	ret = i2c_write(pdata->addr, reg, 1, &value, 1);
	if (ret < 0)
		printf("%s: i2c_master_send error %d\n",
				caller, ret);
	i2c_set_bus_num(bus);

	return ret;
}

/*
 * This function calculates ramp step time so that total ramp time is
 * equal to ramp_time defined currently at 200ms
 */
static int lm3532_set_ramp(struct lm3532_data *pdata,
		unsigned int on, unsigned int nsteps, unsigned int *rtime)
{
	int ret, i = 0;
	uint8_t value = 0;
	unsigned int total_time = 0;
	/* Ramp times in microseconds */
	unsigned int lm3532_ramp[] = {8, 1000, 2000, 4000, 8000, 16000, 32000,
		64000};
	int nramp = sizeof(lm3532_ramp) / sizeof(lm3532_ramp[0]);
	unsigned ramp_time = pdata->ramp_time * 1000;

	if (on) {
		/* Calculate the closest possible ramp time */
		for (i = 0; i < nramp; i++) {
			total_time = nsteps * lm3532_ramp[i];
			if (total_time >= ramp_time)
				break;
		}
		if (i > 0 && total_time > ramp_time) {
			i--;
			total_time = nsteps * lm3532_ramp[i];
		}
		value = i | (i << 3);
	} else
		value = 0;

	if (rtime)
		*rtime = total_time;
	ret = lm3532_write_reg(pdata, LM3532_RUN_TIME_RAMP_REG,
			value, __func__);
	ret = lm3532_write_reg(pdata, LM3532_START_UP_RAMP_REG,
			value, __func__);
	return ret;
}

static void lm3532_brightness_set(struct lm3532_data *pdata,
		int value)
{
	int nsteps = 0;
	unsigned int total_time = 0;
	unsigned do_ramp = 1;

	if (pdata->ramp_time == 0)
		do_ramp = 0;

	/* Calculate number of steps for ramping */
	nsteps = value - pdata->bvalue;
	if (nsteps < 0)
		nsteps = -nsteps;

	if (do_ramp) {
		lm3532_set_ramp(pdata, do_ramp, nsteps,
				&total_time);
		printf("%s: 0x%x => 0x%x, %d steps, RT=%dus\n",
				__func__, pdata->bvalue,
				value, nsteps, total_time);
	}

	lm3532_write_reg(pdata, LM3532_CTRL_A_ZT1_REG, value,
			__func__);
	pdata->bvalue = value;
}

static int lm3532_configure(struct lm3532_data *pdata)
{
	int ret = 0;
	static uint8_t r0_fs_current[] = {
		LM3532_R0_5mA_FS_CURRENT,       /* 0x00000 */
		LM3532_R0_5mA_FS_CURRENT,       /* 0x00001 */
		LM3532_R0_8mA_FS_CURRENT,       /* 0x00010 */
		LM3532_R0_8mA_FS_CURRENT,       /* 0x00011 */
		LM3532_R0_8mA_FS_CURRENT,       /* 0x00100 */
		LM3532_R0_8mA_FS_CURRENT,       /* 0x00101 */
		LM3532_R0_8mA_FS_CURRENT,       /* 0x00110 */
		LM3532_R0_12mA_FS_CURRENT,      /* 0x00111 */
		LM3532_R0_12mA_FS_CURRENT,      /* 0x01000 */
		LM3532_R0_12mA_FS_CURRENT,      /* 0x01001 */
		LM3532_R0_12mA_FS_CURRENT,      /* 0x01010 */
		LM3532_R0_15mA_FS_CURRENT,      /* 0x01011 */
		LM3532_R0_15mA_FS_CURRENT,      /* 0x01100 */
		LM3532_R0_15mA_FS_CURRENT,      /* 0x01101 */
		LM3532_R0_15mA_FS_CURRENT,      /* 0x01110 */
		LM3532_R0_19mA_FS_CURRENT,      /* 0x01111 */
		LM3532_R0_19mA_FS_CURRENT,      /* 0x10000 */
		LM3532_R0_19mA_FS_CURRENT,      /* 0x10001 */
		LM3532_R0_19mA_FS_CURRENT,      /* 0x10010 */
		LM3532_R0_19mA_FS_CURRENT,      /* 0x10011 */
		LM3532_R0_22mA_FS_CURRENT,      /* 0x10100 */
		LM3532_R0_22mA_FS_CURRENT,      /* 0x10101 */
		LM3532_R0_22mA_FS_CURRENT,      /* 0x10110 */
		LM3532_R0_22mA_FS_CURRENT,      /* 0x10111 */
		LM3532_R0_22mA_FS_CURRENT,      /* 0x11000 */
		LM3532_R0_26mA_FS_CURRENT,      /* 0x11001 */
		LM3532_R0_26mA_FS_CURRENT,      /* 0x11010 */
		LM3532_R0_26mA_FS_CURRENT,      /* 0x11011 */
		LM3532_R0_26mA_FS_CURRENT,      /* 0x11100 */
		LM3532_R0_29mA_FS_CURRENT,      /* 0x11101 */
		LM3532_R0_29mA_FS_CURRENT,      /* 0x11110 */
		LM3532_R0_29mA_FS_CURRENT,      /* 0x11111 */
	};
	uint8_t ctrl_a_fs_current;

	if (pdata->revision == 0xF6) {
		/* Revision 0 */
		ctrl_a_fs_current =
			r0_fs_current[pdata->ctrl_a_fs_current];
		/* Map ILED1 to CTRL A, ILED2 to CTRL B, ILED3 to CTRL C */
		lm3532_write_reg(pdata,
			LM3532_OUTPUT_CFG_REG, 0x34, __func__);
		lm3532_write_reg(pdata,
			LM3532_CTRL_A_BR_CFG_REG,
			LM3532_I2C_CONTROL | ctrl_a_fs_current |
			pdata->ctrl_a_mapping_mode, __func__);

		ret = lm3532_write_reg(pdata,
				LM3532_CTRL_A_ZT2_REG, 0, __func__);
	} else {
		/* Revision 1 and above */
		lm3532_write_reg(pdata,
			LM3532_FEEDBACK_ENABLE_REG, pdata->feedback_en_val,
			__func__);
		lm3532_write_reg(pdata,
			LM3532_CTRL_A_PWM_REG, pdata->ctrl_a_pwm,
			__func__);
		lm3532_write_reg(pdata,
			LM3532_OUTPUT_CFG_REG, pdata->output_cfg_val,
			__func__);
		lm3532_write_reg(pdata,
			LM3532_CTRL_A_BR_CFG_REG,
			LM3532_I2C_CONTROL | \
			pdata->ctrl_a_mapping_mode,
			__func__);

		lm3532_write_reg(pdata,
			LM3532_CTRL_A_FS_CURR_REG,
			pdata->ctrl_a_fs_current,
			__func__);

		ret = lm3532_write_reg(pdata,
				LM3532_CTRL_B_ZT1_REG, 0, __func__);
	}
	if (pdata->ramp_time == 0) {
		lm3532_write_reg(pdata,
			LM3532_START_UP_RAMP_REG , 0x0, __func__);
		lm3532_write_reg(pdata,
			LM3532_RUN_TIME_RAMP_REG , 0x0, __func__);
	}

	return ret;
}

static int lm3532_enable(struct lm3532_data *pdata)
{
	lm3532_write_reg(pdata,
		pdata->enable_reg, LM3532_CTRLA_ENABLE, __func__);
	return 0;
}

int lm3532_read_version(struct lm3532_data *pdata, uint8_t *version_id)
{
	int ret;

	lm3532_write_reg(pdata, LM3532_VERSION_REG, 0, __func__);
	ret = lm3532_read_reg(pdata, LM3532_VERSION_REG, version_id);
	if (ret < 0) {
		printf("%s: unable to read version: %d\n",
				__func__, ret);
		return ret;
	}

	return ret;
}

static int lm3532_setup(struct lm3532_data *pdata)
{
	int ret;
	uint8_t value;

	/* Read revision number, need to write first to get correct value */
	lm3532_write_reg(pdata, LM3532_VERSION_REG, 0, __func__);
	ret = lm3532_read_reg(pdata, LM3532_VERSION_REG, &value);
	if (ret < 0) {
		printf("%s: unable to read from chip: %d\n",
			__func__, ret);
		return ret;
	}
	pdata->revision = value;
	printf("%s: revision 0x%X\n", __func__,
			pdata->revision);
	if (value == 0xF6)
		pdata->enable_reg = LM3532_R0_ENABLE_REG;
	else
		pdata->enable_reg = LM3532_ENABLE_REG;

	ret = lm3532_configure(pdata);
	pdata->bvalue = 255;

	return ret;
}

int lm3532_init(struct lm3532_data *pdata)
{
	int ret = 0;
	printf("%s: enter, I2C address = 0x%x\n",
		__func__, pdata->addr);

	if (pdata->ctrl_a_fs_current > 0xFF)
		pdata->ctrl_a_fs_current = 0xFF;
	if (pdata->ctrl_b_fs_current > 0xFF)
		pdata->ctrl_b_fs_current = 0xFF;

	ret = lm3532_setup(pdata);
	if (ret < 0) {
		printf("%s: lm3532_setup fail\n", __func__);
		goto setup_failed;
	}
	lm3532_enable(pdata);
	lm3532_brightness_set(pdata, 255);

	return 0;

setup_failed:
	return ret;
}

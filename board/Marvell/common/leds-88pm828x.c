 /*
 * Copyright (C) 2013 Marvell, Inc.
 *     Guoqing Li <ligq@marvell.com>
 *     Xiaolong Ye <yexl@marvell.com>
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
#include    <leds-88pm828x.h>

struct pm828x_reg {
	const char *name;
	uint8_t reg;
} pm828x_regs[] = {
	{"CHIP_ID_REG",           PM828X_CHIP_ID},
	{"REV_REG",               PM828X_REV_ID},
	{"STATUS_REG",            PM828X_STATUS},
	{"STATUS_FLAG_REG",       PM828X_STATUS_FLAG},
	{"IDAC_CTRL0_REG",        PM828X_IDAC_CTRL0},
	{"IDAC_CTRL1_REG",        PM828X_IDAC_CTRL1},
	{"IDAC_RAMP_CLK_REG",     PM828X_IDAC_RAMP_CLK_CTRL0},
	{"MISC_CTRL_REG",         PM828X_MISC_CTRL},
	{"IDAC_OUT_CURRENT0_REG", PM828X_IDAC_OUT_CURRENT0},
	{"IDAC_OUT_CURRENT1_REG", PM828X_IDAC_OUT_CURRENT1},
};

#ifdef PM828X_DEBUG
static const char *pm828x_reg_name(struct pm828x_data *driver_data, int reg)
{
	unsigned reg_count;
	int i;

	reg_count = sizeof(pm828x_regs) / sizeof(pm828x_regs[0]);
	for (i = 0; i < reg_count; i++)
		if (reg == pm828x_regs[i].reg)
			return pm828x_regs[i].name;

	return "UNKNOWN";
}
#endif

static int pm828x_read_reg(struct pm828x_data *pdata,
		unsigned reg, uint8_t *value)
{
	uint8_t buf[1];
	int ret = 0;
	unsigned int bus = i2c_get_bus_num();

	if (!value)
		return -EINVAL;
	i2c_set_bus_num(pdata->i2c_num);
	buf[0] = reg;
	ret = i2c_read(pdata->addr, reg, 1, buf, 1);
	*value = buf[0];
	i2c_set_bus_num(bus);

	return ret;
}

static int pm828x_write_reg(struct pm828x_data *pdata,
		unsigned reg, uint8_t value, const char *caller)
{
	int ret = 0;
	unsigned int bus = i2c_get_bus_num();

	i2c_set_bus_num(pdata->i2c_num);
	ret = i2c_write(pdata->addr, reg, 1, &value, 1);
	if (ret < 0)
		printf("%s: i2c_master_send error %d\n",
				caller, ret);
	i2c_set_bus_num(bus);

	return ret;
}

#if 0
static void pm828x_brightness_set(struct pm828x_data *pdata,
		int value)
{
	u8 val;
	int cur;

	if (pdata->bvalue == value && value < LED_MIN
			&& value > LED_MAX)
		return;

	cur = 0xA00 * value / LED_MAX;
	/* set lower 8-bit of the current */
	val = cur & 0xFF;
	pm828x_write_reg(pdata, PM828X_IDAC_CTRL0, val, __func__);

	/* select ramp mode and upper 4-bit of the current */
	pm828x_read_reg(pdata, PM828X_IDAC_CTRL1, &val);
	val &= ~0xF;
	val |= ((cur >> 8) & 0xF);
	pm828x_write_reg(pdata, PM828X_IDAC_CTRL1, val, __func__);

	pdata->bvalue = value;
}
#endif

static void pm828x_configure(struct pm828x_data *pdata)
{
	u8 val;

	/* set lower 8-bit of the current */
	val = pdata->idac_current & 0xFF;
	pm828x_write_reg(pdata, PM828X_IDAC_CTRL0, val, __func__);

	/* select ramp mode and upper 4-bit of the current */
	val = (pdata->ramp_mode << 5) |
		((pdata->idac_current >> 8) & 0xF);
	pm828x_write_reg(pdata, PM828X_IDAC_CTRL1, val, __func__);

	/* select ramp clk */
	val = pdata->ramp_clk;
	pm828x_write_reg(pdata,
			PM828X_IDAC_RAMP_CLK_CTRL0, val, __func__);

	/* set string configuration */
	val = (pdata->str_config << 1) | (~(pdata->str_config));
	/*
	 * FIXME: enable shut down mode, pwm input and sleep mode.
	 * shutdown mode maybe cound not enter,
	 * most time the i2c bus were shared and pulled high.
	 * Fortunately, we could call power on/off instead.
	 */
	val |= PM828X_EN_SHUTDN | PM828X_EN_PWM | PM828X_EN_SLEEP;
	pm828x_write_reg(pdata, PM828X_MISC_CTRL, val, __func__);
}

int pm828x_read_id(struct pm828x_data *pdata, uint8_t *chip_id)
{
	int ret;

	ret = pm828x_read_reg(pdata, PM828X_CHIP_ID, chip_id);
	if (ret < 0) {
		printf("%s: unable to read chip id: %d\n",
				__func__, ret);
		return ret;
	}

	return ret;
}

static int pm828x_setup(struct pm828x_data *pdata)
{
	int ret;
	u8 value;

	ret = pm828x_read_reg(pdata, PM828X_REV_ID, &value);
	if (ret < 0) {
		printf("%s: unable to read chip revision: %d\n",
			__func__, ret);
		return ret;
	}
	pdata->revision = value;
	printf("%s: revision 0x%X\n", __func__,
			pdata->revision);

	pm828x_configure(pdata);

	return ret;
}

int pm828x_init(struct pm828x_data *pdata)
{
	printf("%s: enter, I2C address = 0x%x\n",
		__func__, pdata->addr);

	if (pdata->idac_current > 0xA00)
		pdata->idac_current = 0xA00;

	return pm828x_setup(pdata);
}

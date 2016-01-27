/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __88PM830_H_
#define __88PM830_H_

#include <power/marvell88pm_pmic.h>

/* 88pm830 registers */
enum {
	PM830_REG_PMIC_ID = 0x0,
	PM830_REG_STATUS = 0x1,
	PM830_REG_INT1 = 0x4,
	PM830_REG_INT2 = 0x5,
	PM830_REG_INT3 = 0x6,
	PM830_REG_INT1MSK = 0x8,
	PM830_REG_INT2MSK = 0x9,
	PM830_REG_INT3MSK = 0xa,
	PM830_REG_MISC1 = 0x10,
	PM830_REG_FG_CTRL1 = 0x24,
	PM830_REG_FG_CTRL2 = 0x32,
	PM830_REG_CHG_CTRL1 = 0x3c,
	PM830_REG_CHG_CTRL2 = 0x3e,
	PM830_REG_CHG_CTRL3 = 0x3f,
	PM830_REG_CHG_CTRL4 = 0x41,
	PM830_REG_CHG_CTRL5 = 0x42,
	PM830_REG_CHG_CTRL6 = 0x43,
	PM830_REG_CHG_CTRL7 = 0x44,
	PM830_REG_CHG_CTRL8 = 0x46,
	PM830_REG_CHG_CTRL9 = 0x4f,
	PM830_REG_CHG_FLAG = 0x50,
	PM830_REG_CHG_CTRL10 = 0x51,
	PM830_REG_MPPT_CTRL1 = 0x54,
	PM830_REG_MPPT_CTRL2 = 0x55,
	PM830_REG_CHG_STATUS1 = 0x57,
	PM830_REG_CHG_STATUS2 = 0x58,
	PM830_REG_MEAS_EN = 0x6A,
	PM830_REG_CHG_BAT_DET = 0x6e,
	PM830_REG_BIAS_ENA = 0x6f,
	PM830_REG_BIAS = 0x70,
	PM830_REG_VBAT_VAL1 = 0x8a,
	PM830_REG_VBAT_VAL2 = 0x8b,
	PM830_REG_MEAS1 = 0x94,
	PM830_REG_MEAS2 = 0x95,
	PM830_REG_VBAT_AVG1 = 0xba,
	PM830_REG_NUM_OF_REGS = 0xc6,
};

#define PM830_A1_VERSION 0x9
#define PM830_B0_VERSION 0x10
#define PM830_B1_VERSION 0x11

#define PM830_BAT_PRESENT	(1 << 1)
#define PM830_CHG_PRESENT	(1 << 0)
#define PM830_ICHG_FAST_MASK	(0xf)
#define PM830_CHG_STAT_POWER_UP	(0 << 5)
#define PM830_CHG_STAT_PRE_REG	(2 << 5)
#define PM830_CHG_STAT_PRE_CHG	(3 << 5)
#define PM830_CHG_STAT_FAST_CHG	(4 << 5)
#define PM830_CHG_STAT_SUPPL	(5 << 5)
#define PM830_CHG_STAT_BATT_PWR	(6 << 5)
#define PM830_CHG_STAT_BOOST	(7 << 5)

#define PM830_CHG_ENABLE	(1 << 0)
#define PM830_BAT_SHRT		(1 << 4)
/* vbat_voltage = (vbat * 5.6) >> 12, if vbat_voltage = 2.2V, vbat should be 0x649. */
#define PM830_VBAT_2V2		(0x649)

#define PM830_BAT_SHRT_EN	(1 << 3)
#define PM830_BAT_DET_EN	(1 << 5)
#define PM830_BAT_PRTY_EN	(1 << 1)

#define PM830_GPADC0_BIAS_EN		(1 << 4)
#define PM830_GPADC0_MEAS_EN		(1 << 5)
#define PM830_GPADC0_GP_BIAS_OUT	(1 << 6)

#define BIAS_GP0_MASK                       (0xF)
#define BIAS_GP0_SET(x)                     ((x - 1) / 5)

#define CHARGER_MIN_CURRENT 200
#define CHARGER_MAX_CURRENT 2000

#define PM830_I2C_ADDR	0x68

#endif /* __88PM830_H_ */

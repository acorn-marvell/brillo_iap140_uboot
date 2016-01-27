/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __88PM880_H_
#define __88PM880_H_

#include <power/marvell88pm_pmic.h>

/* 88PM880 registers */
enum {
	/* base page */
	PM880_REG_STATUS = 0x1,
	PM880_REG_INT1 = 0x5,
	PM880_REG_INT2 = 0x6,
	PM880_REG_INT3 = 0x7,
	PM880_REG_INT4 = 0x8,
	PM880_REG_INT1MSK = 0x0a,
	PM880_REG_INT2MSK = 0x0b,
	PM880_REG_INT3MSK = 0x0c,
	PM880_REG_INT4MSK = 0x0d,
	PM880_REG_WAKE_UP = 0x14,
	PM880_REG_WDOG = 0x1d,
	PM880_REG_LOWPOWER1 = 0x20,
	PM880_REG_LOWPOWER2 = 0x21,
	PM880_REG_CLK_CTRL_1 = 0x25,
	PM880_REG_GPIO_CTRL4 = 0x33,
	PM880_REG_GPIO_CTRL5 = 0x34,
	PM880_REG_RTC_CTRL1 = 0xd0,
	PM880_REG_AON_CTRL2 = 0xe2,

	/* ldo page */
	PM880_LDO1_8_EN1 = 0x09,
	PM880_LDO9_16_EN1 = 0x0a,
	PM880_LDO17_18_EN1 = 0x0b,

	/* gpadc page */
	PM880_GPADC_CONFIG1 = 0x1,
	PM880_GPADC_CONFIG2 = 0x2,
	PM880_GPADC_CONFIG5 = 0x5,
	PM880_GPADC_CONFIG6 = 0x6,
	PM880_GPADC_CONFIG7 = 0x7,
	PM880_GPADC_CONFIG8 = 0x8,
	PM880_VBAT_AVG1 = 0xa0,
	PM880_VBAT_AVG2 = 0xa1,

	/* led page */
	PM880_CFD_CONFIG_4 = 0x64,

	/* battery page */
	PM880_CHG_ID = 0x0,
	PM880_CHG_CC_CONFIG1 = 0x1,
	PM880_CHG_CONFIG1 = 0x28,
	PM880_CHG_CONFIG3 = 0x2a,
	PM880_CHG_CONFIG4 = 0x2b,
	PM880_CHG_PRE_CONFIG1 = 0x2d,
	PM880_CHG_FAST_CONFIG1 = 0x2e,
	PM880_CHG_FAST_CONFIG2 = 0x2f,
	PM880_CHG_FAST_CONFIG3 = 0x30,
	PM880_CHG_TIMER_CONFIG = 0x31,
	PM880_CHG_EXT_ILIM_CONFIG = 0x34,
	PM880_CHG_MPPT_CONFIG1 = 0x3e,
	PM880_CHG_MPPT_CONFIG2 = 0x3f,
	PM880_CHG_LOG1 = 0x45,
	PM880_OTG_LOG1 = 0x47,

	/* buck page */
	PM880_BUCK1_EN = 0x08,
	PM880_BUCK1_RAMP = 0x20,
	PM880_BUCK1_DUAL = 0x25,
	PM880_ID_BUCK1 = 0x28,
	PM880_ID_BUCK2 = 0x58,
	PM880_ID_BUCK3 = 0x70,
	PM880_ID_BUCK4 = 0x88,
	PM880_ID_BUCK5 = 0x98,
	PM880_ID_BUCK6 = 0xa8,
	PM880_ID_BUCK7 = 0xb8,
};

/* base page */
#define PM880_REG_WDOG_DIS	(1 << 0)
#define PM880_REG_XO_LJ		(1 << 5)
#define PM880_REG_USE_XO	(1 << 7)
#define PM880_CLK32K2_SEL_MASK	(0x3 << 2)
#define PM880_CLK32K2_SEL_32	(0x2 << 2)
#define PM880_SW_PDOWN		(1 << 5)
#define PM880_CHG_INT_EN	(1 << 2)
#define PM880_BAT_INT_EN	(1 << 3)
#define PM880_ITEMP_INT_EN	(1 << 3)
#define PM880_CHG_FAIL_INT_EN	(1 << 0)
#define PM880_CHG_DONE_INT_EN	(1 << 1)
#define PM880_RGB_CLK_EN	(1 << 0)
#define PM880_CFD_CLK_EN	(1 << 1)

/* gpadc page */
#define PM880_GPADC_VBAT_EN	(1 << 1)
#define PM880_GPADC_VBUS_EN	(1 << 4)
#define PM880_GPADC_TINT_EN	(1 << 0)
#define PM880_GPADC_GPADC1_EN	(1 << 3)
#define PM880_GPADC_EN		(1 << 0)
#define PM880_GPADC_NON_STOP	(1 << 1)
#define PM880_BD_GP_SEL_3	(1 << 6)
#define PM880_BD_EN		(1 << 5)
#define PM880_BD_PREBIAS	(1 << 4)

/* led page */
#define PM880_CFD_PLS_EN	(1 << 0)

/* buck page */
#define PM880_BK1A_RAMP_MASK	(0x7 << 3)
#define PM880_BK1A_RAMP_12P5	(0x6 << 3)
#define PM880_BK1A_DUAL_SEL	(1 << 0)

/* battery page */
#define PM880_USB_SW		(1 << 0)
#define PM880_CHG_WDG_EN	(1 << 1)
#define PM880_CHG_PRESENT	(1 << 2)
#define PM880_BAT_PRESENT	(1 << 3)
#define PM880_CHG_ENABLE	(1 << 0)
#define PM880_ICHG_FAST_MASK	(0x3f)
#define PM880_FASTCHG_TIMEOUT	(0x7)

#endif /* __88PM880_H_ */

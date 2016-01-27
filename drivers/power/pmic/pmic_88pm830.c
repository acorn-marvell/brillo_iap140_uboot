/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <power/88pm830.h>
#include <power/battery.h>
#include <errno.h>

#define MARVELL_PMIC_BATTERY		"88pm830_battery"

static int pm830_enable_bat_det(struct pmic *p, int enable)
{
	u32 val;

	pmic_reg_read(p, PM830_REG_CHG_BAT_DET, &val);
	if (!!enable)
		val |= PM830_BAT_DET_EN;
	else
		val &= ~PM830_BAT_DET_EN;
	pmic_reg_write(p, PM830_REG_CHG_BAT_DET, val);

	return 0;
}

static int pm830_enable_bat_priority(struct pmic *p, int enable)
{
	u32 val;

	pmic_reg_read(p, PM830_REG_CHG_CTRL3, &val);
	if (!!enable)
		val |= PM830_BAT_PRTY_EN;
	else
		val &= ~PM830_BAT_PRTY_EN;
	pmic_reg_write(p, PM830_REG_CHG_CTRL3, val);

	return 0;
}

/*
 * the method used to get power up mode for 88pm830:
 * step 1. set BD_EN, if battery present, it's BATTERY_MODE
 * step 2. clear BD_PRTY_EN, if VBAT is valid, it's POWER_SUPPLY_MODE
 * step 3. clear BD_EN, set BD_PRTY_EN, if VBAT is valid, it's EXTERNAL_POWER_MODE
 * step 4. if it's CHARGER_ONLY_MODE, system will shutdown in step 3
 */
int pm830_get_powerup_mode(void)
{
	int ret;
	struct pmic *p_bat, *p;

	p_bat = pmic_get(MARVELL_PMIC_BATTERY);
	if (!p_bat) {
		printf("find battery failed\n");
		return INVALID_MODE;
	}
	p = p_bat->pbat->chrg;

	/* disable the auto battery detection circuit */
	pm830_enable_bat_det(p, 0);
	pm830_enable_bat_priority(p, 0);

	ret = p_bat->chrg->chrg_bat_present(p);
	if (!!ret) {
		/* it's battery mode */
		ret = BATTERY_MODE;
		goto handler;
	}

	p_bat->fg->fg_battery_update(p_bat->pbat->fg, p_bat);
	if (p_bat->pbat->bat->voltage_uV > POWER_OFF_THRESHOLD) {
		/* it's power supply mode */
		ret = POWER_SUPPLY_MODE;
		goto handler;
	}

	printf("if you are in charger-only mode, system will power-off now.\n");
	/* set BD_PRTY_EN */
	pm830_enable_bat_priority(p, 1);
	p_bat->fg->fg_battery_update(p_bat->pbat->fg, p_bat);
	if (p_bat->pbat->bat->voltage_uV > POWER_OFF_THRESHOLD) {
		/* it's external power mode */
		ret = EXTERNAL_POWER_MODE;
		goto handler;
	} else {
		/*
		 * the program will not run to here, since if it's charger only mode, the system
		 * already power off
		 */
		ret = CHARGER_ONLY_MODE;
	}

handler:
	switch (ret) {
	case BATTERY_MODE:
		/* in order to handle dead battery case, use charger to supply system */
		printf("battery mode\n");
		break;
	case POWER_SUPPLY_MODE:
	case EXTERNAL_POWER_MODE:
		printf("power-supply mode\n");
		/* use battery supply system */
		pm830_enable_bat_priority(p, 1);
		break;
	case CHARGER_ONLY_MODE:
	default:
		break;
	}

	return ret;
}

/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>
#include <power/marvell88pm_pmic.h>
#include <errno.h>

#define MARVELL_PMIC_BATTERY	"88pm88x_battery"

static struct battery pm88x_bat;

static int pm88x_battery_charge(struct pmic *bat, unsigned int voltage)
{
	struct power_battery *p_bat = bat->pbat;
	struct battery *battery = p_bat->bat;

	/* since MPPT has enabled, we set fast charge current limit as 1500mA */
	if (bat->chrg->chrg_state(p_bat->chrg, CHARGER_ENABLE, 1500))
		return -1;

	printf("start charging until %d[uV]\n", voltage);

	/* loop until battery voltage raise up to voltage threshold */
	do {
		udelay(2000000);
		bat->fg->fg_battery_update(p_bat->fg, bat);
		printf("%d [uV]\n", battery->voltage_uV);

		if (!bat->chrg->chrg_bat_present(p_bat->chrg)) {
			printf("battery removed!\n");
			goto out;
		}

		if (!bat->chrg->chrg_type(p_bat->chrg)) {
			printf("charger removed!\n");
			goto out;
		}

		if (ctrlc()) {
			printf("Charging disabled on request!\n");
			goto exit;
		}
	} while (battery->voltage_uV < voltage);

out:
	printf("finish!\n");
	return 0;
exit:
	bat->chrg->chrg_state(p_bat->chrg, CHARGER_DISABLE, 0);
	return 0;
}

static int pm88x_battery_init(struct pmic *bat_,
				    struct pmic *fg_,
				    struct pmic *chrg_,
				    struct pmic *muic_)
{
	struct pmic *p = chrg_;

	if (!p) {
		printf("Charger invalid\n");
		return -1;
	}

	bat_->pbat->fg = fg_;
	bat_->pbat->chrg = chrg_;
	bat_->pbat->muic = muic_;

	if (fg_)
		bat_->fg = fg_->fg;

	if (chrg_)
		bat_->chrg = chrg_->chrg;

	if (muic_)
		bat_->chrg->chrg_type = muic_->chrg->chrg_type;

	return 0;
}

static struct power_battery power_bat_ops = {
	.bat = &pm88x_bat,
	.battery_init = pm88x_battery_init,
	.battery_charge = pm88x_battery_charge,
};

int pm88x_bat_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	static const char name[] = MARVELL_PMIC_BATTERY;
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board BAT init\n");

	if (!chip) {
		printf("%s: PMIC chip is empty.\n", __func__);
		return -EINVAL;
	}
	chip->battery_name = name;

	p->bus = bus;
	p->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 3;
	p->hw.i2c.tx_num = 1;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->interface = PMIC_I2C;
	p->name = name;

	p->pbat = &power_bat_ops;
	return 0;
}

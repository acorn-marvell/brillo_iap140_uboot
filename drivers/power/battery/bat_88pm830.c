/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>
#include <power/88pm830.h>
#include <errno.h>

#define MARVELL_PMIC_BATTERY		"88pm830_battery"

static struct battery pm830_bat;

static int pm830_battery_charge(struct pmic *bat, unsigned int voltage)
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

static int pm830_battery_init(struct pmic *bat_,
				    struct pmic *fg_,
				    struct pmic *chrg_,
				    struct pmic *muic_)
{
	struct pmic *p = chrg_;
	int ret, version;
	u32 data;

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

	/* init PM830 */
	if (pmic_probe(p)) {
		printf("Can't find 88pm830 battery\n");
		return -1;
	}

	/* read chip version */
	ret = pmic_reg_read(p, PM830_REG_PMIC_ID, &data);
	if (ret != 0) {
		printf("read Lipari ID error: return ...\n");
		return -1;
	} else
		printf("Lipari ID = 0x%x\n", data);
	version = data;

	/* set charging sequence */
	data = 0x7;
	pmic_reg_write(p, PM830_REG_MISC1, data);

	data = 0x6a;
	pmic_reg_write(p, PM830_REG_FG_CTRL1, data);

	data = 0x1;
	pmic_reg_write(p, PM830_REG_FG_CTRL2, data);

	data = 0x7b;
	pmic_reg_write(p, PM830_REG_INT1MSK, data);

	/* change battery short threshold to 2.0v */
	data = 0x9;
	pmic_reg_write(p, PM830_REG_CHG_CTRL2, data);

	data = 0x1f;
	pmic_reg_write(p, PM830_REG_CHG_CTRL4, data);

	data = 0x6f;
	pmic_reg_write(p, PM830_REG_CHG_CTRL5, data);

	data = 0x88;
	pmic_reg_write(p, PM830_REG_CHG_CTRL6, data);

	data = 0x02;
	pmic_reg_write(p, PM830_REG_CHG_CTRL7, data);

	data = 0x07;
	pmic_reg_write(p, PM830_REG_CHG_CTRL8, data);

	/* enable VBUS_SW */
	data = 0x01;
	pmic_reg_write(p, PM830_REG_CHG_CTRL3, data);

	/* read and clear interruption registers */
	pmic_reg_read(p, PM830_REG_INT1, &data);
	pmic_reg_read(p, PM830_REG_INT2, &data);
	pmic_reg_read(p, PM830_REG_INT3, &data);

	/* enable MPPT loop */
	data = 0x02;
	pmic_reg_write(p, PM830_REG_MPPT_CTRL1, data);
	data = 0xff;
	pmic_reg_write(p, PM830_REG_MPPT_CTRL2, data);

	if (version > PM830_A1_VERSION) {
		/* disable charger watchdog and clear SUPPL_PRE_DIS bit */
		pmic_reg_read(p, PM830_REG_CHG_CTRL9, &data);
		data |= 0x10;
		pmic_reg_write(p, PM830_REG_CHG_CTRL9, data);
		pmic_reg_read(p, PM830_REG_CHG_CTRL2, &data);
		data &= ~0x40;
		pmic_reg_write(p, PM830_REG_CHG_CTRL2, data);
	}

	return 0;
}

static struct power_battery power_bat_ops = {
	.bat = &pm830_bat,
	.battery_init = pm830_battery_init,
	.battery_charge = pm830_battery_charge,
};

int pm830_bat_alloc(unsigned char bus, struct pmic_chip_desc *chip)
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

	p->interface = PMIC_NONE;
	p->name = name;
	p->bus = bus;

	p->pbat = &power_bat_ops;
	return 0;
}

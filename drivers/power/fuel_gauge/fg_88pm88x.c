/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <i2c.h>
#include <power/power_chrg.h>
#include <power/battery.h>
#include <power/marvell88pm_pmic.h>
#include <errno.h>

#define MARVELL_PMIC_FG		"88pm88x_fuelgauge"
#define MARVELL_PMIC_GPADC	"88pm88x_gpadc"

/* base page registers */
#define PM88X_CHG_ID		0x0

/* gpadc page registers */
#define PM88X_VBAT_AVG1		0xa0

static int fg_read_regs(struct pmic *p, u8 addr, unsigned char *data, int num)
{
	unsigned int dat;
	int ret = 0;
	int i;

	for (i = 0; i < num; i++, addr++) {
		ret = pmic_reg_read(p, addr, &dat);
		if (ret) {
			printf("access pmic failed...\n");
			return ret;
		}

		*(data + i) = (unsigned char)dat;
	}

	return 0;
}

static int pm88x_get_batt_vol(struct pmic *p)
{
	int ret, data;
	unsigned char buf[2] = {0};

	struct pmic *p_gpadc;
	p_gpadc = pmic_get(MARVELL_PMIC_GPADC);
	if (!p_gpadc || pmic_probe(p_gpadc)) {
		printf("access pmic failed...\n");
		return -1;
	}

	ret = fg_read_regs(p_gpadc, PM88X_VBAT_AVG1, buf, ARRAY_SIZE(buf));
	if (ret)
		return -1;

	data = ((buf[0] & 0xff) << 4) | (buf[1] & 0x0f);

	/* measure(mv) = value * 4 * 1.4 *1000/(2^12) */
	data = ((data & 0xfff) * 7 * 100) >> 9;
	return data;
}

static int pm88x_fg_update_battery(struct pmic *p, struct pmic *bat)
{
	int volt;
	struct power_battery *pb = bat->pbat;

	if (pmic_probe(p)) {
		printf("Can't find 88pm88x fuel gauge\n");
		return -1;
	}

	/* just read VBAT voltage in uV */
	volt = pm88x_get_batt_vol(p);
	if (volt < 0)
		return -1;
	else
		pb->bat->voltage_uV = volt * 1000;

	return 0;
}

static int pm88x_fg_check_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;
	u32 val;
	int ret = 0;

	if (pmic_probe(p)) {
		printf("Can't find 88pm88x fuel gauge\n");
		return -1;
	}


	ret = pmic_reg_read(p, PM88X_CHG_ID, &val);
	pb->bat->version = val;

	pm88x_fg_update_battery(p, bat);
	debug("fg ver: 0x%x\n", pb->bat->version);

	printf("     voltage: %d.%6.6d [V]\n",
	       pb->bat->voltage_uV / 1000000,
	       pb->bat->voltage_uV % 1000000);

	return ret;
}

static struct power_fg power_fg_ops = {
	.fg_battery_check = pm88x_fg_check_battery,
	.fg_battery_update = pm88x_fg_update_battery,
};

int pm88x_fg_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	static const char name[] = MARVELL_PMIC_FG;
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board Fuel Gauge init\n");

	if (!chip) {
		printf("%s: PMIC chip is empty.\n", __func__);
		return -EINVAL;
	}
	chip->fuelgauge_name = name;

	p->bus = bus;
	p->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 3;
	p->hw.i2c.tx_num = 1;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->interface = PMIC_I2C;
	p->name = name;

	p->fg = &power_fg_ops;
	return 0;
}

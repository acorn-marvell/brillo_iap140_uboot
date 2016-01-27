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
#include <power/88pm830.h>
#include <errno.h>

#define MARVELL_PMIC_FG		"88pm830_fuelgauge"

static int fg_read_regs(struct pmic *p, u8 addr, u16 *data, int num)
{
	unsigned int dat;
	int ret = 0;
	int i;

	for (i = 0; i < num; i++, addr++) {
		ret = pmic_reg_read(p, addr, &dat);
		if (ret)
			return ret;

		*(data + i) = (u16)dat;
	}

	return 0;
}

static int pm830_get_batt_vol(struct pmic *p)
{
	int ret, data;
	u16 buf[2] = {0};

	ret = fg_read_regs(p, PM830_REG_VBAT_AVG1, buf, ARRAY_SIZE(buf));
	if (ret)
		return -1;

	data = ((buf[0] & 0xff) << 4) | (buf[1] & 0x0f);

	/* measure(mv) = value * 4 * 1.4 *1000/(2^12) */
	data = ((data & 0xfff) * 7 * 100) >> 9;
	return data;
}

static int pm830_fg_update_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;

	if (pmic_probe(p)) {
		printf("Can't find 88pm830 fuel gauge\n");
		return -1;
	}

	/* just read VBAT voltage in uV */
	pb->bat->voltage_uV = pm830_get_batt_vol(p);
	pb->bat->voltage_uV *= 1000;

	if (pb->bat->voltage_uV < 0)
		return -1;

	return 0;
}

static int pm830_fg_check_battery(struct pmic *p, struct pmic *bat)
{
	struct power_battery *pb = bat->pbat;
	u32 val;
	int ret = 0;

	if (pmic_probe(p)) {
		printf("Can't find 88pm830 fuel gauge\n");
		return -1;
	}

	ret |= pmic_reg_read(p, PM830_REG_STATUS, &val);
	pb->bat->version = val;

	pm830_fg_update_battery(p, bat);
	debug("fg ver: 0x%x\n", pb->bat->version);

	printf("     voltage: %d.%6.6d [V]\n",
	       pb->bat->voltage_uV / 1000000,
	       pb->bat->voltage_uV % 1000000);

	return ret;
}

static struct power_fg power_fg_ops = {
	.fg_battery_check = pm830_fg_check_battery,
	.fg_battery_update = pm830_fg_update_battery,
};

int pm830_fg_alloc(unsigned char bus, struct pmic_chip_desc *chip)
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

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = PM830_REG_NUM_OF_REGS;
	p->hw.i2c.addr = PM830_I2C_ADDR;
	p->hw.i2c.tx_num = 1;
	p->bus = bus;

	p->fg = &power_fg_ops;
	return 0;
}

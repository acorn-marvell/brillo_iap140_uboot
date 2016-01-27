/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/88pm830.h>
#include <i2c.h>
#include <errno.h>

#define MARVELL_PMIC_CHARGE	"88pm830_charger"

static int pm830_charger_type(struct pmic *p)
{
	u32 val;
	int charger;

	/* if probe failed, return cable none */
	if (pmic_probe(p))
		return CHARGER_NO;

	/*
	 * just check the charger present status and if it's present, it will be
	 * recognized as USB charger
	 */
	pmic_reg_read(p, PM830_REG_STATUS, &val);
	if (val & PM830_CHG_PRESENT)
		charger = CHARGER_USB;
	else
		charger = CHARGER_NO;

	return charger;
}

static int pm830_charger_enable(struct pmic *p, int enable)
{
	u32 data;

	if (pmic_probe(p))
		return -1;

	/* start charging, but disable APC */
	data = enable ? 0x3 : 0;
	pmic_reg_write(p, PM830_REG_CHG_CTRL1, data);
	return 0;
}

static inline int get_fastchg_cur(int fastchg_cur)
{
	static int ret;
	ret = (fastchg_cur - 500) / 100;
	return (ret < 0) ? 0 : ret;
}

/*
 * if it's dead battery case, do pre-charge for 2 seconds, and then stop charging
 * else if +/- pin real short, stop charging and return error
 * else it's good battery case, just return
 */
static int pm830_handle_dead_battery(struct pmic *p)
{
	u32 val, vbat;

	/* handle dead battery case */
	pmic_reg_read(p, PM830_REG_CHG_FLAG, &val);
	if (val & PM830_BAT_SHRT) {
		/*
		 * battery short detect, we need to pre-charge for about 2 seconds firstly, if
		 * battery voltage raise up to 2.2V, it should be dead battery , otherwise the
		 * battery is real shorted.
		 */
		printf("battery short or not?\n");
		printf("disable battery short detect and try to pre-charge\n");
		pmic_reg_read(p, PM830_REG_CHG_CTRL2, &val);
		val &= ~PM830_BAT_SHRT_EN;
		pmic_reg_write(p, PM830_REG_CHG_CTRL2, val);

		pm830_charger_enable(p, 1);
		mdelay(2000);
		pm830_charger_enable(p, 0);

		pmic_reg_read(p, PM830_REG_VBAT_VAL1, &val);
		vbat = val << 4;
		pmic_reg_read(p, PM830_REG_VBAT_VAL2, &val);
		vbat |= (val & 0xf);

		/* if battery voltage is above 2.2V, it's dead battery case, enable battery short
		 * detect and start charging
		 * otherwise +/- pin is read short, stop charging
		 */
		if (vbat >= PM830_VBAT_2V2) {
			/* dead battery case */
			printf("dead battery case, enable short detect and start charging\n");
			pmic_reg_read(p, PM830_REG_CHG_CTRL2, &val);
			val |= PM830_BAT_SHRT_EN;
			pmic_reg_write(p, PM830_REG_CHG_CTRL2, val);
		} else {
			printf("real shorted battery case, stop charging\n");
			return -ENODEV;
		}
	} else
		printf("battery not short, start charging\n");

	return 0;
}


static int pm830_charger_state(struct pmic *p, int state, int current)
{
	u32 val;
	int ret;

	if (pmic_probe(p))
		return -EINVAL;

	if (state == CHARGER_DISABLE) {
		printf("Disable the charger.\n");
		pm830_charger_enable(p, 0);
		return 0;
	}

	/* if charging is ongoing, just return */
	pmic_reg_read(p, PM830_REG_CHG_CTRL1, &val);
	if (val & PM830_CHG_ENABLE) {
		printf("Already start charging.\n");
		return 0;
	}

	/* clear interrupt status and charger status */
	pmic_reg_read(p, PM830_REG_INT1, &val);
	pmic_reg_read(p, PM830_REG_INT2, &val);
	pmic_reg_read(p, PM830_REG_INT3, &val);
	pmic_reg_write(p, PM830_REG_CHG_STATUS1, 0xff);
	pmic_reg_read(p, PM830_REG_CHG_STATUS2, &val);
	val |= 0x3;
	pmic_reg_write(p, PM830_REG_CHG_STATUS2, val);

	/* clear battery priority */
	pmic_reg_read(p, PM830_REG_CHG_CTRL3, &val);
	val &= ~PM830_BAT_PRTY_EN;
	pmic_reg_write(p, PM830_REG_CHG_CTRL3, val);

	/* do pre-charge if it's dead battery case */
	ret = pm830_handle_dead_battery(p);
	if (ret < 0) {
		printf("+/- pin short, can't start charging\n");
		return ret;
	}

	/* set fast charge current */
	pmic_reg_read(p, PM830_REG_CHG_CTRL5, &val);
	val &= ~PM830_ICHG_FAST_MASK;
	val |= get_fastchg_cur(current);
	pmic_reg_write(p, PM830_REG_CHG_CTRL5, val);

	/* enable charging */
	pm830_charger_enable(p, 1);

	return 0;
}

static int pm830_charger_bat_present(struct pmic *p)
{
	int ret, voltage;
	static bool charger_bat_init;
	u32 buf[2];
	unsigned int data, mask;

	if (pmic_probe(p))
		return -1;

	if (charger_bat_init == false) {
		charger_bat_init = true;
		/* set bias of 1uA for battery detection */
		data = BIAS_GP0_SET(1);
		mask = BIAS_GP0_MASK;
		pmic_update_bits(p, PM830_REG_BIAS, mask, data);

		/* enable GPADC0 bias */
		mask = PM830_GPADC0_BIAS_EN | PM830_GPADC0_GP_BIAS_OUT;
		pmic_update_bits(p, PM830_REG_BIAS_ENA, mask, mask);

		/* enable GPADC measurement */
		data = PM830_GPADC0_MEAS_EN;
		mask = PM830_GPADC0_MEAS_EN;
		pmic_update_bits(p, PM830_REG_MEAS_EN, mask, data);

		/* wait for voltage to be stable */
		udelay(20000);
	}

	/* read GPADC0 voltage */
	pmic_reg_read(p, PM830_REG_MEAS1, &buf[0]);
	pmic_reg_read(p, PM830_REG_MEAS2, &buf[1]);

	data = ((buf[0] & 0xff) << 4) | (buf[1] & 0x0f);
	voltage = (data * 175) >> 9;

	/* if (voltage < 700) it's battery mode */
	ret = (voltage < 700) ? 1 : 0;

	return ret;
}

static struct power_chrg power_chrg_ops = {
	.chrg_type = pm830_charger_type,
	.chrg_bat_present = pm830_charger_bat_present,
	.chrg_state = pm830_charger_state,
};

int pm830_chrg_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	static const char name[] = MARVELL_PMIC_CHARGE;
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board PMIC init\n");

	if (!chip) {
		printf("%s: PMIC chip is empty.\n", __func__);
		return -EINVAL;
	}
	chip->charger_name = name;

	p->name = name;
	p->interface = PMIC_I2C;
	p->number_of_regs = PM830_REG_NUM_OF_REGS;
	p->hw.i2c.addr = PM830_I2C_ADDR;
	p->hw.i2c.tx_num = 1;
	p->bus = bus;

	p->chrg = &power_chrg_ops;

	return 0;
}

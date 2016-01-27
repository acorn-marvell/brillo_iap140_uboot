/*
 *  Copyright (C) 2014 Marvell aInternational Ltd.
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <i2c.h>
#include <errno.h>

#define BIT(x)			(1UL << (x))

#define MARVELL_PMIC_BASE	"88pm88x_base"
#define MARVELL_PMIC_GPADC	"88pm88x_gpadc"
#define MARVELL_PMIC_CHARGE	"88pm88x_charger"
#define MARVELL_PMIC_BATTERY	"88pm88x_battery"

/* base page registers */
#define PM88X_REG_STATUS	(0x1)
#define PM88X_CHG_PRESENT	(1 << 2)
#define PM88X_BAT_PRESENT	(1 << 3)

/* battery page registers */
#define PM88X_CHG_CONFIG1	(0x28)
#define PM88X_CHG_ENABLE	(1 << 0)

#define PM88X_CHG_FAST_CONFIG2	(0x2f)
#define PM88X_ICHG_FAST_MASK	(0x1f << 0)

/* gpadc page registers */
#define PM88X_GPADC_CONFIG11	(0x0b)
#define PM88X_GPADC_CONFIG12	(0x0c)
#define PM88X_GPADC_CONFIG13	(0x0d)
#define PM88X_GPADC_CONFIG14	(0x0e)
#define PM88X_GPADC_CONFIG20	(0x14)
#define PM88X_GPADC0_MEAS_MSB	(0x54)
#define PM88X_GPADC1_MEAS_MSB	(0x56)
#define PM88X_GPADC2_MEAS_MSB	(0x58)
#define PM88X_GPADC3_MEAS_MSB	(0x5a)

static int pm88x_charger_type(struct pmic *p)
{
	u32 val;
	int charger;
	struct pmic *p_base;

	/* if probe failed, return cable none */
	if (pmic_probe(p))
		return CHARGER_NO;

	/*
	 * just check the charger present status and if it's present, it will be
	 * recognized as USB charger
	 */
	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base || pmic_probe(p)) {
		printf("access pmic failed...\n");
		return -1;
	}
	pmic_reg_read(p_base, PM88X_REG_STATUS, &val);
	if (val & PM88X_CHG_PRESENT)
		charger = CHARGER_USB;
	else
		charger = CHARGER_NO;

	return charger;
}

static int pm88x_charger_enable(struct pmic *p, int enable)
{
	u32 data;

	if (pmic_probe(p))
		return -1;

	/* start/stop charging */
	if (enable)
		data = PM88X_CHG_ENABLE;
	else
		data = 0;
	pmic_update_bits(p, PM88X_CHG_CONFIG1, PM88X_CHG_ENABLE, data);
	return 0;
}

static inline int get_fastchg_cur(int fastchg_cur)
{
	static int ret;

	if (fastchg_cur < 450)
		ret = 0x0;
	else if (fastchg_cur > 1500)
		ret = 0x16;
	else
		ret = (fastchg_cur - 450) / 50 + 1;

	return ret;
}

static int pm88x_charger_state(struct pmic *p, int state, int current)
{
	u32 val;

	if (pmic_probe(p))
		return -EINVAL;

	if (state == CHARGER_DISABLE) {
		printf("Disable the charger.\n");
		pm88x_charger_enable(p, 0);
		return 0;
	}

	/* if charging is ongoing, just return */
	pmic_reg_read(p, PM88X_CHG_CONFIG1, &val);
	if (val & PM88X_CHG_ENABLE) {
		printf("Already start charging.\n");
		return 0;
	}

	/* set fast charge current */
	val = get_fastchg_cur(current);
	pmic_update_bits(p, PM88X_CHG_FAST_CONFIG2, PM88X_ICHG_FAST_MASK, val);

	/* enable charging */
	pm88x_charger_enable(p, 1);

	return 0;
}

#ifdef CONFIG_POWER_88PM88X_USE_SW_BD
static int pm88x_charger_bat_present(struct pmic *p)
{
	struct pmic *p_gpadc;
	struct pmic *p_battery;
	struct pmic_chip_desc *board_pmic_chip;
	int bd_gpadc_no, volt, present, mask, val, gpadc_bias_gp, gpadc_meas_msb;
	u32 buf[2];

	bd_gpadc_no = CONFIG_POWER_88PM88X_GPADC_NO;

	if (pmic_probe(p))
		goto out;

	p_gpadc = pmic_get(MARVELL_PMIC_GPADC);
	if (!p_gpadc || pmic_probe(p_gpadc)) {
		printf("access pmic gpadc page failed...\n");
		goto out;
	}

	p_battery = pmic_get(MARVELL_PMIC_BATTERY);
	if (!p_battery || pmic_probe(p_battery)) {
		printf("access pmic battery page failed...\n");
		goto out;
	}

	board_pmic_chip = get_marvell_pmic();
	if (!board_pmic_chip) {
		printf("%s: chip is NULL.\n", __func__);
		goto out;
	}

	/* enable bias current */
	mask = BIT(bd_gpadc_no) << 4 | BIT(bd_gpadc_no);
	val = mask;
	pmic_update_bits(p_gpadc, PM88X_GPADC_CONFIG20, mask, val);

	switch (bd_gpadc_no) {
	case 0:
		gpadc_bias_gp = PM88X_GPADC_CONFIG11;
		gpadc_meas_msb = PM88X_GPADC0_MEAS_MSB;
		break;
	case 1:
		gpadc_bias_gp = PM88X_GPADC_CONFIG12;
		gpadc_meas_msb = PM88X_GPADC1_MEAS_MSB;
		break;
	case 2:
		gpadc_bias_gp = PM88X_GPADC_CONFIG13;
		gpadc_meas_msb = PM88X_GPADC2_MEAS_MSB;
		break;
	case 3:
		gpadc_bias_gp = PM88X_GPADC_CONFIG14;
		gpadc_meas_msb = PM88X_GPADC3_MEAS_MSB;
		break;
	default:
		printf("%s: GPADC number is wrong.\n", __func__);
		goto out;
	}
	/* set bias_current = 1uA */
	pmic_update_bits(p_gpadc, gpadc_bias_gp, 0x0f, 0);

	/* wait for voltage to be stable */
	udelay(20000);
	/* measure the gpadc voltage */
	pmic_reg_read(p_gpadc, gpadc_meas_msb, &buf[0]);
	pmic_reg_read(p_gpadc, gpadc_meas_msb + 1, &buf[1]);

	volt = ((buf[0] & 0xff) << 4) | (buf[1] & 0x0f);
	volt = (volt * 175) >> 9;
	present = (volt < 700) ? 1 : 0;

	return present;

out:
	return -1;
}
#else
static int pm88x_charger_bat_present(struct pmic *p)
{
	u32 val;
	struct pmic *p_base;

	if (pmic_probe(p))
		return -1;

	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic base page failed...\n");
		return -1;
	}

	pmic_reg_read(p_base, PM88X_REG_STATUS, &val);
	return val & PM88X_BAT_PRESENT;
}
#endif

static struct power_chrg power_chrg_ops = {
	.chrg_type = pm88x_charger_type,
	.chrg_bat_present = pm88x_charger_bat_present,
	.chrg_state = pm88x_charger_state,
};

int pm88x_chrg_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	static const char name[] = MARVELL_PMIC_CHARGE;
	struct pmic *p = pmic_alloc();

	if (!p) {
		printf("%s: POWER allocation error!\n", __func__);
		return -ENOMEM;
	}

	debug("Board charger init\n");

	if (!chip) {
		printf("%s: PMIC chip is empty.\n", __func__);
		return -EINVAL;
	}
	chip->charger_name = name;

	p->bus = bus;
	p->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 3;
	p->hw.i2c.tx_num = 1;
	p->number_of_regs = PMIC_NUM_OF_REGS;
	p->interface = PMIC_I2C;
	p->name = name;

	p->chrg = &power_chrg_ops;

	return 0;
}

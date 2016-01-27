/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Hongyan Song <hysong@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <i2c.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <power/battery.h>
#include <errno.h>

#define MARVELL_PMIC_BASE	"88pm88x_base"
#define MARVELL_PMIC_POWER	"88pm88x_power"
#define MARVELL_PMIC_GPADC	"88pm88x_gpadc"
#define MARVELL_PMIC_CHARGE	"88pm88x_charger"
#define MARVELL_PMIC_FG		"88pm88x_fuelgauge"
#define MARVELL_PMIC_BATTERY	"88pm88x_battery"
#define MARVELL_PMIC_TEST	"88pm88x_test"

#define	PM886_ID_BUCK1		(0xa5)
#define	PM886_ID_BUCK2		(0xb3)
#define	PM886_ID_BUCK3		(0xc1)
#define	PM886_ID_BUCK4		(0xcf)
#define	PM886_ID_BUCK5		(0xdd)

#define	PM886_BUCK_EN1		(0x08)
#define	PM886_LDO1_8_EN1	(0x09)
#define	PM886_LDO9_16_EN1	(0x0a)

static struct pmic_chip_desc *g_chip;
static void pmic_88pm886_stepping_fixup(struct pmic_chip_desc *chip);

/* ldo1~3 */
static const unsigned int ldo1_to_3_voltage_table[] = {
	1700000, 1800000, 1900000, 2500000, 2800000, 2900000, 3100000, 3300000,
};
/* ldo4~15 */
static const unsigned int ldo4_to_15_voltage_table[] = {
	1200000, 1250000, 1700000, 1800000, 1850000, 1900000, 2500000, 2600000,
	2700000, 2750000, 2800000, 2850000, 2900000, 3000000, 3100000, 3300000,
};
/* ldo16 */
static const unsigned int ldo16_voltage_table[] = {
	1700000, 1800000, 1900000, 2000000, 2100000, 2500000, 2700000, 2800000,
};

static int pm886_buck_volt2hex(unsigned int buck, unsigned int uV)
{
	unsigned int hex = 0;

	switch (buck) {
	case 1:
		if (uV > 1800000) {
			printf("%d is wrong voltage for BUCK%d\n", uV, buck);
			return 0;
		}
		break;
	case 2:
	case 3:
	case 4:
	case 5:
		if (uV > 3300000) {
			printf("%d is wrong voltage for BUCK%d\n", uV, buck);
			return 0;
		}
		break;
	default:
		printf("%d is wrong voltage for BUCK%d\n", uV, buck);
		return -EINVAL;
	}

	if (uV <= 1600000)
		hex = (uV - 600000) / 12500;
	else
		hex = 0x50 + (uV - 1600000) / 50000;

	printf("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, hex);

	return hex;
}

int marvell88pm886_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV)
{
	unsigned int ret, addr = PM886_ID_BUCK1;
	int hex = 0;

	if (buck < 1 || buck > 5) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}

	addr += (buck - 1) * 0xd;

	hex = pm886_buck_volt2hex(buck, uV);

	if (hex < 0)
		return -1;

	ret = pmic_update_bits(p, addr, MARVELL88PM_BUCK_VOLT_MASK, hex);

	return ret;
}

static int pm886_buck_hex2volt(unsigned int buck, unsigned int hex)
{
	unsigned int uV = 0;

	if (hex <= 0x50)
		uV = hex * 12500 + 600000;
	else
		uV = 1600000 + (hex - 0x50) * 50000;

	printf("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, hex);

	return uV;
}

int marvell88pm886_get_buck_vol(struct pmic *p, unsigned int buck)
{
	unsigned int val, ret, addr = PM886_ID_BUCK1;
	int uV;

	if (buck < 1 || buck > 5) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}

	addr += (buck - 1) * 0xd;

	ret = pmic_reg_read(p, addr, &val);
	if (ret)
		return ret;

	uV = pm886_buck_hex2volt(buck, val);
	if (uV < 0)
		return -1;

	return uV;
}

static int pm886_ldo_volt2hex(unsigned int ldo, unsigned int uV)
{
	const unsigned int *voltage_table = NULL;
	unsigned int table_size = 0, hex = 0;
	/*choose ldo voltage table*/
	switch (ldo) {
	case 1:
	case 2:
	case 3:
		voltage_table = ldo1_to_3_voltage_table;
		table_size = ARRAY_SIZE(ldo1_to_3_voltage_table);
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		voltage_table = ldo4_to_15_voltage_table;
		table_size = ARRAY_SIZE(ldo4_to_15_voltage_table);
		break;
	case 16:
		voltage_table = ldo16_voltage_table;
		table_size = ARRAY_SIZE(ldo16_voltage_table);
		break;
	default:
		printf("%s: %d is wrong LDO number\n", __func__, ldo);
		return -EINVAL;
	}

	for (hex = 0; hex < table_size; hex++) {
		if (uV <= voltage_table[hex]) {
			printf("ldo %d, voltage %d, reg value 0x%x\n", ldo, uV, hex);
			return hex;
		}
	}

	return table_size - 1;
}
int marvell88pm886_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV)
{
	unsigned int ret, addr, ldo_en_mask, ldo_en;
	int hex = 0;

	if (ldo < 1 || ldo > 16) {
		printf("%s: %d is wrong ldo number\n", __func__, ldo);
		return -1;
	}

	addr = 0x20 + (ldo - 1) * 6;
	hex = pm886_ldo_volt2hex(ldo, uV);

	if (hex < 0)
		return -1;

	ret = pmic_update_bits(p, addr, MARVELL88PM_LDO_VOLT_MASK, hex);
	if (ret)
		return ret;

	/* check whether the LDO is enabled, if not enable it.*/
	if (ldo < 9 && ldo > 0)
		ldo_en = PM886_LDO1_8_EN1;
	else
		ldo_en = PM886_LDO9_16_EN1;

	ldo_en_mask = 1 << ((ldo - 1) % 8);

	ret = pmic_update_bits(p, ldo_en, ldo_en_mask, ldo_en_mask);

	return ret;
}

void marvell88pm886_reset_bd(struct pmic *p_base)
{
	pmic_reg_write(p_base, 0x14, 0x20);
}

int pmic_88pm886_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	struct pmic *p_base = pmic_alloc();
	struct pmic *p_power = pmic_alloc();
	struct pmic *p_gpadc = pmic_alloc();
	struct pmic *p_test = pmic_alloc();
	if (!p_base || !p_power || !p_gpadc || !p_test) {
		printf("%s: pmic allocation error!\n", __func__);
		return -ENOMEM;
	}

	if (!chip) {
		printf("%s: chip description is empty!\n", __func__);
		return -EINVAL;
	}
	chip->base_name = MARVELL_PMIC_BASE;
	chip->power_name = MARVELL_PMIC_POWER;
	chip->gpadc_name = MARVELL_PMIC_GPADC;
	chip->charger_name = MARVELL_PMIC_CHARGE;
	chip->fuelgauge_name = MARVELL_PMIC_FG;
	chip->battery_name = MARVELL_PMIC_BATTERY;

	p_base->bus = bus;
	p_base->hw.i2c.addr = MARVELL88PM_I2C_ADDR;
	p_base->name = MARVELL_PMIC_BASE;
	p_base->interface = PMIC_I2C;
	p_base->number_of_regs = PMIC_NUM_OF_REGS;
	p_base->hw.i2c.tx_num = 1;

	p_power->bus = bus;
	p_power->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 1;
	p_power->name = MARVELL_PMIC_POWER;
	p_power->interface = PMIC_I2C;
	p_power->number_of_regs = PMIC_NUM_OF_REGS;
	p_power->hw.i2c.tx_num = 1;

	p_gpadc->bus = bus;
	p_gpadc->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 2;
	p_gpadc->name = MARVELL_PMIC_GPADC;
	p_gpadc->interface = PMIC_I2C;
	p_gpadc->number_of_regs = PMIC_NUM_OF_REGS;
	p_gpadc->hw.i2c.tx_num = 1;

	/*
	 * this page is special, we just use it to apply some
	 * WA for pm886
	 */
	p_test->bus = bus;
	p_test->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 7;
	p_test->name = MARVELL_PMIC_TEST;
	p_test->interface = PMIC_I2C;
	p_test->number_of_regs = PMIC_NUM_OF_REGS;
	p_test->hw.i2c.tx_num = 1;

	/* get functions */
	chip->set_buck_vol = marvell88pm886_set_buck_vol;
	chip->set_ldo_vol = marvell88pm886_set_ldo_vol;
	chip->get_buck_vol = marvell88pm886_get_buck_vol;
	chip->reset_bd = marvell88pm886_reset_bd;

	puts("Board PMIC init\n");

	g_chip = chip;

	return 0;
}

void pmic_88pm886_base_init(struct pmic *p_base)
{
	u32 data;
	/* disable watchdog */
	pmic_update_bits(p_base, 0x1d, (1 << 0), (1 << 0));

	/*----following setting is for Audio Codec ---*/
	/* enable 32Khz low jitter XO_LJ = 1 */
	pmic_update_bits(p_base, 0x21, (1 << 5), (1 << 5));
	/* enable USE_XO = 1 */
	pmic_update_bits(p_base, 0xd0, (1 << 7), (1 << 7));
	/* output 32k from XO */
	pmic_update_bits(p_base, 0xe2, 0xf, 0xa);

	/* read-clear interruption registers */
	pmic_reg_read(p_base, PM886_REG_INT1, &data);
	pmic_reg_read(p_base, PM886_REG_INT2, &data);
	pmic_reg_read(p_base, PM886_REG_INT3, &data);
	pmic_reg_read(p_base, PM886_REG_INT4, &data);

	/*
	 * print out power up/down log and save in memory
	 * the power up log has little change vesus to
	 * 88pm820/88pm860
	 */
	get_powerup_down_log(g_chip);
}

void pmic_88pm886_gpadc_init(struct pmic *p_gpadc)
{
	/* enable VBAT, VBUS, TINT, GPADC3 */
	pmic_update_bits(p_gpadc, PM886_GPADC_CONFIG1, 0x12, 0x12);
	pmic_update_bits(p_gpadc, PM886_GPADC_CONFIG2, 0x21, 0x21);
	/* enable GPADC and set non-stop mode */
	pmic_update_bits(p_gpadc, PM886_GPADC_CONFIG6, 0x3, 0x3);

	/* TODO: enable battery detection: board specific */
}

void pmic_88pm886_power_init(struct pmic *p_power)
{
	/* set buck1 DVC ramp up rate to 12.5mV/us */
	pmic_update_bits(p_power, 0x9e, (0x7 << 3), (0x6 << 3));
}

void pm886_battery_init(struct pmic *p_battery)
{
	struct pmic_chip_desc *board_pmic_chip;

	board_pmic_chip = get_marvell_pmic();
	/* set USB_SW */
	pmic_update_bits(p_battery, PM886_CHG_CONFIG4, 0x1, 0x1);
	/* disable charger's watchdog */
	pmic_update_bits(p_battery, PM886_CHG_CONFIG1, PM886_CHG_WDG_EN, 0);
	/* set precharge voltage to 3V, current to 300mA */
	pmic_reg_write(p_battery, PM886_CHG_PRE_CONFIG1, 0x70);
	/* set fastchg votlage to 4.2V */
	pmic_reg_write(p_battery, PM886_CHG_FAST_CONFIG1, 0x30);
	/* set fastchg current to 1500mA */
	pmic_reg_write(p_battery, PM886_CHG_FAST_CONFIG2, 0x16);
	/* set fastchg timeout to 8h */
	pmic_update_bits(p_battery, PM886_CHG_TIMER_CONFIG, PM886_FASTCHG_TIMEOUT, 0x4);
	/* set chg current limit to 500mA */
	pmic_reg_write(p_battery, PM886_CHG_EXT_ILIM_CONFIG, 0x4);
	/* clear the log */
	pmic_reg_write(p_battery, PM886_CHG_LOG1, 0xff);
	pmic_reg_write(p_battery, PM886_OTG_LOG1, 0x3);

	pmic_88pm886_stepping_fixup(board_pmic_chip);
}

static void pmic_88pm886_stepping_fixup(struct pmic_chip_desc *chip)
{
	struct pmic *p_base, *p_battery, *p_test;
	p_base = pmic_get(chip->base_name);
	p_battery = pmic_get(chip->battery_name);
	p_test = pmic_get(MARVELL_PMIC_TEST);

	/* WA for A0 to prevent OV_VBAT fault and support dead battery case */
	if (chip->chip_id == 0x00) {
		if (p_battery && !pmic_probe(p_battery)) {
			/* set BATTEMP_MON2_DIS */
			pmic_update_bits(p_battery, PM886_CHG_CONFIG1,
					PM886_BATTEMP_MON2_DIS, PM886_BATTEMP_MON2_DIS);
			/* disable OV_VBAT */
			pmic_update_bits(p_battery, PM886_CHG_CONFIG3, PM886_OV_VBAT_EN, 0);
		}

		if (p_base && p_test && !pmic_probe(p_base) && !pmic_probe(p_test)) {
			/* open test page */
			pmic_reg_write(p_base, 0x1f, 0x1);
			/* change the defaults to disable OV_VBAT */
			pmic_reg_write(p_test, 0x50, 0x2a);
			pmic_reg_write(p_test, 0x51, 0x0c);
			/* change defaults to enable charging */
			pmic_reg_write(p_test, 0x52, 0x28);
			pmic_reg_write(p_test, 0x53, 0x01);
			/* change defaults to disable OV_VSYS1 and UV_VSYS1 */
			pmic_reg_write(p_test, 0x54, 0x23);
			pmic_reg_write(p_test, 0x55, 0x14);
			pmic_reg_write(p_test, 0x58, 0xbb);
			pmic_reg_write(p_test, 0x59, 0x08);
			/* close test page */
			pmic_reg_write(p_base, 0x1f, 0x0);
			/* disable OV_VSYS1 and UV_VSYS1 */
			pmic_reg_write(p_base, PM886_REG_LOWPOWER4, 0x14);
		}
	}
}

void pmic_pm886_powerdown(struct pmic *p_base)
{
	pmic_reg_write(p_base, PM886_REG_WAKE_UP, PM886_SW_PDOWN);
}

#define	ONKEY_STATUS		0x1
int pmic_pm886_onkey_check(struct pmic *p_base)
{
	int ret;
	u32 val;

	ret = pmic_reg_read(p_base, PM886_REG_STATUS, &val);
	if (ret)
		return -1;

	return val & ONKEY_STATUS;
}

static int pm886_enable_bat_det(struct pmic *p, int enable)
{
	u32 val;
	struct pmic *p_gpadc;

	p_gpadc = pmic_get(MARVELL_PMIC_GPADC);
	if (!p_gpadc || pmic_probe(p_gpadc)) {
		printf("access pmic failed...\n");
		return -1;
	}

	if (!!enable)
		val = PM886_BD_EN;
	else
		val = 0;
	pmic_update_bits(p_gpadc, PM886_GPADC_CONFIG8, PM886_BD_EN, val);

	return 0;
}

/*
 * the method used to get power up mode for 88pm886:
 * if BD circuit is used, set BD_EN, else use software BD.
 * if battery present
 *   -->  it's real-battery mode
 * else if VBAT is OK
 *   -->  it is power supply mode
 * else
 *   --> invalid power supply
 */
int pm886_get_powerup_mode(void)
{
	int ret = BATTERY_MODE;
	struct pmic *p_bat, *p;

	p_bat = pmic_get(MARVELL_PMIC_BATTERY);
	if (!p_bat || pmic_probe(p_bat)) {
		printf("access battery failed...\n");
		return INVALID_MODE;
	}
	p = p_bat->pbat->chrg;

	/* 1. battery detection */
#ifdef CONFIG_POWER_88PM88X_USE_SW_BD
	pm886_enable_bat_det(p, 0);
#else
	pm886_enable_bat_det(p, 1);
#endif
	ret = p_bat->chrg->chrg_bat_present(p);
	if (!!ret) {
		/* it's battery mode */
		ret = BATTERY_MODE;
		printf("power up mode - battery mode\n");
		goto OUT;
	}

	/* 2. measure vbat */
	p_bat->fg->fg_battery_update(p_bat->pbat->fg, p_bat);
	if (p_bat->pbat->bat->voltage_uV > POWER_OFF_THRESHOLD) {
		/* it's power supply mode */
		ret = POWER_SUPPLY_MODE;
		printf("power up mode - power supply mode\n");
		goto OUT;
	}

	/* invalid power supply mode */
	ret = INVALID_MODE;
	printf("invalid power supply! VBAT=%d(mv)\n", p_bat->pbat->bat->voltage_uV / 1000);

OUT:
	return ret;
}

int pmic_88pm886_init(struct pmic_chip_desc *chip)
{
	struct pmic *p_base, *p_power, *p_gpadc;
	if (!chip) {
		printf("---%s: chip is empty.\n", __func__);
		return -EINVAL;
	}

	/*------------base page setting-----------*/
	p_base = pmic_get(chip->base_name);
	if (!p_base)
		return -1;
	if (pmic_probe(p_base))
		return -1;
	pmic_88pm886_base_init(p_base);
	board_pmic_base_fixup(p_base);

	/*------------gpadc page setting -----------*/
	p_gpadc = pmic_get(chip->gpadc_name);
	if (!p_gpadc)
		return -1;
	if (pmic_probe(p_gpadc))
		return -1;
	pmic_88pm886_gpadc_init(p_gpadc);
	board_pmic_gpadc_fixup(p_gpadc);

	/*------------power page setting -----------*/
	p_power = pmic_get(chip->power_name);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;
	pmic_88pm886_power_init(p_power);
	board_pmic_power_fixup(p_power);


	printf("PMIC init done!\n");
	return 0;
}

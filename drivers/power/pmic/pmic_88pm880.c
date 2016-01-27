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
#include <power/88pm880.h>
#include <power/battery.h>
#include <errno.h>

#define MARVELL_PMIC_BASE	"88pm88x_base"
#define MARVELL_PMIC_BUCK	"88pm88x_buck"
#define MARVELL_PMIC_LDO	"88pm88x_ldo"
#define MARVELL_PMIC_POWER	"88pm88x_power"
#define MARVELL_PMIC_GPADC	"88pm88x_gpadc"
#define MARVELL_PMIC_CHARGE	"88pm88x_charger"
#define MARVELL_PMIC_FG		"88pm88x_fuelgauge"
#define MARVELL_PMIC_BATTERY	"88pm88x_battery"
#define MARVELL_PMIC_LED	"88pm88x_led"

static struct pmic_chip_desc *g_chip;

/* ldo1~3 */
static const unsigned int ldo1_to_3_voltage_table[] = {
	1700000, 1800000, 1900000, 2500000, 2800000, 2900000, 3100000, 3300000,
};
/* ldo4~17 */
static const unsigned int ldo4_to_17_voltage_table[] = {
	1200000, 1250000, 1700000, 1800000, 1850000, 1900000, 2500000, 2600000,
	2700000, 2750000, 2800000, 2850000, 2900000, 3000000, 3100000, 3300000,
};
/* ldo18 */
static const unsigned int ldo18_voltage_table[] = {
	1700000, 1800000, 1900000, 2000000, 2100000, 2500000, 2700000, 2800000,
};

static int pm880_buck_volt2hex(unsigned int buck, unsigned int uV)
{
	unsigned int regval = 0;

	switch (buck) {
	case 1:
		if (uV > 1800000) {
			printf("%d is wrong voltage for BUCK%d\n", uV, buck);
			return -EINVAL;
		}
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		if (uV > 3300000) {
			printf("%d is wrong voltage for BUCK%d\n", uV, buck);
			return -EINVAL;
		}
		break;
	default:
		printf("%d is wrong voltage for BUCK%d\n", uV, buck);
		return -EINVAL;
	}

	if (uV <= 1600000)
		regval = (uV - 600000) / 12500;
	else
		regval = 0x50 + (uV - 1600000) / 50000;

	printf("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, regval);

	return regval;
}

int marvell88pm880_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV)
{
	unsigned int ret, addr = PM880_ID_BUCK1;
	int regval = 0;
	struct pmic *p_buck;

	p_buck = pmic_get(g_chip->buck_name);
	if (!p_buck || pmic_probe(p_buck)) {
		printf("%s: get PMIC - buck fail!\n", __func__);
		return -ENODEV;
	}

	if (buck < 1 || buck > 7) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -EINVAL;
	}

	switch (buck) {
	case 1:
		addr = PM880_ID_BUCK1;
		break;
	case 2:
		addr = PM880_ID_BUCK2;
		break;
	case 3:
		addr = PM880_ID_BUCK3;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		addr = PM880_ID_BUCK4 + 0x10 * (buck - 4);
		break;
	default:
		return -EINVAL;
	}

	regval = pm880_buck_volt2hex(buck, uV);

	if (regval < 0)
		return -EINVAL;

	ret = pmic_update_bits(p_buck, addr, MARVELL88PM_BUCK_VOLT_MASK, regval);

	return ret;
}

static int pm880_buck_hex2volt(unsigned int buck, unsigned int hex)
{
	unsigned int uV = 0;

	if (hex <= 0x50)
		uV = hex * 12500 + 600000;
	else
		uV = 1600000 + (hex - 0x50) * 50000;

	printf("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, hex);

	return uV;
}

int marvell88pm880_get_buck_vol(struct pmic *p, unsigned int buck)
{
	unsigned int val, ret, addr = PM880_ID_BUCK1;
	int uV;
	struct pmic *p_buck;

	p_buck = pmic_get(g_chip->buck_name);
	if (!p_buck || pmic_probe(p_buck)) {
		printf("%s: get PMIC - buck fial!\n", __func__);
		return -ENODEV;
	}

	if (buck < 1 || buck > 7) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -EINVAL;
	}

	switch (buck) {
	case 1:
		addr = PM880_ID_BUCK1;
		break;
	case 2:
		addr = PM880_ID_BUCK2;
		break;
	case 3:
		addr = PM880_ID_BUCK3;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		addr = PM880_ID_BUCK4 + 0x10 * (buck - 4);
		break;
	default:
		return -EINVAL;
	}

	ret = pmic_reg_read(p_buck, addr, &val);
	if (ret)
		return ret;

	uV = pm880_buck_hex2volt(buck, val);
	if (uV < 0)
		return -EINVAL;

	return uV;
}

static int pm880_ldo_volt2hex(unsigned int ldo, unsigned int uV)
{
	const unsigned int *voltage_table = NULL;
	unsigned int table_size = 0, regval = 0;

	/* choose ldo voltage table */
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
	case 16:
	case 17:
		voltage_table = ldo4_to_17_voltage_table;
		table_size = ARRAY_SIZE(ldo4_to_17_voltage_table);
		break;
	case 18:
		voltage_table = ldo18_voltage_table;
		table_size = ARRAY_SIZE(ldo18_voltage_table);
		break;
	default:
		printf("%s: %d is wrong LDO number\n", __func__, ldo);
		return -EINVAL;
	}

	for (regval = 0; regval < table_size; regval++) {
		if (uV <= voltage_table[regval]) {
			printf("ldo %d, voltage %d, reg value 0x%x\n", ldo, uV, regval);
			return regval;
		}
	}

	return table_size - 1;
}

int marvell88pm880_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV)
{
	unsigned int addr, ldo_en_mask, ldo_en;
	int ret, regval = 0;
	struct pmic *p_ldo;

	p_ldo = pmic_get(g_chip->ldo_name);
	if (!p_ldo || pmic_probe(p_ldo)) {
		printf("%s: get PMIC - ldo fail!\n", __func__);
		return -ENODEV;
	}

	if (ldo < 1 || ldo > 18) {
		printf("%s: %d is wrong ldo number\n", __func__, ldo);
		return -EINVAL;
	}

	addr = 0x20 + (ldo - 1) * 6;
	regval = pm880_ldo_volt2hex(ldo, uV);

	if (regval < 0)
		return -EINVAL;

	ret = pmic_update_bits(p, addr, MARVELL88PM_LDO_VOLT_MASK, regval);
	if (ret)
		return ret;

	/* check whether the LDO is enabled, if not enable it.*/
	if (ldo < 9 && ldo > 0)
		ldo_en = PM880_LDO1_8_EN1;
	else if (ldo < 17 && ldo > 8)
		ldo_en = PM880_LDO9_16_EN1;
	else
		ldo_en = PM880_LDO17_18_EN1;

	ldo_en_mask = 1 << ((ldo - 1) % 8);

	ret = pmic_update_bits(p_ldo, ldo_en, ldo_en_mask, ldo_en_mask);

	return ret;
}

void marvell88pm880_reset_bd(struct pmic *p_base)
{
	pmic_reg_write(p_base, 0x14, 0x20);
}

int pmic_88pm880_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	struct pmic *p_base = pmic_alloc();
	struct pmic *p_gpadc = pmic_alloc();
	struct pmic *p_buck = pmic_alloc();
	struct pmic *p_ldo = pmic_alloc();
	struct pmic *p_power = pmic_alloc();
	struct pmic *p_led = pmic_alloc();
	if (!p_base || !p_gpadc || !p_ldo || !p_buck || !p_power) {
		printf("%s: pmic allocation error!\n", __func__);
		return -ENOMEM;
	}

	if (!chip) {
		printf("%s: chip description is empty!\n", __func__);
		return -EINVAL;
	}
	chip->base_name = MARVELL_PMIC_BASE;
	chip->buck_name = MARVELL_PMIC_BUCK;
	chip->ldo_name = MARVELL_PMIC_LDO;
	chip->power_name = MARVELL_PMIC_POWER;
	chip->gpadc_name = MARVELL_PMIC_GPADC;
	chip->led_name = MARVELL_PMIC_LED;
	chip->charger_name = MARVELL_PMIC_CHARGE;
	chip->fuelgauge_name = MARVELL_PMIC_FG;
	chip->battery_name = MARVELL_PMIC_BATTERY;

	p_base->bus = bus;
	p_base->hw.i2c.addr = MARVELL88PM_I2C_ADDR;
	p_base->name = MARVELL_PMIC_BASE;
	p_base->interface = PMIC_I2C;
	p_base->number_of_regs = PMIC_NUM_OF_REGS;
	p_base->hw.i2c.tx_num = 1;

	p_ldo->bus = bus;
	p_ldo->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 1;
	p_ldo->name = MARVELL_PMIC_LDO;
	p_ldo->interface = PMIC_I2C;
	p_ldo->number_of_regs = PMIC_NUM_OF_REGS;
	p_ldo->hw.i2c.tx_num = 1;

	p_gpadc->bus = bus;
	p_gpadc->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 2;
	p_gpadc->name = MARVELL_PMIC_GPADC;
	p_gpadc->interface = PMIC_I2C;
	p_gpadc->number_of_regs = PMIC_NUM_OF_REGS;
	p_gpadc->hw.i2c.tx_num = 1;

	p_led->bus = bus;
	p_led->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 3;
	p_led->name = MARVELL_PMIC_LED;
	p_led->interface = PMIC_I2C;
	p_led->number_of_regs = PMIC_NUM_OF_REGS;
	p_led->hw.i2c.tx_num = 1;

	p_buck->bus = bus;
	p_buck->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 4;
	p_buck->name = MARVELL_PMIC_BUCK;
	p_buck->interface = PMIC_I2C;
	p_buck->number_of_regs = PMIC_NUM_OF_REGS;
	p_buck->hw.i2c.tx_num = 1;

	/*
	 * dummy power page
	 * set the parameters as same as ldo
	 */
	p_power->bus = bus;
	p_power->hw.i2c.addr = MARVELL88PM_I2C_ADDR + 1;
	p_power->name = MARVELL_PMIC_POWER;
	p_power->interface = PMIC_I2C;
	p_power->number_of_regs = PMIC_NUM_OF_REGS;
	p_power->hw.i2c.tx_num = 1;

	/* get functions */
	chip->set_buck_vol = marvell88pm880_set_buck_vol;
	chip->set_ldo_vol = marvell88pm880_set_ldo_vol;
	chip->get_buck_vol = marvell88pm880_get_buck_vol;
	chip->reset_bd = marvell88pm880_reset_bd;

	puts("Board PMIC init\n");

	g_chip = chip;

	return 0;
}

#define ONKEY_STATUS           0x1

static int onkey_state(struct pmic *p_base)
{
	u32 status_reg1;
	pmic_reg_read(p_base, PM880_REG_STATUS, &status_reg1);

	return (status_reg1 & ONKEY_STATUS)?1:0;
}

int press_onkey_check(struct pmic_chip_desc *chip, int bootdelay)
{
	int pressed;
	int flag = 0;
	struct pmic *p_base;

	p_base = pmic_get(chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return 0;
	}
	pressed = onkey_state(p_base);
	while (bootdelay > 0) {
		int i = 0;
		--bootdelay;
		if (pressed == 0) {
			flag = 1;
			break;
		}
		for (i = 0; i < 2; ++i)
			udelay(10000);

		pressed = onkey_state(p_base);
	}

	if (flag == 1) {
		printf("Power key is pressed by mistake.Power down device here.\n");
		pmic_pm880_powerdown(p_base);
		return 0;
	} else
		return flag;
}

void pmic_88pm880_base_init(struct pmic *p_base)
{
	u32 data;
	/* disable watchdog */
	pmic_update_bits(p_base, PM880_REG_WDOG, PM880_REG_WDOG_DIS, PM880_REG_WDOG_DIS);

	/*----following setting is for Audio Codec ---*/
	/* enable 32Khz low jitter XO_LJ = 1 */
	pmic_update_bits(p_base, PM880_REG_LOWPOWER2, PM880_REG_XO_LJ, PM880_REG_XO_LJ);
	/* enable USE_XO = 1 */
	pmic_update_bits(p_base, PM880_REG_RTC_CTRL1, PM880_REG_USE_XO, PM880_REG_USE_XO);
	/* output 32k from XO */
	pmic_update_bits(p_base, PM880_REG_AON_CTRL2, PM880_CLK32K2_SEL_MASK, PM880_CLK32K2_SEL_32);

	/* read-clear interruption registers */
	pmic_reg_read(p_base, PM880_REG_INT1, &data);
	pmic_reg_read(p_base, PM880_REG_INT2, &data);
	pmic_reg_read(p_base, PM880_REG_INT3, &data);
	pmic_reg_read(p_base, PM880_REG_INT4, &data);

	/* turn on cfd clock, so we could turn off CFD_PLS_EN later */
	pmic_update_bits(p_base, PM880_REG_CLK_CTRL_1, PM880_CFD_CLK_EN, PM880_CFD_CLK_EN);

	/*
	 * print out power up/down log and save in memory
	 * the power up log has little change vesus to
	 * 88pm820/88pm860/88pm880
	 */
	get_powerup_down_log(g_chip);
}


void pmic_88pm880_led_init(struct pmic *p_led)
{
	/* turn torch off by clearing cfd pulse, CFD_CLK_EN should be enabled before */
	pmic_update_bits(p_led, PM880_CFD_CONFIG_4, PM880_CFD_PLS_EN, 0);
}

void pmic_88pm880_gpadc_init(struct pmic *p_gpadc)
{
	unsigned int data;

	/* enable VBAT, VBUS, TINT, GPADC1 */
	data = PM880_GPADC_VBAT_EN | PM880_GPADC_VBUS_EN;
	pmic_update_bits(p_gpadc, PM880_GPADC_CONFIG1, data, data);
	data = PM880_GPADC_TINT_EN | PM880_GPADC_GPADC1_EN;
	pmic_update_bits(p_gpadc, PM880_GPADC_CONFIG2, data, data);

	/* enable GPADC and set non-stop mode */
	data = PM880_GPADC_EN | PM880_GPADC_NON_STOP;
	pmic_update_bits(p_gpadc, PM880_GPADC_CONFIG6, data, data);

	/* TODO: enable battery detection: board specific */
}

void pmic_88pm880_buck_init(struct pmic *p_buck)
{
	/* set buck1 DVC ramp up rate to 12.5mV/us */
	pmic_update_bits(p_buck, PM880_BUCK1_RAMP, PM880_BK1A_RAMP_MASK, PM880_BK1A_RAMP_12P5);
}

void pmic_88pm880_ldo_init(struct pmic *p_ldo)
{
}

void pm880_battery_init(struct pmic *p_battery)
{
	/* set USB_SW */
	pmic_update_bits(p_battery, PM880_CHG_CONFIG4, PM880_USB_SW, PM880_USB_SW);
	/* disable charger's watchdog */
	pmic_update_bits(p_battery, PM880_CHG_CONFIG1, PM880_CHG_WDG_EN, 0);
	/* set precharge voltage to 3V, current to 300mA */
	pmic_reg_write(p_battery, PM880_CHG_PRE_CONFIG1, 0x70);
	/* set fastchg votlage to 4.2V */
	pmic_reg_write(p_battery, PM880_CHG_FAST_CONFIG1, 0x30);
	/* set fastchg current to 1500mA */
	pmic_reg_write(p_battery, PM880_CHG_FAST_CONFIG2, 0x16);
	/* set fastchg timeout to 8h */
	pmic_update_bits(p_battery, PM880_CHG_TIMER_CONFIG, PM880_FASTCHG_TIMEOUT, 0x4);
	/* set chg current limit to 500mA */
	pmic_reg_write(p_battery, PM880_CHG_EXT_ILIM_CONFIG, 0x4);
	/* clear the log */
	pmic_reg_write(p_battery, PM880_CHG_LOG1, 0xff);
	pmic_reg_write(p_battery, PM880_OTG_LOG1, 0x3);
}

void pmic_pm880_powerdown(struct pmic *p_base)
{
	pmic_reg_write(p_base, PM880_REG_WAKE_UP, PM880_SW_PDOWN);
}

static int pm880_enable_bat_det(struct pmic *p, int enable)
{
	u32 val;
	struct pmic *p_gpadc;

	p_gpadc = pmic_get(MARVELL_PMIC_GPADC);
	if (!p_gpadc || pmic_probe(p_gpadc)) {
		printf("access pmic failed...\n");
		return -1;
	}

	if (!!enable)
		val = PM880_BD_EN;
	else
		val = 0;

	pmic_update_bits(p_gpadc, PM880_GPADC_CONFIG8, PM880_BD_EN, val);

	return 0;
}

/*
 * the method used to get power up mode for 88pm880:
 * if BD circuit is used, set BD_EN, else use software BD.
 * if battery present
 *   -->  it's real-battery mode
 * else if VBAT is OK
 *   -->  it is power supply mode
 * else
 *   --> invalid power supply
 */
int pm880_get_powerup_mode(void)
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
	pm880_enable_bat_det(p, 0);
#else
	pm880_enable_bat_det(p, 1);
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

int pmic_88pm880_init(struct pmic_chip_desc *chip)
{
	struct pmic *p_base, *p_buck, *p_ldo, *p_gpadc, *p_led;

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
	pmic_88pm880_base_init(p_base);
	board_pmic_base_fixup(p_base);

	/*------------gpadc page setting -----------*/
	p_gpadc = pmic_get(chip->gpadc_name);
	if (!p_gpadc)
		return -1;
	if (pmic_probe(p_gpadc))
		return -1;
	pmic_88pm880_gpadc_init(p_gpadc);
	board_pmic_gpadc_fixup(p_gpadc);

	/*------------buck page setting -----------*/
	p_buck = pmic_get(chip->buck_name);
	if (!p_buck)
		return -1;
	if (pmic_probe(p_buck))
		return -1;
	pmic_88pm880_buck_init(p_buck);
	board_pmic_buck_fixup(p_buck);

	/*------------ldo page setting ------------*/
	p_ldo = pmic_get(chip->ldo_name);
	if (!p_ldo)
		return -1;
	if (pmic_probe(p_ldo))
		return -1;
	pmic_88pm880_ldo_init(p_ldo);
	board_pmic_ldo_fixup(p_ldo);

	/*------------led page setting ------------*/
	p_led = pmic_get(chip->led_name);
	if (!p_led)
		return -1;
	if (pmic_probe(p_led))
		return -1;
	pmic_88pm880_led_init(p_led);

	printf("PMIC init done!\n");
	return 0;
}

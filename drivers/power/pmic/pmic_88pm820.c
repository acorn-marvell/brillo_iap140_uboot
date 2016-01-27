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
#include <errno.h>

#define MARVELL_PMIC_BASE	"88pm820_base"
#define MARVELL_PMIC_POWER	"88pm820_power"
#define MARVELL_PMIC_GPADC	"88pm820_gpadc"

enum {
	PM820_ID_LDO1 = 0x08,
	PM820_ID_LDO2 = 0x0b,
	PM820_ID_LDO3,
	PM820_ID_LDO4,
	PM820_ID_LDO5,
	PM820_ID_LDO6,
	PM820_ID_LDO7,
	PM820_ID_LDO8,
	PM820_ID_LDO9,
	PM820_ID_LDO10,
	PM820_ID_LDO11,
	PM820_ID_LDO12,
	PM820_ID_LDO13,
	PM820_ID_LDO14,

	PM820_ID_BUCK1 = 0x3c,
	PM820_ID_BUCK2 = 0x40,
	PM820_ID_BUCK3 = 0x41,
	PM820_ID_BUCK4 = 0x45,
	PM820_ID_BUCK5 = 0x46,

	PM820_BUCK_EN1 = 0x50,
	PM820_LDO1_8_EN1 = 0x51,
	PM820_LDO9_14_EN1 = 0x52,

};

static unsigned int ldo1_2_13_voltage_table[] = {
	1700000, 1800000, 1900000, 2500000, 2800000, 2900000, 3100000, 3300000,
};
static unsigned int ldo3_to_11_voltage_table[] = {
	1200000, 1250000, 1700000, 1800000, 1850000, 1900000, 2500000, 2600000,
	2700000, 2750000, 2800000, 2850000, 2900000, 3000000, 3100000, 3300000,
};
static unsigned int ldo12_voltage_table[] = {
	600000,  650000,  700000,  750000,  800000,  850000,  900000,  950000,
	1000000, 1050000, 1100000, 1150000, 1200000, 1300000, 1400000, 1500000,
};
static unsigned int ldo14_voltage_table[] = {
	1700000, 1800000, 1900000, 2000000, 2100000, 2500000, 2700000, 2800000,
};

int marvell88pm820_reg_update(struct pmic *p, int reg, unsigned int regval)
{
	u32 val;
	int ret = 0;

	ret = pmic_reg_read(p, reg, &val);
	if (ret) {
		debug("%s: PMIC %d register read failed\n", __func__, reg);
		return -1;
	}
	val |= regval;
	ret = pmic_reg_write(p, reg, val);
	if (ret) {
		debug("%s: PMIC %d register write failed\n", __func__, reg);
		return -1;
	}
	return 0;
}

static int pm820_buck_volt2hex(unsigned int buck, unsigned int uV)
{
	unsigned int hex = 0;

	switch (buck) {
	case 1:
		if (uV > 1800000) {
			debug("%d is wrong voltage for BUCK%d\n", uV, buck);
			return 0;
		}
		break;
	case 2:
	case 3:
	case 4:
		if (uV > 3300000) {
			debug("%d is wrong voltage for BUCK%d\n", uV, buck);
			return 0;
		}
		break;
	case 5:
		if (uV > 3950000) {
			debug("%d is wrong voltage for BUCK%d\n", uV, buck);
			return 0;
		}
		break;
	default:
		debug("%d is wrong voltage for BUCK%d\n", uV, buck);
		return -EINVAL;
	}

	if (uV <= 1600000)
		hex = (uV - 600000) / 12500;
	else
		hex = 0x50 + (uV - 1600000) / 50000;

	debug("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, hex);

	return hex;
}

int marvell88pm820_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV)
{
	unsigned int val, ret, adr = 0;
	int hex = 0;

	if (buck < 1 || buck > 5) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}

	switch (buck) {
	case 1:
		adr = PM820_ID_BUCK1;
		break;
	case 2:
		adr = PM820_ID_BUCK2;
		break;
	case 3:
		adr = PM820_ID_BUCK3;
		break;
	case 4:
		adr = PM820_ID_BUCK4;
		break;
	case 5:
		adr = PM820_ID_BUCK5;
		break;
	default:
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -EINVAL;
	}

	hex = pm820_buck_volt2hex(buck, uV);

	if (hex < 0)
		return -1;

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	val &= ~MARVELL88PM_BUCK_VOLT_MASK;
	val |= hex;
	ret = pmic_reg_write(p, adr, val);

	return ret;
}

static int pm820_buck_hex2volt(unsigned int buck, unsigned int hex)
{
	unsigned int uV = 0;

	if (hex <= 0x50)
		uV = hex * 12500 + 600000;
	else
		uV = 1600000 + (hex - 0x50) * 50000;

	debug("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, hex);

	return uV;
}

int marvell88pm820_get_buck_vol(struct pmic *p, unsigned int buck)
{
	unsigned int val, ret, adr = 0;
	int uV;

	if (buck < 1 || buck > 5) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}

	switch (buck) {
	case 1:
		adr = PM820_ID_BUCK1;
		break;
	case 2:
		adr = PM820_ID_BUCK2;
		break;
	case 3:
		adr = PM820_ID_BUCK3;
		break;
	case 4:
		adr = PM820_ID_BUCK4;
		break;
	case 5:
		adr = PM820_ID_BUCK5;
		break;
	default:
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -EINVAL;
	}

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	uV = pm820_buck_hex2volt(buck, val);
	if (uV < 0)
		return -1;

	return uV;
}

static int pm820_ldo_volt2hex(unsigned int ldo, unsigned int uV)
{
	unsigned int *voltage_table = NULL, table_size = 0, hex = 0;
	/*choose ldo voltage table*/
	switch (ldo) {
	case 1:
	case 2:
	case 13:
		voltage_table = ldo1_2_13_voltage_table;
		table_size = ARRAY_SIZE(ldo1_2_13_voltage_table);
		break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
		voltage_table = ldo3_to_11_voltage_table;
		table_size = ARRAY_SIZE(ldo3_to_11_voltage_table);
		break;
	case 12:
		voltage_table = ldo12_voltage_table;
		table_size = ARRAY_SIZE(ldo12_voltage_table);
		break;
	case 14:
		voltage_table = ldo14_voltage_table;
		table_size = ARRAY_SIZE(ldo14_voltage_table);
		break;
	default:
		debug("%s: %d is wrong LDO number\n", __func__, ldo);
		return -EINVAL;
	}

	for (hex = 0; hex < table_size; hex++) {
		if (uV <= voltage_table[hex]) {
			debug("ldo %d, voltage %d, reg value 0x%x\n", ldo, uV, hex);
			return hex;
		}
	}

	return table_size - 1;
}
int marvell88pm820_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV)
{
	unsigned int val, ret, adr, ldo_en_mask;
	int hex = 0;

	if (ldo < 1 || ldo > 14) {
		printf("%s: %d is wrong ldo number\n", __func__, ldo);
		return -1;
	}

	if (ldo == 1)
		adr = PM820_ID_LDO1;
	else if (ldo  == 2)
		adr = PM820_ID_LDO2;
	else
		adr = PM820_ID_LDO2 + ldo - 2;

	hex = pm820_ldo_volt2hex(ldo, uV);

	if (hex < 0)
		return -1;

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	val &= ~MARVELL88PM_LDO_VOLT_MASK;
	val |= hex;
	ret = pmic_reg_write(p, adr, val);

	/* check whether the LDO is enabled, if not enable it.*/
	adr = PM820_LDO1_8_EN1 + (ldo - 1)/8;
	ldo_en_mask = (ldo - 1)%8;

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	val |= (1 << ldo_en_mask);
	ret = pmic_reg_write(p, adr, val);

	return ret;
}

int pmic_88pm820_alloc(unsigned char bus, struct pmic_chip_desc *chip)
{
	struct pmic *p_base = pmic_alloc();
	struct pmic *p_power = pmic_alloc();
	struct pmic *p_gpadc = pmic_alloc();
	if (!p_base || !p_power || !p_gpadc) {
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
	chip->charger_name = NULL;
	chip->fuelgauge_name = NULL;
	chip->battery_name = NULL;

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

	/* get functions */
	chip->set_buck_vol = marvell88pm820_set_buck_vol;
	chip->set_ldo_vol = marvell88pm820_set_ldo_vo;
	chip->get_buck_vol = marvell88pm820_get_buck_vol;
	chip->reg_update = marvell88pm820_reg_update;

	puts("Board PMIC init\n");

	return 0;
}

void pmic_88pm820_base_init(struct pmic *p_base)
{
	u32 val;
	/* Enable 32Khz-out-2 low jitter XO_LJ = 1 */
	val = 0x20;
	pmic_reg_write(p_base, 0x21, val);

	/* 0xe2 and 0xd0 registers need to set at the same time to
	 * make 32k clock work. there are  two options:
	 * 1)  0xd0 = 0, 0xe2 = 0x05 or 0x06
	 * 2)  0xd0 bit 7 is 1 , 0xe2 = 0xa (the formal one we use)
	 */
	/*base page:reg 0xd0.7 = 1 32kHZ generated from XO */
	pmic_reg_read(p_base, 0xd0, &val);
	val |= (1 << 7);
	pmic_reg_write(p_base, 0xd0, val);
	/* Enable external 32KHZ clock in pmic */
	/*Enable 32Khz-out-from XO 1, 2 all enabled */
	pmic_reg_read(p_base, 0xe2, &val);
	val = (val & (~0x0f)) | 0x0a;
	pmic_reg_write(p_base, 0xe2, val);


	/* set oscillator running locked to 32kHZ*/
	pmic_reg_read(p_base, 0x50, &val);
	val &= ~(1 << 1);
	pmic_reg_write(p_base, 0x50, val);

	pmic_reg_read(p_base, 0x55, &val);
	val &= ~(1 << 1);
	pmic_reg_write(p_base, 0x55, val);

	/*
	 * print out power up/down log
	 * save it in EMMD
	 */
	get_powerup_down_log();

}

void pmic_88pm820_power_init(struct pmic *p_power)
{
	u32 val;
	/* Set buck1 DVC ramp up rate to 12.5mV/us */
	pmic_reg_read(p_power, 0x78, &val);
	val &= ~(0x7 << 3);
	val |= (0x6 << 3);
	pmic_reg_write(p_power, 0x78, val);

	/* change drive selection from half to full */
	val = 0xff;
	pmic_reg_write(p_power, 0x3a, val);
	pmic_reg_write(p_power, 0x3b, val);
}

int pmic_88pm820_init(struct pmic_chip_desc *chip)
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
	pmic_88pm820_base_init(p_base);
	board_pmic_base_fixup(p_base);

	/*------------gpadc page setting -----------*/
	p_gpadc = pmic_get(chip->gpadc_name);
	if (!p_gpadc)
		return -1;
	if (pmic_probe(p_gpadc))
		return -1;
	pmic_88pm820_gpadc_init(p_gpadc);
	board_pmic_gpadc_fixup(p_gpadc);

	/*------------power page setting -----------*/
	p_power = pmic_get(chip->power_name);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;
	pmic_88pm820_power_init(p_power);
	board_pmic_power_fixup(p_power);

	return 0;
}

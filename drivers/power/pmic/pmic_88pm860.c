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

#define MARVELL_PMIC_BASE	"88pm860_base"
#define MARVELL_PMIC_POWER	"88pm860_power"
#define MARVELL_PMIC_GPADC	"88pm860_gpadc"

#define DVC_CONTROL_REG	0x4F

static struct pmic_chip_desc *g_chip;

enum {
	PM860_ID_LDO1 = 0x08,
	PM860_ID_LDO2 = 0x0b,
	PM860_ID_LDO3,
	PM860_ID_LDO4,
	PM860_ID_LDO5,
	PM860_ID_LDO6,
	PM860_ID_LDO7,
	PM860_ID_LDO8,
	PM860_ID_LDO9,
	PM860_ID_LDO10,
	PM860_ID_LDO11,
	PM860_ID_LDO12,
	PM860_ID_LDO13,
	PM860_ID_LDO14,
	PM860_ID_LDO15,
	PM860_ID_LDO16,
	PM860_ID_LDO17,
	PM860_ID_LDO18,
	PM860_ID_LDO19,
	PM860_ID_LDO20 = 0x1d,

	PM860_ID_BUCK1 = 0x3c,
	PM860_ID_BUCK2 = 0x40,
	PM860_ID_BUCK3 = 0x41,
	PM860_ID_BUCK4 = 0x45,
	PM860_ID_BUCK5 = 0x46,
	PM860_ID_BUCK6 = 0x4a,

	PM860_BUCK_EN1 = 0x50,
	PM860_LDO1_8_EN1 = 0x51,
	PM860_LDO9_16_EN1 = 0x52,
	PM860_LDO17_20_EN1 = 0x53,

};

static unsigned int ldo1_to_3_voltage_table[] = {
	1700000, 1800000, 1900000, 2500000, 2800000, 2900000, 3100000, 3300000,
};
static unsigned int ldo3_to_18_voltage_table[] = {
	1200000, 1250000, 1700000, 1800000, 1850000, 1900000, 2500000, 2600000,
	2700000, 2750000, 2800000, 2850000, 2900000, 3000000, 3100000, 3300000,
};
static unsigned int ldo19_voltage_table[] = {
	600000,  650000,  700000,  750000,  800000,  850000,  900000,  950000,
	1000000, 1050000, 1100000, 1150000, 1200000, 1300000, 1400000, 1500000,
};
static unsigned int ldo20_voltage_table[] = {
	1700000, 1800000, 1900000, 2000000, 2100000, 2500000, 2700000, 2800000,
};

enum {
	PM860_VERSION_Z3 = 0,
	PM860_VERSION_A0 = 1,
};

static int get_pm860_version(struct pmic *p_base)
{
	u32 val;
	int ret = 0;

	ret = pmic_reg_read(p_base, 0x0, &val);
	if (ret) {
		printf("%s: read base page 0x0 register fails.\n", __func__);
		return PM860_VERSION_A0;
	}
	if (val == 0x91)
		return PM860_VERSION_A0;
	else
		return PM860_VERSION_Z3;
}

int marvell88pm860_reg_update(struct pmic *p, int reg, unsigned int regval)
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

static int pm860_buck_volt2hex(unsigned int buck, unsigned int uV)
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
	case 5:
	case 6:
		if (uV > 3300000) {
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

int marvell88pm860_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV)
{
	unsigned int val, ret, adr = 0;
	int hex = 0;

	if (buck < 1 || buck > 6) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}
	switch (buck) {
	case 1:
		adr = PM860_ID_BUCK1;
		break;
	case 2:
		adr = PM860_ID_BUCK2;
		break;
	case 3:
		adr = PM860_ID_BUCK3;
		break;
	case 4:
		adr = PM860_ID_BUCK4;
		break;
	case 5:
		adr = PM860_ID_BUCK5;
		break;
	case 6:
		adr = PM860_ID_BUCK6;
		break;
	default:
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -EINVAL;
	}

	hex = pm860_buck_volt2hex(buck, uV);

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

static int pm860_buck_hex2volt(unsigned int buck, unsigned int hex)
{
	unsigned int uV = 0;

	if (hex <= 0x50)
		uV = hex * 12500 + 600000;
	else
		uV = 1600000 + (hex - 0x50) * 50000;

	debug("%s: buck=%d, uV=%d, hex= %d\n", __func__, buck, uV, hex);

	return uV;
}

int marvell88pm860_get_buck_vol(struct pmic *p, unsigned int buck)
{
	unsigned int val, ret, adr = 0;
	int uV;

	if (buck < 1 || buck > 6) {
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -1;
	}

	switch (buck) {
	case 1:
		adr = PM860_ID_BUCK1;
		break;
	case 2:
		adr = PM860_ID_BUCK2;
		break;
	case 3:
		adr = PM860_ID_BUCK3;
		break;
	case 4:
		adr = PM860_ID_BUCK4;
		break;
	case 5:
		adr = PM860_ID_BUCK5;
		break;
	case 6:
		adr = PM860_ID_BUCK6;
		break;
	default:
		printf("%s: %d is wrong buck number\n", __func__, buck);
		return -EINVAL;
	}

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	uV = pm860_buck_hex2volt(buck, val);
	if (uV < 0)
		return -1;

	return uV;
}

static int pm860_ldo_volt2hex(unsigned int ldo, unsigned int uV)
{
	unsigned int *voltage_table = NULL, table_size = 0, hex = 0;
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
	case 16:
	case 17:
	case 18:
		voltage_table = ldo3_to_18_voltage_table;
		table_size = ARRAY_SIZE(ldo3_to_18_voltage_table);
		break;
	case 19:
		voltage_table = ldo19_voltage_table;
		table_size = ARRAY_SIZE(ldo19_voltage_table);
		break;
	case 20:
		voltage_table = ldo20_voltage_table;
		table_size = ARRAY_SIZE(ldo20_voltage_table);
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
int marvell88pm860_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV)
{
	unsigned int val, ret, adr, ldo_en_mask;
	int hex = 0;

	if (ldo < 1 || ldo > 20) {
		printf("%s: %d is wrong ldo number\n", __func__, ldo);
		return -1;
	}

	if (ldo == 1)
		adr = PM860_ID_LDO1;
	else if (ldo  == 2)
		adr = PM860_ID_LDO2;
	else
		adr = PM860_ID_LDO2 + ldo - 2;

	hex = pm860_ldo_volt2hex(ldo, uV);

	if (hex < 0)
		return -1;

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	val &= ~MARVELL88PM_LDO_VOLT_MASK;
	val |= hex;
	ret = pmic_reg_write(p, adr, val);

	/* check whether the LDO is enabled, if not enable it.*/
	adr = PM860_LDO1_8_EN1 + (ldo - 1)/8;
	ldo_en_mask = (ldo - 1)%8;

	ret = pmic_reg_read(p, adr, &val);
	if (ret)
		return ret;

	val |= (1 << ldo_en_mask);
	ret = pmic_reg_write(p, adr, val);

	return ret;
}

int pmic_88pm860_alloc(unsigned char bus, struct pmic_chip_desc *chip)
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
	chip->set_buck_vol = marvell88pm860_set_buck_vol;
	chip->set_ldo_vol = marvell88pm860_set_ldo_vol;
	chip->get_buck_vol = marvell88pm860_get_buck_vol;
	chip->reg_update = marvell88pm860_reg_update;

	puts("Board PMIC init\n");

	g_chip = chip;

	return 0;
}

void pmic_88pm860_base_init(struct pmic *p_base)
{
	u32 val;
	/* Enable low jitter XO_LJ = 1 */
	val = 0x20;
	pmic_reg_write(p_base, 0x21, val);

	/*disable watchdog timer*/
	val = 0x01;
	pmic_reg_write(p_base, 0x1d, val);

	/*xo_cap sel = 100 0xe8*/
	pmic_reg_read(p_base, 0xe8, &val);
	val |= (0x4 << 4);
	pmic_reg_write(p_base, 0xe8, val);

	/* use_xo = ON 0xd0 */
	pmic_reg_read(p_base, 0xd0, &val);
	val |= (1 << 7);
	pmic_reg_write(p_base, 0xd0, val);
	/* clk32k_out2_sel = 01 (bit2,3)*/
	pmic_reg_read(p_base, 0xe2, &val);
	val &= ~(0x3 << 2);
	val |= (1 << 2);
	pmic_reg_write(p_base, 0xe2, val);

	/* set oscillator running locked to 32kHZ*/
	pmic_reg_read(p_base, 0x50, &val);
	val &= ~(1 << 1);
	pmic_reg_write(p_base, 0x50, val);

	pmic_reg_read(p_base, 0x55, &val);
	val &= ~(1 << 1);
	pmic_reg_write(p_base, 0x55, val);

	/* enable pwr_hold */
	pmic_reg_read(p_base, 0x0d, &val);
	val |= (1 << 7);
	pmic_reg_write(p_base, 0x0d, val);

	/*
	 * set gpio3 and gpio4 to be DVC mode:
	 * a puzzle:
	 * for z3 version:
	 *   pin J4 -> gpio3, can be configured as dvc3
	 *   pin L2 -> gpio4, can be configured as dvc4
	 *   pin D6 -> gpio5, named as gpio3 when J4/L2 are configured as dvc
	 * for a0 version:
	 *   pin J4 -> gpio4, can be configured as dvc3
	 *   pin L2 -> gpio5, can be configured as dvc4
	 *   pin D6 -> gpio3
	 */
	if (get_pm860_version(p_base) == PM860_VERSION_Z3) {
		pmic_update_bits(p_base, 0x31, 0x7 << 5, 0x7 << 5);
		pmic_update_bits(p_base, 0x32, 0x7 << 5, 0x7 << 5);
	} else {
		pmic_reg_write(p_base, 0x32, 0xee);
	}

	/*
	 * print out power up/down log
	 * save it in EMMD
	 */
	get_powerup_down_log(g_chip);
}

void pmic_88pm860_gpadc_init(struct pmic *p_gpadc)
{}

void pmic_88pm860_power_init(struct pmic *p_power)
{
	u32 val;

	/* change drive selection from half to full */
	val = 0xff;
	pmic_reg_write(p_power, 0x3a, val);

	/* use 12.5mV/us for DVC ramp up */
	pmic_reg_read(p_power, 0x78, &val);
	val &= ~(0x7 << 3);
	val |= (0x6 << 3);
	pmic_reg_write(p_power, 0x78, val);

	/* enable buck1 sleep mode and set sleep voltage to 0.8v */
	pmic_update_bits(p_power, 0x30, 0x7f, 0x10);
	pmic_update_bits(p_power, 0x5a, 0x3, 0x2);
}

int pmic_88pm860_init(struct pmic_chip_desc *chip)
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
	pmic_88pm860_base_init(p_base);
	board_pmic_base_fixup(p_base);

	/*------------gpadc page setting -----------*/
	p_gpadc = pmic_get(chip->gpadc_name);
	if (!p_gpadc)
		return -1;
	if (pmic_probe(p_gpadc))
		return -1;
	pmic_88pm860_gpadc_init(p_gpadc);
	board_pmic_gpadc_fixup(p_gpadc);

	/*------------power page setting -----------*/
	p_power = pmic_get(chip->power_name);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;
	pmic_88pm860_power_init(p_power);
	board_pmic_power_fixup(p_power);

	return 0;
}

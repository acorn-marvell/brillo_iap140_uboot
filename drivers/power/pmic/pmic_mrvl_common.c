/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Xiang Wang <wangx@marvell.com>
 *  Yi Zhang <yizhang@marvell.com>
 *  Fenghang Yin <yinfh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/battery.h>
#include <power/marvell88pm_pmic.h>
#include <emmd_rsv.h>
#include <asm/errno.h>

__weak void board_pmic_base_fixup(struct pmic *p_base) {}
__weak void board_pmic_power_fixup(struct pmic *p_power) {}
__weak void board_pmic_buck_fixup(struct pmic *p_buck) {}
__weak void board_pmic_ldo_fixup(struct pmic *p_ldo) {}
__weak void board_pmic_gpadc_fixup(struct pmic *p_gpadc) {}
__weak void board_pmic_battery_fixup(struct pmic *p_gpadc) {}
__weak void pmic_pm886_powerdown(struct pmic *p_base) {}
__weak int pmic_pm886_onkey_check(struct pmic *p_base) {return 0; }
__weak void pmic_pm880_powerdown(struct pmic *p_base) {}

void pmic_default_powerdown(struct pmic *p_base)
{
	pmic_reg_write(p_base, PMIC_WAKE_UP, PMIC_SW_PDOWN);
}

void pmic_default_reset_bd(struct pmic *p_base)
{
	pmic_reg_write(p_base, 0x0d, 0x20);
}

static u8 pmic_marvell_get_chip_id(u8 bus, u8 reg)
{
	u8 data, old_bus;
	int ret;

	old_bus = i2c_get_bus_num();
	debug("---%s: old_bus = 0x%x\n", __func__, old_bus);

	if (old_bus != bus)
		i2c_set_bus_num(bus);
	/* suppose all of the Marvell PMIC base page addres is 0x30 */
	ret = i2c_read(0x30, reg, 1, &data, 1);

	if (old_bus != bus)
		i2c_set_bus_num(old_bus);
	if (ret < 0)
		return 0;
	else
		return data;
}

int pmic_marvell_register(unsigned char bus, struct pmic_chip_desc *chip)
{
	int ret = 0;

	if (!chip) {
		printf("%s: chip description is empty!\n", __func__);
		return -EINVAL;
	}

	/* set default reset bd function */
	chip->reset_bd = pmic_default_reset_bd;

	switch (chip->chip_id) {
#ifdef CONFIG_POWER_88PM800
	case PM800:
		/* 88pm800 */
		ret = pmic_88pm800_alloc(bus, chip);
		if (ret < 0)
			goto out;
		ret = pmic_88pm800_init(chip);
		if (ret < 0)
			goto out;
		break;
#endif

#ifdef CONFIG_POWER_88PM820
	case PM820:
		/* 88pm822 */
		ret = pmic_88pm820_alloc(bus, chip);
		if (ret < 0)
			goto out;
		ret = pmic_88pm820_init(chip);
		if (ret < 0)
			goto out;
		break;
#endif
#ifdef CONFIG_POWER_88PM860
	case PM860_ZX:
	case PM860_A0:
		/* 88pm860 */
		ret = pmic_88pm860_alloc(bus, chip);
		if (ret < 0)
			goto out;
		ret = pmic_88pm860_init(chip);
		if (ret < 0)
			goto out;
		break;
#endif

#ifdef CONFIG_POWER_88PM886
	case PM886_A1:
	case PM886_A0:
		/* 88pm886 */
		ret = pmic_88pm886_alloc(bus, chip);
		if (ret < 0)
			goto out;
		ret = pmic_88pm886_init(chip);
		if (ret < 0)
			goto out;
		break;
#endif

#ifdef CONFIG_POWER_88PM880
	case PM880_A0:
	case PM880_A1:
		/* 88pm880 */
		ret = pmic_88pm880_alloc(bus, chip);
		if (ret < 0)
			goto out;
		ret = pmic_88pm880_init(chip);
		if (ret < 0)
			goto out;
		break;
#endif

	default:
		printf("--- %s: unsupported PMIC: 0x%x\n",
		       __func__, chip->chip_id);
		ret = -EINVAL;
		break;
	}

out:
	if (ret < 0)
		printf("---> %s: pmic: 0x%x fails\n", __func__, chip->chip_id);

	return ret;
}

int pmic_marvell_init(unsigned char bus, struct pmic_chip_desc *chip)
{
	int ret;
	if (!chip) {
		printf("%s: empty pmic chip pointer!\n", __func__);
		return -EINVAL;
	}

	/* suppose all of the chip id register is 0x0, base page */
	chip->chip_id = pmic_marvell_get_chip_id(bus, 0x0);

	ret = pmic_marvell_register(bus, chip);
	if (ret < 0)
		printf("%s: pmic: 0x%x init fails.\n", __func__, chip->chip_id);
	return ret;
}

void pmic_reset_bd(struct pmic_chip_desc *chip)
{
	struct pmic *p_base;

	p_base = pmic_get(chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return;
	}
	/* 1.Enable FAULT_WU and FAULT_WU_EN, FAULT_WU can not be set until
	 *   FAULT_WU_EN is enabled */
	pmic_update_bits(p_base, 0xe7, 0x04, 0x04);
	pmic_update_bits(p_base, 0xe7, 0x08, 0x08);
	/* 2.Issue SW power down */
	if (!chip->reset_bd) {
		printf("error! there is no reset function\n");
		return;
	}
	chip->reset_bd(p_base);
	/* Rebooting... */
}

static inline int append_pm_reason_into_str(u32 power_reasons_bits, const char * const *power_name,
					u32 power_arr_size, int chars_left, char *power_reason)
{
	u32 bit;

	for (bit = 0; (bit < power_arr_size) && (chars_left > 0); bit++) {
		if ((power_reasons_bits >> bit) & 1) {
				strncat(power_reason, power_name[bit], chars_left);
				chars_left -= strlen(power_name[bit]);
				if (chars_left > 0) {
					strncat(power_reason, "\n", chars_left);
					chars_left--;
				}
		}
	}
	return chars_left;
}

/* get_powerup_down_log(void) function do following things:
 * save power up/down log in DDR for debug,
 * print them out
 */
void get_powerup_down_log(struct pmic_chip_desc *chip)
{
	u32 val, powerup_l, powerD_l1, powerD_l2, bit;
	struct emmd_page *emmd_pg = (struct emmd_page *)CRASH_BASE_ADDR;
	char *power_reason = &emmd_pg->pmic_power_reason[0];
	int chars_left = sizeof(emmd_pg->pmic_power_reason) - 1;

	struct pmic *p_base;
	static const char * const powerup_name[8] = {
		"ONKEY_WAKEUP    ",
		"CHG_WAKEUP      ",
		"EXTON_WAKEUP    ",
#if (defined CONFIG_POWER_88PM886) || (defined CONFIG_POWER_88PM880)
		"SMPL_WU_LOG     ",
#else
		"RSVD            ",
#endif
		"RTC_ALARM_WAKEUP",
		"FAULT_WAKEUP    ",
		"BAT_WAKEUP      ",
#ifdef CONFIG_POWER_88PM880
		"WLCHG_WAKEUP    ",
#else
		"RSVD            ",
#endif
	};
	static const char * const powerD1_name[8] = {
		"OVER_TEMP ",
#if (defined CONFIG_POWER_88PM886) || (defined CONFIG_POWER_88PM880)
		"UV_VANA5  ",
#else
		"UV_VSYS1  ",
#endif
		"SW_PDOWN  ",
		"FL_ALARM  ",
		"WD        ",
		"LONG_ONKEY",
		"OV_VSYS   ",
		"RTC_RESET "
	};
	static const char * const powerD2_name[6] = {
		"HYB_DONE   ",
#if (defined CONFIG_POWER_88PM886) || (defined CONFIG_POWER_88PM880)
		"UV_VBAT    ",
#else
		"UV_VSYS2   ",
#endif
#ifdef CONFIG_POWER_88PM880
		"HW_RESET2  ",
#else
		"HW_RESET   ",
#endif
		"PGOOD_PDOWN",
		"LONKEY_RTC ",
#ifdef CONFIG_POWER_88PM880
		"HW_RESET1  ",
#else
		"RSVD       ",
#endif
	};


	p_base = pmic_get(chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return;
	}

	/*
	 * dump power up/down log
	 * save the power down log registers in  pmic rtc expire registers
	 */
	pmic_reg_read(p_base, POWER_UP_LOG, &powerup_l);
	pmic_reg_read(p_base, POWER_DOWN_LOG1, &powerD_l1);
	pmic_reg_read(p_base, POWER_DOWN_LOG2, &powerD_l2);

	printf("PowerUP log register:0x%x\n", powerup_l);
	printf(" ------------------------------------\n");
	printf("|     name(power up)      |  status  |\n");
	printf("|-------------------------|----------|\n");
	for (bit = 0; bit < ARRAY_SIZE(powerup_name); bit++)
		printf("|    %s     |    %x     |\n", powerup_name[bit], (powerup_l >> bit) & 1);
	printf(" ------------------------------------\n");

	printf("PowerDown log register1:0x%x\n", powerD_l1);
	printf(" -------------------------------\n");
	printf("|  name(power down1) |  status  |\n");
	printf("|--------------------|----------|\n");
	for (bit = 0; bit < ARRAY_SIZE(powerD1_name); bit++)
		printf("|    %s      |    %x     |\n", powerD1_name[bit], (powerD_l1 >> bit) & 1);
	printf(" -------------------------------\n");

	printf("PowerDown log register2:0x%x\n", powerD_l2);
	printf(" -------------------------------\n");
	printf("|  name(power down2) |  status  |\n");
	printf("|--------------------|----------|\n");
	for (bit = 0; bit < ARRAY_SIZE(powerD2_name); bit++)
		printf("|    %s     |    %x     |\n", powerD2_name[bit], (powerD_l2 >> bit) & 1);
	printf(" -------------------------------\n");

	/* write power up/down log to DDR */
	val = powerup_l | (powerD_l1 << 8) | (powerD_l2 << 16);
	emmd_pg->pmic_power_status = val;

	/* write power up/down reasons to ramdump area */
	memset(&emmd_pg->pmic_power_reason[0], ' ', sizeof(emmd_pg->pmic_power_reason));
	emmd_pg->pmic_power_reason[0] = 0;

	chars_left = append_pm_reason_into_str(powerup_l, powerup_name, ARRAY_SIZE(powerup_name),
					chars_left, power_reason);
	chars_left = append_pm_reason_into_str(powerD_l1, powerD1_name, ARRAY_SIZE(powerD1_name),
					chars_left, power_reason);
	chars_left = append_pm_reason_into_str(powerD_l2, powerD2_name, ARRAY_SIZE(powerD2_name),
					chars_left, power_reason);
}

enum power_key_status pmic_powerkey_state(void)
{
	int val;
	struct pmic *p_base;
	struct pmic_chip_desc *p_chip;
	enum power_key_status status = POWER_KEY_UNKNOWN;

	p_chip = get_marvell_pmic();
	p_base = pmic_get(get_marvell_pmic()->base_name);
	if (!p_chip || !p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return status;
	}

	switch (p_chip->chip_id) {
	case PM886_A0:
	case PM886_A1:
		/* 88pm886 */
		val = pmic_pm886_onkey_check(p_base);
		if (val >= 0)
			status = val ? POWER_KEY_DOWN : POWER_KEY_UP;
		break;
	default:
		break;
	}

	return status;
}

enum sys_boot_up_reason get_boot_up_reason(u32 *emmd_pmic_addr)
{
	u32 pmic_log = *emmd_pmic_addr;
	/* [power down log2: 0xe6][power down log1: 0xe5][power up log: 0x10] */
	if (!(pmic_log & 0x1eff00))
		return SYS_BR_REBOOT;

	/* get power up log */
	pmic_log &= 0xff;

	switch (pmic_log) {
	case 0x1:  return SYS_BR_ONKEY;
	case 0x2:  return SYS_BR_CHARGE;
	case 0x10: return SYS_BR_RTC_ALARM;
	case 0x20:  return SYS_BR_FAULT_WAKEUP;
	case 0x40:  return SYS_BR_BAT_WAKEUP;
	default: return SYS_BR_POWER_OFF;
	}
}

void pmic_enter_powerdown(struct pmic_chip_desc *chip)
{
	struct pmic *p_base;

	p_base = pmic_get(chip->base_name);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return;
	}

	/* wait before power off in order to complete console prints */
	udelay(500000);
	/* power down */
	switch (chip->chip_id) {
	case PM886_A0:
	case PM886_A1:
		/* 88pm886 */
		pmic_pm886_powerdown(p_base);
		break;
	case PM880_A0:
	case PM880_A1:
		/* 88pm880 */
		pmic_pm880_powerdown(p_base);
		break;
	default:
		pmic_default_powerdown(p_base);
		break;
	}
}

/* marvell pmic operations */
int marvell88pm_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV)
{
	struct pmic_chip_desc *p_chip = get_marvell_pmic();
	if (!p_chip || !p_chip->set_buck_vol) {
		printf("can't set buck %d voltage\n", buck);
		return -1;
	}
	return p_chip->set_buck_vol(p, buck, uV);
}

int marvell88pm_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV)
{
	struct pmic_chip_desc *p_chip = get_marvell_pmic();
	if (!p_chip || !p_chip->set_ldo_vol) {
		printf("can't set ldo %d voltage\n", ldo);
		return -1;
	}
	return p_chip->set_ldo_vol(p, ldo, uV);
}

int marvell88pm_get_buck_vol(struct pmic *p, unsigned int buck)
{
	struct pmic_chip_desc *p_chip = get_marvell_pmic();
	if (!p_chip || !p_chip->get_buck_vol) {
		printf("can't get buck %d vol\n", buck);
		return -1;
	}
	return p_chip->get_buck_vol(p, buck);
}

int marvell88pm_reg_update(struct pmic *p, int reg, unsigned int regval)
{
	struct pmic_chip_desc *p_chip = get_marvell_pmic();
	if (!p_chip || !p_chip->reg_update) {
		printf("can't update reg 0x%x\n", reg);
		return -1;
	}
	return p_chip->reg_update(p, reg, regval);
}

int get_powerup_mode(void)
{
	struct pmic_chip_desc *p_chip = get_marvell_pmic();
	if (!p_chip || !p_chip->get_powerup_mode) {
		printf("can't get power up mode\n");
		return -1;
	}
	return p_chip->get_powerup_mode();
}

/* power off charger famekwork init */
__weak int pm830_fg_alloc(unsigned char bus, struct pmic_chip_desc *chip) {return -1; }
__weak int pm830_chrg_alloc(unsigned char bus, struct pmic_chip_desc *chip) {return -1; }
__weak int pm830_bat_alloc(unsigned char bus, struct pmic_chip_desc *chip) {return -1; }
__weak int pm830_get_powerup_mode(void) {return INVALID_MODE; }

__weak int pm88x_fg_alloc(unsigned char bus, struct pmic_chip_desc *chip) {return -1; }
__weak int pm88x_chrg_alloc(unsigned char bus, struct pmic_chip_desc *chip) {return -1; }
__weak int pm88x_bat_alloc(unsigned char bus, struct pmic_chip_desc *chip) {return -1; }
__weak int pm886_get_powerup_mode(void) {return INVALID_MODE; }
__weak void pm886_battery_init(struct pmic *p_battery) {}
__weak int pm880_get_powerup_mode(void) {return INVALID_MODE; }
__weak void pm880_battery_init(struct pmic *p_battery) {}

int batt_marvell_init(struct pmic_chip_desc *chip,
		      u8 pmic_i2c_bus,
		      u8 chg_i2c_bus,
		      u8 fg_i2c_bus)
{
	int ret;
	struct pmic *p_fg, *p_chrg, *p_bat;

	if (!chip) {
		printf("--- %s: chip description is empty.\n", __func__);
		return -EINVAL;
	}

	switch(chip->chip_id) {
	case PM886_A0:
	case PM886_A1:
	case PM880_A0:
	case PM880_A1:
		ret = pm88x_chrg_alloc(chg_i2c_bus, chip);
		if (ret) {
			printf("init charger fails.\n");
			return -1;
		}

		ret = pm88x_fg_alloc(fg_i2c_bus, chip);
		if (ret) {
			printf("init fuelgauge fails.\n");
			return -1;
		}

		ret = pm88x_bat_alloc(pmic_i2c_bus, chip);
		if (ret) {
			printf("init charger/fuelgauge parent fails.\n");
			return -1;
		}

		if ((chip->chip_id == PM886_A0) || (chip->chip_id == PM886_A1)) {
			chip->get_powerup_mode = pm886_get_powerup_mode;
			chip->battery_init = pm886_battery_init;
		} else {
			chip->get_powerup_mode = pm880_get_powerup_mode;
			chip->battery_init = pm880_battery_init;
		}

		printf("%s: PMIC: 0x%x integrated with charger/fuelgauge\n",
		       __func__, chip->chip_id);

		break;

	default:
		/* let's use the name with 88pm830 first */
		ret = pm830_chrg_alloc(chg_i2c_bus, chip);
		if (ret) {
			printf("init charger fails.\n");
			return -1;
		}

		ret = pm830_fg_alloc(fg_i2c_bus, chip);
		if (ret) {
			printf("init fuelgauge fails.\n");
			return -1;
		}

		ret = pm830_bat_alloc(pmic_i2c_bus, chip);
		if (ret) {
			printf("init charger/fuelgauge parent fails.\n");
			return -1;
		}
		chip->get_powerup_mode = pm830_get_powerup_mode;
		break;
	}

	p_chrg = pmic_get(chip->charger_name);
	if (!p_chrg) {
		printf("%s: access charger fails\n", chip->charger_name);
		return -1;
	}

	p_fg = pmic_get(chip->fuelgauge_name);
	if (!p_fg) {
		printf("%s: access fuelgauge fails\n", chip->fuelgauge_name);
		return -1;
	}

	p_bat = pmic_get(chip->battery_name);
	if (!p_bat) {
		printf("%s: access charger/fuelgauge parent fails\n",
		       chip->battery_name);
		return -1;
	}

	p_fg->parent = p_bat;
	p_chrg->parent = p_bat;

	/* do silicon related setting */
	if (chip->battery_init)
		chip->battery_init(p_bat);
	p_bat->pbat->battery_init(p_bat, p_fg, p_chrg, NULL);

	return 0;
}

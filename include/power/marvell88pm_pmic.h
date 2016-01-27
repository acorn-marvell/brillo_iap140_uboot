/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Hongyan Song <hysong@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MARVELL88PM_PMIC_H_
#define __MARVELL88PM_PMIC_H_

#include <power/pmic.h>
#include <asm/errno.h>

/* include necessary header files */
#ifdef CONFIG_POWER_88PM880
#include <power/88pm880.h>
#endif
#ifdef CONFIG_POWER_88PM886
#include <power/88pm886.h>
#endif
#ifdef CONFIG_POWER_88PM830
#include <power/88pm830.h>
#endif

#define MARVELL88PM_BUCK_VOLT_MASK		0x7f
#define MARVELL88PM_LDO_VOLT_MASK		0xf

#define MARVELL88PM_I2C_ADDR		0x30
#define PMIC_NUM_OF_REGS	0xff

#define PMIC_WAKE_UP	(0xd)
#define PMIC_SW_PDOWN	(1 << 5)

#define RTC_USR_DATA6 (0xEF)

#if (defined CONFIG_POWER_88PM886) || (defined CONFIG_POWER_88PM880)
#define POWER_UP_LOG  (0x17)
#else
#define POWER_UP_LOG  (0x10)
#endif

#define CHG_WAKEUP	(1 << 1)

#define POWER_DOWN_LOG1		(0xE5)
#define POWER_DOWN_LOG2		(0xE6)

#define LED_PWM_CONTROL7	(0x46)
#define LED_EN			(1 << 0)
#define R_LED_EN		(1 << 2)
#define LED_BLK_MODE		(1 << 4)

#define POWER_OFF_THRESHOLD	(3400000)	/* uV */

/* PMIC ID */
enum {
	PM800 = 0x3,
	PM820 = 0x4,
	PM860_ZX = 0x90,
	PM860_A0 = 0x91,
	PM886_A0 = 0x00,
	PM886_A1 = 0xa1,
	PM880_A0 = 0xb0,
	PM880_A1 = 0xb1,
};

struct pmic_chip_desc {
	const char *base_name;
	const char *power_name;
	const char *buck_name;
	const char *ldo_name;
	const char *gpadc_name;
	const char *led_name;
	const char *charger_name;
	const char *fuelgauge_name;
	const char *battery_name;
	int (*set_buck_vol)(struct pmic *p, unsigned int buck, unsigned int uV);
	int (*set_ldo_vol)(struct pmic *p, unsigned int ldo, unsigned int uV);
	int (*get_buck_vol)(struct pmic *p, unsigned int buck);
	int (*reg_update)(struct pmic *p, int reg, unsigned int regval);
	int (*get_powerup_mode)(void);
	void (*reset_bd)(struct pmic *p_base);
	void (*battery_init)(struct pmic *p_battery);
	u8 chip_id;
};

enum {
	INVALID_MODE,
	BATTERY_MODE,
	POWER_SUPPLY_MODE,
	CHARGER_ONLY_MODE,
	EXTERNAL_POWER_MODE,
};

enum sys_boot_up_reason {
	SYS_BR_POWER_OFF,
	SYS_BR_ONKEY,
	SYS_BR_CHARGE,
	SYS_BR_RTC_ALARM,
	SYS_BR_FAULT_WAKEUP,
	SYS_BR_BAT_WAKEUP,
	SYS_BR_REBOOT,
	SYS_BR_MAX,
};

enum power_key_status {
	POWER_KEY_UNKNOWN,
	POWER_KEY_UP,
	POWER_KEY_DOWN,
};

enum {CHARGER_ENABLE, CHARGER_DISABLE};

int marvell88pm_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV);
int marvell88pm_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV);
int marvell88pm_get_buck_vol(struct pmic *p, unsigned int buck);
int marvell88pm_reg_update(struct pmic *p, int reg, unsigned int regval);

int pmic_88pm800_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pmic_88pm820_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pmic_88pm860_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pmic_88pm886_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pmic_88pm880_alloc(unsigned char bus, struct pmic_chip_desc *chip);

int pmic_88pm800_init(struct pmic_chip_desc *chip);
int pmic_88pm820_init(struct pmic_chip_desc *chip);
int pmic_88pm860_init(struct pmic_chip_desc *chip);
int pmic_88pm886_init(struct pmic_chip_desc *chip);
int pmic_88pm880_init(struct pmic_chip_desc *chip);

void pmic_base_init(struct pmic *p_base);
void pmic_power_init(struct pmic *p_power);
void pmic_buck_init(struct pmic *p_buck);
void pmic_ldo_init(struct pmic *p_ldo);
void pmic_gpadc_init(struct pmic *p_gpadc);

void board_pmic_base_fixup(struct pmic *p_base);
void board_pmic_power_fixup(struct pmic *p_power);
void board_pmic_gpadc_fixup(struct pmic *p_gpadc);
void board_pmic_battery_fixup(struct pmic *p_battery);
void board_pmic_buck_fixup(struct pmic *p_buck);
void board_pmic_ldo_fixup(struct pmic *p_ldo);

int pmic_marvell_init(unsigned char i2c_bus, struct pmic_chip_desc *chip);

void pmic_reset_bd(struct pmic_chip_desc *chip);
void pmic88x_reset_bd(struct pmic_chip_desc *chip);
void get_powerup_down_log(struct pmic_chip_desc *chip);
enum sys_boot_up_reason get_boot_up_reason(u32 *emmd_pmic_addr);

int press_onkey_check(struct pmic_chip_desc *chip, int bootdelay);

void pmic_default_powerdown(struct pmic *p_base);
void pmic_pm886_powerdown(struct pmic *p_base);
int pmic_pm886_onkey_check(struct pmic *p_base);
int pmic_pm886_powerkey_state(struct pmic *p_base);
void pmic_pm880_powerdown(struct pmic *p_base);
void pmic_enter_powerdown(struct pmic_chip_desc *chip);

int batt_marvell_init(struct pmic_chip_desc *chip, u8 pmic_i2c_bus,
		      u8 chg_i2c_bus, u8 fg_i2c_bus);
int get_powerup_mode(void);

/* init funtions */
int pm886_get_powerup_mode(void);
void pm886_battery_init(struct pmic *p_battery);
int pm880_get_powerup_mode(void);
void pm880_battery_init(struct pmic *p_battery);

int pm88x_fg_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pm88x_chrg_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pm88x_bat_alloc(unsigned char bus, struct pmic_chip_desc *chip);

int pm830_get_powerup_mode(void);
int pm830_fg_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pm830_chrg_alloc(unsigned char bus, struct pmic_chip_desc *chip);
int pm830_bat_alloc(unsigned char bus, struct pmic_chip_desc *chip);

struct pmic_chip_desc *get_marvell_pmic(void);
enum power_key_status pmic_powerkey_state(void);

#endif /* __MARVELL88PM_PMIC_H_ */

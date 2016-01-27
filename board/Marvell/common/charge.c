/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Yi Zhang <yizhang@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "charge.h"
DECLARE_GLOBAL_DATA_PTR;

static bool check_charger_fg(struct pmic *p_bat)
{
	int charger_type;
	struct pmic *p_chrg;

	if (!p_bat || !p_bat->pbat || !p_bat->pbat->chrg) {
		printf("charger is NULL\n");
		return false;
	}
	p_chrg = p_bat->pbat->chrg;

	charger_type = p_chrg->chrg->chrg_type(p_chrg);
	printf("charger type: %d\n", charger_type);

	if (!p_chrg->chrg->chrg_bat_present(p_chrg)) {
		printf("No battery detected\n");
		return false;
	}
	return true;
}

static void do_charging(struct pmic *p_bat, u32 target_uv)
{
	if (!p_bat || !p_bat->pbat)
		return;

	if (p_bat->pbat->battery_charge)
		p_bat->pbat->battery_charge(p_bat, target_uv);
}

/*
 * [ 0,     0     ] --> no charge in uboot, show charging logo and boot
 * [ lo_uv, 0     ] --> charge only until lo_uv
 * [ 0,     hi_uv ] --> show logo and charge until hi_uv
 * [ lo_uv, hi_uv ] --> charge until lo_uv, show logo, and then charge until hi_uv
 */
static void loop_charge(struct pmic *p_bat, int lo_uv, u32 hi_uv,
		void (*charging_indication)(bool))
{
	if (!p_bat || !p_bat->pbat) {
		printf("%s: battery information is NULL!\n", __func__);
		return;
	}

	/* for sequence test */
	if ((lo_uv == 0) && (hi_uv == 0)) {
		/* TODO: show logo here */
		return;
	}

	printf("\n%s begins...\n\n", __func__);

	/* update battery voltage and compare it with the threshold */
	p_bat->fg->fg_battery_update(p_bat->pbat->fg, p_bat);

	if (p_bat->pbat->bat->voltage_uV <= lo_uv) {
		printf("charging phase #1\n");
		do_charging(p_bat, lo_uv);
	}

	if (p_bat->pbat->bat->voltage_uV <= hi_uv) {
		if (charging_indication)
			(*charging_indication)(true);
		printf("charging phase #2\n");
		do_charging(p_bat, hi_uv);
		if (charging_indication)
			(*charging_indication)(false);
	}

	printf("\n%s finishes...\n\n", __func__);
}

static bool add_charger_cmdline(void)
{
	char *cmdline = malloc(COMMAND_LINE_SIZE);
	if (!cmdline) {
		printf("%s: alloc for cmdline fails.\n", __func__);
		return false;
	}
	/*
	 * we need to access before the env is relocated,
	 * gd->env_buf is too small, don't use getenv();
	 */
	if (gd->flags & GD_FLG_ENV_READY) {
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		printf("%s: getenv()\n", __func__);
	} else {
		getenv_f("bootargs", cmdline, COMMAND_LINE_SIZE);
		printf("%s: getenv_f()\n", __func__);
	}
	debug("%s: bootargs = %s\n", __func__, getenv("bootargs"));
	sprintf(cmdline + strlen(cmdline), " androidboot.mode=charger");
	if (setenv("bootargs", cmdline)) {
		printf("%s: set bootargs fails\n", __func__);
		free(cmdline);
		return false;
	}
	debug("%s: bootargs = %s\n", __func__, getenv("bootargs"));

	free(cmdline);
	return true;
}

void power_off_charge(u32 *emmd_pmic, u32 lo_uv,
		u32 hi_uv, void (*charging_indication)(bool))
{
	struct pmic *p_bat;
	struct pmic_chip_desc *board_pmic_chip;
	bool chg_fg_status, cmd_status;
	enum sys_boot_up_reason reason;
	bool force_chg = !!((lo_uv == 0) && (hi_uv == 0));

	board_pmic_chip = get_marvell_pmic();
	if (!board_pmic_chip) {
		printf("--- %s: chip is NULL.\n", __func__);
		return;
	}

	p_bat = pmic_get(board_pmic_chip->battery_name);
	if (!p_bat) {
		printf("get battery pmic fails!\n");
		if (!force_chg)
			return;
	}

	reason = get_boot_up_reason(emmd_pmic);
	printf("boot up reason = %d\n", reason);
	if ((reason == SYS_BR_CHARGE) || force_chg) {
		chg_fg_status = check_charger_fg(p_bat);
		if (!chg_fg_status) {
			printf("charger/fuelgauge status is not right!\n");
			if (!force_chg)
				return;
		}

		/* if "force_chg" is true, we are sure to arrive here */
		cmd_status = add_charger_cmdline();
		if (!cmd_status) {
			printf("add charger cmdline failes");
			return;
		}
		/* TODO: disable backlight */
		/* setop(0); */

		printf("run cp_d2 to put cp/msa into LPM\n");
		run_command("cp_d2", 0);

		/* charge until the system can boot up */
		loop_charge(p_bat, lo_uv, hi_uv, charging_indication);

		/* if charger is removed, shut down the device */
		if (!p_bat->chrg->chrg_type(p_bat->pbat->chrg)) {
			printf("charger is removed, power down!\n");
			pmic_enter_powerdown(board_pmic_chip);
		}
	} else {
		printf("%s: do nothing for charge.\n", __func__);
		/*
		 * battery voltage is too low and charger is not present, just power down the device
		 * TODO: power down voltage should be chosen by the customer
		 * here we choose 3.4V as power down voltage
		 */
		p_bat->fg->fg_battery_update(p_bat->pbat->fg, p_bat);
		if ((!p_bat->chrg->chrg_type(p_bat->pbat->chrg)) &&
				(p_bat->pbat->bat->voltage_uV < 3400000)) {
			printf("batt volt is low (%dmV) and charger is not present, power down\n",
					p_bat->pbat->bat->voltage_uV / 1000);
			pmic_enter_powerdown(board_pmic_chip);
		}
		return;
	}
}

int do_poff_chg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u8 pmic_i2c, chg_i2c, fg_i2c;
	u32 *emmd_pmic;
	u32 lo_uv, hi_uv;

	if (argc != 7)
		return -1;

	emmd_pmic = (void *)simple_strtoul(argv[1], NULL, 16);
	pmic_i2c = simple_strtoul(argv[2], NULL, 16);
	chg_i2c = simple_strtoul(argv[3], NULL, 16);
	fg_i2c = simple_strtoul(argv[4], NULL, 16);
	lo_uv = simple_strtoul(argv[5], NULL, 10);
	hi_uv = simple_strtoul(argv[6], NULL, 10);

	batt_marvell_init(get_marvell_pmic(), pmic_i2c, chg_i2c, fg_i2c);
	power_off_charge((u32 *)emmd_pmic, lo_uv, hi_uv, NULL);

	return 0;
}

static char help_text[] =
	"Usage:\n"
	"pwroffchg emmd_pmic_addr pmic_i2c_number chg_i2c_number fg_i2c_number"
	" low_volt high_volt\n"
	"if you choose low_volt and high_volt as 0, it enters into test mode:\n"
	"only show uboot charging logo and set cmmdline\n"
	"for example:\n"
	"pwroffchg 0x8140040 2 2 2 0 0"
	"";

U_BOOT_CMD(
	pwroffchg, 7, 1, do_poff_chg,
	"Test powr off charging", help_text
);

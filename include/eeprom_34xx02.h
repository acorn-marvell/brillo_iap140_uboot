/*
 * eeprom_34xx02.h EEPROM 34xx02 support
 *
 * Copyright (C) 2013 MARVELL Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

struct eeprom_data {
	u8 addr;
	unsigned int index;
	unsigned int i2c_num;
};

struct eeprom1_map {
	/* only list permanently protected area */
	u8 board_name[64];
	u8 board_sn[32];
	u8 board_rev[1];
	u8 board_type[1];
	u8 board_month[1];
	u8 board_year[1];
	u8 board_mac_addr[6];
};

struct eeprom2_map {
	/* only list permanently protected area */
	u8 board_category[32];
	u8 board_sn[32];
	u8 chip_name[16];
	u8 chip_stepping[8];
	u8 board_reg_date[16];
	u8 board_state[8];
	u8 user_team[24];
	u8 current_user[24];
	u8 board_eco[16];
	u8 lcd_resolution[16];
	u8 lcd_screen_size[8];
	u8 ddr_type[8];
	u8 ddr_size_speed[8];
	u8 emmc_type[8];
	u8 emmc_size[8];
	u8 rf_name_ver[8];
	u8 rf_type[16];
};

int eeprom_get_map(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_rev(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_type(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_sn(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_name(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_eco(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_manu_y_m(struct eeprom_data *peeprom_data);
int eeprom_get_board_category(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_chip_name(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_chip_stepping(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_board_reg_date(struct eeprom_data *peeprom_data);
int eeprom_get_board_state(struct eeprom_data *peeprom_data);
int eeprom_get_user_team(struct eeprom_data *peeprom_data);
int eeprom_get_current_user(struct eeprom_data *peeprom_data);
int eeprom_get_lcd_resolution(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_lcd_screen_size(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_ddr_type(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_ddr_size_speed(struct eeprom_data *peeprom_data);
int eeprom_get_ddr_size(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_ddr_speed(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_emmc_type(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_emmc_size(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_rf_name_ver(struct eeprom_data *peeprom_data);
int eeprom_get_rf_type(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_rf_name(struct eeprom_data *peeprom_data, u8 *data);
int eeprom_get_rf_info(struct eeprom_data *peeprom_data, u8 *data);

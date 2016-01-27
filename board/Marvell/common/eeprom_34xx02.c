/*
 * eeprom_34xx02.c EEPROM 34xx02 support
 *
 * Copyright (C) 2013 MARVELL Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <i2c.h>
#include <malloc.h>
#include <eeprom_34xx02.h>

/* I2C address */
#define CHIP_ADDR	0x50

/* EEPROM1 registers*/
#define EEPROM1_BOARD_NAME	0x0
#define EEPROM1_BOARD_SN	0x40
#define EEPROM1_BOARD_REV	0x60
#define EEPROM1_BOARD_TYPE	0x61
#define EEPROM1_BOARD_MONTH	0x62
#define EEPROM1_BOARD_YEAR	0x63
#define EEPROM1_BOARD_ECO	0xf8

/* EEPROM2 registers*/
#define EEPROM2_BOARD_CATEGORY  0x00
#define EEPROM2_BOARD_SN	0x20
#define EEPROM2_CHIP_NAME       0x40
#define EEPROM2_CHIP_STEP       0x50
#define EEPROM2_BOARD_REG_DATE  0x58
#define EEPROM2_BOARD_STATE     0x68
#define EEPROM2_USER_TEAM       0x70
#define EEPROM2_CURRENT_USER    0x88
#define EEPROM2_BOARD_ECO	0xA0
#define EEPROM2_LCD_RESOLUTION  0xB0
#define EEPROM2_LCD_SRC_SIZE    0xC0
#define EEPROM2_DDR_TYPE        0xC8
#define EEPROM2_DDR_SIZE_SPEED  0xD0
#define EEPROM2_EMMC_TYPE       0xD8
#define EEPROM2_EMMC_SIZE       0xE0
#define EEPROM2_RF_NAME_VERSION 0xE8
#define EEPROM2_RF_TYPE         0xF0
#define EEPROM2_RF_NAME         0xE8
#define EEPROM2_RF_INFO         0xF0

void print_ascii(uchar *data, unsigned int len)
{
	unsigned int i;
	for (i = 0; i < len; i++) {
		if (*data == 0)
			break;
		else if ((*data < 0x20) || (*data > 0x7e))
			puts(".");
		else
			printf("%c", *data);
		data++;
	}
	putc('\n');
}

static int get_eeprom(struct eeprom_data *peeprom_data, unsigned int offset,
						unsigned int len, u8 *data)
{
	int ret = 0;
	unsigned int i;
	unsigned int count = len >> 4;
	unsigned int more = len - (count << 4);
	unsigned int bus = i2c_get_bus_num();

	i2c_set_bus_num(peeprom_data->i2c_num);
	for (i = 0; i < count; i++)
		ret |= i2c_read(peeprom_data->addr, offset + (i << 4), 1, data + (i << 4), 16);

	if (more > 0)
		ret |= i2c_read(peeprom_data->addr, offset + (count << 4), 1,
				data + (count << 4), more);
	i2c_set_bus_num(bus);

	return ret;
}

int eeprom_get_map(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset = 0;
	unsigned int len;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		len = sizeof(struct eeprom1_map);
		break;

	case 2:
		peeprom_data->addr = CHIP_ADDR;
		len = sizeof(struct eeprom2_map);
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	return ret;
}

int eeprom_get_board_name(struct eeprom_data *peeprom_data, uchar *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM1_BOARD_NAME;
		len = 0x40;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Board Name :");
		print_ascii(data, len);
	}
	return ret;
}

static int check_board_sn(uchar *data, unsigned int len)
{
	int i, j;
	int boardsn_len = 0;
	int min_len = 6;
	int max_len = 20;
	int ret = 0;

	for (i = len - 1; i >= 0; i--) {
		if (data[i] == '_') {
			data[i] = 'N';
			break;
		}
	}

	for (i = 0; data[i] != '\0' && i < len; i++) {
		if (data[i] < '0' || (data[i] > '9' && data[i] < 'A') ||
			(data[i] > 'Z' && data[i] < 'a') || data[i] > 'z') {
			if (i == len - 1) {
				data[i] = 0;
			} else {
				for (j = i; data[j] != 0 && j < len - 1; j++)
					data[j] = data[j+1];
			}
		} else {
			boardsn_len++;
		}
	}

	if (boardsn_len < min_len) {
		*data = '\0';
		ret = -1;
	} else if (boardsn_len > max_len) {
		data[max_len] = '\0';
	}

	return ret;
}

/* Get board serial number from eeprom and check it. */
int eeprom_get_board_sn(struct eeprom_data *peeprom_data, uchar *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM1_BOARD_SN;
		len = 0x20;
		break;

	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_BOARD_SN;
		len = 0x20;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		if (!check_board_sn(data, len)) {
			printf("Board Serial Number :");
			print_ascii(data, len);
		} else {
			ret = -1;
		}
	}
	return ret;
}

int eeprom_get_board_type(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM1_BOARD_TYPE;
		len = 1;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Board Type : ");
		switch (*data) {
		case 0x0:
			printf("Brownstone\n");
			break;
		case 0x1:
			printf("Arsenal\n");
			break;
		case 0x2:
			printf("Qseven\n");
			break;
		case 0x3:
			printf("Abilene\n");
			break;
		case 0x4:
			printf("Qseven Carrier\n");
			break;
		case 0x5:
			printf("Yellowstone\n");
			break;
		case 0x6:
			printf("Salem\n");
			break;
		case 0x7:
			printf("Concord\n");
			break;
		case 0xff:
			printf("Not programmed\n");
			break;
		default:
			printf("Undefined\n");
			break;
		}
	}
	return ret;
}

int eeprom_get_board_rev(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM1_BOARD_REV;
		len = 1;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);

	if (!ret) {
		printf("Board Layout Rev: ");
		switch (*data) {
		case 0x0:
			printf("Initial Layout\n");
			break;
		case 0xff:
			printf("Undefined/Unprogrammed\n");
			break;
		default:
			printf("0x%x\n", *data);
			break;
		}
	}
	return ret;
}

int eeprom_get_board_eco(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM1_BOARD_ECO;
		len = 1;
		break;

	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_BOARD_ECO;
		len = 0x10;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);

	if (!ret) {
		switch (peeprom_data->index) {
		case 1:
			printf("Applied Platform ECO[64:1] : 0x%llx\n", (unsigned long long)*data);
			break;

		case 2:
			printf("Board ECO :");
			print_ascii(data, 0x10);
			break;
		}
	}
	return ret;
}

int eeprom_get_board_manu_y_m(struct eeprom_data *peeprom_data)
{
	int ret;
	unsigned int offset;
	unsigned int len;
	u8 month;
	u8 year;

	switch (peeprom_data->index) {
	case 1:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM1_BOARD_MONTH;
		len = 1;
		ret = get_eeprom(peeprom_data, offset, len, &month);
		offset = EEPROM1_BOARD_YEAR;
		ret |= get_eeprom(peeprom_data, offset, len, &year);
		break;

	default:
		ret = -1;
		return ret;
	}

	if (!ret)
		printf("Board Manufacture Year-Month : %u-%u\n", 2000 + year, month);

	return ret;
}

int eeprom_get_board_category(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_BOARD_CATEGORY;
		len = 0x20;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Board Category :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_chip_name(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_CHIP_NAME;
		len = 0x10;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Chip Name :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_chip_stepping(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_CHIP_STEP;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Chip Stepping :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_board_reg_date(struct eeprom_data *peeprom_data)
{
	int ret;
	unsigned int offset;
	unsigned int len;
	u8 data[16];

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_BOARD_REG_DATE;
		len = 0x10;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Board Register Date :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_board_state(struct eeprom_data *peeprom_data)
{
	int ret;
	unsigned int offset;
	unsigned int len;
	u8 data[8];

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_BOARD_STATE;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Board State :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_user_team(struct eeprom_data *peeprom_data)
{
	int ret;
	unsigned int offset;
	unsigned int len;
	u8 data[24];

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_USER_TEAM;
		len = 0x18;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("User Team :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_current_user(struct eeprom_data *peeprom_data)
{
	int ret;
	unsigned int offset;
	unsigned int len;
	u8 data[24];

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_CURRENT_USER;
		len = 0x18;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("Current User :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_lcd_resolution(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_LCD_RESOLUTION;
		len = 0x10;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("LCD Resolution :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_lcd_screen_size(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_LCD_SRC_SIZE;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("LCD Screen Size :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_ddr_type(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_DDR_TYPE;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("DDR Type :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_ddr_size_speed(struct eeprom_data *peeprom_data)
{
	int ret;
	unsigned int offset;
	unsigned int len;
	u8 data[8];

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_DDR_SIZE_SPEED;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("DDR Size & Speed :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_ddr_size(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	int i;
	unsigned int offset;
	unsigned int len;
	u8 tmp[8];

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_DDR_SIZE_SPEED;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, tmp);
	if (!ret) {
		for (i = 0; i < 8; i++) {
			if (tmp[i] == 0 || tmp[i] == 64)
				break;
			else
				data[i] = tmp[i];
		}
	}
	return ret;
}

int eeprom_get_ddr_speed(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	int i;
	int j;
	u8 tmp[8];
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_DDR_SIZE_SPEED;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, tmp);
	if (!ret) {
		for (i = 0; i < 8; i++) {
			if (tmp[i] == 0) {
				break;
			} else if (tmp[i] == 64) {
				for (j = i + 1; j < 8; j++) {
					if (tmp[j] == 0)
						break;
					else
						data[j-i-1] = tmp[j];
				}
				break;
			}
		}
	}
	return ret;
}

int eeprom_get_emmc_type(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_EMMC_TYPE;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("eMMC Type :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_emmc_size(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_EMMC_SIZE;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("eMMC Size :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_rf_name_ver(struct eeprom_data *peeprom_data)
{
	int ret;
	u8 data[8];
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_RF_NAME_VERSION;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("RF Name,Version :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_rf_type(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_RF_TYPE;
		len = 0x10;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("RF Type :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_rf_name(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_RF_NAME;
		len = 0x8;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("RF Name :");
		print_ascii(data, len);
	}
	return ret;
}

int eeprom_get_rf_info(struct eeprom_data *peeprom_data, u8 *data)
{
	int ret;
	unsigned int offset;
	unsigned int len;

	switch (peeprom_data->index) {
	case 2:
		peeprom_data->addr = CHIP_ADDR;
		offset = EEPROM2_RF_INFO;
		len = 0x10;
		break;

	default:
		ret = -1;
		return ret;
	}

	ret = get_eeprom(peeprom_data, offset, len, data);
	if (!ret) {
		printf("RF INFO :");
		print_ascii(data, len);
	}
	return ret;
}

/*
 * (C) Copyright 2013  Marvell company.
 */
#include <common.h>
#include <asm/arch/cpu.h>
#include "ddr_tests.h"

#define   MTSP_MSG_WRITING_OFFSET 13

uint32_t exit_when_error = 1;
uint32_t mtsp_total_error;
uint32_t smtd_stat;
uint32_t smtd_pat  = 0x55555555;
static uint32_t bit_byte_check;
static uint32_t byte_err_sta[4];
static uint32_t bit_err_sta[32];

#ifdef LOG_SILENT
static enum print_mode logout_mode = SILENT_MODE;
#else
static enum print_mode logout_mode = FULL_MODE;
#endif

static const struct pattern_dword pattern_list[] = {
	{0x00000000, 0xFFFFFFFF},
	{0x55AA55AA, 0xAA55AA55},
	{0x7FFF7FFF, 0x80008000},
	{0xFFFFFFFF, 0xFFFFFFFF},/*+decrement*/
	{0x55555555, 0xAAAAAAAA},
	{0x7F7F7F7F, 0x80808080},/*+rotate*/
	{0x7FFF7FFF, 0x80008000},/*+rotate*/
	{0x7FFFFFFF, 0x80000000},/*+rotate*/
	{0x01010101, 0xFEFEFEFE},/*+rotate left*/
	{0x00010001, 0xFFFEFFFE},/*+rotate left*/
	{0x00000001, 0xFFFFFFFE},/*+rotate left*/
	{0x7F7F7F7F, 0x7F7F7F7F},/*+rotate + without second pattern*/
	{0x7FFF7FFF, 0x7FFF7FFF},/*+rotate + without second pattern*/
	{0x7FFFFFFF, 0x7FFFFFFF},/*+rotate + without second pattern*/
	{0x01010101, 0x01010101},/*+rotate left*/
	{0x00010001, 0x00010001},/*+rotate left*/
	{0x00000001, 0x00000001},/*+rotate left*/
	{0x01010101, 0xFFFFFFFF},/*+rotate left*/
	{0x00010001, 0xFFFFFFFF},/*+rotate left*/
	{0x00000001, 0xFFFFFFFF},/*+rotate left*/
	{0xFEFEFEFE, 0x00000000},/*+rotate left*/
	{0xFFFEFFFE, 0x00000000},/*+rotate left*/
	{0xFFFFFFFE, 0x00000000},/*+rotate left*/
	{0x00000000, 0x00000001},/*+32 bytes+rotate left*/
	{0x00000001, 0x00000001},/*+32 bytes+rotate left*/
	{0x00000000, 0x00000001},/*+32 bytes+rotate left*/
	{0x00000000, 0x00000001},/*+32 bytes+rotate left*/
	{0x00000000, 0xFFFFFFFF},/*FREQ*/
	{0x00000000, 0x00000000},/*increment*/
	{0x00000001, 0x00000000},/*special test*/
	{0x00000001, 0xFFFFFFFF},/*special test*/
	{0x00000001, 0x00000000},/*special test*/
};

#ifndef LOG_SILENT
const char mtsp_msg_read[7] = {"Reading"};
const char mtsp_msg_write[7] = {"Writing"};
char mtsp_test_msg[32][85] = {
	{"0x00000001 - Writing Analog data pattern - Start\n"},
	{"0x00000002 - Writing Constant data pattern - Start\n"},
	{"0x00000004 - Writing Complement data pattern - Start\n"},
	{"0x00000008 - Writing Decrement data pattern - Start\n"},
	{"0x00000010 - Writing Miller Effect data pattern - Start\n"},
	{"0x00000020 - Writing Zero Rotate Right & Invert Byte data "
		"pattern - Start\n"},
	{"0x00000040 - Writing Zero Rotate Right & Invert Word data "
		"pattern - Start\n"},
	{"0x00000080 - Writing Zero Rotate Right & Invert DoubleWord data "
		"pattern - Start\n"},
	{"0x00000100 - Writing One Rotate left & Invert Byte data "
		"pattern - Start\n"},
	{"0x00000200 - Writing One Rotate left & Invert Word data "
		"pattern - Start\n"},
	{"0x00000400 - Writing One Rotate left & Invert DoubleWord data "
		"pattern - Start\n"},
	{"0x00000800 - Writing Zero Rotate Right Byte data "
		"pattern - Start\n"},
	{"0x00001000 - Writing Zero Rotate Right Word data pattern - Start\n"},
	{"0x00002000 - Writing Zero Rotate Right DoubleWord data "
		"pattern - Start\n"},
	{"0x00004000 - Writing One Rotate left Byte data pattern - Start\n"},
	{"0x00008000 - Writing One Rotate left Word data pattern - Start\n"},
	{"0x00010000 - Writing One Rotate left DoubleWord data "
		"pattern - Start\n"},
	{"0x00020000 - Writing Walking 1 Byte data pattern - Start\n"},
	{"0x00040000 - Writing Walking 1 Word data pattern - Start\n"},
	{"0x00080000 - Writing Walking 1 DoubleWord data pattern - Start\n"},
	{"0x00100000 - Writing Walking Zero Byte data pattern - Start\n"},
	{"0x00200000 - Writing Walking Zero Word data pattern - Start\n"},
	{"0x00400000 - Writing Walking Zero DoubleWord data "
		"pattern - Start\n"},
	{"0x00800000 - Writing Single Signal Tailgating 0101_0000 data "
		"pattern - Start\n"},
	{"0x01000000 - Writing Single Signal Tailgating 0101_1111 data "
		"pattern - Start\n"},
	{"0x02000000 - Writing Single Signal Tailgating 0101_1010 data "
		"pattern - Start\n"},
	{"0x04000000 - Writing Multiple Signal Tailgating data "
		"pattern - Start\n"},
	{"0x08000000 - Writing Frequency Sweep data pattern - Start\n"},
	{"0x10000000 - Writing Vertical increment data pattern - Start\n"},
	{"0x20000000 - Writing Horzontal increment data pattern - Start\n"},
	{"0x40000000 - Writing fixed 2nd pattern 0xFFFFFFFF - Start\n"},
	{"0x80000000 - Writing fixed 2nd pattern 0x00000000 - Start\n"},
};
#endif

/**
* print_error - This function prints an error message to the user
* @addr - where data failed
* @access - BYTE,HWORD or WORD
* @data - the current read
* @expected - expected data
*/
void print_error(uint32_t addr, enum access_type access,
		uint32_t data, uint32_t expected)
{
	int bit, byte;
	if (bit_byte_check) {
		for (bit = 0; bit < 32; bit++) {
			if ((data & (1 << bit)) != (expected & (1 << bit)))
				bit_err_sta[bit % 32]++;
		}
		for (byte = 0; byte < 4; byte++) {
			if ((data & (0xFF << (byte * 8))) !=
					(expected & (0xFF << (byte * 8))))
				byte_err_sta[byte % 4]++;
		}
	}
	if (logout_mode == FULL_MODE) {
		printf("\nERROR: Expecting ");
		switch (access) {
		case ACCESS_TYPE_BYTE:
			printb((uint8_t)expected & 0xFF);
			break;
		case ACCESS_TYPE_HWORD:
			printh((uint16_t)expected & 0xFFFF);
			break;
		case ACCESS_TYPE_WORD:
			printw(expected);
			break;
		default:
			break;
		}
		printf(" at address ");
		printw(addr);
		printf(". Actual = ");
		switch (access) {
		case ACCESS_TYPE_BYTE:
			printb((uint8_t)data & 0xFF);
			break;
		case ACCESS_TYPE_HWORD:
			printh((uint16_t)data & 0xFFFF);
			break;
		case ACCESS_TYPE_WORD:
			printw(data);
			break;
		default:
			break;
		}
		printf("\n");
	}
}

/**
* memory_test_type1 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type1(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern, uint32_t pattern)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;

	while (j < size) {
		/*Write the data*/
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = (uint32_t)(second_pattern);
		j += 8;
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(second_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j + 4, ACCESS_TYPE_WORD,
					temp[i-1], second_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		j += 8;
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type2 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* @increment - if its increment or decrement
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type2(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t pattern, uint8_t increment)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	int inc;

	if (increment)
		inc = 1;
	else
		inc = -1;
	while (j < size) {
		/*Write the data*/
		temp[i++] = (uint32_t)(first_pattern);
		j += 4;
		first_pattern = first_pattern + inc;
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	/*Perform activities between write to read*/
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		j += 4;
		first_pattern = first_pattern + inc;
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type3 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* @left - it the rotation is left or right
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type3(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern,
		uint32_t pattern, uint8_t left)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;
	uint32_t rotate_number;

	if (left)
		rotate_number = 31;
	else
		rotate_number = 1;
	while (j < size) {
		/*Write the data*/
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = (uint32_t)(second_pattern);
		j += 8;
		rotate(first_pattern, rotate_number);
		rotate(second_pattern, rotate_number);
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(second_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j + 4, ACCESS_TYPE_WORD,
					temp[i-1], second_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		j += 8;
		rotate(first_pattern, rotate_number);
		rotate(second_pattern, rotate_number);
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type4 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* @left - it the rotation is left or right
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type4(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t pattern, uint8_t left)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t rotate_number;
	if (left)
		rotate_number = 31;
	else
		rotate_number = 1;
	while (j < size) {
		/*Write the data*/
		temp[i++] = (uint32_t)(first_pattern);
		j += 4;
		rotate(first_pattern, rotate_number);
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		j += 4;
		rotate(first_pattern, rotate_number);
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type5 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type5(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern, uint32_t pattern)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;

	while (j < size) {
		/*Write the data*/
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = (uint32_t)(second_pattern);
		j += 8;
		rotate(first_pattern, 31);
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(second_pattern)) {
			if (!error)
				printf("\n Read Fail\n");
			print_error((uintptr_t)temp + j + 4, ACCESS_TYPE_WORD,
					temp[i-1], second_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		j += 8;
		rotate(first_pattern, 31);
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type6 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type6(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern, uint32_t pattern)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t temp_val;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;

	while (j < size) {
		/*Write the data*/
		temp[i++] = 0x0;
		temp_val = (uint32_t)(first_pattern | second_pattern);
		temp[i++] = temp_val;
		temp[i++] = 0x0;
		temp[i++] = temp_val;
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = (uint32_t)(first_pattern);
		j += 32;
		rotate(first_pattern, 31);
		rotate(second_pattern, 31);
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != 0x0) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], 0x0);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		temp_val = (uint32_t)(first_pattern | second_pattern);
		if (temp[i++] != temp_val) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 4, ACCESS_TYPE_WORD,
					temp[i-1], temp_val);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != 0x0) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 8, ACCESS_TYPE_WORD,
					temp[i-1], 0x0);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != temp_val) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 12, ACCESS_TYPE_WORD,
					temp[i-1], temp_val);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 16, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 20, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 24, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 28, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		j += 32;
		rotate(first_pattern, 31);
		rotate(second_pattern, 31);
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type7 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type7(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern, uint32_t pattern)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t temp_val;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;

	while (j < size) {
		/*Write the data*/
		temp[i++] = 0x0;
		temp_val = (uint32_t)(first_pattern | second_pattern);
		temp[i++] = temp_val;
		temp[i++] = 0x0;
		temp[i++] = temp_val;
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = 0x0;
		temp[i++] = (uint32_t)(first_pattern);
		temp[i++] = 0x0;
		j += 32;
		rotate(first_pattern, 31);
		rotate(second_pattern, 31);
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	while (j < size) {
		/*Read the data and compare it to the pattern*/
		if (temp[i++] != 0x0) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j, ACCESS_TYPE_WORD,
					temp[i-1], 0x0);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		temp_val = (uint32_t)(first_pattern | second_pattern);
		if (temp[i++] != temp_val) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 4, ACCESS_TYPE_WORD,
					temp[i-1], temp_val);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != 0x0) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 8, ACCESS_TYPE_WORD,
					temp[i-1], 0x0);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != temp_val) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 12, ACCESS_TYPE_WORD,
					temp[i-1], temp_val);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}
		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 16, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}

		if (temp[i++] != 0x0) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 20, ACCESS_TYPE_WORD,
					temp[i-1], 0x0);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}

		if (temp[i++] != (uint32_t)(first_pattern)) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 24, ACCESS_TYPE_WORD,
					temp[i-1], first_pattern);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}

		if (temp[i++] != 0x0) {
			if (!error)
				printf("\n Read Fail");
			print_error((uintptr_t)temp + j + 28, ACCESS_TYPE_WORD,
					temp[i-1], 0x0);
			mtsp_total_error++;
			error++;
			if (exit_when_error)
				return MTSP_READ_FAIL;
		}

		j += 32;

		rotate(first_pattern, 31);
		rotate(second_pattern, 31);
	}

	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type8 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type8(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern, uint32_t pattern)
{
	uint32_t i = 0, j = 0, error = 0;
	int loop = 1, freq;
	int end_test = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;

	/*Infinite loop - if J=>size break the loop*/
	for (;;) {
		/*Write the data*/
		freq = loop;
		while (freq) {
			temp[i++] = (uint32_t)(first_pattern);
			j += 4;
			if (j >= size) {
				end_test = 1;
				freq = 1;
				break;
			}
			freq--;
		}
		if (end_test)
			break;
		freq = loop;
		while (freq) {
			temp[i++] = (uint32_t)(second_pattern);
			j += 4;
			if (j >= size) {
				end_test = 1;
				freq = 1;
				break;
			}
			freq--;
		}
		if (end_test)
			break;
		loop++;
		if (loop == 0x100)
			loop = 1;
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	loop = 1;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	/*Infinite loop - if J=>size break the loop*/
	for (;;) {
		/*Write the data*/
		freq = loop;
		while (freq) {
			if (temp[i++] != (uint32_t)(first_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j,
						ACCESS_TYPE_WORD, temp[i-1],
						first_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			j += 4;
			if (j >= size) {
				end_test = 1;
				freq = 1;
				break;
			}
			freq--;
		}
		if (end_test)
			break;
		freq = loop;
		while (freq) {
			if (temp[i++] != (uint32_t)(second_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j,
						ACCESS_TYPE_WORD, temp[i-1],
						second_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			j += 4;
			if (j >= size) {
				end_test = 1;
				freq = 1;
				break;
			}
			freq--;
		}
		if (end_test)
			break;
		loop++;
		if (loop == 0x100)
			loop = 1;
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test_type9 - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @first_pattern - pattern to test the memory
* @second_pattern - pattern to test the memory
* @pattern - kind of pattern to test the memory
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test_type9(uintptr_t addr, uint32_t size,
		uint32_t first_pattern,
		uint32_t second_pattern, uint32_t pattern)
{
	uint32_t i = 0, j = 0, error = 0;
	uint32_t *temp = (uint32_t *)addr;
	uint32_t first_pattern_restore = first_pattern;
	uint32_t second_pattern_restore = second_pattern;
	uint32_t addr_restore = addr;

	while (j < size) {
		/*Write the data*/
		if (addr & 0x20) {
			temp[i] = (uint32_t)(first_pattern);
			temp[i + 1] = (uint32_t)(second_pattern);
		} else {
			temp[i] = 0x0;
			temp[i + 1] = 0x0;
		}
		if (addr & 0x40) {
			temp[i + 2] = (uint32_t)(first_pattern);
			temp[i + 3] = (uint32_t)(second_pattern);
		} else {
			temp[i + 2] = 0x0;
			temp[i + 3] = 0x0;
		}
		if (addr&0x80) {
			temp[i + 4] = (uint32_t)(first_pattern);
			temp[i + 5] = (uint32_t)(second_pattern);
		} else {
			temp[i + 4] = 0x0;
			temp[i + 5] = 0x0;
		}
		if (addr&0x100) {
			temp[i + 6] = (uint32_t)(first_pattern);
			temp[i + 7] = (uint32_t)(second_pattern);
		} else {
			temp[i + 6] = 0x0;
			temp[i + 7] = 0x0;
		}
		i += 8;
		j += 32;
		addr += 32;
		if (!(addr & 0x1E0)) {
			if (((uint32_t)(first_pattern)) == 0x80000000) {
				second_pattern = 0x1;
				first_pattern = 0x0;
			} else if (((uint32_t)(second_pattern)) == 0x80000000) {
				second_pattern = 0x0;
				first_pattern = 0x1;
			} else {
				second_pattern = second_pattern << 1;
				first_pattern = first_pattern << 1;
			}
		}
	}
	printf("write pass\n");
#ifndef LOG_SILENT
	/*Inform user that test has continue - read sequence*/
	for (i = 0; i < 7; i++) {
		mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i] =
			mtsp_msg_read[i];
	}
	if (logout_mode == FULL_MODE)
		printf("%s", (char *)mtsp_test_msg[pattern]);
#endif
	j = 0;
	i = 0;
	/*Restore patterns*/
	first_pattern = first_pattern_restore;
	second_pattern = second_pattern_restore;
	addr = addr_restore;
	while (j < size) {
		/*Write the data*/
		if (addr & 0x20) {
			if (temp[i] != (uint32_t)(first_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j,
						ACCESS_TYPE_WORD,
						temp[i], first_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 1] != (uint32_t)(second_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 4,
						ACCESS_TYPE_WORD,
						temp[i + 1], second_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		} else {
			if (temp[i] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j,
						ACCESS_TYPE_WORD,
						temp[i], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 1] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 4,
						ACCESS_TYPE_WORD,
						temp[i + 1], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		}
		if (addr&0x40) {
			if (temp[i + 2] != (uint32_t)(first_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 8,
						ACCESS_TYPE_WORD,
						temp[i + 2], first_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 3] != (uint32_t)(second_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 12,
						ACCESS_TYPE_WORD,
						temp[i + 3], second_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		} else {
			if (temp[i + 2] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 8,
						ACCESS_TYPE_WORD,
						temp[i + 2], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 3] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 12,
						ACCESS_TYPE_WORD,
						temp[i + 3], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		}
		if (addr&0x80) {
			if (temp[i + 4] != (uint32_t)(first_pattern)) {
				printf("\n Read Fail");
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 16,
						ACCESS_TYPE_WORD,
						temp[i + 4], first_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 5] != (uint32_t)(second_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 20,
						ACCESS_TYPE_WORD,
						temp[i + 5], second_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		} else {
			if (temp[i + 4] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 16,
						ACCESS_TYPE_WORD,
						temp[i + 4], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 5] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 20,
						ACCESS_TYPE_WORD,
						temp[i + 5], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		}
		if (addr&0x100) {
			if (temp[i + 6] != (uint32_t)(first_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 24,
						ACCESS_TYPE_WORD,
						temp[i + 6], first_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 7] != (uint32_t)(second_pattern)) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 28,
						ACCESS_TYPE_WORD,
						temp[i + 7], second_pattern);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		} else {
			if (temp[i + 6] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 24,
						ACCESS_TYPE_WORD,
						temp[i + 6], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
			if (temp[i + 7] != 0x0) {
				if (!error)
					printf("\n Read Fail");
				print_error((uintptr_t)temp + j + 28,
						ACCESS_TYPE_WORD,
						temp[i + 7], 0x0);
				mtsp_total_error++;
				error++;
				if (exit_when_error)
					return MTSP_READ_FAIL;
			}
		}
		i += 8;
		j += 32;
		addr += 32;
		if (!(addr & 0x1E0)) {
			if (((uint32_t)(first_pattern)) == 0x80000000) {
				second_pattern = 0x1;
				first_pattern = 0x0;
			} else if (((uint32_t)(second_pattern)) == 0x80000000) {
				second_pattern = 0x0;
				first_pattern = 0x1;
			} else {
				second_pattern = second_pattern << 1;
				first_pattern = first_pattern << 1;
			}
		}
	}
	if (error == 0)
		printf("read pass");
	return MTSP_OPERATION_OK;
}

/**
* memory_test - This function tests the wanted memory
* according to parameters
* @addr - start address for memory to be tested
* @size - size of memory to be tested
* @pattern - kind of pattern to test the memory
* @access - BYTE,WORD or DWORD
* @mtsp - to define if the test is MTSP type
* if test pass, returen 0;if test failed - see error value
*/
uint32_t memory_test(uintptr_t addr, uint32_t size,
		enum pattern_number pattern, uint8_t access, uint8_t mtsp)
{
	uint32_t i = 0, j = 0;
	uint8_t  mtsp_rc = MTSP_OPERATION_OK;
	uint32_t first_pattern = 0, second_pattern = 0;
	uint8_t  *t_addr8  = (uint8_t *)addr;
	uint16_t *t_addr16 = (uint16_t *)addr;
	uint32_t *temp = (uint32_t *)addr;
#ifndef LOG_SILENT
	char SUCCESS_SIGN = '.';
#endif
	/*Get the desired pattern but only for WORD patterns*/
	if (mtsp) {
		if (smtd_stat&0x1) {
			first_pattern  = smtd_pat;
			second_pattern = ~smtd_pat;
		} else {
			first_pattern = pattern_list[pattern].first_pattern;
			second_pattern = pattern_list[pattern].second_pattern;
		}
#ifndef LOG_SILENT
		if (logout_mode == FULL_MODE) {
			/*Inform user that test has begun - write sequence*/
			printf("%s", (char *)mtsp_test_msg[pattern]);
		} else if (logout_mode == SIGN_MODE)
			putc(SUCCESS_SIGN);
#endif
		/*Call test functions*/
		switch (pattern) {
		case PATTERN_NUMBER_ONE:
		case PATTERN_NUMBER_TWO:
		case PATTERN_NUMBER_THREE:
		case PATTERN_NUMBER_FIVE:
			mtsp_rc = memory_test_type1(addr, size, first_pattern,
					second_pattern, pattern);
			break;
		case PATTERN_NUMBER_FOUR:
			mtsp_rc = memory_test_type2(addr, size, first_pattern,
					pattern, 0);
			break;
		case PATTERN_NUMBER_SIX:
		case PATTERN_NUMBER_SEVEN:
		case PATTERN_NUMBER_EIGHT:
			mtsp_rc = memory_test_type3(addr, size, first_pattern,
					second_pattern, pattern, 0);
			break;
		case PATTERN_NUMBER_NINE:
		case PATTERN_NUMBER_TEN:
		case PATTERN_NUMBER_ELEVEN:
			mtsp_rc = memory_test_type3(addr, size, first_pattern,
					second_pattern, pattern, 1);
			break;
		case PATTERN_NUMBER_TWELVE:
		case PATTERN_NUMBER_THIRTEEN:
		case PATTERN_NUMBER_FOURTEEN:
			mtsp_rc = memory_test_type4(addr, size, first_pattern,
					pattern, 0);
			break;
		case PATTERN_NUMBER_FIFTEEN:
		case PATTERN_NUMBER_SIXTEEN:
		case PATTERN_NUMBER_SEVENTEEN:
			mtsp_rc = memory_test_type4(addr, size,
					first_pattern, pattern, 1);
			break;
		case PATTERN_NUMBER_EIGHTEEN:
		case PATTERN_NUMBER_NINETEEN:
		case PATTERN_NUMBER_TWENTY:
		case PATTERN_NUMBER_TWENTYONE:
		case PATTERN_NUMBER_TWENTYTWO:
		case PATTERN_NUMBER_TWENTYTHREE:
			mtsp_rc = memory_test_type5(addr, size, first_pattern,
					second_pattern, pattern);
			break;
		case PATTERN_NUMBER_TWENTYFOUR:
		case PATTERN_NUMBER_TWENTYFIVE:
		case PATTERN_NUMBER_TWENTYSEVEN:
			mtsp_rc = memory_test_type6(addr, size, first_pattern,
					second_pattern, pattern);
			break;
		case PATTERN_NUMBER_TWENTYSIX:
			mtsp_rc = memory_test_type7(addr, size, first_pattern,
					second_pattern, pattern);
			break;
		case PATTERN_NUMBER_TWENTYEIGHT:
			mtsp_rc = memory_test_type8(addr, size, first_pattern,
					second_pattern, pattern);
			break;
		case PATTERN_NUMBER_TWENTYNINE:
			mtsp_rc = memory_test_type2(addr, size, first_pattern,
					pattern, 1);
			break;
		case PATTERN_NUMBER_THIRTY:
			mtsp_rc = memory_test_type9(addr, size, first_pattern,
					second_pattern, pattern);
			break;
		case PATTERN_NUMBER_THIRTYONE:
			mtsp_rc = memory_test_type1(addr, size, first_pattern,
					0xFFFFFFFF, pattern);
			break;
		case PATTERN_NUMBER_THIRTYTWO:
			mtsp_rc = memory_test_type1(addr, size, first_pattern,
					0x00000000, pattern);
			break;
		default:
			break;
		}
#ifndef LOG_SILENT
		if (logout_mode == FULL_MODE)
			printf("\n");
		/*Change back the string to write*/
		for (i = 0; i < 7; i++) {
			mtsp_test_msg[pattern][MTSP_MSG_WRITING_OFFSET + i]
				= mtsp_msg_write[i];
		}
#endif
	} else {
#ifndef LOG_SILENT
		/*Inform user for regular memory test on the desired memory*/
		if (logout_mode == FULL_MODE)
			printf("Regular memory test begins.\n");
#endif
		while (j < size) {
			switch (access) {
			case ACCESS_TYPE_BYTE:
				t_addr8[i] = (uint8_t)(pattern&0xFF);
				if (t_addr8[i++] != (uint8_t)(pattern&0xFF)) {
					print_error((uintptr_t)t_addr8+j, access,
							t_addr8[i-1], pattern);
					mtsp_total_error++;
					if (exit_when_error)
						return MTSP_READ_FAIL;
				}
				j += 1;
				break;
			case ACCESS_TYPE_HWORD:
				t_addr16[i] = (uint16_t)(pattern&0xFFFF);

				if (t_addr16[i++] !=
					(uint16_t)(pattern&0xFFFF)) {
					print_error((uintptr_t)t_addr16+j,
							access, t_addr16[i-1],
							pattern);
					mtsp_total_error++;
					if (exit_when_error)
						return MTSP_READ_FAIL;
				}
				j += 2;
				break;
			case ACCESS_TYPE_WORD:
				temp[i] = (uint32_t)(pattern);
				if (temp[i++] != (uint32_t)(pattern)) {
					print_error((uintptr_t)temp + j, access,
							temp[i-1], pattern);
					mtsp_total_error++;
					if (exit_when_error)
						return MTSP_READ_FAIL;
				}
				j += 4;
				break;
			default:
				return MTSP_WRONG_PARAMETER;
		}
		}
		mtsp_rc = MTSP_OPERATION_OK;
	}
	return mtsp_rc;
}

uint32_t mtsp_test(uintptr_t addr,  uint32_t size, uint32_t pattern,
		uint32_t pre_read)
{
	uint32_t cur_pattern = 0;
	uint32_t index = 0x1;
	int i;
	uint8_t  mtsp_rc = MTSP_OPERATION_OK;
	uint32_t tmsec = 0;

	mtsp_total_error = 0;
	if (bit_byte_check) {
		for (i = 0; i < sizeof(byte_err_sta)/sizeof(uint32_t); i++)
			byte_err_sta[i] = 0;
		for (i = 0; i < sizeof(bit_err_sta)/sizeof(uint32_t); i++)
			bit_err_sta[i] = 0;
	}
	if (logout_mode == FULL_MODE) {
		/*Need to add many prints to the user - OR*/
		printf("MTSP test starts from:");
		printw((uint32_t)addr & 0xFFFFFFFF);
		printf(" to :");
		printw(((uint32_t)addr & 0xFFFFFFFF) + size);
		printf("\n");
		tmsec = get_timer(0);
	}
	for (i = 0; i < 32; i++) {
		cur_pattern = pattern & (index << i);
		if (cur_pattern) {
			mtsp_rc = memory_test(addr, size, i,
					ACCESS_TYPE_WORD, 1);
			if (mtsp_rc)
				break;
		}

	}
	if (logout_mode == FULL_MODE) {
		tmsec = get_timer(0) - tmsec;
		printf("Elapsed time: %d msec\n", tmsec);
		if (mtsp_total_error)
			printf("mtsp mem test finished and %d errors occur\n",
					mtsp_total_error);
		else
			printf("mtsp memory test finished successfully.\n");
	} else if (logout_mode == SIGN_MODE)
		printf("\n");

	if (logout_mode >= MINIMAL_MODE && logout_mode < FULL_MODE &&
			mtsp_total_error)
		printf("mstp-->  Test failed\n");
	if (mtsp_total_error) {
		if (bit_byte_check) {
			printf("*** MTSP BYTE & BIT error statistics ***\n");
			for (i = 0; i < sizeof(byte_err_sta)/sizeof(uint32_t);
					i++)
				printf("BYTE %d error: %d\n",
						i, byte_err_sta[i]);
			printf("----------\n");
			for (i = 0; i < sizeof(bit_err_sta)/sizeof(uint32_t);
					i++)
				printf("BIT %d error: %d\n",
						i, bit_err_sta[i]);
		}
		return MTSP_READ_FAIL;
	}
	return mtsp_rc;
}

int mtsp(int argc, char * const *argv)
{
	uint32_t  mem_test_size, patterns_to_activate;
	uint32_t  startAddr, test_times, i;

	logout_mode = FULL_MODE;
	test_times = 1;
	switch (argc) {
	case 1:
		printf("mtsp--> Enter the start addr to be tested\n");
		break;
	case 2:
		printf("mtsp--> Enter number of byte to be tested\n");
		break;
	case 3:
		printf("mtsp--> Set bits of the test to be run\n");
		break;
	case 5:
		test_times = conv_dec((char *)argv[4]);
	case 4:
		mem_test_size = conv_hex((char *)argv[2]);
		startAddr = conv_hex((char *)argv[1]);
		patterns_to_activate = conv_hex((char *)argv[3]);
		for (i = 0; i < test_times; i++) {
			printf("\n**** MTSP TEST LOOP: %d *****\n", i + 1);
			if (mtsp_test(startAddr, mem_test_size,
						patterns_to_activate, 0))
				return -1;
		}
		break;
	default:
		break;
	}
	return 0;
}

void bbu_smtd_help(void)
{
	printf("smtd - Set Memory Test Defaults.\n");
	printf("The SMTD command is used to set the memory test\n");
	printf("Usage:smtd [<pattern>]\n");
	printf("pattern  = The first pattern for mtsp test\n");
	printf("Note: 1. Use 'smtd ena' to enable/disable default pattern\n");
	printf("2. Use 'smtd mode' to switch the mtsp test mode:\n");
	printf("   continue or terminate the test when error occurs\n");
	printf("3. Use 'smtd check' to enable/disable byte&bit check\n");
	printf("If no argument entered, will display current defaults\n");
}

int smtd(int argc, char * const *argv)
{
	switch (argc) {
	case 1:
		if (smtd_stat == 0)
			printf("\nsmtd--> The default pattern is enabled\n");
		else
			printf("\nsmtd--> The default pattern is disabled\n");
		if (exit_when_error == 0)
			printf("    --> continue test when error happens\n");
		else
			printf("    --> exit test when error happens\n");
		if (bit_byte_check == 0)
			printf("    --> Byte and bit check is disabled\n");
		else
			printf("    --> Byte and bit check is enabled\n");
		break;
	case 2:
		if (!strcmp((const char *)argv[1], "ena")) {
			if (smtd_stat == 0) {
				smtd_stat = 1;
				printf("smtd--> default pattern is disabled\n");
			} else {
				smtd_stat = 0;
				printf("smtd--> default pattern is enabled\n");
			}
		} else if (!strcmp((const char *)argv[1], "mode")) {
			if (exit_when_error) {
				exit_when_error = 0;
				printf("smtd--> error occurs, test continue\n");
			} else {
				exit_when_error = 1;
				printf("smtd--> error occurs, test exit\n");
			}
		} else if (!strcmp((const char *)argv[1], "check")) {
			if (bit_byte_check) {
				bit_byte_check = 0;
				printf("smtd--> byte&bit check is disabled\n");
			} else {
				bit_byte_check = 1;
				printf("smtd--> byte&bit check is enabled\n");
			}
		} else
			smtd_pat = conv_hex((char *)argv[1]);
		break;
	default:
		bbu_smtd_help();
		break;
	}
	return 1;
}

int do_mtsp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return mtsp(argc, argv);
}

U_BOOT_CMD(
		mtsp, 5, 1, do_mtsp,
		"Launch mtsp mem test",
		"Usage:\nmtsp [addr] [size] [pattern] [<times>]"
		);

int do_smtd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return smtd(argc, argv);
}

U_BOOT_CMD(
		smtd, 5, 1, do_smtd,
		"Configure the memory test args",
		"Usage:\nsmtd [<pattern>]"
		);

#ifdef CONFIG_SYS_MCK5_DDR
void mdcp_help(void)
{
	printf("mdcp - memory copy test with configurable delay.\n");
	printf("The mdcp command is used to launch memcpy test repeatedly\n");
	printf("Usage:mdcp [addr] [size] [loop] [idle] [freq]\n");
	printf("copy <size>KB data at <addr> for <loop> times;");
	printf("then idle for <idle>ms at cpu <freq>Mhz\n");
}

int do_mdcp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong start, size, memsize = 0, banksize = 0;
	uint32_t loop, idle, cpufreq;
	uint32_t i = 0;
	uint32_t total, data, busy;
	uint32_t dump = 0, showup = 0;

	if (argc != 6) {
		mdcp_help();
		return 0;
	}

	start = conv_hex((char *)argv[1]);
	size = conv_dec((char *)argv[2]);
	loop = conv_dec((char *)argv[3]);
	idle = conv_dec((char *)argv[4]);
	cpufreq = conv_dec((char *)argv[5]);

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		MCK5_sdram_size(i, &banksize);
		memsize += banksize;
	}

	if ((start + size * 2048) > memsize) {
		printf("memcpy size exceeds the total memory size: 0x%lx\n",
			memsize);
		return 0;
	}

	printf("memcpy %lu KB at 0x%lx for %d loop,", size, start, loop);
	if (idle > 0)
		printf("then idle for %d ms ", idle);
	else
		printf("without idle ");
	printf("at cpu %dMhz\n", cpufreq);

	while (1) {
		dump++;

		if ((dump % 300) == 0) {
			/* init DDR performance counter */
			MCK5_perf_cnt_clear(0x7);
			MCK5_perf_cnt_init();
		}

		/* do memcpy */
		for (i = 0; i < size * 1024; i = i + 4)
			*(u32 *)(start + size * 1024 + i)
				= *(u32 *)(start + i);

		/* sleep */
		if (idle > 0)
			loop_delay(idle * 1000 * cpufreq);

		if ((dump % 300) == 0) {
			/* calculate data_ratio/busy_ratio */
			MCK5_perf_cnt_stop();
			MCK5_perf_cnt_get(&total, &busy, &data);
			data = data * 4;

			if (showup < loop) {
				showup++;
				printf("%d.%d\t %d.%d\n", busy*100/total,
					((busy*100)%total)*100/total,
					data*100/total,
					((data*100)%total)*100/total);
			} else {
				printf("end\n");
				break;
			}
		}
	}

	return 1;
}

U_BOOT_CMD(
	mdcp,       6,      1,      do_mdcp,
	"do memcpy circularly and monitor ddr busy_ratio/data_ratio",
	"USAGE:\nmdcp [addr] [size] [loop] [idle] [freq]"
	);
#endif

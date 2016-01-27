/*
 * FILENAME:	mrd_flag.h
 * PURPOSE:	define mrd flag structure
 *
 * (C) Copyright 2015
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Jian Zhang <jianzh@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MRD_FLAG_H__
#define __MRD_FLAG_H__

#define VALID_FLAG_HEADER 0x50524F44

#define MRD_FLAG_MEP2_OFFSET (512*1024)

#define MEP2_PART_NAME "MEP2"

#define DIAG_OVER_USB_TAG '1'
#define DIAG_OVER_UART_TAG '2'

struct MRD_FLAG_HEADER {
	unsigned int magic;        /* 0x50524F44 PROD */
	unsigned int checksum;     /* adler32 checksum, does not include magic and checksum */
	unsigned int version;      /* version */
	unsigned int length;       /* length */
};

struct MRD_FLAG_ENTRY {
	unsigned int tag;          /* tag */
	unsigned int length;       /* length */
	unsigned char value[0];     /* value, variable-length and type, aligned to 4 bytes */
};

enum MRD_TAG_TYPES {
	TAG_POWER_MODE = 0x5057524D, /*  PWRM power mode */
	TAG_EOF = 0x454F4600, /* EOF */
};

enum POWER_MODE_TYPES {
	POWER_MODE_NORMAL,
	POWER_MODE_DIAG_OVER_USB,
	POWER_MODE_DIAG_OVER_UART,
};

enum MFLAG_ERR_CODES {
	MF_NO_ERROR,
	MF_ERROR,
	MF_FILE_NOT_FOUND_ERROR,
	MF_FILE_ALREADY_EXISTS_ERROR,
	MF_FILE_WRITE_ERROR,
	MF_FILE_READ_ERROR,
	MF_NOT_INITIALIZED_ERROR,
	MF_NO_VALID_STAMP_ERROR,
	MF_INVALID_HEADER_ERROR,
	MF_INVALID_FILE_HEADER_ERROR,
	MF_INVALID_CHECKSUM_ERROR,
	MF_NO_MEMORY_ERROR,
	MF_ALREADY_INITIALIZED_ERROR,
	MF_FILENAME_TOO_LONG,
	MF_NO_SPACE_ERROR,
	MF_INVALID_END_OF_FILE_ERROR,
	MF_FLASH_TYPE_ERROR,
	MF_VERIFY_READ_ERROR,
	MF_VERIFY_CHECKSUM_ERROR,
	MF_DEVICE_OPEN_ERROR,
	MF_DEVICE_SEEK_ERROR,
	MF_DEVICE_WRITE_ERROR,
	MF_DEVICE_READ_ERROR,
	MF_INCORRECT_PARTITION_ERROR,
};
#endif

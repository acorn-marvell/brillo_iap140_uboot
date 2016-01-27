/*
 * FILENAME:	obm2osl.h
 * PURPOSE:	all the parameters passed to uboot by OBM
 *
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaofan Tian <tianxf@marvell.com>
 * Written-by: Jane Li <jiel@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __OBM2OSL_H__
#define __OBM2OSL_H__

#define OBM2OSL_VERSION			0x04
#define OBM2OSL_IDENTIFIER		0x434F4D4D	/* "COMM" */

enum DTIM_TIM_VERSION {
	DTIM_TIM_VER_3_4_0,
	DTIM_TIM_VER_3_5_0,
};

struct DTIM_INFO {
	unsigned int valid_flag;
	unsigned int validation_status;
	unsigned int loading_status;
	unsigned int tim_version; /* 0 - v3.4; 1 - v3.5 */
	unsigned int tim_ddr_address; /* the DDR address of DTIM */
};

struct OBM2OSL {
	/* a common identify for all platforms */
	unsigned int signature;
	/* for future compatibility considerations */
	unsigned int version;
	/*
	 * 0-booting in funtion mode;
	 * 1-booting in product mode with usb;
	 * 2-booting in product mode with uart;
	 */
	unsigned int booting_mode;
	/* 0-400MHZ; 1-533MHZ */
	unsigned int ddr_mode;
	struct DTIM_INFO primary;
	struct DTIM_INFO recovery;
	struct DTIM_INFO cp;
	struct DTIM_INFO mrd;
	/* 0 - primary OBM, 1 - backup OBM */
	unsigned short obm_path;
};

enum DTIM_VALID_FLAG {
	DTIM_INVALID = 0,
	DTIM_VALID	 = 1
};

enum OBM_PATH {
	PRIMARY_OBM = 0,
	BACKUP_OBM  = 1
};

enum BOOTING_MODE {
	FUNCTION_MODE = 0,
	PRODUCT_USB_MODE,
	PRODUCT_UART_MODE,
	RECOVERY_MODE,
	MENU_MODE,
	UNKNOWN_MODE
};

enum LOADING_STATUS {
	LOADING_FAIL = 0,
	LOADING_PASS = 1,
	NOT_LOADING = 2
};

enum VALIDATION_STATUS {
	VALIDATION_FAIL = 0,
	VALIDATION_PASS = 1,
	NOT_VALIDATION = 2
};

enum DDR_MODE {
	DDR_400MHZ_MODE = 0,
	DDR_533MHZ_MODE
};

#endif

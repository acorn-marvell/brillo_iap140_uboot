/*
 * fb_flashing.h - header file for fastboot flashing lock/unlock cmd
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Ethan Xia <xiasb@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#ifndef _FB_FLASHING_H
#define _FB_FLASHING_H

typedef enum {
	FB_FLASHING_DEVICE_LOCKED,
	FB_FLASHING_DEVICE_UNLOCKED
} device_state;

typedef enum {
	FB_FLASHING_INTERNAL_ERROR = -1908,
	FB_FLASHING_DEVICE_ALREADY_LOCKED,
	FB_FLASHING_LOCK_DEVICE_FAILED,
	FB_FLASHING_LOCK_DEVICE_OK,
	FB_FLASHING_DEVICE_ALREADY_UNLOCKED,
	FB_FLASHING_UNLOCK_DEVICE_FAILED,
	FB_FLASHING_UNLOCK_DEVICE_OK
} flashing_state;

device_state get_device_state(char *device_state);
flashing_state lock_device(void);
flashing_state unlock_device(void);

#endif

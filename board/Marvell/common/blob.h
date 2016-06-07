/*
 * blob.h - header file for TLV blob read/write
 *
 * (C) Copyright 2016
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Ethan Xia <xiasb@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#ifndef _BLOB_H
#define _BLOB_H

#define BLOB_OK 0
#define BLOB_FAIL -1

typedef enum {
	BLOB_TYPE_NULL,
	BLOB_TYPE_DEVICE_STATE = 0x19088091,
	BLOB_TYPE_DEVEL_KEY,
	BLOB_TYPE_ROLLBACK_INDEX,
	/* add new type before BLOB_TYPE_END */
	BLOB_TYPE_END
} blob_type;

typedef struct _blob_data {
	blob_type type;
	unsigned int length;
	unsigned char data[0];
} blob_data;

int blob_write(unsigned int offset, blob_type type, unsigned char *buf, unsigned int len);
int blob_read(unsigned int offset, unsigned char *buf, unsigned int *len);

#endif

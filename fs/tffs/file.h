/*
 * file.h
 *
 * Include interface about file operation
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _TFFS_FILE_H
#define _TFFS_FILE_H

#include "comdef.h"
#include "pubstruct.h"

#define FILE_OK						(1)
#define ERR_TFFS_INVALID_PARAM		(-1)
#define ERR_FILE_DEVICE_FAIL		(-2)
#define ERR_FILE_EOF				(-3)
#define ERR_FILE_NO_FREE_CLUSTER	(-4)

int32
file_read_sector(
	IN	tfile_t * pfile
);

int32
file_write_sector(
	IN	tfile_t * pfile
);

#endif

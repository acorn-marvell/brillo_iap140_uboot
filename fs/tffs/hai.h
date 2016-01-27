/*
 * hai.h
 *
 * Hardware Abstraction Interface definition.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _HAI_H
#define _HAI_H

#include "comdef.h"

/*
 * Public structure
 */
typedef struct _tdev_info{
	int32 free_sectors;
}tdev_info_t;

/*
 * Error code definition
 */
#define HAI_OK						(1)
#define ERR_HAI_INVALID_PARAMETER	(-1)
#define ERR_HAI_READ				(-2)
#define ERR_HAI_WRITE				(-3)
#define ERR_HAI_CLOSE				(-4)

/*
 * Interface
 */
typedef struct {} *tdev_handle_t;

tdev_handle_t
HAI_initdevice(
	IN	byte * dev,
	IN	int16 sector_size
);

/*
 * Error code might be returned:
 *  ERR_HAI_INVALID_PARAMETER
 *  ERR_HAI_READ	
 */
int32
HAI_readsector(
	IN	tdev_handle_t hdev,
	IN	int32 addr,
	OUT	ubyte * ptr
);

/*
 * Error code might be returned:
 *  ERR_HAI_INVALID_PARAMETER
 *  ERR_HAI_WRITE	
 */
int32
HAI_writesector(
	IN	tdev_handle_t hdev,
	IN	int32 addr,
	IN	ubyte * ptr
);

/* Marvell fixed */
int32
HAI_writesectors(
	IN	tdev_handle_t hdev,
	IN	int32 addr,
	IN	ubyte * ptr,
	IN	uint32 nsectors
);
/*
 * Error code might be returned:
 *  ERR_HAI_INVALID_PARAMETER
 *  ERR_HAI_CLOSE
 */
int32
HAI_closedevice(
	IN	tdev_handle_t hdev
);

/*
 * Error code might be returned:
 *  ERR_HAI_INVALID_PARAMETER
 */
int32
HAI_getdevinfo(
	IN	tdev_handle_t hdev,
	OUT	tdev_info_t * devinfo
);

#endif

/*
 * fat.h
 *
 * Include functions operation FAT.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _TFFS_FAT_H
#define _TFFS_FAT_H

#include "comdef.h"
#include "pubstruct.h"

/* Return code definitions. */
#define FAT_OK						(1)
#define ERR_FAT_LAST_CLUS			(-1)
#define ERR_FAT_DEVICE_FAIL			(-2)
#define ERR_FAT_NO_FREE_CLUSTER		(-3)


/* Public interface. */
tfat_t *
fat_init(
	IN	tffs_t * ptffs
);

void
fat_destroy(
	IN	tfat_t * pfat
);

/*
 * Return:
 *	FAT_OK
 *	ERR_FAT_DEVICE_FAIL
 *	ERR_FAT_LAST_CLUS
 */
int32
fat_get_next_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

/*
 * Return:
 *	TRUE
 *	FALSE
 */
BOOL
fat_get_next_sec(
	IN	tfat_t * pfat,
	OUT	uint32 * pcur_clus,
	OUT uint32 * pcur_sec
);

/*
 * Return:
 *	FAT_OK
 *	ERR_FAT_DEVICE_FAIL
 *	ERR_FAT_NO_FREE_CLUSTER
 */
int32
fat_malloc_clus(
	IN	tfat_t * pfat,
	IN	uint32 cur_clus,
	OUT	uint32 * pnew_clus
);

int32
fat_free_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

#endif

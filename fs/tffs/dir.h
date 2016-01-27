/*
 * dir.h
 *
 * Include routines to implement operation to the directories.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _TFFS_DIR_H
#define _TFFS_DIR_H

#include "comdef.h"
#include "pubstruct.h"

/* Return value definitions. */
#define MAX_PATH_TOKEN			(64)
#define DIR_OK					(1)
#define ERR_DIR_INVALID_PATH	(-1)
#define ERR_DIR_INVALID_DEVICE	(-2)
#define ERR_DIR_NO_FREE_SPACE	(-3)
#define ERR_DIR_INITIALIZE_FAIL	(-4)

/* Interface declaration. */

int32
dir_init(
	IN	tffs_t * ptffs,
	IN	byte * path,
	OUT	tdir_t ** ppdir
);

int32
dir_init_by_clus(
	IN	tffs_t * ptffs,
	IN	uint32 clus,
	OUT	tdir_t ** ppdir
);

void
dir_destroy(
	IN	tdir_t * pdir
);

int32
dir_append_direntry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry
);

int32
dir_del_direntry(
	IN	tdir_t * pdir,
	IN	byte * fname
);

int32
dir_update_direntry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry
);

int32
dir_read_sector(
	IN	tdir_t * pdir
);

int32
dir_write_sector(
	IN	tdir_t * pdir
);

#endif

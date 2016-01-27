/*
 * dirent.h
 *
 * Include functions implementing directory entry.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _DIR_ENT_H
#define _DIR_ENT_H

#include "comdef.h"
#include "pubstruct.h"

/* Private structure definitions. */
typedef struct _long_dir_entry{
	ubyte		ldir_ord;
	uint16		ldir_name1[5];
	ubyte		ldir_attr;
	ubyte		ldir_type;
	ubyte		ldir_chksum;
	uint16		ldir_name2[6];
	uint16		ldir_fst_clus_lo;
	uint16		ldir_name3[2];
}__attribute__((packed)) long_dir_entry_t;

/* Return value definitions. */
#define DIRENTRY_OK					(1)
#define ERR_DIRENTRY_NOMORE_ENTRY	(-1)
#define ERR_DIRENTRY_NOT_FOUND		(-2)
#define ERR_DIRENTRY_DEVICE_FAIL	(-3)

/* Interface declaration. */

int32
dirent_find(
	IN	tdir_t * pdir,
	IN	byte * dirname,
	OUT	tdir_entry_t * pdir_entry
);

int32
dirent_find_free_entry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry
);

int32
dirent_get_next(
	IN	tdir_t * pdir,
	OUT	tdir_entry_t * pdir_entry
);

BOOL
dirent_init(
	IN	byte * fname,
	IN	ubyte dir_attr,
	IN	byte use_long_name,
	OUT	tdir_entry_t * pdir_entry
);

tdir_entry_t *
dirent_malloc(void
);

void
dirent_release(
	IN	tdir_entry_t * pdir_entry
);

uint16
dirent_get_cur_time(void
);

uint16
dirent_get_cur_date(void
);

ubyte
dirent_get_cur_time_tenth(void
);

BOOL
dirent_is_empty(
	IN	tdir_t * pdir
);

#define _last_entry(pdir_entry)	(pdir_entry->pdirent[pdir_entry->dirent_num - 1])

#define dirent_get_clus(pdir_entry) \
	_last_entry(pdir_entry).dir_fst_clus_hi << 16 | \
	_last_entry(pdir_entry).dir_fst_clus_lo
#define dirent_set_clus(pdir_entry, clus) {\
	_last_entry(pdir_entry).dir_fst_clus_hi = (clus >> 16) & 0xFFFF; \
	_last_entry(pdir_entry).dir_fst_clus_lo = clus & 0xFFFF; }

#define dirent_get_file_size(pdir_entry) \
	_last_entry(pdir_entry).dir_file_size
#define dirent_set_file_size(pdir_entry, file_size) \
	_last_entry(pdir_entry).dir_file_size = file_size

#define dirent_update_lst_acc_date(pdir_entry) \
	_last_entry(pdir_entry).dir_lst_acc_date = dirent_get_cur_date()

#define dirent_update_wrt_time(pdir_entry) \
	_last_entry(pdir_entry).dir_wrt_time = dirent_get_cur_time()

#define dirent_update_wrt_date(pdir_entry) \
	_last_entry(pdir_entry).dir_wrt_date = dirent_get_cur_date()

#define dirent_get_dir_attr(pdir_entry) \
	_last_entry(pdir_entry).dir_attr
#define dirent_set_dir_attr(pdir_entry, dir_attr) \
	_last_entry(pdir_entry).dir_attr = dir_attr & 0xFF

#define dirent_get_crt_time_tenth(pdir_entry) \
	_last_entry(pdir_entry).dir_crt_time_tenth
#define dirent_get_crt_time(pdir_entry) \
	_last_entry(pdir_entry).dir_crt_time
#define dirent_get_crt_date(pdir_entry) \
	_last_entry(pdir_entry).dir_crt_date

#endif

/*
 * pubstruct.h
 *
 * Public structure definition.
 * head file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _PUB_STRUCT_H
#define _PUB_STRUCT_H

#include "comdef.h"
#include "crtdef.h"
#include "cache.h"
#include "hai.h"

typedef struct _boot_sector_bh_fat16{
	ubyte	drv_num;
	ubyte	reserved1;
	ubyte	boot_sig;
	uint32	vol_id;
	ubyte	vol_lab[11];
	ubyte	fil_sys_type[8];
	ubyte	boot_code[448];
}__attribute__((packed)) boot_sector_bh_fat16_t;

typedef struct _boot_sector_bh_fat32{
	uint32	fatsz32;
	uint16	ext_flags;
	uint16	fsver;
	uint32	root_clus;
	uint16	fs_info;
	uint16	bk_boot_sec;
	ubyte	reserved[12];
	ubyte	drv_num;
	ubyte	reserved1;
	ubyte	boot_sig;
	uint32	vol_id;
	ubyte	vol_lab[11];
	ubyte	fil_sys_type[8];
	ubyte	boot_code[420];
}__attribute__((packed)) boot_sector_bh_fat32_t;

typedef struct _boot_sector{
	ubyte	jmp_boot[3];
	ubyte	oem_name[8];
	uint16	byts_per_sec;
	ubyte	sec_per_clus;
	uint16	resvd_sec_cnt;
	ubyte	num_fats;
	uint16	root_ent_cnt;
	uint16	tot_sec16;
	ubyte	media;
	uint16	fatsz16;
	uint16	sec_per_trk;
	uint16	num_heads;
	uint32	hidd_sec;
	uint32	tot_sec32;
	union{
		boot_sector_bh_fat16_t	bh16;
		boot_sector_bh_fat32_t	bh32;
	};
	uint16	bs_sig;
}__attribute__((packed)) boot_sector_t;

#define FT_FAT12		(1)
#define FT_FAT16		(2)
#define FT_FAT32		(3)

typedef struct _tffs{
	boot_sector_t*	pbs;
	tdev_handle_t	hdev;
	struct _tfat*	pfat;
	struct _tdir*	root_dir;
	struct _tdir*	cur_dir;
	tcache_t*		pcache;
	/* Marvell fixed */
#ifdef TFFS_FAT_CACHE
	tcache_t*		pfatcache;
#endif
	ubyte			fat_type;
	/* Marvell fixed */
	ubyte			num_fats;
	uint16			root_dir_sectors;
	uint32			sec_root_dir;
	uint32			sec_first_data;
	uint32			sec_fat;
	uint32			fatsz;
	uint32			total_clusters;
	/* Marvell fixed: added externel helper for nospace cases */
	int32                   (*p_nospace_handler)(void *);
	void                    *nospace_handler_arg;
}tffs_t;

typedef struct _tdir{
	tffs_t *		ptffs;
	ubyte *			secbuf;
	uint32			start_clus;
	uint32			cur_clus;
	uint32			cur_sec;
	uint32			cur_dir_entry;
}tdir_t;

#define ATTR_READ_ONLY 		0x01
#define ATTR_HIDDEN 		0x02
#define ATTR_SYSTEM 		0x04
#define ATTR_VOLUME_ID 		0x08
#define ATTR_DIRECTORY		0x10
#define ATTR_ARCHIVE  		0x20
#define ATTR_LONG_NAME 		(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

#define LAST_LONG_ENTRY		0x40

typedef struct _dir_entry{
	ubyte		dir_name[11];
	ubyte		dir_attr;
	ubyte		dir_ntres;
	ubyte		dir_crt_time_tenth;
	uint16		dir_crt_time;
	uint16		dir_crt_date;
	uint16		dir_lst_acc_date;
	uint16		dir_fst_clus_hi;
	uint16		dir_wrt_time;
	uint16		dir_wrt_date;
	uint16		dir_fst_clus_lo;
	uint32		dir_file_size;
}__attribute__((packed)) dir_entry_t;

#define LONG_NAME_LEN		64
#define SHORT_NAME_LEN		14

typedef struct _tdir_entry{
	dir_entry_t	*	pdirent;
	uint32			dirent_num;
	byte			long_name[LONG_NAME_LEN];		
	byte			short_name[SHORT_NAME_LEN];
}tdir_entry_t;

#define ROOT_DIR_CLUS_FAT16	(0)
#define INVALID_CLUSTER		(1)

#define OPENMODE_READONLY		0x1
#define OPENMODE_WRITE			0x2
#define OPENMODE_APPEND			0x4

typedef struct _tfile{
	tffs_t *		ptffs;
	tdir_t *		pdir;
	tdir_entry_t *	pdir_entry;
	ubyte *			secbuf;
	uint32			open_mode;
	uint32			start_clus;

	uint32			file_size;
	uint32			cur_fp_offset;

	uint32			cur_clus;
	uint32			cur_sec;
	uint32			cur_sec_offset;
}tfile_t;

#define FAT_INVALID_CLUS	(0xF0000000)

/* Marvell fixed */
#ifdef TFFS_FAT_MIRROR
struct fat_mirror {
	ubyte *buffer;
	uint32 first;
	uint32 nsec;
	uint32 maxsec;
};
#endif
typedef struct _tfat{
	tffs_t * ptffs;
	uint32 last_free_clus;
	ubyte * secbuf;
	uint32 cur_fat_sec;
#ifdef TFFS_FAT_MIRROR
	struct fat_mirror fatm; /* Marvell fixed */
#endif
}tfat_t;

#endif

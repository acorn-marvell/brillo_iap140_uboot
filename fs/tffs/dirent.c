/*
 * dirent.c
 *
 * Include functions implementing directory entry.
 * implementation file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "dirent.h"
#include "debug.h"
#include "fat.h"
#include "common.h"
#include "dir.h"

#ifndef DBG_DIRENT
#undef DBG
#define DBG nulldbg
#endif

/* Private routins declaration. */

static int32
_get_dirent(
	IN	tdir_t * pdir,
	OUT	dir_entry_t * pdirent
);

static void
_parse_file_name(
	IN	tdir_t * pdir,
	IN	dir_entry_t * pdirent,
	OUT	tdir_entry_t * pdir_entry
);

static BOOL
_get_long_file_name(
    IN  dir_entry_t * pdirent,
    OUT byte * long_name
);

static BOOL
_convert_short_fname(
	IN	ubyte * dir_name,
	OUT	byte * d_name
);

static BOOL
_convert_to_short_fname(
	IN	byte * fname,
	OUT	ubyte * short_fname
);

static ubyte
_get_check_sum(
	IN	ubyte * fname
);

/*----------------------------------------------------------------------------------------------------*/

int32
dirent_find(
	IN	tdir_t * pdir,
	IN	byte * dirname,
	OUT	tdir_entry_t * pdir_entry)
{
	int32 ret;

	pdir->cur_clus = pdir->start_clus;
	pdir->cur_sec = 0;
	pdir->cur_dir_entry = 0;
	if (dir_read_sector(pdir) != DIR_OK)
		return ERR_DIRENTRY_NOT_FOUND;

	ASSERT(pdir_entry->pdirent == NULL);
	ret = DIRENTRY_OK;

	while(1)	{
		dir_entry_t dirent;

		if ((ret = _get_dirent(pdir, &dirent)) == DIRENTRY_OK)	{

			if (dirent.dir_name[0] == 0x00) {
				ret = ERR_DIRENTRY_NOT_FOUND;
				break;
			}
			else if (dirent.dir_name[0] == 0xE5) {
				//FixMe do with 0x5
				continue;
			}

			Memset(pdir_entry->long_name, 0, LONG_NAME_LEN);
			Memset(pdir_entry->short_name, 0, SHORT_NAME_LEN);
			_parse_file_name(pdir, &dirent, pdir_entry);

			if (!Strcmp(pdir_entry->long_name, dirname)) {
				break;
			}
		}
		else if (ret == ERR_DIRENTRY_NOMORE_ENTRY){
			ret = ERR_DIRENTRY_NOT_FOUND;
			break;
		}
		else {
			break;
		}
	}

	return ret;
}

BOOL
dirent_is_empty(
	IN	tdir_t * pdir)
{
	BOOL ret;
	tdir_entry_t * pdir_entry;

	pdir->cur_clus = pdir->start_clus;
	pdir->cur_sec = 0;
	pdir->cur_dir_entry = 0;
	if (dir_read_sector(pdir) != DIR_OK)
		return ERR_DIRENTRY_NOT_FOUND;

	pdir_entry = dirent_malloc();
	ASSERT(pdir_entry->pdirent == NULL);
	ret = TRUE;

	while(1) {
		dir_entry_t dirent;

		if ((ret = _get_dirent(pdir, &dirent)) == DIRENTRY_OK)	{

			if (dirent.dir_name[0] == 0x00) {
				break;
			}
			else if (dirent.dir_name[0] == 0xE5) {
				//FixMe do with 0x5
				continue;
			}

			Memset(pdir_entry->long_name, 0, LONG_NAME_LEN);
			Memset(pdir_entry->short_name, 0, SHORT_NAME_LEN);
			_parse_file_name(pdir, &dirent, pdir_entry);

			if (!Strcmp(pdir_entry->long_name, ".") || !Strcmp(pdir_entry->long_name, "..")) {
				continue;
			}
			else {
				ret = FALSE;
				break;
			}
		}
		else if (ret == ERR_DIRENTRY_NOMORE_ENTRY){
			break;
		}
		else {
			ERR("%s(): get next direntry failed. ret = %d\n", __FUNCTION__, ret);
			ret = FALSE;
			break;
		}
	}

	dirent_release(pdir_entry);
	return ret;
}

/* Marvell fixed: when a new cluster is being allocated to a directory
	the contents of all dirent's should be 0 */
static void
dirent_clear_new_cluster(tdir_t *pdir)
{
	Memset(pdir->secbuf, 0, pdir->ptffs->pbs->byts_per_sec);
	for (pdir->cur_sec = 0;
		pdir->cur_sec < pdir->ptffs->pbs->sec_per_clus;
		pdir->cur_sec++)
		dir_write_sector(pdir);
	pdir->cur_sec = 0;
	pdir->cur_dir_entry = 0;
	/* Leave secbuf as is, so no need to read it again */
}

/* Marvell fixed: when a new entry is appended at the end of a directory,
all the entries should be allocated in the same sector, as they are later
accessed in the sector buffer. This function:
- checks if the required number of entries (dirent_num) is available;
- otherwise, pads the current sector with E5 (deleted) entries,
  and lets the search continue into the following sector. */
static int32 dirent_space_check_extend(tdir_t *pdir, int32 dirent_num)
{
	int32 nent = pdir->ptffs->pbs->byts_per_sec / sizeof(dir_entry_t);
	dir_entry_t *pde = (dir_entry_t *)pdir->secbuf;
	int32 i, n0, ne5;
	for (i = n0 = ne5 = 0; i < nent; i++) {
		unsigned char c = pde[i].dir_name[0];
		if (n0)
			ASSERT(c == 0);
		if (c == 0)
			n0++;
		else if (c == 0xE5)
			ne5++;
		else
			n0 = ne5 = 0;
		if ((n0 + ne5) == dirent_num) {
			pdir->cur_dir_entry = i + 1 - dirent_num;
			return DIRENTRY_OK;
		}
	}

	/* not found the free entry sequence in this sector */
	if (n0) {
		/* replace 0 entries with E5 as dir continues to next sector */
		for (i = nent - n0; i < nent; i++)
			pde[i].dir_name[0] = 0xE5;
		dir_write_sector(pdir);
	}
	pdir->cur_dir_entry = nent; /* sector ended */
	return ERR_DIRENTRY_NOMORE_ENTRY;
}

int32
dirent_find_free_entry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry)
{
	int32 ret;
	tffs_t * ptffs = pdir->ptffs;

	ret = DIRENTRY_OK;

	while(1) {
		dir_entry_t dirent;

		if ((ret = _get_dirent(pdir, &dirent)) == DIRENTRY_OK)	{
			/* Marvell fixed: revised search for free entries */
			if (dirent_space_check_extend(pdir, pdir_entry->dirent_num) == DIRENTRY_OK)
				break; /* enough free entries in this sector */
		}
		else if (ret == ERR_DIRENTRY_NOMORE_ENTRY) {
			uint32 new_clus;

			dir_write_sector(pdir);

			if ((ret = fat_malloc_clus(ptffs->pfat, pdir->cur_clus, &new_clus)) == FAT_OK) {
				pdir->cur_clus = new_clus;
				pdir->cur_sec = 0;
				pdir->cur_dir_entry = 0;
				/* Marvell fixed: zero the new cluster */
				dirent_clear_new_cluster(pdir);
			}
			else {
				ret = ERR_DIRENTRY_NOMORE_ENTRY;
				break;
			}
		}
		else {
			break;
		}
	}

	return ret;
}

int32
dirent_get_next(
	IN	tdir_t * pdir,
	OUT	tdir_entry_t * pdir_entry)
{
	int32 ret;

	ret = DIRENTRY_OK;

	while(1)	{
		dir_entry_t dirent;

		//print_sector(pdir->secbuf, 1);
		DBG("%s():pdir->cur_clus = %d, pdir->cur_dir_entry = %d\n", __FUNCTION__, pdir->cur_clus, pdir->cur_dir_entry);
		if ((ret = _get_dirent(pdir, &dirent)) == DIRENTRY_OK)	{

			if (dirent.dir_name[0] == 0x00) {
				ret = ERR_DIRENTRY_NOMORE_ENTRY;
				break;
			}
			else if (dirent.dir_name[0] == 0xE5) {
				//FixMe do with 0x5
				continue;
			}

			Memset(pdir_entry->long_name, 0, LONG_NAME_LEN);
			Memset(pdir_entry->short_name, 0, SHORT_NAME_LEN);
			_parse_file_name(pdir, &dirent, pdir_entry); 

			break;
		}
		else {
			break;
		}
	}
	return ret;
}

void
dirent_release(
	IN	tdir_entry_t * pdir_entry)
{
	Free(pdir_entry->pdirent);
	Free(pdir_entry);
}

tdir_entry_t *
dirent_malloc()
{
	tdir_entry_t * pdir_entry;

	pdir_entry = (tdir_entry_t *)Malloc(sizeof(tdir_entry_t));
	Memset(pdir_entry, 0, sizeof(tdir_entry_t));
	return pdir_entry;
}

BOOL
dirent_init(
	IN	byte * fname,
	IN	ubyte dir_attr,
	IN	byte use_long_name,
	OUT	tdir_entry_t * pdir_entry)
{
	long_dir_entry_t * plfent;
	dir_entry_t * pdirent;
	uint32 lfent_num;
	int32 lfent_i;
	byte * pfname;

	if (Strlen(fname) > LONG_NAME_LEN ||
		(!use_long_name && Strlen(fname) > SHORT_NAME_LEN))
		return FALSE;

	if (use_long_name) {
		lfent_num = Strlen(fname) / 13 + 2;
	}
	else {
		lfent_num = 1;
	}

	pfname = fname;

	plfent = (long_dir_entry_t *)Malloc(sizeof(long_dir_entry_t) * lfent_num);
	Memset(plfent, 0, sizeof(long_dir_entry_t) * lfent_num);
	pdirent = (dir_entry_t *)(&plfent[lfent_num - 1]);
	
	_convert_to_short_fname(fname, pdirent->dir_name);
	DBG("%s(): %s=>%s\n", __FUNCTION__, fname, pdirent->dir_name);
	pdirent->dir_attr = dir_attr;
	pdirent->dir_ntres = 0;
	pdirent->dir_crt_time_tenth = dirent_get_cur_time_tenth();
	pdirent->dir_crt_time = dirent_get_cur_time();
	pdirent->dir_crt_date = dirent_get_cur_date();
	pdirent->dir_lst_acc_date = pdirent->dir_crt_date;
	pdirent->dir_wrt_time = pdirent->dir_crt_time;
	pdirent->dir_wrt_date = pdirent->dir_crt_date;
	pdirent->dir_fst_clus_hi = 0;
	pdirent->dir_fst_clus_lo = 0;
	pdirent->dir_file_size = 0;

	if (use_long_name) {
		for (lfent_i = lfent_num - 2; lfent_i >= 0; lfent_i--) {
			ubyte fname_line[13];

			Memset(fname_line, 0xFF, 13);
			Memcpy(fname_line, pfname, min(fname + Strlen(fname) - pfname + 1, 13));

			if (lfent_i == 0) {
				plfent[lfent_i].ldir_ord = (lfent_num - 1 - lfent_i) | LAST_LONG_ENTRY;
			}
			else {
				plfent[lfent_i].ldir_ord = lfent_num - 1 - lfent_i;
			}
			copy_to_unicode(fname_line, 5, plfent[lfent_i].ldir_name1);
			copy_to_unicode(fname_line + 5, 6, plfent[lfent_i].ldir_name2);
			copy_to_unicode(fname_line + 11, 2, plfent[lfent_i].ldir_name3);
			pfname += 13;

			plfent[lfent_i].ldir_attr = ATTR_LONG_NAME;
			plfent[lfent_i].ldir_type = 0;
			plfent[lfent_i].ldir_chksum = _get_check_sum(pdirent->dir_name);
			plfent[lfent_i].ldir_fst_clus_lo = 0;
		}
	}

	pdir_entry->pdirent = (dir_entry_t *)plfent;
	pdir_entry->dirent_num = lfent_num;
	Strcpy(pdir_entry->long_name, fname);	
	_convert_short_fname(pdirent->dir_name, pdir_entry->short_name);

	return TRUE;
}

uint16
dirent_get_cur_time()
{
	tffs_sys_time_t curtm;
	uint16 ret;

	Getcurtime(&curtm);	

	ret = 0;
	ret |= (curtm.tm_sec >> 2) & 0x1F;
	ret |= (curtm.tm_min << 5) & 0x7E0;
	ret |= (curtm.tm_hour << 11) & 0xF800;
	
	return ret;
}

uint16
dirent_get_cur_date()
{
	tffs_sys_time_t curtm;
	uint16 ret;

	Getcurtime(&curtm);	
	
	ret = 0;
	ret |= (curtm.tm_mday) & 0x1F;
	ret |= ((curtm.tm_mon + 1) << 5) & 0x1E0;
	ret |= ((curtm.tm_year - 80) << 9) & 0xFE00;
	
	return ret;
}

ubyte
dirent_get_cur_time_tenth()
{
	tffs_sys_time_t curtm;
	uint16 ret;

	Getcurtime(&curtm);	
	
	ret = (curtm.tm_sec & 1) == 0 ? 0 : 100;
	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

static int32
_get_dirent(
	IN	tdir_t * pdir,
	OUT	dir_entry_t * pdirent)
{
	int32 ret;

	ret = DIRENTRY_OK;
	
	if (pdir->cur_dir_entry < 
			(pdir->ptffs->pbs->byts_per_sec / sizeof(dir_entry_t))) {
		Memcpy(pdirent, (dir_entry_t *)pdir->secbuf + pdir->cur_dir_entry, sizeof(dir_entry_t));
		pdir->cur_dir_entry++;
	}
	else {
		if (fat_get_next_sec(pdir->ptffs->pfat, &pdir->cur_clus, &pdir->cur_sec)) {
			pdir->cur_dir_entry = 0;
			if ((ret = dir_read_sector(pdir)) == DIR_OK) {
				Memcpy(pdirent, (dir_entry_t *)pdir->secbuf + pdir->cur_dir_entry, sizeof(dir_entry_t));
				pdir->cur_dir_entry++;
			}
			else {
				ret = ERR_DIRENTRY_DEVICE_FAIL;
			}
		}
		else {
			ret = ERR_DIRENTRY_NOMORE_ENTRY;
		}
	}

	return ret;
}

static void
_parse_file_name(
	IN	tdir_t * pdir,
	IN	dir_entry_t * pdirent,
	OUT	tdir_entry_t * pdir_entry)
{
	int32 lf_entry_num;

	lf_entry_num = 0;
	/* Marvell fixed: was "& ATTR_LONG_NAME", however one of the bits
	is ATTR_VOLUME_ID, which alone indicates a volume id, short */
	if (pdirent->dir_attr == ATTR_LONG_NAME) {
		uint32 lf_i;
		dir_entry_t dirent;

		lf_entry_num = pdirent->dir_name[0] & ~(LAST_LONG_ENTRY);
		pdir_entry->pdirent = (dir_entry_t *)Malloc((lf_entry_num + 1) * sizeof(dir_entry_t));

		_get_long_file_name(pdirent, pdir_entry->long_name + (lf_entry_num - 1) * 13);
		Memcpy(pdir_entry->pdirent, pdirent, sizeof(dir_entry_t));

		for (lf_i = 1; lf_i < lf_entry_num; lf_i++) {
			_get_dirent(pdir, &dirent);
			Memcpy(pdir_entry->pdirent + lf_i, &dirent, sizeof(dir_entry_t));
			_get_long_file_name(&dirent, pdir_entry->long_name + (lf_entry_num - lf_i - 1) * 13);
		}

		_get_dirent(pdir, &dirent);
		Memcpy(pdir_entry->pdirent + lf_i, &dirent, sizeof(dir_entry_t));
	}
	else {
		pdir_entry->pdirent = (dir_entry_t *)Malloc(sizeof(dir_entry_t));

		_convert_short_fname(pdirent->dir_name, pdir_entry->long_name);
		Memcpy(pdir_entry->pdirent, pdirent, sizeof(dir_entry_t));
	}

	_convert_short_fname(pdirent->dir_name, pdir_entry->short_name);
	pdir_entry->dirent_num = lf_entry_num + 1;
}

static ubyte
_get_check_sum(
	IN	ubyte * fname)
{
	int16 fname_len;
	ubyte sum;

	sum = 0;
	for (fname_len = 11; fname_len != 0; fname_len--) {
		sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *fname++;
	}
	return sum;
}

static BOOL
_convert_to_short_fname(
	IN	byte * fname,
	OUT	ubyte * short_fname)
{
	byte * pfname;
	byte * pcur;
	uint16 sf_i;
	static uint16 num_tail = 0;

	for (sf_i = 0; sf_i < 11; sf_i++)
		short_fname[sf_i] = ' ';

	if (!Strcmp(fname, ".") || !Strcmp(fname, "..")) {
		/* Marvell fixed: names should not be 0-terminated */
		Memcpy((byte *)short_fname, fname, Strlen(fname));
		return TRUE;
	}		

	pfname = dup_string(fname);
	pcur = pfname;
	sf_i = 0;

	trip_blanks(pfname);

	while (sf_i < 8) {
		if (*pcur == '\0' || *pcur == '.') {
			break;
		}
		short_fname[sf_i++] = Toupper(*pcur++);
	}

	if (*pcur == '.') {
		pcur++;
	}
	else {
		if (*pcur != '\0') {
			byte str_tail[8];

			while(*pcur && *pcur != '.')
				pcur++;

			if (*pcur == '.')
				pcur++;
		
			Sprintf(str_tail, "~%d", num_tail++);
			Memcpy(short_fname + (8 - Strlen(str_tail)), str_tail, Strlen(str_tail));

			if (*pcur == '\0')
				goto _release;
		}
	}

	sf_i = 8;

	while (sf_i < 11) {
		if (*pcur == '\0')
			break;
		short_fname[sf_i++] = Toupper(*pcur++);
	}

_release:
	Free(pfname);
	return TRUE;
}

static BOOL
_get_long_file_name(
	IN	dir_entry_t * pdirent,
	OUT	byte * long_name)
{
	long_dir_entry_t * pldirent;

	pldirent = (long_dir_entry_t *)pdirent;

	copy_from_unicode(pldirent->ldir_name1, 5, long_name);	
	copy_from_unicode(pldirent->ldir_name2, 6, long_name + 5);	
	copy_from_unicode(pldirent->ldir_name3, 2, long_name + 11);	
	return TRUE;
}

static BOOL
_convert_short_fname(
	IN	ubyte * dir_name,
	OUT	byte * d_name)
{
	uint32 i;

	Memset(d_name, 0, 11);
	for (i = 0; i < 8; i++) {
		if (dir_name[i] == ' ')
			break;
		d_name[i] = dir_name[i];
	}

	if (dir_name[8] != ' ') {
		uint32 j;

		d_name[i++] = '.';
		for (j = 0; j < 3; j++) {
			if (dir_name[8 + j] == ' ')
				break;
			d_name[i + j] = dir_name[8 + j];
		}
	}
	return TRUE;
}


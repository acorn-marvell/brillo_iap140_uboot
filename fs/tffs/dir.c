/*
 * dir.c
 *
 * Include routines to implement operation to the directories.
 * implementation file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "dir.h"
#include "tffs.h"
#include "fat.h"
#include "dirent.h"
#include "common.h"
#include "debug.h"

#ifndef DBG_DIR
#undef DBG
#define DBG nulldbg
#endif

/* Private routins declaration. */

static uint32
_parse_path(
	IN	tffs_t * ptffs,
	IN	byte * path
);

static tdir_t *
_dir_init(
	IN	tffs_t * ptffs,
	IN	uint32 clus
);

static void
_dir_destroy(
	IN	tdir_t * pdir
);

static int32
_initialize_dir(
	IN	tffs_t * ptffs,
	IN	uint32 parent_dir_clus,
	OUT	tdir_entry_t * pdir_entry
);

/*----------------------------------------------------------------------------------------------------*/

int32
TFFS_opendir(
	IN	tffs_handle_t hfs,
	IN	byte * path,
	OUT	tdir_handle_t * phdir)
{
	int32 ret;
	tffs_t * ptffs = (tffs_t *)hfs;
	tdir_t * pdir;

	if (!hfs || !path || !phdir)
		return ERR_TFFS_INVALID_PARAM;

	ret = TFFS_OK;
	if (dir_init(ptffs, path, &pdir) == DIR_OK) {
		*phdir = (tdir_handle_t)pdir;
	}
	else {
		ret = ERR_TFFS_INVALID_PATH;
	}

	return ret;
}

int32
TFFS_readdir(
	IN	tdir_handle_t hdir,
	OUT	dirent_t * pdirent)
{
	tdir_t * pdir = (tdir_t *)hdir;
	uint32 ret;
	tdir_entry_t * pdir_entry;

	if (!hdir || !pdirent)
		return ERR_TFFS_INVALID_PARAM;

	ret = TFFS_OK;
	pdir_entry = dirent_malloc();

	while (1) {
		uint32 de_ret;

		de_ret = dirent_get_next(pdir, pdir_entry);
		if (de_ret == DIRENTRY_OK) {
			Strcpy(pdirent->d_name, pdir_entry->long_name);
			Strcpy(pdirent->d_name_short, pdir_entry->short_name);

			pdirent->dir_attr = dirent_get_dir_attr(pdir_entry);
			pdirent->dir_file_size = dirent_get_file_size(pdir_entry);

			pdirent->crttime.year = ((dirent_get_crt_date(pdir_entry) & 0xFE00) >> 9) + 1980;
			pdirent->crttime.month = ((dirent_get_crt_date(pdir_entry) & 0x1E0) >> 5);
			pdirent->crttime.day = (dirent_get_crt_date(pdir_entry) & 0x1F);

			pdirent->crttime.hour = (dirent_get_crt_time(pdir_entry) & 0xF800) >> 11;
			pdirent->crttime.min = (dirent_get_crt_time(pdir_entry) & 0x7E0) >> 5;
			pdirent->crttime.sec = ((dirent_get_crt_time(pdir_entry) & 0x1F) << 2)
				+ (dirent_get_crt_time_tenth(pdir_entry) & 1);

			break;
		}
		else if (de_ret == ERR_DIRENTRY_NOMORE_ENTRY) {
			ret = ERR_TFFS_LAST_DIRENTRY;
			break;
		}
		else {
			ret = ERR_TFFS_DEVICE_FAIL;
		}
	}

	dirent_release(pdir_entry);
	return ret;
}

int32
TFFS_closedir(
	IN	tdir_handle_t hdir)
{
	tdir_t * pdir;

	if (!hdir)
		return ERR_TFFS_INVALID_PARAM;

	pdir = (tdir_t *)hdir;
	dir_destroy(pdir);

	return TFFS_OK;
}

int32
TFFS_mkdir(
	IN	tffs_handle_t hfs,
	IN	byte * dir_path)
{
	int32 ret;
	byte * dirname, * path;
	byte * dup_dir_path;
	tdir_t * pdir = NULL;
	tdir_entry_t * pdir_entry;
	tffs_t * ptffs;

	if (!hfs || !dir_path)
		return ERR_TFFS_INVALID_PARAM;

	ret = TFFS_OK;
	ptffs = (tffs_t *)hfs;
	dup_dir_path = dup_string(dir_path);
	dirname = (byte *)Malloc(DNAME_MAX);
	pdir_entry = dirent_malloc();
	
	path = dup_dir_path;
	if (!divide_path(dup_dir_path, dirname)) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if ((dir_init(ptffs, path, &pdir)) != DIR_OK) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if (dirent_find(pdir, dirname, pdir_entry) == DIRENTRY_OK) {
		ret = ERR_TFFS_DIR_ALREADY_EXIST;
		goto _release;
	}

	if (!dirent_init(dirname, ATTR_DIRECTORY, TRUE, pdir_entry)) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release;
	}

	if ((ret = _initialize_dir(ptffs, pdir->start_clus, pdir_entry)) != DIR_OK) {
		ret = ERR_TFFS_INITIAL_DIR_FAIL;
		goto _release;
	}

	if ((ret = dir_append_direntry(pdir, pdir_entry)) != DIR_OK) {
		if (ret == ERR_DIR_NO_FREE_SPACE) {
			ret = ERR_TFFS_NO_FREE_SPACE;
		}
		else {
			ret = ERR_TFFS_DEVICE_FAIL;
		}
		goto _release;
	}

	if (ret == TFFS_OK)
		DBG("%s(): %s[%d:%d] ok!\n", __FUNCTION__, dir_path, pdir->start_clus, dirent_get_clus(pdir_entry));
_release:
	if (pdir)
		dir_destroy(pdir);
	Free(dirname);
	Free(dup_dir_path);
	dirent_release(pdir_entry);
	return ret;
}

int32
TFFS_chdir(
	IN	tffs_handle_t hfs,
	IN	byte * dir_path)
{
	int32 ret;
	tffs_t * ptffs;
	tdir_t * pdir;

	ret = TFFS_OK;
	if (!hfs || !dir_path)
		return ERR_TFFS_INVALID_PARAM;

	ptffs = (tffs_t *)hfs;
	if ((dir_init(ptffs, dir_path, &pdir)) == DIR_OK) {
		dir_destroy(ptffs->cur_dir);
		ptffs->cur_dir = pdir;
	}
	else {
		ret = ERR_TFFS_INVALID_PATH;
	}

	return ret;
}

int32
TFFS_rmdir(
	IN	tffs_handle_t hfs,
	IN	byte * dir_path)
{
	int32 ret;
	byte * dirname, * path;
	byte * dup_dir_path;
	tdir_t * pdir;
	tffs_t * ptffs;
	tdir_entry_t * pdir_entry;
	tdir_t * prm_dir;

	ret = TFFS_OK;

	if (!hfs || !dir_path || !Strcmp(dir_path, "/"))
		return ERR_TFFS_INVALID_PARAM;

	ptffs = (tffs_t *)hfs;
	dup_dir_path = dup_string(dir_path);
	dirname = (byte *)Malloc(DNAME_MAX);
	pdir_entry = dirent_malloc();

	path = dup_dir_path;
	if (!divide_path(dup_dir_path, dirname)) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release1;
	}

	if ((dir_init(ptffs, path, &pdir)) != DIR_OK) {
		ret = ERR_TFFS_INVALID_PATH;
		goto _release1;
	}

	if ((ret = dirent_find(pdir, dirname, pdir_entry)) == DIRENTRY_OK) {
		if (!(dirent_get_dir_attr(pdir_entry) & ATTR_DIRECTORY)) {
			ret = ERR_TFFS_IS_NOT_A_DIRECTORY;
			goto _release2;
		}

		if ((dir_init(ptffs, dir_path, &prm_dir)) != DIR_OK) {
		ret = ERR_TFFS_INVALID_PATH;
	        goto _release2;
		}

		if (!dirent_is_empty(prm_dir)) {
			ret = ERR_TFFS_NOT_EMPTY_DIR;
			goto _release3;
		}

		if (dir_del_direntry(pdir, dirname) != DIR_OK) {
			ret = ERR_TFFS_REMOVE_DIR_FAIL;
			goto _release3;
		}

		if (fat_free_clus(pdir->ptffs->pfat, dirent_get_clus(pdir_entry)) != FAT_OK) {
			ret = ERR_TFFS_REMOVE_DIR_FAIL;
			goto _release3;
		}

		ret = TFFS_OK;
	} else {
		ret = ERR_TFFS_NO_SUCH_FILE;
	    goto _release2;
	}

_release3:
	dir_destroy(prm_dir);
_release2:
	dir_destroy(pdir);
_release1:
	Free(dirname);
	Free(dup_dir_path);
	dirent_release(pdir_entry);
	return ret;
}

/*----------------------------------------------------------------------------------------------------*/

int32
dir_init(
	IN	tffs_t * ptffs,
	IN	byte * dirpath,
	OUT	tdir_t ** ppdir)
{
	int32 ret;
	tdir_t * pdir;

	if ((ret = _parse_path(ptffs, dirpath)) != INVALID_CLUSTER) {
		if ((pdir = _dir_init(ptffs, ret)) != NULL) {
			ret = DIR_OK;
			*ppdir = pdir;
		}
		else {
			ret = ERR_DIR_INVALID_DEVICE;
		}
	}
	else {
		ret = ERR_DIR_INVALID_PATH;
	}

	return ret;
}

int32
dir_init_by_clus(
	IN	tffs_t * ptffs,
	IN	uint32 clus,
	OUT	tdir_t ** ppdir)
{
	tdir_t * pdir;

	if ((pdir = _dir_init(ptffs, clus)) != NULL) {
		*ppdir = pdir;
		return DIR_OK;
	}
	else {
		return ERR_DIR_INVALID_DEVICE;
	}
}

void
dir_destroy(
	IN	tdir_t * pdir)
{
	_dir_destroy(pdir);
}

int32
dir_read_sector(
	IN	tdir_t * pdir)
{
	int32 ret;
	ASSERT(pdir->secbuf);

	if (((ret = cache_readsector(pdir->ptffs->pcache, clus2sec(pdir->ptffs, pdir->cur_clus) + pdir->cur_sec,
			pdir->secbuf)) == CACHE_OK)) {
		return DIR_OK;
	}
	else {
		return ERR_DIR_INVALID_DEVICE;
	}
}

int32
dir_write_sector(
	IN	tdir_t * pdir)
{
	int ret;
	ASSERT(pdir->secbuf);

	if ((ret = cache_writesector(pdir->ptffs->pcache, clus2sec(pdir->ptffs, pdir->cur_clus) + pdir->cur_sec,
			pdir->secbuf)) != CACHE_OK) {
		return ERR_DIR_INVALID_DEVICE;
	}
	else {
		return DIR_OK;
	}
}

int32
dir_append_direntry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry)
{
	int32 ret;

    pdir->cur_clus = pdir->start_clus;
    pdir->cur_sec = 0;
    pdir->cur_dir_entry = 0;
    if (dir_read_sector(pdir) != DIR_OK)
        return ERR_DIR_INVALID_DEVICE;

	ret = dirent_find_free_entry(pdir, pdir_entry);

	if (ret == DIRENTRY_OK) {
		dir_entry_t * pdirent;

		pdirent = (dir_entry_t *)pdir->secbuf + pdir->cur_dir_entry;
		/* Marvell fixed: unified the logic, dirent_find_free_entry
		allocates the required number of entries continuosly and
		inside the same sector */
		Memcpy(pdirent,
			pdir_entry->pdirent,
			pdir_entry->dirent_num * sizeof(dir_entry_t));

		pdir->cur_dir_entry += pdir_entry->dirent_num;
		ret = dir_write_sector(pdir);
	}
	else if (ret == ERR_DIRENTRY_NOMORE_ENTRY){
		WARN("%s(): No free clusters, probably disk is full.\n", __FUNCTION__);
		ret = ERR_DIR_NO_FREE_SPACE;
	}

	return ret;
}

int32
dir_del_direntry(
	IN	tdir_t * pdir,
	IN	byte * fname)
{
	int32 ret;
	tdir_entry_t * pdir_entry;

	pdir_entry = dirent_malloc();

	ret = dirent_find(pdir, fname, pdir_entry);
	if (ret == DIRENTRY_OK) {
		dir_entry_t * pdirent;
		int32 i;

		pdirent = (dir_entry_t *)pdir->secbuf + pdir->cur_dir_entry
			- pdir_entry->dirent_num;
		for (i = 0; i < pdir_entry->dirent_num; i++) {
			pdirent->dir_name[0] = 0xE5;
			pdirent++;
		}

		ret = dir_write_sector(pdir);
	}

	dirent_release(pdir_entry);
	return ret;
}

int32
dir_update_direntry(
	IN	tdir_t * pdir,
	IN	tdir_entry_t * pdir_entry)
{
	int32 ret;
	dir_entry_t * pdst_entry;

	/* Mavell fixed: re-fetch the directory sector from disk
	as it might have changed while the file was open */
	dir_read_sector(pdir);
	pdst_entry = (dir_entry_t *)pdir->secbuf + pdir->cur_dir_entry - pdir_entry->dirent_num;

	ASSERT(!Memcmp(pdst_entry[pdir_entry->dirent_num - 1].dir_name,
		pdir_entry->pdirent[pdir_entry->dirent_num - 1].dir_name, 11));

	Memcpy(pdst_entry,
		pdir_entry->pdirent,
		pdir_entry->dirent_num * sizeof(dir_entry_t));

	ret = dir_write_sector(pdir);
	return ret;
}
/*----------------------------------------------------------------------------------------------------*/

static BOOL
_init_dot_dir(
	IN	byte * dir_name,
	IN	uint32 clus,
	OUT	ubyte * psecbuf)
{
	tdir_entry_t * pdir_entry;
	BOOL ret;

	pdir_entry = dirent_malloc();
	ret = TRUE;
	if (!dirent_init(dir_name, ATTR_DIRECTORY, FALSE, pdir_entry)) {
		ret = FALSE;
		goto _release;
	}
	dirent_set_clus(pdir_entry, clus);
	ASSERT(pdir_entry->dirent_num == 1);
	Memcpy(psecbuf, pdir_entry->pdirent, sizeof(dir_entry_t));

_release:
	dirent_release(pdir_entry);
	return ret;
}

static int32
_initialize_dir(
	IN	tffs_t * ptffs,
	IN	uint32 parent_dir_clus,
	OUT	tdir_entry_t * pdir_entry)
{
	uint32 new_clus;
	int32 ret;
	ubyte * secbuf;

	secbuf = (ubyte *)Malloc(ptffs->pbs->byts_per_sec);
	Memset(secbuf, 0, ptffs->pbs->byts_per_sec);

	if ((ret = fat_malloc_clus(ptffs->pfat, FAT_INVALID_CLUS, &new_clus)) == FAT_OK) {
		/*
		 * Marvell fixed: clean the entire cluster, don't leave junk,
		 * see also dirent_clear_new_cluster().
		 */
		int s;
		ret = ERR_DIR_INITIALIZE_FAIL;
		for (s = 1; s < ptffs->pbs->sec_per_clus; s++)
			if (cache_writesector(ptffs->pcache,
				clus2sec(ptffs, new_clus) + s, secbuf)
				!= CACHE_OK)
				goto _release;

		if (!_init_dot_dir(".", new_clus, secbuf))
			goto _release;

		if (!_init_dot_dir("..", parent_dir_clus, secbuf + sizeof(dir_entry_t)))
			goto _release;

		if (cache_writesector(ptffs->pcache, clus2sec(ptffs, new_clus), secbuf) != CACHE_OK)
			goto _release;

		dirent_set_clus(pdir_entry, new_clus);
		ret = FAT_OK;
	}

_release:
	Free(secbuf);
	return ret;
}

static uint32
_parse_path(
	IN	tffs_t * ptffs,
	IN	byte * path)
{
	byte * duppath;
	byte * tokens[MAX_PATH_TOKEN];
	tdir_entry_t * pdir_entry;
	tdir_t * pdir;
	uint32 ntoken;
	uint32 itoken;
	uint32 dir_clus;

	if (path[0] != '/') {
		pdir = _dir_init(ptffs, ptffs->cur_dir->start_clus);
	}
	else {
		pdir = _dir_init(ptffs, ptffs->root_dir->start_clus);
	}

	if (pdir == NULL)
		return INVALID_CLUSTER;

	dir_clus = pdir->start_clus;
	duppath = dup_string(path);
	ntoken = tokenize(duppath, '/', tokens);

	for (itoken = 0; itoken < ntoken; itoken++) {
		pdir_entry = dirent_malloc();

		DBG("%s(): token[%d]=%s\n", __FUNCTION__, itoken, tokens[itoken]);
		if (dirent_find(pdir, tokens[itoken], pdir_entry) != DIRENTRY_OK) {
			dir_clus = INVALID_CLUSTER;
			dirent_release(pdir_entry);
			goto _release;
		}

		if (!(dirent_get_dir_attr(pdir_entry) & ATTR_DIRECTORY)) {
			WARN("%s(): invalid directory token. long_name = %s\n", __FUNCTION__, pdir_entry->long_name);
			dir_clus = INVALID_CLUSTER;
			dirent_release(pdir_entry);
			goto _release;
		}

		dir_clus = dirent_get_clus(pdir_entry);
		pdir->start_clus = dir_clus;
		pdir->cur_clus = dir_clus;
		dirent_release(pdir_entry);
	}

_release:
	Free(duppath);
	_dir_destroy(pdir);

	return dir_clus;
}

static tdir_t *
_dir_init(
	IN	tffs_t * ptffs,
	IN	uint32 clus)
{
	tdir_t * pdir;

	pdir = (tdir_t *)Malloc(sizeof(tdir_t));
	pdir->ptffs = ptffs;
	pdir->secbuf = (ubyte *)Malloc(ptffs->pbs->byts_per_sec);
	pdir->start_clus = clus;
	pdir->cur_clus = clus;
	pdir->cur_sec = 0;
	pdir->cur_dir_entry = 0;

	if (dir_read_sector(pdir) == DIR_OK) {
		return pdir;
	}
	Free(pdir->secbuf);
	Free(pdir);
	return NULL;
}

static void
_dir_destroy(
	IN	tdir_t * pdir)
{
	Free(pdir->secbuf);
	Free(pdir);
}


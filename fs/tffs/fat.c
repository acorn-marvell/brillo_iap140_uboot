/*
 * fat.c
 *
 * Include functions operation FAT.
 * implementation file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "fat.h"
#include "debug.h"
#include "tffs.h"

#ifdef TFFS_FAT_CACHE
#include "cache.h"
#endif
#ifndef DBG_FAT
#undef DBG
#define DBG nulldbg
#endif

/* Internal routines declaration. */

static uint32
_clus2fatsec(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static int32
_lookup_free_clus(
	IN	tfat_t * pfat,
	OUT	uint32 * pfree_clus
);

static uint32
_get_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static void
_set_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus,
	IN	uint32 entry_val
);

static uint16
_get_fat_entry_len(
	IN	tfat_t * pfat
);

static BOOL
_read_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static BOOL
_write_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 clus
);

static BOOL
_is_entry_free(
	IN	tfat_t * pfat,
	IN	uint32 entry_val
);

static BOOL
_is_entry_eof(
	IN	tfat_t * pfat,
	IN	uint32 entry_val
);

/*----------------------------------------------------------------------------------------------------*/
/* Marvell fixed: FAT1 should be sync with main FAT0.
	Just writing each FAT sector in _write_fat_sector to FAT1 as well
	Works but results in performance issue: FAT cache cast-outs
	(writes) two sectors back-to-back, which is very inefficient.
	Solution: accumulate FAT writes as long as they are in sequence,
	and update FAT1 once a disconnect occurs or at the end.
*/
#ifdef TFFS_FAT_MIRROR
#define TFFS_FAT_MIRROR_SECTORS 16 /* costs 8KB and provided reasonable performance */

static void
fatm_flush(tfat_t *pfat)
{
	tffs_t *ptffs = pfat->ptffs;
	struct fat_mirror *fm = &pfat->fatm;
	uint32 sec = ptffs->fatsz + fm->first;
	if (fm->nsec)
		if (HAI_writesectors(ptffs->hdev, sec, fm->buffer, fm->nsec) != HAI_OK)
			ERR("Failed to write FAT mirror data at %ld\n", (long)sec);

	fm->nsec = 0;
	fm->first = 0;
}

static void
fatm_write_sector(
	tfat_t	*pfat,
	ubyte	*pdata,
	uint32	sector)
{
	tffs_t *ptffs = pfat->ptffs;
	struct fat_mirror *fm = &pfat->fatm;

	if (!fm->buffer)
		return;
	if ((sector <= fm->first) || (sector >= (fm->first + fm->nsec))) {
		/* not an existing sector */
		if ((sector != (fm->first + fm->nsec)) || (fm->nsec == fm->maxsec)) {
			fatm_flush(pfat);
			fm->first = sector;
		}
		fm->nsec++;
	}

	Memcpy(fm->buffer + (sector-fm->first)*ptffs->pbs->byts_per_sec,
		pdata, ptffs->pbs->byts_per_sec);
}

static void
fatm_init(
	tfat_t *pfat)
{
	tffs_t *ptffs = pfat->ptffs;
	struct fat_mirror *fm = &pfat->fatm;
	fm->buffer = 0;
	if (ptffs->num_fats < 2)
		return;
	fm->maxsec = TFFS_FAT_MIRROR_SECTORS;
	fm->nsec = 0;
	fm->first = 0;
	fm->buffer = Malloc(fm->maxsec*ptffs->pbs->byts_per_sec);
	if (!fm->buffer)
		ERR("TFFS failed to allocate memory for FAT mirror: will not sync.\n");
}

static void fatm_destroy(
	tfat_t *pfat)
{
	struct fat_mirror *fm = &pfat->fatm;
	if (fm->buffer) {
		fatm_flush(pfat);
		Free(fm->buffer);
	}
}
#endif
/*----------------------------------------------------------------------------------------------------*/

tfat_t *
fat_init(
	IN	tffs_t * ptffs)
{
	tfat_t * pfat;

	ASSERT(ptffs);
	pfat = (tfat_t *)Malloc(sizeof(tfat_t));
	pfat->ptffs = ptffs;
    pfat->last_free_clus = 0;
    pfat->secbuf = (ubyte *)Malloc(ptffs->pbs->byts_per_sec * 2);
#ifdef TFFS_FAT_MIRROR
	fatm_init(pfat);
#endif
	return pfat;
}

void
fat_destroy(
	IN	tfat_t * pfat)
{
#ifdef TFFS_FAT_MIRROR
	fatm_destroy(pfat);
#endif
	Free(pfat->secbuf);
	Free(pfat);
}

int32
fat_get_next_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	uint32 next_clus;

	ASSERT(pfat);
	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, clus)))
		return ERR_FAT_DEVICE_FAIL;

	next_clus = _get_fat_entry(pfat, clus);
	if (_is_entry_eof(pfat, next_clus))
		return ERR_FAT_LAST_CLUS;

	return next_clus;
}

BOOL
fat_get_next_sec(
	IN	tfat_t * pfat,
	OUT	uint32 * pcur_clus,
	OUT uint32 * pcur_sec)
{
	tffs_t * ptffs = pfat->ptffs;
	int32 next_clus;

	DBG("%s(): get next sector for cluster 0x%x\n", __FUNCTION__, *pcur_clus);
	if (*pcur_clus == ROOT_DIR_CLUS_FAT16 && 
		(ptffs->fat_type == FT_FAT12 || ptffs->fat_type == FT_FAT16)) {
		if (*pcur_sec + 1 < ptffs->root_dir_sectors) {
			(*pcur_sec)++;
		}
		else {
			return FALSE;
		}
	}
	else if (*pcur_sec + 1 < ptffs->pbs->sec_per_clus) {
		(*pcur_sec)++;
	}
	else {
		next_clus = fat_get_next_clus(pfat, *pcur_clus);
		if (next_clus == ERR_FAT_LAST_CLUS) {
			return FALSE;
		}
		DBG("%s(): next_clus = 0x%x\n", __FUNCTION__, next_clus);
		*pcur_clus = next_clus;
		*pcur_sec = 0;
	}

	return TRUE;
}

int32
fat_malloc_clus(
	IN	tfat_t * pfat,
	IN	uint32 cur_clus,
	OUT	uint32 * pnew_clus)
{
	uint32 new_clus;
	int32 ret;
	
	ASSERT(pfat && pnew_clus);

	if (cur_clus == FAT_INVALID_CLUS) {

		if ((ret = _lookup_free_clus(pfat, &new_clus)) != FAT_OK)
			return ret;
	}
	else {

		if ((ret = _lookup_free_clus(pfat, &new_clus)) != FAT_OK)
			return ret;

		if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
			return ERR_FAT_DEVICE_FAIL;

		_set_fat_entry(pfat, cur_clus, new_clus);

		if (!_write_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
			return ERR_FAT_DEVICE_FAIL;
	}

	*pnew_clus = new_clus;
	DBG("%s(): new clus = %d\n", __FUNCTION__, new_clus);
	return FAT_OK;
}

int32
fat_free_clus(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	uint32 last_clus;
	uint32 cur_clus;

	ASSERT(pfat);
	
	last_clus = clus;
	cur_clus = clus;
	if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
		return ERR_FAT_DEVICE_FAIL;

	while (1) {
		if (_clus2fatsec(pfat, cur_clus) != _clus2fatsec(pfat, last_clus)) {
			if (!_write_fat_sector(pfat, _clus2fatsec(pfat, last_clus)))
				return ERR_FAT_DEVICE_FAIL;
			if (!_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus)))
				return ERR_FAT_DEVICE_FAIL;
		}

		last_clus = cur_clus;
		cur_clus = _get_fat_entry(pfat, last_clus);
		if (_is_entry_eof(pfat, cur_clus)) {
			_set_fat_entry(pfat, last_clus, 0);
			break;
		}
		
		_set_fat_entry(pfat, last_clus, 0);
	}

	if (!_write_fat_sector(pfat, _clus2fatsec(pfat, last_clus)))
		return ERR_FAT_DEVICE_FAIL;

	return FAT_OK;
}

/*----------------------------------------------------------------------------------------------------*/

static uint32
_get_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	tffs_t * ptffs = pfat->ptffs;
	int16 entry_len;
	void * pclus;
	uint32 entry_val = 0;

	entry_len = _get_fat_entry_len(pfat);
	pclus = pfat->secbuf + (clus * entry_len / 8) % ptffs->pbs->byts_per_sec;
	
	if (ptffs->fat_type == FT_FAT12) {
		uint16 fat_entry;

		fat_entry = *((uint16 *)pclus);
		if (clus & 1) {
			entry_val = fat_entry >> 4;
		}
		else {
			entry_val = fat_entry & 0x0FFF;
		}
	}
	else if (ptffs->fat_type == FT_FAT16) {
		entry_val = *((uint16 *)pclus);
	}
	else if (ptffs->fat_type == FT_FAT32) {
		entry_val = *((uint32 *)pclus) & 0x0FFFFFFF;
	}

	return entry_val;
}

static void
_set_fat_entry(
	IN	tfat_t * pfat,
	IN	uint32 clus,
	IN	uint32 entry_val)
{
	tffs_t * ptffs = pfat->ptffs;
	int16 entry_len;
	void * pclus;

	entry_len = _get_fat_entry_len(pfat);
	pclus = pfat->secbuf + (clus * entry_len / 8) % ptffs->pbs->byts_per_sec;
	
	if (ptffs->fat_type == FT_FAT12) {
		uint16 fat12_entry_val = entry_val & 0xFFFF;

		if (clus & 1) {
			fat12_entry_val = fat12_entry_val << 4;
			*((uint16 *)pclus) = *((uint16 *)pclus) & 0x000F;
		}
		else {
			fat12_entry_val = fat12_entry_val & 0x0FFF;
			*((uint16 *)pclus) = *((uint16 *)pclus) & 0xF000;
		}
		*((uint16 *)pclus) = *((uint16 *)pclus) | fat12_entry_val;
	}
	else if (ptffs->fat_type == FT_FAT16) {
		*((uint16 *)pclus) = entry_val & 0xFFFF;
	}
	else if (ptffs->fat_type == FT_FAT32) {
		*((uint32 *)pclus) = *((uint32 *)pclus) & 0xF0000000;
		*((uint32 *)pclus) = *((uint32 *)pclus) | (entry_val & 0x0FFFFFFF);
	}
}

static uint16
_get_fat_entry_len(
	IN	tfat_t * pfat)
{
	tffs_t * ptffs = pfat->ptffs;
	uint16 entry_len = 0;

	switch (ptffs->fat_type) {
	case FT_FAT12:
		entry_len = 12;
		break;
	case FT_FAT16:
		entry_len = 16;
		break;
	case FT_FAT32:
		entry_len = 32;
		break;
	default:
		ASSERT(0);
	}
	return entry_len;
}

static BOOL
_read_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 fat_sec)
{
	tffs_t * ptffs = pfat->ptffs;

	//if (fat_sec == cur_fat_sec) 
	//	return TRUE;
	/* Marvell fixed */
#ifndef TFFS_FAT_CACHE
	if (HAI_readsector(ptffs->hdev, fat_sec, pfat->secbuf) != HAI_OK) {
#else
	if (cache_readsector(ptffs->pfatcache, fat_sec, pfat->secbuf) != CACHE_OK) {
#endif
		return FALSE;
	}

	if (ptffs->fat_type == FT_FAT12) {

	    /* This cluster access spans a sector boundary in the FAT      */
   		/* There are a number of strategies to handling this. The      */
    	/* easiest is to always load FAT sectors into memory           */
    	/* in pairs if the volume is FAT12 (if you want to load        */
    	/* FAT sector N, you also load FAT sector N+1 immediately      */
    	/* following it in memory unless sector N is the last FAT      */
    	/* sector). It is assumed that this is the strategy used here  */
    	/* which makes this if test for a sector boundary span         */
    	/* unnecessary.                                                */
	/* Marvell fixed */
#ifndef TFFS_FAT_CACHE
		if (HAI_readsector(ptffs->hdev, fat_sec + 1, 
				pfat->secbuf + ptffs->pbs->byts_per_sec) != HAI_OK) {
#else
		if (cache_readsector(ptffs->pfatcache, fat_sec + 1,
				pfat->secbuf + ptffs->pbs->byts_per_sec) != CACHE_OK) {
#endif
			return FALSE;
		}
	}

	pfat->cur_fat_sec = fat_sec;
	return TRUE;
}

static BOOL
_write_fat_sector(
	IN	tfat_t * pfat,
	IN	uint32 fat_sec)
{
	tffs_t * ptffs = pfat->ptffs;
#ifndef TFFS_FAT_CACHE
	if (HAI_writesector(ptffs->hdev, fat_sec, pfat->secbuf) != HAI_OK) {
#else
	if (cache_writesector(ptffs->pfatcache, fat_sec, pfat->secbuf) != CACHE_OK) {
#endif
		return FALSE;
	}
#ifdef TFFS_FAT_MIRROR
	fatm_write_sector(pfat, pfat->secbuf, fat_sec);
#endif

	if (ptffs->fat_type == FT_FAT12) {
#ifndef TFFS_FAT_CACHE
		if (HAI_writesector(ptffs->hdev, fat_sec + 1, 
				pfat->secbuf + ptffs->pbs->byts_per_sec) != HAI_OK) {
#else
		if (cache_writesector(ptffs->pfatcache, fat_sec + 1,
				pfat->secbuf + ptffs->pbs->byts_per_sec) != CACHE_OK) {
#endif
			return FALSE;
		}
#ifdef TFFS_FAT_MIRROR
	fatm_write_sector(pfat, pfat->secbuf + ptffs->pbs->byts_per_sec, fat_sec + 1);
#endif
	}
	return TRUE;
}

static uint32
_clus2fatsec(
	IN	tfat_t * pfat,
	IN	uint32 clus)
{
	return (pfat->ptffs->sec_fat + (clus * _get_fat_entry_len(pfat) / 8) / pfat->ptffs->pbs->byts_per_sec);
}

static BOOL
_is_entry_free(
	IN	tfat_t * pfat,
	IN	uint32 entry_val)
{
	tffs_t * ptffs = pfat->ptffs;

	if (ptffs->fat_type == FT_FAT12) {
		return !(entry_val & 0x0FFF);
	}
	else if (ptffs->fat_type == FT_FAT16) {
		return !(entry_val & 0xFFFF);
	}
	else if (ptffs->fat_type == FT_FAT32) {
		return !(entry_val & 0x0FFFFFFF);
	}

	ASSERT(0);
	return FALSE;
}

static BOOL
_is_entry_eof(
	IN	tfat_t * pfat,
	IN	uint32 entry_val)
{
	tffs_t * ptffs = pfat->ptffs;

	if (ptffs->fat_type == FT_FAT12) {
		return (entry_val & 0x0FFF) > 0x0FF8;
	}
	else if (ptffs->fat_type == FT_FAT16) {
		return (entry_val & 0xFFFF) > 0xFFF8;
	}
	else if (ptffs->fat_type == FT_FAT32) {
		return (entry_val & 0x0FFFFFFF) > 0x0FFFFFF8;
	}

	ASSERT(0);
	return FALSE;
}

/* Marvell fixed */
void TFFS_bind_nospace_handler(tffs_handle_t htffs,
	int32 (*p_func)(void *), void *arg)
{
	tffs_t * ptffs = (tffs_t *)htffs;
	ptffs->p_nospace_handler = p_func;
	ptffs->nospace_handler_arg = arg;
}

static int32
_lookup_free_clus(
	IN	tfat_t * pfat,
	OUT	uint32 * pfree_clus)
{
	tffs_t * ptffs = pfat->ptffs;
	uint32 cur_clus, prev_clus;
	int32 ret;
	
	ret = FAT_OK;
	cur_clus = pfat->last_free_clus;
	if (!_read_fat_sector(pfat, _clus2fatsec(pfat,pfat->last_free_clus)))
		return ERR_FAT_DEVICE_FAIL;

	while (1) {
		uint32 entry_val;
		int32 fetch;

		entry_val = _get_fat_entry(pfat, cur_clus);
		if (_is_entry_free(pfat, entry_val)) {

			_set_fat_entry(pfat, cur_clus, 0x0FFFFFFF);
			if (!_write_fat_sector(pfat, _clus2fatsec(pfat, cur_clus))) {
				ret = ERR_FAT_DEVICE_FAIL;
				break;
			}

			*pfree_clus = cur_clus;
			pfat->last_free_clus = cur_clus;
			break;
		}

		prev_clus = cur_clus++;
		/* Marvell fixed: overflow condition */
		if (cur_clus >= ptffs->total_clusters)
			cur_clus = 0;

		if (cur_clus == pfat->last_free_clus) {
			/* Marvell fixed: add external helper for no space case */
			if (!ptffs->p_nospace_handler
				|| ptffs->p_nospace_handler(ptffs->nospace_handler_arg)) {
				ret = ERR_FAT_NO_FREE_CLUSTER;
				break;
			}
			/* The helper was able to free some space:
				re-fetch and continue searching */
			fetch = 1;
		} else
			fetch = (_clus2fatsec(pfat, prev_clus) != _clus2fatsec(pfat, cur_clus));

		if (fetch && !_read_fat_sector(pfat, _clus2fatsec(pfat, cur_clus))) {
				ret = ERR_FAT_DEVICE_FAIL;
				break;
		}
	}

	return ret;
}


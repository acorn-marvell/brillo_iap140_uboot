/*
 * initfs.c
 *
 * TFFS_mount TFFS_mkfs implementation.
 * implementation file.
 *
 * Copyright (C) knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include "initfs.h"
#include "debug.h"
#include "tffs.h"
#include "hai.h"
#include "fat.h"
#include "dir.h"

#ifndef DBG_INITFS
#undef DBG
#define DBG nulldbg
#endif

int32
TFFS_mount(
    IN  byte * dev,
    OUT tffs_handle_t * phtffs)
{
	int32 ret;
	tffs_t * tffs;
	boot_sector_t * pbs = NULL;
	tdev_handle_t hdev;
	tdir_t * proot_dir = NULL;
	tdir_t * pcur_dir = NULL;
	uint32 rootdir_clus;
	tfat_t * pfat = NULL;
	tcache_t * pcache = NULL;
	
	if (!dev || !phtffs)
		return ERR_TFFS_INVALID_PARAM;

	ret = TFFS_OK;
	tffs = (tffs_t *)Malloc(sizeof(tffs_t));
	Memset(tffs, 0, sizeof(tffs_t));
	pbs = (boot_sector_t *)Malloc(sizeof(boot_sector_t));
	Memset(pbs, 0, sizeof(boot_sector_t));

	/* Marvell fixed: sizeof(boot_sector_t) is actualy 514 but only
		due to sub-struct size alignment to 32-bit.
		This only affects bs_sig offset, and it is unused 
	ASSERT(sizeof(boot_sector_t) == 512);
	*/
	
	hdev = HAI_initdevice(dev, 512);
	if (hdev == NULL) {
		ret = ERR_TFFS_DEVICE_FAIL;
		goto _release;
	}
	tffs->hdev = hdev;

	if (HAI_readsector(hdev, 0, (ubyte *)pbs) != HAI_OK) {
		ret = ERR_TFFS_DEVICE_FAIL;
		goto _release;
	}

	if (!_validate_bs(pbs)) {
		ret = ERR_TFFS_BAD_BOOTSECTOR;
		goto _release;
	}

	tffs->pbs = pbs;
	
	_parse_boot_sector(pbs, tffs);
	DBG("tffs->fat_type:%d\n", tffs->fat_type);
	DBG("tffs->sec_fat:%d\n", tffs->sec_fat);
	DBG("tffs->sec_root_dir:%d\n", tffs->sec_root_dir);
	DBG("tffs->sec_first_data:%d\n", tffs->sec_first_data);

	/* Marvell fixed */
#ifdef TFFS_FAT_CACHE
	if ((pcache = cache_init(hdev, TFFS_FAT_CACHE, tffs->pbs->byts_per_sec)) == NULL) {
		WARN("TFFS: cache is disable.\n");
		pcache = cache_init(hdev, 0, tffs->pbs->byts_per_sec);
	}
	tffs->pfatcache = pcache;
#endif
	if ((pfat = fat_init(tffs)) == NULL) {
		ret = ERR_TFFS_BAD_FAT;
		goto _release;
	}
	tffs->pfat = pfat;

	/* Marvell fixed: TFFS_FILE_CACHE instead of 32 hard-coded */
	if ((pcache = cache_init(hdev, TFFS_FILE_CACHE, tffs->pbs->byts_per_sec)) == NULL) {
		WARN("TFFS: cache is disable.\n");
		pcache = cache_init(hdev, 0, tffs->pbs->byts_per_sec);
	}
	tffs->pcache = pcache;

	rootdir_clus = tffs->fat_type == FT_FAT32 ? tffs->pbs->bh32.root_clus : ROOT_DIR_CLUS_FAT16;

	if (dir_init_by_clus(tffs, rootdir_clus, &proot_dir) != DIR_OK ||
		dir_init_by_clus(tffs, rootdir_clus, &pcur_dir) != DIR_OK) {
		ret = ERR_TFFS_DEVICE_FAIL;
		goto _release;
	}

	tffs->root_dir = proot_dir;
	tffs->cur_dir = pcur_dir;

	*phtffs = (tffs_handle_t)tffs;
	INFO("tiny fat file system mount OK.\n");

	return ret;

_release:
	if (pfat)
		fat_destroy(pfat);
	if (proot_dir)
		dir_destroy(proot_dir);
	if (pcur_dir)
		dir_destroy(pcur_dir);
	if (HAI_closedevice(tffs->hdev) != HAI_OK)
		ret = ERR_TFFS_DEVICE_FAIL;
	Free(pbs);
	Free(tffs);

	return ret;
}

int32
TFFS_umount(
	IN	tffs_handle_t  htffs)
{
	tffs_t * ptffs;
	int32 ret;

	if (!htffs)
		return ERR_TFFS_INVALID_PARAM;
	
	ptffs = (tffs_t *)htffs;
	ret = TFFS_OK;

	if (ptffs->pbs)
		Free(ptffs->pbs);

	if (ptffs->pfat)
		fat_destroy(ptffs->pfat);

	if (ptffs->root_dir)
		dir_destroy(ptffs->root_dir);
	
	if (ptffs->cur_dir)
		dir_destroy(ptffs->cur_dir);
	
	if (ptffs->pcache)
		ret = cache_destroy(ptffs->pcache);
	/* Marvell fixed */
#ifdef TFFS_FAT_CACHE
	if (ptffs->pfatcache)
		ret = cache_destroy(ptffs->pfatcache);
#endif
	if (ret > 0 && (HAI_closedevice(ptffs->hdev) != HAI_OK))
		ret = ERR_TFFS_DEVICE_FAIL;

	Free(ptffs);

	return ret;
}

int32
_validate_bs(
	IN	boot_sector_t * pbs)
{
	INFO("=================Boot sector====================\n");
	INFO("oem_name: %s\n", pbs->oem_name);
	INFO("byts_per_sec: %d\n", pbs->byts_per_sec);
	INFO("resvd_sec_cnt: %d\n", pbs->resvd_sec_cnt);
	INFO("sec_per_clus: %d\n", pbs->sec_per_clus);
	INFO("num_fats: %d\n", pbs->num_fats);
	INFO("================================================\n");
	/* Marvell fixed: check basic parameters are sane */
	if ((pbs->byts_per_sec == 0) || (pbs->sec_per_clus == 0) ||
		(pbs->num_fats < 1) || (pbs->num_fats > 2))
		return FALSE;
	return TRUE;
}

void
_parse_boot_sector(
	IN	boot_sector_t * pbs,
	OUT	tffs_t * tffs)
{
	uint32 totsec;
	uint32 count_of_clusters;
	uint32 datasec;
	
	tffs->root_dir_sectors = ((pbs->root_ent_cnt * 32) + (pbs->byts_per_sec - 1)) /  pbs->byts_per_sec;

	if (pbs->fatsz16 != 0) {
		tffs->fatsz = pbs->fatsz16;
	}
	else {
		tffs->fatsz = pbs->bh32.fatsz32;
	}

	if (pbs->tot_sec16 != 0) {
		totsec = pbs->tot_sec16;
	}
	else {
		totsec = pbs->tot_sec32;
	}

	datasec = totsec - (pbs->resvd_sec_cnt + (pbs->num_fats * tffs->fatsz) + tffs->root_dir_sectors);
	count_of_clusters = datasec / pbs->sec_per_clus;
	tffs->total_clusters = count_of_clusters;
	
	DBG("count_of_clusters = %d\n", count_of_clusters);

	//FIXME. I should find another to determine the fat type.
	if (count_of_clusters < 4085) {
		tffs->fat_type = FT_FAT12;
	}
	else if (count_of_clusters < 65525 && pbs->fatsz16 != 0){
		tffs->fat_type = FT_FAT16;
	}
	else {
		tffs->fat_type = FT_FAT32;
	}

	tffs->sec_fat = pbs->resvd_sec_cnt;
	tffs->sec_root_dir = tffs->sec_fat + pbs->num_fats * tffs->fatsz;
	tffs->sec_first_data = tffs->sec_root_dir + tffs->root_dir_sectors;
	/* Marvell fixed */
	tffs->num_fats = pbs->num_fats;
}


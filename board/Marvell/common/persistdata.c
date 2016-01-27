/*
 * bootctrl.c mrvl A/B boot support
 *
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Zhengxi Hu <zhxihu@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include "persistdata.h"

static unsigned int calc_checksum(PDATA_HEADER_t *pdh)
{
	int i = 0;
	unsigned chksum = 0;
	
	unsigned char *p = (unsigned char *)pdh;
	for(i = 0; i < pdh->length + sizeof(PDATA_HEADER_t); i++){
		chksum += p[i];
	}
	
	return chksum;
}

static int validate_pd(char *pd_buf)
{
	PDATA_HEADER_t *pdh = (PDATA_HEADER_t *)pd_buf;
	if(pdh->magic != PD_HEADER_MAGIC){
		printf("Invalid persist data!\n");
		return -1;
	}
	
	PDATA_BLOCK_t *pdbt = NULL;
	char *pdb = (char *)pdh + sizeof(PDATA_HEADER_t);
	int iter_count = 0;
	
	while(iter_count++ < PD_BLOCK_MAX){
		pdbt = (PDATA_BLOCK_t *)pdb;
		if(pdbt->type == PD_TYPE_CHECKSUM)
			break;
		pdb += (pdbt->length + sizeof(PDATA_BLOCK_t));
	}

	if(iter_count == PD_BLOCK_MAX && pdbt->type != PD_TYPE_CHECKSUM){
		printf("Max block count reached!\n");
		return -1;
	}	

	if(*(unsigned int *)(pdbt->data) != calc_checksum(pdh)){
		printf("Invalid persist data!, checksum:%x, expected:%x\n", calc_checksum(pdh), *(unsigned int *)(pdbt->data));
		return -1;
	}

	return 0;
}

/* Load persist data from emmc to mem */
int emmc_get_persist_data(void)
{
	char cmd[128];
	int ret = 0;

	block_dev_desc_t *dev_desc;
	int mmc_dev = CONFIG_FASTBOOT_FLASH_MMC_DEV;
	char *load_addr = (char *)(CONFIG_LOADADDR - PD_MMC_MAXSIZE);

	dev_desc = get_dev("mmc", mmc_dev);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("PD: invalid mmc device\n");
		return -1;
	}

	sprintf(cmd, "mmc dev %d %d", CONFIG_FASTBOOT_FLASH_MMC_DEV, PD_MMC_PART);
	run_command(cmd, 0);

	if (dev_desc->block_read(dev_desc->dev, PD_MMC_OFFSET/dev_desc->blksz, BLOCK_CNT(PD_MMC_MAXSIZE, dev_desc), load_addr) != BLOCK_CNT(PD_MMC_MAXSIZE, dev_desc)){
		error("read mmc data failed:\n");
		ret = -1;
		goto out;
	}

	if(validate_pd(load_addr) != 0){
		printf("warning, invalid primary persist data, loading from recovery.\n");
		sprintf(cmd, "mmc dev %d %d", CONFIG_FASTBOOT_FLASH_MMC_DEV, PD_MMC_PART_RECOVERY);
		run_command(cmd, 0);

		if (dev_desc->block_read(dev_desc->dev, PD_MMC_OFFSET/dev_desc->blksz, BLOCK_CNT(PD_MMC_MAXSIZE, dev_desc), load_addr) != BLOCK_CNT(PD_MMC_MAXSIZE, dev_desc)){
			error("read mmc data failed:\n");
			ret = -1;
			goto out;
		}

		if(validate_pd(load_addr) != 0){
			error("Persist data is invalid or corrupted, need repair!!!\n");
			ret = -1;
			goto out;
		}
	}

out:
	/* switch back to main part */
	sprintf(cmd, "mmc dev %d 0", CONFIG_FASTBOOT_FLASH_MMC_DEV);
	run_command(cmd, 0);
	
	return ret;
}

/* must be called after emmc_get_persist_data */
int mem_get_persist_data_block(int type, PDATA_BLOCK_t **pd)
{
	char *load_addr = (char *)(CONFIG_LOADADDR - PD_MMC_MAXSIZE);
	PDATA_BLOCK_t *pdbt = (PDATA_BLOCK_t *)(load_addr + sizeof(PDATA_HEADER_t));
	int found  = 0;
	char *pdb = load_addr + sizeof(PDATA_HEADER_t);

	*pd = NULL;
	
	while(1){
		pdbt = (PDATA_BLOCK_t *)pdb;
		if(pdbt->type == type){
			found = 1;
			break;
		}	
		if(pdbt->type == PD_TYPE_CHECKSUM)
			break;
		pdb += (pdbt->length + sizeof(PDATA_BLOCK_t));
	}

	if(found)
		*pd = pdbt;

	return found;
}



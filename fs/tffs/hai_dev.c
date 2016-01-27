/*
 * hai_dev.c
 *
 * Hardware Abstraction Interface for TFFS on uboot block device.
 * Interface definition per TFFS requirement, see hai.h by
 * knightray@gmail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <part.h>
#include "pubstruct.h"
#include "hai_dev.h"

/* This function is adopted from uboot/fs/fat/fat.c:fat_register_device() */
#define SECTOR_SIZE 512
#define DOS_PART_TBL_OFFSET     0x1be
#define DOS_PART_MAGIC_OFFSET   0x1fe
#define DOS_FS_TYPE_OFFSET      0x36 /* Marvell fixed: this is wrong */
#define DOS_FS_SS_OFFSET	0xb /* PBR bytes_per_sector offset */
#define MBR_BOOTABLE_OFFSET	0x1be

static int find_partition(struct tdev_t *pdev)
{
        unsigned char buffer[SECTOR_SIZE];
        disk_partition_t info;
	block_dev_desc_t *dev_desc = pdev->dev_desc;
	int part_no = pdev->part_offset;
        if (!dev_desc->block_read)
                return -1;

        /* check if we have a MBR (on floppies we have only a PBR) */
        if (dev_desc->block_read (dev_desc->dev, 0, 1, (ulong *) buffer) != 1) {
                printf ("** Can't read from device %d **\n", dev_desc->dev);
                return -1;
        }
        if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
                buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
                /* no signature found */
                return -1;
        }
        /* First we assume, there is a MBR */
	/* Marvell fixed: PBR has the same magic as MBR:
		add check for MBR "bootable" field (0 or 0x80 only);
		add check for PBR bytes_per_sector field */
	if (!get_partition_info(dev_desc, part_no, &info) && (buffer[MBR_BOOTABLE_OFFSET]&0x7f) == 0) {
		pdev->part_offset = info.start;
	} else if ((buffer[DOS_FS_SS_OFFSET] == 0) && (buffer[DOS_FS_SS_OFFSET + 1] == 2)) {
		/* Sector size 0x200 at offset 0xb: assume PBR */
		pdev->part_offset = 0;
        } else {
                printf ("** Partition %d not valid on device %d **\n",
                                part_no, dev_desc->dev);
                return -1;
        }
	printf("PART offset = %lx\n", pdev->part_offset);
	return 0;
}

tdev_handle_t
HAI_initdevice(
	IN	byte * dev,
	IN	int16 sector_size)
{
	struct tdev_t *pdev = (struct tdev_t *)dev;

	pdev->sector_size = sector_size;
	if (find_partition(pdev))
		return (tdev_handle_t)NULL;
	return (tdev_handle_t)dev;
}

int32
HAI_readsector(
	IN	tdev_handle_t hdev,
	IN	int32 addr,
	OUT	ubyte * ptr)
{
	struct tdev_t *pdev = (struct tdev_t *)hdev;

	if (!ptr || !pdev || !pdev->dev_desc || !pdev->dev_desc->block_read)
		return ERR_HAI_INVALID_PARAMETER;

	if (pdev->dev_desc->block_read(pdev->dev_desc->dev, addr+pdev->part_offset, 
		1, (unsigned long *)ptr)<0)
		return ERR_HAI_READ;
	return HAI_OK;
}

int32
HAI_writesector(
	IN	tdev_handle_t hdev,
	IN	int32 addr,
	IN	ubyte * ptr)
{
	struct tdev_t *pdev = (struct tdev_t *)hdev;

	if (!ptr || !pdev || !pdev->dev_desc || !pdev->dev_desc->block_write)
		return ERR_HAI_INVALID_PARAMETER;

	if (pdev->dev_desc->block_write(pdev->dev_desc->dev, addr+pdev->part_offset, 
		1, (unsigned long *)ptr)<0)
		return ERR_HAI_WRITE;
	return HAI_OK;
}

/* Marvell fixed */
int32
HAI_writesectors(
	IN	tdev_handle_t hdev,
	IN	int32 addr,
	IN	ubyte * ptr,
	IN	uint32 nsectors)
{
	struct tdev_t *pdev = (struct tdev_t *)hdev;

	if (!ptr || !pdev || !pdev->dev_desc || !pdev->dev_desc->block_write)
		return ERR_HAI_INVALID_PARAMETER;

	if (pdev->dev_desc->block_write(pdev->dev_desc->dev, addr+pdev->part_offset, 
		nsectors, (unsigned long *)ptr)<0)
		return ERR_HAI_WRITE;
	return HAI_OK;
}
int32
HAI_closedevice(
	IN	tdev_handle_t hdev)
{
	return HAI_OK;
}

int32
HAI_getdevinfo(
	IN	tdev_handle_t hdev,
	OUT	tdev_info_t * devinfo)
{
	struct tdev_t *pdev = (struct tdev_t *)hdev;

	if (!devinfo || !pdev)
		return ERR_HAI_INVALID_PARAMETER;

	devinfo->free_sectors = 1000;
	return HAI_OK;
}

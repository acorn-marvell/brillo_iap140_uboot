/*
 * bootctrl.c mrvl A/B boot support
 *
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Zhengxi Hu <zhxihu@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <errno.h>

#include <common.h>
#include <part.h>
#include <android_image.h>
#include "bootctrl.h"

#if 1
void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
#endif

// copy brillo_slot_info from boot control HAL
typedef struct brillo_slot_info {
	// Flag mean that the slot can bootable or not.
	uint8_t bootable :1;

	// Set to true when the OS has successfully booted.
	uint8_t boot_successful :1;

	// Priorty number range [0:15], with 15 meaning highest priortiy,
	// 1 meaning lowest priority and 0 mean that the slot is unbootable.
	uint8_t priority;

	// Boot times left range [0:7].
	uint8_t tries_remaining;

	uint8_t reserved[1];
} BrilloSlotInfo;

// copy BrilloBootInfo from boot control HAL
typedef struct BrilloBootInfo {
	// Magic for identification
	uint8_t magic[3];

	// Version of BrilloBootInfo struct, must be 0 or larger.
	uint8_t version;

	// Information about each slot.
	BrilloSlotInfo slot_info[SLOTS_NUM];

	uint8_t reserved[20];
} BrilloBootInfo;

// copy bootloader_message from recovery
typedef struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];

	// The 'recovery' field used to be 1024 bytes.  It has only ever
	// been used to store the recovery command line, so 768 bytes
	// should be plenty.  We carve off the last 256 bytes to store the
	// stage string (for multistage packages) and possible future
	// expansion.
	char stage[32];
	char slot_suffix[32];
	char reserved[192];
}bootloader_message;


bootloader_message bl_msg[2];
char bootslot_suffix[SLOTS_NUM][3] = {SLOT_HYPHEN  SLOT_A, SLOT_HYPHEN SLOT_B};
char bootslot_suffix_only[SLOTS_NUM][3] = { SLOT_A,  SLOT_B};
/* partition that supports slot, use "" as identifier of end of the array */
char partition_slot[][8] = {"boot", "system", ""}; 

static int bootctrl_get_metadata(void)
{
	block_dev_desc_t *dev_desc;
	disk_partition_t part;
	int mmc_dev = CONFIG_FASTBOOT_FLASH_MMC_DEV;

	memset(bl_msg, '\0', sizeof(bl_msg));

	dev_desc = get_dev("mmc", mmc_dev);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("bootctrl: invalid mmc device\n");
		return -1;
	}

	if (get_partition_info_efi_by_name(dev_desc, "misc", &part)) {
		printf("bootctrl: cannot find partition: 'misc'\n");
		return -1;
	}

	if (dev_desc->block_read(mmc_dev, part.start, 3, bl_msg) != 3) {
		printf("bootctrl: block read from misc failed\n");
		return -1;
	}

	/* save a copy of bl for comparision of bl data change */
	memcpy(bl_msg+1, bl_msg, sizeof(bootloader_message));

	//hexDump("bl_msg", bl_msg, sizeof(bl_msg));

	return 0;
}

static int bootctrl_set_metadata(void)
{
	block_dev_desc_t *dev_desc;
	disk_partition_t part;
	int mmc_dev = CONFIG_FASTBOOT_FLASH_MMC_DEV;

	dev_desc = get_dev("mmc", mmc_dev);
	if (!dev_desc || dev_desc->type == DEV_TYPE_UNKNOWN) {
		printf("bootctrl: invalid mmc device\n");
		return -1;
	}

	if (get_partition_info_efi_by_name(dev_desc, "misc", &part)) {
		printf("bootctrl: cannot find partition: 'misc'\n");
		return -1;
	}

	if(memcmp(bl_msg, bl_msg+1, sizeof(bootloader_message))){
		if (dev_desc->block_write(mmc_dev, part.start, sizeof(bl_msg)/512, bl_msg) != sizeof(bl_msg)/512) {
			printf("bootctrl: block read from misc failed\n");
			return -1;
		}
		printf("bootctrl: Updated misc data.\n");
	}

	return 0;
}

static int bootctrl_kernel_signature_verify(int slot)
{
	char cmd[128];
	struct andr_img_hdr *ahdr =
		(struct andr_img_hdr *)(CONFIG_LOADADDR - 0x1000 - 0x200);

	/* copy boot image and uImage header to parse */
	sprintf(cmd, "%s; mmc read %p %lx 0x9", CONFIG_MMC_BOOT_DEV, \
			ahdr, bootctrl_get_boot_addr(slot) / 512);
	run_command(cmd, 0);

	if (!memcmp(ANDR_BOOT_MAGIC, ahdr->magic, ANDR_BOOT_MAGIC_SIZE)) {
		printf("bootctrl: Android image is valid\n");
		return 0;
	} else {
		printf("bootctrl: Android image is invalid\n");
		return -1;
	}
}

/*
static int bootctrl_boot_partition_verify(int slot)
{
	return 0;
}
*/

static int bootctrl_metadata_verify(void)
{
	BrilloBootInfo *pbbi;

	pbbi = (BrilloBootInfo *)bl_msg[0].slot_suffix;
	if (pbbi->magic[0] != '\0' || pbbi->magic[1] != 'B' || pbbi->magic[2] != 'C')
		return -1;

	return 0;
}

static int bootctrl_metadata_setdefault(void)
{
	BrilloBootInfo *pbbi;

	pbbi = (BrilloBootInfo *)bl_msg[0].slot_suffix;
	memset((void *)pbbi, 0, 32);
	pbbi->magic[0] = '\0';
	pbbi->magic[1] = 'B';
	pbbi->magic[2] = 'C';
	pbbi->version = 0;

	return 0;
}

unsigned long bootctrl_get_boot_addr(int slot)
{
	if (slot == 1)
		return BOOTIMG_B_EMMC_ADDR;
	else
		return BOOTIMG_EMMC_ADDR;
}

char* bootctrl_get_boot_slot_suffix(int slot)
{
	if(slot > SLOTS_NUM || slot < 0)
		return NULL;
	return bootslot_suffix[slot];
}

char* bootctrl_get_boot_slot_suffix_only(int slot)
{
	if(slot > SLOTS_NUM || slot < 0)
		return "";
	return bootslot_suffix_only[slot];
}

int bootctrl_get_boot_slots_num(void)
{
	return SLOTS_NUM;
}

int bootctrl_set_active_slot(char *slot_suffix, char *response)
{
	int i,j;
	BrilloBootInfo *pbbi;
	BrilloSlotInfo *pbsi;
	
	strncpy(response, "FAIL", 4);

	if(bootctrl_get_metadata() != 0){
		strcat(response, "can not get bl data");
		return -1;
	}

	for(i= 0; i<SLOTS_NUM; i++){
		if(strcmp(bootslot_suffix[i], slot_suffix) == 0)
			break;
	}
	
	if(i == SLOTS_NUM){
		strcat(response, "invalid arguments");
		return -1;
	}

	if(bootctrl_metadata_verify() != 0){
		bootctrl_metadata_setdefault();
	}

	pbbi = (BrilloBootInfo *)bl_msg[0].slot_suffix;
	pbsi = pbbi->slot_info;
	pbsi += i;

	pbsi->bootable = 1;
	pbsi->boot_successful = 0;
	pbsi->priority  = 15;
	pbsi->tries_remaining = 7;

	pbsi = pbbi->slot_info;
	for(j= 0; j<SLOTS_NUM; j++){
		if(j == i)
			continue;
		pbsi += j;
		if(pbsi->priority == 15)
			pbsi->priority = 14;
	}
	
	printf("Set active slot to %d\n", i);

	bootctrl_set_metadata();

	strncpy(response, "OKAY", 4);
	return 0;
}

int bootctrl_get_active_slot(void)
{
	int i;
	BrilloBootInfo *pbbi;
	BrilloSlotInfo *pbsi;

	if(bootctrl_get_metadata() != 0){
		return -1;
	}

	pbbi = (BrilloBootInfo *)bl_msg[0].slot_suffix;
	pbsi = pbbi->slot_info;
	for(i= 0; i<SLOTS_NUM; i++){
		pbsi += i;
		if(pbsi->bootable == 1 && pbsi->priority  == 15){
			return i;
		}
	}

	return -1;
}

char* bootctrl_get_active_slot_suffix(void)
{
	return bootctrl_get_boot_slot_suffix(bootctrl_get_active_slot());
 }

int bootctrl_find_boot_slot()
{
	BrilloBootInfo *pbbi;
	BrilloSlotInfo *pbsi;

	int slot = -1;
	
	int i,j;

	int mark[SLOTS_NUM] = {1,1};
	int sorted_index[SLOTS_NUM] = {0};

	int hi_pri = 0;
	int hi_index = 0;

	if(bootctrl_get_metadata() != 0)
		return slot;

	if(bootctrl_metadata_verify()){
		/* todo: bypass the verify to allow first time boot go through */
		printf("Likely first time boot, do 'fastboot --set_active=_a' to init misc data\n");
		return slot;
	}
		
	pbbi = (BrilloBootInfo *)bl_msg[0].slot_suffix;

	printf("Slot suffix: ");
	for(i=0;i<SLOTS_NUM;i++){
		printf("%s,", bootctrl_get_boot_slot_suffix(i));
	}
	printf("\n");
	
	/* sort by priority, save sorted indexes */
	for(i=0;i<SLOTS_NUM;i++){
		pbsi = pbbi->slot_info;
		hi_pri = 0;
		hi_index =0;
		for(j =0; j<SLOTS_NUM; j++){
			if(mark[j]){
				pbsi += j;
				if(pbsi->priority >= hi_pri){
					hi_pri = pbsi->priority;
					hi_index = j;
				}
			}
		}
		sorted_index[i] = hi_index;
		mark[hi_index] = 0;
	}

#if 1
	/* debug of slot info */ 
	for(i =0; i<SLOTS_NUM; i++){
        	pbsi = pbbi->slot_info;
       		pbsi += sorted_index[i];
		printf("[%d]bootable:%d\n", sorted_index[i], pbsi->bootable);
		printf("[%d]boot_successful:%d\n", sorted_index[i], pbsi->boot_successful);
		printf("[%d]priority:%d\n", sorted_index[i], pbsi->priority);
		printf("[%d]tries_remaining:%d\n", sorted_index[i], pbsi->tries_remaining);
	}
#endif

	/* find bootable slot */
	for(i =0; i<SLOTS_NUM; i++){
		pbsi = pbbi->slot_info;
		pbsi += sorted_index[i];
		if(pbsi->priority){
			if(pbsi->boot_successful == 0 && pbsi->tries_remaining == 0){
				pbsi->priority = 0;
				printf("Set slot %d priority to 0\n", sorted_index[i]);
				continue;
			}
			if(pbsi->boot_successful == 0 && pbsi->tries_remaining){
				if(bootctrl_kernel_signature_verify(sorted_index[i])){
					pbsi->priority = 0;
					printf("Set slot %d priority to 0\n", sorted_index[i]);
					continue;
				}
				pbsi->tries_remaining--;
				slot = sorted_index[i];
				printf("Decrease slot %d tries_remaining to %d\n", slot, pbsi->tries_remaining);
				break;
			}
			if(pbsi->boot_successful){
				slot = sorted_index[i];
				printf("Booting good slot %d\n", slot);
				break;
			}
		}
	}

	if(slot>=0){
		bootctrl_set_metadata();
		printf("Booting from slot %d\n", slot);
	}
	
	return slot;
}

int bootctrl_is_partition_support_slot(char *partition)
{
	int i;
	for(i=0; partition_slot[i][0] != 0; i++){
		if(strcmp(partition, partition_slot[i]) == 0)
			return 1;
	}
	return 0;
}


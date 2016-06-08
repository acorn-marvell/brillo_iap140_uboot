/*
 * boot.c mrvl boot support
 *
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaofan Tian <tianxf@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#ifdef CONFIG_OBM_PARAM_ADDR
#include "obm2osl.h"
#endif
#include <mv_recovery.h>
#include "cmdline.h"
#include <android_image.h>
#include <image.h>
#include "bootctrl.h"
#include <mv_bvb.h>
#include <fb_flashing.h>
#include <bvb_mmc.h>

static unsigned char load_op;
struct recovery_reg_funcs *p_recovery_reg_funcs;
int recovery_key_detect_default(void)
{
	return 0;
}
int recovery_key_detect(void)
	__attribute__((weak, alias("recovery_key_detect_default")));
int recovery_default(int primary_valid, int recovery_valid,
		int magic_key, struct recovery_reg_funcs *funcs)
{
	return 0;
}
int recovery(int primary_valid, int recovery_valid,
		int magic_key, struct recovery_reg_funcs *funcs)
	__attribute__((weak, alias("recovery_default")));

#ifdef CONFIG_OF_LIBFDT
void handle_dtb_default(struct fdt_header *devtree)
{
}

void handle_dtb(struct fdt_header *devtree)
	__attribute__((weak, alias("handle_dtb_default")));

unsigned int dtb_offset_default(void)
{
	return 0;
}

unsigned int dtb_offset(void)
	__attribute__((weak, alias("dtb_offset_default")));
#endif

static int get_recovery_flag(void)
{
	int primary_valid = 1, recovery_valid = 1;
	int magic_key;
#ifdef CONFIG_OBM_PARAM_ADDR
	struct OBM2OSL *params = NULL;
#endif

	magic_key = recovery_key_detect();

#ifdef CONFIG_OBM_PARAM_ADDR
	params = (struct OBM2OSL *)(uintptr_t)(*(u32 *)CONFIG_OBM_PARAM_ADDR);

	if (!params || params->signature != OBM2OSL_IDENTIFIER) {
		printf("WARING: no obm parameters !!!\n");
		params = NULL;
	} else {
		magic_key |= (params->booting_mode == RECOVERY_MODE);

		primary_valid = params->primary.validation_status;
		recovery_valid = params->recovery.validation_status;
	}
#endif

	return recovery(primary_valid, recovery_valid,
			magic_key, p_recovery_reg_funcs);
}

#ifdef CONFIG_CMD_MMC
static void load_image_head(struct andr_img_hdr *ahdr,
		unsigned long base_emmc_addr)
{
	char cmd[128];

	/* copy boot image and uImage header to parse */
	sprintf(cmd, "%s; mmc read %p %lx 0x9", CONFIG_MMC_BOOT_DEV,
		ahdr, base_emmc_addr / 512);
	run_command(cmd, 0);
}

#ifdef CONFIG_OF_LIBFDT
static unsigned long dtb_emmc_lookup(struct andr_img_hdr *ahdr,
		unsigned long base_emmc_addr)
{
	unsigned int zimage_size = 0;
	u8 uhead_buf[16];
	unsigned long dtb_emmc_addr;
	unsigned int kernel_offset;

	/*
	 * DTBs are appended after uImage so need parse uImage header
	 * to get zImage length. Only the first 16 uImage header bytes
	 * are needed for this parse.
	 */
	kernel_offset = ahdr->page_size;
	memcpy(uhead_buf, (char *)ahdr + kernel_offset, 16);
	/*
	 * This zImage_size means the zImage without uboot_head size
	 * and we can get the size value from the uboot_head[12]~[15]
	 */
	zimage_size = (uhead_buf[12] << 24) | (uhead_buf[13] << 16) |
		(uhead_buf[14] << 8) | uhead_buf[15];

	dtb_emmc_addr = IH_HEADER_SIZE + kernel_offset + zimage_size;
	dtb_emmc_addr = ALIGN(dtb_emmc_addr, ahdr->page_size);
	dtb_emmc_addr += base_emmc_addr;
	dtb_emmc_addr += dtb_offset();

	return dtb_emmc_addr;
}
#endif

#define MV_KERNEL_LOADED	(1<<0)
#define MV_RAMDISK_LOADED	(1<<1)
#define MV_DTB_LOADED		(1<<2)
#define MV_DTB_FLASH		(1<<3) /* flash dtb to flash */

void image_load_notify(unsigned long load_addr)
{
	switch (load_addr) {
	case CONFIG_LOADADDR: /* same value as RECOVERY_KERNEL_LOADADDR */
		load_op |= MV_KERNEL_LOADED;
		break;
	case RAMDISK_LOADADDR: /* same value as RECOVERY_RAMDISK_LOADADDR */
		load_op |= MV_RAMDISK_LOADED;
		break;
#ifdef CONFIG_OF_LIBFDT
	case DTB_LOADADDR:
		load_op |= MV_DTB_LOADED;
		break;
#endif
	}
}

void image_flash_notify(unsigned long load_addr)
{
#ifdef CONFIG_OF_LIBFDT
	char cmd[128];
	unsigned long base_emmc_addr, dtb_load_addr, dtb_emmc_addr;
	int recovery_flag = 0;
	struct andr_img_hdr *ahdr =
		(struct andr_img_hdr *)(CONFIG_LOADADDR - 0x1000 - 0x200);

	/*load_op bit[3] flash dtb to mmc */
	if (load_op & MV_DTB_FLASH) {
		recovery_flag = get_recovery_flag();
		base_emmc_addr = recovery_flag ?
			RECOVERYIMG_EMMC_ADDR : BOOTIMG_EMMC_ADDR;

		/* Load DTB */
		dtb_load_addr = load_addr;
		load_image_head(ahdr, base_emmc_addr);
		dtb_emmc_addr = dtb_emmc_lookup(ahdr, base_emmc_addr) / 512;

		printf("write dtb to mmc 0x%lx\n", dtb_emmc_addr);
		sprintf(cmd, "%s; mmc write %lx %lx %x", CONFIG_MMC_BOOT_DEV,
			dtb_load_addr, dtb_emmc_addr, DTB_SIZE / 512);
		run_command(cmd, 0);
	}
#endif
}

int do_mrvlboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[128];
	char cmdline[COMMAND_LINE_SIZE] = {0};
	char *bootargs;
	unsigned int kernel_offset = 0, ramdisk_offset = KERNEL_SIZE;
	unsigned int kernel_size = KERNEL_SIZE, ramdisk_size = RAMDISK_SIZE;
	unsigned long base_emmc_addr, kernel_load_addr, ramdisk_load_addr;
	int recovery_flag = 0;
	int fastboot_flag = 0;
#ifdef CONFIG_OF_LIBFDT
	unsigned long dtb_load_addr, dtb_emmc_addr;
	struct fdt_header *devtree;
#endif
#ifdef CONFIG_MULTIPLE_SLOTS
	int slot;
#endif
	block_dev_desc_t *dev;
	disk_partition_t info;
	unsigned long recovery_image_addr;
#ifdef CONFIG_MV_BVB
	const uint8_t *out_key_data;
	size_t out_key_length;
	BvbVerifyResult bvb_result;
	struct BvbBootImageHeader *bhdr;
	unsigned long bvb_load_addr;
	char bvb_cmdline[BVB_KERNEL_CMDLINE_MAX_LEN] = {0};
	unsigned int device_state;
#endif

	/* The bootimg/recoveryimg header + uImage header ddr parse address */
	struct andr_img_hdr *ahdr =
		(struct andr_img_hdr *)(CONFIG_LOADADDR - 0x1000 - 0x200);

	recovery_flag = get_recovery_flag();
	fastboot_flag = fastboot_key_detect();

        dev = get_dev("mmc", CONFIG_FASTBOOT_FLASH_MMC_DEV);
        if(!dev){
                printf("Failed to get mmc device!\n");
                return -1;
        }
        /*Parse flash address from the partition name*/
        if (get_partition_info_efi_by_name(dev, "recovery", &info)){
                printf("Failed to get partition(recovery) info!\n");
                return -1;
        }
        recovery_image_addr = info.start * dev->blksz;

	/*
	 * OBM doesn't load kernel and ramdisk for nontrusted boot, but it
	 * still passes validation_status and loading_status as 1 to uboot
	 * since uboot is loaded and validated.
	 * Always load the kernel and ramdisk here to solve this problem,
	 * negative impact of a little boot time.
	 */
#ifdef CONFIG_MULTIPLE_SLOTS
	slot = bootctrl_find_boot_slot();
	if (slot == -1)
		recovery_flag = 1;

	base_emmc_addr = recovery_flag ? recovery_image_addr :
			bootctrl_get_boot_addr(slot);
#else
	if (get_partition_info_efi_by_name(dev, "boot", &info)) {
		printf("Failed to get partition(boot) info!\n");
		return -1;
	}
	unsigned long boot_image_addr = info.start * dev->blksz;

	base_emmc_addr = recovery_flag ? recovery_image_addr : boot_image_addr;
#endif

#ifdef CONFIG_MV_BVB
	bhdr = (struct BvbBootImageHeader *)malloc(sizeof(*bhdr));
	if (!bhdr) {
		printf("BVB: ERROR, Malloc for bvb header failed\n");
		return -1;
	}

	/* Load the header and the whole image */
	printf("Booting from BVB %s image\n", recovery_flag ? "recovery" : "boot");

	mv_bvb_load_header(bhdr, base_emmc_addr);
	if (memcmp(bhdr->magic, BVB_MAGIC, BVB_MAGIC_LEN) != 0) {
		printf("BVB: ERROR, Header Magic is incorrect.\n");
		// TODO set current slot not bootable
		free(bhdr);
		return -1;
	}
	bvb_load_addr = CONFIG_LOADADDR - mv_bvb_image_size(bhdr) - 0x200;
	mv_bvb_load_image(bvb_load_addr, base_emmc_addr, mv_bvb_image_size(bhdr));

	/* Verify the image */
	if (FB_FLASHING_DEVICE_LOCKED == (device_state = get_device_state(NULL))) {
		bvb_result = mv_bvb_verify(bvb_load_addr,
					   mv_bvb_image_size(bhdr),
					   &out_key_data, &out_key_length);
		if (!bvb_result && out_key_data) {
			bvb_debug("BVB: Verified successful, then compare the out key\n");
			if (!bvb_devkey_match(out_key_data, out_key_length)) {
				bvb_debug("BVB: DEVKEY is equal to the embedding KEY\n");
			} else {
				bvb_debug("BVB: ERROR, DEVKEY is not equal to the "
						"embedding KEY\n");
				if (FB_FLASHING_DEVICE_LOCKED == device_state) {
					free(bhdr);
					return -1;
				}
			}
		} else {
			switch (bvb_result) {
				case BVB_VERIFY_RESULT_OK_NOT_SIGNED:
					printf("BVB: WARNING, image is not signed\n");
					break;
				case BVB_VERIFY_RESULT_INVALID_BOOT_IMAGE_HEADER:
					printf("BVB: ERROR, verified failed, "
						"invalid Boot image header\n");
					break;
				case BVB_VERIFY_RESULT_HASH_MISMATCH:
					printf("BVB: ERROR, verified failed, hash mismatch\n");
					break;
				case BVB_VERIFY_RESULT_SIGNATURE_MISMATCH:
					printf("BVB: ERROR, verified failed, signature mismatch\n");
					break;
				default:
					printf("BVB: ERROR, verified failed, unknown error\n");
			}

			if (bvb_result > BVB_VERIFY_RESULT_OK_NOT_SIGNED) {
				printf("BVB: ERROR, stop booting\n");
				free(bhdr);
				return -1;
			}
		}
	}

	/* Load the kernel and the ramdisk */
	kernel_load_addr = bvb_load_addr + BVB_BOOT_IMAGE_HEADER_SIZE +
				bhdr->authentication_data_block_size +
				bhdr->auxilary_data_block_size;
	kernel_size = ALIGN(bhdr->kernel_size, 512);
	memcpy(CONFIG_LOADADDR, kernel_load_addr, kernel_size);

	ramdisk_load_addr = RAMDISK_LOADADDR;
	ramdisk_size = ALIGN(bhdr->initrd_size, 512);
	memcpy(ramdisk_load_addr, kernel_load_addr + bhdr->initrd_offset,
		   ramdisk_size);

	kernel_load_addr = CONFIG_LOADADDR;

#else
	load_image_head(ahdr, base_emmc_addr);

	if (!memcmp(ANDR_BOOT_MAGIC, ahdr->magic, ANDR_BOOT_MAGIC_SIZE)) {
		printf("This is Android %s image\n",
		       recovery_flag ? "recovery" : "boot");
		kernel_offset = ahdr->page_size;
		kernel_size = ALIGN(ahdr->kernel_size, ahdr->page_size);
		ramdisk_offset = kernel_offset + kernel_size;
		ramdisk_size = ALIGN(ahdr->ramdisk_size, ahdr->page_size);
	}

	/* calculate mmc block number */
	kernel_offset /= dev->blksz;
	ramdisk_offset /= dev->blksz;
	kernel_size /= dev->blksz;
	ramdisk_size /= dev->blksz;

	/* Load kernel */
	kernel_load_addr = CONFIG_LOADADDR;
	if ((load_op & MV_KERNEL_LOADED) == 0) {
		printf("Load kernel to 0x%lx\n", kernel_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", kernel_load_addr,
			base_emmc_addr / 512 + kernel_offset, kernel_size);
		run_command(cmd, 0);
	}

	/* Load ramdisk */
	ramdisk_load_addr = RAMDISK_LOADADDR;
	if ((load_op & MV_RAMDISK_LOADED) == 0) {
		printf("Load ramdisk to 0x%lx\n", ramdisk_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", ramdisk_load_addr,
			base_emmc_addr / 512 + ramdisk_offset, ramdisk_size);
		run_command(cmd, 0);
	}
#endif
	/* Modify ramdisk command line */
	bootargs = getenv("bootargs");
	strncpy(cmdline, bootargs, COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, "initrd=");
	snprintf(cmdline + strlen(cmdline), sizeof(cmdline) - strlen(cmdline),
			" initrd=0x%lx,10m rw", ramdisk_load_addr);

	if (recovery_flag)
		snprintf(cmdline + strlen(cmdline), sizeof(cmdline) - strlen(cmdline),
				" recovery=1");

#ifdef CONFIG_MULTIPLE_SLOTS
	if (slot >= 0)
		snprintf(cmdline + strlen(cmdline), sizeof(cmdline) - strlen(cmdline),
				" androidboot.slot_suffix=%s",
				bootctrl_get_boot_slot_suffix(slot));
#endif

#ifdef CONFIG_MV_BVB
	/* Append the options to command line */
	strncpy(bvb_cmdline, cmdline, COMMAND_LINE_SIZE);
	// TODO Append PARTUUID, HASH, and salt
	/*
	snprintf(bvb_cmdline + strlen(bvb_cmdline), sizeof(bvb_cmdline),
			" androidboot.secure=1 dm=\"1 vroot none ro 1,0 158136 " \
			"verity 1"
			"PARTUUID=$(ANDROID_SYSTEM_PARTUUID) PARTUUID=$(ANDROID_SYSTEM_PARTUUID) " \
			"4096 4096 19767 19767 sha1 " \
			"3b97752913fd33ab924e4385ffe249bf32c1d2b2 " \
			"5f96c19a519c01a9aa881c43044fe8a9e9b967be\"");
	*/
	setenv("bootargs", bvb_cmdline);
#else
	setenv("bootargs", cmdline);
#endif

#ifdef CONFIG_OF_LIBFDT
	/* Load DTB */
	dtb_load_addr = DTB_LOADADDR;
	if ((load_op & MV_DTB_LOADED) == 0) {
		dtb_emmc_addr = dtb_emmc_lookup(ahdr, base_emmc_addr);
		printf("Load dtb to 0x%lx\n", dtb_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", dtb_load_addr,
			dtb_emmc_addr / 512, DTB_SIZE / 512);
		run_command(cmd, 0);
	}

	devtree = (struct fdt_header *)dtb_load_addr;
	if (be32_to_cpu(devtree->magic) == FDT_MAGIC) {
		handle_dtb(devtree);
		sprintf(cmd, "fdt addr 0x%lx; bootm 0x%lx - 0x%lx",
			dtb_load_addr, kernel_load_addr, dtb_load_addr);
	} else
#endif
/* #if !defined(CONFIG_CMD_BOOTZ) || defined(CONFIG_OF_LIBFDT) */
#ifndef CONFIG_BOOTZIMAGE
		snprintf(cmd, sizeof(cmd), "bootm 0x%lx", kernel_load_addr);
#else
		/* zImage-dtb option */
		snprintf(cmd, sizeof(cmd), "bootz 0x%lx", kernel_load_addr);
#endif

	if (fastboot_flag) {
		printf("mrvlboot, fastboot mode\n");
		fastboot_key_set_led();
		run_command("fb", 0);
		return 0;
	} else
		printf("mrvlboot, normal mode\n");

#ifdef CONFIG_MV_BVB
	/* Enable power-on write protection when the device is LOCKED */
	if (FB_FLASHING_DEVICE_LOCKED == device_state) {
		bvb_debug("BVB: Enable PWR write protect\n");
		mv_bvb_power_on_wp_set(CONFIG_FASTBOOT_FLASH_MMC_DEV);
	}
	free(bhdr);
	bhdr = NULL;
#endif
	run_command(cmd, 0);

	return 0;
}

#ifdef CONFIG_OF_LIBFDT
int do_dtb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[128];
	unsigned long base_emmc_addr, dtb_load_addr, dtb_emmc_addr;
	int recovery_flag = 0;
	struct andr_img_hdr *ahdr =
		(struct andr_img_hdr *)(CONFIG_LOADADDR - 0x1000 - 0x200);

	if (argc > 3)
		return cmd_usage(cmdtp);

	if (argc == 1) {
		recovery_flag = get_recovery_flag();
		/* Load DTB from emmc */
		dtb_load_addr = DTB_LOADADDR;
		base_emmc_addr = recovery_flag ?
			RECOVERYIMG_EMMC_ADDR : BOOTIMG_EMMC_ADDR;
		load_image_head(ahdr, base_emmc_addr);
		dtb_emmc_addr = dtb_emmc_lookup(ahdr, base_emmc_addr) / 512;
		printf("Load dtb to 0x%lx\n", dtb_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", dtb_load_addr,
				dtb_emmc_addr, DTB_SIZE / 512);
		run_command(cmd, 0);

		load_op |= MV_DTB_LOADED;
		return 0;
	}
	/*load_op bit[3] flash dtb to mmc */
	if ((argc == 3) && !strcmp(argv[1], "b"))
		load_op |= MV_DTB_FLASH;

	sprintf(cmd, "tftpboot 0x%x %s",
		DTB_LOADADDR, argc == 3 ? argv[2] : argv[1]);
	run_command(cmd, 0);

	return 0;
}

U_BOOT_CMD(
	dtb, 3, 1, do_dtb,
	"Load dtb files through tftp or emmc",
	"Usage:\ndtb  load dtb file from emmc, then mrvlboot sequence would skip dtb load"
	"\ndtb <dtb_file> or dtb d <dtb_filename> to download dtb only"
	"\ndtb b <dtb_filename> to download dtb and flash"
);
#endif
#else
int do_mrvlboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}
#endif

U_BOOT_CMD(
	mrvlboot, 1, 1, do_mrvlboot,
	"Do marvell specific boot",
	"Usage:\nmrvlboot"
);

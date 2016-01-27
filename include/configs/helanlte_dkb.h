/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_HELANLTE_DKB_H
#define __CONFIG_HELANLTE_DKB_H

/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell HELAN DKB"
/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7                    1       /* ARMV7 cpu family */
#define CONFIG_PXA988			1	/* SOC Family Name */
#define CONFIG_PXA1088			1	/* SOC Family Name */
#define CONFIG_PXA1L88			1	/* SOC Family Name */
#define CONFIG_MACH_HELANLTE_DKB	1	/* Machine type */
#define CONFIG_POWER			1
#define CONFIG_POWER_I2C		1

#define CONFIG_POWER_88PM820		1
#define CONFIG_POWER_88PM830		1

#define CONFIG_SYS_SDRAM_BASE           0
#define CONFIG_SYS_TEXT_BASE		0x09000000
#define CONFIG_SYS_RELOC_END		0x09800000
#define CONFIG_SYS_INIT_SP_ADDR		(0xd1000000 + 0x1000)
#define CONFIG_NR_DRAM_BANKS_MAX	2
#define CONFIG_PXA988_POWER		1

#define CONFIG_ASSEMBLE_DCACHE_FLUSH

#ifdef CONFIG_TZ_HYPERVISOR
#define CONFIG_TZ_HYPERVISOR_SIZE	(0x01000000)
#else
#define CONFIG_TZ_HYPERVISOR_SIZE	0
#endif

#ifndef CONFIG_TZ_HYPERVISOR
#define CONFIG_ARM_ERRATA_802022
#endif

#define CONFIG_PXA_AMP_SUPPORT		1

#ifdef CONFIG_PXA_AMP_SUPPORT
#define CONFIG_NR_CPUS			4
#define CONFIG_SYS_AMP_RELOC_END	{0x10000000, 0x11000000, 0x12000000}
#endif
/*
 * Commands configuration
 */
#define CONFIG_SYS_NO_FLASH		/* Declare no flash (NOR/SPI) */
#include <config_cmd_default.h>
#define CONFIG_CMD_I2C
#define CONFIG_CMD_GPIO

#define CONFIG_CMD_MMC

#define CONFIG_USB_ETHER
#define CONFIG_MRVL_USB_PHY
#define CONFIG_MRVL_USB_PHY_40LP
#define CONFIG_USB_ETH_CDC
#define CONFIG_CMD_FASTBOOT
#define CONFIG_CMD_UNSPARSE

#define CONFIG_MMP_DISP
#ifdef CONFIG_MMP_DISP
#define CONFIG_CMD_BMP		1
#define CONFIG_VIDEO		1
#define CONFIG_CFB_CONSOLE	1
#define VIDEO_KBD_INIT_FCT	-1
#define VIDEO_TSTC_FCT		serial_tstc
#define VIDEO_GETC_FCT		serial_getc
#define LCD_RST_GPIO		1
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(262144)
#define CONFIG_LG_720P
#endif

#define CONFIG_RAMDUMP
#define CONFIG_RAMDUMP_TFFS
/*
 * Options (parameters below):
 *             #define CONFIG_RAMDUMP_TFFS
 *             #define CONFIG_RAMDUMP_YAFFS2
 *             #define CONFIG_RD_RAM
 */
#ifdef CONFIG_RAMDUMP
#if defined(CONFIG_RAMDUMP_TFFS)
#define CONFIG_FS_TFFS
#define CONFIG_CMD_MMC
#define CONFIG_RAMDUMP_MMC_DEV 0
#elif defined(CONFIG_RAMDUMP_YAFFS2)
#define CONFIG_YAFFS2
#define CONFIG_NAND_PART_SIZE  0
#define CONFIG_NAND_PART_OFFSET        0
#elif defined(CONFIG_RD_RAM)
#endif
#define CONFIG_GZIP_COMPRESSED
#endif
/*
 * mv-common.h should be defined after CMD configs since it used them
 * to enable certain macros
 */
#include "mv-common.h"

#undef CONFIG_ARCH_MISC_INIT
#define CONFIG_L2_OFF

/*
 * Boot setting
 */
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_SHOW_BOOT_PROGRESS
#define CONFIG_BOOTDELAY                3

#define CONFIG_BOOTARGS			\
			"androidboot.console=ttyS1" \
			" console=ttyS1,115200 panic_debug uart_dma" \
			" vmalloc=0x10000000 crashkernel=4k@0x8140000 RDCA=0x8140400 hwdfc=1"

#define CONFIG_BOOTCOMMAND	"mrvlboot"

#define CONFIG_MMC_BOOT_DEV	"mmc dev 2 0"

#define RAMDISK_LOADADDR	(CONFIG_TZ_HYPERVISOR_SIZE + 0x02000000)

#define BOOTIMG_EMMC_ADDR	0x01000000
#define RECOVERYIMG_EMMC_ADDR	0x00500000
#define KERNEL_SIZE		0x00600000
#define RAMDISK_SIZE		0x00400000
#define RECOVERY_KERNEL_LOADADDR	CONFIG_LOADADDR
#define RECOVERY_RAMDISK_LOADADDR	RAMDISK_LOADADDR

#define CONFIG_MRVL_BOOT	1

#define CONFIG_OF_LIBFDT	1

#ifdef CONFIG_OF_LIBFDT
#define DTB_LOADADDR		(CONFIG_TZ_HYPERVISOR_SIZE + 0x000E0000)
#define DTB_EMMC_ADDR		(BOOTIMG_EMMC_ADDR + 0xF00000)
#define RECOVERY_DTB_EMMC_ADDR	(RECOVERYIMG_EMMC_ADDR + 0x00900000)
#define DTB_SIZE		0x00020000
#endif

/*
 * Environment variables configurations
 */
#define CONFIG_ENV_SIZE		0x8000		/* 32k */

#ifndef CONFIG_CMD_SAVEENV
#define CONFIG_CMD_SAVEENV	1
#endif
#define CONFIG_ENV_OVERWRITE	1

/* The begin addr where to save u-boot.bin in the eMMC/Nand */
#define CONFIG_UBOOT_PA_START	0xF00000
/* The total size can be used for uboot and env in the eMMC/Nand */
#define CONFIG_UBOOT_PA_SIZE	0x100000
#define CONFIG_UBOOT_PA_END		(CONFIG_UBOOT_PA_START + CONFIG_UBOOT_PA_SIZE)
#define CONFIG_ENV_OFFSET		(CONFIG_UBOOT_PA_END - CONFIG_ENV_SIZE)

#ifdef CONFIG_CMD_MMC
#define CONFIG_ENV_IS_IN_MMC	1
#define CONFIG_SYS_MMC_ENV_DEV	2
#else
#define CONFIG_ENV_IS_NOWHERE	1
#endif

#define CONFIG_MISC_INIT_R	1

/* Marvell PXAV3 MMC Controller Configuration */
#define CONFIG_SDHCI_PXAV3	1

/* 11 levels svc */
#define CONFIG_FINE_TUNED_SVC

#endif	/* __CONFIG_HELANLTE_DKB_H */

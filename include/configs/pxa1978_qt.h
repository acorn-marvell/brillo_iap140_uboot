/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_PXA1978_QT_H
#define __CONFIG_PXA1978_QT_H

/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell PXA1978 QT"
/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV7                    1       /* ARMV7 cpu family */
#define CONFIG_PXA1978			1	/* SOC Family Name */

#define CONFIG_SYS_SDRAM_BASE           0
#define CONFIG_SYS_TEXT_BASE		0x09000000
#define CONFIG_SYS_RELOC_END		0x09800000
#define CONFIG_NO_RELOCATION

#define CONFIG_SYS_INIT_SP_ADDR		(0xf7100000 + 0x1000)
#define CONFIG_NR_DRAM_BANKS_MAX	2

#define CONFIG_ASSEMBLE_DCACHE_FLUSH

#ifdef CONFIG_TZ_HYPERVISOR
#define CONFIG_TZ_HYPERVISOR_SIZE	(0x00800000)
#else
#define CONFIG_TZ_HYPERVISOR_SIZE	(0x00800000) /* keep memory map */
#endif

/*
 * Commands configuration
 */
#define CONFIG_SYS_NO_FLASH		/* Declare no flash (NOR/SPI) */
#include <config_cmd_default.h>

#define CONFIG_CMD_GPIO

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
#undef CONFIG_BAUDRATE
#define CONFIG_BAUDRATE 460800
#undef CONFIG_CMD_NET
/*
 * Boot setting
 */
#define CONFIG_SHOW_BOOT_PROGRESS
#define CONFIG_ZERO_BOOTDELAY_CHECK
#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY		0
#define CONFIG_BOOTARGS \
			"console=ttyS1,460800 uart_dma" \
			" earlyprintk=uart8250-32bit,0xf7218000" \
			" panic_debug mem=256M user_debug=31 RDCA=08140400" \
			" root=/dev/ram0 rw rdinit=/bin/sh"
#define CONFIG_BOOTCOMMAND	"bootm 0x00807fc0 - 0x2000000"

#define CONFIG_MMC_BOOT_DEV	"mmc dev 2 0"

#define RAMDISK_LOADADDR	(CONFIG_TZ_HYPERVISOR_SIZE + 0x02000000)

#define BOOTIMG_EMMC_ADDR	0x01000000
#define RECOVERYIMG_EMMC_ADDR	0x00500000
#define KERNEL_SIZE		0x00600000
#define RAMDISK_SIZE		0x00400000
#define RECOVERY_KERNEL_LOADADDR	CONFIG_LOADADDR
#define RECOVERY_RAMDISK_LOADADDR	RAMDISK_LOADADDR

#ifndef CONFIG_QT
#define CONFIG_MRVL_BOOT	1
#endif

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
#define CONFIG_UBOOT_PA_END	(CONFIG_UBOOT_PA_START + CONFIG_UBOOT_PA_SIZE)
#define CONFIG_ENV_OFFSET	(CONFIG_UBOOT_PA_END - CONFIG_ENV_SIZE)

#ifdef CONFIG_CMD_MMC
#define CONFIG_ENV_IS_IN_MMC	1
#define CONFIG_SYS_MMC_ENV_DEV	2
#else
#define CONFIG_ENV_IS_NOWHERE	1
#endif

#define CONFIG_MISC_INIT_R	1

/* Marvell PXAV3 MMC Controller Configuration */
#define CONFIG_SDHCI_PXAV3	1

#endif	/* __CONFIG_PXA1978_QT_H */

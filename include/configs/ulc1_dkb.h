/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_ULC1_DKB_H
#define __CONFIG_ULC1_DKB_H

#define CONFIG_ARM64
#define CONFIG_REMAKE_ELF

/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell PXA1908 DKB"
/*
 * High Level Configuration Options
 */
#define CONFIG_ARMV8
/* Generic Timer Definitions for arm timer */
#define COUNTER_FREQUENCY		26000000     /* 26MHz */
#define SECONDARY_CPU_MAILBOX		(CONFIG_TZ_HYPERVISOR_SIZE + 0x10000)

#define CPU_RELEASE_ADDR		0xffffffff /* should not be touched */

#define CONFIG_PXA988			1	/* SOC Family Name */
#define CONFIG_PXA1088			1	/* SOC Family Name */
#define CONFIG_PXA1L88			1	/* SOC Family Name */
#define CONFIG_PXA1U88			1	/* SOC Family Name */
#define CONFIG_MACH_ULC1_DKB		1	/* Machine type */

#ifndef CONFIG_MMP_FPGA
#define CONFIG_POWER			1
#define CONFIG_POWER_I2C		1
#define CONFIG_POWER_88PM886		1
#define CONFIG_POWER_88PM88X_USE_SW_BD	1
#define CONFIG_POWER_88PM88X_GPADC_NO	3
#define CONFIG_POWER_OFF_CHARGE		1
#endif

#define CONFIG_SYS_SDRAM_BASE           0
#define CONFIG_SYS_TEXT_BASE		0x09000000
#define CONFIG_SYS_RELOC_END		0x09400000
#define CONFIG_SYS_INIT_SP_ADDR		(0xd1000000 + 0x1000)
#define CONFIG_NR_DRAM_BANKS_MAX	2

#define CONFIG_ASSEMBLE_DCACHE_FLUSH

#define CONFIG_SMP

#define CONFIG_PXA_AMP_SUPPORT		1

#ifdef CONFIG_PXA_AMP_SUPPORT
#define CONFIG_NR_CPUS			4
#define CONFIG_SYS_AMP_RELOC_END       {0x10000000, 0x11000000, 0x12000000}
#else
#define CONFIG_NR_CPUS			1
#endif

/*
 * Commands configuration
 */
#define CONFIG_SYS_NO_FLASH		/* Declare no flash (NOR/SPI) */
#include <config_cmd_default.h>
#define CONFIG_CMD_I2C
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_MMC

#define CONFIG_BOARD_EARLY_INIT_F

#define CONFIG_USB_ETHER
#define CONFIG_MRVL_USB_PHY
#define CONFIG_MRVL_USB_PHY_28LP
#define CONFIG_USB_ETH_CDC
#define CONFIG_CMD_UNSPARSE
#define CONFIG_CMD_FASTBOOT
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_USB_FASTBOOT_BUF_ADDR   (CONFIG_SYS_LOAD_ADDR - 2048)
#define CONFIG_SYS_CACHELINE_SIZE 64
#define CONFIG_USB_GADGET
#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_USB_GADGET_VBUS_DRAW     2
#define CONFIG_G_DNL_VENDOR_NUM         0x18d1
#define CONFIG_G_DNL_PRODUCT_NUM        0x4e11
#define CONFIG_G_DNL_MANUFACTURER       "Marvell"
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV   2

#define CONFIG_MMP_DISP
#ifdef CONFIG_MMP_DISP
#define CONFIG_CMD_BMP		1
#define CONFIG_VIDEO		1
#define CONFIG_VDMA		1
#define CONFIG_CFB_CONSOLE	1
#define VIDEO_KBD_INIT_FCT	-1
#define VIDEO_TSTC_FCT		serial_tstc
#define VIDEO_GETC_FCT		serial_getc
#define LCD_RST_GPIO		1
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(262144)
#define CONFIG_LG_720P
#define FRAME_BUFFER_ADDR	0x17000000
#endif

#ifndef CONFIG_MMP_FPGA
/* #define CONFIG_RAMDUMP */
/* #define CONFIG_RAMDUMP_TFFS */
#define CONFIG_HELAN2_POWER 1
#endif
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
#define CONFIG_BOOTDELAY                0

#define CONFIG_BOOTARGS			\
			"androidboot.console=ttyS1" \
			" console=ttyS1,115200 panic_debug uart_dma" \
			" crashkernel=4k@0x8140000 user_debug=31" \
			" earlyprintk=uart8250-32bit,0xd4017000" \
			" androidboot.lcd=720p cma=20M" \
			" cgroup_disable=memory ddr_mode=2 RDCA=08140400 cpmem=0K@0xffffffff" \
			" androidboot.exist.cp=0 androidboot.hardware=iap140"

#define CONFIG_BOOTCOMMAND	"mrvlboot"

#define CONFIG_MMC_BOOT_DEV	"mmc dev 2 0"

#define RAMDISK_LOADADDR	(CONFIG_TZ_HYPERVISOR_SIZE + 0x02000000)

#define BOOTIMG_EMMC_ADDR	0x01780000
#define BOOTIMG_B_EMMC_ADDR	0x15EFE0000
#define RECOVERYIMG_EMMC_ADDR	0x00580000
#define KERNEL_SIZE	0x00600000
#define RAMDISK_SIZE	0x00400000
#define RECOVERY_KERNEL_LOADADDR	CONFIG_LOADADDR
#define RECOVERY_RAMDISK_LOADADDR	RAMDISK_LOADADDR
#define MISC_EMMC_ADDR			0x42780000

#define CONFIG_MULTIPLE_SLOTS 1

#define CONFIG_MRVL_BOOT	1

#define CONFIG_BOOTZIMAGE 1

#define CONFIG_PERSIST_DATA 1

#ifndef CONFIG_BOOTZIMAGE
// Disabled to use original DTB appended to zImage
#define CONFIG_OF_LIBFDT	1
#endif

#define CONFIG_CMD_BOOTZ

#ifdef CONFIG_OF_LIBFDT
#define DTB_LOADADDR		0x9800000
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

#define IH_ARCH_ALTERNATE IH_ARCH_ARM
/* The begin addr where to save u-boot.bin in the eMMC/Nand */
#define CONFIG_UBOOT_PA_START	0x1680000
/* The total size can be used for uboot and env in the eMMC/Nand */
#define CONFIG_UBOOT_PA_SIZE	0x100000
#define CONFIG_UBOOT_PA_END		(CONFIG_UBOOT_PA_START + CONFIG_UBOOT_PA_SIZE)
#define CONFIG_ENV_OFFSET		(CONFIG_UBOOT_PA_END - CONFIG_ENV_SIZE)
#ifdef IH_ARCH_ALTERNATE
#define CONFIG_ENV_RE_OFFSET	0x578000
#endif

#ifdef CONFIG_CMD_MMC
#define CONFIG_ENV_IS_IN_MMC	1
#define CONFIG_SYS_MMC_ENV_DEV	2
#else
#define CONFIG_ENV_IS_NOWHERE	1
#endif

#define CONFIG_MISC_INIT_R	1

/* Marvell PXAV3 MMC Controller Configuration */
#define CONFIG_SDHCI_PXAV3	1

#define CONFIG_EEPROM_34XX02	1
/* Enable PXA1908 lpm test support */
#define CONFIG_PXA1908_LPM
/* debug feature support */
#define CONFIG_SQU_DEBUG_SUPPORT 1

#define CONFIG_NR_CPUS			4
#define CONFIG_CPU_OFFSET		1
#define CONFIG_NR_CLUS			1
#define CONFIG_MV_ETB_DMP_ADDR		0x09ff0000
#define CONFIG_MV_ETM_REG_ADDR0		0xd411c000
#define CONFIG_MV_ETB_REG_ADDR0		0xd410a000

#endif	/* __CONFIG_ULC1_DKB_H */

/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_PXA1928_H
#define __CONFIG_PXA1928_H

#define CONFIG_ARM64
#define CONFIG_REMAKE_ELF
/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell-PXA1928 Platform"

/*
 * High Level Configuration Options
 */
#define CONFIG_PXA1928		1

#define CONFIG_POWER			1
#define CONFIG_POWER_I2C		1
#define CONFIG_POWER_88PM860		1
#define CONFIG_POWER_88PM830		1
#define CONFIG_POWER_OFF_CHARGE		1

#define CONFIG_ARMV8
/* Generic Timer Definitions for arm timer */
#define COUNTER_FREQUENCY              (0x340000)     /* 3.25MHz */
#define CONFIG_SYS_HZ                  1000
#define SECONDARY_CPU_MAILBOX           0x01210000

#define CPU_RELEASE_ADDR		0xffffffff /* should not be touched */

#define CONFIG_CHECK_DISCRETE

#define CONFIG_SMP

#define CONFIG_SYS_SDRAM_BASE           0
#define CONFIG_SYS_TEXT_BASE            0x9000000
#define CONFIG_SYS_RELOC_END            0x09400000
#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SRAM_BASE + 0x1000)
#define CONFIG_NR_DRAM_BANKS_MAX	2

#ifdef CONFIG_TZ_HYPERVISOR
#define CONFIG_TZ_HYPERVISOR_SIZE       (0x01000000)
#else
#define CONFIG_TZ_HYPERVISOR_SIZE       0
#endif

#define CONFIG_PXA_AMP_SUPPORT		1

#ifdef CONFIG_PXA_AMP_SUPPORT
#define CONFIG_NR_CPUS			4
#define CONFIG_SYS_AMP_RELOC_END	{0x10000000, 0x11000000, 0x12000000}
#endif

#define CONFIG_DISPLAY_REBOOTINFO

#define CONFIG_ETM_POWER
#define CONFIG_NR_CPUS			4
#define CONFIG_CPU_OFFSET		1
#define CONFIG_NR_CLUS			1
#define CONFIG_MV_ETB_DMP_ADDR		0x09ff0000
#define CONFIG_MV_ETM_REG_ADDR0		0xd415c000
#define CONFIG_MV_ETB_REG_ADDR0		0xd4190000

/*
 * Marvell RAMDUMP
 */
#define EMMD_INDICATOR		0x8140000
#define EMMD_RDC_OFFSET				0x400

#define CONFIG_RAMDUMP
#define CONFIG_RAMDUMP_TFFS
/*
 * Options (parameters below):
 *              #define CONFIG_RAMDUMP_TFFS
 *              #define CONFIG_RAMDUMP_YAFFS2
 *              #define CONFIG_RD_RAM
 */
#ifdef CONFIG_RAMDUMP
#if defined(CONFIG_RAMDUMP_TFFS)
#define CONFIG_FS_TFFS
#define CONFIG_CMD_MMC
#define CONFIG_RAMDUMP_MMC_DEV  1
#elif defined(CONFIG_RAMDUMP_YAFFS2)
#define CONFIG_YAFFS2
#define CONFIG_NAND_PART_SIZE   0
#define CONFIG_NAND_PART_OFFSET 0
#elif defined(CONFIG_RD_RAM)
#endif
#define CONFIG_GZIP_COMPRESSED
#endif

/*
 * Commands configuration
 */
#define CONFIG_SYS_NO_FLASH		/* Declare no flash (NOR/SPI) */
#include <config_cmd_default.h>
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_MFP
#define CONFIG_CMD_MIPS
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MMC
#define CONFIG_CMD_FOBM
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS

#define CONFIG_MRVL_USB_PHY 1
#define CONFIG_MRVL_USB_PHY_28LP 1
#define CONFIG_USB_ETHER
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
#define CONFIG_FASTBOOT_FLASH_MMC_DEV   0

/*
 * mv-common.h should be defined after CMD configs since it used them
 * to enable certain macros
 */
#include "mv-common.h"

#undef CONFIG_ARCH_MISC_INIT

/*
 * Boot setting
 */
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_SHOW_BOOT_PROGRESS
#define CONFIG_MRVL_BOOT    1
#define CONFIG_MISC_INIT_R	1
#define CONFIG_BOOTDELAY	3

#define CONFIG_BOOTARGS                 \
		"initrd=0x03000000,10m rw androidboot.console=ttyS0" \
		" console=ttyS0,115200 panic_debug uart_dma" \
		" crashkernel=4k@0x8140000 androidboot.lcd=1080_50 user_debug=31" \
		" earlyprintk=uart8250-32bit,0xd4018000"

#define CONFIG_BOOTCOMMAND	"mrvlboot"
#define CONFIG_MMC_BOOT_DEV     "mmc dev 0 0"
#define RAMDISK_LOADADDR        (CONFIG_TZ_HYPERVISOR_SIZE + 0x02000000)
#define BOOTIMG_EMMC_ADDR       0x01000000
#define RECOVERYIMG_EMMC_ADDR   0x00500000
/* Kernel size is set to 4MB for legacy non-boot.img format */
#define KERNEL_SIZE             0x00400000
#define RAMDISK_SIZE            0x00400000
#define RECOVERY_KERNEL_LOADADDR        CONFIG_LOADADDR
#define RECOVERY_RAMDISK_LOADADDR       RAMDISK_LOADADDR
#define MRVL_BOOT               1
#define CONFIG_OF_LIBFDT        1
#ifdef CONFIG_OF_LIBFDT
#define DTB_LOADADDR            0x9800000
#define DTB_EMMC_ADDR           (BOOTIMG_EMMC_ADDR + 0xF00000)
#define RECOVERY_DTB_EMMC_ADDR  (RECOVERYIMG_EMMC_ADDR + 0x900000)
#define DTB_SIZE                0x00020000
#endif
/*
 * Environment variables configurations
 */
#define CONFIG_ENV_IS_IN_MMC   1       /* save env in MMV */
#define CONFIG_SYS_MMC_ENV_DEV 0       /* save env in eMMC */
#define CONFIG_CMD_SAVEENV
/* The begin addr where to save u-boot.bin in the eMMC/Nand */
#define CONFIG_UBOOT_PA_START   0xF00000
/* The total size can be used for uboot and env in the eMMC/Nand */
#define CONFIG_UBOOT_PA_SIZE    0x100000
#define CONFIG_UBOOT_PA_END             (CONFIG_UBOOT_PA_START + CONFIG_UBOOT_PA_SIZE)
#define CONFIG_ENV_SIZE		0x8000		/* env size set to 32KB */
#define CONFIG_ENV_OFFSET               (CONFIG_UBOOT_PA_END - CONFIG_ENV_SIZE)
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2      "> "

#define CONFIG_REVISION_TAG	1
/* Marvell PXAV3 MMC Controller Configuration */
#define CONFIG_SDHCI_PXAV3	1
#define CONFIG_EEPROM_34XX02    1

/* Configure Display Back Light */
#define CONFIG_LEDS_LM3532
#define CONFIG_LEDS_88PM828X

/* Marvell Display Controller Configruation */
#define CONFIG_MMP_DISP

#ifdef CONFIG_MMP_DISP
#define CONFIG_CMD_BMP		1
#define CONFIG_VIDEO		1
#define CONFIG_CFB_CONSOLE	1
#define CONFIG_VDMA		1
#define VIDEO_KBD_INIT_FCT	-1
#define VIDEO_TSTC_FCT		serial_tstc
#define VIDEO_GETC_FCT		serial_getc
#define LCD_RST_GPIO		128
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(262144)
#define FRAME_BUFFER_ADDR	0x17000000
#endif
#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_PXA1928_DFC
#define CONFIG_DDR_HW_DFC
#define CONFIG_PXA1928_COMM_D2

/* BAcklight configuration */
#define LM3532_I2C_NUM	4
#endif	/* __CONFIG_PXA1928_H */

#define CONFIG_CMD_TIMERTEST

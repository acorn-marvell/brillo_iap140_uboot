/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Neil Zhang <zhangwm@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1908_CONFIG_H
#define _PXA1908_CONFIG_H

#include <asm/arch/pxa1908.h>

#ifdef CONFIG_MMP_FPGA
#define CONFIG_SYS_TCLK		(12500000)	/* NS16550 clk config */
#else
#define CONFIG_SYS_TCLK		(14745600)	/* NS16550 clk config */
#endif
#define CONFIG_SYS_HZ_CLOCK	(3250000)	/* Timer Freq. 3.25MHZ */

#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_LOADADDR		0xb000000	/* Default load place */

#ifdef CONFIG_SYS_LOAD_ADDR
#undef CONFIG_SYS_LOAD_ADDR
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR
#endif

/*
 * Power definition
 */

#define CONFIG_MARVELL_MFP	/* Enable mvmfp driver */
#define MV_MFPR_BASE		PXA1908_MFPR_BASE

#define MV_UART_CONSOLE_BASE	PXA1908_UART1_BASE
#define CONFIG_SYS_NS16550_IER	(1 << 6)	/* Bit 6 in UART_IER register
						represents UART Unit Enable */

/* Save some global variables in SRAM */
#define CONFIG_SRAM_BASE		0xd1000000
#define CONFIG_PARAM_BASE		0x090F0000
#define CONFIG_OBM_PARAM_MAGIC		CONFIG_PARAM_BASE
#define CONFIG_OBM_PARAM_ADDR		(CONFIG_PARAM_BASE + 0x4)
#define CONFIG_WARM_RESET		(CONFIG_PARAM_BASE + 0x8)
#define CONFIG_AMP_SYNC_ADDR		(CONFIG_PARAM_BASE + 0xC)
#define CONFIG_IDLE_CNT_BASE		(CONFIG_PARAM_BASE + 0x10)
#define CONFIG_CORE_BUSY_ADDR		(CONFIG_PARAM_BASE + 0x80)
#define CONFIG_LPM_TIMER_WAKEUP		(CONFIG_PARAM_BASE + 0x84)

#define IDLE_CNT_ADDR(cpu)		(CONFIG_IDLE_CNT_BASE + (cpu << 2))

/*
* dram definition
*/
#define CONFIG_SYS_MCK5_DDR

/*
 * I2C definition
 */
#ifdef CONFIG_CMD_I2C
#define CONFIG_I2C_MV			1
#define CONFIG_I2C_MULTI_BUS		1

#ifdef CONFIG_PXA1U88
#define CONFIG_MV_I2C_NUM		4
#define CONFIG_MV_I2C_REG		{0xd4011000, 0xd4010800, 0xd4037000, 0xd4013800}
#else
#define CONFIG_MV_I2C_NUM		3
#define CONFIG_MV_I2C_REG		{0xd4011000, 0xd4010800, 0xd4037000}
#endif

#define CONFIG_HARD_I2C			1
#define CONFIG_SYS_I2C_SPEED		100000
#define CONFIG_MV_I2C_SLAVE		0
#endif

/*
 * GPIO definition
 */
#ifdef CONFIG_CMD_GPIO
#define CONFIG_MARVELL_GPIO    1
#endif

/*
 * MMC definition
 */
#ifdef CONFIG_CMD_MMC
#define CONFIG_CMD_FAT			1
#define CONFIG_MMC			1
#define CONFIG_GENERIC_MMC		1
#define CONFIG_SDHCI			1
#define CONFIG_MMC_SDHCI_IO_ACCESSORS	1
#define CONFIG_SYS_MMC_MAX_BLK_COUNT	0x1000
#define CONFIG_MMC_SDMA			1
#define CONFIG_MV_SDHCI			1
#define CONFIG_DOS_PARTITION		1
#define CONFIG_EFI_PARTITION		1
#define CONFIG_SYS_MMC_NUM		3
#define CONFIG_SYS_MMC_BASE		{0xD4280000, 0xD4280800, 0xd4281000}
#endif

/*
 * USB definition
 */
#ifdef CONFIG_USB_ETHER
#define CONFIG_USB_GADGET_MV	1
#define CONFIG_USB_GADGET_DUALSPEED	1
#define CONFIG_IPADDR		192.168.1.101
#define CONFIG_SERVERIP		192.168.1.100
#define CONFIG_ETHADDR		08:00:3e:26:0a:5b
#define CONFIG_NET_MULTI	1
#define CONFIG_USBNET_DEV_ADDR	"00:0a:fa:63:8b:e8"
#define CONFIG_USBNET_HOST_ADDR	"0a:fa:63:8b:e8:0a"
#define CONFIG_MV_UDC		1
#define CONFIG_URB_BUF_SIZE	256
#define CONFIG_USB_REG_BASE	0xd4208000
#define CONFIG_USB_PHY_BASE	0xd4207000
#endif

#ifdef CONFIG_CMD_FASTBOOT
#define CONFIG_USBD_VENDORID		0x18d1
#define CONFIG_USBD_PRODUCTID		0x4e11
#define CONFIG_USBD_MANUFACTURER	"Marvell Inc."
#define CONFIG_USBD_PRODUCT_NAME	"Android 2.1"
#define CONFIG_SERIAL_NUM		"MRUUUVLs001"
#define CONFIG_USBD_CONFIGURATION_STR	"fastboot"
#define CONFIG_SYS_FB_YAFFS \
	{"cache", "system", "userdata", "telephony"}
#define CONFIG_SYS_FASTBOOT_ONFLY_SZ	0x40000
#define USB_LOADADDR			0x100000
#define CONFIG_FB_RESV			256
#endif

#ifdef CONFIG_EMMD_DUMP
#define CRASH_BASE_ADDR        (0x8140000)
#define EMMD_SIG_SIZE          (4 * 1024)
#define CRASH_SIZE             (8 * 1024)
#endif

#define CONFIG_EXTRA_ENV_SETTINGS			\
	"autostart=yes\0"				\
	"verify=no\0"					\
	"cdc_connect_timeout=60\0"
#endif /* _PXA1908_CONFIG_H */

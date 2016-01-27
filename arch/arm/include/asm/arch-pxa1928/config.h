/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _PXA1928_CONFIG_H
#define _PXA1928_CONFIG_H

#include <asm/arch/pxa1928.h>

/*
 * UART definition
 */
#ifdef CONFIG_PXA1928_FPGA
#define MV_UART_CONSOLE_BASE	PXA1928_UART1_BASE
#else
#define MV_UART_CONSOLE_BASE	PXA1928_UART3_BASE
#endif
#define CONFIG_SYS_TCLK		(26000000)	/* NS16550 clk config */
#define CONFIG_SYS_HZ_CLOCK	(32000)	/* Timer Freq. 32KHZ */

/*
 * load definition
 */
#undef CONFIG_SYS_LOAD_ADDR
#define CONFIG_ANDROID_BOOT_IMAGE

#define CONFIG_LOADADDR		0xb000000

#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR
#define CONFIG_SYS_TIMERBASE	PXA1928_TIMER_BASE
#define CONFIG_SYS_NS16550_IER	(1 << 6)	/* Bit 6 in UART_IER register
						represents UART Unit Enable */

/* Save some global variables in SRAM */
#define CONFIG_SRAM_BASE                0xd1020000
#define CONFIG_PARAM_BASE		0x090F0000
#define CONFIG_OBM_PARAM_MAGIC          CONFIG_PARAM_BASE
#define CONFIG_OBM_PARAM_ADDR           (CONFIG_PARAM_BASE + 0x4)
#define CONFIG_WARM_RESET		(CONFIG_PARAM_BASE + 0x8)
#define CONFIG_AMP_SYNC_ADDR		(CONFIG_PARAM_BASE + 0xC)
#define CONFIG_CORE_BUSY_ADDR		(CONFIG_PARAM_BASE + 0x10)

/*
 * dram definition
 */
#define CONFIG_SYS_MCK5_DDR

/*
 * timer definition
 */
#define CONFIG_TIMER_MMP

/*
 * I2C definition
 */
#ifdef CONFIG_CMD_I2C
#define CONFIG_I2C_MV           1
#define CONFIG_MV_I2C_NUM       6
#define CONFIG_I2C_MULTI_BUS    1
#define CONFIG_MV_I2C_REG       {0xd4011000, 0xd4031000, 0xd4032000, \
					0xd4033000, 0xd4033800, 0xd4034000}
#define CONFIG_HARD_I2C         1
#define CONFIG_SYS_I2C_SPEED    100000
#define CONFIG_SYS_I2C_SLAVE    0
#endif

/*
 * MMC definition
 */
#ifdef CONFIG_CMD_MMC
#define CONFIG_CMD_FAT          1
#define CONFIG_MMC              1
#define CONFIG_GENERIC_MMC      1
#define CONFIG_SDHCI            1
#define CONFIG_MMC_SDMA         1
#define CONFIG_MV_SDHCI         1
#define CONFIG_DOS_PARTITION    1
#define CONFIG_EFI_PARTITION    1
#define CONFIG_SYS_MMC_NUM      2
#define CONFIG_SYS_MMC_BASE     {0xd4217000, 0xD4280000}
#define CONFIG_SYS_MMC_MAX_BLK_COUNT   100
#endif
/*
 * USB definition
 */
#ifdef CONFIG_USB_ETHER
#define CONFIG_USB_GADGET_MV	1
#define CONFIG_USB_GADGET_DUALSPEED	1
#define CONFIG_CMD_NET		1
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
#define CONFIG_FB_RESV			512
#endif

/*
 * GPIO/MFP definition
 */
#ifdef CONFIG_CMD_GPIO
#define CONFIG_MARVELL_GPIO    1
#endif
#ifdef CONFIG_CMD_MFP
#define CONFIG_MARVELL_MFP	1
#define MV_MFPR_BASE		PXA1928_MFPR_BASE
#endif


/*
 * FASTBOOT definition
 */

/*
 * EXTRA ENV definition
 */
#define CONFIG_EXTRA_ENV_SETTINGS			\
	"autostart=yes\0"				\
	"verify=yes\0"					\
	"cdc_connect_timeout=60\0"

/*
 * LCD definition
 */
#endif /* _PXA1928_CONFIG_H */

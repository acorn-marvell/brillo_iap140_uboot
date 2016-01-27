/*
 * Copyright 2008 - 2009 Windriver, <www.windriver.com>
 * Author: Tom Rix <Tom.Rix@windriver.com>
 *
 * (C) Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <g_dnl.h>
#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
#include <fb_mmc.h>
#endif

int stop_fastboot;
static int do_fastboot(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret;

#ifdef CONFIG_FASTBOOT_FLASH_MMC_DEV
	print_partion_info();
#endif
	stop_fastboot = 0;
	ret = g_dnl_register("usb_dnl_fastboot");
	if (ret)
		return ret;

	while (1) {
		if (ctrlc() || stop_fastboot)
			break;
		usb_gadget_handle_interrupts();
	}

	udelay(200);
	stop_fastboot = 0;
	g_dnl_unregister();
	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	fb,	1,	0,	do_fastboot,
	"use USB Fastboot protocol",
	"\n"
	"    - run as a fastboot usb device"
);

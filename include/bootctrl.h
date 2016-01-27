/*
 * bootctrl.c mrvl A/B boot support
 *
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Zhengxi hu <zhxihu@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */
#ifndef __MARVELL_COMMON_BOOTCTRL_H
#define __MARVELL_COMMON_BOOTCTRL_H

#define SLOTS_NUM 2
#define SLOT_HYPHEN "_"
#define SLOT_A "a"
#define SLOT_B "b"

int bootctrl_find_boot_slot(void);
unsigned long bootctrl_get_boot_addr(int slot);
char* bootctrl_get_boot_slot_suffix(int slot);
char* bootctrl_get_boot_slot_suffix_only(int slot);
int bootctrl_get_boot_slots_num(void);
int bootctrl_set_active_slot(char *slot_suffix, char *response);
int bootctrl_get_active_slot(void);
char* bootctrl_get_active_slot_suffix(void);
int bootctrl_is_partition_support_slot(char *partition);

#endif


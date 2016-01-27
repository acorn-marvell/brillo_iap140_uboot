/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Yi Zhang <yizhang@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MARVELL_COMMON_CHARGE_H
#define __MARVELL_COMMON_CHARGE_H

#include <common.h>
#include <malloc.h>
#include <power/pmic.h>
#include <power/battery.h>
#include <power/marvell88pm_pmic.h>
#include <errno.h>

extern void power_off_charge(u32 *emmd_pmic, u32 lo_uv, u32 hi_uv,
		void (*charging_indication)(bool));
#endif

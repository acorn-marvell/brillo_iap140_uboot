/*
 * EMMD RESERVE PAGE APIs
 *
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Qing Zhu <qzhu@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _EMMD_RSV_H_
#define _EMMD_RSV_H_
/*
 * MRVL: the emmd reserve page share APIs to
 * below items, they are aligned with kernel's:
 * 1. emmd indicator;
 * 2. unsigned int indicator;
 * 3. super kmalloc flags;
 * 4. ram tags info;
 * 5. held status registers in SQU;
 * 6. (eden)PMIC power up/down log;
 * 7. (eden)reset status registers;
 * 8. pmic up/down status;
 * 9. pmic up/down reason string - for ramdump.
 *
 * note: RDC for RAMDUMP is defined @0x8140400,size is 0x400, so we will
 * use 0x8140000~0x8140400 to save above info.
 */
#define CRASH_PAGE_SIZE_SKM	8
#define CRASH_PAGE_SIZE_RAMTAG	9
#define CRASH_PAGE_SIZE_HELDSTATUS	64
#define CRASH_PAGE_SIZE_PMIC	2
#define CRASH_PAGE_SIZE_RESET_STATUS	4
#define CRASH_PAGE_SIZE_POWER_REASON	128

struct ram_tag_info {
	char name[24];
	union {
		unsigned long data;
		/* keep 8 bytes on ARM and ARM64 */
		unsigned long long data64;
	};
};

struct emmd_page {
	unsigned int indicator;
	unsigned int dump_style;
	unsigned int p_rsvbuf[CRASH_PAGE_SIZE_SKM];
	struct ram_tag_info ram_tag[CRASH_PAGE_SIZE_RAMTAG];
	unsigned int held_status[CRASH_PAGE_SIZE_HELDSTATUS];
	unsigned int pmic_regs[CRASH_PAGE_SIZE_PMIC];
	unsigned int reset_status[CRASH_PAGE_SIZE_RESET_STATUS];
	unsigned int pmic_power_status;
	char pmic_power_reason[CRASH_PAGE_SIZE_POWER_REASON];
};

extern struct emmd_page *emmd_page;
extern int save_squdbg_regval2ddr(void);
extern int show_squdbg_regval(void);
#endif /* _EMMD_RSV_H_ */

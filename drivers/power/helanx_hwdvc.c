/*
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by:
 * Zhoujie Wu <zjwu@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <errno.h>
#include <common.h>
#include <command.h>
#include <asm/arch/cpu.h>

#define DVC_DVCR	0xd4052000
#define	DVC_VL01STR	0xd4052004
#define	DVC_AP		0xd4052020
#define	DVC_CP		0xd4052024
#define	DVC_DP		0xd4052028
#define	DVC_APSUB	0xd405202c
#define	DVC_APCHIP	0xd4052030
#define	DVC_STATUS	0xd4052040
#define	DVC_IMR		0xd4052050
#define	DVC_ISR		0xd4052054
#define	DVC_DEBUG	0xd4052058
#define DVC_EXRA_STR	0xd405205c

#define MAX_DVC_LV	8

/* sw triggered active&lpm dvc register */
union pmudvc_cr {
	struct {
		unsigned int lpm_avc_en:1;
		unsigned int act_avc_en:1;
		unsigned int rsv:30;
	} b;
	unsigned int v;
};

/* sw triggered active&lpm dvc register */
union pmudvc_xp {
	struct {
		unsigned int lpm_vl:3;
		unsigned int lpm_avc_en:1;
		unsigned int act_vl:3;
		unsigned int act_vcreq:1;
		unsigned int rsv:24;
	} b;
	unsigned int v;
};

/* interrupts mask/status register */
/* IMR: 0 - mask, 1 - unmask, ISR: 0 - clear, 1 - ignore */
union pmudvc_imsr {
	struct {
		unsigned int ap_vc_done:1;
		unsigned int cp_vc_done:1;
		unsigned int dp_vc_done:1;
		unsigned int rsv:29;
	} b;
	unsigned int v;
};

/* stable timer setting of VLi ->VLi+1 */
union pmudvc_stbtimer {
	struct {
		unsigned int stbtimer:16;
		unsigned int rsv:16;
	} b;
	unsigned int v;
};

/* hwdvc apsub apchip register */
union pmudvc_apsubchip {
	struct {
		unsigned int mode0_vl:3; /*apsub_idle/nudr_apchip_sleep */
		unsigned int mode0_en:1;
		unsigned int mode1_vl:3; /*udr_apchip_sleep */
		unsigned int mode1_en:1;
		unsigned int mode2_vl:3; /*nudr_apsub_sleep */
		unsigned int mode2_en:1;
		/* add used field here */
		unsigned int rsv:20;
	} b;
	unsigned int v;
};

static void hwdvc_clr_apvcdone_isr(void)
{
	union pmudvc_imsr pmudvc_isr;

	/* write 0 to clear, write 1 has no effect */
	pmudvc_isr.v = __raw_readl(DVC_ISR);
	pmudvc_isr.b.ap_vc_done = 0;
	pmudvc_isr.b.cp_vc_done = 1;
	pmudvc_isr.b.dp_vc_done = 1;
	__raw_writel(pmudvc_isr.v, DVC_ISR);
}

int setapactivevl(u32 vl)
{
	union pmudvc_cr pmudvc_cr;
	union pmudvc_xp pmudvc_ap;
	union pmudvc_imsr pmudvc_imsr;

	if (vl > MAX_DVC_LV) {
		printf("false dvc vl%u\n", vl);
		return -1;
	}

	/* global enable control */
	pmudvc_cr.v = __raw_readl(DVC_DVCR);
	pmudvc_cr.b.act_avc_en = 1;
	__raw_writel(pmudvc_cr.v, DVC_DVCR);

	/* unmask AP DVC done int */
	pmudvc_imsr.v = __raw_readl(DVC_IMR);
	pmudvc_imsr.b.ap_vc_done = 1;
	__raw_writel(pmudvc_imsr.v, DVC_IMR);

	hwdvc_clr_apvcdone_isr();

	/* trigger vc and wait for done */
	pmudvc_ap.v = __raw_readl(DVC_AP);
	pmudvc_ap.b.act_vl = vl;
	pmudvc_ap.b.act_vcreq = 1;
	__raw_writel(pmudvc_ap.v, DVC_AP);

	pmudvc_imsr.v = __raw_readl(DVC_ISR);
	while (!pmudvc_imsr.b.ap_vc_done) {
		pmudvc_imsr.v = __raw_readl(DVC_ISR);
		udelay(1);
	}

	hwdvc_clr_apvcdone_isr();

	/* By default, we don't enable dvc logic in uboot */
	pmudvc_cr.v = __raw_readl(DVC_DVCR);
	pmudvc_cr.b.act_avc_en = 0;
	__raw_writel(pmudvc_cr.v, DVC_DVCR);

	return 0;
}

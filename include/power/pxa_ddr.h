/*
 * (C) Copyright 2012
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Jane Li <jiel@marvell.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef DDR_H_
#define DDR_H_

#if defined(CONFIG_PXA988)
#define MCK4_BASE				0xC0100000
#endif

#define MCU_CPU_ID_REV				0x0000
#define DRAM_STATUS				0x0008
#define MEMORY_ADDRESS_MAP0			0x0010
#define MEMORY_ADDRESS_MAP1			0x0014
#if defined(CONFIG_PXA988)
#define MEMORY_ADDRESS_MAP2			0x0018
#define MEMORY_ADDRESS_MAP3			0x001C
#endif

#define SDRAM_CONFIG0_TYPE1			0x0020
#define SDRAM_CONFIG1_TYPE1			0x0024
#if defined(CONFIG_PXA988)
#define SDRAM_CONFIG2_TYPE1			0x0028
#define SDRAM_CONFIG3_TYPE1			0x002C
#endif

#define SDRAM_CONFIG0_TYPE2			0x0030
#define SDRAM_CONFIG1_TYPE2			0x0034
#if defined(CONFIG_PXA988)
#define SDRAM_CONFIG2_TYPE2			0x0038
#define SDRAM_CONFIG3_TYPE2			0x003C
#endif

#define SDRAM_CTRL1				0x0050
#define SDRAM_CTRL2				0x0054
#define SDRAM_CTRL4				0x0058
#define SDRAM_CTRL6				0x005C
#define SDRAM_CTRL7				0x0060
#define SDRAM_CTRL13				0x0064
#define SDRAM_CTRL14				0x0068

#define SDRAM_TIMING1				0x0080
#define SDRAM_TIMING2				0x0084
#define SDRAM_TIMING3				0x0088
#define SDRAM_TIMING4				0x008C
#define SDRAM_TIMING5				0x0090
#define SDRAM_TIMING6				0x0094
#define SDRAM_TIMING7				0x0098
#define SDRAM_TIMING8				0x009C

#define EXCLUSIVE_MONITOR_CTRL			0x0100
#define DATA_COH_CTRL				0x0110
#define TRUSTZONE_SEL				0x0120
#define TRUSTZONE_RANGE0			0x0124
#define TRUSTZONE_RANGE1			0x0128
#define TRUSTZONE_PERMISSION			0x012C
#define PORT_PRIORITY				0x0140
#define BQ_STARV_PREV				0x0144
#define RRB_STARV_PREV0				0x0148
#define RRB_STARV_PREV1				0x014C
#define SRAM_CTRL1				0x0150
#define SRAM_CTRL2				0x0154
#define SRAM_CTRL3				0x0158
#define USER_INITIATED_COMMAND0			0x0160
#define USER_INITIATED_COMMAND1			0x0164
#define MODE_RD_DATA				0x0170
#if defined(CONFIG_PXA988)
#define SMR1					0x0180
#define SMR2					0x0184
#endif

#define REGISTER_TABLE_CTRL_0			0x01C0
#define REGISTER_TABLE_DATA_0			0x01C8
#define REGISTER_TABLE_DATA_1			0x01CC
#define PHY_CTRL3				0x0220
#define PHY_CTRL7				0x0230
#define PHY_CTRL8				0x0234
#define PHY_CTRL9				0x0238
#define PHY_CTRL10				0x023C
#define PHY_CTRL11				0x0240
#define PHY_CTRL13				0x0248
#define PHY_CTRL14				0x024C
#define PHY_CTRL15				0x0250
#define PHY_CTRL16				0x0254
#define PHY_CTRL21				0x0258
#define PHY_CTRL19				0x0280
#define PHY_CTRL20				0x0284
#define PHY_CTRL22				0x0288
#define PHY_DQ_BYTE_SEL				0x0300
#define PHY_DLL_CTRL				0x0304
#define PHY_DQ_BYTE_CTRL			0x0308
#define PHY_DLL_WL_SEL				0x0380
#define PHY_DLL_WL_CTRL0			0x0384
#define PHY_DLL_WL_CTRL1			0x0388
#define PHY_DLL_WL_CTRL2			0x038C
#define PHY_DLL_RL_CTRL				0x0390
#if defined(CONFIG_PXA988)
#define PHY_CTRL_TESTMODE			0x0400
#endif
#define TEST_MODE0				0x0410
#define TEST_MODE1				0x0414

#define PERFORMANCE_COUNTER_CTRL_0		0x0440
#define PERFORMANCE_COUNTER_STATUS		0x0444
#define PERFORMANCE_COUNTER_SELECT		0x0448
#define PERFORMANCE_COUNTER0			0x0450
#define PERFORMANCE_COUNTER1			0x0454
#define PERFORMANCE_COUNTER2			0x0458
#define PERFORMANCE_COUNTER3			0x045C

#define SDRAM_CTRL4_TYPE_SHIFT	2
#define DDR3		(0x2 << SDRAM_CTRL4_TYPE_SHIFT)
#define LPDDR2		(0x5 << SDRAM_CTRL4_TYPE_SHIFT)

#define SDRAM_CTRL4_CL_SHIFT	13
#define SDRAM_CTRL4_CL_MASK	(0xF << SDRAM_CTRL4_CL_SHIFT)

struct ddr_timing {
	u32 reg1;
	u32 reg2;
	u32 reg3;
	u32 reg4;
	u32 reg5;
	u32 reg6;
	u32 reg7;
	u32 reg8;
};

struct ddr_phy_ds {
	u32 phy7;
	u32 phy8;
	u32 phy9;
};

struct ddr_phy_misc {
	u32 phy13;
};

struct platform_ddr_setting {
	u32 ddr_freq;
	u32 cas_latency;
	u32 table_idx;
	struct ddr_timing setting;
	struct ddr_phy_ds phy_ds;
	struct ddr_phy_misc phy_misc;
};

#endif /*DDR_H_*/

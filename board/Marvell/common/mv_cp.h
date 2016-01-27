/*
 * FILENAME:	mv_cp.h
 * PURPOSE:	define cp loadtable structure
 *
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Wei Shu <weishu@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __MV_CP_H__
#define __MV_CP_H__

#define LOAD_TABLE_OFFSET       0x1C0
#define LT_APP2COM_DATA_LEN     48
#define LOAD_TABLE_SIGNATURE    "LOAD_TABLE_SIGN"
#define OFFSET_IN_ARBEL_IMAGE   4
/* As agreed with CP side, reserve 0x800 byte for each key section.  */
#define SHM_KEY_SECTION_LEN	0x800
#define SMALL_CODE_SIGNARTURE   0x534c4344

#define NETWORK_MODE_TD 0x5F54445F
#define NETWORK_MODE_WB 0x5F57425F

#define LINK_TYPE_DL 0x5F444C5F
#define LINK_TYPE_SL 0x5F534C5F

struct load_table_dynamic_part {
	unsigned int b2init;
	unsigned int init;
	unsigned int addrLoadTableRWcopy;
	unsigned int ee_postmortem;
	unsigned int numOfLife;

	unsigned int diag_p_diagIntIfQPtrData;
	unsigned int diag_p_diagIntIfReportsList;
	unsigned int spare_CCCC;

	unsigned int ACIPCBegin;
	unsigned int ACIPCEnd;
	unsigned int LTEUpBegin;
	unsigned int LTEUpEnd;
	unsigned int LTEDownBegin;
	unsigned int LTEDownEnd;
	unsigned int apps2commDataLen;
	unsigned char apps2commDataBuf[LT_APP2COM_DATA_LEN];

	/* CP load addresses for AP */
	unsigned int CPLoadAddr;
	unsigned int MSALoadAddrStart;
	unsigned int MSALoadAddrEnd;

	unsigned int ACIPCPSDownlinkBegin;
	unsigned int ACIPCPSDownlinkEnd;
	unsigned int ACIPCPSUplinkBegin;
	unsigned int ACIPCPSUplinkEnd;
	unsigned int ACIPCOtherPortDlBegin;
	unsigned int ACIPCOtherPortDlEnd;
	unsigned int ACIPCOtherPortUlBegin;
	unsigned int ACIPCOtherPortUlEnd;

	unsigned int ACIPCDIAGPortDlBegin;
	unsigned int ACIPCDIAGPortDlEnd;
	unsigned int ACIPCDIAGPortUlBegin;
	unsigned int ACIPCDIAGPortUlEnd;

	unsigned int ACIPCPSKeySectionBegin;
	unsigned int ACIPCOtherPortKeySectionBegin;
	unsigned int ACIPCDIAGKeySectionBegin;

	unsigned int MemLogBegin;
	unsigned int MemLogEnd;
	/* network mode, '_TD_' for TD mode, '_WB_' for WB mode */
	unsigned int NetworkModeIndication;

	unsigned int RFBinLoadAddrStart;
	unsigned int RFBinLoadAddrEnd;

	/* link type, '_DL_' for dual link, '_SL_' for single link */
	unsigned int LinkType;
};

struct cp_load_table_head {
	union {
		struct load_table_dynamic_part dp;
		unsigned char filler[LOAD_TABLE_OFFSET - OFFSET_IN_ARBEL_IMAGE -
				     4];
	};
	unsigned int smallCodeSign;
	unsigned int imageBegin;
	unsigned int imageEnd;
	char Signature[16];
	unsigned int sharedFreeBegin;
	unsigned int sharedFreeEnd;
	unsigned int ramRWbegin;
	unsigned int ramRWend;
	unsigned int spare_EEEE;
	unsigned int ReliableDataBegin;
	unsigned int ReliableDataEnd;

	char OBM_VerString[8];
	unsigned int OBM_Data32bits;
};

struct OBM2OSL;
int get_cp_ddr_range(struct OBM2OSL *obm2osl,
		     unsigned int *start, unsigned int *size);
int get_cp_ddr_range2(struct OBM2OSL *obm2osl, unsigned int *exist_cp_prop,
		     unsigned int *start, unsigned int *size);

int get_powermode(void);
#endif

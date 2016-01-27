/*
 * FILENAME:	tim.h
 * PURPOSE:	define the TIM/NTIM structure
 *
 * (C) Copyright 2014
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Wei Shu <weishu@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __TIM_H__
#define __TIM_H__

/* TIM Versions */
#define TIM_3_2_00			0x30200	/* Support for Partitioning */
#define TIM_3_3_00			0x30300	/* Support for ECDSA-256 */
#define TIM_3_4_00			0x30400	/* Support for ECDSA-521 */
#define TIM_3_5_00			0x30500	/* Support for Encrypted Boot */

/* Predefined Image Identifiers */
#define TIMIDENTIFIER       0x54494D48	/* "TIMH" */
#define TIMDUALBOOTID       0x54494D44	/* "TIMD" */
#define WTMIDENTIFIER       0x57544D49	/* "WTMI" */

#define ARBIID              0x41524249	/* "ARBI" */
#define GRBIID              0x47524249	/* "GRBI" */
#define ARB2                0x41524232	/* "ARB2" */
#define GRB2                0x47524232	/* "GRB2" */
#define RFBIID              0x52464249	/* "RFBI" */
#define RFB2                0x52464232	/* "RFB2" */
#define MDBIID              0x4D444249	/* "MDBI" */
#define MDB2                0x4D444232	/* "MDB2" */

enum ENCRYPTALGORITHMID_T {
	Marvell_DS = 0x00000000,
	PKCS1_v1_5_Caddo = 0x00000001,
	PKCS1_v2_1_Caddo = 0x00000002,
	PKCS1_v1_5_Ippcp = 0x00000003,
	PKCS1_v2_1_Ippcp = 0x00000004,
	ECDSA_256 = 0x00000005,
	ECDSA_521 = 0x00000006,
	AES_TB_CTS_ECB128 = 0x0001E000,
	AES_TB_CTS_ECB256 = 0x0001E001,
	AES_TB_CTS_ECB192 = 0x0001E002,
	AES_TB_CTS_CBC128 = 0x0001E004,
	AES_TB_CTS_CBC256 = 0x0001E005,
	AES_TB_CTS_CBC192 = 0x0001E006,
	DUMMY_ENALG = 0x7FFFFFFF
};

enum HASHALGORITHMID_T {
	SHA160 = 0x00000014,
	SHA256 = 0x00000020,
	SHA512 = 0x00000040,
	DUMMY_HASH = 0x7FFFFFFF
};

struct VERSION_I {
	unsigned int version;
	unsigned int identifier;	/* "TIMH" */
	unsigned int trusted;	/* 1- Trusted, 0 Non */
	unsigned int issue_data;
	unsigned int oem_uniqueid;
};

struct FLASH_I {
	unsigned int wtm_flashsign;
	unsigned int wtm_entryaddr;
	unsigned int wtm_entryaddrback;
	unsigned int wtm_patchsign;
	unsigned int wtm_patchaddr;
	unsigned int boot_flashsign;
};

struct IMAGE_INFO_3_5_0 {
	unsigned int imageid;	/* Indicate which Image */
	unsigned int next_imageid;	/* Indicate next image in the chain */
	unsigned int flash_entryaddr;	/* Block numbers for NAND */
	unsigned int load_addr;
	unsigned int image_size;
	unsigned int image_size_to_hash;
	enum HASHALGORITHMID_T hash_algorithmid;	/* See enum HASHALGORITHMID_T */
	unsigned int hash[16];	/* Reserve 512 bits for the hash */
	unsigned int partition_number;
	enum ENCRYPTALGORITHMID_T enc_algorithmid;	/* See enum ENCRYPTALGORITHMID_T */
	unsigned int encrypt_startoffset;
	unsigned int encrypt_size;
};

struct IMAGE_INFO_3_4_0 {
	unsigned int imageid;	/* Indicate which Image */
	unsigned int next_imageid;	/* Indicate next image in the chain */
	unsigned int flash_entryaddr;	/* Block numbers for NAND */
	unsigned int load_addr;
	unsigned int image_size;
	unsigned int image_size_to_hash;
	enum HASHALGORITHMID_T hash_algorithmid;	/* See enum HASHALGORITHMID_T */
	unsigned int hash[16];	/* Reserve 512 bits for the hash */
	unsigned int partition_number;
};

struct KEY_MOD_3_4_0;
struct PLAT_DS;

/* Constant part of the TIMs */
struct CTIM {
	struct VERSION_I version_bind;
	struct FLASH_I flash_info;
	unsigned int num_images;
	unsigned int num_keys;
	unsigned int sizeof_reserved;
};

/* TIM structure for use by DKB/OBM/BootROM */
struct TIM {
	struct CTIM *ctim;	/* Constant part */
	void *img;		/* Pointer to Images */
	void *key;		/* Pointer to Keys */
	unsigned int *reserved;	/* Pointer to Reserved Area */
	struct PLAT_DS *tbtim_ds;	/* Pointer to Digital Signature */
};

/* NTIM structure for use by DKB/OBM/BootROM */
struct NTIM {
	struct CTIM *ctim;	/* Constant part */
	void *img;		/* Pointer to Images */
	unsigned int *reserved;	/* Pointer to Reserved Area */
};

int set_tim_pointers(unsigned char *start_addr, struct TIM *tim);
void *return_imgptr(struct TIM *tim, unsigned int tim_version,
		    unsigned int img_num);
void *find_image_intim(struct TIM *tim, unsigned int tim_version,
		       unsigned int imageid);
int get_image_flash_entryaddr(struct TIM *tim, unsigned int tim_version,
			      unsigned int imageid, unsigned int *addr,
			      unsigned int *size);

#endif

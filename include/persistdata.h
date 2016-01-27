#ifndef _PERSISTDATA_H
#define _PERSISTDATA_H

#define PD_TYPE_SN			0xF001
#define PD_TYPE_WIFIMAC	0xF002
#define PD_TYPE_BTMAC		0xF003
#define PD_TYPE_ZIGBMAC	0xF004
#define PD_TYPE_CHECKSUM	0xF0FF

#define PD_BLOCK_MAX  		32

#define PD_HEADER_MAGIC			0x19081400

#define PD_MMC_OFFSET		0x300000
#define PD_MMC_MAXSIZE		0x100000
#define PD_MMC_PART		1
#define PD_MMC_PART_RECOVERY 2

typedef struct persistent_data_header {
	unsigned int magic;
	unsigned int length;
}PDATA_HEADER_t;


typedef struct persistent_data_block {
	unsigned int type;
	unsigned int length;
	unsigned char data[0];
}PDATA_BLOCK_t;

int emmc_get_persist_data(void);

int mem_get_persist_data_block(int type, PDATA_BLOCK_t **pd);

#endif

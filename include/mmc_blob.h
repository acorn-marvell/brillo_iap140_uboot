#ifndef _MMC_BLOB_H
#define _MMC_BLOB_H

typedef enum e_mmc_blob_type {
	BLOB_TYPE_HEADER,
	BLOB_TYPE_GENERIC,
	BLOB_TYPE_MAX
}mmc_blob_type;

typedef enum e_mmc_blob_op {
	MMC_BLOB_READ,
	MMC_BLOB_WRITE,
	MMC_BLOB_ERASE,
} mmc_blob_op;

typedef struct mmc_blob_desc {
	u_char mmc_dev;
	u_char mmc_part;
	u_int offset;
	u_int max_size;
	u_int magic;
	mmc_blob_type type;
} mmc_blob_desc_t;

typedef struct mmc_blob_data {
	u_int magic;
	u_int length;
	u_char data[0];
} mmc_blob_data_t;


void mmc_blob_desc_init(mmc_blob_desc_t *desc, u_char mmc_dev,
		u_char mmc_part, u_int offset, u_int max_size, u_int magic,
		mmc_blob_type type);
mmc_blob_data_t* mmc_blob_data_new(u_char *buf, u_int len);
void mmc_blob_data_delete(mmc_blob_data_t *blob_data);
int mmc_blob_oper(mmc_blob_desc_t *desc, mmc_blob_data_t *bdata,
		mmc_blob_op op);
int mmc_blob_cmp(const mmc_blob_data_t *bdata1,
		const mmc_blob_data_t *bdata2, int cmp_magic);

#endif /* _MMC_BLOB_H */

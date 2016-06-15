#ifndef _BVB_MMC_H
#define _BVB_MMC_H

#include <mmc_blob.h>

#define MMC_DEV		2
#define MMC_BOOT0	1
#define MMC_BOOT1	2

#define BVB_OK				0
#define BVB_ERR				-1
#define BVB_ERR_WRITE_SAME	-99

#define BVB_LOCKED			1
#define BVB_UNLOCKED		0

#define BVB_CHECK_STATE_YES	1
#define BVB_CHECK_STATE_NO	0

/**
 * bvb_read_rollback_index - read NVRAM_minimum_rollback_index from WP eMMC
 *
 * @index: output argument for NVRAM_minimum_rollback_index
 * @return: BVB_OK if success, BVB_ERR if fail
 */
int bvb_read_rollback_index(u_int *index);

/**
 * bvb_write_rollback_index - write NVRAM_minimum_rollback_index to WP eMMC
 *
 * @index: input argument for NVRAM_minimum_rollback_index
 * @return: BVB_OK if success, BVB_ERR if fail
 */
int bvb_write_rollback_index(u_int index);

/* device lock state */
int bvb_check_device_lock(void);
int bvb_lock_device(void);
int bvb_unlock_device(void);

/* develop key */
int bvb_write_key(u_char *buf, u_int size);
int bvb_read_key_to_buf(u_char *buf);
mmc_blob_data_t* bvb_make_key_from_mmc(void);
int bvb_erase_key(void);
int bvb_compare_key(const mmc_blob_data_t *dev_key,
		const mmc_blob_data_t *img_key);
int bvb_devkey_match(u_char *buf, u_int len);

#endif /* _BVB_MMC_H */

/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BVB_VERIFY_H_
#define BVB_VERIFY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bvb_boot_image_header.h"

/* Return codes used in bvb_verify_boot_image().
 *
 * BVB_VERIFY_RESULT_OK is returned if the boot image header is valid,
 * the hash is correct and the signature is correct. Keep in mind that
 * you still need to check that you know the public key used to sign
 * the image, see bvb_verify_boot_image() for details.
 *
 * BVB_VERIFY_RESULT_OK_NOT_SIGNED is returned if the boot image
 * header is valid but there is no signature or hash.
 *
 * BVB_VERIFY_INVALID_BOOT_IMAGE_HEADER is returned if the header of
 * the boot image is invalid, for example, invalid magic or
 * inconsistent data.
 *
 * BVB_VERIFY_HASH_MISMATCH is returned if the hash stored in the
 * "Authentication data" block does not match the calculated hash.
 *
 * BVB_VERIFY_SIGNATURE_MISMATCH is returned if the signature stored
 * in the "Authentication data" block is invalid or doesn't match the
 * public key stored in the boot image.
 */
typedef enum {
  BVB_VERIFY_RESULT_OK,
  BVB_VERIFY_RESULT_OK_NOT_SIGNED,
  BVB_VERIFY_RESULT_INVALID_BOOT_IMAGE_HEADER,
  BVB_VERIFY_RESULT_HASH_MISMATCH,
  BVB_VERIFY_RESULT_SIGNATURE_MISMATCH,
} BvbVerifyResult;

/*
 * Checks that raw boot image at |data| of size |length| is a valid
 * Brillo boot image. The complete contents of the boot image must be
 * passed in. It's fine if |length| is bigger than the actual image,
 * typically callers of this function will load the entire contents of
 * the 'boot_a' or 'boot_b' partition and pass in its length (for
 * example, 32 MiB).
 *
 * See the |BvbBootImageHeader| struct for information about the four
 * blocks (header, authentication, auxilary, payload) that make up a
 * boot image.
 *
 * If the function returns |BVB_VERIFY_RESULT_OK| and
 * |out_public_key_data| is non-NULL, it will be set to point inside
 * |data| for where the serialized public key data is stored and
 * |out_public_key_length|, if non-NULL, will be set to the length of
 * the public key data.
 *
 * See the |BvbVerifyResult| enum for possible return values.
 *
 * VERY IMPORTANT:
 *
 *   1. Even if |BVB_VERIFY_RESULT_OK| is returned, you still need to
 *      check that the public key embedded in the image matches a
 *      known key! You can use 'bvbtool extract_public_key' to extract
 *      the key at build time and compare it to what is returned in
 *      |out_public_key_data|.
 *
 *   2. You need to check the |rollback_index| field against a stored
 *      value in NVRAM and reject the boot image if the value in NVRAM
 *      is bigger than |rollback_index|. You must also update the
 *      value stored in NVRAM to the smallest value of
 *      |rollback_index| field from boot images in all bootable and
 *      authentic slots marked as GOOD.
 */
BvbVerifyResult bvb_verify_boot_image(
    const uint8_t* data, size_t length,
    const uint8_t** out_public_key_data, size_t* out_public_key_length);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_VERIFY_H_ */

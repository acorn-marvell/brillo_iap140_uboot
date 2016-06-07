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

#ifndef BVB_UTIL_H_
#define BVB_UTIL_H_

#include "bvb_boot_image_header.h"
#include "bvb_rsa.h"
#include "bvb_sysdeps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Converts a 32-bit unsigned integer from big-endian to host byte order. */
uint32_t bvb_be32toh(uint32_t in);

/* Converts a 64-bit unsigned integer from big-endian to host byte order. */
uint64_t bvb_be64toh(uint64_t in);

/* Adds |value_to_add| to |value| with overflow protection.
 *
 * Returns zero if the addition overflows, non-zero otherwise. In
 * either case, |value| is always modified.
 */
int bvb_safe_add_to(uint64_t *value, uint64_t value_to_add);

/* Adds |a| and |b| with overflow protection, returning the value in
 * |out_result|.
 *
 * It's permissible to pass NULL for |out_result| if you just want to
 * check that the addition would not overflow.
 *
 * Returns zero if the addition overflows, non-zero otherwise.
 */
int bvb_safe_add(uint64_t *out_result, uint64_t a, uint64_t b);

/* Copies |src| to |dest|, byte-swapping fields in the process. */
void bvb_boot_image_header_to_host_byte_order(
    const BvbBootImageHeader* src,
    BvbBootImageHeader* dest);

/* Copies |header| to |dest|, byte-swapping fields in the process. */
void bvb_rsa_public_key_header_to_host_byte_order(
    const BvbRSAPublicKeyHeader* src,
    BvbRSAPublicKeyHeader* dest);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_UTIL_H_ */

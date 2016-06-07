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

#ifndef BVB_PROPERTY_H_
#define BVB_PROPERTY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bvb_boot_image_header.h"

/* Convenience function for looking up the value for a property with
 * name |key| in a Brillo boot image. If |key| is NUL-terminated,
 * |key_size| may be set to 0.
 *
 * The |image_data| parameter must be a pointer to a Brillo Boot Image
 * of size |image_size|.
 *
 * This function returns a pointer to the value inside the passed-in
 * image or NULL if not found. Note that the value is always
 * guaranteed to be followed by a NUL byte.
 *
 * If the value was found and |out_value_size| is not NULL, the size
 * of the value is returned there.
 *
 * This function is O(n) in number of properties so if you need to
 * look up a lot of values, you may want to build a more efficient
 * lookup-table by manually walking all properties yourself.
 *
 * Before using this function, you MUST verify |image_data| with
 * bvb_verify_boot_image() and reject it unless it's signed by a known
 * good public key.
 */
const char* bvb_lookup_property(const uint8_t* image_data, size_t image_size,
                                const char* key, size_t key_size,
                                size_t* out_value_size);

/* Like bvb_lookup_property() but parses the value as an unsigned
 * 64-bit integer. Both decimal and hexadecimal representations
 * (e.g. "0x2a") are supported. Returns 0 on failure and non-zero on
 * success. On success, the parsed value is returned in |out_value|.
 */
int bvb_lookup_property_uint64(const uint8_t* image_data, size_t image_size,
                               const char* key, size_t key_size,
                               uint64_t* out_value);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_PROPERTY_H_ */

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

#include "bvb/bvb_boot_image_header.h"
#include "bvb/bvb_util.h"

const char* bvb_lookup_property(const uint8_t* image_data, size_t image_size,
                                const char* key, size_t key_size,
                                size_t* out_value_size) {
  const BvbBootImageHeader *header = NULL;
  const char* ret = NULL;
  const uint8_t* image_end;
  const uint8_t* prop_start;
  const uint8_t* prop_end;
  const uint8_t* p;

  if (out_value_size != NULL)
    *out_value_size = 0;

  if (image_data == NULL) {
    bvb_debug("image_data is NULL\n.");
    goto out;
  }

  if (key == NULL) {
    bvb_debug("key is NULL\n.");
    goto out;
  }

  if (image_size < sizeof(BvbBootImageHeader)) {
    bvb_debug("Length is smaller than header.\n");
    goto out;
  }

  // Ensure magic is correct.
  if (bvb_memcmp(image_data, BVB_MAGIC, BVB_MAGIC_LEN) != 0) {
    bvb_debug("Magic is incorrect.\n");
    goto out;
  }

  if (key_size == 0)
    key_size = bvb_strlen(key);

  // Careful, not byteswapped - also ensure it's aligned properly.
  bvb_assert_word_aligned(image_data);
  header = (const BvbBootImageHeader *) image_data;
  image_end = image_data + image_size;

  prop_start = image_data + sizeof(BvbBootImageHeader) +
      bvb_be64toh(header->authentication_data_block_size) +
      bvb_be64toh(header->properties_offset);

  prop_end = prop_start + bvb_be64toh(header->properties_size);

  if (prop_start < image_data || prop_start > image_end ||
      prop_end < image_data || prop_end > image_end ||
      prop_end < prop_start) {
    bvb_debug("Properties not inside passed-in data.\n");
    goto out;
  }

  for (p = prop_start; p < prop_end; ) {
    const BvbPropertyHeader *ph = (const BvbPropertyHeader *) p;
    bvb_assert_word_aligned(ph);
    uint64_t key_nb = bvb_be64toh(ph->key_num_bytes);
    uint64_t value_nb = bvb_be64toh(ph->value_num_bytes);
    uint64_t total = sizeof(BvbPropertyHeader) + 2 /* NUL bytes */
        + key_nb + value_nb;
    uint64_t remainder = total % 8;

    if (remainder != 0)
      total += 8 - remainder;

    if (total + p < prop_start || total + p > prop_end) {
      bvb_debug("Invalid data in properties array.\n");
      goto out;
    }
    if (p[sizeof(BvbPropertyHeader) + key_nb] != 0) {
      bvb_debug("No terminating NUL byte in key.\n");
      goto out;
    }
    if (p[sizeof(BvbPropertyHeader) + key_nb + 1 + value_nb] != 0) {
      bvb_debug("No terminating NUL byte in value.\n");
      goto out;
    }
    if (key_size == key_nb) {
      if (bvb_memcmp(p + sizeof(BvbPropertyHeader), key, key_size) == 0) {
        ret = (const char *) (p + sizeof(BvbPropertyHeader) + key_nb + 1);
        if (out_value_size != NULL)
          *out_value_size = value_nb;
        goto out;
      }
    }
    p += total;
  }

out:
  return ret;
}

int bvb_lookup_property_uint64(const uint8_t* image_data, size_t image_size,
                               const char* key, size_t key_size,
                               uint64_t* out_value) {
  const char *value;
  int ret = 0;
  uint64_t parsed_val;
  int base;
  int n;

  value = bvb_lookup_property(image_data, image_size, key, key_size, NULL);
  if (value == NULL)
    goto out;

  base = 10;
  if (bvb_memcmp(value, "0x", 2) == 0) {
    base = 16;
    value += 2;
  }

  parsed_val = 0;
  for (n = 0; value[n] != '\0'; n++) {
    int c = value[n];
    int digit;

    parsed_val *= base;

    switch (base) {
      case 10:
        if (c >= '0' && c <= '9') {
          digit = c - '0';
        } else {
          bvb_debug("Invalid digit.\n");
          goto out;
        }
        break;

      case 16:
        if (c >= '0' && c <= '9') {
          digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
          digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
          digit = c - 'A' + 10;
        } else {
          bvb_debug("Invalid digit.\n");
          goto out;
        }
        break;

      default:
        goto out;
    }

    parsed_val += digit;
  }

  ret = 1;
  if (out_value != NULL)
    *out_value = parsed_val;

out:
  return ret;
}

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

#ifndef BVB_SHA_H_
#define BVB_SHA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "bvb_sysdeps.h"

/* Size in bytes of a SHA-256 digest. */
#define BVB_SHA256_DIGEST_SIZE 32

/* Block size in bytes of a SHA-256 digest. */
#define BVB_SHA256_BLOCK_SIZE 64

/* Size in bytes of a SHA-512 digest. */
#define BVB_SHA512_DIGEST_SIZE 64

/* Block size in bytes of a SHA-512 digest. */
#define BVB_SHA512_BLOCK_SIZE 128

/* Data structure used for SHA-256. */
typedef struct {
  uint32_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * BVB_SHA256_BLOCK_SIZE];
  uint8_t buf[BVB_SHA256_DIGEST_SIZE];  /* Used for storing the final digest. */
} BvbSHA256Ctx;

/* Data structure used for SHA-512. */
typedef struct {
  uint64_t h[8];
  uint32_t tot_len;
  uint32_t len;
  uint8_t block[2 * BVB_SHA512_BLOCK_SIZE];
  uint8_t buf[BVB_SHA512_DIGEST_SIZE];  /* Used for storing the final digest. */
} BvbSHA512Ctx;

/* Initializes the SHA-256 context. */
void bvb_sha256_init(BvbSHA256Ctx* ctx);

/* Updates the SHA-256 context with |len| bytes from |data|. */
void bvb_sha256_update(BvbSHA256Ctx* ctx, const uint8_t* data, uint32_t len);

/* Returns the SHA-256 digest. */
uint8_t* bvb_sha256_final(BvbSHA256Ctx* ctx);

/* Initializes the SHA-512 context. */
void bvb_sha512_init(BvbSHA512Ctx* ctx);

/* Updates the SHA-512 context with |len| bytes from |data|. */
void bvb_sha512_update(BvbSHA512Ctx* ctx, const uint8_t* data, uint32_t len);

/* Returns the SHA-512 digest. */
uint8_t* bvb_sha512_final(BvbSHA512Ctx* ctx);

#ifdef __cplusplus
}
#endif

#endif  /* BVB_SHA_H_ */

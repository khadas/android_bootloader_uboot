/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020, Linaro Limited
 */

#ifndef __CRYPTO_CRYPTO_ACCEL_H
#define __CRYPTO_CRYPTO_ACCEL_H

#include <linux/types.h>
#include <uboot_aes.h>

struct aes_block {
	uint8_t b[AES_BLOCK_LENGTH];
};

int crypto_accel_aes_expand_keys(const void *key, size_t key_len,
					void *enc_key, void *dec_key,
					size_t expanded_key_len,
					unsigned int *round_count);

void crypto_accel_aes_make_dec_key(unsigned int round_count,
		const struct aes_block *key_enc, struct aes_block *key_dec);

void crypto_accel_aes_ecb_enc(void *out, const void *in, const void *key,
			      unsigned int round_count,
			      unsigned int block_count);
void crypto_accel_aes_ecb_dec(void *out, const void *in, const void *key,
			      unsigned int round_count,
			      unsigned int block_count);

void crypto_accel_aes_cbc_enc(void *out, const void *in, const void *key,
			      unsigned int round_count,
			      unsigned int block_count, void *iv);
void crypto_accel_aes_cbc_dec(void *out, const void *in, const void *key,
			      unsigned int round_count,
			      unsigned int block_count, void *iv);

void crypto_accel_aes_ctr_be_enc(void *out, const void *in, const void *key,
				 unsigned int round_count,
				 unsigned int block_count, void *iv);

void crypto_accel_aes_xts_enc(void *out, const void *in, const void *key1,
			      unsigned int round_count,
			      unsigned int block_count, const void *key2,
			      void *tweak);
void crypto_accel_aes_xts_dec(void *out, const void *in, const void *key1,
			      unsigned int round_count,
			      unsigned int block_count, const void *key2,
			      void *tweak);
#endif /*__CRYPTO_CRYPTO_ACCEL_H*/

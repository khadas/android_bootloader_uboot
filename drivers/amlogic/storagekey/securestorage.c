// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/types.h>
#include <asm/arch/secure_apb.h>
#include <amlogic/secure_storage.h>
#include <asm/arch/bl31_apis.h>
#if CONFIG_AML_FLUSH_CACHE
#include <../keymanage/key_manage_i.h>
#endif
#include <linux/arm-smccc.h>

static uint64_t storage_share_in_base;
static uint64_t storage_share_out_base;
static uint64_t storage_share_block_base;
static uint64_t storage_share_block_size;
static int32_t storage_init_status;

static uint64_t bl31_storage_ops(uint64_t function_id)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, 0, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}
uint64_t bl31_storage_ops2(uint64_t function_id, uint64_t arg1)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg1, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}
uint64_t bl31_storage_ops3(uint64_t function_id, uint64_t arg1, uint32_t arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg1, arg2, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static uint64_t bl31_storage_write(uint8_t *keyname, uint8_t *keybuf,
				uint32_t keylen, uint32_t keyattr)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint8_t *data;
	uint32_t namelen;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	*input++ = keylen;
	*input++ = keyattr;
	data = (uint8_t *)input;
	memcpy(data, keyname, namelen);
	data += namelen;
	memcpy(data, keybuf, keylen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp,
				sizeof(namelen) + sizeof(keylen)
				+ sizeof(keyattr) + namelen + keylen);
#endif
	return bl31_storage_ops(SECURITY_KEY_WRITE);
}

static uint64_t bl31_storage_read(uint8_t *keyname, uint8_t *keybuf,
				uint32_t keylen, uint32_t *readlen)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint32_t *output = (uint32_t *)storage_share_out_base;
	uint32_t namelen;
	uint8_t *name, *buf;
	uint64_t ret;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	*input++ = keylen;
	name = (uint8_t *)input;
	memcpy(name, keyname, namelen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp, sizeof(namelen) + sizeof(keylen) + namelen);
#endif
	ret = bl31_storage_ops(SECURITY_KEY_READ);
	if (ret == RET_OK) {
		*readlen = *output;
		buf = (uint8_t *)(output+1);
		memcpy(keybuf, buf, *readlen);
	}
	return ret;
}

static uint64_t bl31_storage_query(uint8_t *keyname, uint32_t *retval)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint32_t *output = (uint32_t *)storage_share_out_base;
	uint32_t namelen;
	uint8_t *name;
	uint64_t ret;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	name = (uint8_t *)input;
	memcpy(name, keyname, namelen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp, sizeof(namelen) + namelen);
#endif
	ret = bl31_storage_ops(SECURITY_KEY_QUERY);
	if (ret == RET_OK)
		*retval = *output;
	return ret;
}

static uint64_t bl31_storage_status(uint8_t *keyname, uint32_t *retval)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint32_t *output = (uint32_t *)storage_share_out_base;
	uint32_t namelen;
	uint8_t *name;
	uint64_t ret;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	name = (uint8_t *)input;
	memcpy(name, keyname, namelen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp, sizeof(namelen) + namelen);
#endif
	ret = bl31_storage_ops(SECURITY_KEY_STATUS);
	if (ret == RET_OK)
		*retval = *output;
	return ret;
}
static uint64_t bl31_storage_tell(uint8_t *keyname, uint32_t *retval)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint32_t *output = (uint32_t *)storage_share_out_base;
	uint32_t namelen;
	uint8_t *name;
	uint64_t ret;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	name = (uint8_t *)input;
	memcpy(name, keyname, namelen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp, sizeof(namelen) + namelen);
#endif
	ret = bl31_storage_ops(SECURITY_KEY_TELL);
	if (ret == RET_OK)
		*retval = *output;
	return ret;
}

static uint64_t bl31_storage_verify(uint8_t *keyname, uint8_t *hashbuf)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint32_t *output = (uint32_t *)storage_share_out_base;
	uint32_t namelen;
	uint8_t *name;
	uint64_t ret;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	name = (uint8_t *)input;
	memcpy(name, keyname, namelen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp, sizeof(namelen) + namelen);
#endif
	ret = bl31_storage_ops(SECURITY_KEY_VERIFY);

	if (ret == RET_OK)
		memcpy(hashbuf, (uint8_t *)output, 32);
	return ret;
}

static uint64_t bl31_storage_list(uint8_t *listbuf,
	uint32_t outlen, uint32_t *readlen)
{
	uint32_t *output = (uint32_t *)storage_share_out_base;
	uint64_t ret;

	ret = bl31_storage_ops(SECURITY_KEY_LIST);
	if (ret == RET_OK) {
		if (*output > outlen)
			*readlen = outlen;
		else
			*readlen = *output;
		memcpy(listbuf, (uint8_t *)(output+1), *readlen);
	}
	return ret;
}

static uint64_t bl31_storage_remove(uint8_t *keyname)
{
	uint32_t *input = (uint32_t *)storage_share_in_base;
	uint32_t namelen;
	uint8_t *name;
#if CONFIG_AML_FLUSH_CACHE
	uint32_t *input_tmp;

	input_tmp = input;
#endif
	namelen = strlen((const char *)keyname);
	*input++ = namelen;
	name = (uint8_t *)input;
	memcpy(name, keyname, namelen);
#if CONFIG_AML_FLUSH_CACHE
	flush_cache((unsigned long)input_tmp, sizeof(namelen) + namelen);
#endif
	return bl31_storage_ops(SECURITY_KEY_REMOVE);
}

static inline int32_t smc_to_ns_errno(uint64_t errno)
{
	int32_t ret = (int32_t)(errno&0xffffffff);
	return ret;
}

void secure_storage_init(void)
{
		storage_share_in_base =
				bl31_storage_ops(GET_SHARE_STORAGE_IN_BASE);
		storage_share_out_base =
				bl31_storage_ops(GET_SHARE_STORAGE_OUT_BASE);
		storage_share_block_base =
				bl31_storage_ops(GET_SHARE_STORAGE_BLOCK_BASE);
		storage_share_block_size =
				bl31_storage_ops(GET_SHARE_STORAGE_BLOCK_SIZE);

		if ((int)storage_share_in_base == SMC_UNK ||
			(int)storage_share_out_base == SMC_UNK ||
			(int)storage_share_block_base == SMC_UNK ||
			(int)storage_share_block_size == SMC_UNK)
			storage_init_status = -1;
		else
			storage_init_status = 1;
}

void *secure_storage_getbuffer(uint32_t *size)
{
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1) {
			storage_share_block_size =
				bl31_storage_ops(GET_SHARE_STORAGE_BLOCK_SIZE);
		*size = (uint32_t)storage_share_block_size;
		return (void *)storage_share_block_base;
	}
	else {
		*size = 0;
		return NULL;
	}
}
void secure_storage_notifier(void)
{
	bl31_storage_ops(SECURITY_KEY_NOTIFY);
}

void secure_storage_notifier_ex(uint32_t storagesize, uint32_t rsvarg)
{
	bl31_storage_ops3(SECURITY_KEY_NOTIFY_EX, storagesize, rsvarg);
}

int32_t secure_storage_write(uint8_t *keyname, uint8_t *keybuf,
				uint32_t keylen, uint32_t keyattr)
{
	uint32_t ret;

	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_write(keyname, keybuf, keylen, keyattr);
	else
		ret = RET_EUND;
	return smc_to_ns_errno(ret);
}

int32_t secure_storage_read(uint8_t *keyname, uint8_t *keybuf,
			uint32_t keylen, uint32_t *readlen)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_read(keyname, keybuf, keylen, readlen);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

int32_t secure_storage_query(uint8_t *keyname, uint32_t *retval)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_query(keyname, retval);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

int32_t secure_storage_status(uint8_t *keyname, uint32_t *retval)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_status(keyname, retval);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

int32_t secure_storage_tell(uint8_t *keyname, uint32_t *retval)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_tell(keyname, retval);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

int32_t secure_storage_verify(uint8_t *keyname, uint8_t *hashbuf)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_verify(keyname, hashbuf);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

int32_t secure_storage_list(uint8_t *listbuf,
		uint32_t buflen, uint32_t *readlen)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_list(listbuf, buflen, readlen);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

int32_t secure_storage_remove(uint8_t *keyname)
{
	uint64_t ret;
	if (storage_init_status == 0)
		secure_storage_init();

	if (storage_init_status == 1)
		ret = bl31_storage_remove(keyname);
	else
		ret = RET_EUND;

	return smc_to_ns_errno(ret);
}

void secure_storage_set_info(uint32_t info)
{
	struct arm_smccc_res res;

	arm_smccc_smc(SET_STORAGE_INFO, info, 0, 0, 0, 0, 0, 0, &res);
}

int32_t secure_storage_set_enctype(uint32_t type)
{
	uint64_t  ret;
	ret = bl31_storage_ops2(SECURITY_KEY_SET_ENCTYPE, type);
	return smc_to_ns_errno(ret);
}

int32_t secure_storage_get_enctype(void)
{
	uint64_t ret;
	ret = bl31_storage_ops(SECURITY_KEY_GET_ENCTYPE);
	return smc_to_ns_errno(ret);
}

int32_t secure_storage_version(void)
{
	uint64_t ret;
	ret = bl31_storage_ops(SECURITY_KEY_VERSION);
	return smc_to_ns_errno(ret);
}

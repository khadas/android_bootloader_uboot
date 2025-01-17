// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <tee.h>
#include <tee/optee_ta_avb.h>
#include <avb_verify.h>
#include <amlogic/anti-rollback.h>
#include <amlogic/storage.h>
#include <amlogic/aml_efuse.h>
#include <mmc.h>

static struct AvbOps *g_avb_ops;
static struct udevice *g_dev;
static struct tee_open_session_arg g_open_arg;

#define TA_RPMB_UUID { 0x0ab5a718, 0xee63, 0x4115, \
	{ 0xa0, 0xad, 0xef, 0x6e, 0xe9, 0x8f, 0xcb, 0xc7 } }

#define CMD_RPMB_AUTH_KEY_INIT      0
#define CMD_RPMB_AUTH_KEY_STATE     1

static bool is_rpmb_available(void)
{
	int ret = 0;
	struct udevice *dev = NULL;
	struct tee_open_session_arg open_arg = { 0 };
	struct tee_invoke_arg invoke_arg = { 0 };
	const struct tee_optee_ta_uuid uuid = TA_RPMB_UUID;
	struct tee_param param = { .attr = TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT };
	struct mmc *mmc = NULL;

	if (store_get_type() != BOOT_EMMC) {
		printf("Storage Medium isn't EMMC\n");
		return false;
	}

	mmc = find_mmc_device(1);
	if (!mmc) {
		printf("Not find EMMC\n");
		return false;
	}

	dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!dev) {
		printf("tee_find_device() failed\n");
		return false;
	}

	memset(&open_arg, 0, sizeof(open_arg));
	tee_optee_ta_uuid_to_octets(open_arg.uuid, &uuid);
	ret = tee_open_session(dev, &open_arg, 0, NULL);
	if (ret) {
		printf("tee_open_session() failed, return 0x%x\n", ret);
		return false;
	}

	if (open_arg.ret) {
		printf("tee_open_session() failed, return 0x%x, origin 0x%x\n",
				open_arg.ret, open_arg.ret_origin);
		return false;
	}

	memset(&invoke_arg, 0, sizeof(invoke_arg));
	invoke_arg.session = open_arg.session;
	invoke_arg.func = CMD_RPMB_AUTH_KEY_STATE;

	ret = tee_invoke_func(dev, &invoke_arg, 1, &param);
	tee_close_session(dev, open_arg.session);

	if (ret) {
		printf("tee_invoke_func() failed, return 0x%x\n", ret);
		return false;
	}
	if (invoke_arg.ret) {
		printf("tee_invoke_func() failed, return 0x%x, origin 0x%x\n",
				invoke_arg.ret, invoke_arg.ret_origin);
		return false;
	}

	return param.u.value.a ? true : false;
}

static bool avb2_init(void)
{
	unsigned long mmc_dev = 0;

	if (!g_avb_ops)
		g_avb_ops = avb_ops_alloc(mmc_dev);

	if (!g_avb_ops) {
		printf("AVB 2.0 initialize failed\n");
		return false;
	};

	return true;
}

static bool open_session(void)
{
	int ret = 0;
	const struct tee_optee_ta_uuid uuid = TA_AVB_UUID;

	g_dev = tee_find_device(NULL, NULL, NULL, NULL);
	if (!g_dev) {
		printf("tee_find_device() failed\n");
		return false;
	}

	memset(&g_open_arg, 0, sizeof(g_open_arg));
	tee_optee_ta_uuid_to_octets(g_open_arg.uuid, &uuid);
	ret = tee_open_session(g_dev, &g_open_arg, 0, NULL);
	if (ret)
		printf("tee_open_session() failed, return %d\n", ret);

	return ret == 0;
}

static bool close_session(void)
{
	int ret = tee_close_session(g_dev, g_open_arg.session);

	if (ret)
		printf("tee_close_session() failed, return %d\n", ret);

	return ret == 0;
}

static bool invoke_cmd(uint32_t cmd, int param_num, struct tee_param *param)
{
	int ret = 0;
	struct tee_invoke_arg arg;

	memset(&arg, 0, sizeof(arg));
	arg.func = cmd;
	arg.session = g_open_arg.session;

	ret = tee_invoke_func(g_dev, &arg, param_num, param);
	if (ret || arg.ret != TEE_SUCCESS) {
		printf("tee_invoke_func() failed, ret = %d, arg.ret = 0x%x\n",
				ret, arg.ret);
		return false;
	}

	return true;
}

static bool write_lock_state(int lock_state)
{
	bool ret = false;
	struct tee_param param;

	if (open_session()) {
		memset(&param, 0, sizeof(param));
		param.attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
		param.u.value.a = lock_state;

		ret = invoke_cmd(TA_AVB_CMD_WRITE_LOCK_STATE, 1, &param);

		close_session();
	}

	return ret;
}

bool is_avb_arb_available(void)
{
	// secureboot + rpmb
	return (IS_FEAT_BOOT_VERIFY() == 1) && is_rpmb_available();
}

bool set_avb_antirollback(uint32_t index, uint32_t version)
{
	if (!is_avb_arb_available())
		return false;

	if (avb2_init()) {
		if (g_avb_ops->write_rollback_index(g_avb_ops, index, version)
				== AVB_IO_RESULT_OK) {
			printf("Set avb2 rollback index(loc = %d, idx = %d) success\n",
					index, version);
			return true;
		}
		printf("Set avb2 rollback index failed\n");
		return false;
	}

	return false;
}

bool get_avb_antirollback(uint32_t index, uint32_t *version)
{
	if (!is_avb_arb_available())
		return false;

	if (avb2_init()) {
		uint64_t tmp_version = 0;

		if (g_avb_ops->read_rollback_index(g_avb_ops, index, &tmp_version)
				== AVB_IO_RESULT_OK) {
			*version = tmp_version;
			printf("Get avb2 rollback index(loc = %d, idx = %d) success\n",
					index, *version);
			return true;
		}
		printf("Get avb2 rollback index failed\n");
		return false;
	}

	return false;
}

bool get_avb_lock_state(uint32_t *lock_state)
{
	if (!is_avb_arb_available())
		return false;

	if (avb2_init()) {
		bool unlock = false;

		if (g_avb_ops->read_is_device_unlocked(g_avb_ops, &unlock)
				== AVB_IO_RESULT_OK) {
			*lock_state = unlock ? 0 : 1;
			printf("Get avb2 lock state success, state = %d(%s)\n",
					*lock_state,
					(*lock_state == AVB_LOCK_STATE) ? "lock" : "unlock");
			return true;
		}
		printf("Get avb2 lock state failed\n");
		return false;
	}

	return false;
}

bool avb_lock(void)
{
	if (!is_avb_arb_available())
		return false;

	bool ret = write_lock_state(AVB_LOCK_STATE);

	if (ret)
		printf("avb2 lock success\n");
	else
		printf("avb2 lock failed\n");

	return ret;
}

bool avb_unlock(void)
{
	if (!is_avb_arb_available())
		return false;

	bool ret = write_lock_state(AVB_UNLOCK_STATE);

	if (ret)
		printf("avb2 unlock success\n");
	else
		printf("avb2 unlock failed\n");

	return ret;
}

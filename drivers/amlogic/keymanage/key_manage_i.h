/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __KEY_MANAGE_I_H__
#define __KEY_MANAGE_I_H__

#include <config.h>
#include <common.h>
#include <linux/string.h>
#include <errno.h>
#include <malloc.h>
#include "key_manage.h"
#include <amlogic/keyunify.h>
#include <amlogic/amlkey_if.h>
#include <linux/printk.h>

#define KM_DBG(fmt ...)     //printf("[KM]Dbg:"fmt)
#define KM_MSG(fmt ...)     pr_info("[KM]Msg:" fmt)
#define KM_ERR(fmt ...)     do {pr_err("[KM]Err:L%d:", __LINE__); pr_err(fmt); } while (0)

int _keyman_hex_ascii_to_buf(const char* input, char* buf, const unsigned bufSz);
int _keyman_buf_to_hex_ascii(const uint8_t* pdata, const unsigned dataLen, char* fmtStr/*pr if NULL*/, int fmtSize);

int keymanage_dts_parse(const void* dt_addr);
enum key_manager_df_e keymanage_dts_get_key_fmt(const char *keyname);
enum key_manager_dev_e keymanage_dts_get_key_device(const char *keyname);
const char* keymanage_dts_get_key_type(const char* keyname);
const char* keymanage_dts_get_enc_type(const char* keyname);
char unifykey_get_efuse_version(void);
int unifykey_get_encrypt_type(void);

int keymanage_efuse_init(const char *buf, int len);
int keymange_efuse_exit(void);
int keymanage_efuse_write(const char *keyname, const void* keydata, unsigned int datalen);
int keymanage_efuse_exist(const char* keyname);
ssize_t keymanage_efuse_size(const char* keyname);
int keymanage_efuse_query_can_read(const char* keyname);
int keymanage_efuse_read(const char *keyname, void* databuf, const unsigned bufsz);

int keymanage_securekey_init(const char* buf, int len);
int keymanage_securekey_exit(void);
int keymanage_secukey_write(const char *keyname, const void* keydata, unsigned int datalen);
ssize_t keymanage_secukey_size(const char* keyname);
int keymanage_secukey_exist(const char* keyname);
int keymanage_secukey_can_read(const char* keyname);
int keymanage_secukey_read(const char* keyname, void* databuf,  unsigned buflen);

//provision key ops
int keymanage_provision_init(const char *buf, int len);
int keymanage_provision_exit(void);
int keymanage_provision_write(const char *keyname, const void* keydata, unsigned int datalen);
ssize_t keymanage_provision_size(const char* keyname);
int keymanage_provision_exist(const char* keyname);
int keymanage_provision_query_can_read(const char* keyname);
int keymanage_provision_read(const char *keyname, void* databuf, const unsigned bufSz);

#endif//#ifndef __KEY_MANAGE_I_H__


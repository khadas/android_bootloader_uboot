/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SECURE_STORAGE_H__
#define __SECURE_STORAGE_H__

/* return value*/
#define RET_OK		0
#define RET_EFAIL	1
#define RET_EINVAL	2
#define RET_EMEM	3

#define RET_EUND	-1
#define SMC_UNK		-1


#ifdef CONFIG_SECURE_STORAGE
/* funtion name: secure_storage_write
 * keyname : key name is ascii string
 * keybuf : key buf
 * keylen : key buf len
 * keyattr: Secure/Normal, ...
 *
 * return  0: ok, 0x1fe: no space, other fail
 * */
int32_t secure_storage_write(uint8_t *keyname, uint8_t *keybuf,
				uint32_t keylen, uint32_t keyattr);
int32_t secure_storage_read(uint8_t *keyname, uint8_t *keybuf,
				uint32_t keylen, uint32_t *reallen);
int32_t secure_storage_query(uint8_t *keyname, uint32_t *retval);
int32_t secure_storage_tell(uint8_t *keyname, uint32_t *retval);
int32_t secure_storage_verify(uint8_t *keyname, uint8_t *hashbuf);
int32_t secure_storage_status(uint8_t *keyname, uint32_t *retval);
void *secure_storage_getbuffer(uint32_t *size);
void secure_storage_notifier(void);
void secure_storage_notifier_ex(uint32_t storagesize, uint32_t rsvarg);
int32_t secure_storage_list(uint8_t *listbuf, uint32_t buflen,
				uint32_t *readlen);
int32_t secure_storage_remove(uint8_t *keyname);
void secure_storage_set_info(uint32_t info);
int32_t secure_storage_set_enctype(uint32_t type);
/* return 0: success, -1: fail*/
int32_t secure_storage_get_enctype(void);
/*return -1: no storage, 0: default enc, 1: efuse enc, 2: fixed enc*/
int32_t secure_storage_version(void);
/*return -1: no storage, others: version*/
#endif

#endif

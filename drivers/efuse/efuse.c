// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/arch/io.h>
#include <asm/arch/efuse.h>

char efuse_buf[EFUSE_BYTES] = {0};

ssize_t efuse_read(char *buf, size_t count, loff_t *ppos)
{
	unsigned int pos = *ppos;

	struct efuse_hal_api_arg arg;
	unsigned int retcnt;
	int ret;

	arg.cmd = EFUSE_HAL_API_READ;
	arg.offset = pos;
	arg.size = count;
	arg.buffer_phy = (unsigned long)buf;
	arg.retcnt_phy = (unsigned long)&retcnt;
	ret = meson_trustzone_efuse(&arg);
	if (ret == 0) {
		*ppos += retcnt;
		return retcnt;
	} else{
		return ret;
	}
}

ssize_t efuse_read_cali(char *buf, size_t count, uint32_t offset)
{
#ifdef EFUSE_HAL_API_READ_CALI
	char data[EFUSE_BYTES];
	struct efuse_hal_api_arg arg;
	unsigned long retcnt;
	int ret;
	memset(data, 0, count);
	arg.cmd = EFUSE_HAL_API_READ_CALI;
	arg.offset = offset;
	arg.size = count;
	arg.buffer_phy = (unsigned long)data;
	arg.retcnt_phy = (unsigned long)&retcnt;
	ret = meson_trustzone_efuse(&arg);
	if (ret == 0) {
		memcpy(buf, data, count);
		printf("retcnt=%ld\n", retcnt);
		return (unsigned int)retcnt;
	} else{
		return ret;
	}
#else
	return -1;
#endif
}

int efuse_get_cali_cvbs(void)
{
#ifdef EFUSE_CALI_CVBS
	efuse_cali_t cali_data;
	int ret;

	ret = efuse_read_cali((char *)&(cali_data), EFUSE_CALI_SIZE, 0x00);
	if (ret == EFUSE_CALI_SIZE && cali_data.cvbs_flag)
		return cali_data.cvbs_data;
#endif
	return -1;
}

/*
 * return: >=0:succ and valid data, <0:fail
*/
int efuse_get_cali_item(char *str)
{
#ifdef EFUSE_HAL_API_READ_CALI_ITEM
	int64_t ret;

	extern int64_t meson_trustzone_efuse_caliItem(const char *str);
	ret = meson_trustzone_efuse_caliItem(str);

	return ret;
#else
	return -1;
#endif
}

int efuse_check_pattern_item(char *str)
{
#ifdef EFUSE_HAL_API_CHECKPATTERN_ITEM
	int64_t ret;

	//extern int64_t meson_trustzone_efuse_lockitem(const char *str);
	ret = meson_trustzone_efuse_lockitem(str);
	return ret;
#else
	return -1;
#endif
}

ssize_t efuse_write(const char *buf, size_t count, loff_t *ppos)
{
	unsigned int pos = *ppos;

	if ((pos & 0xffff) >= EFUSE_BYTES)
		return 0;	/* Past EOF */
	if (count > EFUSE_BYTES - pos)
		count = EFUSE_BYTES - pos;
	if (count > EFUSE_BYTES)
		return -1;

	struct efuse_hal_api_arg arg;
	unsigned int retcnt;

	arg.cmd = EFUSE_HAL_API_WRITE;
	arg.offset = pos;
	arg.size = count;
	arg.buffer_phy = (unsigned long)buf;
	arg.retcnt_phy = (unsigned long)&retcnt;
	int ret;

	ret = meson_trustzone_efuse(&arg);
	if (ret == 0) {
		*ppos = retcnt;
		return retcnt;
	} else{
		return ret;
	}
}

int efuse_read_usr(char *buf, size_t count, loff_t *ppos)
{
	char data[EFUSE_BYTES];
	char *pdata = NULL;
	int ret;
	loff_t pos;

	memset(data, 0, count);

	pdata = data;
	pos = *ppos;
	ret = efuse_read(pdata, count, (loff_t *)&pos);

	memcpy(buf, data, count);

	return ret;
}

int efuse_write_usr(char *buf, size_t count, loff_t *ppos)
{
	char data[EFUSE_BYTES];
	char *pdata = NULL;
	char *penc = NULL;
	int ret;
	loff_t pos;

	if (count == 0) {
		printf("data length: 0 is error!\n");
		return -1;
	}

	memset(data, 0, EFUSE_BYTES);
	memset(efuse_buf, 0, EFUSE_BYTES);

	memcpy(data, buf, count);
	pdata = data;
	penc = efuse_buf;

	memcpy(penc, pdata, count);
	pos = *ppos;

	ret = efuse_write(efuse_buf, count, (loff_t *)&pos);

	return ret;
}

uint32_t efuse_get_max(void)
{
	struct efuse_hal_api_arg arg;
	int ret;

	arg.cmd = EFUSE_HAL_API_USER_MAX;

	ret = meson_trustzone_efuse_get_max(&arg);
	if (ret == 0) {
		printf("ERROR: can not get efuse user max bytes!!!\n");
		return -1;
	} else{
		return ret;
	}
}

#ifdef CONFIG_EFUSE_OBJ_API
uint32_t efuse_obj_write(uint32_t obj_id, char *name, uint8_t *buff, uint32_t size)
{
	uint32_t ret;
	efuse_obj_field_t efuseinfo;

	memset(&efuseinfo, 0, sizeof(efuseinfo));
	strncpy(efuseinfo.name, name, sizeof(efuseinfo.name) - 1);
	if (size > sizeof(efuseinfo.data))
		return EFUSE_OBJ_ERR_SIZE;
	efuseinfo.size = size;
	memcpy(efuseinfo.data, buff, efuseinfo.size);
	ret = meson_efuse_obj_write(obj_id, (uint8_t *)&efuseinfo, sizeof(efuseinfo));
	return ret;
}

uint32_t efuse_obj_read(uint32_t obj_id, char *name, uint8_t *buff, uint32_t *size)
{
	uint32_t ret;
	efuse_obj_field_t efuseinfo;

	memset(&efuseinfo, 0, sizeof(efuseinfo));
	strncpy(efuseinfo.name, name, sizeof(efuseinfo.name) - 1);
	*size = sizeof(efuseinfo);
	ret = meson_efuse_obj_read(obj_id, (uint8_t *)&efuseinfo, size);
	memcpy(buff, efuseinfo.data, efuseinfo.size);
	*size = efuseinfo.size;
	return ret;
}

static int hex2bin(char *hex, void *bin, size_t binlen)
{
	int i, c, n1, n2, hexlen, k;

	hexlen = strnlen(hex, 64);
	k = 0;
	n1 = -1;
	n2 = -1;
	for (i = 0; i < hexlen; i++) {
		n2 = n1;
		c = hex[i];
		if (c >= '0' && c <= '9') {
			n1 = c - '0';
		} else if (c >= 'a' && c <= 'f') {
			n1 = c - 'a' + 10;
		} else if (c >= 'A' && c <= 'F') {
			n1 = c - 'A' + 10;
		} else if (c == ' ') {
			n1 = -1;
			continue;
		} else {
			return -1;
		}

		if (n1 >= 0 && n2 >= 0) {
			((uint8_t *)bin)[k] = (n2 << 4) | n1;
			n1 = -1;
			k++;
		}
	}
	return k;
}

uint32_t efuse_obj_set_data(char *name, char *data)
{
	uint32_t ret;
	int dlen = strnlen(data, 64);
	uint8_t databuf[32] = {0};

	dlen = hex2bin(data, databuf, dlen);
	if (dlen < 0) {
		printf("parse data hex2bin error\n");
		return EFUSE_OBJ_ERR_INVALID_DATA;
	}
	ret = efuse_obj_write(EFUSE_OBJ_EFUSE_DATA, name, databuf, dlen);

	return ret;
}

uint32_t efuse_obj_set_license(char *name)
{
	uint32_t ret;
	uint8_t databuf = 0x01;

	ret = efuse_obj_write(EFUSE_OBJ_EFUSE_DATA, name, &databuf, 1);

	return ret;
}

uint32_t efuse_obj_lock(char *name)
{
	uint32_t ret;
	uint8_t databuf = 0x01;

	ret = efuse_obj_write(EFUSE_OBJ_LOCK_STATUS, name, &databuf, 1);

	return ret;
}

efuse_obj_field_t efuse_field;

uint32_t efuse_obj_get_data(char *name)
{
	uint32_t ret;
	uint8_t buff[32];
	uint32_t bufflen = sizeof(buff);

	ret = efuse_obj_read(EFUSE_OBJ_EFUSE_DATA, name, buff, &bufflen);
	if (ret == EFUSE_OBJ_SUCCESS) {
		memset(&efuse_field, 0, sizeof(efuse_field));
		strncpy(efuse_field.name, name, sizeof(efuse_field.name) - 1);
		memcpy(efuse_field.data, buff, bufflen);
		efuse_field.size = bufflen;
	}

	return ret;
}

uint32_t efuse_obj_get_lock(char *name)
{
	uint32_t ret;
	uint8_t buff[32];
	uint32_t bufflen = sizeof(buff);

	ret = efuse_obj_read(EFUSE_OBJ_LOCK_STATUS, name, buff, &bufflen);
	if (ret == EFUSE_OBJ_SUCCESS) {
		memset(&efuse_field, 0, sizeof(efuse_field));
		strncpy(efuse_field.name, name, sizeof(efuse_field.name) - 1);
		memcpy(efuse_field.data, buff, bufflen);
		efuse_field.size = bufflen;
	}

	return ret;
}
#endif /* CONFIG_EFUSE_OBJ_API */

#ifdef CONFIG_EFUSE_MRK_GET_CHECKNUM
uint32_t efuse_mrk_get_checknum(char *name, uint32_t *checknum)
{
	uint32_t ret;
	char mrk_name[16];

	memset(mrk_name, 0, sizeof(mrk_name));
	strncpy(mrk_name, name, sizeof(mrk_name) - 1);
	ret = meson_efuse_mrk_get_checknum(name, checknum);
	return ret;
}
#endif

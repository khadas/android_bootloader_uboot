// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "ini_config.h"

#define LOG_TAG "model"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_proxy.h"
#include "ini_handler.h"
#include "ini_platform.h"
#include "ini_io.h"
#include "model.h"
#include <partition_table.h>

#ifndef CONFIG_YOCTO
#define DEFAULT_MODEL_SUM_PATH1 "/odm/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH1 "/odm/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH1 "/odm/etc/tvconfig/model/model2_sum.ini"
#else
#define DEFAULT_MODEL_SUM_PATH1 "/vendor/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH1 "/vendor/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH1 "/vendor/etc/tvconfig/model/model2_sum.ini"
#endif
#define DEFAULT_MODEL_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model_sum.ini"
#define DEFAULT_MODEL1_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model1_sum.ini"
#define DEFAULT_MODEL2_SUM_PATH2 "/odm_ext/etc/tvconfig/model/model2_sum.ini"
#define AML_START		"amlogic_start"
#define AML_END			"amlogic_end"

#define CC_PARAM_CHECK_OK                             (0)
#define CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM        (-1)
#define CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM    (-2)

int model_debug_flag;

#ifdef CONFIG_AML_LCD
static int glcd_dcnt, glcd_ext_dcnt, gbl_dcnt, glcd_optical_dcnt;
static int g_lcd_pwr_on_seq_cnt, g_lcd_pwr_off_seq_cnt;
#ifdef CONFIG_AML_LCD_BL_LDIM
static int gldim_dev_dcnt, g_ldim_dev_init_on_cnt, g_ldim_dev_init_off_cnt;
static unsigned int g_ldim_dev_valid;
#endif
static int glcd_ext_init_on_cnt, glcd_ext_init_off_cnt, glcd_ext_cmd_size;
static struct lcd_ext_attr_s *lcd_ext_attr;
static unsigned int g_lcd_if, g_lcd_tcon_valid;
#ifdef CONFIG_AML_LCD_TCON
static int gLcdTconDataCnt, gLcdTconSpi_cnt;
static unsigned int g_lcd_tcon_bin_block_cnt;
static unsigned char *g_lcd_tcon_bin_path_mem, *g_lcd_tcon_bin_path_resv_mem;

static int handle_tcon_ext_pmu_data(int index, int flag, unsigned char *buf,
				    unsigned int offset, unsigned int data_len,
				    int mul_flag);
#endif
#endif

int trans_buffer_data(const char *data_str, unsigned int data_buf[])
{
	int item_ind = 0;
	char *token = NULL;
	char *pSave = NULL;
	char *tmp_buf = NULL;

	if (data_str == NULL)
		return 0;

	tmp_buf = (char *) malloc(CC_MAX_TEMP_BUF_SIZE);
	if (tmp_buf == NULL) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}

	memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
	strncpy(tmp_buf, data_str, CC_MAX_TEMP_BUF_SIZE - 1);
	token = plat_strtok_r(tmp_buf, ",", &pSave);
	while (token != NULL) {
		data_buf[item_ind] = strtoul(token, NULL, 0);
		item_ind++;
		token = plat_strtok_r(NULL, ",", &pSave);
	}

	free(tmp_buf);
	tmp_buf = NULL;

	return item_ind;
}

#ifdef CONFIG_AML_LCD
static int check_param_valid(int mode, int parse_len, unsigned char parse_buf[], int ori_len, unsigned char ori_buf[])
{
	unsigned int ori_cal_crc32 = 0, parse_cal_crc32 = 0;
	struct lcd_header_s head, head2;
	unsigned char *p;

	if (mode == 0) {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		if (check_hex_data_have_header_valid(&parse_cal_crc32, CC_MAX_DATA_SIZE, parse_len, parse_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		if (check_hex_data_have_header_valid(&ori_cal_crc32, CC_MAX_DATA_SIZE, ori_len, ori_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32) {
			//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
		}
		// end check parse data valid
	} else if (mode == 1) {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		if (check_hex_data_no_header_valid(&parse_cal_crc32, CC_MAX_DATA_SIZE, parse_len, parse_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		if (check_hex_data_no_header_valid(&ori_cal_crc32, CC_MAX_DATA_SIZE, ori_len, ori_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32) {
			//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
		}
		// end check parse data valid
	} else if (mode == 2) {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		memcpy((void *)&head, (void *)parse_buf, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&parse_cal_crc32, CC_MAX_DATA_SIZE,
		    head.data_len, parse_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		memcpy((void *)&head2, (void *)ori_buf, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&ori_cal_crc32, CC_MAX_DATA_SIZE, head2.data_len,
		    ori_buf) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32)
		//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n",
			//__func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		p = parse_buf + head.data_len;
		memcpy((void *)&head, (void *)p, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&parse_cal_crc32, CC_MAX_DATA_SIZE, head.data_len, p) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		p = parse_buf + head2.data_len;
		memcpy((void *)&head2, (void *)ori_buf, sizeof(struct lcd_header_s));
		if (check_hex_data_have_header_valid
		    (&ori_cal_crc32, CC_MAX_DATA_SIZE, head2.data_len, p) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32)
		//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n",
			//__func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
	} else {
		// start check parse data valid
		//ALOGD("%s, start check parse data valid\n", __func__);
		if (check_string_data_have_header_valid(&parse_cal_crc32, (char *)parse_buf, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN) < 0)
			return CC_PARAM_CHECK_ERROR_NOT_NEED_UPDATE_PARAM;

		// start check flash key data valid
		//ALOGD("%s, start check flash key data valid\n", __func__);
		if (check_string_data_have_header_valid(&ori_cal_crc32, (char *)ori_buf, CC_HEAD_CHKSUM_LEN, CC_VERSION_LEN) < 0)
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;

		if (parse_cal_crc32 != ori_cal_crc32) {
			//ALOGE("%s, parse data not equal flash data(0x%08X, 0x%08X)\n", __func__, parse_cal_crc32, ori_cal_crc32);
			return CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM;
		}
		// end check parse data valid
	}

	//ALOGD("%s, param check ok!\n", __func__);
	return CC_PARAM_CHECK_OK;
}

static int handle_integrity_flag(void)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("start", "start_tag", "null");
	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s, start_tag is (%s)\n", __func__, ini_value);
	if (strncasecmp(ini_value, AML_START, strlen(AML_START))) {
		ALOGE("%s, start_tag (%s) is error!!!\n", __func__, ini_value);
		return -1;
	}

	ini_value = IniGetString("end", "end_tag", "null");
	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s, end_tag is (%s)\n", __func__, ini_value);
	if (strncasecmp(ini_value, AML_END, strlen(AML_END))) {
		ALOGE("%s, end_tag (%s) is error!!!\n", __func__, ini_value);
		return -1;
	}

	return 0;
}

void *handle_lcd_ext_buf_get(void)
{
	return (void *)lcd_ext_attr;
}

#ifdef CONFIG_AML_LCD_TCON
static unsigned int handle_tcon_char_data_size_align(unsigned int size)
{
	unsigned int new_size;

	if (size % 4)
		new_size = (size / 4 + 1) * 4;
	else
		new_size = size;

	return new_size;
}

void *handle_tcon_path_mem_get(unsigned int size)
{
	unsigned int data_size = 0;

	if (!g_lcd_tcon_bin_path_mem) {
		ALOGE("%s, buf is null\n", __func__);
		return NULL;
	}

	data_size = g_lcd_tcon_bin_path_mem[4] |
		(g_lcd_tcon_bin_path_mem[5] << 8) |
		(g_lcd_tcon_bin_path_mem[6] << 16) |
		(g_lcd_tcon_bin_path_mem[7] << 24);
	if (data_size > size) {
		ALOGE("%s, buf size invalid\n", __func__);
		return NULL;
	}

	return g_lcd_tcon_bin_path_mem;
}

void *handle_tcon_path_resv_mem_get(unsigned int size)
{
	unsigned int data_size = 0;

	if (!g_lcd_tcon_bin_path_resv_mem) {
		ALOGE("%s, buf is null\n", __func__);
		return NULL;
	}

	data_size = g_lcd_tcon_bin_path_resv_mem[4] |
		(g_lcd_tcon_bin_path_resv_mem[5] << 8) |
		(g_lcd_tcon_bin_path_resv_mem[6] << 16) |
		(g_lcd_tcon_bin_path_resv_mem[7] << 24);
	if (data_size > size) {
		ALOGE("%s, buf size invalid\n", __func__);
		return NULL;
	}

	return g_lcd_tcon_bin_path_resv_mem;
}

static char *handle_tcon_path_file_name_get(unsigned int index)
{
	unsigned int n;
	char *str;

	if (!g_lcd_tcon_bin_path_mem) {
		ALOGE("%s, tcon_path buf is null\n", __func__);
		return NULL;
	}

	if (index >= g_lcd_tcon_bin_block_cnt) {
		ALOGE("%s, invalid index %d\n", __func__, index);
		return NULL;
	}

	n = 32 + (index * 256) + 4;
	str = (char *)&g_lcd_tcon_bin_path_mem[n];
	return str;
}

static int handle_tcon_path_default(unsigned int version)
{
	unsigned char *buf;
	char str[30];
	const char *ini_value = NULL;
	unsigned int temp, i, n, block_cnt, data_size, crc32;

	/* tcon data bin path */
	g_lcd_tcon_bin_path_mem = (unsigned char *) malloc(CC_MAX_TCON_BIN_PATH_SIZE);
	if (!g_lcd_tcon_bin_path_mem) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset(g_lcd_tcon_bin_path_mem, 0, CC_MAX_TCON_BIN_PATH_SIZE);
	buf = g_lcd_tcon_bin_path_mem;

	/* version */
	buf[8] = version & 0xff;
	buf[9] = (version >> 8) & 0xff;
	buf[10] = (version >> 16) & 0xff;
	buf[11] = (version >> 24) & 0xff;

	/* data_load_level */
	ini_value = IniGetString("tcon_Path", "data_load_level", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, data_load_level is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	buf[12] = temp & 0xff;
	buf[13] = (temp >> 8) & 0xff;
	buf[14] = (temp >> 16) & 0xff;
	buf[15] = (temp >> 24) & 0xff;

	ini_value = IniGetString("tcon_Path", "init_load", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, init_load is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	buf[20] = temp & 0xff;

	block_cnt = 0;
	n = 32;

	if (version == 0) {/* tcon data bin: old data format */
		ini_value = IniGetString("tcon_Path", "TCON_VAC_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no vac ini file\n", __func__);
		}
		env_set("model_tcon_vac", ini_value);
		strncpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = IniGetString("tcon_Path", "TCON_DEMURA_SET_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_set file\n", __func__);
		}
		env_set("model_tcon_demura_set", ini_value);
		strncpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = IniGetString("tcon_Path", "TCON_DEMURA_LUT_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_lut file\n", __func__);
		}
		env_set("model_tcon_demura_lut", ini_value);
		strncpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = IniGetString("tcon_Path", "TCON_ACC_LUT_PATH", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no acc_lut file\n", __func__);
		}
		env_set("model_tcon_acc_lut", ini_value);
		strncpy((char *)&buf[n + 4], ini_value, 256);

		/* block cnt */
		block_cnt = 4;
		buf[16] = block_cnt & 0xff;
		buf[17] = (block_cnt >> 8) & 0xff;
		buf[18] = (block_cnt >> 16) & 0xff;
		buf[19] = (block_cnt >> 24) & 0xff;
	} else {/* tcon data bin: new data format */
		for (i = 0; i < 32; i++) {
			snprintf(str, 30, "TCON_DATA_%d_BIN_PATH", i);
			ini_value = IniGetString("tcon_Path", str, "null");
			if (strcmp(ini_value, "null") == 0)
				break;

			if (model_debug_flag & DEBUG_TCON) {
				ALOGD("%s, tcon_path %d is (%s)\n",
					__func__, i, ini_value);
			}
			strncpy((char *)&buf[n + 4], ini_value, 252);
			block_cnt++;
			n += 256;
		}

		/* block cnt */
		buf[16] = block_cnt & 0xff;
		buf[17] = (block_cnt >> 8) & 0xff;
		buf[18] = (block_cnt >> 16) & 0xff;
		buf[19] = (block_cnt >> 24) & 0xff;
	}

	/* g_lcd_tcon_bin_block_cnt default for uboot load */
	g_lcd_tcon_bin_block_cnt = block_cnt;

	/* data size */
	data_size = 32 + block_cnt * 256;
	buf[4] = data_size & 0xff;
	buf[5] = (data_size >> 8) & 0xff;
	buf[6] = (data_size >> 16) & 0xff;
	buf[7] = (data_size >> 24) & 0xff;

	/* data check */
	crc32 = CalCRC32(0, &buf[4], (data_size - 4));
	buf[0] = crc32 & 0xff;
	buf[1] = (crc32 >> 8) & 0xff;
	buf[2] = (crc32 >> 16) & 0xff;
	buf[3] = (crc32 >> 24) & 0xff;

	return 0;
}

static int handle_tcon_path_resv_for_kernel(unsigned int version)
{
	unsigned char *buf;
	char str[30];
	const char *ini_value = NULL;
	unsigned int temp, i, n, block_cnt, data_size, crc32;

	g_lcd_tcon_bin_path_resv_mem = (unsigned char *)malloc(CC_MAX_TCON_BIN_PATH_SIZE);
	if (!g_lcd_tcon_bin_path_resv_mem) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset(g_lcd_tcon_bin_path_resv_mem, 0, CC_MAX_TCON_BIN_PATH_SIZE);
	buf = g_lcd_tcon_bin_path_resv_mem;

	/* detect tcon_path resv_for_kernel exist or not */
	ini_value = IniGetString("tcon_Path", "TCON_BIN_PATH_K", "null");
	if (!strcmp(ini_value, "null")) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, PATH_K not exist, use default path\n", __func__);
		return -1;
	}

	/* version */
	buf[8] = version & 0xff;
	buf[9] = (version >> 8) & 0xff;
	buf[10] = (version >> 16) & 0xff;
	buf[11] = (version >> 24) & 0xff;

	/* data_load_level */
	ini_value = IniGetString("tcon_Path", "data_load_level", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, data_load_level is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	buf[12] = temp & 0xff;
	buf[13] = (temp >> 8) & 0xff;
	buf[14] = (temp >> 16) & 0xff;
	buf[15] = (temp >> 24) & 0xff;

	ini_value = IniGetString("tcon_Path", "init_load", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, init_load is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	buf[20] = temp & 0xff;

	block_cnt = 0;
	n = 32;

	if (version == 0) {/* tcon data bin: old data format */
		ini_value = IniGetString("tcon_Path", "TCON_VAC_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no vac ini file\n", __func__);
		}
		strncpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = IniGetString("tcon_Path", "TCON_DEMURA_SET_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_set file\n", __func__);
		}
		strncpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = IniGetString("tcon_Path", "TCON_DEMURA_LUT_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no demura_lut file\n", __func__);
		}
		strncpy((char *)&buf[n + 4], ini_value, 256);
		n += 256;

		ini_value = IniGetString("tcon_Path", "TCON_ACC_LUT_PATH_K", "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no acc_lut file\n", __func__);
		}
		strncpy((char *)&buf[n + 4], ini_value, 256);

		/* block cnt */
		block_cnt = 4;
		buf[16] = block_cnt & 0xff;
		buf[17] = (block_cnt >> 8) & 0xff;
		buf[18] = (block_cnt >> 16) & 0xff;
		buf[19] = (block_cnt >> 24) & 0xff;
	} else {/* tcon data bin: new data format */
		for (i = 0; i < 32; i++) {
			snprintf(str, 30, "TCON_DATA_%d_BIN_PATH_K", i);
			ini_value = IniGetString("tcon_Path", str, "null");
			if (strcmp(ini_value, "null") == 0)
				break;

			if (model_debug_flag & DEBUG_TCON) {
				ALOGD("%s, tcon_path %d is (%s)\n",
					__func__, i, ini_value);
			}
			strncpy((char *)&buf[n + 4], ini_value, 252);
			block_cnt++;
			n += 256;
		}

		/* block cnt */
		buf[16] = block_cnt & 0xff;
		buf[17] = (block_cnt >> 8) & 0xff;
		buf[18] = (block_cnt >> 16) & 0xff;
		buf[19] = (block_cnt >> 24) & 0xff;
	}

	/* data size */
	data_size = 32 + block_cnt * 256;
	buf[4] = data_size & 0xff;
	buf[5] = (data_size >> 8) & 0xff;
	buf[6] = (data_size >> 16) & 0xff;
	buf[7] = (data_size >> 24) & 0xff;

	/* data check */
	crc32 = CalCRC32(0, &buf[4], (data_size - 4));
	buf[0] = crc32 & 0xff;
	buf[1] = (crc32 >> 8) & 0xff;
	buf[2] = (crc32 >> 16) & 0xff;
	buf[3] = (crc32 >> 24) & 0xff;

	return 0;
}

static int handle_tcon_path(void)
{
	char str[50], env_str[50];
	const char *ini_value = NULL;
	unsigned int version, header;
	int i, j, ret;

	/* version */
	ini_value = IniGetString("tcon_Path", "version", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	version = strtoul(ini_value, NULL, 0);

	/* tcon_bin_header */
	ini_value = IniGetString("tcon_Path", "header", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, header is (%s)\n", __func__, ini_value);
	header = strtoul(ini_value, NULL, 0);
	snprintf(str, 50, "%d", header);
	env_set("model_tcon_bin_header", str);

	/* tcon regs bin */
	ini_value = IniGetString("tcon_Path", "TCON_BIN_PATH", "null");
	if (!strcmp(ini_value, "null")) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGE("%s, tcon bin load file error!\n", __func__);
	}
	env_set("model_tcon", ini_value);

	handle_tcon_path_default(version);
	ret = handle_tcon_path_resv_for_kernel(version);
	if (ret) {
		if (g_lcd_tcon_bin_path_resv_mem && g_lcd_tcon_bin_path_mem) {
			memcpy(g_lcd_tcon_bin_path_resv_mem,
			       g_lcd_tcon_bin_path_mem,
			       CC_MAX_TCON_BIN_PATH_SIZE);
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, tcon bin path kernel same as uboot\n", __func__);
		}
	}

	/* pmu bin */
	for (i = 0; i < 4; i++) {
		snprintf(str, 50, "TCON_EXT_B%d_BIN_PATH", i);
		snprintf(env_str, 50, "model_tcon_ext_b%d", i);
		ini_value = IniGetString("tcon_Path", str, "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no %s file\n", __func__, str);
			goto handle_tcon_path_pmu_bin_multi;
		}
		env_set(env_str, ini_value);
	}

handle_tcon_path_pmu_bin_multi:
	for (j = 0; j < 10; j++) {
		snprintf(str, 50, "TCON_EXT_B%d_%d_BIN_PATH", i, j);
		snprintf(env_str, 50, "model_tcon_ext_b%d_%d", i, j);
		ini_value = IniGetString("tcon_Path", str, "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no %s file\n", __func__, str);
			continue;
		}
		env_set(env_str, ini_value);
	}

	for (i = 0; i < 4; i++) {
		snprintf(str, 50, "TCON_EXT_B%d_SPI_BIN_PATH", i);
		snprintf(env_str, 50, "model_tcon_ext_b%d_spi", i);
		ini_value = IniGetString("tcon_Path", str, "null");
		if (!strcmp(ini_value, "null")) {
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, no %s file\n", __func__, str);
			goto handle_tcon_path_pmu_spi_bin_multi;
		}
		env_set(env_str, ini_value);
	}

handle_tcon_path_pmu_spi_bin_multi:
	for (j = 0; j < 10; j++) {
		snprintf(str, 50, "TCON_EXT_B%d_%d_SPI_BIN_PATH", i, j);
			snprintf(env_str, 50, "model_tcon_ext_b%d_%d_spi", i, j);
			ini_value = IniGetString("tcon_Path", str, "null");
			if (!strcmp(ini_value, "null")) {
				if (model_debug_flag & DEBUG_TCON)
					ALOGD("%s, no %s file\n", __func__, str);
				continue;
			}
			env_set(env_str, ini_value);
	}

	return 0;
}
#endif

static int handle_lcd_basic(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int config_chk;
	unsigned int bits, cfmt;

	ini_value = IniGetString("lcd_Attr", "model_name", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, model_name is (%s)\n", __func__, ini_value);
	strncpy(p_attr->basic.model_name, ini_value, CC_LCD_NAME_LEN_MAX - 1);
	p_attr->basic.model_name[CC_LCD_NAME_LEN_MAX - 1] = '\0';

	ini_value = IniGetString("lcd_Attr", "interface", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, interface is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LCD_RGB") == 0)
		g_lcd_if = LCD_RGB;
	else if (strcmp(ini_value, "LCD_LVDS") == 0)
		g_lcd_if = LCD_LVDS;
	else if (strcmp(ini_value, "LCD_VBYONE") == 0)
		g_lcd_if = LCD_VBYONE;
	else if (strcmp(ini_value, "LCD_MIPI") == 0)
		g_lcd_if = LCD_MIPI;
	else if (strcmp(ini_value, "LCD_MLVDS") == 0)
		g_lcd_if = LCD_MLVDS;
	else if (strcmp(ini_value, "LCD_P2P") == 0)
		g_lcd_if = LCD_P2P;
	else if (strcmp(ini_value, "LCD_EDP") == 0)
		g_lcd_if = LCD_EDP;
	else if (strcmp(ini_value, "LCD_BT656") == 0)
		g_lcd_if = LCD_BT656;
	else if (strcmp(ini_value, "LCD_BT1120") == 0)
		g_lcd_if = LCD_BT1120;
	else
		g_lcd_if = LCD_TYPE_MAX;

	ini_value = IniGetString("lcd_Attr", "config_check", "none");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, config_check is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "none") == 0)
		config_chk = 0;
	else
		config_chk = strtoul(ini_value, NULL, 0) ? 0x3 : 0x2;
	p_attr->basic.lcd_if_chk = (g_lcd_if & 0x3f) | ((config_chk & 0x3) << 6);

	ini_value = IniGetString("lcd_Attr", "lcd_bits", "10");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, lcd_bits is (%s)\n", __func__, ini_value);
	bits = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "cmft_in", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, cmft_in is (%s)\n", __func__, ini_value);
	cfmt = strtoul(ini_value, NULL, 0);
	p_attr->basic.lcd_bits_cfmt = ((cfmt & 0x3) << 6) | (bits & 0x3f);

	ini_value = IniGetString("lcd_Attr", "screen_width", "16");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, screen_width is (%s)\n", __func__, ini_value);
	p_attr->basic.screen_width = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "screen_height", "9");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, screen_height is (%s)\n", __func__, ini_value);
	p_attr->basic.screen_height = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_timming(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int width, pol;

	ini_value = IniGetString("lcd_Attr", "h_active", "1920");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_active is (%s)\n", __func__, ini_value);
	p_attr->timming.h_active = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "v_active", "1080");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_active is (%s)\n", __func__, ini_value);
	p_attr->timming.v_active = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "h_period", "2200");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_period is (%s)\n", __func__, ini_value);
	p_attr->timming.h_period = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "v_period", "1125");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_period is (%s)\n", __func__, ini_value);
	p_attr->timming.v_period = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "hsync_width", "44");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, hsync_width is (%s)\n", __func__, ini_value);
	width = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "hsync_bp", "148");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, hsync_bp is (%s)\n", __func__, ini_value);
	p_attr->timming.hsync_bp = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "hsync_pol", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, hsync_pol is (%s)\n", __func__, ini_value);
	pol = strtoul(ini_value, NULL, 0);
	p_attr->timming.hsync_width_pol = ((pol & 0xf) << 12) | (width & 0xfff);

	ini_value = IniGetString("lcd_Attr", "vsync_width", "5");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vsync_width is (%s)\n", __func__, ini_value);
	width = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "vsync_bp", "30");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vsync_bp is (%s)\n", __func__, ini_value);
	p_attr->timming.vsync_bp = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "vsync_pol", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vsync_pol is (%s)\n", __func__, ini_value);
	pol = strtoul(ini_value, NULL, 0);
	p_attr->timming.vsync_width_pol = ((pol & 0xf) << 12) | (width & 0xfff);

	ini_value = IniGetString("lcd_Attr", "pre_de_h", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pre_de_h is (%s)\n", __func__, ini_value);
	p_attr->timming.pre_de_h = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "pre_de_v", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pre_de_v is (%s)\n", __func__, ini_value);
	p_attr->timming.pre_de_v = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_customer(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned char clk_auto, clk_mode, ppc, custom_pinmux;

	ini_value = IniGetString("lcd_Attr", "fr_adjust_type", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, fr_adjust_type is (%s)\n", __func__, ini_value);
	p_attr->customer.fr_adjust_type = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "ss_level", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, ss_level is (%s)\n", __func__, ini_value);
	p_attr->customer.ss_level = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "clk_auto_gen", "1");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, clk_auto_gen is (%s)\n", __func__, ini_value);
	clk_auto = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "clk_mode", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, clk_mode is (%s)\n", __func__, ini_value);
	clk_mode = strtoul(ini_value, NULL, 0);
	p_attr->customer.custom_val0 = ((clk_mode & 0xf) << 4) | (clk_auto & 0xf);

	ini_value = IniGetString("lcd_Attr", "pixel_clk", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pixel_clk is (%s)\n", __func__, ini_value);
	p_attr->customer.pixel_clk = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "h_period_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_period_min is (%s)\n", __func__, ini_value);
	p_attr->customer.h_period_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "h_period_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, h_period_max is (%s)\n", __func__, ini_value);
	p_attr->customer.h_period_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "v_period_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_period_min is (%s)\n", __func__, ini_value);
	p_attr->customer.v_period_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "v_period_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, v_period_max is (%s)\n", __func__, ini_value);
	p_attr->customer.v_period_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "pixel_clk_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pixel_clk_min is (%s)\n", __func__, ini_value);
	p_attr->customer.pixel_clk_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "pixel_clk_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, pixel_clk_max is (%s)\n", __func__, ini_value);
	p_attr->customer.pixel_clk_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "vlock_val_0", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_0 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "vlock_val_1", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_1 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "vlock_val_2", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_2 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "vlock_val_3", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, vlock_val_3 is (%s)\n", __func__, ini_value);
	p_attr->customer.vlock_val_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "custom_pinmux", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, custom_pinmux is (%s)\n", __func__, ini_value);
	custom_pinmux = strtoul(ini_value, NULL, 0);
	if (custom_pinmux == 0) {
		ini_value = IniGetString("lcd_Attr", "customer_value_9", "0");
		if (model_debug_flag & DEBUG_LCD)
			ALOGD("%s, customer_value_9 is (%s)\n", __func__, ini_value);
		custom_pinmux = strtoul(ini_value, NULL, 0);
	}

	ini_value = IniGetString("lcd_Attr", "ppc_mode", "1");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, ppc_mode is (%s)\n", __func__, ini_value);
	ppc = strtoul(ini_value, NULL, 0);
	p_attr->customer.custom_val1 = ((ppc & 0xf) << 4) | (custom_pinmux & 0xf);

	ini_value = IniGetString("lcd_Attr", "fr_auto_custom", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, fr_auto_custom is (%s)\n", __func__, ini_value);
	p_attr->customer.fr_auto_cus = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "frame_rate_min", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, frame_rate_min is (%s)\n", __func__, ini_value);
	p_attr->customer.frame_rate_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "frame_rate_max", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, frame_rate_max is (%s)\n", __func__, ini_value);
	p_attr->customer.frame_rate_max = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_interface(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("lcd_Attr", "if_attr_0", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_1", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_2", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_3", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_4", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_5", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_6", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_7", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_8", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "if_attr_9", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, if_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_9 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_pwr(struct lcd_attr_s *p_attr)
{
	int i = 0, tmp_cnt = 0, tmp_base_ind = 0;
	const char *ini_value = NULL;
	unsigned int tmp_buf[1024];

	ini_value = IniGetString("lcd_Attr", "power_on_step", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, power_on_step is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf + 0);
	g_lcd_pwr_on_seq_cnt = tmp_cnt / CC_LCD_PWR_ITEM_CNT;
	for (i = 0; i < g_lcd_pwr_on_seq_cnt; i++) {
		tmp_base_ind = i * CC_LCD_PWR_ITEM_CNT;
		p_attr->pwr[i].pwr_step_type = tmp_buf[tmp_base_ind + 0];
		p_attr->pwr[i].pwr_step_index = tmp_buf[tmp_base_ind + 1];
		p_attr->pwr[i].pwr_step_val = tmp_buf[tmp_base_ind + 2];
		p_attr->pwr[i].pwr_step_delay = tmp_buf[tmp_base_ind + 3];
	}

	ini_value = IniGetString("lcd_Attr", "power_off_step", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, power_off_step is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf + tmp_cnt);
	g_lcd_pwr_off_seq_cnt = tmp_cnt / CC_LCD_PWR_ITEM_CNT;
	for (i = 0; i < g_lcd_pwr_off_seq_cnt; i++) {
		tmp_base_ind = (g_lcd_pwr_on_seq_cnt + i)* CC_LCD_PWR_ITEM_CNT;
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_type = tmp_buf[tmp_base_ind + 0];
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_index = tmp_buf[tmp_base_ind + 1];
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_val = tmp_buf[tmp_base_ind + 2];
		p_attr->pwr[i + g_lcd_pwr_on_seq_cnt].pwr_step_delay = tmp_buf[tmp_base_ind + 3];
	}

	return 0;
}

static int handle_lcd_header(struct lcd_attr_s *p_attr)
{
	const char *ini_value = NULL;

	glcd_dcnt = 0;
	glcd_dcnt += sizeof(struct lcd_header_s);
	glcd_dcnt += sizeof(struct lcd_basic_s);
	glcd_dcnt += sizeof(struct lcd_timming_s);
	glcd_dcnt += sizeof(struct lcd_customer_s);
	glcd_dcnt += sizeof(struct lcd_interface_s);

	glcd_dcnt += sizeof(struct lcd_pwr_s) * g_lcd_pwr_on_seq_cnt;
	glcd_dcnt += sizeof(struct lcd_pwr_s) * g_lcd_pwr_off_seq_cnt;

	p_attr->head.data_len = glcd_dcnt;
	p_attr->head.block_cur_size = glcd_dcnt;

	ini_value = IniGetString("lcd_Attr", "version", "null");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	//p_attr->head.rev = 0;
	if (p_attr->head.version >= 2)
		p_attr->head.block_next_flag = 1;

	return 0;
}

static int handle_lcd_phy(struct lcd_v2_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int reg_buf[216];
	int reg_cnt = 0;
	int i, j = 0;

	ini_value = IniGetString("lcd_Attr", "phy_attr_flag", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_flag is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_flag = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_0", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_1", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_2", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_3", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_4", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_5", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_6", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_7", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_8", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_9", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_9 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_10", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_10 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_10 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_attr_11", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_attr_11 is (%s)\n", __func__, ini_value);
	p_attr->phy.phy_attr_11 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_Attr", "phy_lane_pn_swap", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_lane_pn_swap is (%s)\n", __func__, ini_value);
	j += reg_cnt;
	reg_cnt = trans_buffer_data(ini_value, reg_buf + 0);
	for (i = 0; i < 4; i++)
		p_attr->phy.phy_lane_pn_swap[i] = reg_buf[i];

	ini_value = IniGetString("lcd_Attr", "phy_lane_ctrl", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_lane_ctrl is (%s)\n", __func__, ini_value);
	j += reg_cnt;
	reg_cnt = trans_buffer_data(ini_value, reg_buf + reg_cnt);
	for (i = 0; i < reg_cnt; i++) {
		p_attr->phy.phy_lane_ctrl[i] = reg_buf[i + j];
		if (model_debug_flag & DEBUG_LCD) {
			ALOGD("%s, phy_lane_ctrl[%d] is (0x%x)\n", __func__,
				i, p_attr->phy.phy_lane_ctrl[i]);
		}
	}

	ini_value = IniGetString("lcd_Attr", "phy_lane_swap", "0");
	if (model_debug_flag & DEBUG_LCD)
		ALOGD("%s, phy_lane_swap is (%s)\n", __func__, ini_value);
	j += reg_cnt;
	reg_cnt = trans_buffer_data(ini_value, reg_buf + reg_cnt);
	for (i = 0; i < reg_cnt; i++)
		p_attr->phy.phy_lane_swap[i] = reg_buf[i + j];

	return 0;
}

static int handle_lcd_v2_header(struct lcd_v2_attr_s *p_attr)
{
	unsigned int data_cnt;

	data_cnt = 0;
	data_cnt += sizeof(struct lcd_header_s);
	data_cnt += sizeof(struct lcd_phy_s);
	data_cnt += glcd_cus_ctrl_cnt;

	p_attr->head.crc32 = 0xffffffff;
	p_attr->head.data_len = 0;
	p_attr->head.version = 2;
	p_attr->head.block_next_flag = 0;
	p_attr->head.block_cur_size = data_cnt;

	return 0;
}

static int handle_lcd_ext_basic(struct lcd_ext_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("lcd_ext_Attr", "ext_name", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_name is (%s)\n", __func__, ini_value);
	strncpy(p_attr->basic.ext_name, ini_value, CC_LCD_EXT_NAME_LEN_MAX - 1);
	p_attr->basic.ext_name[CC_LCD_EXT_NAME_LEN_MAX - 1] = '\0';

	ini_value = IniGetString("lcd_ext_Attr", "ext_index", "0xff");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_index is (%s)\n", __func__, ini_value);
	p_attr->basic.ext_index = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "ext_type", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_type is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LCD_EXTERN_I2C") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_I2C;
	else if (strcmp(ini_value, "LCD_EXTERN_SPI") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_SPI;
	else if (strcmp(ini_value, "LCD_EXTERN_MIPI") == 0)
		p_attr->basic.ext_type = LCD_EXTERN_MIPI;
	else
		p_attr->basic.ext_type = LCD_EXTERN_MAX;

	ini_value = IniGetString("lcd_ext_Attr", "ext_status", "0");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, ext_status is (%s)\n", __func__, ini_value);
	p_attr->basic.ext_status = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_ext_type(struct lcd_ext_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("lcd_ext_Attr", "value_0", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_0 is (%s)\n", __func__, ini_value);
	p_attr->type.value_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_1", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_1 is (%s)\n", __func__, ini_value);
	p_attr->type.value_1 = strtoul(ini_value, NULL, 0);

	if (p_attr->basic.ext_type == LCD_EXTERN_I2C)
		p_attr->type.value_2 = LCD_EXTERN_I2C_BUS_INVALID;
	else {
		ini_value = IniGetString("lcd_ext_Attr", "value_2", "null");
		if (model_debug_flag & DEBUG_LCD_EXTERN)
			ALOGD("%s, value_2 is (%s)\n", __func__, ini_value);
		p_attr->type.value_2 = strtoul(ini_value, NULL, 0);
	}

	ini_value = IniGetString("lcd_ext_Attr", "value_3", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_3 is (%s)\n", __func__, ini_value);
	p_attr->type.value_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_4", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_4 is (%s)\n", __func__, ini_value);
	p_attr->type.value_4 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_5", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_5 is (%s)\n", __func__, ini_value);
	p_attr->type.value_5 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_6", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_6 is (%s)\n", __func__, ini_value);
	p_attr->type.value_6 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_7", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_7 is (%s)\n", __func__, ini_value);
	p_attr->type.value_7 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_8", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_8 is (%s)\n", __func__, ini_value);
	p_attr->type.value_8 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_ext_Attr", "value_9", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, value_9 is (%s)\n", __func__, ini_value);
	p_attr->type.value_9 = strtoul(ini_value, NULL, 0);

	if (p_attr->basic.ext_type == LCD_EXTERN_I2C)
		glcd_ext_cmd_size = p_attr->type.value_3;
	else if (p_attr->basic.ext_type == LCD_EXTERN_SPI)
		glcd_ext_cmd_size = p_attr->type.value_6;
	else
		glcd_ext_cmd_size = p_attr->type.value_9;

	return 0;
}

#ifdef CONFIG_AML_LCD_TCON
static int handle_lcd_ext_cmd_type_flag(unsigned int type, unsigned int sub_type)
{
	int flag = 0;

	if (type == 0xa) {
		flag = 1;
	} else if (type == 0xb) {
		flag = 2;
	} else if (type == 0xd) {
		flag = 3;
	} else if (type == 0xe) {
		if (sub_type == 0xa)
			flag = 4;
		else if (sub_type == 0xb)
			flag = 5;
		else if (sub_type == 0xd)
			flag = 6;
		else if (sub_type == 0xc)
			flag = 0;
	}
	return flag;
}
#endif

static int handle_lcd_ext_cmd_data(struct lcd_ext_attr_s *p_attr)
{
	int i = 0, j = 0, k, tmp_cnt = 0, tmp_off = 0;
	const char *ini_value = NULL;
	unsigned int tmp_buf[2048];
	unsigned char raw_size, data_size;
#ifdef CONFIG_AML_LCD_TCON
	unsigned char type, sub_type;
	unsigned char *data_buf = NULL;
	unsigned int n, multi_id;
	unsigned int offset = 0, data_len = 0;
	unsigned int flag = 0;
	int ret;
#endif

	/* orignal data in ini */
	ini_value = IniGetString("lcd_ext_Attr", "init_on", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, init_on is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);

#ifdef CONFIG_AML_LCD_TCON
	data_buf = (unsigned char *)malloc(LCD_EXTERN_INIT_ON_MAX);
	if (data_buf == NULL) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset(data_buf, 0, LCD_EXTERN_INIT_ON_MAX);
#endif

	/* init on */
	if (tmp_cnt > LCD_EXTERN_INIT_ON_MAX) {
		ALOGE("%s: invalid init_on data\n", __func__);
		p_attr->cmd_data[0] = LCD_EXTERN_INIT_END;
		p_attr->cmd_data[1] = 0;
		glcd_ext_init_on_cnt = 2;
		goto handle_lcd_ext_cmd_data_inif_off;
	}

	if (glcd_ext_cmd_size == 0xff) {
		i = 0;
		j = 0;
		while (i < tmp_cnt) {
			p_attr->cmd_data[j] = tmp_buf[i];
			raw_size = tmp_buf[i + 1];
			data_size = raw_size;
			if (p_attr->cmd_data[j] == LCD_EXTERN_INIT_END) {
				p_attr->cmd_data[j + 1] = 0;
				j += 2;
				break;
			}

#ifdef CONFIG_AML_LCD_TCON
			type = tmp_buf[i] >> 4 & 0xf;
			if (type == 0xe) {
				sub_type = tmp_buf[i + 3] & 0xf;
				multi_id = tmp_buf[i + 2];
			} else {
				sub_type = 0xff;
				multi_id = 0xff;
			}
			flag = handle_lcd_ext_cmd_type_flag(type, sub_type);
			if (flag) {
				if (flag == 1) {
					data_len = tmp_buf[i + 1];
					offset = tmp_buf[i + 2];
				} else if (flag == 4) {
					data_len = tmp_buf[i + 1] - 2;
					offset = tmp_buf[i + 4];
				}
				n = p_attr->cmd_data[j] & 0xf;
				memset(data_buf, 0, LCD_EXTERN_INIT_ON_MAX);
				ret = handle_tcon_ext_pmu_data(n, flag, data_buf,
					offset, data_len, multi_id);
				if (ret || data_buf[0] == 0) /* bin data size invalid */
					goto handle_lcd_ext_cmd_data_original;
				switch (flag) {
				case 1:
				case 2:
				case 3:
					data_size = data_buf[0];
					p_attr->cmd_data[j + 1] = data_size;
					memcpy(&p_attr->cmd_data[j + 2],
						&data_buf[1], data_buf[0]);
					goto handle_lcd_ext_cmd_data_next;
				case 4:
				case 5:
				case 6:
					data_size = data_buf[0] + 2;
					p_attr->cmd_data[j + 1] = data_size;
					p_attr->cmd_data[j + 2] = tmp_buf[i + 2];
					p_attr->cmd_data[j + 3] = tmp_buf[i + 3];
					memcpy(&p_attr->cmd_data[j + 4],
						&data_buf[1], data_buf[0]);
					goto handle_lcd_ext_cmd_data_next;
				default:
					break;
				}
			}
handle_lcd_ext_cmd_data_original:
#endif
			/* original ini data */
			p_attr->cmd_data[j + 1] = data_size;
			for (k = 0; k < data_size; k++) {
				p_attr->cmd_data[j + 2 + k] =
					(unsigned char)tmp_buf[i + 2 + k];
			}
#ifdef CONFIG_AML_LCD_TCON
handle_lcd_ext_cmd_data_next:
#endif
			j += data_size + 2;
			i += raw_size + 2; /* raw data */
		}
		glcd_ext_init_on_cnt = j;
	} else {
		for (i = 0; i < tmp_cnt; i++)
			p_attr->cmd_data[i] = tmp_buf[i];
		glcd_ext_init_on_cnt = tmp_cnt;
	}

handle_lcd_ext_cmd_data_inif_off:
	/* init off */
	tmp_off = glcd_ext_init_on_cnt;
	ini_value = IniGetString("lcd_ext_Attr", "init_off", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, init_off is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	if (tmp_cnt > LCD_EXTERN_INIT_OFF_MAX) {
		ALOGE("%s: invalid init_off data\n", __func__);
		p_attr->cmd_data[tmp_off+0] = LCD_EXTERN_INIT_END;
		p_attr->cmd_data[tmp_off+1] = 0;
		glcd_ext_init_on_cnt = 2;
	} else {
		for (i = 0; i < tmp_cnt; i++)
			p_attr->cmd_data[tmp_off+i] = tmp_buf[i];
		glcd_ext_init_off_cnt = tmp_cnt;
	}

	if (model_debug_flag & DEBUG_LCD_EXTERN) {
		ALOGD("%s, init_on_data:\n", __func__);
		for (i = 0; i < glcd_ext_init_on_cnt; i++)
			printf("  [%d] = 0x%02x\n", i, p_attr->cmd_data[i]);

		ALOGD("%s, init_off_data:\n", __func__);
		for (i = 0; i < glcd_ext_init_off_cnt; i++)
			ALOGD("  [%d] = 0x%02x\n", i, p_attr->cmd_data[tmp_off+i]);
	}

#ifdef CONFIG_AML_LCD_TCON
	memset(data_buf, 0, LCD_EXTERN_INIT_ON_MAX);
	free(data_buf);
	data_buf = NULL;
#endif
	return 0;
}

static int lcd_ext_data_to_buf(unsigned char tmp_buf[], struct lcd_ext_attr_s *p_attr)
{
	int i = 0;
	int tmp_len = 0, tmp_off = 0;

	tmp_off = 0;

	tmp_len = sizeof(struct lcd_ext_header_s);
	memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->head), tmp_len);
	tmp_off += tmp_len;

	tmp_len = sizeof(struct lcd_ext_basic_s);
	memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->basic), tmp_len);
	tmp_off += tmp_len;

	tmp_len = sizeof(struct lcd_ext_type_s);
	memcpy((void *)(tmp_buf + tmp_off), (void *)(&p_attr->type), tmp_len);
	tmp_off += tmp_len;

	tmp_len = glcd_ext_init_on_cnt;
	for (i = 0; i < glcd_ext_init_on_cnt; i++)
		tmp_buf[tmp_off + i] = p_attr->cmd_data[i];
	tmp_off += tmp_len;

	for (i = 0; i < glcd_ext_init_off_cnt; i++)
		tmp_buf[tmp_off + i] = p_attr->cmd_data[tmp_len+i];

	return 0;
}

static int handle_lcd_ext_header(struct lcd_ext_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned char *tmp_buf = NULL;

	tmp_buf = (unsigned char *) malloc(CC_MAX_TEMP_BUF_SIZE);
	if (tmp_buf == NULL) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}

	glcd_ext_dcnt = 0;
	glcd_ext_dcnt += sizeof(struct lcd_ext_header_s);
	glcd_ext_dcnt += sizeof(struct lcd_ext_basic_s);
	glcd_ext_dcnt += sizeof(struct lcd_ext_type_s);

	glcd_ext_dcnt += glcd_ext_init_on_cnt;
	glcd_ext_dcnt += glcd_ext_init_off_cnt;

	p_attr->head.data_len = glcd_ext_dcnt;

	ini_value = IniGetString("lcd_ext_Attr", "version", "null");
	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	p_attr->head.rev = 0;

	memset((void *)tmp_buf, 0, CC_MAX_TEMP_BUF_SIZE);
	lcd_ext_data_to_buf(tmp_buf, p_attr);
	p_attr->head.crc32 = CalCRC32(0, (tmp_buf + 4), glcd_ext_dcnt - 4);

	if (model_debug_flag & DEBUG_LCD_EXTERN)
		ALOGD("%s, glcd_ext_dcnt = %d\n", __func__, glcd_ext_dcnt);

	free(tmp_buf);
	tmp_buf = NULL;

	return 0;
}

static int handle_bl_basic(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "bl_name", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_name is (%s)\n", __func__, ini_value);
	strncpy(p_attr->basic.bl_name, ini_value, CC_BL_NAME_LEN_MAX - 1);
	p_attr->basic.bl_name[CC_BL_NAME_LEN_MAX - 1] = '\0';

	return 0;
}

static int handle_bl_level(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "bl_level_uboot", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_uboot is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_uboot = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_level_kernel", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_kernel is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_kernel = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_level_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_max is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_level_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_min is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_level_mid", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_mid is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_mid = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_level_mid_mapping", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_level_mid_mapping is (%s)\n", __func__, ini_value);
	p_attr->level.bl_level_mid_mapping = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_bl_method(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "bl_method", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_method is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "BL_CTRL_GPIO") == 0)
		p_attr->method.bl_method = BL_CTRL_GPIO;
	else if (strcmp(ini_value, "BL_CTRL_PWM") == 0)
		p_attr->method.bl_method = BL_CTRL_PWM;
	else if (strcmp(ini_value, "BL_CTRL_PWM_COMBO") == 0)
		p_attr->method.bl_method = BL_CTRL_PWM_COMBO;
	else if (strcmp(ini_value, "BL_CTRL_LOCAL_DIMING") == 0)
		p_attr->method.bl_method = BL_CTRL_LOCAL_DIMMING;
	else if (strcmp(ini_value, "BL_CTRL_LOCAL_DIMMING") == 0)
		p_attr->method.bl_method = BL_CTRL_LOCAL_DIMMING;
	else if (strcmp(ini_value, "BL_CTRL_EXTERN") == 0)
		p_attr->method.bl_method = BL_CTRL_EXTERN;
	else
		p_attr->method.bl_method = BL_CTRL_MAX;

	ini_value = IniGetString("Backlight_Attr", "bl_en_gpio", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_en_gpio is (%s)\n", __func__, ini_value);
	p_attr->method.bl_en_gpio = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_en_gpio_on", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_en_gpio_on is (%s)\n", __func__, ini_value);
	p_attr->method.bl_en_gpio_on = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_en_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_en_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->method.bl_en_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_on_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_on_delay is (%s)\n", __func__, ini_value);
	p_attr->method.bl_on_delay = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_off_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_off_delay is (%s)\n", __func__, ini_value);
	p_attr->method.bl_off_delay = strtoul(ini_value, NULL, 0);

	return 0;
}

static int get_pwm_method(const char *ini_value, int def_val)
{
	if (strcmp(ini_value, "BL_PWM_NEGATIVE") == 0)
		return BL_PWM_NEGATIVE;
	else if (strcmp(ini_value, "BL_PWM_POSITIVE") == 0)
		return BL_PWM_POSITIVE;
	else
		return def_val;
}

static char *bl_pwm_name[] = {
	"BL_PWM_A",
	"BL_PWM_B",
	"BL_PWM_C",
	"BL_PWM_D",
	"BL_PWM_E",
	"BL_PWM_F",
	"BL_PWM_G",
	"BL_PWM_H",
	"BL_PWM_I",
	"BL_PWM_J"
};

static char *bl_pwm_ao_name[] = {
	"BL_PWM_AO_A",
	"BL_PWM_AO_B",
	"BL_PWM_AO_C",
	"BL_PWM_AO_D",
	"BL_PWM_AO_E",
	"BL_PWM_AO_F",
	"BL_PWM_AO_G",
	"BL_PWM_AO_H"
};

static char bl_pwm_vs_name[] = {"BL_PWM_VS"};

static unsigned int get_pwm_port_index(const char *str)
{
	enum bl_pwm_port_e pwm_port = BL_PWM_MAX;
	int i, cnt;

	cnt = ARRAY_SIZE(bl_pwm_name);
	for (i = 0; i < cnt; i++) {
		if (strcmp(str, bl_pwm_name[i]) == 0) {
			pwm_port = i + BL_PWM_A;
			return pwm_port;
		}
	}

	cnt = ARRAY_SIZE(bl_pwm_ao_name);
	for (i = 0; i < cnt; i++) {
		if (strcmp(str, bl_pwm_ao_name[i]) == 0) {
			pwm_port = i + BL_PWM_AO_A;
			return pwm_port;
		}
	}

	if (strcmp(str, bl_pwm_vs_name) == 0) {
		pwm_port = BL_PWM_VS;
		return pwm_port;
	}

	return BL_PWM_MAX;
}

static int handle_bl_pwm(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "pwm_method", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_method is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_method = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = IniGetString("Backlight_Attr", "pwm_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_port = get_pwm_port_index(ini_value);

	ini_value = IniGetString("Backlight_Attr", "pwm_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_freq = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_duty_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_duty_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_duty_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_duty_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_duty_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_duty_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_gpio", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_gpio is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_gpio = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_method", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_method is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_method = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = IniGetString("Backlight_Attr", "pwm2_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_port = get_pwm_port_index(ini_value);

	ini_value = IniGetString("Backlight_Attr", "pwm2_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_freq = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_duty_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_duty_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_duty_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_duty_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_duty_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_duty_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_gpio", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_gpio is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_gpio = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_on_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_on_delay is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_on_delay = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_off_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_off_delay is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_off_delay = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_level_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_level_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_level_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm_level_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_level_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_level_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_level_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_level_max is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_level_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "pwm2_level_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm2_level_min is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm2_level_min = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_bl_ldim(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "bl_ldim_row", "1");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_row is (%s)\n", __func__, ini_value);
	p_attr->ldim.ldim_row = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_ldim_col", "1");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_col is (%s)\n", __func__, ini_value);
	p_attr->ldim.ldim_col = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_ldim_mode", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_mode is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LDIM_LR_SIDE") == 0)
		p_attr->ldim.ldim_mode = LDIM_MODE_LR_SIDE;
	else if (strcmp(ini_value, "LDIM_TB_SIDE") == 0)
		p_attr->ldim.ldim_mode = LDIM_MODE_TB_SIDE;
	else if (strcmp(ini_value, "LDIM_DIRECT") == 0)
		p_attr->ldim.ldim_mode = LDIM_MODE_DIRECT;
	else
		p_attr->ldim.ldim_mode = LDIM_MODE_TB_SIDE;

	ini_value = IniGetString("Backlight_Attr", "bl_ldim_dev_index", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_ldim_dev_index is (%s)\n", __func__, ini_value);
	p_attr->ldim.ldim_dev_index = strtoul(ini_value, NULL, 0);

	p_attr->ldim.ldim_attr_4 = 0;
	p_attr->ldim.ldim_attr_5 = 0;
	p_attr->ldim.ldim_attr_6 = 0;
	p_attr->ldim.ldim_attr_7 = 0;
	p_attr->ldim.ldim_attr_8 = 0;
	p_attr->ldim.ldim_attr_9 = 0;

	return 0;
}

static int handle_bl_custome(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "bl_custome_val_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_0 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_custome_val_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_1 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_1 = get_pwm_port_index(ini_value);

	ini_value = IniGetString("Backlight_Attr", "bl_custome_val_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_2 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_custome_val_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_3 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Backlight_Attr", "bl_custome_val_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, bl_custome_val_4 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_val_4 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_bl_header(struct bl_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Backlight_Attr", "version", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	gbl_dcnt = 0;
	gbl_dcnt += sizeof(struct bl_header_s);
	gbl_dcnt += sizeof(struct bl_basic_s);
	gbl_dcnt += sizeof(struct bl_level_s);
	gbl_dcnt += sizeof(struct bl_method_s);
	gbl_dcnt += sizeof(struct bl_pwm_s);
	if (p_attr->head.version == 2) {
		gbl_dcnt += sizeof(struct bl_ldim_s);
		gbl_dcnt += sizeof(struct bl_custome_s);
	}
	p_attr->head.data_len = gbl_dcnt;

	p_attr->head.rev = 0;
	p_attr->head.crc32 = CalCRC32(0, (((unsigned char *)p_attr) + 4), gbl_dcnt - 4);

	return 0;
}

#ifdef CONFIG_AML_LCD_BL_LDIM
static int handle_ldim_dev_basic(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Ldim_dev_Attr", "dev_name", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, dev_name is (%s)\n", __func__, ini_value);
	strncpy(p_attr->basic.dev_name, ini_value, CC_LDIM_DEV_NAME_LEN_MAX - 1);
	p_attr->basic.dev_name[CC_LDIM_DEV_NAME_LEN_MAX - 1] = '\0';

	return 0;
}

static int handle_ldim_dev_if(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Ldim_dev_Attr", "if_type", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_type is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "LDIM_DEV_I2C") == 0)
		p_attr->interface.type = LDIM_DEV_TYPE_I2C;
	else if (strcmp(ini_value, "LDIM_DEV_SPI") == 0)
		p_attr->interface.type = LDIM_DEV_TYPE_SPI;
	else
		p_attr->interface.type = LCD_EXTERN_MAX;

	ini_value = IniGetString("Ldim_dev_Attr", "if_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_freq is (%s)\n", __func__, ini_value);
	p_attr->interface.freq = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_5", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_6", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_7", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_8", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "if_attr_9", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, if_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->interface.if_attr_9 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_ldim_dev_pwm(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_port = get_pwm_port_index(ini_value);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_pol", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_pol is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_pol = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_freq = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_duty", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_duty is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_duty = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_vs_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_vs_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_vs_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_port = get_pwm_port_index(ini_value);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_pol", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_pol is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_pol = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_freq = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_duty", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_duty is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_duty = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_hs_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_hs_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_hs_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_port", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_port is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_port = get_pwm_port_index(ini_value);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_pol", "BL_PWM_POSITIVE");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_pol is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_pol = get_pwm_method(ini_value, BL_PWM_POSITIVE);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_freq", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_freq is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_freq = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_duty", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_duty is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_duty = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pwm_adj_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pwm_adj_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->pwm.pwm_adj_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "pinmux_sel", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, pinmux_sel is (%s)\n", __func__, ini_value);
	strncpy(p_attr->pwm.pinmux_sel, ini_value, 29);

	return 0;
}

static int handle_ldim_dev_ctrl(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Ldim_dev_Attr", "en_gpio", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, en_gpio is (%s)\n", __func__, ini_value);
	p_attr->ctrl.en_gpio = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "en_gpio_on", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, en_gpio_on is (%s)\n", __func__, ini_value);
	p_attr->ctrl.en_gpio_on = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "en_gpio_off", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, en_gpio_off is (%s)\n", __func__, ini_value);
	p_attr->ctrl.en_gpio_off = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "on_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, on_delay is (%s)\n", __func__, ini_value);
	p_attr->ctrl.on_delay = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "off_delay", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, off_delay is (%s)\n", __func__, ini_value);
	p_attr->ctrl.off_delay = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "err_gpio", "0xff");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, err_gpio is (%s)\n", __func__, ini_value);
	p_attr->ctrl.err_gpio = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "write_check", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, write_check is (%s)\n", __func__, ini_value);
	p_attr->ctrl.write_check = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "dim_max", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, dim_max is (%s)\n", __func__, ini_value);
	p_attr->ctrl.dim_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "dim_min", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, dim_min is (%s)\n", __func__, ini_value);
	p_attr->ctrl.dim_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "chip_count", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, chip_count is (%s)\n", __func__, ini_value);
	p_attr->ctrl.chip_cnt = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "zone_mapping_path", "null");
	if (strcmp(ini_value, "null") != 0) {
		env_set("bl_mapping_path", ini_value);
		strncpy(p_attr->ctrl.zone_map_path, ini_value, 255); //if no path_k
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGE("%s, zone_mapping_path is (%s)\n", __func__, ini_value);
	}

	ini_value = IniGetString("Ldim_dev_Attr", "zone_mapping_path_k", "null");
	if (strcmp(ini_value, "null") != 0) {
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGD("%s, zone_mapping_path_k is (%s)\n", __func__, ini_value);
		strncpy(p_attr->ctrl.zone_map_path, ini_value, 255);
	} else {
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGD("%s, zone_mapping_path_k is NOT FOUND\n", __func__);
	}

	return 0;
}

static int handle_ldim_dev_profile(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Ldim_dev_Attr", "profile_mode", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_mode is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_mode = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_path", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_path is (%s)\n", __func__, ini_value);
	strncpy(p_attr->profile.profile_path, ini_value, 255);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_5", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_6", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "profile_attr_7", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, profile_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->profile.profile_attr_7 = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_ldim_dev_custom(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;
	unsigned int tmp_buf[32];
	int i = 0, tmp_cnt = 0;

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_0", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_0 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_0 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_1", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_1 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_1 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_2", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_2 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_2 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_3", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_3 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_3 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_4", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_4 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_4 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_5", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_5 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_5 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_6", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_6 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_6 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_7", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_7 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_7 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_8", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_8 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_8 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "custome_attr_9", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, custome_attr_9 is (%s)\n", __func__, ini_value);
	p_attr->custome.custome_attr_9 = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("Ldim_dev_Attr", "param_data", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, param_data is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		p_attr->custome.custome_param_size = 0;
		printf("%s, panel.ini no param_data(%s)\n", __func__, ini_value);
	} else {
		tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
		if (model_debug_flag & DEBUG_BACKLIGHT)
			ALOGD("%s, param_data is (%d)\n", __func__, tmp_cnt);
		/* data check and copy */
		if (tmp_cnt > LDIM_PARAM_MAX) {
			printf("%s: invalid param data\n", __func__);
		} else {
			p_attr->custome.custome_param_size = tmp_cnt;
			i = 0;
			while (i < tmp_cnt) {
				p_attr->custome.custome_param[i] = tmp_buf[i];
				if (model_debug_flag & DEBUG_BACKLIGHT)
					ALOGD("param_data[%d] is (%d)\n", i, tmp_buf[i]);
				i++;
			}
		}
	}

	return 0;
}

static int handle_ldim_dev_init(struct ldim_dev_attr_s *p_attr)
{
	int i = 0, j = 0, k, tmp_cnt = 0, tmp_off = 0;
	const char *ini_value = NULL;
	unsigned int tmp_buf[2048];
	unsigned int data_size = 0;

	ini_value = IniGetString("Ldim_dev_Attr", "cmd_size", "0");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, cmd_size is (%s)\n", __func__, ini_value);
	p_attr->init.cmd_size = strtoul(ini_value, NULL, 0);

	if (p_attr->init.cmd_size != 0xff) {
		ALOGE("%s: invalid cmd_size 0x%x\n", __func__, p_attr->init.cmd_size);
		p_attr->init.cmd_data[0] = 0xff;
		p_attr->init.cmd_data[1] = 0;
		g_ldim_dev_init_on_cnt = 2;
		p_attr->init.cmd_data[2] = 0xff;
		p_attr->init.cmd_data[3] = 0;
		g_ldim_dev_init_off_cnt = 2;
		return 0;
	}

	ini_value = IniGetString("Ldim_dev_Attr", "init_on", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, init_on is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);

	/* data check and copy */
	if (tmp_cnt > LDIM_INIT_ON_MAX) {
		ALOGE("%s: invalid init_on data\n", __func__);
		p_attr->init.cmd_data[0] = 0xff;
		p_attr->init.cmd_data[1] = 0;
		g_ldim_dev_init_on_cnt = 2;
	} else {
		i = 0;
		j = 0;
		while (i < tmp_cnt) {
			p_attr->init.cmd_data[j] = tmp_buf[i];
			if (p_attr->init.cmd_data[j] == 0xff) {
				p_attr->init.cmd_data[j + 1] = 0;
				j += 2;
				break;
			}

			data_size = tmp_buf[i + 1];
			p_attr->init.cmd_data[j + 1] = data_size;
			for (k = 0; k < data_size; k++) {
				p_attr->init.cmd_data[j + 2 + k] =
					(unsigned char)tmp_buf[i + 2 + k];
			}

			j += data_size + 2;
			i += tmp_buf[i + 1] + 2; /* raw data */
		}
		g_ldim_dev_init_on_cnt = j;
	}

	tmp_off = g_ldim_dev_init_on_cnt;
	ini_value = IniGetString("Ldim_dev_Attr", "init_off", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, init_off is (%s)\n", __func__, ini_value);
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	if (tmp_cnt > LDIM_INIT_OFF_MAX) {
		ALOGE("%s: invalid init_off data\n", __func__);
		p_attr->init.cmd_data[tmp_off + 0] = 0xff;
		p_attr->init.cmd_data[tmp_off + 1] = 0;
		g_ldim_dev_init_off_cnt = 2;
	} else {
		for (i = 0; i < tmp_cnt; i++)
			p_attr->init.cmd_data[tmp_off + i] = tmp_buf[i];
		g_ldim_dev_init_off_cnt = tmp_cnt;
	}

	if (model_debug_flag & DEBUG_BACKLIGHT) {
		ALOGD("%s, init_on_data:\n", __func__);
		for (i = 0; i < g_ldim_dev_init_on_cnt; i++)
			printf("  [%d] = 0x%02x\n", i, p_attr->init.cmd_data[i]);

		ALOGD("%s, init_off_data:\n", __func__);
		for (i = 0; i < g_ldim_dev_init_off_cnt; i++)
			ALOGD("  [%d] = 0x%02x\n", i, p_attr->init.cmd_data[tmp_off + i]);
	}

	return 0;
}

static int handle_ldim_dev_header(struct ldim_dev_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("Ldim_dev_Attr", "version", "null");
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0)
		p_attr->head.version = 0;
	else
		p_attr->head.version = strtoul(ini_value, NULL, 0);

	gldim_dev_dcnt = 0;
	gldim_dev_dcnt += sizeof(struct ldim_dev_header_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_basic_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_if_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_pwm_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_ctrl_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_profile_s);
	gldim_dev_dcnt += sizeof(struct ldim_dev_custom_s);
	gldim_dev_dcnt += g_ldim_dev_init_on_cnt;
	gldim_dev_dcnt += g_ldim_dev_init_off_cnt;
	p_attr->head.data_len = gldim_dev_dcnt;

	p_attr->head.rev = 0;
	p_attr->head.crc32 = CalCRC32(0, (((unsigned char *)p_attr) + 4), gldim_dev_dcnt - 4);

	return 0;
}
#endif

static int handle_panel_misc(struct panel_misc_s *p_misc)
{
	int tmp_val = 0;
	const char *ini_value = NULL;
	const char *display_layer = NULL;
	char *rev_ctrl = NULL;
	char *ret = NULL;
	char buf[64] = {0};
	unsigned char connector_idx;

	ini_value = IniGetString("panel_misc", "panel_misc_version", "null");
	if (model_debug_flag & DEBUG_MISC)
		ALOGD("%s, panel_misc_version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		strcpy(p_misc->version, "V001");
	} else {
		tmp_val = strtol(ini_value, NULL, 0);
		if (tmp_val < 1)
			tmp_val = 1;

		sprintf(p_misc->version, "V%03d", tmp_val);
	}

	tmp_val = env_get_ulong("model_outputmode_bypass", 10, 0);
	if (tmp_val) {
		ALOGI("model_outputmode_bypass\n");
		goto handle_panel_misc_next;
	}
	ini_value = IniGetString("panel_misc", "outputmode2", "null");
	if (model_debug_flag & DEBUG_MISC)
		ALOGD("%s, outputmode2 is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		ini_value = IniGetString("panel_misc", "outputmode", "null");
		if (model_debug_flag & DEBUG_MISC)
			ALOGD("%s, outputmode is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null")) {
			strncpy(p_misc->outputmode, ini_value,
				sizeof(p_misc->outputmode) - 1);
			p_misc->outputmode[sizeof(p_misc->outputmode) - 1]
				= '\0';
			snprintf(buf, 63, "setenv outputmode %s", p_misc->outputmode);
			run_command(buf, 0);
		}
	} else {
		strncpy(p_misc->outputmode, ini_value, 63);
		snprintf(buf, 63, "setenv outputmode2 %s", p_misc->outputmode);
		run_command(buf, 0);
	}

handle_panel_misc_next:
	tmp_val = env_get_ulong("model_connector_bypass", 10, 0);
	if (tmp_val) {
		ALOGI("model_connector_bypass\n");
		goto handle_panel_misc_next2;
	}

	ini_value = IniGetString("panel_misc", "connector2_type", "null");
	if (strcmp(ini_value, "null")) {
		connector_idx = 2;
		goto handle_panel_misc_set_connector;
	}

	ini_value = IniGetString("panel_misc", "connector1_type", "null");
	if (strcmp(ini_value, "null")) {
		connector_idx = 1;
		goto handle_panel_misc_set_connector;
	}

	ini_value = IniGetString("panel_misc", "connector0_type", "null");
	if (strcmp(ini_value, "null")) {
		connector_idx = 0;
		goto handle_panel_misc_set_connector;
	} else {
		ini_value = IniGetString("panel_misc", "connector_type", "null");
		if (strcmp(ini_value, "null")) {
			connector_idx = 0;
			goto handle_panel_misc_set_connector;
		}
	}

	ALOGD("%s: connector not assigned\n", __func__);
	goto handle_panel_misc_next2;

handle_panel_misc_set_connector:
	if (model_debug_flag & DEBUG_MISC)
		ALOGD("%s, connector%u_type is (%s)\n", __func__, connector_idx, ini_value);
	strncpy(p_misc->connector_type, ini_value, sizeof(p_misc->connector_type) - 1);
	p_misc->connector_type[sizeof(p_misc->connector_type) - 1] = '\0';
	ret = strstr(p_misc->connector_type, "_");
	if (ret)
		p_misc->connector_type[ret - p_misc->connector_type] = '-';
	snprintf(buf, 63, "setenv connector%u_type %s", connector_idx, p_misc->connector_type);
	run_command(buf, 0);

handle_panel_misc_next2:
	rev_ctrl = env_get("reverse_ctrl");
	if (!rev_ctrl || strcmp(rev_ctrl, "0") == 0) {
		ini_value = IniGetString("panel_misc", "panel_reverse", "null");
		if (model_debug_flag & DEBUG_MISC)
			ALOGD("%s, panel_reverse is (%s)\n", __func__, ini_value);
		if (strcmp(ini_value, "null") == 0 || strcmp(ini_value, "0") == 0 ||
			strcmp(ini_value, "false") == 0 || strcmp(ini_value, "no_rev") == 0) {
			p_misc->panel_reverse = 0;
		} else if (strcmp(ini_value, "true") == 0 || strcmp(ini_value, "1") == 0 ||
			strcmp(ini_value, "have_rev") == 0) {
			p_misc->panel_reverse = 1;
		} else if (strcmp(ini_value, "x_rev") == 0 || strcmp(ini_value, "2") == 0) {
			p_misc->panel_reverse = 2;
		} else if (strcmp(ini_value, "y_rev") == 0 || strcmp(ini_value, "3") == 0) {
			p_misc->panel_reverse = 3;
		} else {
			p_misc->panel_reverse = 0;
		}
		if (p_misc->panel_reverse) {
			display_layer = IniGetString("panel_misc", "display_layer", "null");
			if (!display_layer)
				p_misc->display_layer = 4;
			else if (strcmp(display_layer, "osd0") == 0 ||
					strcmp(display_layer, "0") == 0)
				p_misc->display_layer = 0;
			else if (strcmp(display_layer, "osd1") == 0 ||
					strcmp(display_layer, "1") == 0)
				p_misc->display_layer = 1;
			else
				p_misc->display_layer = 4;
		}
		switch (p_misc->panel_reverse) {
		case 1:
			run_command("setenv panel_reverse 1", 0);
			switch (p_misc->display_layer) {
			case 0:
				run_command("setenv osd_reverse osd0,true", 0);
				break;
			case 1:
				run_command("setenv osd_reverse osd1,true", 0);
				break;
			default:
				run_command("setenv osd_reverse all,true", 0);
				break;
			}
			run_command("setenv video_reverse 1", 0);
			break;
		case 2:
			run_command("setenv panel_reverse 2", 0);
			switch (p_misc->display_layer) {
			case 0:
				run_command("setenv osd_reverse osd0,x_rev", 0);
				break;
			case 1:
				run_command("setenv osd_reverse osd1,x_rev", 0);
				break;
			default:
				run_command("setenv osd_reverse all,x_rev", 0);
				break;
			}
			run_command("setenv video_reverse 2", 0);
			break;
		case 3:
			run_command("setenv panel_reverse 3", 0);
			switch (p_misc->display_layer) {
			case 0:
				run_command("setenv osd_reverse osd0,y_rev", 0);
				break;
			case 1:
				run_command("setenv osd_reverse osd1,y_rev", 0);
				break;
			default:
				run_command("setenv osd_reverse all,y_rev", 0);
				break;
			}
			run_command("setenv video_reverse 3", 0);
			break;
		default:
			run_command("setenv panel_reverse 0", 0);
			run_command("setenv osd_reverse n", 0);
			run_command("setenv video_reverse 0", 0);
			break;
		}
	}
	return 0;
}

#ifdef CONFIG_AML_LCD_TCON
static int handle_tcon_spi_v0(unsigned char *buff)
{
	const char *ini_value = NULL;
	char str[30];
	unsigned int null_cnt = 0, block_cnt = 4;
	unsigned int temp, i, j, n;

	/* header */
	/* version */
	temp = 0;
	buff[8] = temp & 0xff;
	buff[9] = (temp >> 8) & 0xff;
	buff[10] = (temp >> 16) & 0xff;
	buff[11] = (temp >> 24) & 0xff;

	/* block 0: demura_lut */
	n = 16;
	ini_value = IniGetString("tcon_spi_Attr", "demura_lut_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, demura_lut_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_1;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = IniGetString("tcon_spi_Attr", "demura_lut_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, demura_lut_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_1;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block0_param_%d", j);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_1:
	/* block 1: p_gamma */
	ini_value = IniGetString("tcon_spi_Attr", "p_gamma_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, p_gamma_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_2;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = IniGetString("tcon_spi_Attr", "p_gamma_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, p_gamma_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_2;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block1_param_%d", j);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_2:
	/* block 2: acc_lut */
	ini_value = IniGetString("tcon_spi_Attr", "acc_lut_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, acc_lut_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_3;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = IniGetString("tcon_spi_Attr", "acc_lut_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, acc_lut_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_block_3;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block2_param_%d", j);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_block_3:
	/* block 3: auto_flicker */
	ini_value = IniGetString("tcon_spi_Attr", "auto_flicker_offset", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, auto_flicker_offset is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_next;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	ini_value = IniGetString("tcon_spi_Attr", "auto_flicker_size", "null");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, auto_flicker_size is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		null_cnt++;
		goto handle_tcon_spi_v0_next;
	}
	temp = strtoul(ini_value, NULL, 0);
	for (i = 0; i < 4; i++)
		buff[n + i] = (temp >> (i * 8)) & 0xff;
	n += 4;

	for (j = 0; j < 6; j++) {
		sprintf(str, "block3_param_%d", j);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		for (i = 0; i < 4; i++)
			buff[n + i] = (temp >> (i * 8)) & 0xff;
		n += 4;
	}

handle_tcon_spi_v0_next:
	if (null_cnt >= 4) {
		block_cnt = 0;
		gLcdTconSpi_cnt = 0;
	} else {
		block_cnt = 4;
		gLcdTconSpi_cnt = (16 + 32 * block_cnt);
	}

	/* block cnt */
	buff[12] = block_cnt & 0xff;
	buff[13] = (block_cnt >> 8) & 0xff;
	buff[14] = (block_cnt >> 16) & 0xff;
	buff[15] = (block_cnt >> 24) & 0xff;

	/* data size */
	buff[4] = gLcdTconSpi_cnt & 0xff;
	buff[5] = (gLcdTconSpi_cnt >> 8) & 0xff;
	buff[6] = (gLcdTconSpi_cnt >> 16) & 0xff;
	buff[7] = (gLcdTconSpi_cnt >> 24) & 0xff;

	/* crc */
	temp = CalCRC32(0, (buff + 4), gLcdTconSpi_cnt - 4);
	buff[0] = temp & 0xff;
	buff[1] = (temp >> 8) & 0xff;
	buff[2] = (temp >> 16) & 0xff;
	buff[3] = (temp >> 24) & 0xff;

	return 0;
}

static int handle_tcon_spi(unsigned char *buff)
{
	unsigned char *p;
	const char *ini_value = NULL;
	char str[30];
	unsigned int data_size, block_cnt, param_cnt;
	unsigned int temp, i, j, k, n;

	/* header */
	/* version */
	ini_value = IniGetString("tcon_spi_Attr", "version", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	temp = strtoul(ini_value, NULL, 0);
	if (temp == 0) {
		handle_tcon_spi_v0(buff);
		return 0;
	}

	/* new data format */
	/* version */
	buff[8] = temp & 0xff;
	buff[9] = (temp >> 8) & 0xff;

	/* block cnt */
	ini_value = IniGetString("tcon_spi_Attr", "block_cnt", "0");
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s, block_cnt is (%s)\n", __func__, ini_value);
	block_cnt = strtoul(ini_value, NULL, 0);
	buff[14] = block_cnt & 0xff;
	buff[15] = (block_cnt >> 8) & 0xff;

	p = &buff[16];
	n = 0;
	for (i = 0; i < block_cnt; i++) {
		snprintf(str, 30, "block%d_data_type", i);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n] = temp & 0xff;
		p[n + 1] = (temp >> 8) & 0xff;

		snprintf(str, 30, "block%d_data_index", i);
		ini_value = IniGetString("tcon_spi_Attr", str, "0xff");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 2] = temp & 0xff;
		p[n + 3] = (temp >> 8) & 0xff;

		snprintf(str, 30, "block%d_data_flag", i);
		ini_value = IniGetString("tcon_spi_Attr", str, "0xff");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 4] = temp & 0xff;
		p[n + 5] = (temp >> 8) & 0xff;
		p[n + 6] = (temp >> 16) & 0xff;
		p[n + 7] = (temp >> 24) & 0xff;

		snprintf(str, 30, "block%d_spi_data_offset", i);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 8] = temp & 0xff;
		p[n + 9] = (temp >> 8) & 0xff;
		p[n + 10] = (temp >> 16) & 0xff;
		p[n + 11] = (temp >> 24) & 0xff;

		snprintf(str, 30, "block%d_spi_data_size", i);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		temp = strtoul(ini_value, NULL, 0);
		p[n + 12] = temp & 0xff;
		p[n + 13] = (temp >> 8) & 0xff;
		p[n + 14] = (temp >> 16) & 0xff;
		p[n + 15] = (temp >> 24) & 0xff;

		snprintf(str, 30, "block%d_param_cnt", i);
		ini_value = IniGetString("tcon_spi_Attr", str, "0");
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
		param_cnt = strtoul(ini_value, NULL, 0);
		p[n + 16] = param_cnt & 0xff;
		p[n + 17] = (param_cnt >> 8) & 0xff;
		p[n + 18] = (param_cnt >> 16) & 0xff;
		p[n + 19] = (param_cnt >> 24) & 0xff;

		/* conversion parameters */
		k = n + 20;
		for (j = 0; j < param_cnt; j++) {
			snprintf(str, 30, "block%d_param_%d", i, j);
			ini_value = IniGetString("tcon_spi_Attr", str, "0");
			if (model_debug_flag & DEBUG_TCON)
				ALOGD("%s, %s is (%s)\n", __func__, str, ini_value);
			temp = strtoul(ini_value, NULL, 0);
			p[k] = temp & 0xff;
			p[k + 1] = (temp >> 8) & 0xff;
			p[k + 2] = (temp >> 16) & 0xff;
			p[k + 3] = (temp >> 24) & 0xff;
			k += 4;
		}
		n += (20 + param_cnt * 4);
	}

	/* data size */
	data_size = 16 + n;
	buff[4] = data_size & 0xff;
	buff[5] = (data_size >> 8) & 0xff;
	buff[6] = (data_size >> 16) & 0xff;
	buff[7] = (data_size >> 24) & 0xff;
	gLcdTconSpi_cnt = data_size;

	/* crc */
	temp = CalCRC32(0, (buff + 4), gLcdTconSpi_cnt - 4);
	buff[0] = temp & 0xff;
	buff[1] = (temp >> 8) & 0xff;
	buff[2] = (temp >> 16) & 0xff;
	buff[3] = (temp >> 24) & 0xff;

	return 0;
}
#endif

static int handle_lcd_optical_attr(struct lcd_optical_attr_s *p_attr)
{
	const char *ini_value = NULL;

	ini_value = IniGetString("lcd_optical_Attr", "version", "null");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, version is (%s)\n", __func__, ini_value);
	if (strcmp(ini_value, "null") == 0) {
		glcd_optical_dcnt = 0;
		return -1;
	}
	p_attr->head.version = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "hdr_support", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, hdr_support is (%s)\n", __func__, ini_value);
	p_attr->hdr_support = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "features", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, features is (%s)\n", __func__, ini_value);
	p_attr->features = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "primaries_r_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_r_x is (%s)\n", __func__, ini_value);
	p_attr->primaries_r_x = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "primaries_r_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_r_y is (%s)\n", __func__, ini_value);
	p_attr->primaries_r_y = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "primaries_g_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_g_x is (%s)\n", __func__, ini_value);
	p_attr->primaries_g_x = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "primaries_g_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_g_y is (%s)\n", __func__, ini_value);
	p_attr->primaries_g_y = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "primaries_b_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_b_x is (%s)\n", __func__, ini_value);
	p_attr->primaries_b_x = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "primaries_b_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, primaries_b_y is (%s)\n", __func__, ini_value);
	p_attr->primaries_b_y = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "white_point_x", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, white_point_x is (%s)\n", __func__, ini_value);
	p_attr->white_point_x = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "white_point_y", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, white_point_y is (%s)\n", __func__, ini_value);
	p_attr->white_point_y = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "luma_max", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_max is (%s)\n", __func__, ini_value);
	p_attr->luma_max = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "luma_min", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_min is (%s)\n", __func__, ini_value);
	p_attr->luma_min = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "luma_avg", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_avg is (%s)\n", __func__, ini_value);
	p_attr->luma_avg = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "ldim_support", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, ldim_support is (%s)\n", __func__, ini_value);
	p_attr->ldim_support = strtoul(ini_value, NULL, 0);

	ini_value = IniGetString("lcd_optical_Attr", "luma_peak", "0");
	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, luma_peak is (%s)\n", __func__, ini_value);
	p_attr->luma_peak = strtoul(ini_value, NULL, 0);

	return 0;
}

static int handle_lcd_optical_header(struct lcd_optical_attr_s *p_attr)
{
	unsigned char *tmp_buf = NULL;

	glcd_optical_dcnt = sizeof(struct lcd_optical_attr_s);

	tmp_buf = (unsigned char *)malloc(glcd_optical_dcnt);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset((void *)tmp_buf, 0, glcd_optical_dcnt);

	p_attr->head.data_len = glcd_optical_dcnt;

	p_attr->head.block_next_flag = 0;
	p_attr->head.block_cur_size = glcd_optical_dcnt;

	memcpy(tmp_buf, p_attr, glcd_optical_dcnt);
	p_attr->head.crc32 = CalCRC32(0, (tmp_buf + 4), glcd_optical_dcnt - 4);

	if (model_debug_flag & DEBUG_LCD_OPTICAL)
		ALOGD("%s, glcd_optical_dcnt = %d\n", __func__, glcd_optical_dcnt);

	free(tmp_buf);
	tmp_buf = NULL;

	return 0;
}

static int parse_panel_ini(const char *file_name, unsigned char *lcd_buf,
			   struct lcd_ext_attr_s *ext_attr,
			   struct bl_attr_s *bl_attr,
			   struct ldim_dev_attr_s *ldim_dev_attr,
			   struct panel_misc_s *misc_attr,
			   unsigned char *tcon_spi_buf,
			   struct lcd_optical_attr_s *optical_attr)
{
	struct lcd_attr_s *lcd_attr;
	struct lcd_v2_attr_s *lcd_v2_attr;
	unsigned short lcd_size = 0;
	struct lcd_header_s *header;
	int ret;

	IniParserInit();

	if (IniParseFile(file_name) < 0) {
		ALOGE("%s, ini load file error!\n", __func__);
		IniParserUninit();
		return -1;
	}

	// handle integrity flag
	if (handle_integrity_flag() < 0) {
		ALOGE("%s, handle_integrity_flag error!\n", __func__);
		IniParserUninit();
		return -1;
	}

	lcd_attr = (struct lcd_attr_s *)malloc(sizeof(struct lcd_attr_s));
	if (!lcd_attr) {
		IniParserUninit();
		return -1;
	}
	memset(lcd_attr, 0, sizeof(struct lcd_attr_s));
	lcd_v2_attr = (struct lcd_v2_attr_s *)malloc(sizeof(struct lcd_v2_attr_s));
	if (!lcd_v2_attr) {
		free(lcd_attr);
		IniParserUninit();
		return -1;
	}
	memset(lcd_v2_attr, 0, sizeof(struct lcd_v2_attr_s));

	/* handle lcd attr */
	handle_lcd_basic(lcd_attr);
	handle_lcd_timming(lcd_attr);
	handle_lcd_customer(lcd_attr);
	handle_lcd_interface(lcd_attr);
	handle_lcd_pwr(lcd_attr);
	handle_lcd_header(lcd_attr);

	lcd_size = lcd_attr->head.block_cur_size;
	memcpy((void *)lcd_buf, (void *)lcd_attr, lcd_attr->head.block_cur_size);
	/* handle lcd_v2 attr*/
	if (lcd_attr->head.version == 2) {
		handle_lcd_phy(lcd_v2_attr);
		handle_lcd_cus_ctrl(lcd_v2_attr);
		handle_lcd_v2_header(lcd_v2_attr);
		lcd_size += lcd_v2_attr->head.block_cur_size;
		memcpy((void *)(lcd_buf + lcd_attr->head.block_cur_size),
			(void *)lcd_v2_attr, lcd_v2_attr->head.block_cur_size);
	}

	header = (struct lcd_header_s *)lcd_buf;
	glcd_dcnt = lcd_size;
	header->data_len = lcd_size;
	header->crc32 = CalCRC32(0, (lcd_buf + 4), lcd_size - 4);
	if (model_debug_flag & DEBUG_LCD) {
		ALOGD("%s: data_len=%d, glcd_dcnt=%d, block1_size=%d, block2_size=%d\n",
			__func__, header->data_len, glcd_dcnt,
			lcd_attr->head.block_cur_size,
			lcd_v2_attr->head.block_cur_size);
	}

	if (g_lcd_if == LCD_MLVDS ||
	    g_lcd_if == LCD_P2P)
		g_lcd_tcon_valid = 1;
	else
		g_lcd_tcon_valid = 0;

#ifdef CONFIG_AML_LCD_TCON
	/*should ready tcon path here, for lcd_ext usage */
	if (g_lcd_tcon_valid)
		handle_tcon_path();
#endif

	// handle lcd extern attr
	handle_lcd_ext_basic(ext_attr);
	handle_lcd_ext_type(ext_attr);
	handle_lcd_ext_cmd_data(ext_attr);
	handle_lcd_ext_header(ext_attr);

	// handle bl attr
	handle_bl_basic(bl_attr);
	handle_bl_level(bl_attr);
	handle_bl_method(bl_attr);
	handle_bl_pwm(bl_attr);
	handle_bl_ldim(bl_attr);
	handle_bl_custome(bl_attr);
	handle_bl_header(bl_attr);

#ifdef CONFIG_AML_LCD_BL_LDIM
	if (bl_attr->method.bl_method == BL_CTRL_LOCAL_DIMMING)
		g_ldim_dev_valid = 1;
	else
		g_ldim_dev_valid = 0;

	// handle ldim_dev attr
	if (g_ldim_dev_valid) {
		handle_ldim_dev_basic(ldim_dev_attr);
		handle_ldim_dev_if(ldim_dev_attr);
		handle_ldim_dev_pwm(ldim_dev_attr);
		handle_ldim_dev_ctrl(ldim_dev_attr);
		handle_ldim_dev_profile(ldim_dev_attr);
		handle_ldim_dev_custom(ldim_dev_attr);
		handle_ldim_dev_init(ldim_dev_attr);
		handle_ldim_dev_header(ldim_dev_attr);
	}
#endif

	handle_panel_misc(misc_attr);

#ifdef CONFIG_AML_LCD_TCON
	if (g_lcd_tcon_valid)
		handle_tcon_spi(tcon_spi_buf);
	else
		gLcdTconSpi_cnt = 0;
#endif

	// handle lcd optical attr
	ret = handle_lcd_optical_attr(optical_attr);
	if (ret == 0)
		handle_lcd_optical_header(optical_attr);

	IniParserUninit();

	memset(lcd_v2_attr, 0, sizeof(struct lcd_v2_attr_s));
	free(lcd_v2_attr);
	memset(lcd_attr, 0, sizeof(struct lcd_attr_s));
	free(lcd_attr);
	return 0;
}

int handle_read_bin_file(const char *file_name, unsigned long max_len)
{
	int size;

	BinFileInit();

	size = ReadBinFile(file_name);
	if (size < 0) {
		ALOGE("%s, load bin file error!\n", __func__);
		BinFileUninit();
		return 0;
	}

	if (size > max_len) {
		ALOGE("%s, bin file size out of support!\n", __func__);
		BinFileUninit();
		return 0;
	}

	return size;
}

#ifdef CONFIG_AML_LCD_TCON
static int handle_read_bin_file_with_header(const char *file_name, unsigned long max_len)
{
	unsigned char buf[16];
	int bin_size, data_size = 0;

	BinFileInit();

	bin_size = ReadBinFile(file_name);
	if (bin_size < 64) {
		ALOGE("%s, load bin file error!\n", __func__);
		BinFileUninit();
		return 0;
	}

	GetBinData(buf, 16);
	data_size = (buf[8] | (buf[9] << 8) |
		     (buf[10] << 16) | (buf[11] << 24));
	if ((data_size > bin_size) || (data_size < 64)) {
		ALOGE("%s, bin file size less than expectation!\n", __func__);
		BinFileUninit();
		return 0;
	}
	if (data_size > max_len) {
		ALOGE("%s, bin file size out of support!\n", __func__);
		BinFileUninit();
		return 0;
	}

	return data_size;
}

static int handle_tcon_bin(void)
{
	int tmp_len = 0;
	unsigned int size = 0;
	unsigned char *tmp_buf = NULL;
	unsigned char *tcon_buf = NULL;
	char *file_name;
	unsigned int bypass, header, data_crc32, temp_crc32;
	int tmp;

	tmp = env_get_ulong("model_tcon_bypass", 10, 0xffff);
	if (tmp != 0xffff) {
		bypass = tmp;
		if (bypass) {
			ALOGI("model_tcon_bypass\n");
			return 0;
		}
	}

	header = env_get_ulong("model_tcon_bin_header", 10, 0);

	file_name = env_get("model_tcon");
	if (!file_name) {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s, no model_tcon path\n", __func__);
		return 0;
	}

	tmp_buf = (unsigned char *)malloc(CC_MAX_TCON_BIN_SIZE);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}

	// start handle tcon bin name
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s: model_tcon: %s\n", __func__, file_name);
	if (header)
		size = handle_read_bin_file_with_header(file_name, CC_MAX_TCON_BIN_SIZE);
	else
		size = handle_read_bin_file(file_name, CC_MAX_TCON_BIN_SIZE);
	if (size == 0) {
		free(tmp_buf);
		tmp_buf = NULL;
		return -1;
	}

	GetBinData(tmp_buf, size);
	if (header) {
		data_crc32 = tmp_buf[0] | (tmp_buf[1] << 8) |
			(tmp_buf[2] << 16) | (tmp_buf[3] << 24);
		temp_crc32 = CalCRC32(0, &tmp_buf[4], (size - 4));
		if (data_crc32 != temp_crc32) {
			free(tmp_buf);
			tmp_buf = NULL;
			if (model_debug_flag & DEBUG_TCON) {
				ALOGE("%s, tcon bin crc error! raw:0x%08x, temp:0x%08x\n",
					__func__, data_crc32, temp_crc32);
			} else {
				ALOGE("%s, tcon bin crc error!!!\n", __func__);
			}
			return -1;
		}
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s: load tcon bin with header, size:0x%x\n", __func__, size);
	} else {
		if (model_debug_flag & DEBUG_TCON)
			ALOGD("%s: load tcon bin, size:0x%x\n", __func__, size);
	}

	gLcdTconDataCnt = size;
	tcon_buf = (unsigned char *)malloc(size);
	if (!tcon_buf) {
		free(tmp_buf);
		tmp_buf = NULL;
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memcpy(tcon_buf, tmp_buf, size);

	BinFileUninit();

	// start handle lcd_tcon param
	memset((void *)tmp_buf, 0, CC_MAX_TCON_BIN_SIZE);
	tmp_len = ReadTconBinParam(tmp_buf);
	//ALOGD("%s, start check lcd_tcon param data (0x%x).\n", __func__, tmp_len);
	if (check_param_valid(1, gLcdTconDataCnt, tcon_buf, tmp_len, tmp_buf) ==
		CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check tcon bin data diff (0x%x), save tcon bin data.\n",
			__func__, tmp_len);
		SaveTconBinParam(gLcdTconDataCnt, tcon_buf);
	}
	// end handle lcd_tcon param

	free(tmp_buf);
	tmp_buf = NULL;
	free(tcon_buf);
	tcon_buf = NULL;

	return 0;
}

static int handle_tcon_ext_pmu_data(int index, int flag, unsigned char *buf,
				    unsigned int offset, unsigned int data_len,
				    int mul_index)
{
	char *file_name, str[2][50];
	unsigned int data_size = 0, i, file_find = 0;
	unsigned char *bin_buf = NULL;

	if (!buf) {
		ALOGE("%s, buf is null\n", __func__);
		return -1;
	}
	buf[0] = 0; /* init invalid data */
	i = 0;

	if (index >= 4) {
		ALOGE("%s, invalid index %d\n", __func__, index);
		return -1;
	}

	if (flag == 4 || flag == 5 || flag == 6) {
		sprintf(str[0], "model_tcon_ext_b%d_%d_spi", index, mul_index);
		sprintf(str[1], "model_tcon_ext_b%d_%d", index, mul_index);
	} else {
		sprintf(str[0], "model_tcon_ext_b%d_spi", index);
		sprintf(str[1], "model_tcon_ext_b%d", index);
	}

	while (i < 2) {
		file_name = env_get(str[i]);
		if (file_name == NULL) {
			if (model_debug_flag & DEBUG_NORMAL)
				ALOGD("%s: no %s path\n", __func__, str[i]);
		} else {
			if (iniIsFileExist(file_name)) {
				if (model_debug_flag & DEBUG_NORMAL)
					ALOGD("%s: %s: %s\n", __func__, str[i], file_name);
				file_find = 1;
				break;
			}
			if (model_debug_flag & DEBUG_NORMAL) {
				ALOGE("%s: %s: \"%s\" not exist.\n",
					__func__, str[i], file_name);
			}
		}
		i++;
	}
	if (file_find == 0)
		return -1;

	data_size = handle_read_bin_file(file_name, LCD_EXTERN_INIT_ON_MAX);
	if (data_size == 0) {
		ALOGE("%s, %s data_size %d error!\n", __func__, str[i], data_size);
		return -1;
	}
	if (data_size > LCD_EXTERN_INIT_ON_MAX) {
		ALOGE("%s, %s data_size %d out of support(max %d)!\n",
			__func__, str[i], data_size, LCD_EXTERN_INIT_ON_MAX);
		return -1;
	}

	switch (flag) {
	case 1:
	case 4:
		buf[0] = data_size;
		GetBinData(&buf[1], data_size);
		break;
	case 2: /* data with reg addr auto fill */
	case 5:
		buf[0] = (data_size + 1); /* data size include reg start */
		buf[1] = 0x00;            /* reg start */
		GetBinData(&buf[2], data_size);
		break;
	case 3:
	case 6:
		if (data_size < (offset + data_len - 1)) {
			ALOGE("%s, %s suspend size %d out of data_size(%d)!\n",
			      __func__, str[i], (offset + data_len - 1),
			      data_size);
			return -1;
		}

		bin_buf = (unsigned char *)malloc(data_size);
		if (bin_buf == NULL) {
			ALOGE("%s, malloc buffer memory error!!!\n", __func__);
			return -1;
		}
		buf[0] = data_len;
		buf[1] = offset;
		GetBinData(bin_buf, data_size);
		memcpy(&buf[2], &bin_buf[offset], data_len - 1);
		free(bin_buf);
		break;
	default:
		break;
	}

	if (model_debug_flag & DEBUG_LCD_EXTERN) {
		ALOGD("%s: %s:\n", __func__, str[i]);
		for (i = 0; i < (buf[0] + 1); i++)
			printf(" 0x%02x", buf[i]);
		printf("\n");
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s %s finish\n", __func__, str[i]);

	BinFileUninit();

	return 0;
}

#define TCON_VAC_SET_PARAM_NUM    3
#define TCON_VAC_LUT_PARAM_NUM    256
int handle_tcon_vac(unsigned char *vac_data, unsigned int vac_mem_size)
{
	int i, n, tmp_cnt, len;
	char *file_name;
	const char *ini_value = NULL;
	unsigned int tmp_buf[512];
	unsigned int data_cnt = 0;

	file_name = env_get("model_tcon_vac");
	if (file_name == NULL) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_vac path\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_vac: %s\n", __func__, file_name);
	if ((vac_data == NULL) || (!vac_mem_size)) {
		ALOGE("%s, buffer memory or data size error!!!\n", __func__);
		return -1;
	}

	IniParserInit();

	if (IniParseFile(file_name) < 0) {
		ALOGE("%s, ini load file error!\n", __func__);
		IniParserUninit();
		free(vac_data);
		vac_data = NULL;
		return -1;
	}
	if (model_debug_flag & DEBUG_TCON)
		ALOGD("vac_data addr: 0x%p\n", vac_data);

	n = 8;
	len = TCON_VAC_SET_PARAM_NUM;

	ini_value = IniGetString("lcd_tcon_vac", "vac_set", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt = tmp_cnt;

	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_set data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if (model_debug_flag & DEBUG_TCON) {
			ALOGD("vac_set: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n+i*2], vac_data[n+i*2+1],
			      tmp_buf[i]);
		}
	}

	len = TCON_VAC_LUT_PARAM_NUM;

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt1", "null");
		tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt1 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (TCON_VAC_SET_PARAM_NUM * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt1_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt2", "null");
		tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt2 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt2_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
				vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt3_1", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt3_1 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt3_1_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
				vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt3_2", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt3_2 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if ((data_cnt * 2) > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt3_2_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
				vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt3_3", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((data_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt3_3 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt3_3_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
				vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt3_4", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt3_4 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt3_4_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
				vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt3_5", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt3_5 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt3_5_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
				vac_data[n+i*2], vac_data[n+i*2+1],
				tmp_buf[i]);
		}
	}

	ini_value = IniGetString("lcd_tcon_vac", "vac_ramt3_6", "null");
	tmp_cnt = trans_buffer_data(ini_value, tmp_buf);
	data_cnt += tmp_cnt;
	if ((tmp_cnt > CC_MAX_TCON_VAC_SIZE) || (tmp_cnt < len)) {
		ALOGE("%s: invalid vac_ramt3_6 data cnt %d\n", __func__, tmp_cnt);
		return -1;
	}
	if (data_cnt > vac_mem_size) {
		ALOGE("data size %d is out of memory size %d (data_cnt=%d)\n",
		      (data_cnt * 2), vac_mem_size, data_cnt);
		return -1;
	}
	n += (len * 2);
	for (i = 0; i < len; i++) {
		vac_data[n+i*2] = tmp_buf[i] & 0xff;
		vac_data[n+i*2+1] = (tmp_buf[i] >> 8) & 0xff;
		if ((model_debug_flag & DEBUG_TCON) && (i < 30)) {
			ALOGD("vac_ramt3_6_data: 0x%02x, 0x%02x; tmp_buf: 0x%04x\n",
			      vac_data[n+i*2], vac_data[n+i*2+1],
			      tmp_buf[i]);
		}
	}

	/*add check data: total_size(4byte) + crc(4byte) +
	 *crc todo
	*/
	vac_data[0] = data_cnt & 0xff;
	vac_data[1] = (data_cnt >> 8) & 0xff;
	vac_data[2] = (data_cnt >> 16) & 0xff;
	vac_data[3] = (data_cnt >> 24) & 0xff;

	vac_data[4] = model_data_checksum(&vac_data[8], data_cnt);
	vac_data[5] = model_data_lrc(&vac_data[8], data_cnt);
	vac_data[6] = 0x55;
	vac_data[7] = 0xaa;

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s finish\n", __func__);

	IniParserUninit();
	return 0;
}

int handle_tcon_demura_set(unsigned char *demura_set_data,
			   unsigned int demura_set_size)
{
	unsigned long int bin_size;
	char *file_name;
	int n;

	file_name = env_get("model_tcon_demura_set");
	if (file_name == NULL) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_demura_set path\n", __func__);
		return -1;
	}

	if ((demura_set_data == NULL) || (!demura_set_size)) {
		ALOGE("%s, buffer or size error!!!\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_demura_set: %s\n", __func__, file_name);
	bin_size = handle_read_bin_file(file_name, CC_MAX_TCON_DEMURA_SET_SIZE);
	if (!bin_size || (bin_size > demura_set_size)) {
		ALOGE("%s, bin_size 0x%lx error!(memory_size 0x%x)\n",
		      __func__, bin_size, demura_set_size);
		return -1;
	}

	n = 8;
	GetBinData(&demura_set_data[n], bin_size);

	demura_set_data[0] = bin_size & 0xff;
	demura_set_data[1] = (bin_size >> 8) & 0xff;
	demura_set_data[2] = (bin_size >> 16) & 0xff;
	demura_set_data[3] = (bin_size >> 24) & 0xff;

	demura_set_data[4] = model_data_checksum(&demura_set_data[8], bin_size);
	demura_set_data[5] = model_data_lrc(&demura_set_data[8], bin_size);
	demura_set_data[6] = 0x55;
	demura_set_data[7] = 0xaa;

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s finish\n", __func__);

	BinFileUninit();

	return 0;
}

int handle_tcon_demura_lut(unsigned char *demura_lut_data,
			   unsigned int demura_lut_size)
{
	unsigned long int bin_size;
	char *file_name;
	int n;

	file_name = env_get("model_tcon_demura_lut");
	if (file_name == NULL) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_demura_lut path\n", __func__);
		return -1;
	}

	if ((demura_lut_data == NULL) || (!demura_lut_size)) {
		ALOGE("%s, buffer memory or size error!!!\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_demura_lut: %s\n", __func__, file_name);
	bin_size = handle_read_bin_file(file_name, CC_MAX_TCON_DEMURA_LUT_SIZE);
	if (!bin_size || (bin_size > demura_lut_size)) {
		ALOGE("%s, bin_size 0x%lx error!(memory_size 0x%x)\n",
		      __func__, bin_size, demura_lut_size);
		return -1;
	}

	n = 8;
	GetBinData(&demura_lut_data[n], bin_size);

	demura_lut_data[0] = bin_size & 0xff;
	demura_lut_data[1] = (bin_size >> 8) & 0xff;
	demura_lut_data[2] = (bin_size >> 16) & 0xff;
	demura_lut_data[3] = (bin_size >> 24) & 0xff;

	demura_lut_data[4] = model_data_checksum(&demura_lut_data[8], bin_size);
	demura_lut_data[5] = model_data_lrc(&demura_lut_data[8], bin_size);
	demura_lut_data[6] = 0x55;
	demura_lut_data[7] = 0xaa;

	if (model_debug_flag)
		ALOGD("%s finish, bin_size = 0x%lx\n", __func__, bin_size);

	BinFileUninit();

	return 0;
}

int handle_tcon_acc_lut(unsigned char *acc_lut_data, unsigned int acc_lut_size)
{
	unsigned long int bin_size;
	char *file_name;
	int n;

	file_name = env_get("model_tcon_acc_lut");
	if (!file_name) {
		if (model_debug_flag & DEBUG_NORMAL)
			ALOGD("%s, no model_tcon_acc_lut path\n", __func__);
		return -1;
	}

	if ((!acc_lut_data) || (acc_lut_size == 0)) {
		ALOGE("%s, buffer memory or size error!!!\n", __func__);
		return -1;
	}

	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: model_tcon_acc_lut: %s\n", __func__, file_name);
	bin_size = handle_read_bin_file(file_name, CC_MAX_TCON_ACC_LUT_SIZE);
	if (!bin_size || (bin_size > acc_lut_size)) {
		ALOGE("%s, bin_size 0x%lx error!(memory_size 0x%x)\n",
		      __func__, bin_size, acc_lut_size);
		return -1;
	}

	n = 8;
	GetBinData(&acc_lut_data[n], bin_size);

	acc_lut_data[0] = bin_size & 0xff;
	acc_lut_data[1] = (bin_size >> 8) & 0xff;
	acc_lut_data[2] = (bin_size >> 16) & 0xff;
	acc_lut_data[3] = (bin_size >> 24) & 0xff;

	acc_lut_data[4] = model_data_checksum(&acc_lut_data[8], bin_size);
	acc_lut_data[5] = model_data_lrc(&acc_lut_data[8], bin_size);
	acc_lut_data[6] = 0x55;
	acc_lut_data[7] = 0xaa;

	if (model_debug_flag)
		ALOGD("%s finish, bin_size = 0x%lx\n", __func__, bin_size);

	BinFileUninit();

	return 0;
}

int handle_tcon_data_load(unsigned char **buf, unsigned int index)
{
	unsigned char *data_buf;
	unsigned long int bin_size, new_size;
	unsigned int data_size;
	unsigned int data_crc32, temp_crc32;
	char *file_name;

	if (!buf) {
		ALOGE("%s, buf is null\n", __func__);
		return -1;
	}

	file_name = handle_tcon_path_file_name_get(index);
	if (!file_name)
		return -1;

	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s: tcon_data[%d] file name: %s\n", __func__, index, file_name);

	bin_size = handle_read_bin_file(file_name, CC_MAX_DATA_SIZE);
	if (bin_size == 0) {
		ALOGE("%s, bin_size 0x%lx error!\n", __func__, bin_size);
		return -1;
	}

	data_buf = buf[index];
	if (data_buf) { /* already exist for reload */
		data_size = data_buf[8] |
			(data_buf[9] << 8) |
			(data_buf[10] << 16) |
			(data_buf[11] << 24);
		if (data_size >= bin_size) {
			memset(data_buf, 0, data_size);
			goto handle_tcon_data_load_next;
		}
		free(data_buf);
		buf[index] = NULL;
	}
	/* note: all the tcon data buf size must align to 32byte */
	new_size = handle_tcon_char_data_size_align(bin_size);
	data_buf = (unsigned char *)malloc(new_size);
	if (!data_buf) {
		ALOGE("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(data_buf, 0, new_size);
	buf[index] = data_buf;

handle_tcon_data_load_next:
	GetBinData(data_buf, bin_size);
	data_size = data_buf[8] |
		(data_buf[9] << 8) |
		(data_buf[10] << 16) |
		(data_buf[11] << 24);
	if (data_size > bin_size || data_size == 0) {
		ALOGE("%s: data_size 0x%x invalid, bin_size 0x%lx\n",
			__func__, data_size, bin_size);
		free(data_buf);
		buf[index] = NULL;
		return -1;
	}

	/* data check */
	data_crc32 = data_buf[0] |
		(data_buf[1] << 8) |
		(data_buf[2] << 16) |
		(data_buf[3] << 24);
	temp_crc32 = CalCRC32(0, &data_buf[4], (data_size - 4));

	if (model_debug_flag & DEBUG_TCON) {
		ALOGD("%s: tcon_data[%d] crc32=0x%08x(0x%02x)\n",
			 __func__, index, temp_crc32, data_crc32);
	}
	if (data_crc32 != temp_crc32) {
		ALOGE("%s: tcon_data[%d] crc32 check error\n", __func__, index);
		free(data_buf);
		buf[index] = NULL;
		return -1;
	}

	if (model_debug_flag & DEBUG_TCON)
		ALOGD("%s %d finish, bin_size = 0x%lx\n", __func__, index, bin_size);

	BinFileUninit();

	return 0;
}
#endif

#ifdef CONFIG_AML_LCD_BL_LDIM
int handle_ldim_dev_zone_mapping_get(unsigned char *buf, unsigned int size,
				     const char *path)
{
	unsigned int bin_size = 0;

	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s: %s\n", __func__, path);

	if (!buf) {
		ALOGE("%s, buf is null\n", __func__);
		return -1;
	}

	bin_size = handle_read_bin_file(path, CC_MAX_LDIM_DEV_ZONE_MAP_SIZE);
	if (bin_size == 0)
		return -1;
	if (bin_size > size) {
		ALOGE("%s, file \"%s\" size 0x%x bigger than buf size 0x%x\n",
		      __func__, path, bin_size, size);
		return -1;
	}

	GetBinData(buf, size);
	if (model_debug_flag & DEBUG_BACKLIGHT)
		ALOGD("%s: load ldim zone_mapping bin\n", __func__);

	return 0;
}
#endif

int handle_panel_ini(int index)
{
	int tmp_len = 0;
	unsigned char *tmp_buf = NULL;
	unsigned char *lcd_buf = NULL;
	struct bl_attr_s *bl_attr = NULL;
	struct ldim_dev_attr_s *ldim_dev_attr = NULL;
	struct panel_misc_s misc_attr;
	unsigned char *tcon_spi = NULL;
	struct lcd_optical_attr_s *optical_attr = NULL;
	char *file_name;
	int print_flag;
	char str[15];

	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	print_flag = env_get_ulong("model_debug_print", 16, 0xffff);
	if (print_flag != 0xffff) {
		model_debug_flag = print_flag;
		ALOGD("model_debug_flag: %d\n", model_debug_flag);
	}

	file_name = env_get(str);
	if (!file_name) {
		ALOGE("%s, %s path error!!!\n", __func__, str);
		return -1;
	}

	tmp_buf = (unsigned char *)malloc(CC_MAX_DATA_SIZE);
	if (!tmp_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		return -1;
	}
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);

	lcd_buf = (unsigned char *)malloc(CC_MAX_DATA_SIZE);
	if (!lcd_buf) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err0;
	}
	memset((void *)lcd_buf, 0, CC_MAX_DATA_SIZE);

	if (!lcd_ext_attr) {
		lcd_ext_attr = (struct lcd_ext_attr_s *)malloc(sizeof(struct lcd_ext_attr_s));
		if (!lcd_ext_attr) {
			ALOGE("%s, malloc buffer memory error!!!\n", __func__);
			goto handle_panel_ini_err1;
		}
	}
	memset((void *)lcd_ext_attr, 0, sizeof(struct lcd_ext_attr_s));

	bl_attr = (struct bl_attr_s *)malloc(sizeof(struct bl_attr_s));
	if (!bl_attr) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err1;
	}
	memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));

#ifdef CONFIG_AML_LCD_BL_LDIM
	ldim_dev_attr = (struct ldim_dev_attr_s *)malloc(sizeof(struct ldim_dev_attr_s));
	if (!ldim_dev_attr) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err2;
	}
	memset((void *)ldim_dev_attr, 0, sizeof(struct ldim_dev_attr_s));
#endif

#ifdef CONFIG_AML_LCD_TCON
	tcon_spi = (unsigned char *)malloc(CC_MAX_TCON_SPI_SIZE);
	if (!tcon_spi) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err3;
	}
	memset(tcon_spi, 0, CC_MAX_TCON_SPI_SIZE);
#endif

	optical_attr = (struct lcd_optical_attr_s *)malloc(sizeof(struct lcd_optical_attr_s));
	if (!optical_attr) {
		ALOGE("%s, malloc buffer memory error!!!\n", __func__);
		goto handle_panel_ini_err4;
	}
	memset((void *)optical_attr, 0, sizeof(struct lcd_optical_attr_s));

	//init misc attr as default
	memset((void *)&misc_attr, 0, sizeof(struct panel_misc_s));
	strcpy(misc_attr.version, "V001");
	strcpy(misc_attr.outputmode, "1080p60hz");
	misc_attr.panel_reverse = 0;

	// start handle panel ini name
	if (model_debug_flag & DEBUG_NORMAL)
		ALOGD("%s: %s: %s\n", __func__, str, file_name);
	if (parse_panel_ini(file_name, lcd_buf, lcd_ext_attr,
		bl_attr, ldim_dev_attr, &misc_attr,
		tcon_spi, optical_attr) < 0) {
		ALOGE("%s, parse_panel_ini file name \"%s\" fail.\n",
		      __func__, file_name);
		goto handle_panel_ini_err5;
	}

	// start handle lcd param
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	tmp_len = read_lcd_param(index, tmp_buf);
	//ALOGD("%s, start check lcd param data (0x%x).\n", __func__, tmp_len);
	if (check_param_valid(0, glcd_dcnt, lcd_buf, tmp_len, tmp_buf) ==
	    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check lcd param data diff (0x%x), save new param.\n",
		      __func__, tmp_len);
		save_lcd_param(index, glcd_dcnt, lcd_buf);
	}
	// end handle lcd param

	// start handle lcd extern param
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	tmp_len = read_lcd_extern_param(index, tmp_buf);
	//ALOGD("%s, start check lcd extern param data (0x%x).\n", __func__, tmp_len);
	if (check_param_valid(0, glcd_ext_dcnt, (unsigned char *)lcd_ext_attr, tmp_len, tmp_buf) ==
	    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check lcd extern param data diff (0x%x), save new param.\n",
		      __func__, tmp_len);
		save_lcd_extern_param(index, glcd_ext_dcnt, (unsigned char *)lcd_ext_attr);
	}
	// end handle lcd extern param

	// start handle backlight param
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	tmp_len = read_backlight_param(index, tmp_buf);
	//ALOGD("%s, start check backlight param data (0x%x).\n", __func__, tmp_len);
	if (check_param_valid(0, gbl_dcnt, (unsigned char *)bl_attr, tmp_len, tmp_buf) ==
	    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
		ALOGD("%s, check backlight param data diff (0x%x), save new param.\n",
		      __func__, tmp_len);
		save_backlight_param(index, gbl_dcnt, (unsigned char *)bl_attr);
	}
	// end handle backlight param

#ifdef CONFIG_AML_LCD_BL_LDIM
	// start handle ldim_dev param
	if (g_ldim_dev_valid) {
		memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
		tmp_len = read_ldim_dev_param(tmp_buf);
		//ALOGD("%s, start check ldim_dev param data (0x%x).\n", __func__, tmp_len);
		if (check_param_valid(0, gldim_dev_dcnt, (unsigned char *)ldim_dev_attr,
			tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
			ALOGD("%s, check ldim_dev param data diff (0x%x), save new param.\n",
			      __func__, tmp_len);
			save_ldim_dev_param(gldim_dev_dcnt, (unsigned char *)ldim_dev_attr);
		}
	}
	// end handle ldim_dev param
#endif

#ifdef CONFIG_AML_LCD_TCON
	// start handle lcd_tcon_spi param
	if (gLcdTconSpi_cnt) {
		memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
		tmp_len = ReadTconSpiParam(tmp_buf);
		//ALOGD("%s, start check lcd_tcon_spi param data (0x%x).\n", __func__, tmp_len);
		if (check_param_valid(0, gLcdTconSpi_cnt, tcon_spi, tmp_len, tmp_buf) ==
		    CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
			ALOGD("%s, check lcd_tcon_spi param data diff (0x%x), save new param.\n",
			      __func__, tmp_len);
			SaveTconSpiParam(gLcdTconSpi_cnt, tcon_spi);
		}
	}
	// end handle lcd_tcon_spi param
#endif

	// start handle lcd_optical param
	if (glcd_optical_dcnt) {
		memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
		tmp_len = ReadLcdOpticalParam(index, tmp_buf);
		//ALOGD("%s, start check lcd_tcon_spi param data (0x%x).\n", __func__, tmp_len);
		if (check_param_valid(0, glcd_optical_dcnt, (unsigned char *)optical_attr,
			tmp_len, tmp_buf) == CC_PARAM_CHECK_ERROR_NEED_UPDATE_PARAM) {
			ALOGD("%s, check lcd_optical param data diff (0x%x), save new param.\n",
			      __func__, tmp_len);
			SaveLcdOpticalParam(index, glcd_optical_dcnt,
				(unsigned char *)optical_attr);
		}
	}
	// end handle lcd_optical param

	memset((void *)optical_attr, 0, sizeof(struct lcd_optical_attr_s));
	free(optical_attr);
#ifdef CONFIG_AML_LCD_TCON
	memset(tcon_spi, 0, CC_MAX_TCON_SPI_SIZE);
	free(tcon_spi);
#endif
#ifdef CONFIG_AML_LCD_BL_LDIM
	memset((void *)ldim_dev_attr, 0, sizeof(struct ldim_dev_attr_s));
	free(ldim_dev_attr);
#endif
	memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));
	free(bl_attr);
	memset((void *)lcd_buf, 0, CC_MAX_DATA_SIZE);
	free(lcd_buf);
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	free(tmp_buf);

#ifdef CONFIG_AML_LCD_TCON
	if (g_lcd_tcon_valid)
		handle_tcon_bin();
#endif

	return 0;

handle_panel_ini_err5:
	memset((void *)optical_attr, 0, sizeof(struct lcd_optical_attr_s));
	free(optical_attr);
handle_panel_ini_err4:
#ifdef CONFIG_AML_LCD_TCON
	memset(tcon_spi, 0, CC_MAX_TCON_SPI_SIZE);
	free(tcon_spi);
handle_panel_ini_err3:
#endif
#ifdef CONFIG_AML_LCD_BL_LDIM
	memset((void *)ldim_dev_attr, 0, sizeof(struct ldim_dev_attr_s));
	free(ldim_dev_attr);
handle_panel_ini_err2:
#endif
	memset((void *)bl_attr, 0, sizeof(struct bl_attr_s));
	free(bl_attr);
handle_panel_ini_err1:
	memset((void *)lcd_buf, 0, CC_MAX_DATA_SIZE);
	free(lcd_buf);
handle_panel_ini_err0:
	memset((void *)tmp_buf, 0, CC_MAX_DATA_SIZE);
	free(tmp_buf);

	return -1;
}

static void model_list_panel_path(int index)
{
	char *path_str, str[15];

	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	path_str = env_get(str);
	if (path_str)
		printf("current %s: %s\n", str, path_str);
}
#endif

int parse_model_sum(int index, const char *file_name, char *model_name)
{
	const char *ini_value = NULL;
#ifdef CONFIG_AML_LCD
	char str[15];
#endif

	IniParserInit();

	if (IniParseFile(file_name) < 0) {
		ALOGE("%s, ini load file error!\n", __func__);
		IniParserUninit();
		return -1;
	}

#ifdef CONFIG_AML_LCD
	if (index == 0)
		sprintf(str, "model_panel");
	else
		sprintf(str, "model%d_panel", index);

	ini_value = IniGetString(model_name, "PANELINI_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set(str, ini_value);
	else
		ALOGE("%s, invalid PANELINI_PATH!!!\n", __func__);
#endif

	ini_value = IniGetString(model_name, "EDID_14_FILE_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set("model_edid", ini_value);
	else
		ALOGD("%s, invalid EDID_14_FILE_PATH!!!\n", __func__);
	/*
	ini_value = IniGetString(model_name, "PQINI_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set("model_pq", ini_value);

	ini_value = IniGetString(model_name, "AMLOGIC_AUDIO_EFFECT_INI_PATH", "null");
	if (strcmp(ini_value, "null") != 0)
		env_set("model_audio", ini_value);
	*/
	IniParserUninit();

	return 0;
}

const char *get_model_sum_path(int index)
{
	char *model_path, str[15];

	if (index == 0)
		sprintf(str, "model_path");
	else
		sprintf(str, "model%d_path", index);

	model_path = env_get(str);
	if (model_path == NULL) {
		if (dynamic_partition) {
			if (index == 2)
				return DEFAULT_MODEL2_SUM_PATH2;
			else if (index == 1)
				return DEFAULT_MODEL1_SUM_PATH2;
			else
				return DEFAULT_MODEL_SUM_PATH2;
		} else {
			if (index == 2)
				return DEFAULT_MODEL2_SUM_PATH1;
			else if (index == 1)
				return DEFAULT_MODEL1_SUM_PATH1;
			else
				return DEFAULT_MODEL_SUM_PATH1;
		}
	}

	printf("%s: %s\n", __func__, model_path);
	return model_path;
}

int handle_model_list(void)
{
	char *model, str[15];
	int i;

	for (i = 0; i < 3; i++) {
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		model = env_get(str);
		if (!model) {
			if (model_debug_flag & DEBUG_NORMAL)
				ALOGD("%s, no %s\n", __func__, str);
			continue;
		}
		printf("current %s: %s\n", str, model);
#ifdef CONFIG_AML_LCD
		model_list_panel_path(i);
#endif

		IniParserInit();

		if (IniParseFile(get_model_sum_path(i)) < 0) {
			ALOGE("%s, ini load file error!\n", __func__);
			IniParserUninit();
			return -1;
		}

		printf("%s list:\n", str);
		IniListSection();
		printf("\n");

		IniParserUninit();
	}

	return 0;
}

int handle_model_sum(void)
{
	char *model, str[15];
	int i, ret;

	for (i = 0; i < 3; i++) {
		if (i == 0)
			sprintf(str, "model_name");
		else
			sprintf(str, "model%d_name", i);

		model = env_get(str);
		if (!model) {
			if (model_debug_flag & DEBUG_NORMAL)
				ALOGD("%s, no %s\n", __func__, str);
			continue;
		}
		ret = parse_model_sum(i, get_model_sum_path(i), model);
		if (ret < 0)
			continue;
#ifdef CONFIG_AML_LCD
		handle_panel_ini(i);
#endif
	}

	return 0;
}


// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <linux/stddef.h>
#include <amlogic/media/vout/hdmitx/hdmitx.h>

#define CEA_DATA_BLOCK_COLLECTION_ADDR_1StP 0x04
#define VIDEO_TAG 0x40
#define AUDIO_TAG 0x20
#define VENDOR_TAG 0x60
#define SPEAKER_TAG 0x80
#define EDID_MAX_BLOCK 8

#define HDMI_EDID_BLOCK_TYPE_RESERVED	        0
#define HDMI_EDID_BLOCK_TYPE_AUDIO		1
#define HDMI_EDID_BLOCK_TYPE_VIDEO		2
#define HDMI_EDID_BLOCK_TYPE_VENDER	        3
#define HDMI_EDID_BLOCK_TYPE_SPEAKER	        4
#define HDMI_EDID_BLOCK_TYPE_VESA		5
#define HDMI_EDID_BLOCK_TYPE_RESERVED2	        6
#define HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG       7

#define EXTENSION_VENDOR_SPECIFIC 0x1
#define EXTENSION_COLORMETRY_TAG 0x5
/* DRM stands for "Dynamic Range and Mastering " */
#define EXTENSION_DRM_STATIC_TAG	0x6
/* Video Format Preference Data block */
#define EXTENSION_VFPDB_TAG	0xd
#define EXTENSION_Y420_VDB_TAG	0xe
#define EXTENSION_Y420_CMDB_TAG	0xf
#define EXTENSION_SCDB_EXT_TAG	0x79

#define EDID_DETAILED_TIMING_DES_BLOCK0_POS 0x36
#define EDID_DETAILED_TIMING_DES_BLOCK1_POS 0x48
#define EDID_DETAILED_TIMING_DES_BLOCK2_POS 0x5A
#define EDID_DETAILED_TIMING_DES_BLOCK3_POS 0x6C

/* EDID descriptor Tag */
#define TAG_PRODUCT_SERIAL_NUMBER 0xFF
#define TAG_ALPHA_DATA_STRING 0xFE
#define TAG_RANGE_LIMITS 0xFD
#define TAG_DISPLAY_PRODUCT_NAME_STRING 0xFC /* MONITOR NAME */
#define TAG_COLOR_POINT_DATA 0xFB
#define TAG_STANDARD_TIMINGS 0xFA
#define TAG_DISPLAY_COLOR_MANAGEMENT 0xF9
#define TAG_CVT_TIMING_CODES 0xF8
#define TAG_ESTABLISHED_TIMING_III 0xF7
#define TAG_DUMMY_DES 0x10

static struct dispmode_vic dispmode_vic_tab[] = {
	{"480i60hz_4x3", HDMI_720x480i60_4x3},
	{"480p60hz_4x3", HDMI_720x480p60_4x3},
	{"576i50hz_4x3", HDMI_720x576i50_4x3},
	{"576p50hz_4x3", HDMI_720x576p50_4x3},
	{"480i60hz", HDMI_720x480i60_16x9},
	{"480p60hz", HDMI_720x480p60_16x9},
	{"576i50hz", HDMI_720x576i50_16x9},
	{"576p50hz", HDMI_720x576p50_16x9},
	{"720p50hz", HDMI_720p50},
	{"720p60hz", HDMI_720p60},
	{"1080i50hz", HDMI_1080i50},
	{"1080i60hz", HDMI_1080i60},
	{"1080p50hz", HDMI_1080p50},
	{"1080p30hz", HDMI_1080p30},
	{"1080p25hz", HDMI_1080p25},
	{"1080p24hz", HDMI_1080p24},
	{"1080p60hz", HDMI_1080p60},
	{"1080p120hz", HDMI_1080p120},
	{"2560x1080p50hz", HDMI_2560x1080p50_64x27},
	{"2560x1080p60hz", HDMI_2560x1080p60_64x27},
	{"2160p30hz", HDMI_4k2k_30},
	{"2160p25hz", HDMI_4k2k_25},
	{"2160p24hz", HDMI_4k2k_24},
	{"smpte24hz", HDMI_4k2k_smpte_24},
	{"smpte25hz", HDMI_4096x2160p25_256x135},
	{"smpte30hz", HDMI_4096x2160p30_256x135},
	{"smpte50hz420", HDMI_4096x2160p50_256x135_Y420},
	{"smpte60hz420", HDMI_4096x2160p60_256x135_Y420},
	{"2160p60hz420", HDMI_3840x2160p60_16x9_Y420},
	{"2160p50hz420", HDMI_3840x2160p50_16x9_Y420},
	{"smpte50hz", HDMI_4096x2160p50_256x135},
	{"smpte60hz", HDMI_4096x2160p60_256x135},
	{"2160p60hz", HDMI_4k2k_60},
	{"2160p50hz", HDMI_4k2k_50},
	{"640x480p60hz", HDMIV_640x480p60hz},
	{"800x480p60hz", HDMIV_800x480p60hz},
	{"800x600p60hz", HDMIV_800x600p60hz},
	{"852x480p60hz", HDMIV_852x480p60hz},
	{"854x480p60hz", HDMIV_854x480p60hz},
	{"1024x600p60hz", HDMIV_1024x600p60hz},
	{"1024x768p60hz", HDMIV_1024x768p60hz},
	{"1152x864p75hz", HDMIV_1152x864p75hz},
	{"1280x600p60hz", HDMIV_1280x600p60hz},
	{"1280x768p60hz", HDMIV_1280x768p60hz},
	{"1280x800p60hz", HDMIV_1280x800p60hz},
	{"1280x960p60hz", HDMIV_1280x960p60hz},
	{"1280x1024p60hz", HDMIV_1280x1024p60hz},
	{"1280x1024", HDMIV_1280x1024p60hz}, /* alias of "1280x1024p60hz" */
	{"1360x768p60hz", HDMIV_1360x768p60hz},
	{"1366x768p60hz", HDMIV_1366x768p60hz},
	{"1400x1050p60hz", HDMIV_1400x1050p60hz},
	{"1440x900p60hz", HDMIV_1440x900p60hz},
	{"1440x2560p60hz", HDMIV_1440x2560p60hz},
	{"1600x900p60hz", HDMIV_1600x900p60hz},
	{"1600x1200p60hz", HDMIV_1600x1200p60hz},
	{"1680x1050p60hz", HDMIV_1680x1050p60hz},
	{"1920x1200p60hz", HDMIV_1920x1200p60hz},
	{"2160x1200p90hz", HDMIV_2160x1200p90hz},
	{"2560x1440p60hz", HDMIV_2560x1440p60hz},
	{"2560x1600p60hz", HDMIV_2560x1600p60hz},
	{"3440x1440p60hz", HDMIV_3440x1440p60hz},
	{"2400x1200p90hz", HDMIV_2400x1200p90hz},
	{"3840x1080p60hz", HDMIV_3840x1080p60hz},
};

static int hdmitx_edid_search_IEEEOUI(unsigned char *buf)
{
	int i;

	for (i = 0; i < 0x180 - 2; i++) {
		if (buf[i] == 0x03 && buf[i + 1] == 0x0c &&
		    buf[i + 2] == 0x00)
			return 1;
	}
	return 0;
}

/* check EDID strictly */
static int edid_check_valid(unsigned char *buf)
{
	unsigned int chksum = 0;
	unsigned int i = 0;

	/* check block 0 first 8 bytes */
	if (buf[0] != 0 || buf[7] != 0)
		return 0;
	for (i = 1; i < 7; i++) {
		if (buf[i] != 0xff)
			return 0;
	}

	/* check block 0 checksum */
	for (chksum = 0, i = 0; i < 0x80; i++)
		chksum += buf[i];

	if ((chksum & 0xff) != 0)
		return 0;

	/* check Extension flag at block 0 */
	/* for DVI: there may be >= 0 cta block,
	 * so it's normal to have only basic block
	 */
	if (buf[0x7e] == 0)
		return 1;

	/* check block 1 extension tag */
	if (!(buf[0x80] == 0x2 || buf[0x80] == 0xf0))
		return 0;

	/* check block 1 checksum */
	for (chksum = 0, i = 0x80; i < 0x100; i++)
		chksum += buf[i];

	if ((chksum & 0xff) != 0)
		return 0;

	return 1;
}

/* check the checksum for each sub block */
static int _check_edid_blk_chksum(unsigned char *block)
{
	unsigned int chksum = 0;
	unsigned int i = 0;
	struct hdmitx_dev *hdev = hdmitx_get_hdev();

	if (!(hdev->edid_check & 0x02)) {
		for (chksum = 0, i = 0; i < 0x80; i++)
			chksum += block[i];
		if ((chksum & 0xff) != 0)
			return 0;
	}
	return 1;
}

/* check the first edid block */
static int _check_base_structure(unsigned char *buf)
{
	unsigned int i = 0;
	struct hdmitx_dev *hdev = hdmitx_get_hdev();

	if (!(hdev->edid_check & 0x01)) {
		/* check block 0 first 8 bytes */
		if (buf[0] != 0 || buf[7] != 0)
			return 0;

		for (i = 1; i < 7; i++) {
			if (buf[i] != 0xff)
				return 0;
		}
	}

	/* check block 0 checksum */
	if (_check_edid_blk_chksum(buf) == 0)
		return 0;

	return 1;
}

/*
 * check the EDID validatiy
 * base structure: header, checksum
 * extension: the first non-zero byte, checksum
 */
static int check_dvi_hdmi_edid_valid(unsigned char *buf)
{
	int i;
	int blk_cnt = buf[0x7e] + 1;
	struct hdmitx_dev *hdev = hdmitx_get_hdev();

	/* limit blk_cnt to EDID_BLK_NO  */
	if (blk_cnt > EDID_BLK_NO)
		blk_cnt = EDID_BLK_NO;

	/* check block 0 */
	if (_check_base_structure(&buf[0]) == 0)
		return 0;

	if (blk_cnt == 1)
		return 1;

	/* check extension block 1 and more */
	for (i = 1; i < blk_cnt; i++) {
		if (!(hdev->edid_check & 0x01)) {
			if (buf[i * 0x80] == 0)
				return 0;
		}
		if (_check_edid_blk_chksum(&buf[i * 0x80]) == 0)
			return 0;
	}

	return 1;
}

/*
 * check EDID buf contains valid block numbers
 */
static unsigned int hdmitx_edid_check_valid_blocks(unsigned char *buf)
{
	unsigned int valid_blk_no = 0;
	unsigned int i = 0, j = 0;
	unsigned int tmp_chksum = 0;

	for (j = 0; j < EDID_MAX_BLOCK; j++) {
		for (i = 0; i < 128; i++)
			tmp_chksum += buf[i + j * 128];
		if (tmp_chksum != 0) {
			valid_blk_no++;
			if ((tmp_chksum & 0xff) == 0)
				printf("check sum valid\n");
			else
				printf("check sum invalid\n");
		}
		tmp_chksum = 0;
	}
	return valid_blk_no;
}

/*
 * if the EDID is invalid, then set the fallback mode
 * Resolution & RefreshRate:
 *   1920x1080p60hz 16:9
 *   1280x720p60hz 16:9 (default)
 *   720x480p 16:9
 * ColorSpace: RGB
 * ColorDepth: 8bit
 */
static void edid_set_fallback_mode(struct rx_cap *prxcap)
{
	if (!prxcap)
		return;

	/* EDID extended blk chk error, set the 720p60, rgb,8bit */
	prxcap->IEEEOUI = HDMI_IEEEOUI;
	prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK; /* 165MHZ / 5 */
	prxcap->native_Mode = 0; /* only RGB */
	prxcap->dc_y444 = 0; /* only 8bit */
	prxcap->VIC_count = 0x3;
	prxcap->VIC[0] = HDMI_1920x1080p60_16x9;
	prxcap->VIC[1] = HDMI_1280x720p60_16x9;
	prxcap->VIC[2] = HDMI_720x480p60_16x9;
	prxcap->native_VIC = HDMI_1280x720p60_16x9;
}

/* add default VICs for all zeroes case */
static void hdmitx_edid_set_default_vic(struct rx_cap *prxcap)
{
	prxcap->VIC_count = 0x2;
	prxcap->VIC[0] = HDMI_720x480p60_16x9;
	prxcap->VIC[1] = HDMI_1280x720p60_16x9;
	prxcap->native_VIC = HDMI_720x480p60_16x9;
	/* hdmitx_device->vic_count = prxcap->VIC_count; */
	printf("set default vic\n");
}

static void _edid_parse_base_structure(struct rx_cap *prxcap,
	unsigned char *EDID_buf)
{
	unsigned char checksum;
	unsigned char zero_numbers;
	unsigned char cta_block_count;
	int i;

	/* skip parsing base block, DTD will be parsed later */
	cta_block_count = EDID_buf[0x7E];

	if (cta_block_count == 0) {
		printf("EDID BlockCount=0\n");
		/* DVI case judgement: only contains one block and
		 * checksum valid
		 */
		checksum = 0;
		zero_numbers = 0;
		for (i = 0; i < 128; i++) {
			checksum += EDID_buf[i];
			if (EDID_buf[i] == 0)
				zero_numbers++;
		}
		printf("edid blk0 checksum:%d ext_flag:%d\n",
			checksum, EDID_buf[0x7e]);
		if ((checksum & 0xff) == 0)
			prxcap->IEEEOUI = 0;
		else
			prxcap->IEEEOUI = HDMI_IEEEOUI;
		if (zero_numbers > 120)
			prxcap->IEEEOUI = HDMI_IEEEOUI;
		hdmitx_edid_set_default_vic(prxcap);
	}
}

/* if edid block 0 are all zeros, then consider RX as HDMI device */
static int edid_zero_data(unsigned char *buf)
{
	int sum = 0;
	int i = 0;

	for (i = 0; i < 128; i++)
		sum += buf[i];

	if (sum == 0)
		return 1;
	else
		return 0;
}

static void dump_dtd_info(struct dtd *t)
{
	return; /* debug only */
	printk("%s[%d]\n", __func__, __LINE__);
#define PR(a) pr_info("%s %d\n", #a, t->a)
	PR(pixel_clock);
	PR(h_active);
	PR(h_blank);
	PR(v_active);
	PR(v_blank);
	PR(h_sync_offset);
	PR(h_sync);
	PR(v_sync_offset);
	PR(v_sync);
}

static void store_cea_idx(struct rx_cap *prxcap, enum hdmi_vic vic)
{
	int i;
	int already = 0;

	if (!prxcap || !vic)
		return;

	for (i = 0; (i < VIC_MAX_NUM) && (i < prxcap->VIC_count); i++) {
		if (vic == prxcap->VIC[i]) {
			already = 1;
			break;
		}
	}
	if (!already && prxcap->VIC_count < VIC_MAX_NUM - 1) {
		prxcap->VIC[prxcap->VIC_count] = vic;
		prxcap->VIC_count++;
	}
}

static int edid_parsingdrmstaticblock(struct rx_cap *prxcap,
	unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, data_end = 0;
	unsigned int pos = 0;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f);
	memset(prxcap->hdr_info.rawdata, 0, 7);
	memcpy(prxcap->hdr_info.rawdata, buf, data_end + 1);
	pos++;
	ext_tag = buf[pos];
	if ((tag != HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG)
		|| (ext_tag != EXTENSION_DRM_STATIC_TAG))
		goto INVALID_DRM_STATIC;
	pos++;
	prxcap->hdr_info.hdr_sup_eotf_sdr = !!(buf[pos] & (0x1 << 0));
	prxcap->hdr_info.hdr_sup_eotf_hdr = !!(buf[pos] & (0x1 << 1));
	prxcap->hdr_info.hdr_sup_eotf_smpte_st_2084 = !!(buf[pos] & (0x1 << 2));
	prxcap->hdr_info.hdr_sup_eotf_hlg = !!(buf[pos] & (0x1 << 3));
	pos++;
	prxcap->hdr_info.hdr_sup_SMD_type1 = !!(buf[pos] & (0x1 << 0));
	pos++;
	if (data_end == 3)
		return 0;
	if (data_end == 4) {
		prxcap->hdr_info.hdr_lum_max = buf[pos];
		return 0;
	}
	if (data_end == 5) {
		prxcap->hdr_info.hdr_lum_max = buf[pos];
		prxcap->hdr_info.hdr_lum_avg = buf[pos + 1];
		return 0;
	}
	if (data_end == 6) {
		prxcap->hdr_info.hdr_lum_max = buf[pos];
		prxcap->hdr_info.hdr_lum_avg = buf[pos + 1];
		prxcap->hdr_info.hdr_lum_min = buf[pos + 2];
		return 0;
	}
	return 0;
INVALID_DRM_STATIC:
	printf("[%s] it's not a valid DRM STATIC BLOCK\n", __func__);
	return -1;
}

static void edid_parsingvendspec(struct rx_cap *prxcap,
	unsigned char *buf)
{
	struct dv_info *dv = &prxcap->dv_info;
	struct hdr10_plus_info *hdr10_plus = &prxcap->hdr10plus_info;
	unsigned char *dat = buf;
	unsigned char pos = 0;
	unsigned int ieeeoui = 0;
	u8 length = 0;

	length = dat[pos] & 0x1f;
	pos++;

	if (dat[pos] != 1) {
		printf("hdmitx: edid: parsing fail %s[%d]\n", __func__,
			__LINE__);
		return;
	}

	pos++;
	ieeeoui = dat[pos++];
	ieeeoui += dat[pos++] << 8;
	ieeeoui += dat[pos++] << 16;
	printf("%s:ieeeoui=0x%x,len=%u\n", __func__, ieeeoui, length);

	/*HDR10+ use vsvdb*/
	if (ieeeoui == HDR10_PLUS_IEEE_OUI) {
		memset(hdr10_plus, 0, sizeof(struct hdr10_plus_info));
		hdr10_plus->length = length;
		hdr10_plus->ieeeoui = ieeeoui;
		hdr10_plus->application_version = dat[pos] & 0x3;
		pos++;
		return;
	}

	if (ieeeoui == DV_IEEE_OUI) {
		/* it is a Dovi block*/
		memset(dv, 0, sizeof(struct dv_info));
		dv->block_flag = CORRECT;
		dv->length = length;
		memcpy(dv->rawdata, dat, dv->length + 1);
		dv->ieeeoui = ieeeoui;
		dv->ver = (dat[pos] >> 5) & 0x7;
		if (dv->ver > 2) {
			dv->block_flag = ERROR_VER;
			return;
		}
		/* Refer to DV 2.9 Page 27 */
		if (dv->ver == 0) {
			if (dv->length == 0x19) {
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				dv->sup_global_dimming = (dat[pos] >> 2) & 0x1;
				pos++;
				dv->Rx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Ry =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Gx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Gy =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Bx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->By =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->Wx =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->Wy =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->tminPQ =
					(dat[pos + 1] << 4) | (dat[pos] >> 4);
				dv->tmaxPQ =
					(dat[pos + 2] << 4) | (dat[pos] & 0xf);
				pos += 3;
				dv->dm_major_ver = dat[pos] >> 4;
				dv->dm_minor_ver = dat[pos] & 0xf;
				pos++;
				dv->support_DV_RGB_444_8BIT = 1;
				printf("v0 VSVDB: len=%d, sup_2160p60hz=%d\n",
					 dv->length, dv->sup_2160p60hz);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}

		if (dv->ver == 1) {
			if (dv->length == 0x0B) {/* Refer to DV 2.9 Page 33 */
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = dat[pos] & 0x1;
				dv->tmaxLUM = dat[pos] >> 1;
				pos++;
				dv->colorimetry = dat[pos] & 0x1;
				dv->tminLUM = dat[pos] >> 1;
				pos++;
				dv->low_latency = dat[pos] & 0x3;
				dv->Bx = 0x20 | ((dat[pos] >> 5) & 0x7);
				dv->By = 0x08 | ((dat[pos] >> 2) & 0x7);
				pos++;
				dv->Gx = 0x00 | (dat[pos] >> 1);
				dv->Ry = 0x40 | ((dat[pos] & 0x1) |
					((dat[pos + 1] & 0x1) << 1) |
					((dat[pos + 2] & 0x3) << 2));
				pos++;
				dv->Gy = 0x80 | (dat[pos] >> 1);
				pos++;
				dv->Rx = 0xA0 | (dat[pos] >> 3);
				pos++;
				dv->support_DV_RGB_444_8BIT = 1;
				if (dv->low_latency == 0x01)
					dv->support_LL_YCbCr_422_12BIT = 1;
				printf("v1 VSVDB: len=%d, sup_2160p60hz=%d, low_latency=%d\n",
				dv->length, dv->sup_2160p60hz, dv->low_latency);
			} else if (dv->length == 0x0E) {
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_2160p60hz = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = dat[pos] & 0x1;
				dv->tmaxLUM = dat[pos] >> 1;
				pos++;
				dv->colorimetry = dat[pos] & 0x1;
				dv->tminLUM = dat[pos] >> 1;
				pos += 2; /* byte8 is reserved as 0 */
				dv->Rx = dat[pos++];
				dv->Ry = dat[pos++];
				dv->Gx = dat[pos++];
				dv->Gy = dat[pos++];
				dv->Bx = dat[pos++];
				dv->By = dat[pos++];
				dv->support_DV_RGB_444_8BIT = 1;
				printf("v1 VSVDB: len=%d, sup_2160p60hz=%d\n",
					 dv->length, dv->sup_2160p60hz);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}
		if (dv->ver == 2) {
			/* v2 VSVDB length could be greater than 0xB
			 * and should not be treated as unrecognized
			 * block. Instead, we should parse it as a regular
			 * v2 VSVDB using just the remaining 11 bytes here
			 */
			if (dv->length >= 0x0B) {
				dv->sup_2160p60hz = 0x1;/*default*/
				dv->dm_version = (dat[pos] >> 2) & 0x7;
				dv->sup_yuv422_12bit = dat[pos] & 0x1;
				dv->sup_backlight_control = (dat[pos] >> 1) & 0x1;
				pos++;
				dv->sup_global_dimming = (dat[pos] >> 2) & 0x1;
				dv->backlt_min_luma = dat[pos] & 0x3;
				dv->tminPQ = dat[pos] >> 3;
				pos++;
				dv->Interface = dat[pos] & 0x3;
				dv->tmaxPQ = dat[pos] >> 3;
				pos++;
				dv->sup_10b_12b_444 = ((dat[pos] & 0x1) << 1) |
					(dat[pos + 1] & 0x1);
				dv->Gx = 0x00 | (dat[pos] >> 1);
				pos++;
				dv->Gy = 0x80 | (dat[pos] >> 1);
				pos++;
				dv->Rx = 0xA0 | (dat[pos] >> 3);
				dv->Bx = 0x20 | (dat[pos] & 0x7);
				pos++;
				dv->Ry = 0x40  | (dat[pos] >> 3);
				dv->By = 0x08  | (dat[pos] & 0x7);
				pos++;
				if (dv->Interface != 0x00 &&
				    dv->Interface != 0x01)
					dv->support_DV_RGB_444_8BIT = 1;

				dv->support_LL_YCbCr_422_12BIT = 1;
				if (dv->Interface == 0x01 ||
				    dv->Interface == 0x03) {
					if (dv->sup_10b_12b_444 == 0x1)
						dv->support_LL_RGB_444_10BIT = 1;
					if (dv->sup_10b_12b_444 == 0x2)
						dv->support_LL_RGB_444_12BIT = 1;
				}
				printf("v2 VSVDB: len=%d, sup_2160p60hz=%d, Interface=%d\n",
					 dv->length, dv->sup_2160p60hz, dv->Interface);
			} else {
				dv->block_flag = ERROR_LENGTH;
			}
		}

		if (pos > dv->length + 1)
			printf("hdmitx: edid: maybe invalid dv%d data\n", dv->ver);
		return;
	}
	/* future: other new VSVDB add here: */
}

static void edid_dtd_parsing(struct rx_cap *prxcap, unsigned char *data)
{
	struct hdmi_format_para *para = NULL;
	struct dtd *t = &prxcap->dtd[prxcap->dtd_idx];

	memset(t, 0, sizeof(struct dtd));
	t->pixel_clock = data[0] + (data[1] << 8);
	t->h_active = (((data[4] >> 4) & 0xf) << 8) + data[2];
	t->h_blank = ((data[4] & 0xf) << 8) + data[3];
	t->v_active = (((data[7] >> 4) & 0xf) << 8) + data[5];
	t->v_blank = ((data[7] & 0xf) << 8) + data[6];
	t->h_sync_offset = (((data[11] >> 6) & 0x3) << 8) + data[8];
	t->h_sync = (((data[11] >> 4) & 0x3) << 8) + data[9];
	t->v_sync_offset = (((data[11] >> 2) & 0x3) << 4) +
		((data[10] >> 4) & 0xf);
	t->v_sync = (((data[11] >> 0) & 0x3) << 4) + ((data[10] >> 0) & 0xf);
	t->flags = data[17];
/*
 * Special handling of 1080i60hz, 1080i50hz
 */
	if ((t->pixel_clock == 7425) && (t->h_active == 1920) &&
		(t->v_active == 1080)) {
		t->v_active = t->v_active / 2;
		t->v_blank = t->v_blank / 2;
	}
/*
 * Special handling of 480i60hz, 576i50hz
 */
	if (((((t->flags) >> 1) & 0x3) == 0) && (t->h_active == 1440)) {
		if (t->pixel_clock == 2700) /* 576i50hz */
			goto next;
		if ((t->pixel_clock - 2700) < 10) /* 480i60hz */
			t->pixel_clock = 2702;
next:
		t->v_active = t->v_active / 2;
		t->v_blank = t->v_blank / 2;
	}
/*
 * call hdmi_match_dtd_paras() to check t is matched with VIC
 */
	para = hdmi_match_dtd_paras(t);
	if (para) {
		/* diff 4x3 and 16x9 mode */
		if (para->vic == HDMI_720x480i60_4x3 ||
			para->vic == HDMI_720x480p60_4x3 ||
			para->vic == HDMI_720x576i50_4x3 ||
			para->vic == HDMI_720x576p50_4x3) {
			if (abs(t->v_image_size * 100 / t->h_image_size - 3 * 100 / 4) <= 2)
				t->vic = para->vic;
			else
				t->vic = para->vic + 1;
		} else {
			t->vic = para->vic;
		}
		prxcap->preferred_mode = prxcap->dtd[0].vic; /* Select dtd0 */
		if (0) /* debug only */
			pr_info("hdmitx: get dtd%d vic: %d\n",
				prxcap->dtd_idx, t->vic);
		prxcap->dtd_idx++;
		if (t->vic < HDMITX_VESA_OFFSET)
			store_cea_idx(prxcap, t->vic);
	} else
		dump_dtd_info(t);
}

/* parse Sink 4k2k information */
static void hdmitx_edid_4k2k_parse(struct rx_cap *prxcap, unsigned char *dat,
	unsigned size)
{
	if ((size > 4) || (size == 0)) {
		return;
	}
	while (size--) {
		if (*dat == 1)
			prxcap->VIC[prxcap->VIC_count] = HDMI_3840x2160p30_16x9;
		else if (*dat == 2)
			prxcap->VIC[prxcap->VIC_count] = HDMI_3840x2160p25_16x9;
		else if (*dat == 3)
			prxcap->VIC[prxcap->VIC_count] = HDMI_3840x2160p24_16x9;
		else if (*dat == 4)
			prxcap->VIC[prxcap->VIC_count] = HDMI_4096x2160p24_256x135;
		else
			;
		dat++;
		prxcap->VIC_count++;
	}
}

static void set_vsdb_dc_cap(struct rx_cap *prxcap)
{
	prxcap->dc_y444 = !!(prxcap->ColorDeepSupport & (1 << 3));
	prxcap->dc_30bit = !!(prxcap->ColorDeepSupport & (1 << 4));
	prxcap->dc_36bit = !!(prxcap->ColorDeepSupport & (1 << 5));
	prxcap->dc_48bit = !!(prxcap->ColorDeepSupport & (1 << 6));
}

static void set_vsdb_dc_420_cap(struct rx_cap *prxcap,
	unsigned char *edid_offset)
{
	prxcap->dc_30bit_420 = !!(edid_offset[6] & (1 << 0));
	prxcap->dc_36bit_420 = !!(edid_offset[6] & (1 << 1));
	prxcap->dc_48bit_420 = !!(edid_offset[6] & (1 << 2));
}

static bool y420vicright(enum hdmi_vic vic)
{
	bool rtn_val;

	rtn_val = false;
	if (vic == HDMI_3840x2160p60_64x27 ||
	    vic == HDMI_3840x2160p50_64x27 ||
	    vic == HDMI_4096x2160p60_256x135 ||
	    vic == HDMI_4096x2160p50_256x135 ||
	    vic == HDMI_3840x2160p60_16x9 ||
	    vic == HDMI_3840x2160p50_16x9)
		rtn_val = true;
	return rtn_val;
}

bool _is_y420_vic(enum hdmi_vic vic)
{
	return y420vicright(vic);
}

static int edid_parsingy420vdbblock(struct rx_cap *prxcap,
	unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, data_end = 0;
	unsigned int pos = 0;
	int i = 0, found = 0;

	tag = (buf[pos] >> 5) & 0x7;
	data_end = (buf[pos] & 0x1f)+1;
	pos++;
	ext_tag = buf[pos];

	if ((tag != 0x7) || (ext_tag != 0xe))
		goto INVALID_Y420VDB;

	prxcap->dc_y420 = 1;
	pos++;
	while (pos < data_end) {
		if (prxcap->VIC_count < VIC_MAX_NUM) {
			for (i = 0; i < prxcap->VIC_count; i++) {
				if (prxcap->VIC[i] == buf[pos] &&
				    y420vicright(buf[pos])) {
					prxcap->VIC[i] =
					HDMITX_VIC420_OFFSET + buf[pos];
					found = 1;
					/* Here we do not break,because
						some EDID may have the same
						repeated VICs
					*/
				}
			}
			if (0 == found) {
				prxcap->VIC[prxcap->VIC_count] =
				HDMITX_VIC420_OFFSET + buf[pos];
				prxcap->VIC_count++;
			}
		}
		pos++;
	}

	return 0;

INVALID_Y420VDB:
	printf("[%s] it's not a valid y420vdb!\n", __func__);
	return -1;
}

static int edid_parsingy420cmdbblock(struct rx_cap *prxcap,
	unsigned char *buf)
{
	unsigned char tag = 0, ext_tag = 0, length = 0, data_end = 0;
	unsigned int pos = 0, i = 0;

	tag = (buf[pos] >> 5) & 0x7;
	length = buf[pos] & 0x1f;
	data_end = length + 1;
	pos++;
	ext_tag = buf[pos];

	if ((tag != 0x7) || (ext_tag != 0xf))
		goto INVALID_Y420CMDB;

	if (length == 1) {
		prxcap->y420_all_vic = 1;
		return 0;
	}

	prxcap->bitmap_length = 0;
	prxcap->bitmap_valid = 0;
	memset(prxcap->y420cmdb_bitmap, 0x00, Y420CMDB_MAX);

	pos++;
	if (pos < data_end) {
		prxcap->bitmap_length = data_end - pos;
		prxcap->bitmap_valid = 1;
	}
	while (pos < data_end) {
		if (i < Y420CMDB_MAX)
			prxcap->y420cmdb_bitmap[i] = buf[pos];
		pos++;
		i++;
	}

	return 0;

INVALID_Y420CMDB:
	printf("[%s] it's not a valid y420cmdb!\n", __func__);
	return -1;
}

static int edid_y420cmdb_fill_all_vic(struct rx_cap *prxcap)
{
	unsigned int count = prxcap->VIC_count;
	unsigned int a, b;

	if (prxcap->y420_all_vic != 1)
		return 1;

	a = count/8;
	a = (a >= Y420CMDB_MAX)?Y420CMDB_MAX:a;
	b = count%8;

	if (a > 0)
		memset(&prxcap->y420cmdb_bitmap[0], 0xff, a);

	if ((b != 0) && (a < Y420CMDB_MAX))
		prxcap->y420cmdb_bitmap[a] = (1 << b) - 1;

	prxcap->bitmap_length = (b == 0) ? a : (a + 1);
	prxcap->bitmap_valid = (prxcap->bitmap_length != 0) ? 1 : 0;

	return 0;
}

static int edid_y420cmdb_postprocess(struct rx_cap *prxcap)
{
	unsigned int i = 0, j = 0, valid = 0;
	unsigned char *p = NULL;
	enum hdmi_vic vic;

	if (prxcap->y420_all_vic == 1)
		edid_y420cmdb_fill_all_vic(prxcap);

	if (prxcap->bitmap_valid == 0)
		goto PROCESS_END;

	prxcap->dc_y420 = 1;
	for (i = 0; i < prxcap->bitmap_length; i++) {
		p = &prxcap->y420cmdb_bitmap[i];
		for (j = 0; j < 8; j++) {
			valid = ((*p >> j) & 0x1);
			vic = prxcap->SVD_VIC[i * 8 + j];
			if (valid != 0 && y420vicright(vic)) {
				prxcap->VIC[prxcap->VIC_count] =
				HDMITX_VIC420_OFFSET + vic;
				prxcap->VIC_count++;
			}
		}
	}

PROCESS_END:
	return 0;
}

static int edid_parsingvfpdb(struct rx_cap *prxcap, unsigned char *buf)
{
	unsigned int len = buf[0] & 0x1f;
	enum hdmi_vic svr = HDMI_unknown;

	if (buf[1] != EXTENSION_VFPDB_TAG)
		return 0;
	if (len < 2)
		return 0;

	svr = buf[2];
	if (((svr >= 1) && (svr <= 127)) ||
		((svr >= 193) && (svr <= 253))) {
		prxcap->flag_vfpdb = 1;
		prxcap->preferred_mode = svr;
		pr_info("preferred mode 0 srv %d\n", prxcap->preferred_mode);
		return 1;
	}
	if ((svr >= 129) && (svr <= 144)) {
		prxcap->flag_vfpdb = 1;
		prxcap->preferred_mode = prxcap->dtd[svr - 129].vic;
		pr_info("preferred mode 0 dtd %d\n", prxcap->preferred_mode);
		return 1;
	}
	return 0;
}

static void hdmitx_edid_parse_hdmi14(struct rx_cap *prxcap,
				     unsigned char offset,
				     unsigned char *blockbuf,
				     unsigned char count)
{
	int idx = 0, tmp = 0;

	prxcap->IEEEOUI = 0x000c03;
	prxcap->ColorDeepSupport = (count > 5) ? (unsigned long)blockbuf[offset + 5] : 0;
	printf("HDMI_EDID_BLOCK_TYPE_VENDER: prxcap->ColorDeepSupport=0x%x\n",
		prxcap->ColorDeepSupport);
	if (count > 5)
		set_vsdb_dc_cap(prxcap);
	prxcap->Max_TMDS_Clock1 =
		(count > 6) ? (unsigned long)blockbuf[offset + 6] : DEFAULT_MAX_TMDS_CLK;
	if (count > 7) {
		tmp = blockbuf[offset + 7];
		idx = offset + 8;
		if (tmp & (1 << 6))
			idx += 2;
		if (tmp & (1 << 7))
			idx += 2;
		if (tmp & (1 << 5)) {
			idx += 1;
			/* valid 4k */
			if (blockbuf[idx] & 0xe0) {
				hdmitx_edid_4k2k_parse(prxcap,
					&blockbuf[idx + 1], blockbuf[idx] >> 5);
			}
		}
	}
}

static void hdmitx_parse_sink_capability(struct rx_cap *prxcap,
	unsigned char offset, unsigned char *blockbuf,
	unsigned char count)
{
	prxcap->HF_IEEEOUI = 0xd85dc4;
	prxcap->Max_TMDS_Clock2 = blockbuf[offset + 4];
	prxcap->scdc_present = !!(blockbuf[offset + 5] & (1 << 7));
	prxcap->scdc_rr_capable = !!(blockbuf[offset + 5] & (1 << 6));
	prxcap->lte_340mcsc_scramble = !!(blockbuf[offset + 5] & (1 << 3));
	set_vsdb_dc_420_cap(prxcap, &blockbuf[offset]);
}

static int hdmitx_edid_cta_block_parse(struct rx_cap *prxcap,
	unsigned char *blockbuf)
{
	unsigned char offset, end;
	unsigned char count;
	unsigned char tag;
	int i;
	unsigned char *vfpdb_offset = NULL;

	end = blockbuf[2]; /* CEA description. */
	prxcap->native_Mode = blockbuf[1] >= 2 ? blockbuf[3] : 0;
	prxcap->number_of_dtd += blockbuf[1] >= 2 ? (blockbuf[3] & 0xf) : 0;
	/* bit 5 (YCBCR 4:4:4) = 1 if sink device supports YCBCR 4:4:4
	 * in addition to RGB;
	 * bit 4 (YCBCR 4:2:2) = 1 if sink device supports YCBCR 4:2:2
	 * in addition to RGB
	 */
	prxcap->pref_colorspace = blockbuf[3] & 0x30;
	/* Initialize SVD_VIC used for SVD storage in the video data block */
	prxcap->SVD_VIC_count = 0;
	memset(prxcap->SVD_VIC, 0, sizeof(prxcap->SVD_VIC));
	/* prxcap->native_VIC = 0xff; */
	/* the descriptor offset of special EDID is 127
	 * which is the offset of checksum byte
	 */
	if (end > 127)
		return 0;
	if (blockbuf[1] <= 2) {
		/* skip below for loop */
		goto next;
	}

	for (offset = 4 ; offset < end ; ) {
		tag = blockbuf[offset] >> 5;
		count = blockbuf[offset] & 0x1f;
		switch (tag) {
		case HDMI_EDID_BLOCK_TYPE_AUDIO:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VIDEO:
			offset++;
			for (i = 0 ; i < count ; i++) {
				unsigned char VIC;
				/* 7.5.1 Video Data Block Table 58
				 * and CTA-861-G page101: only 1~64
				 * maybe Native Video Format. and
				 * need to take care hdmi2.1 VIC:
				 * 193~253
				 */
				VIC = blockbuf[offset + i];
				if (VIC >= 129 && VIC <= 192) {
					VIC &= (~0x80);
					prxcap->native_VIC = VIC;
				}
				/* The SVD in the video data block is stored in SVD_VIC
				 * and mapped with 420 CMDB
				 */
				prxcap->SVD_VIC[prxcap->SVD_VIC_count] = VIC;
				prxcap->SVD_VIC_count++;
				store_cea_idx(prxcap, VIC);
			}
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VENDER:
			offset++;
			if (blockbuf[offset] == 0x03 &&
				blockbuf[offset + 1] == 0x0c &&
				blockbuf[offset + 2] == 0x00)
				hdmitx_edid_parse_hdmi14(prxcap, offset, blockbuf, count);
			else if (blockbuf[offset] == 0xd8 &&
				   blockbuf[offset + 1] == 0x5d &&
				   blockbuf[offset + 2] == 0xc4)
				hdmitx_parse_sink_capability(prxcap, offset, blockbuf, count);
			offset += count; /* ignore the remains. */
			break;

		case HDMI_EDID_BLOCK_TYPE_SPEAKER:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_VESA:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_EXTENDED_TAG:
			{
				unsigned char ext_tag = 0;

				ext_tag = blockbuf[offset + 1];
				switch (ext_tag) {
				case EXTENSION_VENDOR_SPECIFIC:
					edid_parsingvendspec(prxcap,
						&blockbuf[offset]);
					break;
				case EXTENSION_COLORMETRY_TAG:
					prxcap->colorimetry_data =
						blockbuf[offset + 2];
					break;
				case EXTENSION_DRM_STATIC_TAG:
					edid_parsingdrmstaticblock(prxcap,
						&blockbuf[offset]);
					break;
				case EXTENSION_VFPDB_TAG:
/* Just record VFPDB offset address, call edid_parsingvfpdb() after DTD
 * parsing, in case that
 * SVR >=129 and SVR <=144, Interpret as the Kth DTD in the EDID,
 * where K = SVR - 128 (for K=1 to 16)
 */
					vfpdb_offset = &blockbuf[offset];
					break;
				case EXTENSION_Y420_VDB_TAG:
					edid_parsingy420vdbblock(prxcap,
						&blockbuf[offset]);
					break;
				case EXTENSION_Y420_CMDB_TAG:
					edid_parsingy420cmdbblock(prxcap,
						&blockbuf[offset]);
					break;
				case EXTENSION_SCDB_EXT_TAG:
					hdmitx_parse_sink_capability(prxcap, offset + 1,
						blockbuf, count);
					break;
				default:
					break;
				}
			}
			offset += count+1;
			break;

		case HDMI_EDID_BLOCK_TYPE_RESERVED:
			offset++;
			offset += count;
			break;

		case HDMI_EDID_BLOCK_TYPE_RESERVED2:
			offset++;
			offset += count;
			break;

		default:
			break;
		}
	}
next:
	edid_y420cmdb_postprocess(prxcap);
	/* dtds in extended blocks */
	i = 0;
	offset = blockbuf[2] + i * 18;
	for ( ; (offset + 18) < 0x7f; i++) {
		edid_dtd_parsing(prxcap, &blockbuf[offset]);
		offset += 18;
	}
	if (vfpdb_offset)
		edid_parsingvfpdb(prxcap, vfpdb_offset);

	return 0;
}

void edid_monitorcapable861(struct rx_cap *prxcap,
	unsigned char edid_flag)
{
	if (edid_flag & 0x20)
		prxcap->support_ycbcr444_flag = 1;
	if (edid_flag & 0x10)
		prxcap->support_ycbcr422_flag = 1;
	printf("%s: ycbcr444=%d, ycbcr422=%d\n", __func__,
		prxcap->support_ycbcr444_flag, prxcap->support_ycbcr422_flag);
}

static bool is_4k60_supported(struct rx_cap *prxcap)
{
	int i = 0;

	if (!prxcap)
		return false;

	for (i = 0; (i < prxcap->VIC_count) && (i < VIC_MAX_NUM); i++) {
		if (((prxcap->VIC[i] & 0xff) == HDMI_3840x2160p50_16x9) ||
		    ((prxcap->VIC[i] & 0xff) == HDMI_3840x2160p60_16x9)) {
			return true;
		}
	}
	return false;
}

bool is_support_y422(struct rx_cap *prxcap)
{
	if (!prxcap)
		return false;

	if (prxcap->native_Mode & (1 << 4))
		return true;

	return false;
}

static void check_dv_truly_support(struct rx_cap *prxcap, struct dv_info *dv)
{
	unsigned int max_tmds_clk = 0;

	if (!prxcap || !dv)
		return;
	if ((dv->ieeeoui == DV_IEEE_OUI) && (dv->ver <= 2)) {
		/* check max tmds rate to determine if 4k60 DV can truly be
		 * supported.
		 */
		if (prxcap->Max_TMDS_Clock2) {
			max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
		} else {
			/* Default min is 74.25 / 5 */
			if (prxcap->Max_TMDS_Clock1 < 0xf)
				prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK;
			max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
		}
		if (dv->ver == 0)
			dv->sup_2160p60hz = dv->sup_2160p60hz &&
						(max_tmds_clk >= 594);

		if ((dv->ver == 1) && (dv->length == 0xB)) {
			if (dv->low_latency == 0x00) {
				/*standard mode */
				dv->sup_2160p60hz = dv->sup_2160p60hz &&
							(max_tmds_clk >= 594);
			} else if (dv->low_latency == 0x01) {
				/* both standard and LL are supported. 4k60 LL
				 * DV support should/can be determined using
				 * video formats supported inthe E-EDID as flag
				 * sup_2160p60hz might not be set.
				 */
				if ((dv->sup_2160p60hz ||
				     is_4k60_supported(prxcap)) &&
				     (max_tmds_clk >= 594))
					dv->sup_2160p60hz = 1;
				else
					dv->sup_2160p60hz = 0;
			}
		}

		if ((dv->ver == 1) && (dv->length == 0xE))
			dv->sup_2160p60hz = dv->sup_2160p60hz &&
						(max_tmds_clk >= 594);

		if (dv->ver == 2) {
			/* 4k60 DV support should be determined using video
			 * formats supported in the EEDID as flag sup_2160p60hz
			 * is not applicable for VSVDB V2.
			 */
			if (is_4k60_supported(prxcap) && (max_tmds_clk >= 594))
				dv->sup_2160p60hz = 1;
			else
				dv->sup_2160p60hz = 0;
		}
	}
}

/*
 * Parsing RAW EDID data from edid to prxcap
 */
unsigned int hdmi_edid_parsing(unsigned char *EDID_buf, struct rx_cap *prxcap)
{
	int i;
	int idx[4];
	struct dv_info *dv = &prxcap->dv_info;
	unsigned char cta_block_count;
	struct hdmitx_dev *hdev = hdmitx_get_hdev();

	/* Clear all parsing data */
	memset(prxcap, 0, sizeof(struct rx_cap));
	prxcap->IEEEOUI = 0x00; /* Default is DVI device */

	if (check_dvi_hdmi_edid_valid(EDID_buf) == 0) {
		edid_set_fallback_mode(prxcap);
		printf("set fallback mode\n");
		return 0;
	}
	if (_check_base_structure(EDID_buf))
		_edid_parse_base_structure(prxcap, EDID_buf);

	cta_block_count = EDID_buf[0x7E];
	/* HF-EEODB */
	if (cta_block_count && EDID_buf[128 + 4] == 0xe2 &&
		EDID_buf[128 + 5] == 0x78)
		cta_block_count = EDID_buf[128 + 6];
	/* limit cta_block_count to EDID_MAX_BLOCK - 1 */
	if (cta_block_count > EDID_MAX_BLOCK - 1)
		cta_block_count = EDID_MAX_BLOCK - 1;
	for (i = 1; i <= cta_block_count; i++) {
		if (EDID_buf[i * 0x80] == 0x02 || hdev->edid_check & 0x01)
			hdmitx_edid_cta_block_parse(prxcap, &EDID_buf[i * 0x80]);
	}
/*
 * Because DTDs are not able to represent some Video Formats, which can be
 * represented as SVDs and might be preferred by Sinks, the first DTD in the
 * base EDID data structure and the first SVD in the first CEA Extension can
 * differ. When the first DTD and SVD do not match and the total number of
 * DTDs defining Native Video Formats in the whole EDID is zero, the first
 * SVD shall take precedence.
 */
	if (!prxcap->flag_vfpdb && prxcap->preferred_mode != prxcap->VIC[0] &&
		prxcap->number_of_dtd == 0) {
		pr_info("hdmitx: edid: change preferred_mode from %d to %d\n",
			prxcap->preferred_mode,	prxcap->VIC[0]);
		prxcap->preferred_mode = prxcap->VIC[0];
	}
	check_dv_truly_support(prxcap, dv);
	idx[0] = EDID_DETAILED_TIMING_DES_BLOCK0_POS;
	idx[1] = EDID_DETAILED_TIMING_DES_BLOCK1_POS;
	idx[2] = EDID_DETAILED_TIMING_DES_BLOCK2_POS;
	idx[3] = EDID_DETAILED_TIMING_DES_BLOCK3_POS;
	for (i = 0; i < 4; i++) {
		if ((EDID_buf[idx[i]]) && (EDID_buf[idx[i] + 1]))
			edid_dtd_parsing(prxcap, &EDID_buf[idx[i]]);
	}

	if (hdmitx_edid_search_IEEEOUI(&EDID_buf[128])) {
		prxcap->IEEEOUI = HDMI_IEEEOUI;
		printf("find IEEEOUT\n");
	} else {
		prxcap->IEEEOUI = 0x0;
		printf("not find IEEEOUT\n");
	}

	/* strictly DVI device judgement */
	/* valid EDID & no audio tag & no IEEEOUI */
	if (edid_check_valid(&EDID_buf[0]) &&
		!hdmitx_edid_search_IEEEOUI(&EDID_buf[128])) {
		prxcap->IEEEOUI = 0x0;
		printf("sink is DVI device\n");
	} else {
		prxcap->IEEEOUI = HDMI_IEEEOUI;
	}
	if (edid_zero_data(EDID_buf))
		prxcap->IEEEOUI = HDMI_IEEEOUI;

	if (!hdmitx_edid_check_valid_blocks(&EDID_buf[0])) {
		prxcap->IEEEOUI = HDMI_IEEEOUI;
		printf("Invalid edid, consider RX as HDMI device\n");
	}

	/* if edid block0 are all zeroes, or no VIC, set default vic */
	if (edid_zero_data(EDID_buf) || prxcap->VIC_count == 0)
		hdmitx_edid_set_default_vic(prxcap);
	return 1;
}

int hdmitx_edid_VIC_support(enum hdmi_vic vic)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dispmode_vic_tab); i++) {
		if (vic == dispmode_vic_tab[i].VIC)
			return 1;
	}

	return 0;
}

enum hdmi_vic hdmitx_edid_vic_tab_map_vic(const char *disp_mode)
{
	enum hdmi_vic vic = HDMI_unknown;
	int i;

	for (i = 0; i < ARRAY_SIZE(dispmode_vic_tab); i++) {
		if (strncmp(disp_mode, dispmode_vic_tab[i].disp_mode,
			    strlen(dispmode_vic_tab[i].disp_mode)) == 0) {
			vic = dispmode_vic_tab[i].VIC;
			break;
		}
	}

	if (vic == HDMI_unknown)
		printf("not find mapped vic\n");

	return vic;
}

const char *hdmitx_edid_vic_tab_map_string(enum hdmi_vic vic)
{
	int i;
	const char *disp_str = NULL;

	for (i = 0; i < ARRAY_SIZE(dispmode_vic_tab); i++) {
		if (vic == dispmode_vic_tab[i].VIC) {
			disp_str = dispmode_vic_tab[i].disp_mode;
			break;
		}
	}

	return disp_str;
}

const char *hdmitx_edid_vic_to_string(enum hdmi_vic vic)
{
	int i;
	const char *disp_str = NULL;

	for (i = 0; i < ARRAY_SIZE(dispmode_vic_tab); i++) {
		if (vic == dispmode_vic_tab[i].VIC) {
			disp_str = dispmode_vic_tab[i].disp_mode;
			break;
		}
	}

	return disp_str;
}

static bool is_rx_support_y420(struct hdmitx_dev *hdev, enum hdmi_vic vic)
{
	unsigned int i = 0;
	struct rx_cap *prxcap = &hdev->RXCap;
	bool ret = false;

	if (vic < HDMITX_VIC420_OFFSET)
		vic += HDMITX_VIC420_OFFSET;
	for (i = 0; i < VIC_MAX_NUM; i++) {
		if (prxcap->VIC[i]) {
			if (prxcap->VIC[i] == vic) {
				ret = true;
				break;
			}
		} else {
			ret = false;
			break;
		}
	}
	return ret;
}

static int is_4k_fmt(char *mode)
{
	int i;
	static char const *hdmi4k[] = {
		"2160p",
		"smpte",
		NULL
	};

	for (i = 0; hdmi4k[i]; i++) {
		if (strstr(mode, hdmi4k[i]))
			return 1;
	}
	return 0;
}

static bool is_over_60hz(struct hdmi_format_para *para)
{
	if (!para)
		return 1;

	if (para->timing.v_freq > 60000)
		return 1;

	return 0;
}

/* check the resolution is over 1920x1080 or not */
static bool is_over_1080p(struct hdmi_format_para *para)
{
	if (!para)
		return 1;

	if (para->timing.h_active > 1920 || para->timing.v_active > 1080)
		return 1;

	return 0;
}

/* test current vic is over 150MHz or not */
static bool is_over_pixel_150mhz(struct hdmi_format_para *para)
{
	if (!para)
		return 1;

	if (para->timing.pixel_freq > 150000)
		return 1;

	return 0;
}

bool is_vic_over_limited_1080p(enum hdmi_vic vic)
{
	struct hdmi_format_para *para = hdmi_get_fmt_paras(vic);

	if (vic == HDMI_unknown || vic >= HDMITX_VESA_OFFSET)
		return 1;

	if (is_over_1080p(para) || is_over_60hz(para) ||
		is_over_pixel_150mhz(para)) {
		pr_err("over limited vic: %d\n", vic);
		return 1;
	}
	return 0;
}

static bool hdmitx_check_4x3_16x9_mode(struct hdmitx_dev *hdev,
		enum hdmi_vic vic)
{
	bool flag = 0;
	int j;
	struct rx_cap *prxcap = NULL;

	prxcap = &hdev->RXCap;
	if (vic == HDMI_720x480p60_4x3 ||
		vic == HDMI_720x480i60_4x3 ||
		vic == HDMI_720x576p50_4x3 ||
		vic == HDMI_720x576i50_4x3) {
		for (j = 0; (j < prxcap->VIC_count) && (j < VIC_MAX_NUM); j++) {
			if ((vic + 1) == (prxcap->VIC[j] & 0xff)) {
				flag = 1;
				break;
			}
		}
	} else if (vic == HDMI_720x480p60_16x9 ||
			vic == HDMI_720x480i60_16x9 ||
			vic == HDMI_720x576p50_16x9 ||
			vic == HDMI_720x576i50_16x9) {
		for (j = 0; (j < prxcap->VIC_count) && (j < VIC_MAX_NUM); j++) {
			if ((vic - 1) == (prxcap->VIC[j] & 0xff)) {
				flag = 1;
				break;
			}
		}
	}
	return flag;
}

/* For some TV's EDID, there maybe exist some information ambiguous.
 * Such as EDID declare support 2160p60hz(Y444 8bit), but no valid
 * Max_TMDS_Clock2 to indicate that it can support 5.94G signal.
 */
bool hdmitx_edid_check_valid_mode(struct hdmitx_dev *hdev,
	struct hdmi_format_para *para)
{
	bool valid = 0;
	struct rx_cap *prxcap = NULL;
	struct dv_info *dv = &hdev->RXCap.dv_info;
	unsigned int rx_max_tmds_clk = 0;
	unsigned int calc_tmds_clk = 0;
	int i = 0;
	int svd_flag = 0;
	/* Default max color depth is 24 bit */
	enum hdmi_color_depth rx_y444_max_dc = HDMI_COLOR_DEPTH_24B;
	enum hdmi_color_depth rx_rgb_max_dc = HDMI_COLOR_DEPTH_24B;

	if (!hdev || !para)
		return 0;

	if (para->sname && strcmp(para->sname, "invalid") == 0)
		return 0;
	/* if current limits to 1080p, here will check the freshrate and
	 * 4k resolution
	 */
	if (is_hdmitx_limited_1080p()) {
		if (is_vic_over_limited_1080p(para->vic)) {
			printf("over limited vic%d in %s\n", para->vic, __func__);
			return 0;
		}
	}
	/* add efuse ctrl */
	if (hdev->efuse_dis_output_4k)
		if (para->timing.v_active >= 2160)
			return false;
	if (hdev->efuse_dis_hdmi_4k60)
		if (para->timing.v_active >= 2160 &&
			para->timing.v_freq >= 5000)
			return false;

	if (!is_support_4k() && para->sname && is_4k_fmt(para->sname))
		return false;
	/* exclude such as: 2160p60hz YCbCr444 10bit */
	switch (para->vic) {
	case HDMI_3840x2160p50_16x9:
	case HDMI_3840x2160p60_16x9:
	case HDMI_4096x2160p50_256x135:
	case HDMI_4096x2160p60_256x135:
	case HDMI_3840x2160p50_64x27:
	case HDMI_3840x2160p60_64x27:
		if ((para->cs == HDMI_COLOR_FORMAT_RGB) ||
		    (para->cs == HDMI_COLOR_FORMAT_444))
			if (para->cd != HDMI_COLOR_DEPTH_24B)
				return 0;
		break;
	case HDMI_720x480i60_4x3:
	case HDMI_720x480i60_16x9:
	case HDMI_720x576i50_4x3:
	case HDMI_720x576i50_16x9:
		if (para->cs == HDMI_COLOR_FORMAT_422)
			return 0;
		break;
	default:
		break;
	}

	prxcap = &hdev->RXCap;

	/* DVI case, only rgb,8bit */
	if (prxcap->IEEEOUI != HDMI_IEEEOUI) {
		if (para->cd != HDMI_COLOR_DEPTH_24B ||
			para->cs != HDMI_COLOR_FORMAT_RGB)
			return 0;
	}

	/* target mode is not contained at RX SVD */
	for (i = 0; (i < prxcap->VIC_count) && (i < VIC_MAX_NUM); i++) {
		if ((para->vic & 0xff) == (prxcap->VIC[i] & 0xff)) {
			svd_flag = 1;
			break;
		} else if (hdmitx_check_4x3_16x9_mode(hdev, para->vic & 0xff)) {
			svd_flag = 1;
			break;
		}
	}
	if (svd_flag == 0)
		return 0;

	/* Get RX Max_TMDS_Clock */
	if (prxcap->Max_TMDS_Clock2) {
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock2 * 5;
	} else {
		/* Default min is 74.25 / 5 */
		if (prxcap->Max_TMDS_Clock1 < 0xf)
			prxcap->Max_TMDS_Clock1 = DEFAULT_MAX_TMDS_CLK;
		rx_max_tmds_clk = prxcap->Max_TMDS_Clock1 * 5;
	}
	/* if current status already limited to 1080p, so here also needs to
	 * limit the rx_max_tmds_clk as 150 * 1.5 = 225 to make the valid mode
	 * checking works
	 */
	if (is_hdmitx_limited_1080p()) {
		if (rx_max_tmds_clk > 225)
			rx_max_tmds_clk = 225;
	}

	calc_tmds_clk = para->tmds_clk;
	if (para->cs == HDMI_COLOR_FORMAT_420)
		calc_tmds_clk = calc_tmds_clk / 2;
	if (para->cs != HDMI_COLOR_FORMAT_422) {
		switch (para->cd) {
		case HDMI_COLOR_DEPTH_30B:
			calc_tmds_clk = calc_tmds_clk * 5 / 4;
			break;
		case HDMI_COLOR_DEPTH_36B:
			calc_tmds_clk = calc_tmds_clk * 3 / 2;
			break;
		case HDMI_COLOR_DEPTH_48B:
			calc_tmds_clk = calc_tmds_clk * 2;
			break;
		case HDMI_COLOR_DEPTH_24B:
		default:
			calc_tmds_clk = calc_tmds_clk * 1;
			break;
		}
	}

	calc_tmds_clk = calc_tmds_clk / 1000;
	/* printf("RX tmds clk: %d   Calc clk: %d\n", */
	/* rx_max_tmds_clk, calc_tmds_clk); */
	if (calc_tmds_clk < rx_max_tmds_clk)
		valid = 1;
	else
		return 0;

	if (para->cs == HDMI_COLOR_FORMAT_444) {
		/* Rx may not support Y444 */
		if (!(prxcap->native_Mode & (1 << 5)))
			return 0;
		if ((prxcap->dc_y444 && prxcap->dc_30bit) ||
		    (dv->sup_10b_12b_444 == 0x1))
			rx_y444_max_dc = HDMI_COLOR_DEPTH_30B;
		if ((prxcap->dc_y444 && prxcap->dc_36bit) ||
		    (dv->sup_10b_12b_444 == 0x2))
			rx_y444_max_dc = HDMI_COLOR_DEPTH_36B;
		if (para->cd <= rx_y444_max_dc)
			valid = 1;
		else
			valid = 0;
		return valid;
	}
	if (para->cs == HDMI_COLOR_FORMAT_422) {
		/* Rx may not support Y422 */
		if (!(prxcap->native_Mode & (1 << 4)))
			return 0;
		return 1;
	}
	if (para->cs == HDMI_COLOR_FORMAT_RGB) {
		/* Always assume RX supports RGB444 */
		if ((prxcap->dc_30bit) || (dv->sup_10b_12b_444 == 0x1))
			rx_rgb_max_dc = HDMI_COLOR_DEPTH_30B;
		if ((prxcap->dc_36bit) || (dv->sup_10b_12b_444 == 0x2))
			rx_rgb_max_dc = HDMI_COLOR_DEPTH_36B;
		if (para->cd <= rx_rgb_max_dc)
			valid = 1;
		else
			valid = 0;
		return valid;
	}
	if (para->cs == HDMI_COLOR_FORMAT_420) {
		if (!is_rx_support_y420(hdev, para->vic))
			return 0;
		if (!prxcap->dc_30bit_420)
			if (para->cd == HDMI_COLOR_DEPTH_30B)
				return 0;
		if (!prxcap->dc_36bit_420)
			if (para->cd == HDMI_COLOR_DEPTH_36B)
				return 0;
		valid = 1;
	}

	return valid;
}

bool is_supported_mode_attr(hdmi_data_t *hdmi_data, char *mode_attr)
{
	struct hdmi_format_para *para = NULL;
	struct hdmitx_dev *hdev = NULL;

	if (!hdmi_data || !mode_attr)
		return false;
	hdev = container_of(hdmi_data->prxcap,
			struct hdmitx_dev, RXCap);

	if (mode_attr[0]) {
		if (!pre_process_str(mode_attr))
			return false;
		para = hdmi_tst_fmt_name(mode_attr, mode_attr);
	}
	/* if (para) { */
		/* printf("sname = %s\n", para->sname); */
		/* printf("char_clk = %d\n", para->tmds_clk); */
		/* printf("cd = %d\n", para->cd); */
		/* printf("cs = %d\n", para->cs); */
	/* } */

	return hdmitx_edid_check_valid_mode(hdev, para);
}

bool hdmitx_chk_mode_attr_sup(hdmi_data_t *hdmi_data, char *mode, char *attr)
{
	struct hdmi_format_para *para = NULL;
	struct hdmitx_dev *hdev = NULL;

	if (!hdmi_data || !mode || !attr)
		return false;
	hdev = container_of(hdmi_data->prxcap,
			struct hdmitx_dev, RXCap);

	if (attr[0]) {
		if (!pre_process_str(attr))
			return false;
		para = hdmi_tst_fmt_name(mode, attr);
	}
	/* if (para) { */
		/* printf("sname = %s\n", para->sname); */
		/* printf("char_clk = %d\n", para->tmds_clk); */
		/* printf("cd = %d\n", para->cd); */
		/* printf("cs = %d\n", para->cs); */
	/* } */

	return hdmitx_edid_check_valid_mode(hdev, para);
}

/* force_flag: 0 means check with RX's edid */
/* 1 means no check with RX's edid */
enum hdmi_vic hdmitx_edid_get_VIC(struct hdmitx_dev *hdev,
	const char *disp_mode, char force_flag)
{
	struct rx_cap *prxcap = &hdev->RXCap;
	int j;
	enum hdmi_vic vic = hdmitx_edid_vic_tab_map_vic(disp_mode);
	#if 0
	struct hdmi_format_para *para = NULL;
	enum hdmi_vic *vesa_t = &hdev->RXCap.vesa_timing[0];
	enum hdmi_vic vesa_vic;

	if (vic >= HDMITX_VESA_OFFSET)
		vesa_vic = vic;
	else
		vesa_vic = HDMI_unknown;
	#endif
	if (vic != HDMI_unknown) {
		if (force_flag == 0) {
			for (j = 0 ; j < prxcap->VIC_count ; j++) {
				if (prxcap->VIC[j] == vic)
					break;
			}
			if (j >= prxcap->VIC_count)
				vic = HDMI_unknown;
		}
	}
	#if 0
	if (vic == HDMI_unknown &&
	    vesa_vic != HDMI_unknown) {
		for (j = 0; vesa_t[j] && j < VESA_MAX_TIMING; j++) {
			para = hdmi_get_fmt_paras(vesa_t[j]);
			if (para) {
				if ((para->vic >= HDMITX_VESA_OFFSET) &&
				    (vesa_vic == para->vic)) {
					vic = para->vic;
					break;
				}
			}
		}
	}
	#endif
	return vic;
}

static bool hdmitx_edid_notify_ng(unsigned char *buf)
{
	if (!buf)
		return true;
	return check_dvi_hdmi_edid_valid(buf) == 0;
	/* notify EDID NG to systemcontrol */
	/* if (hdmitx_check_edid_all_zeros(buf)) { */
		/* printf("ERR: edid all zero\n"); */
		/* return true; */
	/* } else if ((buf[0x7e] > 3) && */
		/* hdmitx_edid_header_invalid(buf)) { */
		/* printf("ERR: edid header invalid\n"); */
		/* return true; */
	/* } */
	/* may extend NG case here */

	/* return false; */
}

bool edid_parsing_ok(struct hdmitx_dev *hdev)
{
	if (!hdev)
		return false;

	if (hdmitx_edid_notify_ng(hdev->rawedid))
		return false;
	return true;
}


// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/hdmitx21/hdmitx_module.h>
#include "hdmi_param.h"

const struct hdmi_timing *hdmitx21_get_timing_para0(void)
{
	return &hdmi_timing_all[0];
}

int hdmitx21_timing_size(void)
{
	return ARRAY_SIZE(hdmi_timing_all);
}

static struct parse_cd parse_cd_[] = {
	{COLORDEPTH_24B, "8bit",},
	{COLORDEPTH_30B, "10bit"},
	{COLORDEPTH_36B, "12bit"},
	{COLORDEPTH_48B, "16bit"},
};

static struct parse_cs parse_cs_[] = {
	{HDMI_COLORSPACE_RGB, "rgb",},
	{HDMI_COLORSPACE_YUV422, "422",},
	{HDMI_COLORSPACE_YUV444, "444",},
	{HDMI_COLORSPACE_YUV420, "420",},
};

static struct parse_cr parse_cr_[] = {
	{COLORRANGE_LIM, "limit",},
	{COLORRANGE_FUL, "full",},
};

/* parse the name string to cs/cd/cr */
static void _parse_hdmi_attr(char const *name,
	enum hdmi_colorspace *cs,
	enum hdmi_color_depth *cd,
	enum hdmi_color_range *cr)
{
	int i;

	if (!cs || !cd || !cr)
		return;
	if (!name) {
		/* assign defalut value*/
		*cs = HDMI_COLORSPACE_RGB;
		*cd = COLORDEPTH_24B;
		*cr = COLORRANGE_FUL;
		return;
	}

	/* parse color depth */
	for (i = 0; i < sizeof(parse_cd_) / sizeof(struct parse_cd); i++) {
		if (strstr(name, parse_cd_[i].name)) {
			*cd = parse_cd_[i].cd;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cd_) / sizeof(struct parse_cd))
		*cd = COLORDEPTH_24B;

	/* parse color space */
	for (i = 0; i < sizeof(parse_cs_) / sizeof(struct parse_cs); i++) {
		if (strstr(name, parse_cs_[i].name)) {
			*cs = parse_cs_[i].cs;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cs_) / sizeof(struct parse_cs))
		*cs = HDMI_COLORSPACE_RGB;

	/* parse color range */
	for (i = 0; i < sizeof(parse_cr_) / sizeof(struct parse_cr); i++) {
		if (strstr(name, parse_cr_[i].name)) {
			*cr = parse_cr_[i].cr;
			break;
		}
	}
	/* set default value */
	if (i == sizeof(parse_cr_) / sizeof(struct parse_cr))
		*cr = COLORRANGE_FUL;
}

static u32 _calc_tmds_clk(u32 pixel_freq, enum hdmi_colorspace cs,
	enum hdmi_color_depth cd)
{
	u32 tmds_clk = pixel_freq;

	if (cs == HDMI_COLORSPACE_YUV420)
		tmds_clk = tmds_clk / 2;
	if (cs != HDMI_COLORSPACE_YUV422) {
		switch (cd) {
		case COLORDEPTH_48B:
			tmds_clk *= 2;
			break;
		case COLORDEPTH_36B:
			tmds_clk = tmds_clk * 3 / 2;
			break;
		case COLORDEPTH_30B:
			tmds_clk = tmds_clk * 5 / 4;
			break;
		case COLORDEPTH_24B:
		default:
			break;
		}
	}

	return tmds_clk;
}

static bool _tst_fmt_name(struct hdmi_format_para *para,
	char const *name, char const *attr)
{
	int i;
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();
	enum hdmi_vic prefer_vic = HDMI_0_UNKNOWN;

	if (!para || !name || !attr)
		return 0;
	/* check sname first */
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (timing->sname && (strncmp(name, timing->sname, strlen(timing->sname)) == 0)) {
			para->timing = *timing;
			goto next;
		}
		timing++;
	}

	/* check name */
	timing = hdmitx21_get_timing_para0();
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (strncmp(name, timing->name, strlen(timing->name)) == 0) {
			para->timing = *timing;
			break;
		}
		timing++;
	}
	if (i == hdmitx21_timing_size())
		return 0;
next:
	prefer_vic = hdmitx21_get_prefer_vic(hdev, timing->vic);
	timing = hdmitx21_gettiming_from_vic(prefer_vic);
	if (!timing)
		return 0;
	para->timing = *timing;
	_parse_hdmi_attr(attr, &para->cs, &para->cd, &para->cr);

	para->tmds_clk = _calc_tmds_clk(timing->pixel_freq, para->cs, para->cd);

	return 1;
}

const struct hdmi_timing *hdmitx21_match_dtd_timing(struct dtd *t)
{
	int i;
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();

	if (!t)
		return NULL;

	/* interlace mode, all vertical timing parameters
	 * are halved, while vactive/vtotal is doubled
	 * in timing table. need double vactive before compare
	 */
	if (t->flags >> 7 == 0x1)
		t->v_active = t->v_active * 2;
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if ((abs(timing->pixel_freq / 10 - t->pixel_clock) <=
			(t->pixel_clock + 1000) / 1000) &&
		    t->h_active == timing->h_active &&
		    t->h_blank == timing->h_blank &&
		    t->v_active == timing->v_active &&
		    t->v_blank == timing->v_blank &&
		    t->h_sync_offset == timing->h_front &&
		    t->h_sync == timing->h_sync &&
		    t->v_sync_offset == timing->v_front &&
		    t->v_sync == timing->v_sync)
			return timing;
		timing++;
	}
	return NULL;
}

struct hdmi_format_para *hdmitx21_tst_fmt_name(const char *name,
	const char *attr)
{
	static struct hdmi_format_para para;

	if (!name)
		return NULL;

	if (!attr)
		attr = "rgb,8bit";

	memset(&para, 0, sizeof(para));
	if (_tst_fmt_name(&para, name, attr))
		return &para;
	else
		return NULL;
}

const struct hdmi_timing *hdmitx21_gettiming_from_vic(enum hdmi_vic vic)
{
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();
	int i;

	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (timing->vic == vic)
			break;
		timing++;
	}
	if (i == hdmitx21_timing_size())
		return NULL;

	return timing;
}

const struct hdmi_timing *hdmitx21_gettiming_from_name(const char *name)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	const struct hdmi_timing *timing = hdmitx21_get_timing_para0();
	int i;
	enum hdmi_vic prefer_vic = HDMI_UNKNOWN;

	/* check sname first */
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (timing->sname && strstr(timing->sname, name))
			goto next;
		timing++;
	}

	timing = hdmitx21_get_timing_para0();
	for (i = 0; i < hdmitx21_timing_size(); i++) {
		if (strncmp(timing->name, name, strlen(timing->name)) == 0)
			break;
		timing++;
	}
	if (i == hdmitx21_timing_size())
		return NULL;
next:
	prefer_vic = hdmitx21_get_prefer_vic(hdev, timing->vic);
	timing = hdmitx21_gettiming_from_vic(prefer_vic);
	return timing;
}

/*
 * Parameter 'name' can should be full name as 1920x1080p60hz,
 * 3840x2160p60hz, etc
 * attr strings likes as '444,8bit'
 */
struct hdmi_format_para *hdmitx21_get_fmtpara(const char *mode,
	const char *attr)
{
	struct hdmitx_dev *hdev = get_hdmitx21_device();
	const struct hdmi_timing *timing;
	struct hdmi_format_para *para = hdev->para;

	if (!mode || !attr)
		return NULL;

	timing = hdmitx21_gettiming_from_name(mode);
	if (!timing)
		return NULL;

	para->timing = *timing;
	para->sname = timing->sname;
	_parse_hdmi_attr(attr, &para->cs, &para->cd, &para->cr);

	para->tmds_clk = _calc_tmds_clk(para->timing.pixel_freq,
		para->cs, para->cd);

	return hdev->para;
}

/* For check all format parameters only */
void check21_detail_fmt(void)
{
}

/* Recommended N and Expected CTS for 32kHz */
static const struct hdmi_audio_fs_ncts aud_32k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 4576,
		.cts = 28125,
		.n_36bit = 9152,
		.cts_36bit = 84375,
		.n_48bit = 4576,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 4096,
		.cts = 25200,
		.n_36bit = 4096,
		.cts_36bit = 37800,
		.n_48bit = 4096,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 4096,
		.cts = 27000,
		.n_36bit = 4096,
		.cts_36bit = 40500,
		.n_48bit = 4096,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 4096,
		.cts = 27027,
		.n_36bit = 8192,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 4096,
		.cts = 54000,
		.n_36bit = 4096,
		.cts_36bit = 81000,
		.n_48bit = 4096,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 4096,
		.cts = 54054,
		.n_36bit = 4096,
		.cts_36bit = 81081,
		.n_48bit = 4096,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 210937,
		.n_36bit = 11648,
		.cts_36bit = 316406,
		.n_48bit = 11648,
		.cts_48bit = 421875,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 4096,
		.cts = 74250,
		.n_36bit = 4096,
		.cts_36bit = 111375,
		.n_48bit = 4096,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 11648,
		.cts = 421875,
		.n_36bit = 11648,
		.cts_36bit = 632812,
		.n_48bit = 11648,
		.cts_48bit = 843750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 4096,
		.cts = 148500,
		.n_36bit = 4096,
		.cts_36bit = 222750,
		.n_48bit = 4096,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 421875,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 3072,
		.cts = 222750,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 843750,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 3072,
		.cts = 445500,
	},
	.def_n = 4096,
};

/* Recommended N and Expected CTS for 44.1kHz and Multiples */
static const struct hdmi_audio_fs_ncts aud_44k1_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 7007,
		.cts = 31250,
		.n_36bit = 7007,
		.cts_36bit = 46875,
		.n_48bit = 7007,
		.cts_48bit = 62500,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6272,
		.cts = 28000,
		.n_36bit = 6272,
		.cts_36bit = 42000,
		.n_48bit = 6272,
		.cts_48bit = 56000,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6272,
		.cts = 30000,
		.n_36bit = 6272,
		.cts_36bit = 45000,
		.n_48bit = 6272,
		.cts_48bit = 60000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6272,
		.cts = 30030,
		.n_36bit = 6272,
		.cts_36bit = 45045,
		.n_48bit = 6272,
		.cts_48bit = 60060,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6272,
		.cts = 60000,
		.n_36bit = 6272,
		.cts_36bit = 90000,
		.n_48bit = 6272,
		.cts_48bit = 120000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6272,
		.cts = 60060,
		.n_36bit = 6272,
		.cts_36bit = 90090,
		.n_48bit = 6272,
		.cts_48bit = 120120,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 17836,
		.cts = 234375,
		.n_36bit = 17836,
		.cts_36bit = 351562,
		.n_48bit = 17836,
		.cts_48bit = 468750,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6272,
		.cts = 82500,
		.n_36bit = 6272,
		.cts_36bit = 123750,
		.n_48bit = 6272,
		.cts_48bit = 165000,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 8918,
		.cts = 234375,
		.n_36bit = 17836,
		.cts_36bit = 703125,
		.n_48bit = 8918,
		.cts_48bit = 468750,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6272,
		.cts = 165000,
		.n_36bit = 6272,
		.cts_36bit = 247500,
		.n_48bit = 6272,
		.cts_48bit = 330000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 4459,
		.cts = 234375,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 4707,
		.cts = 247500,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 8918,
		.cts = 937500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 9408,
		.cts = 990000,
	},
	.def_n = 6272,
};

/* Recommended N and Expected CTS for 48kHz and Multiples */
static const struct hdmi_audio_fs_ncts aud_48k_para = {
	.array[0] = {
		.tmds_clk = 25175,
		.n = 6864,
		.cts = 28125,
		.n_36bit = 9152,
		.cts_36bit = 58250,
		.n_48bit = 6864,
		.cts_48bit = 56250,
	},
	.array[1] = {
		.tmds_clk = 25200,
		.n = 6144,
		.cts = 25200,
		.n_36bit = 6144,
		.cts_36bit = 37800,
		.n_48bit = 6144,
		.cts_48bit = 50400,
	},
	.array[2] = {
		.tmds_clk = 27000,
		.n = 6144,
		.cts = 27000,
		.n_36bit = 6144,
		.cts_36bit = 40500,
		.n_48bit = 6144,
		.cts_48bit = 54000,
	},
	.array[3] = {
		.tmds_clk = 27027,
		.n = 6144,
		.cts = 27027,
		.n_36bit = 8192,
		.cts_36bit = 54054,
		.n_48bit = 6144,
		.cts_48bit = 54054,
	},
	.array[4] = {
		.tmds_clk = 54000,
		.n = 6144,
		.cts = 54000,
		.n_36bit = 6144,
		.cts_36bit = 81000,
		.n_48bit = 6144,
		.cts_48bit = 108000,
	},
	.array[5] = {
		.tmds_clk = 54054,
		.n = 6144,
		.cts = 54054,
		.n_36bit = 6144,
		.cts_36bit = 81081,
		.n_48bit = 6144,
		.cts_48bit = 108108,
	},
	.array[6] = {
		.tmds_clk = 74176,
		.n = 11648,
		.cts = 140625,
		.n_36bit = 11648,
		.cts_36bit = 210937,
		.n_48bit = 11648,
		.cts_48bit = 281250,
	},
	.array[7] = {
		.tmds_clk = 74250,
		.n = 6144,
		.cts = 74250,
		.n_36bit = 6144,
		.cts_36bit = 111375,
		.n_48bit = 6144,
		.cts_48bit = 148500,
	},
	.array[8] = {
		.tmds_clk = 148352,
		.n = 5824,
		.cts = 140625,
		.n_36bit = 11648,
		.cts_36bit = 421875,
		.n_48bit = 5824,
		.cts_48bit = 281250,
	},
	.array[9] = {
		.tmds_clk = 148500,
		.n = 6144,
		.cts = 148500,
		.n_36bit = 6144,
		.cts_36bit = 222750,
		.n_48bit = 6144,
		.cts_48bit = 297000,
	},
	.array[10] = {
		.tmds_clk = 296703,
		.n = 5824,
		.cts = 281250,
	},
	.array[11] = {
		.tmds_clk = 297000,
		.n = 5120,
		.cts = 247500,
	},
	.array[12] = {
		.tmds_clk = 593407,
		.n = 5824,
		.cts = 562500,
	},
	.array[13] = {
		.tmds_clk = 594000,
		.n = 6144,
		.cts = 594000,
	},
	.def_n = 6144,
};

static const struct hdmi_audio_fs_ncts *all_aud_paras[] = {
	&aud_32k_para,
	&aud_44k1_para,
	&aud_48k_para,
	NULL,
};

u32 hdmi21_get_aud_n_paras(enum hdmi_audio_fs fs,
				  enum hdmi_color_depth cd,
				  u32 tmds_clk)
{
	const struct hdmi_audio_fs_ncts *p = NULL;
	u32 i, n;
	u32 N_multiples = 1;

	pr_info("hdmitx: fs = %d, cd = %d, tmds_clk = %d\n", fs, cd, tmds_clk);
	switch (fs) {
	case FS_32K:
		p = all_aud_paras[0];
		N_multiples = 1;
		break;
	case FS_44K1:
		p = all_aud_paras[1];
		N_multiples = 1;
		break;
	case FS_88K2:
		p = all_aud_paras[1];
		N_multiples = 2;
		break;
	case FS_176K4:
		p = all_aud_paras[1];
		N_multiples = 4;
		break;
	case FS_48K:
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	case FS_96K:
		p = all_aud_paras[2];
		N_multiples = 2;
		break;
	case FS_192K:
		p = all_aud_paras[2];
		N_multiples = 4;
		break;
	default: /* Default as FS_48K */
		p = all_aud_paras[2];
		N_multiples = 1;
		break;
	}
	for (i = 0; i < AUDIO_PARA_MAX_NUM; i++) {
		if (tmds_clk == p->array[i].tmds_clk)
			break;
	}

	if (i < AUDIO_PARA_MAX_NUM)
		if (cd == COLORDEPTH_24B || cd == COLORDEPTH_30B)
			n = p->array[i].n ? p->array[i].n : p->def_n;
		else if (cd == COLORDEPTH_36B)
			n = p->array[i].n_36bit ?
				p->array[i].n_36bit : p->def_n;
		else if (cd == COLORDEPTH_48B)
			n = p->array[i].n_48bit ?
				p->array[i].n_48bit : p->def_n;
		else
			n = p->array[i].n ? p->array[i].n : p->def_n;
	else
		n = p->def_n;
	return n * N_multiples;
}

bool _is_hdmi14_4k(enum hdmi_vic vic)
{
	bool ret = 0;

	switch (vic) {
	case HDMI_93_3840x2160p24_16x9:
	case HDMI_94_3840x2160p25_16x9:
	case HDMI_95_3840x2160p30_16x9:
	case HDMI_98_4096x2160p24_256x135:
		ret = 1;
		break;
	default:
		ret = 0;
		break;
	}

	return ret;
}

bool _is_hdmi4k_420(enum hdmi_vic vic)
{
	return 0;
}


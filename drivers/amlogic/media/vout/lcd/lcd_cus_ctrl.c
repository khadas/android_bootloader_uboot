// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/media/vout/lcd/lcd_cus_ctrl.h>
#include "lcd_common.h"

void lcd_cus_ctrl_dump_raw_data(struct aml_lcd_drv_s *pdrv)
{
	//todo
}

void lcd_cus_ctrl_dump_info(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_cus_ctrl_attr_config_s *attr_conf;
	struct lcd_detail_timing_s *ptiming;
	int i, j, ret, herr, verr;

	printf("cus_ctrl info:\n"
		"  ctrl_en:     0x%x\n"
		"  ctrl_cnt:    %d\n"
		"  timing_ctrl: valid:%d, active:0x%x\n",
		pdrv->config.cus_ctrl.ctrl_en,
		pdrv->config.cus_ctrl.ctrl_cnt,
		pdrv->config.cus_ctrl.timing_ctrl_valid,
		pdrv->config.cus_ctrl.active_timing_type);

	if (!pdrv->config.cus_ctrl.attr_config) {
		printf("\n");
		return;
	}
	for (i = 0; i < pdrv->config.cus_ctrl.ctrl_cnt; i++) {
		attr_conf = &pdrv->config.cus_ctrl.attr_config[i];
		printf("attr[%d] %s:\n"
			"  attr_type:  0x%02x\n"
			"  attr_flag:  %d\n"
			"  param_flag: %d\n"
			"  param_size: %d\n",
			attr_conf->attr_index,
			(attr_conf->active ? "(v)" : ""),
			attr_conf->attr_type,
			attr_conf->attr_flag,
			attr_conf->param_flag,
			attr_conf->param_size);

		if (attr_conf->param_size == 0)
			continue;
		switch (attr_conf->attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			if (!attr_conf->attr.ufr_attr)
				break;
			ret = lcd_config_timing_check(pdrv, &attr_conf->attr.ufr_attr->timing);
			verr = (ret >> 4) & 0xf;
			printf("  ufr_conf: %dx%d@%dhz\n"
				"    vtotal_min:  %d\n"
				"    vtotal_max:  %d\n",
				attr_conf->attr.ufr_attr->timing.h_active,
				attr_conf->attr.ufr_attr->timing.v_active,
				attr_conf->attr.ufr_attr->timing.frame_rate,
				attr_conf->attr.ufr_attr->timing.v_period_min,
				attr_conf->attr.ufr_attr->timing.v_period_max);
			printf("    fr_range:    %d~%d\n"
				"    vrr_range:   %d~%d\n",
				attr_conf->attr.ufr_attr->timing.frame_rate_min,
				attr_conf->attr.ufr_attr->timing.frame_rate_max,
				attr_conf->attr.ufr_attr->timing.vfreq_vrr_min,
				attr_conf->attr.ufr_attr->timing.vfreq_vrr_max);
			printf("    vpw:         %d\n"
				"    vbp:         %d%s\n"
				"    vfp:         %d%s\n",
				attr_conf->attr.ufr_attr->timing.vsync_width,
				attr_conf->attr.ufr_attr->timing.vsync_bp,
				((verr & 0x4) ? "(X)" : ((verr & 0x8) ? "(!)" : "")),
				attr_conf->attr.ufr_attr->timing.vsync_fp,
				((verr & 0x1) ? "(X)" : ((verr & 0x2) ? "(!)" : "")));
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			if (!attr_conf->attr.dfr_attr || !attr_conf->attr.dfr_attr->fr)
				break;
			for (j = 0; j < attr_conf->attr.dfr_attr->fr_cnt; j++) {
				ptiming = &attr_conf->attr.dfr_attr->fr[j].timing;
				ret = lcd_config_timing_check(pdrv, ptiming);
				herr = ret & 0xf;
				verr = (ret >> 4) & 0xf;
				printf("  dfr_conf[%d]: %dx%d@%dhz %s:\n"
					"    fr_range:      %d~%d\n"
					"    vrr_range:     %d~%d\n"
					"    is_dft_timing: %d\n"
					"    timing_check:  hbp(%s),hfp(%s),vbp(%s),vfp(%s)\n",
					j,
					ptiming->h_active,
					ptiming->v_active,
					ptiming->frame_rate,
					attr_conf->active ?
						((attr_conf->priv_sel == j) ? "(v)" : "") : "",
					ptiming->frame_rate_min,
					ptiming->frame_rate_max,
					ptiming->vfreq_vrr_min,
					ptiming->vfreq_vrr_max,
					(attr_conf->attr.dfr_attr->fr[j].timing_index ? 0 : 1),
					((herr & 0x4) ? "X" : ((herr & 0x8) ? "!" : "v")),
					((herr & 0x1) ? "X" : ((herr & 0x2) ? "!" : "v")),
					((verr & 0x4) ? "X" : ((verr & 0x8) ? "!" : "v")),
					((verr & 0x1) ? "X" : ((verr & 0x2) ? "!" : "v")));
			}
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			if (!attr_conf->attr.extend_tmg_attr ||
			    !attr_conf->attr.extend_tmg_attr->timing)
				break;
			for (j = 0; j < attr_conf->attr.extend_tmg_attr->group_cnt; j++) {
				ptiming = &attr_conf->attr.extend_tmg_attr->timing[j];
				ret = lcd_config_timing_check(pdrv, ptiming);
				herr = ret & 0xf;
				verr = (ret >> 4) & 0xf;
				printf("  extend_tmg_conf[%d]: %dx%d@%dhz %s:\n"
					"    fr_adj_type: %d\n"
					"    pclk:      %dHz\n"
					"    htotal:    %d\n"
					"    vtotal:    %d\n"
					"    hsync:     %d %d%s %d%s %d\n"
					"    vsync:     %d %d%s %d%s %d\n"
					"    fr_range:  %d~%d\n"
					"    vrr_range: %d~%d\n",
					j,
					ptiming->h_active,
					ptiming->v_active,
					ptiming->frame_rate,
					attr_conf->active ?
						((attr_conf->priv_sel == j) ? "(v)" : "") : "",
					ptiming->fr_adjust_type,
					ptiming->pixel_clk, ptiming->h_period,
					ptiming->v_period, ptiming->hsync_width,
					ptiming->hsync_bp,
					((herr & 0x4) ? "(X)" : ((herr & 0x8) ? "(!)" : "")),
					ptiming->hsync_fp,
					((herr & 0x1) ? "(X)" : ((herr & 0x2) ? "(!)" : "")),
					ptiming->hsync_pol,
					ptiming->vsync_width,
					ptiming->vsync_bp,
					((verr & 0x4) ? "(X)" : ((verr & 0x8) ? "(!)" : "")),
					ptiming->vsync_fp,
					((verr & 0x1) ? "(X)" : ((verr & 0x2) ? "(!)" : "")),
					ptiming->vsync_pol,
					ptiming->frame_rate_min,
					ptiming->frame_rate_max,
					ptiming->vfreq_vrr_min,
					ptiming->vfreq_vrr_max);
			}
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			printf("  clk_adv:\n"
				"    ss_freq:  %d\n"
				"    ss_mode:  %d\n",
				attr_conf->attr.clk_adv_attr->ss_freq,
				attr_conf->attr.clk_adv_attr->ss_mode);
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
			break;
		default:
			break;
		}
	}
	printf("\n");
}

static int lcd_cus_ctrl_parse_clk_adv_dts(struct aml_lcd_drv_s *pdrv,
					  struct lcd_cus_ctrl_attr_config_s *attr_conf)
{
	struct lcd_clk_adv_s *adv_attr;
	char *propdata, *dt_addr = lcd_get_dt_addr();
	int node_ofst = lcd_get_dts_panel_node_ofst(pdrv->index);

	adv_attr = malloc(sizeof(struct lcd_clk_adv_s));
	if (!adv_attr)
		return -1;
	memset(adv_attr, 0, sizeof(struct lcd_clk_adv_s));

	propdata = (char *)fdt_getprop(dt_addr, node_ofst, "clk_advanced", NULL);
	if (!propdata) {
		LCDERR("[%d]: failed to get clk_advanced\n", pdrv->index);
		free(adv_attr);
		return -1;
	}
	adv_attr->ss_freq = be32_to_cpup(((u32 *)propdata) + 0);
	adv_attr->ss_mode = be32_to_cpup(((u32 *)propdata) + 1);

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: cus_ctrl:CLK_ADV, freq=%hu, mode=%hu\n",
			pdrv->index, __func__, adv_attr->ss_freq, adv_attr->ss_mode);
	}

	attr_conf->attr.clk_adv_attr = adv_attr;
	return 0;
}

#define LCD_EXT_TIMING_MAX 8
static int lcd_cus_ctrl_parse_extend_tmg_dts(struct aml_lcd_drv_s *pdrv,
					     struct lcd_cus_ctrl_attr_config_s *attr_conf)
{
	struct lcd_extend_tmg_s *extend_tmg_attr;
	struct lcd_detail_timing_s *ptiming = NULL, *ptiming0 = NULL;
	char *propdata, snode[32], dbg_buf[128], *dt_addr = lcd_get_dt_addr();
	int node_ofst = lcd_get_dts_panel_node_ofst(pdrv->index);
	unsigned char etmg_idx, c_bit;//, tmp;

	if (node_ofst < 0)
		return -1;

	extend_tmg_attr = malloc(sizeof(struct lcd_extend_tmg_s));
	if (!extend_tmg_attr)
		return -1;
	memset(extend_tmg_attr, 0, sizeof(struct lcd_extend_tmg_s));

	for (etmg_idx = 0; etmg_idx < LCD_EXT_TIMING_MAX; etmg_idx++) {
		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (!propdata)
			break;

		ptiming = ptiming0;
		ptiming0 = realloc(ptiming, (etmg_idx + 1) * sizeof(struct lcd_detail_timing_s));
		if (!ptiming0) {
			LCDERR("[%d]: %s: detailed tmg realloc(%d) failed\n",
				pdrv->index, __func__, etmg_idx + 1);
			ptiming0 = ptiming;
			break;
		}

		extend_tmg_attr->group_cnt++;

		ptiming = ptiming0 + etmg_idx;
		memset(ptiming, 0, sizeof(struct lcd_detail_timing_s));

		ptiming->h_period    = be32_to_cpup(((u32 *)propdata) + 0);
		ptiming->h_active    = be32_to_cpup(((u32 *)propdata) + 1);
		ptiming->hsync_width = be32_to_cpup(((u32 *)propdata) + 2);
		ptiming->hsync_bp    = be32_to_cpup(((u32 *)propdata) + 3);
		ptiming->hsync_pol   = be32_to_cpup(((u32 *)propdata) + 4);
		ptiming->hsync_fp    = ptiming->h_period - ptiming->h_active -
						ptiming->hsync_width - ptiming->hsync_bp;
		ptiming->v_period    = be32_to_cpup(((u32 *)propdata) + 5);
		ptiming->v_active    = be32_to_cpup(((u32 *)propdata) + 6);
		ptiming->vsync_width = be32_to_cpup(((u32 *)propdata) + 7);
		ptiming->vsync_bp    = be32_to_cpup(((u32 *)propdata) + 8);
		ptiming->vsync_pol   = be32_to_cpup(((u32 *)propdata) + 9);
		ptiming->vsync_fp    = ptiming->v_period - ptiming->v_active -
						ptiming->vsync_width - ptiming->vsync_bp;

		c_bit = be32_to_cpup(((u32 *)propdata) + 10);
		ptiming->pixel_clk = be32_to_cpup(((u32 *)propdata) + 13);

		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu_frame_rate", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (propdata) {
			ptiming->frame_rate_min = be32_to_cpup(((u32 *)propdata) + 0);
			ptiming->frame_rate_max = be32_to_cpup(((u32 *)propdata) + 1);
		}

		memset(snode, 0, 32 * sizeof(char));
		sprintf(snode, "extend_tmg_%hu_range_setting", etmg_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, NULL);
		if (propdata) {
			ptiming->h_period_min = be32_to_cpup(((u32 *)propdata) + 0);
			ptiming->h_period_max = be32_to_cpup(((u32 *)propdata) + 1);
			ptiming->v_period_min = be32_to_cpup(((u32 *)propdata) + 2);
			ptiming->v_period_max = be32_to_cpup(((u32 *)propdata) + 3);
			ptiming->pclk_min     = be32_to_cpup(((u32 *)propdata) + 4);
			ptiming->pclk_max     = be32_to_cpup(((u32 *)propdata) + 5);
		} else {
			ptiming->h_period_min = ptiming->h_period;
			ptiming->h_period_max = ptiming->h_period;
			ptiming->v_period_min = ptiming->v_period;
			ptiming->v_period_max = ptiming->v_period;
			ptiming->pclk_min     = ptiming->pixel_clk;
			ptiming->pclk_max     = ptiming->pixel_clk;
		}
		ptiming->fr_adjust_type = propdata ? 3 : 0xff;

		//ptiming->fixed_type = 0xff;
		//memset(snode, 0, 32 * sizeof(char));
		//sprintf(snode, "extend_tmg_%hu_fixed", etmg_idx);
		//propdata = (char *)fdt_getprop(dt_addr, node_ofst, snode, &len);
		//if (propdata) {
		//	ptiming->fixed_type = be32_to_cpup((u32 *)propdata);
		//	len = (len > 36) ? 8 : (len / 4 - 1);
		//	for (tmp = 0; tmp < len; tmp++) {
		//		ptiming->fixed_val_set[tmp + 1] =
		//			be32_to_cpup(((u32 *)propdata) + tmp + 1);
		//	}
		//}

		lcd_clk_frame_rate_init(ptiming);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			memset(dbg_buf, 0, sizeof(char) * 128);
			dtimg_info_add(dbg_buf, ptiming, c_bit);
			LCDPR("[%d]: %s: extend timing[%d]: %s\n",
				pdrv->index, __func__, etmg_idx, dbg_buf);
		}

		lcd_config_timing_check(pdrv, ptiming);
	}

	extend_tmg_attr->timing = ptiming0;
	pdrv->config.cus_ctrl.timing_cnt += extend_tmg_attr->group_cnt;
	attr_conf->attr.extend_tmg_attr = extend_tmg_attr;

	return 0;
}

int lcd_cus_ctrl_load_from_dts(struct aml_lcd_drv_s *pdrv)
{
	int para_size, node_ofst;
	char *dt_addr = lcd_get_dt_addr();
	char *propdata, cusnode[25];
	unsigned char timing_ctrl_valid = 0, ctrl_cnt = 0, cus_idx, cus_type;
	struct lcd_cus_ctrl_attr_config_s *cus_cfg = NULL, *cus_cfg_tmp = NULL;
	int ret;

	node_ofst = lcd_get_dts_panel_node_ofst(pdrv->index);

	pdrv->config.cus_ctrl.ctrl_en = 0;
	pdrv->config.cus_ctrl.ctrl_cnt = 0;
	pdrv->config.cus_ctrl.timing_cnt = 0;
	pdrv->config.cus_ctrl.active_timing_type = LCD_CUS_CTRL_TYPE_MAX;
	pdrv->config.cus_ctrl.timing_switch_flag = 0;

	if (node_ofst < 0)
		return -1;

	for (cus_idx = 0; cus_idx < LCD_CUS_CTRL_ATTR_CNT_MAX; cus_idx++) {
		sprintf(cusnode, "cus_ctrl_%hu_attr", cus_idx);
		propdata = (char *)fdt_getprop(dt_addr, node_ofst, cusnode, &para_size);
		if (!propdata)
			break;

		cus_cfg_tmp = cus_cfg;
		cus_cfg = realloc(cus_cfg_tmp,
			(ctrl_cnt + 1) * sizeof(struct lcd_cus_ctrl_attr_config_s));
		if (!cus_cfg) {
			LCDERR("[%d]: %s: cus_ctrl_attr realloc(%d) failed\n",
				pdrv->index, __func__, ctrl_cnt + 1);
			cus_cfg = cus_cfg_tmp;
			break;
		}
		cus_cfg_tmp = cus_cfg + ctrl_cnt;

		cus_cfg_tmp->attr_index = cus_idx;
		cus_cfg_tmp->param_flag = 0;
		cus_cfg_tmp->param_size = 1;
		cus_cfg_tmp->priv_sel = 0;

		cus_type = be32_to_cpup(((u32 *)propdata) + 0);
		cus_cfg_tmp->attr_type = cus_type;
		switch (cus_type) {
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			pdrv->config.cus_ctrl.timing_switch_flag =
				be32_to_cpup(((u32 *)propdata) + 1);
			ret = lcd_cus_ctrl_parse_extend_tmg_dts(pdrv, cus_cfg_tmp);
			if (ret >= 0) {
				cus_cfg_tmp->param_flag = ret;
				cus_cfg_tmp->attr_flag = pdrv->config.cus_ctrl.timing_switch_flag;
				pdrv->config.cus_ctrl.timing_cnt += ret;
				ctrl_cnt++;
				pdrv->config.cus_ctrl.ctrl_en |= 1 << cus_idx;
				timing_ctrl_valid++;
			}
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			ret = lcd_cus_ctrl_parse_clk_adv_dts(pdrv, cus_cfg_tmp);
			if (ret >= 0) {
				ctrl_cnt++;
				pdrv->config.cus_ctrl.ctrl_en |= 1 << cus_idx;
			}
			break;
		default:
			LCDERR("[%d]: %s: invalid cus_type: %d\n",
				pdrv->index, __func__, cus_type);
			break;
		}
	}
	pdrv->config.cus_ctrl.attr_config = cus_cfg;
	pdrv->config.cus_ctrl.ctrl_cnt = ctrl_cnt;
	pdrv->config.cus_ctrl.timing_ctrl_valid = (bool)timing_ctrl_valid;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: ctrl_en=0x%x, ctrl_cnt=%d, timing_switch_flag=%d\n",
			pdrv->index, __func__, pdrv->config.cus_ctrl.ctrl_en, ctrl_cnt,
			pdrv->config.cus_ctrl.timing_switch_flag);
	}

	return 0;
}

static int lcd_cus_ctrl_attr_parse_ufr_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_ufr_s *ufr_attr;
	struct lcd_detail_timing_s *ptiming;
	unsigned int hfp, vfp, offset = 0;

	ufr_attr = malloc(sizeof(struct lcd_ufr_s));
	if (!ufr_attr)
		return -1;
	memset(ufr_attr, 0, sizeof(struct lcd_ufr_s));

	ptiming = &ufr_attr->timing;
	memcpy(ptiming, &pdrv->config.timing.dft_timing, sizeof(struct lcd_detail_timing_s));

	ptiming->v_period_min = *(unsigned short *)(p + offset);
	offset += 2;
	ptiming->v_period_max = *(unsigned short *)(p + offset);
	offset += 2;
	if (attr_conf->param_size < offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_err;
	if (attr_conf->param_size == offset) {
		//frame_rate range is update for compatibility in lcd_fr_range_update
		ptiming->frame_rate_min = 0;
		ptiming->frame_rate_max = 0;
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_next;
	}

	ptiming->frame_rate_min = *(unsigned short *)(p + offset);
	offset += 2;
	ptiming->frame_rate_max = *(unsigned short *)(p + offset);
	offset += 2;
	if (attr_conf->param_size < offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_err;
	if (attr_conf->param_size == offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_next;

	ptiming->vsync_width = *(unsigned short *)(p + offset);
	offset += 2;
	ptiming->vsync_bp = *(unsigned short *)(p + offset);
	offset += 2;
	if (attr_conf->param_size < offset)
		goto lcd_cus_ctrl_attr_parse_ufr_ukey_err;

lcd_cus_ctrl_attr_parse_ufr_ukey_next:
	ptiming->v_active /= 2;
	ptiming->v_period /= 2;
	hfp = ptiming->h_period - ptiming->h_active - ptiming->hsync_width - ptiming->hsync_bp;
	vfp = ptiming->v_period - ptiming->v_active - ptiming->vsync_width - ptiming->vsync_bp;
	ptiming->hsync_fp = hfp;
	ptiming->vsync_fp = vfp;
	lcd_clk_frame_rate_init(ptiming);
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: %dx%dp%dhz\n",
			pdrv->index, __func__,
			ptiming->h_active, ptiming->v_active, ptiming->frame_rate);
	}

	lcd_config_timing_check(pdrv, ptiming);

	pdrv->config.cus_ctrl.timing_cnt += 1;
	attr_conf->attr.ufr_attr = ufr_attr;
	return 0;

lcd_cus_ctrl_attr_parse_ufr_ukey_err:
	memset(ufr_attr, 0, sizeof(struct lcd_ufr_s));
	free(ufr_attr);
	LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
		pdrv->index, __func__, attr_conf->param_size, offset);
	return -1;
}

static int lcd_cus_ctrl_attr_parse_dfr_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_dfr_s *dfr_attr;
	struct lcd_detail_timing_s *ptiming;
	unsigned int offset = 0;
	unsigned int hfp, vfp, temp, fr_size, tmg_size;
	int i, n;

	dfr_attr = malloc(sizeof(struct lcd_dfr_s));
	if (!dfr_attr)
		return -1;
	memset(dfr_attr, 0, sizeof(struct lcd_dfr_s));

	dfr_attr->fr_cnt = *(p + offset);
	offset += 1;
	dfr_attr->tmg_group_cnt = *(p + offset);
	offset += 1;
	fr_size = dfr_attr->fr_cnt * sizeof(struct lcd_dfr_fr_s);
	tmg_size = dfr_attr->tmg_group_cnt * sizeof(struct lcd_dfr_timing_s);

	if (dfr_attr->fr_cnt) {
		dfr_attr->fr = malloc(fr_size);
		if (!dfr_attr->fr) {
			memset(dfr_attr, 0, sizeof(struct lcd_dfr_s));
			free(dfr_attr);
			return -1;
		}
		memset(dfr_attr->fr, 0, fr_size);
		for (i = 0; i < dfr_attr->fr_cnt; i++) {
			temp = *(unsigned short *)(p + offset);
			dfr_attr->fr[i].frame_rate = temp & 0xfff;
			dfr_attr->fr[i].timing_index = (temp >> 12) & 0xf;
			offset += 2;
		}
	}

	if (dfr_attr->tmg_group_cnt) {
		dfr_attr->dfr_timing = malloc(tmg_size);
		if (!dfr_attr->dfr_timing) {
			memset(dfr_attr->fr, 0, fr_size);
			free(dfr_attr->fr);
			memset(dfr_attr, 0, sizeof(struct lcd_dfr_s));
			free(dfr_attr);
			return -1;
		}
		memset(dfr_attr->dfr_timing, 0, tmg_size);
		for (i = 0;  i < dfr_attr->tmg_group_cnt; i++) {
			dfr_attr->dfr_timing[i].htotal = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].vtotal = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].vtotal_min = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].vtotal_max = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].frame_rate_min = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].frame_rate_max = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].hpw = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].hbp = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].vpw = *(unsigned short *)(p + offset);
			offset += 2;
			dfr_attr->dfr_timing[i].vbp = *(unsigned short *)(p + offset);
			offset += 2;
		}
	}

	if (attr_conf->param_size < offset) {
		memset(dfr_attr->dfr_timing, 0, tmg_size);
		free(dfr_attr->dfr_timing);
		memset(dfr_attr->fr, 0, fr_size);
		free(dfr_attr->fr);
		memset(dfr_attr, 0, sizeof(struct lcd_dfr_s));
		free(dfr_attr);
		LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
			pdrv->index, __func__, attr_conf->param_size, offset);
		return -1;
	}

	for (i = 0; i < dfr_attr->fr_cnt; i++) {
		ptiming = &dfr_attr->fr[i].timing;
		memcpy(ptiming, &pdrv->config.timing.dft_timing,
			sizeof(struct lcd_detail_timing_s));
		ptiming->frame_rate = dfr_attr->fr[i].frame_rate;
		if (dfr_attr->fr[i].timing_index == 0) {
			ptiming->pixel_clk = ptiming->frame_rate *
				ptiming->h_period * ptiming->v_period;
			//frame_rate range is update for compatibility in lcd_fr_range_update
			ptiming->frame_rate_min = 0;
			ptiming->frame_rate_max = 0;
		} else {
			n = dfr_attr->fr[i].timing_index - 1;
			if (n >= dfr_attr->tmg_group_cnt) {
				LCDERR("[%d]: %s: timing_index %d err, tmg_group_cnt %d\n",
					pdrv->index, __func__, dfr_attr->fr[i].timing_index,
					dfr_attr->tmg_group_cnt);
				memset(dfr_attr->dfr_timing, 0, tmg_size);
				free(dfr_attr->dfr_timing);
				memset(dfr_attr->fr, 0, fr_size);
				free(dfr_attr->fr);
				memset(dfr_attr, 0, sizeof(struct lcd_dfr_s));
				free(dfr_attr);
				return -1;
			}
			ptiming->h_period = dfr_attr->dfr_timing[n].htotal;
			ptiming->v_period = dfr_attr->dfr_timing[n].vtotal;
			ptiming->v_period_min = dfr_attr->dfr_timing[n].vtotal_min;
			ptiming->v_period_max = dfr_attr->dfr_timing[n].vtotal_max;
			ptiming->frame_rate_min = dfr_attr->dfr_timing[n].frame_rate_min;
			ptiming->frame_rate_max = dfr_attr->dfr_timing[n].frame_rate_max;
			ptiming->pixel_clk = ptiming->frame_rate *
				ptiming->h_period * ptiming->v_period;
		}
		hfp = ptiming->h_period - ptiming->h_active -
			ptiming->hsync_width - ptiming->hsync_bp;
		vfp = ptiming->v_period - ptiming->v_active -
			ptiming->vsync_width - ptiming->vsync_bp;
		ptiming->hsync_fp = hfp;
		ptiming->vsync_fp = vfp;
		lcd_clk_frame_rate_init(ptiming);
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: %s: dfr[%d]: %dx%dp%dhz\n",
				pdrv->index, __func__, i,
				ptiming->h_active, ptiming->v_active, ptiming->frame_rate);
		}

		lcd_config_timing_check(pdrv, ptiming);
	}

	pdrv->config.cus_ctrl.timing_cnt += dfr_attr->fr_cnt;
	attr_conf->attr.dfr_attr = dfr_attr;
	return 0;
}

static int lcd_cus_ctrl_attr_parse_extend_tmg_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_extend_tmg_s *extend_tmg_attr;
	struct lcd_detail_timing_s *ptiming;
	unsigned int offset = 0;
	unsigned int hfp, vfp, temp, size;
	int i;

	extend_tmg_attr = malloc(sizeof(struct lcd_extend_tmg_s));
	if (!extend_tmg_attr)
		return -1;
	memset(extend_tmg_attr, 0, sizeof(struct lcd_extend_tmg_s));

	extend_tmg_attr->group_cnt = attr_conf->param_flag;
	size = extend_tmg_attr->group_cnt * sizeof(struct lcd_detail_timing_s);
	if (extend_tmg_attr->group_cnt) {
		extend_tmg_attr->timing = malloc(size);
		if (!extend_tmg_attr->timing) {
			memset(extend_tmg_attr, 0, sizeof(struct lcd_extend_tmg_s));
			free(extend_tmg_attr);
			return -1;
		}
		memset(extend_tmg_attr->timing, 0, size);
		for (i = 0;  i < extend_tmg_attr->group_cnt; i++) {
			ptiming = &extend_tmg_attr->timing[i];
			ptiming->h_active = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->v_active = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->h_period = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->v_period = *(unsigned short *)(p + offset);
			offset += 2;
			temp = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->hsync_width = temp & 0xfff;
			ptiming->hsync_pol = (temp >> 12) & 0xf;
			ptiming->hsync_bp = *(unsigned short *)(p + offset);
			offset += 2;
			temp = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->vsync_width = temp & 0xfff;
			ptiming->vsync_pol = (temp >> 12) & 0xf;
			ptiming->vsync_bp = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->fr_adjust_type = *(p + offset);
			offset += 1;
			ptiming->pixel_clk = *(unsigned int *)(p + offset);
			offset += 4;

			ptiming->h_period_min = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->h_period_max = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->v_period_min = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->v_period_max = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->frame_rate_min = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->frame_rate_max = *(unsigned short *)(p + offset);
			offset += 2;
			ptiming->pclk_min = *(unsigned int *)(p + offset);
			offset += 4;
			ptiming->pclk_max = *(unsigned int *)(p + offset);
			offset += 4;

			hfp = ptiming->h_period - ptiming->h_active -
				ptiming->hsync_width - ptiming->hsync_bp;
			vfp = ptiming->v_period - ptiming->v_active -
				ptiming->vsync_width - ptiming->vsync_bp;
			ptiming->hsync_fp = hfp;
			ptiming->vsync_fp = vfp;

			lcd_clk_frame_rate_init(&extend_tmg_attr->timing[i]);
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: extend_tmg[%d]: %dx%dp%dhz\n",
					pdrv->index, __func__, i,
					ptiming->h_active, ptiming->v_active, ptiming->frame_rate);
			}

			lcd_config_timing_check(pdrv, ptiming);
		}
	}

	if (attr_conf->param_size < offset) {
		memset(extend_tmg_attr->timing, 0, size);
		free(extend_tmg_attr->timing);
		memset(extend_tmg_attr, 0, sizeof(struct lcd_extend_tmg_s));
		free(extend_tmg_attr);
		LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
			pdrv->index, __func__, attr_conf->param_size, offset);
		return -1;
	}

	pdrv->config.cus_ctrl.timing_cnt += extend_tmg_attr->group_cnt;
	attr_conf->attr.extend_tmg_attr = extend_tmg_attr;
	return 0;
}

static int lcd_cus_ctrl_attr_parse_clk_adv_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	struct lcd_clk_adv_s *adv_attr;
	unsigned int offset = 0;

	adv_attr = malloc(sizeof(struct lcd_clk_adv_s));
	if (!adv_attr)
		return -1;
	memset(adv_attr, 0, sizeof(struct lcd_clk_adv_s));

	if (attr_conf->attr_flag & (1 << 0)) {
		adv_attr->ss_freq = *(unsigned char *)(p + offset);
		offset += 1;
		adv_attr->ss_mode = *(unsigned char *)(p + offset);
		offset += 1;
	}

	if (attr_conf->param_size < offset) {
		memset(adv_attr, 0, sizeof(struct lcd_clk_adv_s));
		free(adv_attr);
		LCDERR("[%d]: %s: param_size(%d) and offset(%d) are mismatch!\n",
			pdrv->index, __func__, attr_conf->param_size, offset);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: ss_freq = %d, ss_mode = %d\n",
			pdrv->index, __func__,
			adv_attr->ss_freq, adv_attr->ss_mode);
	}

	attr_conf->attr.clk_adv_attr = adv_attr;
	return 0;
}

static int lcd_cus_ctrl_attr_parse_tcon_sw_pol_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	return 0;
}

static int lcd_cus_ctrl_attr_parse_tcon_sw_pdf_ukey(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf, unsigned char *p)
{
	return 0;
}

int lcd_cus_ctrl_load_from_unifykey(struct aml_lcd_drv_s *pdrv, unsigned char *buf,
		unsigned int max_size)
{
	unsigned char *p;
	struct lcd_cus_ctrl_attr_config_s *attr_conf;
	unsigned int ctrl_en, ctrl_attr, ctrl_cnt = 0;
	unsigned int offset = 0, param_size, i, n;
	unsigned char timing_ctrl_valid = 0;
	char str[128];
	int len, ret;

	ctrl_en = *(unsigned int *)buf;
	for (i = 0; i < LCD_CUS_CTRL_ATTR_CNT_MAX; i++) {
		if (ctrl_en & (1 << i))
			ctrl_cnt = i + 1;
	}
	if (ctrl_cnt == 0)
		return 0;

	pdrv->config.cus_ctrl.ctrl_en = ctrl_en;
	pdrv->config.cus_ctrl.ctrl_cnt = ctrl_cnt;
	pdrv->config.cus_ctrl.timing_cnt = 0;
	pdrv->config.cus_ctrl.active_timing_type = LCD_CUS_CTRL_TYPE_MAX;
	pdrv->config.cus_ctrl.timing_switch_flag = LCD_VMODE_SWITCH_NONE;
	pdrv->config.cus_ctrl.cur_timing_attr = NULL;

	attr_conf = malloc(ctrl_cnt * sizeof(struct lcd_cus_ctrl_attr_config_s));
	if (!attr_conf)
		return -1;
	memset(attr_conf, 0, ctrl_cnt * sizeof(struct lcd_cus_ctrl_attr_config_s));
	pdrv->config.cus_ctrl.attr_config = attr_conf;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: ctrl_en=0x%x, ctrl_cnt=%d\n",
			pdrv->index, __func__, ctrl_en, ctrl_cnt);
	}

	offset = 4; //ctrl_en size
	n = 0;
	for (i = 0; i < LCD_CUS_CTRL_ATTR_CNT_MAX; i++) {
		if (n >= ctrl_cnt)
			break;
		if (offset >= max_size) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: offset %d reach max_size %d, exit\n",
					pdrv->index, __func__, offset, max_size);
			}
			break;
		}
		p = buf + offset;

		if (ctrl_en & (1 << i)) { //valid
			ctrl_attr = *(unsigned short *)p;
			attr_conf[n].attr_index = i;
			attr_conf[n].attr_flag = ctrl_attr & 0xf;
			attr_conf[n].param_flag = (ctrl_attr >> 4) & 0xf;
			attr_conf[n].attr_type = (ctrl_attr >> 8) & 0xff;
			param_size = *(unsigned short *)(p + 2);
			attr_conf[n].param_size = param_size;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				len = sprintf(str, "attr_type=%x, attr_flag=%d, ",
					attr_conf[n].attr_type,
					attr_conf[n].attr_flag);
				sprintf(str + len, "param_flag=%d, param_size=%d",
					attr_conf[n].param_flag,
					attr_conf[n].param_size);
				LCDPR("[%d]: %s: attr[%d]: %s\n",
					pdrv->index, __func__, i, str);
			}

			switch (attr_conf[n].attr_type) {
			case LCD_CUS_CTRL_TYPE_UFR:
				if (attr_conf[n].attr_flag > 0)
					timing_ctrl_valid = 1;
				ret = lcd_cus_ctrl_attr_parse_ufr_ukey(pdrv,
					&attr_conf[n], (p + 4));
				break;
			case LCD_CUS_CTRL_TYPE_DFR:
				if (attr_conf[n].attr_flag > 0)
					timing_ctrl_valid = 1;
				ret = lcd_cus_ctrl_attr_parse_dfr_ukey(pdrv,
					&attr_conf[n], (p + 4));
				break;
			case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
				if (attr_conf[n].attr_flag > 0)
					timing_ctrl_valid = 1;
				ret = lcd_cus_ctrl_attr_parse_extend_tmg_ukey(pdrv,
					&attr_conf[n], (p + 4));
				break;
			case LCD_CUS_CTRL_TYPE_CLK_ADV:
				ret = lcd_cus_ctrl_attr_parse_clk_adv_ukey(pdrv,
					&attr_conf[n], (p + 4));
				break;
			case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
				LCDERR("todo\n");
				ret = lcd_cus_ctrl_attr_parse_tcon_sw_pol_ukey(pdrv,
					&attr_conf[n], (p + 4));
				break;
			case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
				LCDERR("todo\n");
				ret = lcd_cus_ctrl_attr_parse_tcon_sw_pdf_ukey(pdrv,
					&attr_conf[n], (p + 4));
				break;
			default:
				LCDERR("[%d]: %s: invalid attr_type: 0x%x\n",
					pdrv->index, __func__, attr_conf[n].attr_type);
				goto lcd_cus_ctrl_load_from_unifykey_err;
			}
			n++;
		} else {
			param_size = 0;
			ret = 0;
		}
		if (ret)
			goto lcd_cus_ctrl_load_from_unifykey_err;
		offset += (param_size + 4); //4 for attr_x and param_size
	}

	if (timing_ctrl_valid)
		pdrv->config.cus_ctrl.timing_ctrl_valid = timing_ctrl_valid;
	return 0;

lcd_cus_ctrl_load_from_unifykey_err:
	lcd_cus_ctrl_config_remove(pdrv);
	return -1;
}

void lcd_cus_ctrl_config_remove(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_cus_ctrl_attr_config_s *attr_conf;
	int size, i;

	if (!pdrv->config.cus_ctrl.attr_config)
		return;

	for (i = 0; i < pdrv->config.cus_ctrl.ctrl_cnt; i++) {
		attr_conf = &pdrv->config.cus_ctrl.attr_config[i];
		switch (attr_conf->attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			if (!attr_conf->attr.ufr_attr)
				break;
			memset(attr_conf->attr.ufr_attr, 0, sizeof(struct lcd_ufr_s));
			free(attr_conf->attr.ufr_attr);
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			if (!attr_conf->attr.dfr_attr)
				break;
			if (attr_conf->attr.dfr_attr->fr) {
				size = attr_conf->attr.dfr_attr->fr_cnt *
					sizeof(struct lcd_dfr_fr_s);
				memset(attr_conf->attr.dfr_attr->fr, 0, size);
				free(attr_conf->attr.dfr_attr->fr);
			}
			if (attr_conf->attr.dfr_attr->dfr_timing) {
				size = attr_conf->attr.dfr_attr->tmg_group_cnt *
					sizeof(struct lcd_dfr_timing_s);
				memset(attr_conf->attr.dfr_attr->dfr_timing, 0, size);
				free(attr_conf->attr.dfr_attr->dfr_timing);
			}
			memset(attr_conf->attr.dfr_attr, 0, sizeof(struct lcd_dfr_s));
			free(attr_conf->attr.dfr_attr);
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			if (!attr_conf->attr.extend_tmg_attr)
				break;
			if (attr_conf->attr.extend_tmg_attr->timing) {
				size = attr_conf->attr.extend_tmg_attr->group_cnt *
					sizeof(struct lcd_detail_timing_s);
				memset(attr_conf->attr.extend_tmg_attr->timing, 0, size);
				free(attr_conf->attr.extend_tmg_attr->timing);
			}
			memset(attr_conf->attr.extend_tmg_attr, 0, sizeof(struct lcd_extend_tmg_s));
			free(attr_conf->attr.extend_tmg_attr);
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			if (!attr_conf->attr.clk_adv_attr)
				break;
			memset(attr_conf->attr.clk_adv_attr, 0, sizeof(struct lcd_clk_adv_s));
			free(attr_conf->attr.clk_adv_attr);
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
		default:
			break;
		}
	}
	size = pdrv->config.cus_ctrl.ctrl_cnt * sizeof(struct lcd_cus_ctrl_attr_config_s);
	memset(pdrv->config.cus_ctrl.attr_config, 0, size);
	free(pdrv->config.cus_ctrl.attr_config);
	pdrv->config.cus_ctrl.attr_config = NULL;
}

static int lcd_cus_ctrl_config_update_clk_adv(struct aml_lcd_drv_s *pdrv,
		struct lcd_cus_ctrl_attr_config_s *attr_conf)
{
	if (attr_conf->attr_flag & (1 << 0)) {
		pdrv->config.timing.ss_freq = attr_conf->attr.clk_adv_attr->ss_freq;
		pdrv->config.timing.ss_mode = attr_conf->attr.clk_adv_attr->ss_mode;
	}

	attr_conf->active = 1;
	return 0;
}

int lcd_cus_ctrl_config_update(struct aml_lcd_drv_s *pdrv, void *param, unsigned int mask_sel)
{
	struct lcd_cus_ctrl_attr_config_s *attr_conf;
	struct lcd_detail_timing_s *tmg_match;
	struct lcd_detail_timing_s *ptiming;
	struct lcd_dfr_s *p_dfr;
	char str[128];
	int tmg_sel = 0, i, j, ret = -1;

	if (!pdrv->config.cus_ctrl.attr_config)
		return -1;

	for (i = 0; i < pdrv->config.cus_ctrl.ctrl_cnt; i++) {
		attr_conf = &pdrv->config.cus_ctrl.attr_config[i];
		switch (attr_conf->attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			if ((mask_sel & LCD_CUS_CTRL_SEL_UFR) == 0)
				break;
			tmg_sel = 1;

			if (attr_conf->attr_flag == 0)
				break;
			if (!attr_conf->attr.ufr_attr || !param)
				break;

			tmg_match = (struct lcd_detail_timing_s *)param;
			ptiming = &attr_conf->attr.ufr_attr->timing;
			if (tmg_match == ptiming) {
				if (pdrv->config.cus_ctrl.cur_timing_attr)
					pdrv->config.cus_ctrl.cur_timing_attr->active = 0;
				pdrv->config.cus_ctrl.cur_timing_attr = attr_conf;
				pdrv->config.cus_ctrl.active_timing_type = LCD_CUS_CTRL_TYPE_UFR;
				pdrv->config.cus_ctrl.timing_switch_flag = attr_conf->attr_flag;
				attr_conf->active = 1;
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
					LCDPR("[%d]: %s: attr[%d] ufr: %dx%dp%dhz\n",
						pdrv->index, __func__, i,
						ptiming->h_active,
						ptiming->v_active,
						ptiming->frame_rate);
				}
				return 0;
			}
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			if ((mask_sel & LCD_CUS_CTRL_SEL_DFR) == 0)
				break;
			tmg_sel = 1;

			if (attr_conf->attr_flag == 0)
				break;
			if (!attr_conf->attr.dfr_attr || !param)
				break;
			tmg_match = (struct lcd_detail_timing_s *)param;
			p_dfr = attr_conf->attr.dfr_attr;
			for (j = 0; j < p_dfr->fr_cnt; j++) {
				ptiming = &p_dfr->fr[j].timing;
				if (tmg_match == ptiming) {
					if (pdrv->config.cus_ctrl.cur_timing_attr)
						pdrv->config.cus_ctrl.cur_timing_attr->active = 0;
					pdrv->config.cus_ctrl.cur_timing_attr = attr_conf;
					pdrv->config.cus_ctrl.active_timing_type =
						LCD_CUS_CTRL_TYPE_DFR;
					pdrv->config.cus_ctrl.timing_switch_flag =
						attr_conf->attr_flag;
					attr_conf->active = 1;
					attr_conf->priv_sel = j;
					if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
						LCDPR("[%d]: %s: attr[%d] dfr[%d]: %dx%dp%dhz\n",
							pdrv->index, __func__, i, j,
							ptiming->h_active,
							ptiming->v_active,
							ptiming->frame_rate);
					}
					return 0;
				}
			}
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			if ((mask_sel & LCD_CUS_CTRL_SEL_EXTEND_TMG) == 0)
				break;
			tmg_sel = 1;

			if (attr_conf->attr_flag == 0)
				break;
			if (!attr_conf->attr.extend_tmg_attr || !param)
				break;
			tmg_match = (struct lcd_detail_timing_s *)param;
			for (j = 0; j < attr_conf->attr.extend_tmg_attr->group_cnt; j++) {
				ptiming = &attr_conf->attr.extend_tmg_attr->timing[j];
				if (tmg_match == ptiming) {
					if (pdrv->config.cus_ctrl.cur_timing_attr)
						pdrv->config.cus_ctrl.cur_timing_attr->active = 0;
					pdrv->config.cus_ctrl.cur_timing_attr = attr_conf;
					pdrv->config.cus_ctrl.active_timing_type =
						LCD_CUS_CTRL_TYPE_EXTEND_TMG;
					pdrv->config.cus_ctrl.timing_switch_flag =
						attr_conf->attr_flag;
					attr_conf->active = 1;
					attr_conf->priv_sel = j;
					if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
						sprintf(str, "extend_tmg[%d]: %dx%dp%dhz",
							j, ptiming->h_active,
							ptiming->v_active,
							ptiming->frame_rate);
						LCDPR("[%d]: %s: attr[%d] %s\n",
							pdrv->index, __func__, i, str);
					}
					return 0;
				}
			}
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			if ((mask_sel & LCD_CUS_CTRL_SEL_CLK_ADV) == 0)
				break;
			if (!attr_conf->attr.clk_adv_attr)
				break;
			ret = lcd_cus_ctrl_config_update_clk_adv(pdrv, attr_conf);
			return ret;
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
			if ((mask_sel & LCD_CUS_CTRL_SEL_TCON_SW_POL) == 0)
				break;
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
			if ((mask_sel & LCD_CUS_CTRL_SEL_TCON_SW_PDF) == 0)
				break;
			break;
		default:
			break;
		}
	}

	if (tmg_sel) { //timing_sel non active, means use default timing
		pdrv->config.cus_ctrl.active_timing_type = LCD_CUS_CTRL_TYPE_MAX;
		pdrv->config.cus_ctrl.timing_switch_flag = LCD_VMODE_SWITCH_NONE;
		pdrv->config.cus_ctrl.cur_timing_attr = NULL;
	}

	return ret;
}

void lcd_cus_ctrl_state_clear(struct aml_lcd_drv_s *pdrv, unsigned int mask_sel)
{
	struct lcd_cus_ctrl_attr_config_s *attr_conf;
	int i;

	if (!pdrv->config.cus_ctrl.attr_config)
		return;

	for (i = 0; i < pdrv->config.cus_ctrl.ctrl_cnt; i++) {
		attr_conf = &pdrv->config.cus_ctrl.attr_config[i];
		switch (attr_conf->attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			if (mask_sel & LCD_CUS_CTRL_SEL_UFR)
				attr_conf->active = 0;
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			if (mask_sel & LCD_CUS_CTRL_SEL_DFR)
				attr_conf->active = 0;
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			if (mask_sel & LCD_CUS_CTRL_SEL_EXTEND_TMG)
				attr_conf->active = 0;
			break;
		case LCD_CUS_CTRL_TYPE_CLK_ADV:
			if (mask_sel & LCD_CUS_CTRL_SEL_CLK_ADV)
				attr_conf->active = 0;
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_POL:
			if (mask_sel & LCD_CUS_CTRL_SEL_TCON_SW_POL)
				attr_conf->active = 0;
			break;
		case LCD_CUS_CTRL_TYPE_TCON_SW_PDF:
			if (mask_sel & LCD_CUS_CTRL_SEL_TCON_SW_PDF)
				attr_conf->active = 0;
			break;
		default:
			break;
		}
	}
}

int lcd_cus_ctrl_timing_is_valid(struct aml_lcd_drv_s *pdrv)
{
	if (pdrv->config.cus_ctrl.timing_ctrl_valid)
		return 1;

	return 0;
}

int lcd_cus_ctrl_timing_is_activated(struct aml_lcd_drv_s *pdrv)
{
	int ret = 0;

	switch (pdrv->config.cus_ctrl.active_timing_type) {
	case LCD_CUS_CTRL_TYPE_UFR:
	case LCD_CUS_CTRL_TYPE_DFR:
	case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
		if (pdrv->config.cus_ctrl.timing_switch_flag > 0)
			ret = 1;
		break;
	default:
		break;
	}

	return ret;
}

struct lcd_detail_timing_s **lcd_cus_ctrl_timing_match_get(struct aml_lcd_drv_s *pdrv)
{
	union lcd_cus_ctrl_attr_u *cus_ctrl_attr;
	struct lcd_detail_timing_s **timing_match;
	int cnt, i, j, n = 0;

	if (!pdrv->config.cus_ctrl.attr_config)
		return NULL;

	cnt = pdrv->config.cus_ctrl.timing_cnt * sizeof(struct lcd_detail_timing_s *);
	timing_match = malloc(cnt);
	if (!timing_match)
		return NULL;
	memset(timing_match, 0, cnt);

	for (i = 0; i < pdrv->config.cus_ctrl.ctrl_cnt; i++) {
		cus_ctrl_attr = &pdrv->config.cus_ctrl.attr_config[i].attr;
		switch (pdrv->config.cus_ctrl.attr_config[i].attr_type) {
		case LCD_CUS_CTRL_TYPE_UFR:
			if (!cus_ctrl_attr->ufr_attr)
				break;
			timing_match[n] = &cus_ctrl_attr->ufr_attr->timing;
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("[%d]: %s: attr[%d] ufr: %dx%dp%dhz\n",
					pdrv->index, __func__, i,
					timing_match[n]->h_active,
					timing_match[n]->v_active,
					timing_match[n]->frame_rate);
			}
			n++;
			break;
		case LCD_CUS_CTRL_TYPE_DFR:
			if (!cus_ctrl_attr->dfr_attr || !cus_ctrl_attr->dfr_attr->fr)
				break;
			for (j = 0; j < cus_ctrl_attr->dfr_attr->fr_cnt; j++) {
				timing_match[n] = &cus_ctrl_attr->dfr_attr->fr[j].timing;
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
					LCDPR("[%d]: %s: attr[%d] dfr[%d]: %dx%dp%dhz\n",
						pdrv->index, __func__, i, j,
						timing_match[n]->h_active,
						timing_match[n]->v_active,
						timing_match[n]->frame_rate);
				}
				n++;
			}
			break;
		case LCD_CUS_CTRL_TYPE_EXTEND_TMG:
			if (!cus_ctrl_attr->extend_tmg_attr ||
			    !cus_ctrl_attr->extend_tmg_attr->timing)
				break;
			for (j = 0; j < cus_ctrl_attr->extend_tmg_attr->group_cnt; j++) {
				timing_match[n] = &cus_ctrl_attr->extend_tmg_attr->timing[j];
				if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
					LCDPR("[%d]: %s: attr[%d] extend_tmg[%d]: %dx%dp%dhz\n",
						pdrv->index, __func__, i, j,
						timing_match[n]->h_active,
						timing_match[n]->v_active,
						timing_match[n]->frame_rate);
				}
				n++;
			}
			break;
		default:
			break;
		}
	}

	return timing_match;
}


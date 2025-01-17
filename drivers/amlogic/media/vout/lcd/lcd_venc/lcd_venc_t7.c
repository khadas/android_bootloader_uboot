// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/io.h>
#ifdef CONFIG_AML_VPP
#include <amlogic/media/vpp/vpp.h>
#endif
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "../lcd_common.h"
#include "lcd_venc.h"

static void lcd_venc_wait_vsync(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset, reg;
	int line_cnt, line_cnt_previous;
	int i = 0;

	offset = pdrv->data->offset_venc[pdrv->index];
	reg = VPU_VENCP_STAT + offset;

	line_cnt = 0x1fff;
	line_cnt_previous = lcd_vcbus_getb(reg, 16, 13);
	while (i++ < LCD_WAIT_VSYNC_TIMEOUT) {
		line_cnt = lcd_vcbus_getb(reg, 16, 13);
		if (line_cnt < line_cnt_previous)
			break;
		line_cnt_previous = line_cnt;
		udelay(2);
	}
	/*LCDPR("line_cnt=%d, line_cnt_previous=%d, i=%d\n",
	 *	line_cnt, line_cnt_previous, i);
	 */
}

static unsigned int lcd_venc_get_max_lint_cnt(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset, reg, line_cnt;

	offset = pdrv->data->offset_venc[pdrv->index];
	reg = ENCL_VIDEO_MAX_LNCNT + offset;

	line_cnt = lcd_vcbus_read(reg) + 1;
	/*LCDPR("[%d]: %s: line_cnt=%d", pdrv->index, __func__, line_cnt); */

	return line_cnt;
}

#define LCD_ENC_TST_NUM_MAX    9
static char *lcd_enc_tst_str[] = {
	"0-None",        /* 0 */
	"1-Color Bar",   /* 1 */
	"2-Thin Line",   /* 2 */
	"3-Dot Grid",    /* 3 */
	"4-Gray",        /* 4 */
	"5-Red",         /* 5 */
	"6-Green",       /* 6 */
	"7-Blue",        /* 7 */
	"8-Black",       /* 8 */
};

static unsigned int lcd_enc_tst[][7] = {
/*tst_mode,    Y,       Cb,     Cr,     tst_en,  vfifo_en  rgbin*/
	{0,    0x200,   0x200,  0x200,   0,      1,        3},  /* 0 */
	{1,    0x200,   0x200,  0x200,   1,      0,        1},  /* 1 */
	{2,    0x200,   0x200,  0x200,   1,      0,        1},  /* 2 */
	{3,    0x200,   0x200,  0x200,   1,      0,        1},  /* 3 */
	{0,    0x1ff,   0x1ff,  0x1ff,   1,      0,        3},  /* 4 */
	{0,    0x3ff,     0x0,    0x0,   1,      0,        3},  /* 5 */
	{0,      0x0,   0x3ff,    0x0,   1,      0,        3},  /* 6 */
	{0,      0x0,     0x0,  0x3ff,   1,      0,        3},  /* 7 */
	{0,      0x0,     0x0,    0x0,   1,      0,        3},  /* 8 */
};

static int lcd_venc_debug_test(struct aml_lcd_drv_s *pdrv, unsigned int num)
{
	unsigned int start, width, offset;

	if (num >= LCD_ENC_TST_NUM_MAX)
		return -1;

	offset = pdrv->data->offset_venc[pdrv->index];
	start = pdrv->config.timing.hstart;
	width = pdrv->config.timing.act_timing.h_active / 9;

	lcd_venc_wait_vsync(pdrv);
	lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL + offset, lcd_enc_tst[num][6]);
	lcd_vcbus_write(ENCL_TST_MDSEL + offset, lcd_enc_tst[num][0]);
	lcd_vcbus_write(ENCL_TST_Y + offset, lcd_enc_tst[num][1]);
	lcd_vcbus_write(ENCL_TST_CB + offset, lcd_enc_tst[num][2]);
	lcd_vcbus_write(ENCL_TST_CR + offset, lcd_enc_tst[num][3]);
	lcd_vcbus_write(ENCL_TST_CLRBAR_STRT + offset, start);
	lcd_vcbus_write(ENCL_TST_CLRBAR_WIDTH + offset, width);
	lcd_vcbus_write(ENCL_TST_EN + offset, lcd_enc_tst[num][4]);
	lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV + offset, lcd_enc_tst[num][5], 3, 1);
	if (num > 0) {
		LCDPR("[%d]: show test pattern: %s\n",
		      pdrv->index, lcd_enc_tst_str[num]);
	}

	return 0;
}

static void lcd_venc_set_tcon(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int offset_if, offset_data;
	unsigned int reg_rgb_base, reg_rgb_coeff, reg_dith_ctrl, reg_pol_ctrl;
	unsigned int reg_de_hs, reg_de_he, reg_de_vs, reg_de_ve;
	unsigned int reg_hsync_hs, reg_hsync_he, reg_hsync_vs, reg_hsync_ve;
	unsigned int reg_vsync_hs, reg_vsync_he, reg_vsync_vs, reg_vsync_ve;

	offset_data = pdrv->data->offset_venc_data[pdrv->index];
	offset_if = pdrv->data->offset_venc_if[pdrv->index];
	reg_rgb_base = LCD_RGB_BASE_ADDR + offset_data;
	reg_rgb_coeff = LCD_RGB_COEFF_ADDR + offset_data;
	reg_dith_ctrl = LCD_DITH_CNTL_ADDR + offset_data;
	reg_pol_ctrl = LCD_POL_CNTL_ADDR + offset_data;
	reg_de_hs = DE_HS_ADDR + offset_if;
	reg_de_he = DE_HE_ADDR + offset_if;
	reg_de_vs = DE_VS_ADDR + offset_if;
	reg_de_ve = DE_VE_ADDR + offset_if;
	reg_hsync_hs = HSYNC_HS_ADDR + offset_if;
	reg_hsync_he = HSYNC_HE_ADDR + offset_if;
	reg_hsync_vs = HSYNC_VS_ADDR + offset_if;
	reg_hsync_ve = HSYNC_VE_ADDR + offset_if;
	reg_vsync_hs = VSYNC_HS_ADDR + offset_if;
	reg_vsync_he = VSYNC_HE_ADDR + offset_if;
	reg_vsync_vs = VSYNC_VS_ADDR + offset_if;
	reg_vsync_ve = VSYNC_VE_ADDR + offset_if;

	lcd_vcbus_write(reg_rgb_base, 0x0);
	lcd_vcbus_write(reg_rgb_coeff, 0x400);

	if (pconf->basic.lcd_type != LCD_P2P &&
		pconf->basic.lcd_type != LCD_MLVDS) {
		switch (pconf->basic.lcd_bits) {
		case 6:
			lcd_vcbus_write(reg_dith_ctrl,  0x600);
			break;
		case 8:
			lcd_vcbus_write(reg_dith_ctrl,  0x400);
			break;
		case 10:
		default:
			lcd_vcbus_write(reg_dith_ctrl,  0x0);
			break;
		}
	} else {
		lcd_vcbus_write(reg_dith_ctrl,  0x0);
	}

	switch (pconf->basic.lcd_type) {
	case LCD_LVDS:
		lcd_vcbus_setb(reg_pol_ctrl, 1, 0, 3);
		// refs to lcd_lvds.c@lcd_lvds_enable
		if (pconf->timing.act_timing.vsync_pol == pconf->timing.act_timing.hsync_pol)
			lcd_vcbus_setb(reg_pol_ctrl, 1, 1, 1);
		break;
	case LCD_VBYONE:
		if (pconf->timing.act_timing.hsync_pol)
			lcd_vcbus_setb(reg_pol_ctrl, 1, 0, 1);
		if (pconf->timing.act_timing.vsync_pol)
			lcd_vcbus_setb(reg_pol_ctrl, 1, 1, 1);
		break;
	case LCD_MIPI:
		//lcd_vcbus_setb(reg_pol_ctrl, 0x3, 0, 2);
		/*lcd_vcbus_write(reg_pol_ctrl,
		 *	(lcd_vcbus_read(reg_pol_ctrl) |
		 *	 ((0 << 2) | (vs_pol_adj << 1) | (hs_pol_adj << 0))));
		 */
		/*lcd_vcbus_write(reg_pol_ctrl, (lcd_vcbus_read(reg_pol_ctrl) |
		 *	 ((1 << LCD_TCON_DE_SEL) | (1 << LCD_TCON_VS_SEL) |
		 *	  (1 << LCD_TCON_HS_SEL))));
		 */
		break;
	case LCD_EDP:
		lcd_vcbus_setb(reg_pol_ctrl, 1, 0, 1);
		break;
	default:
		break;
	}

	/* DE signal */
	lcd_vcbus_write(reg_de_hs,    pconf->timing.de_hs_addr);
	lcd_vcbus_write(reg_de_he,    pconf->timing.de_he_addr);
	lcd_vcbus_write(reg_de_vs,    pconf->timing.de_vs_addr);
	lcd_vcbus_write(reg_de_ve,    pconf->timing.de_ve_addr);

	/* Hsync signal */
	lcd_vcbus_write(reg_hsync_hs, pconf->timing.hs_hs_addr);
	lcd_vcbus_write(reg_hsync_he, pconf->timing.hs_he_addr);
	lcd_vcbus_write(reg_hsync_vs, pconf->timing.hs_vs_addr);
	lcd_vcbus_write(reg_hsync_ve, pconf->timing.hs_ve_addr);

	/* Vsync signal */
	lcd_vcbus_write(reg_vsync_hs, pconf->timing.vs_hs_addr);
	lcd_vcbus_write(reg_vsync_he, pconf->timing.vs_he_addr);
	lcd_vcbus_write(reg_vsync_vs, pconf->timing.vs_vs_addr);
	lcd_vcbus_write(reg_vsync_ve, pconf->timing.vs_ve_addr);
}

static void lcd_venc_set_timing(struct aml_lcd_drv_s *pdrv)
{
	struct lcd_config_s *pconf = &pdrv->config;
	unsigned int hstart, hend, vstart, vend;
	unsigned int offset;
	unsigned int pre_vde, pre_de_vs, pre_de_ve, pre_de_hs, pre_de_he;

	hstart = pconf->timing.hstart;
	hend = pconf->timing.hend;
	vstart = pconf->timing.vstart;
	vend = pconf->timing.vend;
	offset = pdrv->data->offset_venc[pdrv->index];

	lcd_vcbus_write(ENCL_VIDEO_MAX_PXCNT + offset, pconf->timing.act_timing.h_period - 1);
	lcd_vcbus_write(ENCL_VIDEO_MAX_LNCNT + offset, pconf->timing.act_timing.v_period - 1);
	lcd_vcbus_write(ENCL_VIDEO_HAVON_BEGIN + offset, hstart);
	lcd_vcbus_write(ENCL_VIDEO_HAVON_END + offset,   hend);
	lcd_vcbus_write(ENCL_VIDEO_VAVON_BLINE + offset, vstart);
	lcd_vcbus_write(ENCL_VIDEO_VAVON_ELINE + offset, vend);
	if (pconf->basic.lcd_type == LCD_P2P ||
	    pconf->basic.lcd_type == LCD_MLVDS) {
		pre_vde = pconf->timing.pre_de_v ? pconf->timing.pre_de_v : 8;
		pre_de_vs = vstart - pre_vde;
		pre_de_ve = pconf->timing.act_timing.v_active + pre_de_vs;
		pre_de_hs = hstart + PRE_DE_DELAY;
		pre_de_he = pconf->timing.act_timing.h_active - 1 + pre_de_hs;
		lcd_vcbus_write(ENCL_VIDEO_V_PRE_DE_BLINE + offset, pre_de_vs);
		lcd_vcbus_write(ENCL_VIDEO_V_PRE_DE_ELINE + offset, pre_de_ve);
		lcd_vcbus_write(ENCL_VIDEO_H_PRE_DE_BEGIN + offset, pre_de_hs);
		lcd_vcbus_write(ENCL_VIDEO_H_PRE_DE_END + offset,   pre_de_he);
	}

	lcd_vcbus_write(ENCL_VIDEO_HSO_BEGIN + offset, pconf->timing.hs_hs_addr);
	lcd_vcbus_write(ENCL_VIDEO_HSO_END + offset,   pconf->timing.hs_he_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_BEGIN + offset, pconf->timing.vs_hs_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_END + offset,   pconf->timing.vs_he_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_BLINE + offset, pconf->timing.vs_vs_addr);
	lcd_vcbus_write(ENCL_VIDEO_VSO_ELINE + offset, pconf->timing.vs_ve_addr);

	switch (pdrv->data->chip_type) {
	case LCD_CHIP_T7:
		lcd_vcbus_write(ENCL_INBUF_CNTL1 + offset,
				(5 << 13) | (pconf->timing.act_timing.h_active - 1));
		lcd_vcbus_write(ENCL_INBUF_CNTL0 + offset, 0x200);
		break;
	case LCD_CHIP_T3:
	case LCD_CHIP_T5W:
	case LCD_CHIP_T5M:
		lcd_vcbus_write(ENCL_INBUF_CNTL1 + offset,
				(4 << 13) | (pconf->timing.act_timing.h_active - 1));
		lcd_vcbus_write(ENCL_INBUF_CNTL0 + offset, 0x200);
		break;
	default:
		break;
	}

	lcd_venc_set_tcon(pdrv);
}

static void lcd_venc_set(struct aml_lcd_drv_s *pdrv)
{
	unsigned int reg_disp_viu_ctrl, offset;

	offset = pdrv->data->offset_venc[pdrv->index];

	lcd_vcbus_write(ENCL_VIDEO_EN + offset, 0);

	lcd_vcbus_write(ENCL_VIDEO_MODE + offset, 0x8000); /* bit[15] shadown en */
	lcd_vcbus_write(ENCL_VIDEO_MODE_ADV + offset, 0x0418); /* Sampling rate: 1 */
	lcd_vcbus_write(ENCL_VIDEO_FILT_CTRL + offset, 0x1000); /* bypass filter */

	lcd_venc_set_timing(pdrv);

	lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL + offset, 3);

	lcd_vcbus_write(ENCL_VIDEO_EN + offset, 1);

	switch (pdrv->index) {
	case 0:
		reg_disp_viu_ctrl = VPU_DISP_VIU0_CTRL;
		break;
	case 1:
		reg_disp_viu_ctrl = VPU_DISP_VIU1_CTRL;
		break;
	case 2:
		reg_disp_viu_ctrl = VPU_DISP_VIU2_CTRL;
		break;
	default:
		LCDERR("[%d]: %s: invalid drv_index\n",
			pdrv->index, __func__);
		return;
	}

	/*
	 * bit31: lvds enable
	 * bit30: vx1 enable
	 * bit29: hdmitx enable
	 * bit28: dsi_edp enable
	 */
	switch (pdrv->config.basic.lcd_type) {
	case LCD_LVDS:
		lcd_vcbus_write(reg_disp_viu_ctrl, (1 << 31) |
						(0 << 30) |
						(0 << 29) |
						(0 << 28));
		break;
	case LCD_VBYONE:
		lcd_vcbus_write(reg_disp_viu_ctrl, (0 << 31) |
						(1 << 30) |
						(0 << 29) |
						(0 << 28));
		break;
	case LCD_MIPI:
	case LCD_EDP:
		lcd_vcbus_write(reg_disp_viu_ctrl, (0 << 31) |
						(0 << 30) |
						(0 << 29) |
						(1 << 28));
		break;
	default:
		break;
	}
	lcd_vcbus_write(VPU_VENC_CTRL + offset, 2);
}

static void lcd_venc_enable_ctrl(struct aml_lcd_drv_s *pdrv, int flag)
{
	unsigned int offset;

	offset = pdrv->data->offset_venc[pdrv->index];
	if (flag)
		lcd_vcbus_write(ENCL_VIDEO_EN + offset, 1);
	else
		lcd_vcbus_write(ENCL_VIDEO_EN + offset, 0);
}

static void lcd_venc_mute_set(struct aml_lcd_drv_s *pdrv, unsigned char flag)
{
	unsigned int offset;

	offset = pdrv->data->offset_venc[pdrv->index];

	lcd_venc_wait_vsync(pdrv);
	if (flag) {
		lcd_vcbus_write(ENCL_VIDEO_RGBIN_CTRL + offset, 3);
		lcd_vcbus_write(ENCL_TST_MDSEL + offset, 0);
		lcd_vcbus_write(ENCL_TST_Y + offset, 0);
		lcd_vcbus_write(ENCL_TST_CB + offset, 0);
		lcd_vcbus_write(ENCL_TST_CR + offset, 0);
		lcd_vcbus_write(ENCL_TST_EN + offset, 1);
		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV + offset, 0, 3, 1);
	} else {
		lcd_vcbus_setb(ENCL_VIDEO_MODE_ADV + offset, 1, 3, 1);
		lcd_vcbus_write(ENCL_TST_EN + offset, 0);
	}
}

static unsigned int lcd_venc_get_encl_line_cnt(struct aml_lcd_drv_s *pdrv)
{
	unsigned int offset, cnt;

	if (!pdrv)
		return 0;

	offset = pdrv->data->offset_venc[pdrv->index];

	cnt = lcd_vcbus_getb(VPU_VENCP_STAT + offset, 16, 13);
	return cnt;
}

int lcd_venc_op_init_t7(struct lcd_venc_op_s *venc_op)
{
	if (!venc_op)
		return -1;

	venc_op->wait_vsync = lcd_venc_wait_vsync;
	venc_op->get_max_lcnt = lcd_venc_get_max_lint_cnt;
	venc_op->venc_debug_test = lcd_venc_debug_test;
	venc_op->venc_set_timing = lcd_venc_set_timing;
	venc_op->venc_set = lcd_venc_set;
	venc_op->venc_enable = lcd_venc_enable_ctrl;
	venc_op->mute_set = lcd_venc_mute_set;
	venc_op->get_encl_line_cnt = lcd_venc_get_encl_line_cnt;

	return 0;
};

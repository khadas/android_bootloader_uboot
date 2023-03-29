// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "../lcd_reg.h"
#include "lcd_phy_config.h"
#include "../lcd_common.h"

static struct lcd_phy_ctrl_s *phy_ctrl_p;

static unsigned int p2p_low_common_phy_ch_tl1 = 0x000b000b;
static unsigned int lvds_vx1_p2p_phy_ch_tl1 = 0x00020002;

static void lcd_phy_cntl14_update(struct phy_config_s *phy, unsigned int cntl14)
{
	/* vcm */
	if ((phy->flag & (1 << 1))) {
		cntl14 &= ~(0x7ff << 4);
		cntl14 |= (phy->vcm & 0x7ff) << 4;
	}
	/* ref bias switch */
	if ((phy->flag & (1 << 2))) {
		cntl14 &= ~(1 << 15);
		cntl14 |= (phy->ref_bias & 0x1) << 15;
	}
	/* odt */
	if ((phy->flag & (1 << 3))) {
		cntl14 &= ~(0xff << 24);
		cntl14 |= (phy->odt & 0xff) << 24;
	}
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, cntl14);
}

/*
 *    chreg: channel ctrl
 *    bypass: 1=bypass
 *    mode: 1=normal mode, 0=low common mode
 *    ckdi: clk phase for minilvds
 */
static void lcd_phy_cntl_set(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy, int status,
				int bypass, unsigned int mode, unsigned int ckdi)
{
	unsigned int cntl15 = 0, cntl16 = 0;
	unsigned int data = 0, chreg = 0;
	unsigned int tmp = 0;

	if (!phy_ctrl_p)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	if (status) {
		chreg |= ((phy_ctrl_p->ctrl_bit_on << 16) |
			  (phy_ctrl_p->ctrl_bit_on << 0));
		if (pdrv->data->chip_type == LCD_CHIP_T5W)
			tmp |= ((0x7 << 19) | (0x7 << 3));

		if (bypass)
			tmp |= ((1 << 18) | (1 << 2));
		if (mode) {
			chreg |= lvds_vx1_p2p_phy_ch_tl1;
			cntl15 = 0x00070000;
		} else {
			chreg |= p2p_low_common_phy_ch_tl1;
			if (phy->weakly_pull_down)
				chreg &= ~((1 << 19) | (1 << 3));
			cntl15 = 0x000e0000;
		}
		cntl16 = ckdi | 0x80000000;
	} else {
		if (phy_ctrl_p->ctrl_bit_on)
			data = 0;
		else
			data = 1;
		chreg |= ((data << 16) | (data << 0));
		cntl15 = 0;
		cntl16 = 0;
		lcd_ana_write(HHI_DIF_CSI_PHY_CNTL14, 0);
	}

	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL15, cntl15);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL16, cntl16);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL8, tmp);
	data = ((phy->lane[0].preem & 0xff) << 8 |
	       (phy->lane[1].preem & 0xff) << 24);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL1, chreg | data);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL9, tmp);
	data = ((phy->lane[2].preem & 0xff) << 8 |
	       (phy->lane[3].preem & 0xff) << 24);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL2, chreg | data);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL10, tmp);
	data = ((phy->lane[4].preem & 0xff) << 8 |
	       (phy->lane[5].preem & 0xff) << 24);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL3, chreg | data);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL11, tmp);
	data = ((phy->lane[6].preem & 0xff) << 8 |
	       (phy->lane[7].preem & 0xff) << 24);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL4, chreg | data);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL12, tmp);
	data = ((phy->lane[8].preem & 0xff) << 8 |
	       (phy->lane[9].preem & 0xff) << 24);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL6, chreg | data);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL13, tmp);
	data = ((phy->lane[10].preem & 0xff) << 8 |
	       (phy->lane[11].preem & 0xff) << 24);
	lcd_ana_write(HHI_DIF_CSI_PHY_CNTL7, chreg | data);
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	if (status) {
		if (lcd_debug_print_flag)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);

		cntl14 = 0xff2027e0 | phy->vswing;
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, 0, 1, 0);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, 0, 0, 0);
	}
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("vswing_level=0x%x, ext_pullup=%d\n",
			      phy->vswing_level, phy->ext_pullup);
		}

		if (phy->ext_pullup)
			cntl14 = 0xff2027e0 | phy->vswing;
		else
			cntl14 = 0xf02027a0 | phy->vswing;
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, 1, 1, 0);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, 1, 0, 0);
	}
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct mlvds_config_s *mlvds_conf;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int ckdi, cntl14 = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	mlvds_conf = &pdrv->config.control.mlvds_cfg;
	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);

		cntl14 = 0xff2027e0 | phy->vswing;
		ckdi = (mlvds_conf->pi_clk_sel << 12);
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, 0, 1, ckdi);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, 0, 0, 0);
	}
}

static void lcd_p2p_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int p2p_type, vcm_flag;
	struct p2p_config_s *p2p_conf;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s: %d\n", __func__, status);

	p2p_conf = &pdrv->config.control.p2p_cfg;
	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);
		p2p_type = p2p_conf->p2p_type & 0x1f;
		vcm_flag = (p2p_conf->p2p_type >> 5) & 0x1;

		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			cntl14 = 0xff2027a0 | phy->vswing;
			lcd_phy_cntl14_update(phy, cntl14);
			lcd_phy_cntl_set(pdrv, phy, status, 1, 1, 0);
			break;
		case P2P_CHPI: /* low common mode */
		case P2P_CSPI:
		case P2P_USIT:
			if (p2p_type == P2P_CHPI)
				phy->weakly_pull_down = 1;
			if (vcm_flag) /* 580mV */
				cntl14 = 0xe0600272;
			else /* default 385mV */
				cntl14 = 0xfe60027f;
			lcd_phy_cntl14_update(phy, cntl14);
			lcd_phy_cntl_set(pdrv, phy, status, 1, 0, 0);
			break;
		default:
			LCDERR("%s: invalid p2p_type %d\n", __func__, p2p_type);
			break;
		}
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, 1, 0, 0);
	}
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t5 = {
	.lane_lock = 0,
	.ctrl_bit_on = 1,
	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_value_dft,
	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = lcd_p2p_phy_set,
	.phy_set_mipi = NULL,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t5(struct aml_lcd_data_s *pdata)
{
	phy_ctrl_p = &lcd_phy_ctrl_t5;
	return phy_ctrl_p;
}
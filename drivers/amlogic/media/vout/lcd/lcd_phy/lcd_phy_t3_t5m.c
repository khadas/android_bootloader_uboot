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
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, cntl14);
}

/*
 *    chreg: channel ctrl
 *    bypass: 1=bypass
 *    mode: 1=normal mode, 0=low common mode
 *    ckdi: clk phase for minilvds
 */
static void lcd_phy_cntl_set(struct aml_lcd_drv_s *pdrv, struct phy_config_s *phy, int status,
			unsigned int flag, int bypass, unsigned int mode, unsigned int ckdi)
{
	unsigned int cntl15 = 0, cntl16 = 0;
	unsigned int chreg, reg_data = 0, chdig = 0;
	unsigned char i, bit;

	uint32_t chreg_reg[6] = {
		ANACTRL_DIF_PHY_CNTL1, ANACTRL_DIF_PHY_CNTL2,
		ANACTRL_DIF_PHY_CNTL3, ANACTRL_DIF_PHY_CNTL4,
		ANACTRL_DIF_PHY_CNTL6, ANACTRL_DIF_PHY_CNTL7,
	};
	uint32_t chdig_reg[6] = {
		ANACTRL_DIF_PHY_CNTL8, ANACTRL_DIF_PHY_CNTL9,
		ANACTRL_DIF_PHY_CNTL10, ANACTRL_DIF_PHY_CNTL11,
		ANACTRL_DIF_PHY_CNTL12, ANACTRL_DIF_PHY_CNTL13,
	};

	if (!phy_ctrl_p)
		return;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: %d, bypass:%d\n", __func__, status, bypass);

	if (status) {
		reg_data = phy_ctrl_p->ctrl_bit_on << 0;

		if (mode) {
			reg_data |= 0x0002;
			cntl15 = 0x00070000;
		} else {
			reg_data |= 0x000b;
			if (phy->weakly_pull_down)
				reg_data &= ~(1 << 3);
			cntl15 = 0x000e0000;
		}
		cntl16 = (ckdi << 12) | 0x80000000;
	} else {
		reg_data = (!phy_ctrl_p->ctrl_bit_on) << 0;
		lcd_ana_write(ANACTRL_DIF_PHY_CNTL14, 0);
	}

	lcd_ana_write(ANACTRL_DIF_PHY_CNTL15, cntl15);
	lcd_ana_write(ANACTRL_DIF_PHY_CNTL16, cntl16);

	for (i = 0; i < 12; i++) {
		if (flag & (1 << i)) {
			bit = i & 0x1 ? 16 : 0;
			chreg = reg_data;
			chdig = (ckdi & (0x1 << i)) && bypass ? 0 : 0x4;
			if (status) {
				chreg |= (phy->lane[i].preem & 0xff) << 8;
				chdig |= (phy->lane[i].amp & 0x7) << 3;
			}
			lcd_ana_setb(chreg_reg[i >> 1], chreg, bit, 16);
			lcd_ana_setb(chdig_reg[i >> 1], chdig, bit, 16);
		}
	}
}

static void lcd_lvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0, flag;
	unsigned short lvds_flag_6lane_map0[2][3] = {
		{0x000f, 0x001f, 0x003f}, {0x03c0, 0x07c0, 0x0fc0}};
	unsigned char bit_idx;

	if (pdrv->config.basic.lcd_bits == 6)
		bit_idx = 0;
	else if (pdrv->config.basic.lcd_bits == 8)
		bit_idx = 1;
	else //pdrv->config.basic.lcd_bits == 10
		bit_idx = 2;

	if (pdrv->config.control.lvds_cfg.dual_port) {
		flag = lvds_flag_6lane_map0[0][bit_idx] | lvds_flag_6lane_map0[1][bit_idx];
	} else {
		if (pdrv->config.control.lvds_cfg.port_swap)
			flag = lvds_flag_6lane_map0[1][bit_idx];
		else
			flag = lvds_flag_6lane_map0[0][bit_idx];
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d, flag=0x%04x\n", pdrv->index, __func__, status, flag);

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);

		cntl14 = 0xff2027e0 | phy->vswing;
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 1, 0);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 0, 0);
	}
}

static void lcd_vbyone_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
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
		lcd_phy_cntl_set(pdrv, phy, status, 0xff, 1, 1, 0);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, 0xff, 1, 0, 0);
	}
}

static void lcd_mlvds_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	struct mlvds_config_s *mlvds_conf;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14 = 0, flag = 0;
	uint8_t i;
	uint64_t channel_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: %d\n", __func__, status);

	mlvds_conf = &pdrv->config.control.mlvds_cfg;
	channel_sel = mlvds_conf->channel_sel1;
	channel_sel = channel_sel << 32 | mlvds_conf->channel_sel0;
	for (i = 0; i < 12; i++)
		flag |= ((channel_sel >> (4 * i)) & 0xf) == 0xf ? 0 : 1 << i;

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);

		cntl14 = 0xff2027e0 | phy->vswing;
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 1, mlvds_conf->pi_clk_sel);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 0, 0);
	}
}

static void lcd_p2p_phy_set(struct aml_lcd_drv_s *pdrv, int status)
{
	unsigned int p2p_type, vcm_flag;
	struct p2p_config_s *p2p_conf;
	struct phy_config_s *phy = &pdrv->config.phy_cfg;
	unsigned int cntl14, flag = 0;
	uint8_t i, mode = 1;
	uint64_t channel_sel;

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
		LCDPR("%s: %d\n", __func__, status);

	p2p_conf = &pdrv->config.control.p2p_cfg;
	channel_sel = p2p_conf->channel_sel1;
	channel_sel = channel_sel << 32 | p2p_conf->channel_sel0;
	for (i = 0; i < 12; i++)
		flag |= ((channel_sel >> (4 * i)) & 0xf) == 0xf ? 0 : 1 << i;

	if (status) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("vswing_level=0x%x\n", phy->vswing_level);
		p2p_type = p2p_conf->p2p_type & 0x1f;
		vcm_flag = (p2p_conf->p2p_type >> 5) & 0x1;

		if (lcd_debug_print_flag & LCD_DBG_PR_ADV)
			printf("%s: phy->flag=0x%x\n", __func__, phy->flag);

		switch (p2p_type) {
		case P2P_CEDS:
		case P2P_CMPI:
		case P2P_ISP:
		case P2P_EPI:
			cntl14 = 0xff2027a0 | phy->vswing;
			mode = 1;
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
			/* vswing */
			cntl14 &= ~(0xf);
			cntl14 |= phy->vswing;

			mode = 0;
			break;
		default:
			LCDERR("%s: invalid p2p_type %d\n", __func__, p2p_type);
			return;
		}
		lcd_phy_cntl14_update(phy, cntl14);
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, mode, 0);
	} else {
		lcd_phy_cntl_set(pdrv, phy, status, flag, 1, 0, 0);
	}
}

static unsigned int lcd_phy_amp_dft_t5m(struct aml_lcd_drv_s *pdrv)
{
	unsigned int amp_value = 0;

	if (pdrv->data->chip_type == LCD_CHIP_T5M)
		amp_value = 0x7;

	return amp_value;
}

static struct lcd_phy_ctrl_s lcd_phy_ctrl_t3_t5m = {
	.lane_lock = 0,
	.ctrl_bit_on = 1,
	.phy_vswing_level_to_val = lcd_phy_vswing_level_to_value_dft,
	.phy_amp_dft_val = lcd_phy_amp_dft_t5m,
	.phy_preem_level_to_val = lcd_phy_preem_level_to_value_legacy,
	.phy_set_lvds = lcd_lvds_phy_set,
	.phy_set_vx1 = lcd_vbyone_phy_set,
	.phy_set_mlvds = lcd_mlvds_phy_set,
	.phy_set_p2p = lcd_p2p_phy_set,
	.phy_set_mipi = NULL,
	.phy_set_edp = NULL,
};

struct lcd_phy_ctrl_s *lcd_phy_config_init_t3_t5m(struct aml_lcd_data_s *pdata)
{
	phy_ctrl_p = &lcd_phy_ctrl_t3_t5m;
	return phy_ctrl_p;
}


/*
 * arch/arm/cpu/armv8/gxl/hdmitx20/enc_clk_config.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <common.h>
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#include "hdmitx_clk.h"

static uint32_t frac_rate;

#define msleep(i) udelay(i*1000)

#define check_clk_config(para)\
	if (para == -1)\
		return;

#define check_div() \
	if (div == -1)\
		return ;\
	switch (div) {\
	case 1:\
		div = 0; break;\
	case 2:\
		div = 1; break;\
	case 4:\
		div = 2; break;\
	case 6:\
		div = 3; break;\
	case 12:\
		div = 4; break;\
	default:\
		break;\
	}

#define WAIT_FOR_PLL_LOCKED(_reg) \
	do { \
		unsigned int st = 0; \
		int cnt = 10; \
		unsigned int reg = _reg; \
		while (cnt--) { \
			msleep(5); \
			st = (((hd_read_reg(reg) >> 30) & 0x3) == 3); \
			if (st) \
				break; \
			else { \
				/* reset hpll */ \
				hd_set_reg_bits(reg, 1, 29, 1); \
				hd_set_reg_bits(reg, 0, 29, 1); \
			} \
		} \
		if (cnt < 9) \
			printf("pll[0x%x] reset %d times\n", reg, 9 - cnt);\
	} while (0)

static void set_hdmitx_sys_clk(void)
{
	hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, 0, 9, 3);
	hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, 0, 0, 7);
	hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, 1, 8, 1);
}

/*
 * When VCO outputs 6.0 GHz, if VCO unlock with default v1
 * steps, then need reset with v2 or v3
 */
static bool set_hpll_hclk_v1(unsigned int m, unsigned int frac_val)
{
	int ret = 0;
	struct hdmitx_dev *hdev = hdmitx_get_hdev();

	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x0b3a0400 | (m & 0xff));
	hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x3, 28, 2);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, frac_val);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);

	if (frac_val == 0x8148) {
		if (((hdev->para->vic == HDMI_3840x2160p50_16x9) ||
		     (hdev->para->vic == HDMI_3840x2160p60_16x9) ||
		     (hdev->para->vic == HDMI_3840x2160p50_64x27) ||
		     (hdev->para->vic == HDMI_3840x2160p60_64x27)) &&
		     (hdev->para->cs != HDMI_COLOR_FORMAT_420)) {
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x11551293);
		} else {
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x44331290);
		}
	} else {
		if (hdmitx_find_vendor(hdev) &&
		    ((hdev->para->vic == HDMI_3840x2160p50_16x9) ||
		    (hdev->para->vic == HDMI_3840x2160p60_16x9) ||
		    (hdev->para->vic == HDMI_3840x2160p50_64x27) ||
		    (hdev->para->vic == HDMI_3840x2160p60_64x27) ||
		    (hdev->para->vic == HDMI_4096x2160p50_256x135) ||
		    (hdev->para->vic == HDMI_4096x2160p60_256x135)) &&
		    (hdev->para->cs != HDMI_COLOR_FORMAT_420)) {
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x11551293);
		} else {
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a68dc00);
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x65771290);
		}
	}
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39272000);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x56540000);
	hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);

	ret = (((hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static bool set_hpll_hclk_v2(unsigned int m, unsigned int frac_val)
{
	int ret = 0;

	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x0b3a0400 | (m & 0xff));
	hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x3, 28, 2);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, frac_val);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0xea68dc00);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x65771290);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39272000);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x56540000);
	hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);

	ret = (((hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static bool set_hpll_hclk_v3(unsigned int m, unsigned int frac_val)
{
	int ret = 0;

	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x0b3a0400 | (m & 0xff));
	hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x3, 28, 2);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, frac_val);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0xea68dc00);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x65771290);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39272000);
	hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x55540000);
	hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);

	ret = (((hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

void set_hpll_clk_out(unsigned int clk)
{
	pr_info("config HPLL = %d frac_rate = %d\n", clk, frac_rate);
	switch (clk) {
	case 5940000:
		if (set_hpll_hclk_v1(0xf7, frac_rate ? 0x8148 : 0x10000))
			break;
		if (set_hpll_hclk_v2(0x7b, 0x18000))
			break;
		if (set_hpll_hclk_v3(0xf7, 0x10000))
			break;
		break;
	case 5850000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004f3);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00018000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5680000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004ec);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00015555);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5600000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004e9);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x0000aaab);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);/*test*/
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5405400:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004e1);
		if (frac_rate)
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00007333);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5200000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004d8);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00015555);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4897000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004cc);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x0000d560);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x43231290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x29272000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x56540028);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4455000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004b9);
		if (frac_rate)
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x0000e10e);
		else
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00014000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x43231290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x29272000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x56540028);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4324320:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004b4);
		if (frac_rate)
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00005c29);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4320000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004b4);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4260000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b0004b1);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00010000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3712500:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b00049a);
		if (frac_rate)
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x000110e1);
		else
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00016000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x43231290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x29272000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x56540028);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3450000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b00048f);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00018000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3420000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b00048e);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00010000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3243240:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b000487);
		if (frac_rate)
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x0000451f);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3200000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b000485);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x0000aaab);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3197500:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b000485);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00007555);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3180000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b000484);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00010000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	case 2970000:
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL0, 0x3b00047b);
		if (frac_rate)
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x000140b4);
		else
			hd_write_reg(P_ANACTRL_HDMIPLL_CTRL1, 0x00018000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL3, 0x0a691c00);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL5, 0x39270000);
		hd_write_reg(P_ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(P_ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd_read_reg(P_ANACTRL_HDMIPLL_CTRL0));
		break;
	default:
		printf("error hpll clk: %d\n", clk);
		break;
	}
}

static void set_hpll_sspll(struct hdmitx_dev *hdev)
{
	enum hdmi_vic vic = hdev->vic;

	switch (vic) {
	case HDMI_1920x1080p60_16x9:
	case HDMI_1920x1080p50_16x9:
	case HDMI_1280x720p60_16x9:
	case HDMI_1280x720p50_16x9:
	case HDMI_1920x1080i60_16x9:
	case HDMI_1920x1080i50_16x9:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 1, 29, 1);
		/* bit[22:20] hdmi_dpll_fref_sel
		 * bit[8] hdmi_dpll_ssc_en
		 * bit[7:4] hdmi_dpll_ssc_dep_sel
		 */
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL2, 1, 20, 3);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL2, 1, 8, 1);
		/* 2: 1000ppm  1: 500ppm */
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL2, 2, 4, 4);
		if (hdev->dongle_mode)
			hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL2, 4, 4, 4);
		/* bit[15] hdmi_dpll_sdmnc_en */
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL3, 0, 15, 1);
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0, 29, 1);
		break;
	default:
		break;
	}
}

static void set_hpll_od1(unsigned div)
{
	switch (div) {
	case 1:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0, 16, 2);
		break;
	case 2:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 1, 16, 2);
		break;
	case 4:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 2, 16, 2);
		break;
	default:
		break;
	}
}

static void set_hpll_od2(unsigned div)
{
	switch (div) {
	case 1:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0, 18, 2);
		break;
	case 2:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 1, 18, 2);
		break;
	case 4:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 2, 18, 2);
		break;
	default:
		break;
	}
}

static void set_hpll_od3(unsigned div)
{
	switch (div) {
	case 1:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 0, 20, 2);
		break;
	case 2:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 1, 20, 2);
		break;
	case 4:
		hd_set_reg_bits(P_ANACTRL_HDMIPLL_CTRL0, 2, 20, 2);
		break;
	default:
		printf("Err %s[%d]\n", __func__, __LINE__);
		break;
	}
}

// --------------------------------------------------
//              clocks_set_vid_clk_div
// --------------------------------------------------
// wire            clk_final_en    = control[19];
// wire            clk_div1        = control[18];
// wire    [1:0]   clk_sel         = control[17:16];
// wire            set_preset      = control[15];
// wire    [14:0]  shift_preset    = control[14:0];
static void set_hpll_od3_clk_div(int div_sel)
{
	int shift_val = 0;
	int shift_sel = 0;

	/* When div 6.25, need to reset vid_pll_div */
	if (div_sel == VID_PLL_DIV_6p25) {
		msleep(1);
		hd_write_reg(P_RESETCTRL_RESET0, 1 << 19);
	}
	// Disable the output clock
	hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 18, 2);
	hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 15, 1);

	switch (div_sel) {
	case VID_PLL_DIV_1:      shift_val = 0xFFFF; shift_sel = 0; break;
	case VID_PLL_DIV_2:      shift_val = 0x0aaa; shift_sel = 0; break;
	case VID_PLL_DIV_3:      shift_val = 0x0db6; shift_sel = 0; break;
	case VID_PLL_DIV_3p5:    shift_val = 0x36cc; shift_sel = 1; break;
	case VID_PLL_DIV_3p75:   shift_val = 0x6666; shift_sel = 2; break;
	case VID_PLL_DIV_4:      shift_val = 0x0ccc; shift_sel = 0; break;
	case VID_PLL_DIV_5:      shift_val = 0x739c; shift_sel = 2; break;
	case VID_PLL_DIV_6:      shift_val = 0x0e38; shift_sel = 0; break;
	case VID_PLL_DIV_6p25:   shift_val = 0x0000; shift_sel = 3; break;
	case VID_PLL_DIV_7:      shift_val = 0x3c78; shift_sel = 1; break;
	case VID_PLL_DIV_7p5:    shift_val = 0x78f0; shift_sel = 2; break;
	case VID_PLL_DIV_12:     shift_val = 0x0fc0; shift_sel = 0; break;
	case VID_PLL_DIV_14:     shift_val = 0x3f80; shift_sel = 1; break;
	case VID_PLL_DIV_15:     shift_val = 0x7f80; shift_sel = 2; break;
	case VID_PLL_DIV_2p5:    shift_val = 0x5294; shift_sel = 2; break;
	case VID_PLL_DIV_3p25:   shift_val = 0x66cc; shift_sel = 2; break;
	default:
		debug("Error: clocks_set_vid_clk_div:  Invalid parameter\n");
		break;
	}

	if (shift_val == 0xffff ) {      // if divide by 1
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 1, 18, 1);
	} else {
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 18, 1);
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 16, 2);
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 15, 1);
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 0, 15);

		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, shift_sel, 16, 2);
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 1, 15, 1);
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, shift_val, 0, 15);
		hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 0, 15, 1);
	}
	// Enable the final output clock
	hd_set_reg_bits(P_CLKCTRL_VID_PLL_CLK_DIV, 1, 19, 1);
}

static void set_vid_clk_div(unsigned div)
{
	check_clk_config(div);
	if (div == 0)
		div = 1;
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 0, 16, 3);   // select vid_pll_clk
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_DIV, div-1, 0, 8);
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 7, 0, 3);
}

static void set_hdmi_tx_pixel_div(unsigned div)
{
	check_div();
	hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, div, 16, 4);
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 5, 1);   //enable gate
}

static void set_encp_div(unsigned div)
{
	check_div();
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_DIV, div, 24, 4);
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 2, 1);   //enable gate
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 1, 19, 1);
}

static void set_enci_div(unsigned div)
{
	check_div();
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_DIV, div, 28, 4);
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 0, 1);   //enable gate
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL, 1, 19, 1);
}

/* mode hpll_clk_out od1 od2(PHY) od3
 * vid_pll_div vid_clk_div hdmi_tx_pixel_div encp_div enci_div
 */
/* For colordepth 8bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_24[] = {
	{{HDMI_720x480i60_16x9,
	  HDMI_720x576i50_16x9,
	  GROUP_END},
		1, VIU_ENCI, 4324320, 4, 4, 1, VID_PLL_DIV_5, 1, 2, -1, 2},
	{{HDMI_720x576p50_16x9,
	  HDMI_720x480p60_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4324320, 4, 4, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_720x576p100_16x9,
	  HDMI_720x480p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4324320, 4, 2, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 4, 2, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1920x1080i60_16x9,
	  HDMI_1920x1080i50_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 4, 2, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1920x1080i100_16x9,
	  HDMI_1920x1080i120_16x9,
	  HDMI_1280x720p100_16x9,
	  HDMI_1280x720p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 4, 1, 1, VID_PLL_DIV_5, 1, 2, 2, -1},
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 4, 1, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 4, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_3840x2160p30_16x9,
	  HDMI_3840x2160p25_16x9,
	  HDMI_3840x2160p24_16x9,
	  HDMI_4096x2160p24_256x135,
	  HDMI_4096x2160p25_256x135,
	  HDMI_4096x2160p30_256x135,
	  HDMI_1920x1080p100_16x9,
	  HDMI_1920x1080p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2160x1200p90hz,
	  GROUP_END},
		1, VIU_ENCP, 5371100, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_3840x2160p60_16x9,
	  HDMI_3840x2160p50_16x9,
	  HDMI_4096x2160p60_256x135,
	  HDMI_4096x2160p50_256x135,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 1, 1, 2, VID_PLL_DIV_5, 1, 1, 1, -1},
	{{HDMI_4096x2160p60_256x135_Y420,
	  HDMI_4096x2160p50_256x135_Y420,
	  HDMI_3840x2160p60_16x9_Y420,
	  HDMI_3840x2160p50_16x9_Y420,
	  GROUP_END},
		1, VIU_ENCP, 5940000, 2, 1, 1, VID_PLL_DIV_5, 1, 2, 1, -1},
	/* pll setting for VESA modes */
	{{HDMIV_640x480p60hz, /* 4.028G / 16 = 251.75M */
	  GROUP_END},
		1, VIU_ENCP, 4028000, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_800x480p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4761600, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_800x600p60hz,
	  GROUP_END},
		1, VIU_ENCP, 3200000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_852x480p60hz,
	   HDMIV_854x480p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4838400, 4, 4, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1024x600p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4115866, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1024x768p60hz,
	  GROUP_END},
		1, VIU_ENCP, 5200000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1280x768p60hz,
	  GROUP_END},
		1, VIU_ENCP, 3180000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1280x800p60hz,
	  GROUP_END},
		1, VIU_ENCP, 5680000, 4, 2, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1152x864p75hz,
	  HDMIV_1280x960p60hz,
	  HDMIV_1280x1024p60hz,
	  HDMIV_1600x900p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4320000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1600x1200p60hz,
	  GROUP_END},
		1, VIU_ENCP, 3240000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1360x768p60hz,
	  HDMIV_1366x768p60hz,
	  GROUP_END},
		1, VIU_ENCP, 3420000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1400x1050p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4870000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1440x900p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4260000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1440x2560p60hz,
	  GROUP_END},
		1, VIU_ENCP, 4897000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1440x2560p70hz,
	  GROUP_END},
		1, VIU_ENCP, 5600000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1680x1050p60hz,
	  GROUP_END},
		1, VIU_ENCP, 5850000, 4, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_1920x1200p60hz,
	  GROUP_END},
		1, VIU_ENCP, 3865000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{{HDMIV_2560x1600p60hz,
	  GROUP_END},
		1, VIU_ENCP, 3485000, 1, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1},
	{
		{
			HDMIV_3440x1440p60hz, GROUP_END
		},
		1, VIU_ENCP, 3197500, 1, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1
	},
	{
		{
			HDMIV_2400x1200p90hz, GROUP_END
		},
		1, VIU_ENCP, 5600000, 2, 1, 1, VID_PLL_DIV_5, 2, 1, 1, -1
	},
};

/* For colordepth 10bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_30[] = {
	{{HDMI_720x480i60_16x9,
	  HDMI_720x576i50_16x9,
	  GROUP_END},
		1, VIU_ENCI, 5405400, 4, 4, 1, VID_PLL_DIV_6p25, 1, 2, -1, 2},
	{{HDMI_720x576p50_16x9,
	  HDMI_720x480p60_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5405400, 4, 4, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_720x576p100_16x9,
	  HDMI_720x480p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 5405400, 4, 2, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 4, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1920x1080i60_16x9,
	  HDMI_1920x1080i50_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 4, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1920x1080i120_16x9,
	  HDMI_1920x1080i100_16x9,
	  HDMI_1280x720p100_16x9,
	  HDMI_1280x720p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 2, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 1, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, -1},
	{{HDMI_1920x1080p120_16x9,
	  HDMI_1920x1080p100_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 1, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
	{{HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 2, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, -1},
	{{HDMI_4096x2160p60_256x135_Y420,
	  HDMI_4096x2160p50_256x135_Y420,
	  HDMI_3840x2160p60_16x9_Y420,
	  HDMI_3840x2160p50_16x9_Y420,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 1, 1, 1, VID_PLL_DIV_6p25, 1, 2, 1, -1},
	{{HDMI_3840x2160p24_16x9,
	  HDMI_3840x2160p25_16x9,
	  HDMI_3840x2160p30_16x9,
	  HDMI_4096x2160p24_256x135,
	  HDMI_4096x2160p25_256x135,
	  HDMI_4096x2160p30_256x135,
	  GROUP_END},
		1, VIU_ENCP, 3712500, 1, 1, 1, VID_PLL_DIV_6p25, 1, 2, 2, -1},
};

/* For colordepth 12bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_36[] = {
	{{HDMI_720x480i60_16x9,
	  HDMI_720x576i50_16x9,
	  GROUP_END},
		1, VIU_ENCI, 3243240, 2, 4, 1, VID_PLL_DIV_7p5, 1, 2, -1, 2},
	{{HDMI_720x576p50_16x9,
	  HDMI_720x480p60_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3243240, 2, 4, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_720x576p100_16x9,
	  HDMI_720x480p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 3243240, 2, 2, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1280x720p50_16x9,
	  HDMI_1280x720p60_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 4, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1920x1080i60_16x9,
	  HDMI_1920x1080i50_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 4, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1920x1080i120_16x9,
	  HDMI_1920x1080i100_16x9,
	  HDMI_1280x720p100_16x9,
	  HDMI_1280x720p120_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 2, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1920x1080p60_16x9,
	  HDMI_1920x1080p50_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 1, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, -1},
	{{HDMI_1920x1080p120_16x9,
	  HDMI_1920x1080p100_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 1, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
	{{HDMI_1920x1080p30_16x9,
	  HDMI_1920x1080p24_16x9,
	  HDMI_1920x1080p25_16x9,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 2, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, -1},
	{{HDMI_4096x2160p60_256x135_Y420,
	  HDMI_4096x2160p50_256x135_Y420,
	  HDMI_3840x2160p60_16x9_Y420,
	  HDMI_3840x2160p50_16x9_Y420,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 1, 1, 2, VID_PLL_DIV_3p25, 1, 2, 1, -1},
	{{HDMI_3840x2160p24_16x9,
	  HDMI_3840x2160p25_16x9,
	  HDMI_3840x2160p30_16x9,
	  HDMI_4096x2160p24_256x135,
	  HDMI_4096x2160p25_256x135,
	  HDMI_4096x2160p30_256x135,
	  GROUP_END},
		1, VIU_ENCP, 4455000, 1, 1, 1, VID_PLL_DIV_7p5, 1, 2, 2, -1},
};

static void set_hdmitx_fe_clk(struct hdmitx_dev *hdev)
{
	unsigned int tmp = 0;
	enum hdmi_vic vic = hdev->vic;
	hd_set_reg_bits(P_CLKCTRL_VID_CLK_CTRL2, 1, 9, 1);

	switch (vic) {
	case HDMI_720x480i60_16x9:
	case HDMI_720x576i50_16x9:
	case HDMI_720x480i60_4x3:
	case HDMI_720x576i50_4x3:
		tmp = (hd_read_reg(P_CLKCTRL_VID_CLK_DIV) >> 28) & 0xf;
		break;
	default:
		tmp = (hd_read_reg(P_CLKCTRL_VID_CLK_DIV) >> 24) & 0xf;
		break;
	}

	hd_set_reg_bits(P_CLKCTRL_HDMI_CLK_CTRL, tmp, 20, 4);
}

static void set_hdmitx_clk_(struct hdmitx_dev *hdev, enum hdmi_color_depth cd)
{
	int i = 0;
	int j = 0;
	struct hw_enc_clk_val_group *p_enc = NULL;
	enum hdmi_vic vic = hdev->vic;
	char *sspll_dis = NULL;

	if (cd == HDMI_COLOR_DEPTH_24B) {
		p_enc = &setting_enc_clk_val_24[0];
		for (j = 0; j < ARRAY_SIZE(setting_enc_clk_val_24); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i] != GROUP_END)); i ++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == ARRAY_SIZE(setting_enc_clk_val_24)) {
			debug("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == HDMI_COLOR_DEPTH_30B) {
		p_enc = &setting_enc_clk_val_30[0];
		for (j = 0; j < ARRAY_SIZE(setting_enc_clk_val_30); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i] != GROUP_END)); i ++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == ARRAY_SIZE(setting_enc_clk_val_30)) {
			debug("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == HDMI_COLOR_DEPTH_36B) {
		p_enc = &setting_enc_clk_val_36[0];
		for (j = 0; j < ARRAY_SIZE(setting_enc_clk_val_36); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= GROUP_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == ARRAY_SIZE(setting_enc_clk_val_36)) {
			printf("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else {
		printf("not support colordepth 48bits\n");
		return;
	}
next:
	set_hdmitx_sys_clk();
	set_hpll_clk_out(p_enc[j].hpll_clk_out);
	sspll_dis = env_get("sspll_dis");
	if ((!sspll_dis || !strcmp(sspll_dis, "0")) &&
		(cd == HDMI_COLOR_DEPTH_24B))
		set_hpll_sspll(hdev);
	set_hpll_od1(p_enc[j].od1);
	set_hpll_od2(p_enc[j].od2);
	set_hpll_od3(p_enc[j].od3);
	set_hpll_od3_clk_div(p_enc[j].vid_pll_div);
	debug("j = %d  vid_clk_div = %d\n", j, p_enc[j].vid_clk_div);
	set_vid_clk_div(p_enc[j].vid_clk_div);
	set_hdmi_tx_pixel_div(p_enc[j].hdmi_tx_pixel_div);
	set_encp_div(p_enc[j].encp_div);
	set_enci_div(p_enc[j].enci_div);
	set_hdmitx_fe_clk(hdev);
}

static int likely_frac_rate_mode(char *m)
{
	if (strstr(m, "24hz") || strstr(m, "30hz") || strstr(m, "60hz")
		|| strstr(m, "120hz") || strstr(m, "240hz"))
		return 1;
	else
		return 0;
}

void hdmitx_set_clk(struct hdmitx_dev *hdev)
{
	char *frac_rate_str = NULL;

	frac_rate_str = env_get("frac_rate_policy");
	if (frac_rate_str && (frac_rate_str[0] == '0'))
		frac_rate = 0;
	else if (likely_frac_rate_mode(hdev->para->ext_name))
		frac_rate = 1;
	hdev->frac_rate_policy = frac_rate;

	if (hdev->para->cs != HDMI_COLOR_FORMAT_422)
		set_hdmitx_clk_(hdev, hdev->para->cd);
	else
		set_hdmitx_clk_(hdev, HDMI_COLOR_DEPTH_24B);
}


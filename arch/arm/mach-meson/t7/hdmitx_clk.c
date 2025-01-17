// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <amlogic/clk_measure.h>
#include <amlogic/media/vout/hdmitx21/hdmitx.h>
#include "hdmitx_clk.h"

#define SET_CLK_MAX_TIMES 10
#define CLK_TOLERANCE 2 /* Unit: MHz */

#define usleep_range(a, b) udelay(a)

/* local frac_rate flag */
static u32 frac_rate;
static void set_crt_video_enc(u32 vidx, u32 in_sel, u32 div_n);
static void set_crt_video_enc2(u32 vidx, u32 in_sel, u32 div_n);
/*
 * HDMITX Clock configuration
 */

#define WAIT_FOR_PLL_LOCKED(_reg) \
	do { \
		u32 st = 0; \
		int cnt = 10; \
		u32 reg = _reg; \
		while (cnt--) { \
			usleep_range(50, 60); \
			st = (((hd21_read_reg(reg) >> 30) & 0x3) == 3); \
			if (st) \
				break; \
			else { \
				/* reset hpll */ \
				hd21_set_reg_bits(reg, 1, 29, 1); \
				hd21_set_reg_bits(reg, 0, 29, 1); \
			} \
		} \
		if (cnt < 9) \
			pr_info("pll[0x%x] reset %d times\n", reg, 9 - cnt);\
	} while (0)

/*
 * When VCO outputs 6.0 GHz, if VCO unlock with default v1
 * steps, then need reset with v2 or v3
 */
static bool set_hpll_hclk_v1(u32 m, u32 frac_val)
{
	int ret = 0;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0b3a0400 | (m & 0xff));
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x3, 28, 2);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, frac_val);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);

	if (frac_val == 0x8148) {
		if ((hdev->para->timing.vic == HDMI_96_3840x2160p50_16x9 ||
		     hdev->para->timing.vic == HDMI_97_3840x2160p60_16x9 ||
		     hdev->para->timing.vic == HDMI_106_3840x2160p50_64x27 ||
		     hdev->para->timing.vic == HDMI_107_3840x2160p60_64x27) &&
		     hdev->para->cs != HDMI_COLORSPACE_YUV420) {
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x11551293);
		} else {
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x44331290);
		}
	} else {
		if ((hdev->para->timing.vic == HDMI_96_3840x2160p50_16x9 ||
		    hdev->para->timing.vic == HDMI_97_3840x2160p60_16x9 ||
		    hdev->para->timing.vic == HDMI_106_3840x2160p50_64x27 ||
		    hdev->para->timing.vic == HDMI_107_3840x2160p60_64x27 ||
		    hdev->para->timing.vic == HDMI_101_4096x2160p50_256x135 ||
		    hdev->para->timing.vic == HDMI_102_4096x2160p60_256x135) &&
		    hdev->para->cs != HDMI_COLORSPACE_YUV420) {
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x11551293);
		} else {
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a68dc00);
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x65771290);
		}
	}
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927200a);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x56540000);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);

	ret = (((hd21_read_reg(ANACTRL_HDMIPLL_CTRL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static bool set_hpll_hclk_v2(u32 m, u32 frac_val)
{
	int ret = 0;

	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0b3a0400 | (m & 0xff));
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x3, 28, 2);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, frac_val);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0xea68dc00);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x65771290);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927200a);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x56540000);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);

	ret = (((hd21_read_reg(ANACTRL_HDMIPLL_CTRL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static bool set_hpll_hclk_v3(u32 m, u32 frac_val)
{
	int ret = 0;

	hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x0b3a0400 | (m & 0xff));
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x3, 28, 2);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, frac_val);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0xea68dc00);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x65771290);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927200a);
	hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x55540000);
	hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
	WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);

	ret = (((hd21_read_reg(ANACTRL_HDMIPLL_CTRL0) >> 30) & 0x3) == 0x3);
	return ret; /* return hpll locked status */
}

static void set21_t7_hpll_clk_out(u32 frac_rate, u32 clk)
{
	switch (clk) {
	case 5940000:
		if (set_hpll_hclk_v1(0xf7, frac_rate ? 0x8148 : 0x10000))
			break;
		if (set_hpll_hclk_v2(0x7b, 0x18000))
			break;
		if (set_hpll_hclk_v3(0xf7, 0x10000))
			break;
		break;
	case 5600000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004e9);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x0000aaab);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);/*test*/
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5405400:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004e1);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00007333);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 5035000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004d1);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00019555);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4897000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004cc);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x0000d560);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x43231290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x2927200a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x56540028);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4455000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004b9);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x0000e10e);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00014000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x43231290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x2927200a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x56540028);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4324320:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004b4);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00005c29);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4032000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004a8);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 4028000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b0004a7);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x0001aa80);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3712500:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b00049a);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x000110e1);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00016000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x6a685c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x43231290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x2927200a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x56540028);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3450000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b00048f);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00018000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3243240:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b000487);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00000000);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x0000451f);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3197500:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b000485);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00007555);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 3021000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b00047d);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x0001c000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	case 2970000:
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL0, 0x3b00047b);
		if (frac_rate)
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x000140b4);
		else
			hd21_write_reg(ANACTRL_HDMIPLL_CTRL1, 0x00018000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL2, 0x00000000);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL3, 0x4a691c00);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL4, 0x33771290);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL5, 0x3927000a);
		hd21_write_reg(ANACTRL_HDMIPLL_CTRL6, 0x50540000);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0x0, 29, 1);
		WAIT_FOR_PLL_LOCKED(ANACTRL_HDMIPLL_CTRL0);
		pr_info("HPLL: 0x%x\n", hd21_read_reg(ANACTRL_HDMIPLL_CTRL0));
		break;
	default:
		pr_info("error hpll clk: %d\n", clk);
		break;
	}
}

static void set21_hpll_sspll_t7(enum hdmi_vic vic)
{
	switch (vic) {
	case HDMI_16_1920x1080p60_16x9:
	case HDMI_31_1920x1080p50_16x9:
	case HDMI_4_1280x720p60_16x9:
	case HDMI_19_1280x720p50_16x9:
	case HDMI_5_1920x1080i60_16x9:
	case HDMI_20_1920x1080i50_16x9:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 29, 1);
		/* bit[22:20] hdmi_dpll_fref_sel
		 * bit[8] hdmi_dpll_ssc_en
		 * bit[7:4] hdmi_dpll_ssc_dep_sel
		 */
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 1, 20, 3);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 1, 8, 1);
		/* 2: 1000ppm  1: 500ppm */
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL2, 2, 4, 4);
		/* bit[15] hdmi_dpll_sdmnc_en */
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL3, 0, 15, 1);
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0, 29, 1);
		break;
	default:
		break;
	}
}

static void set21_hpll_od1_t7(u32 div)
{
	switch (div) {
	case 1:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0, 16, 2);
		break;
	case 2:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 16, 2);
		break;
	case 4:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 2, 16, 2);
		break;
	case 8:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 3, 16, 2);
		break;
	default:
		break;
	}
}

static void set21_hpll_od2_t7(u32 div)
{
	switch (div) {
	case 1:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0, 18, 2);
		break;
	case 2:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 18, 2);
		break;
	case 4:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 2, 18, 2);
		break;
	default:
		break;
	}
}

static void set21_hpll_od3_t7(u32 div)
{
	switch (div) {
	case 1:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 0, 20, 2);
		break;
	case 2:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 1, 20, 2);
		break;
	case 4:
		hd21_set_reg_bits(ANACTRL_HDMIPLL_CTRL0, 2, 20, 2);
		break;
	default:
		break;
	}
}

static inline int check_div(u32 div)
{
	if (div == -1)
		return -1;
	switch (div) {
	case 1:
		div = 0;
		break;
	case 2:
		div = 1;
		break;
	case 4:
		div = 2;
		break;
	case 6:
		div = 3;
		break;
	case 12:
		div = 4;
		break;
	default:
		break;
	}
	return div;
}

static void hdmitx_enable_encp_clk(struct hdmitx_dev *hdev)
{
	//hd21_set_reg_bits(CLKCTRL_VID_CLK_CTRL2, 1, 2, 1);
}

void hdmitx21_set_audioclk(bool en)
{
	u32 data32;

	// Enable hdmitx_aud_clk
	// [10: 9] clk_sel for cts_hdmitx_aud_clk: 2=fclk_div3
	// [    8] clk_en for cts_hdmitx_aud_clk
	// [ 6: 0] clk_div for cts_hdmitx_aud_clk: fclk_div3/aud_clk_div
	data32 = 0;
	data32 |= (2 << 9);
	data32 |= (0 << 8);
	data32 |= ((18 - 1) << 0);
	hd21_write_reg(CLKCTRL_HTX_CLK_CTRL1, data32);
	// [    8] clk_en for cts_hdmitx_aud_clk
	hd21_set_reg_bits(CLKCTRL_HTX_CLK_CTRL1, en, 8, 1);
}

void hdmitx21_set_default_clk(void)
{
	u32 data32;

	// Enable clk81_hdmitx_pclk
	hd21_set_reg_bits(CLKCTRL_SYS_CLK_EN0_REG2, 1, 4, 1);

	// Enable fixed hdmitx_sys_clk
	data32 = 0;
	data32 |= (3 << 9); // [10: 9] clk_sel for cts_hdmitx_sys_clk: 3=fclk_div5
	data32 |= (0 << 8); // [    8] clk_en for cts_hdmitx_sys_clk
	data32 |= (1 << 0); // [ 6: 0] clk_div for cts_hdmitx_sys_clk: fclk_dvi5/2=400/2=200M
	hd21_write_reg(CLKCTRL_HDMI_CLK_CTRL, data32);
	data32 |= (1 << 8); // [    8] clk_en for cts_hdmitx_sys_clk
	hd21_write_reg(CLKCTRL_HDMI_CLK_CTRL, data32);

	// Enable fixed hdmitx_prif_clk, hdmitx_200m_clk
	data32 = 0;
	data32 |= (3 << 25); // [26:25] clk_sel for cts_hdmitx_200m_clk: 3=fclk_div5
	data32 |= (0 << 24); // [   24] clk_en for cts_hdmitx_200m_clk
	data32 |= (1 << 16); // [22:16] clk_div for cts_hdmitx_200m_clk: fclk_dvi5/16=400/16=25M
	data32 |= (3 << 9); // [10: 9] clk_sel for cts_hdmitx_prif_clk: 3=fclk_div5
	data32 |= (0 << 8); // [    8] clk_en for cts_hdmitx_prif_clk
	data32 |= (1 << 0); // [ 6: 0] clk_div for cts_hdmitx_prif_clk: fclk_dvi5/2=400/2=200M
	hd21_write_reg(CLKCTRL_HTX_CLK_CTRL0, data32);
	data32 |= (1 << 24); // [   24] clk_en for cts_hdmitx_200m_clk
	data32 |= (1 << 8); // [    8] clk_en for cts_hdmitx_prif_clk
	hd21_write_reg(CLKCTRL_HTX_CLK_CTRL0, data32);

	hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, 7, 0, 3);

	// wire    wr_enable = control[3];
	// wire    fifo_enable = control[2];
	// assign  phy_clk_en = control[1];
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL1, 1, 1, 1); // Enable tmds_clk
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL1, 1, 2, 1); // Enable the decoupling FIFO
	// Enable enable the write/read decoupling state machine
	hd21_set_reg_bits(ANACTRL_HDMIPHY_CTRL1, 1, 3, 1);
	// Bring HDMITX MEM output of power down
	hd21_set_reg_bits(PWRCTRL_MEM_PD11, 0, 8, 8);
	// Bring out of reset
	hdmitx21_wr_reg(HDMITX_TOP_SW_RESET, 0);
	// Test after initial out of reset, cannot write to IP register, unless enable access
	hdmitx21_wr_reg(INTR3_MASK_IVCTX, 0xff);
	hdmitx21_wr_reg(HDMITX_TOP_SEC_SCRATCH, 1);
}

void hdmitx21_set_cts_hdcp22_clk(struct hdmitx_dev *hdev)
{
	//hd21_write_reg(CLKCTRL_HDCP22_CLK_CTRL, 0x01000100);
}

void hdmitx21_set_hdcp_pclk(struct hdmitx_dev *hdev)
{
	/* top hdcp pixel clock */
	hd21_set_reg_bits(CLKCTRL_SYS_CLK_EN0_REG2, 1, 3, 1);
}

static void set_hpll_clk_out(u32 clk)
{
	pr_info("config HPLL = %d frac_rate = %d\n", clk, frac_rate);

	set21_t7_hpll_clk_out(frac_rate, clk);
}

/* HERE MUST BE BIT OPERATION!!! */
static void set_hpll_sspll(enum hdmi_vic vic)
{
	set21_hpll_sspll_t7(vic);
}

static void set_hpll_od1(u32 div)
{
	set21_hpll_od1_t7(div);
}

static void set_hpll_od2(u32 div)
{
	set21_hpll_od2_t7(div);
}

static void set_hpll_od3(u32 div)
{
	set21_hpll_od3_t7(div);
}

/* --------------------------------------------------
 *              clocks_set_vid_clk_div
 * --------------------------------------------------
 * wire            clk_final_en    = control[19];
 * wire            clk_div1        = control[18];
 * wire    [1:0]   clk_sel         = control[17:16];
 * wire            set_preset      = control[15];
 * wire    [14:0]  shift_preset    = control[14:0];
 */
static void clocks_set_vid_clk_div_for_hdmi(int div_sel)
{
	int shift_val = 0;
	int shift_sel = 0;
	u32 reg_vid_pll = CLKCTRL_HDMI_VID_PLL_CLK_DIV;
	struct hdmitx_dev *hdev = get_hdmitx21_device();

	pr_info("%s[%d] div = %d\n", __func__, __LINE__, div_sel);

	/* Disable the output clock */
	hd21_set_reg_bits(reg_vid_pll, 0, 18, 2);
	hd21_set_reg_bits(reg_vid_pll, 0, 15, 1);
	if (hdev->enc_idx == 2)
		hd21_set_reg_bits(reg_vid_pll, 1, 25, 1); /* vid_pll2_clk_sel_hdmi */
	else
		hd21_set_reg_bits(reg_vid_pll, 1, 24, 1); /* vid_pll0_clk_sel_hdmi */

	switch (div_sel) {
	case VID_PLL_DIV_1:
		shift_val = 0xFFFF;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_2:
		shift_val = 0x0aaa;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3:
		shift_val = 0x0db6;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3p5:
		shift_val = 0x36cc;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_3p75:
		shift_val = 0x6666;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_4:
		shift_val = 0x0ccc;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_5:
		shift_val = 0x739c;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_6:
		shift_val = 0x0e38;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_6p25:
		shift_val = 0x0000;
		shift_sel = 3;
		break;
	case VID_PLL_DIV_7:
		shift_val = 0x3c78;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_7p5:
		shift_val = 0x78f0;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_12:
		shift_val = 0x0fc0;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_14:
		shift_val = 0x3f80;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_15:
		shift_val = 0x7f80;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_2p5:
		shift_val = 0x5294;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_3p25:
		shift_val = 0x66cc;
		shift_sel = 2;
		break;
	default:
		pr_info("Error: clocks_set_vid_clk_div:  Invalid parameter\n");
		break;
	}

	if (shift_val == 0xffff) {      /* if divide by 1 */
		hd21_set_reg_bits(reg_vid_pll, 1, 18, 1);
	} else {
		hd21_set_reg_bits(reg_vid_pll, 0, 18, 1);
		hd21_set_reg_bits(reg_vid_pll, 0, 16, 2);
		hd21_set_reg_bits(reg_vid_pll, 0, 15, 1);
		hd21_set_reg_bits(reg_vid_pll, 0, 0, 15);

		hd21_set_reg_bits(reg_vid_pll, shift_sel, 16, 2);
		hd21_set_reg_bits(reg_vid_pll, 1, 15, 1);
		hd21_set_reg_bits(reg_vid_pll, shift_val, 0, 15);
		hd21_set_reg_bits(reg_vid_pll, 0, 15, 1);
	}
	/* Enable the final output clock */
	hd21_set_reg_bits(reg_vid_pll, 1, 19, 1);
}

static void set_vid_clk_div(struct hdmitx_dev *hdev, u32 div)
{
	hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, 0, 16, 3);
	hd21_set_reg_bits(CLKCTRL_VID_CLK0_DIV, div - 1, 0, 8);
	hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, 7, 0, 3);
}

static void set_hdmitx_enc_div(struct hdmitx_dev *hdev, u32 div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->enc_idx == 2) {
		hd21_set_reg_bits(CLKCTRL_VIID_CLK2_DIV, div, 12, 4);
		hd21_set_reg_bits(CLKCTRL_VID_CLK2_CTRL2, 1, 3, 1);
	} else {
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_DIV, div, 12, 4);
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL2, 1, 3, 1);
	}
}

static void set_hdmitx_fe_div(struct hdmitx_dev *hdev, u32 div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->enc_idx == 2)
		hd21_set_reg_bits(CLKCTRL_ENC2_HDMI_CLK_CTRL, div, 20, 4);
	else
		hd21_set_reg_bits(CLKCTRL_ENC0_HDMI_CLK_CTRL, div, 20, 4);
	hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL2, 1, 9, 1);
}

static void set_hdmitx_pnx_div(struct hdmitx_dev *hdev, u32 div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->enc_idx == 2)
		hd21_set_reg_bits(CLKCTRL_ENC2_HDMI_CLK_CTRL, div, 24, 4);
	else
		hd21_set_reg_bits(CLKCTRL_ENC0_HDMI_CLK_CTRL, div, 24, 4);
	hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL2, 1, 10, 1);
}

static void set_hdmitx_pixel_div(struct hdmitx_dev *hdev, u32 div)
{
	div = check_div(div);
	if (div == -1)
		return;
	if (hdev->enc_idx == 2)
		hd21_set_reg_bits(CLKCTRL_ENC2_HDMI_CLK_CTRL, div, 16, 4);
	else
		hd21_set_reg_bits(CLKCTRL_ENC0_HDMI_CLK_CTRL, div, 16, 4);
	hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL2, 1, 5, 1);
}

/* mode hpll_clk_out od1 od2(PHY) od3
 * vid_pll_div vid_clk_div hdmi_tx_pixel_div encp_div enci_div
 */
static struct hw_enc_clk_val_group setting_enc_clk_val_24[] = {
	{{HDMI_7_720x480i60_16x9,
	  HDMI_6_720x480i60_4x3,
	  HDMI_22_720x576i50_16x9,
	  HDMI_21_720x576i50_4x3,
	  HDMI_18_720x576p50_16x9,
	  HDMI_17_720x576p50_4x3,
	  HDMI_3_720x480p60_16x9,
	  HDMI_2_720x480p60_4x3,
	  HDMI_VIC_END},
		4324320, 4, 4, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_19_1280x720p50_16x9,
	  HDMI_4_1280x720p60_16x9,
	  HDMI_5_1920x1080i60_16x9,
	  HDMI_20_1920x1080i50_16x9,
	  HDMI_VIC_END},
		5940000, 4, 2, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_16_1920x1080p60_16x9,
	  HDMI_31_1920x1080p50_16x9,
	  HDMI_VIC_END},
		5940000, 4, 1, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_34_1920x1080p30_16x9,
	  HDMI_32_1920x1080p24_16x9,
	  HDMI_33_1920x1080p25_16x9,
	  HDMI_VIC_END},
		5940000, 4, 2, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_89_2560x1080p50_64x27,
	  HDMI_VIC_END},
		3712500, 2, 1, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_90_2560x1080p60_64x27,
	  HDMI_VIC_END},
		3960000, 1, 2, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_95_3840x2160p30_16x9,
	  HDMI_94_3840x2160p25_16x9,
	  HDMI_93_3840x2160p24_16x9,
	  HDMI_63_1920x1080p120_16x9,
	  HDMI_98_4096x2160p24_256x135,
	  HDMI_99_4096x2160p25_256x135,
	  HDMI_100_4096x2160p30_256x135,
	  HDMI_VIC_END},
		5940000, 2, 1, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
	{{HDMI_97_3840x2160p60_16x9,
	  HDMI_96_3840x2160p50_16x9,
	  HDMI_102_4096x2160p60_256x135,
	  HDMI_101_4096x2160p50_256x135,
	  HDMI_VIC_END},
		5940000, 1, 1, 2, VID_PLL_DIV_5, 1, 1, 1, 1},
	{{HDMI_1_640x480p60_4x3,
	  HDMI_VIC_END},
		4028000, 4, 4, 2, VID_PLL_DIV_5, 1, 1, 1, 1, 1},
};

/* For colordepth 10bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_30[] = {
	{{HDMI_7_720x480i60_16x9,
	  HDMI_6_720x480i60_4x3,
	  HDMI_22_720x576i50_16x9,
	  HDMI_21_720x576i50_4x3,
	  HDMI_18_720x576p50_16x9,
	  HDMI_17_720x576p50_4x3,
	  HDMI_3_720x480p60_16x9,
	  HDMI_2_720x480p60_4x3,
	  HDMI_VIC_END},
		5405400, 4, 4, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_19_1280x720p50_16x9,
	  HDMI_4_1280x720p60_16x9,
	  HDMI_5_1920x1080i60_16x9,
	  HDMI_20_1920x1080i50_16x9,
	  HDMI_VIC_END},
		3712500, 4, 1, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_16_1920x1080p60_16x9,
	  HDMI_31_1920x1080p50_16x9,
	  HDMI_VIC_END},
		3712500, 2, 1, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_34_1920x1080p30_16x9,
	  HDMI_32_1920x1080p24_16x9,
	  HDMI_33_1920x1080p25_16x9,
	  HDMI_VIC_END},
		3712500, 2, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_89_2560x1080p50_64x27,
	  HDMI_VIC_END},
		4640625, 2, 1, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_90_2560x1080p60_64x27,
	  HDMI_VIC_END},
		4950000, 1, 2, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_95_3840x2160p30_16x9,
	  HDMI_94_3840x2160p25_16x9,
	  HDMI_93_3840x2160p24_16x9,
	  HDMI_63_1920x1080p120_16x9,
	  HDMI_98_4096x2160p24_256x135,
	  HDMI_99_4096x2160p25_256x135,
	  HDMI_100_4096x2160p30_256x135,
	  HDMI_VIC_END},
		3712500, 1, 1, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_97_3840x2160p60_16x9,
	  HDMI_96_3840x2160p50_16x9,
	  HDMI_102_4096x2160p60_256x135,
	  HDMI_101_4096x2160p50_256x135,
	  HDMI_VIC_END},
		3712500, 1, 1, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
	{{HDMI_1_640x480p60_4x3,
	  HDMI_VIC_END},
		5035000, 4, 4, 2, VID_PLL_DIV_6p25, 1, 1, 1, 1, 1},
};

/* For colordepth 12bits */
static struct hw_enc_clk_val_group setting_enc_clk_val_36[] = {
	{{HDMI_7_720x480i60_16x9,
	  HDMI_6_720x480i60_4x3,
	  HDMI_22_720x576i50_16x9,
	  HDMI_21_720x576i50_4x3,
	  HDMI_18_720x576p50_16x9,
	  HDMI_17_720x576p50_4x3,
	  HDMI_3_720x480p60_16x9,
	  HDMI_2_720x480p60_4x3,
	  HDMI_VIC_END},
		3243240, 4, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_19_1280x720p50_16x9,
	  HDMI_4_1280x720p60_16x9,
	  HDMI_5_1920x1080i60_16x9,
	  HDMI_20_1920x1080i50_16x9,
	  HDMI_VIC_END},
		4455000, 4, 1, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_16_1920x1080p60_16x9,
	  HDMI_31_1920x1080p50_16x9,
	  HDMI_VIC_END},
		4455000, 2, 1, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_34_1920x1080p30_16x9,
	  HDMI_32_1920x1080p24_16x9,
	  HDMI_33_1920x1080p25_16x9,
	  HDMI_VIC_END},
		4455000, 2, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_89_2560x1080p50_64x27,
	  HDMI_VIC_END},
		5568750, 2, 1, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_90_2560x1080p60_64x27,
	  HDMI_VIC_END},
		5940000, 1, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_95_3840x2160p30_16x9,
	  HDMI_94_3840x2160p25_16x9,
	  HDMI_93_3840x2160p24_16x9,
	  HDMI_63_1920x1080p120_16x9,
	  HDMI_98_4096x2160p24_256x135,
	  HDMI_99_4096x2160p25_256x135,
	  HDMI_100_4096x2160p30_256x135,
	  HDMI_VIC_END},
		4455000, 1, 1, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_102_4096x2160p60_256x135,
	  HDMI_101_4096x2160p50_256x135,
	  HDMI_97_3840x2160p60_16x9,
	  HDMI_96_3840x2160p50_16x9,
	  HDMI_VIC_END},
		4455000, 1, 1, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
	{{HDMI_1_640x480p60_4x3,
	  HDMI_VIC_END},
		3021000, 4, 2, 2, VID_PLL_DIV_7p5, 1, 1, 1, 1, 1},
};

static void hdmitx21_set_clk_(struct hdmitx_dev *hdev,
		struct hw_enc_clk_val_group *test_clk)
{
	int i = 0;
	int j = 0;
	struct hw_enc_clk_val_group *p_enc = NULL;
	enum hdmi_vic vic = hdev->para->timing.vic;
	enum hdmi_colorspace cs = hdev->para->cs;
	enum hdmi_color_depth cd = hdev->para->cd;
	char *sspll_dis = NULL;
	struct hw_enc_clk_val_group tmp_clk = {0};

	if (!test_clk)
		return;

	/* YUV 422 always use 24B mode */
	if (cs == HDMI_COLORSPACE_YUV422)
		cd = COLORDEPTH_24B;
	if (cd == COLORDEPTH_24B) {
		p_enc = &setting_enc_clk_val_24[0];
		for (j = 0; j < sizeof(setting_enc_clk_val_24)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_enc_clk_val_24)
			/ sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == COLORDEPTH_30B) {
		p_enc = &setting_enc_clk_val_30[0];
		for (j = 0; j < sizeof(setting_enc_clk_val_30)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_enc_clk_val_30) /
			sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else if (cd == COLORDEPTH_36B) {
		p_enc = &setting_enc_clk_val_36[0];
		for (j = 0; j < sizeof(setting_enc_clk_val_36)
			/ sizeof(struct hw_enc_clk_val_group); j++) {
			for (i = 0; ((i < GROUP_MAX) && (p_enc[j].group[i]
				!= HDMI_VIC_END)); i++) {
				if (vic == p_enc[j].group[i])
					goto next;
			}
		}
		if (j == sizeof(setting_enc_clk_val_36) /
			sizeof(struct hw_enc_clk_val_group)) {
			pr_info("Not find VIC = %d for hpll setting\n", vic);
			return;
		}
	} else {
		pr_info("not support colordepth 48bits\n");
		return;
	}
next:
	*test_clk = p_enc[j];
	memcpy(&tmp_clk, &p_enc[j], sizeof(struct hw_enc_clk_val_group));
	if (cs == HDMI_COLORSPACE_YUV420) {
		/* adjust the sub-clock under Y420 */
		if (cd == COLORDEPTH_24B)
			tmp_clk.od1 = 2;
		tmp_clk.od3 = 1;
		tmp_clk.pnx_div = 2;
		tmp_clk.pixel_div = 2;
	}
	set_hpll_clk_out(tmp_clk.hpll_clk_out);
	sspll_dis = env_get("sspll_dis");
	if ((!sspll_dis || !strcmp(sspll_dis, "0")) &&
		(cd == COLORDEPTH_24B))
		set_hpll_sspll(vic);
	set_hpll_od1(tmp_clk.od1);
	set_hpll_od2(tmp_clk.od2);
	set_hpll_od3(tmp_clk.od3);
	clocks_set_vid_clk_div_for_hdmi(tmp_clk.vid_pll_div);
	set_vid_clk_div(hdev, tmp_clk.vid_clk_div);
	set_hdmitx_enc_div(hdev, tmp_clk.enc_div);
	set_hdmitx_fe_div(hdev, tmp_clk.fe_div);
	set_hdmitx_pnx_div(hdev, tmp_clk.pnx_div);
	set_hdmitx_pixel_div(hdev, tmp_clk.pixel_div);
	hdmitx_enable_encp_clk(hdev);

	//configure crt_video V1: in_sel=vid_pll_clk(0),div_n=xd)
	set_crt_video_enc(0, 0, 1);
	if (hdev->enc_idx == 2)
		set_crt_video_enc2(0, 0, 1);
}

static int likely_frac_rate_mode(char *m)
{
	if (strstr(m, "24hz") || strstr(m, "30hz") || strstr(m, "60hz") ||
	    strstr(m, "120hz") || strstr(m, "240hz"))
		return 1;
	else
		return 0;
}

static void hdmitx_check_frac_rate(struct hdmitx_dev *hdev)
{
	struct hdmi_format_para *para = NULL;
	char *frac_rate_str = NULL;

	para = hdev->para;
	frac_rate_str = env_get("frac_rate_policy");
	if (frac_rate_str && (frac_rate_str[0] == '0'))
		frac_rate = 0;
	else if (para && para->timing.name && likely_frac_rate_mode(para->timing.name))
		frac_rate = 1;

	hdev->frac_rate_policy = frac_rate;
	pr_info("hdmitx_check_frac_rate: frac_rate:%d\n", frac_rate);
}

/*
 * calculate the pixel clock with current clock parameters
 * and measure the pixel clock from hardware clkmsr
 * then compare above 2 clocks
 */
static bool test_pixel_clk(struct hdmitx_dev *hdev, const struct hw_enc_clk_val_group *t)
{
	u32 idx;
	u32 calc_pixel_clk;
	u32 msr_pixel_clk;

	if (!hdev || !t)
		return 0;

	/* refer to meson-clk-measure.c, here can see that before SC2,
	 * the pixel index is 36, and since or after SC2, the index is 59
	 * the index may change in later chips
	 */
	idx = 59;

	/* calculate the pixel_clk firstly */
	calc_pixel_clk = t->hpll_clk_out;
	if (frac_rate)
		calc_pixel_clk = calc_pixel_clk - calc_pixel_clk / 1001;
	calc_pixel_clk /= (t->od1 > 0) ? t->od1 : 1;
	calc_pixel_clk /= (t->od2 > 0) ? t->od2 : 1;
	calc_pixel_clk /= (t->od3 > 0) ? t->od3 : 1;
	switch (t->vid_pll_div) {
	case VID_PLL_DIV_2:
		calc_pixel_clk /= 2;
		break;
	case VID_PLL_DIV_2p5:
		calc_pixel_clk = calc_pixel_clk * 2 / 5;
		break;
	case VID_PLL_DIV_3:
		calc_pixel_clk /= 3;
		break;
	case VID_PLL_DIV_3p25:
		calc_pixel_clk = calc_pixel_clk * 4 / 13;
		break;
	case VID_PLL_DIV_3p5:
		calc_pixel_clk = calc_pixel_clk * 2 / 7;
		break;
	case VID_PLL_DIV_3p75:
		calc_pixel_clk = calc_pixel_clk * 4 / 15;
		break;
	case VID_PLL_DIV_4:
		calc_pixel_clk /= 4;
		break;
	case VID_PLL_DIV_5:
		calc_pixel_clk /= 5;
		break;
	case VID_PLL_DIV_6:
		calc_pixel_clk /= 6;
		break;
	case VID_PLL_DIV_6p25:
		calc_pixel_clk = calc_pixel_clk * 4 / 25;
		break;
	case VID_PLL_DIV_7:
		calc_pixel_clk /= 7;
		break;
	case VID_PLL_DIV_7p5:
		calc_pixel_clk = calc_pixel_clk * 2 / 15;
		break;
	case VID_PLL_DIV_12:
		calc_pixel_clk /= 12;
		break;
	case VID_PLL_DIV_14:
		calc_pixel_clk /= 14;
		break;
	case VID_PLL_DIV_15:
		calc_pixel_clk /= 15;
		break;
	case VID_PLL_DIV_1:
	default:
		calc_pixel_clk /= 1;
		break;
	}
	calc_pixel_clk /= (t->vid_clk_div > 0) ? t->vid_clk_div : 1;
	calc_pixel_clk /= (t->pixel_div > 0) ? t->pixel_div : 1;

	/* measure the current HW pixel_clk */
	msr_pixel_clk = clk_util_clk_msr(idx);

	/* convert both unit to MHz and compare */
	calc_pixel_clk /= 1000;
	if (calc_pixel_clk == msr_pixel_clk)
		return 1;
	if (calc_pixel_clk > msr_pixel_clk && ((calc_pixel_clk - msr_pixel_clk) <= CLK_TOLERANCE))
		return 1;
	if (calc_pixel_clk < msr_pixel_clk && ((msr_pixel_clk - calc_pixel_clk) <= CLK_TOLERANCE))
		return 1;
	pr_info("calc_pixel_clk %dMHz msr_pixel_clk %dMHz\n", calc_pixel_clk, msr_pixel_clk);
	return 0;
}

void hdmitx21_set_clk(struct hdmitx_dev *hdev)
{
	int i = 0;
		struct hw_enc_clk_val_group test_clks = {0};

	hdmitx_check_frac_rate(hdev);

	/* set the clock and test the pixel clock */
	for (i = 0; i < SET_CLK_MAX_TIMES; i++) {
		hdmitx21_set_clk_(hdev, &test_clks);
		if (test_pixel_clk(hdev, &test_clks))
			break;
	}
	if (i == SET_CLK_MAX_TIMES)
		pr_info("need check hdmitx clocks\n");
}

//===============================================================
//  CRT_VIDEO SETTING FUNCTIONS
//===============================================================
static void set_crt_video_enc(u32 vidx, u32 in_sel, u32 div_n)
//input :
//vidx      : 0:V1; 1:V2;     there have 2 parallel set clock generator: V1 and V2
//in_sel     : 0:vid_pll_clk;  1:fclk_div4; 2:flck_div3; 3:fclk_div5;
//            4:vid_pll2_clk; 5:fclk_div7; 6:vid_pll2_clk;
//div_n      : clock divider for enci_clk/encp_clk/encl_clk/vda_clk/hdmi_tx_pixel_clk;
{
	if (vidx == 0) { //V1
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, 0, 19, 1); //[19] -disable clk_div0
		udelay(2);
		// [18:16] - cntl_clk_in_sel
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, in_sel, 16, 3);
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_DIV, div_n - 1, 0, 8); // [7:0]   - cntl_xd0
		udelay(5);
		hd21_set_reg_bits(CLKCTRL_VID_CLK0_CTRL, 1, 19, 1); //[19] -enable clk_div0
	} else { //V2
		hd21_set_reg_bits(CLKCTRL_VIID_CLK0_CTRL, 0, 19, 1); //[19] -disable clk_div0
		udelay(2);
		// [18:16] - cntl_clk_in_sel
		hd21_set_reg_bits(CLKCTRL_VIID_CLK0_CTRL, in_sel, 16, 3);
		hd21_set_reg_bits(CLKCTRL_VIID_CLK0_DIV, div_n - 1, 0, 8); // [7:0]   - cntl_xd0
		udelay(5);
		hd21_set_reg_bits(CLKCTRL_VIID_CLK0_CTRL, 1, 19, 1); //[19] -enable clk_div0
	}
}

//===============================================================
//  CRT_VIDEO SETTING FUNCTIONS
//===============================================================
static void set_crt_video_enc2(u32 vidx, u32 in_sel, u32 div_n)
//input :
//vidx      : 0:V1; 1:V2;     there have 2 parallel set clock generator: V1 and V2
//in_sel     : 0:vid_pll_clk;  1:fclk_div4; 2:flck_div3; 3:fclk_div5;
//            4:vid_pll2_clk; 5:fclk_div7; 6:vid_pll2_clk;
//div_n      : clock divider for enci_clk/encp_clk/encl_clk/vda_clk/hdmi_tx_pixel_clk;
{
	if (vidx == 0) { //V1
		hd21_set_reg_bits(CLKCTRL_VID_CLK2_CTRL, 0, 19, 1); //[19] -disable clk_div0
		udelay(2);
		// [18:16] - cntl_clk_in_sel
		hd21_set_reg_bits(CLKCTRL_VID_CLK2_CTRL, in_sel, 16, 3);
		hd21_set_reg_bits(CLKCTRL_VID_CLK2_DIV, (div_n - 1), 0, 8); // [7:0]   - cntl_xd0
		udelay(5);
		hd21_set_reg_bits(CLKCTRL_VID_CLK2_CTRL, 1, 19, 1); //[19] -enable clk_div0
	} else { //V2
		hd21_set_reg_bits(CLKCTRL_VIID_CLK2_CTRL, 0, 19, 1); //[19] -disable clk_div0
		udelay(2);
		// [18:16] - cntl_clk_in_sel
		hd21_set_reg_bits(CLKCTRL_VIID_CLK2_CTRL, in_sel, 16, 3);
		// [7:0]   - cntl_xd0
		hd21_set_reg_bits(CLKCTRL_VIID_CLK2_DIV, (div_n - 1), 0, 8);
		udelay(5);
		hd21_set_reg_bits(CLKCTRL_VIID_CLK2_CTRL, 1, 19, 1); //[19] -enable clk_div0
	}
}

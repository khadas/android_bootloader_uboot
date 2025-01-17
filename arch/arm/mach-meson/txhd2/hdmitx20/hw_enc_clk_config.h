/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * arch/arm/cpu/armv8/t5d/hdmitx20/hw_enc_clk_config.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __HW_ENC_CLK_CONFIG_H__
#define __HW_ENC_CLK_CONFIG_H__
#include <amlogic/hdmi.h>

//#include <linux/amlogic/vout/enc_clk_config.h>

#define CLK_UTIL_VID_PLL_DIV_1      0
#define CLK_UTIL_VID_PLL_DIV_2      1
#define CLK_UTIL_VID_PLL_DIV_3      2
#define CLK_UTIL_VID_PLL_DIV_3p5    3
#define CLK_UTIL_VID_PLL_DIV_3p75   4
#define CLK_UTIL_VID_PLL_DIV_4      5
#define CLK_UTIL_VID_PLL_DIV_5      6
#define CLK_UTIL_VID_PLL_DIV_6      7
#define CLK_UTIL_VID_PLL_DIV_6p25   8
#define CLK_UTIL_VID_PLL_DIV_7      9
#define CLK_UTIL_VID_PLL_DIV_7p5    10
#define CLK_UTIL_VID_PLL_DIV_12     11
#define CLK_UTIL_VID_PLL_DIV_14     12
#define CLK_UTIL_VID_PLL_DIV_15     13
#define CLK_UTIL_VID_PLL_DIV_2p5    14
#define CLK_UTIL_VID_PLL_DIV_3p25	15

enum viu_type {
    VIU_ENCL = 0,
    VIU_ENCI,
    VIU_ENCP,
    VIU_ENCT,
};

typedef struct{
    enum hdmi_vic vic;
    unsigned viu_path;
    enum viu_type viu_type;
    unsigned hpll_clk_out;
    unsigned od1;
    unsigned od2;
    unsigned od3;
    unsigned vid_pll_div;
    unsigned vid_clk_div;
    unsigned hdmi_tx_pixel_div;
    unsigned encp_div;
    unsigned enci_div;
}hw_enc_clk_val_t;

#define GROUP_MAX	10
#define GROUP_END	-1
struct hw_enc_clk_val_group {
    enum hdmi_vic group[GROUP_MAX];
    unsigned viu_path;
    enum viu_type viu_type;
    unsigned hpll_clk_out;
    unsigned od1;
    unsigned od2;
    unsigned od3;
    unsigned vid_pll_div;
    unsigned vid_clk_div;
    unsigned hdmi_tx_pixel_div;
    unsigned encp_div;
    unsigned enci_div;
};

void hdmitx_set_clk(struct hdmitx_dev *hdev);
void set_hdmitx_clk_420(void);

#endif

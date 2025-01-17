/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __HDMITX_MISC_H__
#define __HDMITX_MISC_H__

/* miscellaneous definition */

/* REG_BASE:  REGISTER_BASE_ADDR = 0xfe000000 */
#define CLKCTRL_SYS_CLK_EN0_REG2 CLKCTRL_REG_ADDR(0x0013)
#define CLKCTRL_VID_CLK_CTRL CLKCTRL_REG_ADDR(0x0030)
#define CLKCTRL_VID_CLK_CTRL2 CLKCTRL_REG_ADDR(0x0031)
#define CLKCTRL_VID_CLK_DIV CLKCTRL_REG_ADDR(0x0032)
#define CLKCTRL_HDMI_CLK_CTRL CLKCTRL_REG_ADDR(0x0038)
#define CLKCTRL_VID_PLL_CLK_DIV CLKCTRL_REG_ADDR(0x0039)
#define CLKCTRL_HDCP22_CLK_CTRL CLKCTRL_REG_ADDR(0x0040)
#define CLKCTRL_HDMI_VID_PLL_CLK_DIV CLKCTRL_REG_ADDR(0x0081)
#define CLKCTRL_HDMI_PLL_TMDS_CLK_DIV CLKCTRL_REG_ADDR(0x0086)
#define CLKCTRL_GP2PLL_CTRL0 CLKCTRL_REG_ADDR(0x0285)
#define CLKCTRL_GP2PLL_CTRL1 CLKCTRL_REG_ADDR(0x0286)
#define CLKCTRL_GP2PLL_CTRL2 CLKCTRL_REG_ADDR(0x0287)
#define CLKCTRL_GP2PLL_CTRL3 CLKCTRL_REG_ADDR(0x0288)
#define CLKCTRL_GP2PLL_STS   CLKCTRL_REG_ADDR(0x0289)
#define CLKCTRL_FPLL_CTRL0   CLKCTRL_REG_ADDR(0x02c0)
#define CLKCTRL_FPLL_CTRL1   CLKCTRL_REG_ADDR(0x02c1)
#define CLKCTRL_FPLL_CTRL2   CLKCTRL_REG_ADDR(0x02c2)
#define CLKCTRL_FPLL_CTRL3   CLKCTRL_REG_ADDR(0x02c3)
#define CLKCTRL_FPLL_STS     CLKCTRL_REG_ADDR(0x02c4)

#define PADCTRL_PIN_MUX_REG8 PADCTRL_REG_ADDR(0x0008)
#define PADCTRL_GPIOH_I      PADCTRL_REG_ADDR(0x00D0)

/* REG_BASE:  REGISTER_BASE_ADDR = 0xfe002000 */
#define RESETCTRL_RESET0 RESETCTRL_REG_ADDR(0x0000)

/* REG_BASE:  REGISTER_BASE_ADDR = 0xfe008000 */
#define ANACTRL_HTX_PLL_CTRL0 ANACTRL_REG_ADDR(0x0070)
#define ANACTRL_HTX_PLL_CTRL1 ANACTRL_REG_ADDR(0x0071)
#define ANACTRL_HTX_PLL_CTRL2 ANACTRL_REG_ADDR(0x0072)
#define ANACTRL_HTX_PLL_CTRL3 ANACTRL_REG_ADDR(0x0073)
#define ANACTRL_HTX_PLL_CTRL4 ANACTRL_REG_ADDR(0x0074)
#define ANACTRL_HTX_PLL_CTRL5 ANACTRL_REG_ADDR(0x0075)
#define ANACTRL_HTX_PLL_CTRL6 ANACTRL_REG_ADDR(0x0076)
#define ANACTRL_HTX_PLL_CTRL7 ANACTRL_REG_ADDR(0x0077)
#define ANACTRL_HDMIPHY_CTRL0 ANACTRL_REG_ADDR(0x0080)
#define ANACTRL_HDMIPHY_CTRL1 ANACTRL_REG_ADDR(0x0081)
#define ANACTRL_HDMIPHY_CTRL2 ANACTRL_REG_ADDR(0x0082)
#define ANACTRL_HDMIPHY_CTRL3 ANACTRL_REG_ADDR(0x0083)
#define ANACTRL_HDMIPHY_CTRL4 ANACTRL_REG_ADDR(0x0084)
#define ANACTRL_HDMIPHY_CTRL5 ANACTRL_REG_ADDR(0x0085)
#define ANACTRL_HDMIPHY_STS   ANACTRL_REG_ADDR(0x0086)
#define ANACTRL_HDMIPHY_CTRL6 ANACTRL_REG_ADDR(0x0087)
#define ANACTRL_DIF_PHY_STS   ANACTRL_REG_ADDR(0x00f3)

/* REG_BASE:  REGISTER_BASE_ADDR = 0xfe00c000 */
#define PWRCTRL_MEM_PD11 PWRCTRL_REG_ADDR(0x001b)

#endif

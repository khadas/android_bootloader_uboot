/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _LCD_CLK_CONFIG_CTRL_H
#define _LCD_CLK_CONFIG_CTRL_H

#include "../lcd_reg.h"
#include "lcd_clk_config.h"

/* G12A */
/* ******** register bit ******** */
/* PLL_CNTL bit: GP0 */
#define LCD_PLL_LOCK_GP0_G12A        31
#define LCD_PLL_EN_GP0_G12A          28
#define LCD_PLL_RST_GP0_G12A         29
#define LCD_PLL_OD_GP0_G12A          16
#define LCD_PLL_N_GP0_G12A           10
#define LCD_PLL_M_GP0_G12A           0

/* PLL_CNTL bit: hpll */
#define LCD_PLL_LOCK_HPLL_G12A       31
#define LCD_PLL_EN_HPLL_G12A         28
#define LCD_PLL_RST_HPLL_G12A        29
#define LCD_PLL_N_HPLL_G12A          10
#define LCD_PLL_M_HPLL_G12A          0

#define LCD_PLL_OD3_HPLL_G12A        20
#define LCD_PLL_OD2_HPLL_G12A        18
#define LCD_PLL_OD1_HPLL_G12A        16

/* **********************************
 * TL1
 * **********************************
 */
/* ******** register bit ******** */
/* PLL_CNTL 0x20 */
#define LCD_PLL_LOCK_TL1             31
#define LCD_PLL_EN_TL1               28
#define LCD_PLL_RST_TL1              29
#define LCD_PLL_N_TL1                10
#define LCD_PLL_M_TL1                0

#define LCD_PLL_OD3_TL1              19
#define LCD_PLL_OD2_TL1              23
#define LCD_PLL_OD1_TL1              21

/* **********************************
 * T7
 * **********************************
 */
#define LCD_PLL_OD3_T7               23
#define LCD_PLL_OD2_T7               21
#define LCD_PLL_OD1_T7               19
#define LCD_PLL_LOCK_T7              31

/* **********************************
 * Spread Spectrum
 * **********************************
 */
#define LCD_SS_STEP_BASE            500 /* ppm */

/* **********************************
 * pll & clk parameter
 * **********************************/
/* ******** clk calculation *********/
#define PLL_WAIT_LOCK_CNT           200
 /* frequency unit: kHz */
#define FIN_FREQ                    (24 * 1000000)
/* clk max error */
#define MAX_ERROR                   (2 * 1000000)

/* ******** register bit ******** */
/* divider */
#define DIV_PRE_SEL_MAX             6
#define EDP_DIV0_SEL_MAX            15
#define EDP_DIV1_SEL_MAX            8

/* g9tv, g9bb, gxbb divider */
#define CLK_DIV_I2O     0
#define CLK_DIV_O2I     1
enum div_sel_e {
	CLK_DIV_SEL_1 = 0,
	CLK_DIV_SEL_2,    /* 1 */
	CLK_DIV_SEL_3,    /* 2 */
	CLK_DIV_SEL_3p5,  /* 3 */
	CLK_DIV_SEL_3p75, /* 4 */
	CLK_DIV_SEL_4,    /* 5 */
	CLK_DIV_SEL_5,    /* 6 */
	CLK_DIV_SEL_6,    /* 7 */
	CLK_DIV_SEL_6p25, /* 8 */
	CLK_DIV_SEL_7,    /* 9 */
	CLK_DIV_SEL_7p5,  /* 10 */
	CLK_DIV_SEL_12,   /* 11 */
	CLK_DIV_SEL_14,   /* 12 */
	CLK_DIV_SEL_15,   /* 13 */
	CLK_DIV_SEL_2p5,  /* 14 */
	CLK_DIV_SEL_4p67, /* 15 */
	CLK_DIV_SEL_2p33, /* 16 */
	CLK_DIV_SEL_MAX,
};

struct lcd_clk_div_table_s {
	char *name;
	unsigned char divider;
	unsigned char num;
	unsigned char den;
	unsigned char shift_sel;
	unsigned short shift_val;
};

extern struct lcd_clk_div_table_s lcd_clk_div_table[];

#endif

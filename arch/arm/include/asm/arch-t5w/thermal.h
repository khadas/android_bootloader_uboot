/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * arch/arm/include/asm/arch-t5d/thermal.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#define SAR_ADC_BASE			0xff809000

#define SAR_ADC_REG0 			SAR_ADC_BASE +(0x0*4)//0xc8100600
#define SAR_ADC_CHAN_LIST		SAR_ADC_BASE +(0x1*4)//0xc8100604
#define SAR_ADC_AVG_CNTL		SAR_ADC_BASE +(0x2*4)//0xc8100608
#define SAR_ADC_REG3			SAR_ADC_BASE +(0x3*4)//0xc810060c
#define SAR_ADC_DELAY			SAR_ADC_BASE +(0x4*4)//0xc8100610
#define SAR_ADC_LAST_RD			SAR_ADC_BASE +(0x5*4)//0xc8100614
#define SAR_ADC_FIFO_RD			SAR_ADC_BASE +(0x6*4)//0xc8100618
#define SAR_ADC_AUX_SW			SAR_ADC_BASE +(0x7*4)//0xc810061c
#define SAR_ADC_CHAN_10_SW		SAR_ADC_BASE +(0x8*4)//0xc8100620
#define SAR_ADC_DETECT_IDLE_SW	SAR_ADC_BASE +(0x9*4)//0xc8100624
#define SAR_ADC_DELTA_10		SAR_ADC_BASE +(0xa*4)//0xc8100628
#define SAR_ADC_REG11			SAR_ADC_BASE +(0xb*4)//0xc810062c
#define SAR_ADC_REG12			SAR_ADC_BASE +(0xc*4)//0xc8100630
#define SAR_ADC_REG13			SAR_ADC_BASE +(0xd*4)//0xc8100634
#define SAR_CLK_CNTL			AO_SAR_CLK
#define SAR_BUS_CLK_EN			AO_RTI_GEN_CNTL_REG0

#define EN_BIT					7
#define FLAG_BUSY_KERNEL		(1<<14)
#define FLAG_BUSY_BL30			(1<<15)

#define SAMPLE_BIT_MASK			0xfff  /*12bit*/

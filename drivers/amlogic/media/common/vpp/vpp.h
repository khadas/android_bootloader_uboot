/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __VPP_H__
#define __VPP_H__

/* OSD csc defines */
enum vpp_matrix_sel_e {
	VPP_MATRIX_0 = 0,	/* OSD convert matrix - new from GXL */
	VPP_MATRIX_1,		/* vd1 matrix before post-blend */
	VPP_MATRIX_2,		/* post matrix */
	VPP_MATRIX_3,		/* xvycc matrix */
	VPP_MATRIX_4,		/* in video eotf - new from GXL */
	VPP_MATRIX_5,		/* in osd eotf - new from GXL */
	VPP_MATRIX_6		/* vd2 matrix before pre-blend */
};
#define NUM_MATRIX 6

/* matrix names */
#define VPP_MATRIX_OSD		VPP_MATRIX_0
#define VPP_MATRIX_VD1		VPP_MATRIX_1
#define VPP_MATRIX_POST		VPP_MATRIX_2
#define VPP_MATRIX_XVYCC	VPP_MATRIX_3
#define VPP_MATRIX_EOTF		VPP_MATRIX_4
#define VPP_MATRIX_OSD_EOTF	VPP_MATRIX_5
#define VPP_MATRIX_VD2		VPP_MATRIX_6

#define CSC_ON              1
#define CSC_OFF             0

enum vpp_lut_sel_e {
	VPP_LUT_OSD_EOTF = 0,
	VPP_LUT_OSD_OETF,
	VPP_LUT_EOTF,
	VPP_LUT_OETF
};
#define NUM_LUT 4

/* matrix registers */
struct matrix_s {
	u16 pre_offset[3];
	u16 matrix[3][3];
	u16 offset[3];
	u16 right_shift;
};

enum vpp_matrix_e {
	MTX_NULL = 0,
	VD1_MTX = 0x1,
	POST2_MTX = 0x2,
	POST_MTX = 0x4,
	VPP1_POST2_MTX = 0x8,
	VPP2_POST2_MTX = 0x10,
	OSD_BLEDN_D0_MTX = 0x20,
	OSD_BLEDN_D1_MTX = 0x21,
	OSD3_BYP_MTX = 0x22
};

enum mtx_csc_e {
	MATRIX_NULL = 0,
	MATRIX_RGB_YUV601 = 0x1,
	MATRIX_RGB_YUV601F = 0x2,
	MATRIX_RGB_YUV709 = 0x3,
	MATRIX_RGB_YUV709F = 0x4,
	MATRIX_YUV601_RGB = 0x10,
	MATRIX_YUV601_YUV601F = 0x11,
	MATRIX_YUV601_YUV709 = 0x12,
	MATRIX_YUV601_YUV709F = 0x13,
	MATRIX_YUV601F_RGB = 0x14,
	MATRIX_YUV601F_YUV601 = 0x15,
	MATRIX_YUV601F_YUV709 = 0x16,
	MATRIX_YUV601F_YUV709F = 0x17,
	MATRIX_YUV709_RGB = 0x20,
	MATRIX_YUV709_YUV601 = 0x21,
	MATRIX_YUV709_YUV601F = 0x22,
	MATRIX_YUV709_YUV709F = 0x23,
	MATRIX_YUV709F_RGB = 0x24,
	MATRIX_YUV709F_YUV601 = 0x25,
	MATRIX_YUV709F_YUV709 = 0x26,
	MATRIX_BT2020YUV_BT2020RGB = 0x40,
	MATRIX_BT2020RGB_709RGB,
	MATRIX_BT2020RGB_CUSRGB,
};

enum vpp_slice_e {
	SLICE0 = 0,
	SLICE1,
	SLICE2,
	SLICE3,
	SLICE_MAX
};

struct matrix_coef_s {
	__u16 pre_offset[3];
	__u16 matrix_coef[3][3];
	__u16 post_offset[3];
	__u16 right_shift;
	__u16 en;
};

void mtx_setting(enum vpp_matrix_e mtx_sel,
		 enum mtx_csc_e mtx_csc,
	int mtx_on);

/* vpp1 post2 matrix */
#ifndef VPP1_MATRIX_COEF00_01
#define VPP1_MATRIX_COEF00_01                      0x5990
#endif
#ifndef VPP1_MATRIX_COEF02_10
#define VPP1_MATRIX_COEF02_10                      0x5991
#endif
#ifndef VPP1_MATRIX_COEF11_12
#define VPP1_MATRIX_COEF11_12                      0x5992
#endif
#ifndef VPP1_MATRIX_COEF20_21
#define VPP1_MATRIX_COEF20_21                      0x5993
#endif
#ifndef VPP1_MATRIX_COEF22
#define VPP1_MATRIX_COEF22                         0x5994
#endif
#ifndef VPP1_MATRIX_COEF13_14
#define VPP1_MATRIX_COEF13_14                      0x5995
#endif
#ifndef VPP1_MATRIX_COEF23_24
#define VPP1_MATRIX_COEF23_24                      0x5996
#endif
#ifndef VPP1_MATRIX_COEF15_25
#define VPP1_MATRIX_COEF15_25                      0x5997
#endif
#ifndef VPP1_MATRIX_CLIP
#define VPP1_MATRIX_CLIP                           0x5998
#endif
#ifndef VPP1_MATRIX_OFFSET0_1
#define VPP1_MATRIX_OFFSET0_1                      0x5999
#endif
#ifndef VPP1_MATRIX_OFFSET2
#define VPP1_MATRIX_OFFSET2                        0x599a
#endif
#ifndef VPP1_MATRIX_PRE_OFFSET0_1
#define VPP1_MATRIX_PRE_OFFSET0_1                  0x599b
#endif
#ifndef VPP1_MATRIX_PRE_OFFSET2
#define VPP1_MATRIX_PRE_OFFSET2                    0x599c
#endif
#ifndef VPP1_MATRIX_EN_CTRL
#define VPP1_MATRIX_EN_CTRL                        0x599d
#endif

/* vpp2 post2 matrix */
#ifndef VPP2_MATRIX_COEF00_01
#define VPP2_MATRIX_COEF00_01                      0x59d0
#endif
#ifndef VPP2_MATRIX_COEF02_10
#define VPP2_MATRIX_COEF02_10                      0x59d1
#endif
#ifndef VPP2_MATRIX_COEF11_12
#define VPP2_MATRIX_COEF11_12                      0x59d2
#endif
#ifndef VPP2_MATRIX_COEF20_21
#define VPP2_MATRIX_COEF20_21                      0x59d3
#endif
#ifndef VPP2_MATRIX_COEF22
#define VPP2_MATRIX_COEF22                         0x59d4
#endif
#ifndef VPP2_MATRIX_COEF13_14
#define VPP2_MATRIX_COEF13_14                      0x59d5
#endif
#ifndef VPP2_MATRIX_COEF23_24
#define VPP2_MATRIX_COEF23_24                      0x59d6
#endif
#ifndef VPP2_MATRIX_COEF15_25
#define VPP2_MATRIX_COEF15_25                      0x59d7
#endif
#ifndef VPP2_MATRIX_CLIP
#define VPP2_MATRIX_CLIP                           0x59d8
#endif
#ifndef VPP2_MATRIX_OFFSET0_1
#define VPP2_MATRIX_OFFSET0_1                      0x59d9
#endif
#ifndef VPP2_MATRIX_OFFSET2
#define VPP2_MATRIX_OFFSET2                        0x59da
#endif
#ifndef VPP2_MATRIX_PRE_OFFSET0_1
#define VPP2_MATRIX_PRE_OFFSET0_1                  0x59db
#endif
#ifndef VPP2_MATRIX_PRE_OFFSET2
#define VPP2_MATRIX_PRE_OFFSET2                    0x59dc
#endif
#ifndef VPP2_MATRIX_EN_CTRL
#define VPP2_MATRIX_EN_CTRL                        0x59dd
#endif
#ifndef L_GAMMA_CNTL_PORT
#define L_GAMMA_CNTL_PORT                          0x1900
#endif
#ifndef L_GAMMA_DATA_PORT
#define L_GAMMA_DATA_PORT                          0x1901
#endif
#ifndef L_GAMMA_ADDR_PORT
#define L_GAMMA_ADDR_PORT                          0x1902
#endif

/*after OSD blend matrix */
#ifndef BLEND_D0_MATRIX_COEF00_01
#define BLEND_D0_MATRIX_COEF00_01                  0x6100
#endif
#ifndef BLEND_D0_MATRIX_COEF02_10
#define BLEND_D0_MATRIX_COEF02_10                  0x6101
#endif
#ifndef BLEND_D0_MATRIX_COEF11_12
#define BLEND_D0_MATRIX_COEF11_12                  0x6102
#endif
#ifndef BLEND_D0_MATRIX_COEF20_21
#define BLEND_D0_MATRIX_COEF20_21                  0x6103
#endif
#ifndef BLEND_D0_MATRIX_COEF22
#define BLEND_D0_MATRIX_COEF22                     0x6104
#endif
#ifndef BLEND_D0_MATRIX_COEF30_31
#define BLEND_D0_MATRIX_COEF30_31                  0x6105
#endif
#ifndef BLEND_D0_MATRIX_COEF32_40
#define BLEND_D0_MATRIX_COEF32_40                  0x6106
#endif
#ifndef BLEND_D0_MATRIX_COEF41_42
#define BLEND_D0_MATRIX_COEF41_42                  0x6107
#endif
#ifndef BLEND_D0_MATRIX_OFFSET0_1
#define BLEND_D0_MATRIX_OFFSET0_1                  0x6108
#endif
#ifndef BLEND_D0_MATRIX_OFFSET2
#define BLEND_D0_MATRIX_OFFSET2                    0x6109
#endif
#ifndef BLEND_D0_MATRIX_PRE_OFFSET0_1
#define BLEND_D0_MATRIX_PRE_OFFSET0_1              0x610a
#endif
#ifndef BLEND_D0_MATRIX_PRE_OFFSET2
#define BLEND_D0_MATRIX_PRE_OFFSET2                0x610b
#endif
#ifndef BLEND_D0_MATRIX_CLIP
#define BLEND_D0_MATRIX_CLIP                       0x6118
#endif
#ifndef BLEND_D0_MATRIX_EN_CTRL
#define BLEND_D0_MATRIX_EN_CTRL                    0x6139
#endif
#ifndef BLEND_D1_MATRIX_COEF00_01
#define BLEND_D1_MATRIX_COEF00_01                  0x6120
#endif
#ifndef BLEND_D1_MATRIX_COEF02_10
#define BLEND_D1_MATRIX_COEF02_10                  0x6121
#endif
#ifndef BLEND_D1_MATRIX_COEF11_12
#define BLEND_D1_MATRIX_COEF11_12                  0x6122
#endif
#ifndef BLEND_D1_MATRIX_COEF20_21
#define BLEND_D1_MATRIX_COEF20_21                  0x6123
#endif
#ifndef BLEND_D1_MATRIX_COEF22
#define BLEND_D1_MATRIX_COEF22                     0x6124
#endif
#ifndef BLEND_D1_MATRIX_COEF30_31
#define BLEND_D1_MATRIX_COEF30_31                  0x6125
#endif
#ifndef BLEND_D1_MATRIX_COEF32_40
#define BLEND_D1_MATRIX_COEF32_40                  0x6126
#endif
#ifndef BLEND_D1_MATRIX_COEF41_42
#define BLEND_D1_MATRIX_COEF41_42                  0x6127
#endif
#ifndef BLEND_D1_MATRIX_OFFSET0_1
#define BLEND_D1_MATRIX_OFFSET0_1                  0x6128
#endif
#ifndef BLEND_D1_MATRIX_OFFSET2
#define BLEND_D1_MATRIX_OFFSET2                    0x6129
#endif
#ifndef BLEND_D1_MATRIX_PRE_OFFSET0_1
#define BLEND_D1_MATRIX_PRE_OFFSET0_1              0x612a
#endif
#ifndef BLEND_D1_MATRIX_PRE_OFFSET2
#define BLEND_D1_MATRIX_PRE_OFFSET2                0x612b
#endif
#ifndef BLEND_D1_MATRIX_CLIP
#define BLEND_D1_MATRIX_CLIP                       0x6138
#endif
#ifndef BLEND_D1_MATRIX_EN_CTRL
#define BLEND_D1_MATRIX_EN_CTRL                    0x6159
#endif
#ifndef OSD3_BYP_MATRIX_COEF00_01
#define OSD3_BYP_MATRIX_COEF00_01                  0x6180
#endif
#ifndef OSD3_BYP_MATRIX_COEF02_10
#define OSD3_BYP_MATRIX_COEF02_10                  0x6181
#endif
#ifndef OSD3_BYP_MATRIX_COEF11_12
#define OSD3_BYP_MATRIX_COEF11_12                  0x6182
#endif
#ifndef OSD3_BYP_MATRIX_COEF20_21
#define OSD3_BYP_MATRIX_COEF20_21                  0x6183
#endif
#ifndef OSD3_BYP_MATRIX_COEF22
#define OSD3_BYP_MATRIX_COEF22                     0x6184
#endif
#ifndef OSD3_BYP_MATRIX_COEF30_31
#define OSD3_BYP_MATRIX_COEF30_31                  0x6185
#endif
#ifndef OSD3_BYP_MATRIX_COEF32_40
#define OSD3_BYP_MATRIX_COEF32_40                  0x6186
#endif
#ifndef OSD3_BYP_MATRIX_COEF41_42
#define OSD3_BYP_MATRIX_COEF41_42                  0x6187
#endif
#ifndef OSD3_BYP_MATRIX_OFFSET0_1
#define OSD3_BYP_MATRIX_OFFSET0_1                  0x6188
#endif
#ifndef OSD3_BYP_MATRIX_OFFSET2
#define OSD3_BYP_MATRIX_OFFSET2                    0x6189
#endif
#ifndef OSD3_BYP_MATRIX_PRE_OFFSET0_1
#define OSD3_BYP_MATRIX_PRE_OFFSET0_1              0x618a
#endif
#ifndef OSD3_BYP_MATRIX_PRE_OFFSET2
#define OSD3_BYP_MATRIX_PRE_OFFSET2                0x618b
#endif
#ifndef OSD3_BYP_MATRIX_CLIP
#define OSD3_BYP_MATRIX_CLIP                       0x6198
#endif
#ifndef OSD3_BYP_MATRIX_EN_CTRL
#define OSD3_BYP_MATRIX_EN_CTRL                    0x61b9
#endif
#endif

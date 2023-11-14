#ifndef __EXTRA_REGISTER_H__
#define __EXTRA_REGISTER_H__

//OTP
#define OTP_LIC                                    ((0x0010  << 2) + 0xfe440000)
#define OTP_LIC00                                  (OTP_LIC + 0x00)
#define OTP_LIC01                                  (OTP_LIC + 0x04)
#define OTP_LIC02                                  (OTP_LIC + 0x08)
#define OTP_LIC03                                  (OTP_LIC + 0x0C)

#define OTP_LIC10                                  (OTP_LIC + 0x10)
#define OTP_LIC11                                  (OTP_LIC + 0x14)
#define OTP_LIC12                                  (OTP_LIC + 0x18)
#define OTP_LIC13                                  (OTP_LIC + 0x1C)

#define OTP_LIC20                                  (OTP_LIC + 0x20)
#define OTP_LIC21                                  (OTP_LIC + 0x24)
#define OTP_LIC22                                  (OTP_LIC + 0x28)
#define OTP_LIC23                                  (OTP_LIC + 0x2C)

#define OTP_LIC30                                  (OTP_LIC + 0x30)
#define OTP_LIC31                                  (OTP_LIC + 0x34)
#define OTP_LIC32                                  (OTP_LIC + 0x38)
#define OTP_LIC33                                  (OTP_LIC + 0x3C)

#define OTP_LIC0                                   (OTP_LIC00)

//mailbox
#define MAILBOX_IRQA_CLR1                          ((0x0421  << 2) + 0xfe006000)

//nna
#define TS_NNA_CFG_REG1                            ((0x0001  << 2) + 0xfe020000)
#define TS_NNA_STAT0                               ((0x0010  << 2) + 0xfe020000)

//vpu
#define TS_VPU_CFG_REG1                            ((0x0001  << 2) + 0xfe01c000)
#define TS_VPU_STAT0                               ((0x0010  << 2) + 0xfe01c000)

//lcd
#define MIPI_DSI_PHY_CTRL                          ((0x0000  << 2) + 0xfe014000)
#define MIPI_DSI_CHAN_CTRL                         ((0x0001  << 2) + 0xfe014000)
#define MIPI_DSI_CHAN_STS                          ((0x0002  << 2) + 0xfe014000)
#define MIPI_DSI_CLK_TIM                           ((0x0003  << 2) + 0xfe014000)
#define MIPI_DSI_HS_TIM                            ((0x0004  << 2) + 0xfe014000)
#define MIPI_DSI_LP_TIM                            ((0x0005  << 2) + 0xfe014000)
#define MIPI_DSI_ANA_UP_TIM                        ((0x0006  << 2) + 0xfe014000)
#define MIPI_DSI_INIT_TIM                          ((0x0007  << 2) + 0xfe014000)
#define MIPI_DSI_WAKEUP_TIM                        ((0x0008  << 2) + 0xfe014000)
#define MIPI_DSI_LPOK_TIM                          ((0x0009  << 2) + 0xfe014000)
#define MIPI_DSI_LP_WCHDOG                         ((0x000a  << 2) + 0xfe014000)
#define MIPI_DSI_ANA_CTRL                          ((0x000b  << 2) + 0xfe014000)
#define MIPI_DSI_CLK_TIM1                          ((0x000c  << 2) + 0xfe014000)
#define MIPI_DSI_TURN_WCHDOG                       ((0x000d  << 2) + 0xfe014000)
#define MIPI_DSI_ULPS_CHECK                        ((0x000e  << 2) + 0xfe014000)
#define MIPI_DSI_TEST_CTRL0                        ((0x000f  << 2) + 0xfe014000)
#define MIPI_DSI_TEST_CTRL1                        ((0x0010  << 2) + 0xfe014000)

#define MIPI_DSI_CLK_TIM1                          ((0x000c  << 2) + 0xfe014000)

// -----------------------------------------------
#define MIPI_DSI_DWC_VERSION_OS                    ((0x0000  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PWR_UP_OS                     ((0x0001  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_CLKMGR_CFG_OS                 ((0x0002  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_DPI_VCID_OS                   ((0x0003  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_DPI_COLOR_CODING_OS           ((0x0004  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_DPI_CFG_POL_OS                ((0x0005  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_DPI_LP_CMD_TIM_OS             ((0x0006  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PCKHDL_CFG_OS                 ((0x000b  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_GEN_VCID_OS                   ((0x000c  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_MODE_CFG_OS                   ((0x000d  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_MODE_CFG_OS               ((0x000e  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_PKT_SIZE_OS               ((0x000f  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_NUM_CHUNKS_OS             ((0x0010  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_NULL_SIZE_OS              ((0x0011  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_HSA_TIME_OS               ((0x0012  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_HBP_TIME_OS               ((0x0013  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_HLINE_TIME_OS             ((0x0014  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_VSA_LINES_OS              ((0x0015  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_VBP_LINES_OS              ((0x0016  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_VFP_LINES_OS              ((0x0017  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_VID_VACTIVE_LINES_OS          ((0x0018  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_EDPI_CMD_SIZE_OS              ((0x0019  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_CMD_MODE_CFG_OS               ((0x001a  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_GEN_HDR_OS                    ((0x001b  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_GEN_PLD_DATA_OS               ((0x001c  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_CMD_PKT_STATUS_OS             ((0x001d  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_TO_CNT_CFG_OS                 ((0x001e  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_HS_RD_TO_CNT_OS               ((0x001f  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_LP_RD_TO_CNT_OS               ((0x0020  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_HS_WR_TO_CNT_OS               ((0x0021  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_LP_WR_TO_CNT_OS               ((0x0022  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_BTA_TO_CNT_OS                 ((0x0023  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_SDF_3D_OS                     ((0x0024  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_LPCLK_CTRL_OS                 ((0x0025  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_TMR_LPCLK_CFG_OS          ((0x0026  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_TMR_CFG_OS                ((0x0027  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_RSTZ_OS                   ((0x0028  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_IF_CFG_OS                 ((0x0029  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_ULPS_CTRL_OS              ((0x002a  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_TX_TRIGGERS_OS            ((0x002b  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_STATUS_OS                 ((0x002c  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_TST_CTRL0_OS              ((0x002d  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_PHY_TST_CTRL1_OS              ((0x002e  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_INT_ST0_OS                    ((0x002f  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_INT_ST1_OS                    ((0x0030  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_INT_MSK0_OS                   ((0x0031  << 2) + 0xfe074000)
#define MIPI_DSI_DWC_INT_MSK1_OS                   ((0x0032  << 2) + 0xfe074000)

#define MIPI_DSI_TOP_SW_RESET                      ((0x00f0  << 2) + 0xfe074000)
#define MIPI_DSI_TOP_CLK_CNTL                      ((0x00f1  << 2) + 0xfe074000)
#define MIPI_DSI_TOP_CNTL                          ((0x00f2  << 2) + 0xfe074000)
#define MIPI_DSI_TOP_INTR_CNTL_STAT                ((0x00fc  << 2) + 0xfe074000)
#define MIPI_DSI_TOP_MEAS_CNTL                     ((0x00f6  << 2) + 0xfe074000)
#define MIPI_DSI_TOP_STAT                          ((0x00f7  << 2) + 0xfe074000)
#define MIPI_DSI_TOP_MEM_PD                        ((0x00fd  << 2) + 0xfe074000)

//dsp
#define CLKCTRL_DSPA_CLK_CTRL0                     ((0x0027  << 2) + 0xfe000000)
#define CLKCTRL_DSPB_CLK_CTRL0                     ((0x0028  << 2) + 0xfe000000)

#endif
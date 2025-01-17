// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <asm/arch/secure_apb.h>
#include <asm/arch/timing.h>
#include <asm/arch/ddr_define.h>

/* board clk defines */
#define CPU_CLK                                 1512

/* ddr config support multiple configs for boards which use same bootloader:
 * config steps:
 * 1. add a new data struct in __ddr_setting[]
 * 2. config correct board_id, ddr_type, freq, etc..
 */


/* CAUTION!! */
/* Confirm ddr configs with hardware designer,
 * if you don't know how to config, then don't edit it
 */

/* Key configs */
/*
 * board_id: check hardware adc config
 * dram_rank_config:
 *            #define CONFIG_DDR_CHL_AUTO					0xF
 *            #define CONFIG_DDR0_16BIT_CH0				0x1
 *            #define CONFIG_DDR0_16BIT_RANK01_CH0		0x4
 *            #define CONFIG_DDR0_32BIT_RANK0_CH0			0x2
 *            #define CONFIG_DDR0_32BIT_RANK01_CH01		0x3
 *            #define CONFIG_DDR0_32BIT_16BIT_RANK0_CH0	0x5
 *            #define CONFIG_DDR0_32BIT_16BIT_RANK01_CH0	0x6
 * DramType:
 *            #define CONFIG_DDR_TYPE_DDR3				0
 *            #define CONFIG_DDR_TYPE_DDR4				1
 *            #define CONFIG_DDR_TYPE_LPDDR4				2
 *            #define CONFIG_DDR_TYPE_LPDDR3				3
 * DRAMFreq:
 *            {pstate0, pstate1, pstate2, pstate3} //more than one pstate means use dynamic freq
 *
 */

#define 		ENABLE_LPDDR4X_MODE 			1

#define         AN_408_DDR0_1G_DDR1_0G          0
#define         AN_408_DDR0_2G_DDR1_0G          1
#define         AN_408_DDR0_1G_DDR1_1G          2
#define         AN_408_DDR0_2G_DDR1_1G          3
#define         AN_408_DDR0_2G_DDR1_2G          4
#define         AN_408_DDR0_2G_DDR1_2G_RANK01   5
#define         AN_408_DDR0_4G_DDR1_4G_RANK01   6

#define DDR_SIZE_TYPE_1         AN_408_DDR0_1G_DDR1_1G //AN_408_DDR0_2G_DDR1_0G//AN_408_DDR0_2G_DDR1_1G//AN_408_DDR0_1G_DDR1_1G

ddr_set_t __ddr_setting[] __attribute__ ((section(".ddr_param"))) = {
#if 1
		{
		/* tl1 skt (x309) ddr4 */
		.board_id = CONFIG_BOARD_ID_MASK,
		.version = 1,
		//.dram_rank_config		= CONFIG_DDR0_32BIT_RANK0_CH0,
		.DramType = CONFIG_DDR_TYPE_DDR4,
		.DRAMFreq ={ 1200,		       0,	0,	 0	 },
		.ddr_rfc_type = DDR_RFC_TYPE_DDR4_2Gbx8,
		.ddr_base_addr = CFG_DDR_BASE_ADDR,
		.ddr_start_offset = CFG_DDR_START_OFFSET,
		.DisabledDbyte[0] = 0xf0,                       //bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.DisabledDbyte[1] = 0xf0,                       //bit 0 -3 ch1 cs0 ,bit 4-7 ch1 cs1,
		.Is2Ttiming = 1,
		.HdtCtrl = 0xC8,
		//.dram_ch0_size_MB		= 0xffff,
		//.dram_ch1_size_MB		= 0,
#if (DDR_SIZE_TYPE_1 == AN_408_DDR0_1G_DDR1_0G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_0G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_1G_DDR1_1G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_1G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_2G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#endif
		.training_SequenceCtrl ={ 0x31f,		    0x61 },     //ddr3 0x21f 0x31f
		.phy_odt_config_rank ={ 0x23,			  0x13 }, //use 0x23 0x13  compatibility with 1rank and 2rank //targeting rank 0. [3:0] is used //for write ODT [7:4] is used for //read ODT
		.dfi_odt_config = 0x0d0d,               //use 0d0d compatibility with 1rank and 2rank  //0808
		.PllBypassEn = 0,                       //bit0-ps0,bit1-ps1
		.ddr_rdbi_wr_enable = 0x0,              //bit 0 read-dbi,bit 1 write dbi
		.clk_drv_ohm = 40,
		.cs_drv_ohm = 40,
		.ac_drv_ohm = 40,
		.soc_data_drv_ohm_p = 40,
		.soc_data_drv_ohm_n = 40,
		.soc_data_odt_ohm_p = 48,
		.soc_data_odt_ohm_n = 0,
		.dram_data_drv_ohm = 40,        //48, //34, //ddr4 sdram only 34 or 48, skt board use 34 better
		.dram_data_odt_ohm = 48,        //48,                //60,
		.dram_ac_odt_ohm = 0,
		.soc_clk_slew_rate = 0x3ff,
		.soc_cs_slew_rate = 0x3ff,
		.soc_ac_slew_rate = 0x3ff,
		.soc_data_slew_rate = 0x2ff,
		.vref_output_permil = 500,
		.vref_receiver_permil = 0,
		.vref_dram_permil = 0,
		//.vref_reverse			= 0,
		//.ac_trace_delay			={0x0,0x0},// {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40},
		.ac_trace_delay ={ 32 + 20,		     32 + 20, 32 + 20, 32 + 20, 32, 32 + 0, 32 + 20, 32 + 20, 32 + 20, 32 + 20},
	#if 0
		.ddr_dmc_remap =
		{
		[0] = (5 | 6 << 5 | 7 << 10 | 8 << 15 | 9 << 20 | 10 << 25),
		[1] = (11 | 0 << 5 | 0 << 10 | 15 << 15 | 16 << 20 | 17 << 25),
		[2] = (18 | 19 << 5 | 20 << 10 | 21 << 15 | 22 << 20 | 23 << 25),
		[3] = (24 | 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25),
		[4] = (30 | 12 << 5 | 13 << 10 | 14 << 15 | 0 << 20 | 0 << 25),
		},
	#endif
	#if 1
		.ddr_dmc_remap =
		{
		[0] = ( 5 |  7 << 5 |  8 << 10 |  9 << 15 | 10 << 20 | 11 << 25 ),
		[1] = ( 12|  0 << 5 |  0 << 10 | 14 << 15 | 15 << 20 | 16 << 25 ),
		[2] = ( 17| 18 << 5 | 19 << 10 | 21 << 15 | 22 << 20 | 23 << 25 ),
		[3] = ( 24| 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25 ),
		[4] = ( 30| 13 << 5 | 20 << 10 |  6 << 15 |  0 << 20 |  0 << 25 ),
		},
	#endif
		.ddr_lpddr34_ca_remap ={ 00,			   00 },
		.ddr_lpddr34_dq_remap ={ 00,			   00 },
		.dram_rtt_nom_wr_park ={ 00,			   00 },

		/* pll ssc config:
		 *
		 *   pll_ssc_mode = (1<<20) | (1<<8) | ([strength] << 4) | [mode],
		 *      ppm = strength * 500
		 *      mode: 0=center, 1=up, 2=down
		 *
		 *   eg:
		 *     1. config 1000ppm center ss. then mode=0, strength=2
		 *        .pll_ssc_mode = (1<<20) | (1<<8) | (2 << 4) | 0,
		 *     2. config 3000ppm down ss. then mode=2, strength=6
		 *        .pll_ssc_mode = (1<<20) | (1<<8) | (6 << 4) | 2,
		 */
		//.pll_ssc_mode			= (1<<20) | (1<<8) | (2<<4) | 0,//center_ssc_1000ppm
		.ddr_func = DDR_FUNC_COPY_CHANNEL_CFG,  // DDR_FUNC,
		.magic = DRAM_CFG_MAGIC,
		.slt_test_function ={ 0x0,			0x0 }, //{0x1,0x0},enable slt 4 DRAMFreq test;{0x0,0x0},disable slt 4 DRAMFreq test;
		.fast_boot[0] = 0,
		.ac_pinmux =
		{
		2,
		5,
		20,
		14,
		8,
		24,
		4,
		13,
		25,
		12,
		10,
		23,
		26,
		18,
		0,
		0,
		0,
		9,
		1,
		19,
		0,
		22,
		11,
		7,
		6,
		3,
		0,
		0,
		},
		.dfi_pinmux =
		{
		18,
		15,
		16,
		12,
		5,
		20,
		10,
		7,
		0,
		11,
		14,
		8,
		23,
		17,
		3,
		13,
		22,
		2,
		21,
		19,
		6,
		4,
		1,
		0,
		0,
		0,
		},
		},
#endif
#if 1
		{
		//.fast_boot[0]=0x000000fd,// 253
		.board_id = CONFIG_BOARD_ID_MASK,
		.version = 1,
		//.dram_rank_config		= CONFIG_DDR0_32BIT_RANK0_CH0,
		.DramType = CONFIG_DDR_TYPE_LPDDR4,
		.enable_lpddr4x_mode = ENABLE_LPDDR4X_MODE,
		.DRAMFreq ={ 1200,		       0,	0,	 0	 },
		.ddr_rfc_type = DDR_RFC_TYPE_DDR4_2Gbx8,
		.ddr_base_addr = CFG_DDR_BASE_ADDR,
		.ddr_start_offset = CFG_DDR_START_OFFSET,
		.DisabledDbyte[0] = 0xf0,                       //bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.DisabledDbyte[1] = 0xf0,                       //bit 0 -3 ch1 cs0 ,bit 4-7 ch1 cs1,
		.Is2Ttiming = 0,
		.HdtCtrl = 0xc8,
		//.dram_ch0_size_MB		= 0xffff,
		//.dram_ch1_size_MB		= 0,
#if (DDR_SIZE_TYPE_1 == AN_408_DDR0_1G_DDR1_0G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_0G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_1G_DDR1_1G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_1G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_2G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_2G_RANK01)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_4G_DDR1_4G_RANK01)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#endif
		.training_SequenceCtrl ={ 0x131f,		    0x61 },     //ddr3 0x21f 0x31f
		.phy_odt_config_rank ={ 0x23,			  0x13 }, //use 0x23 0x13  compatibility with 1rank and 2rank //targeting rank 0. [3:0] is used //for write ODT [7:4] is used for //read ODT
		.dfi_odt_config = 0x0d0d,               //use 0d0d compatibility with 1rank and 2rank  //0808
		.PllBypassEn = 0,                       //bit0-ps0,bit1-ps1
		.ddr_rdbi_wr_enable = 0x0,              //bit 0 read-dbi,bit 1 write dbi
		.clk_drv_ohm = 40,
		.cs_drv_ohm = 40,
		.ac_drv_ohm = 40,
		.soc_data_drv_ohm_p = 40,
		.soc_data_drv_ohm_n = 40,
		.soc_data_odt_ohm_p = 40,
		.soc_data_odt_ohm_n = 40,
		.dram_data_drv_ohm = 40,                //48, //34, //ddr4 sdram only 34 or 48, skt board use 34 better
		.dram_data_odt_ohm = 40,                //60,
		.dram_ac_odt_ohm = 240,
		.soc_clk_slew_rate = 0x1ff,
		.soc_cs_slew_rate = 0x1ff,
		.soc_ac_slew_rate = 0x1ff,
		.soc_data_slew_rate = 0x5ff,
		.vref_output_permil = 000, //300
		.vref_receiver_permil = 0,
		.vref_dram_permil = 200,
		//.vref_reverse			= 0,
		//.ac_trace_delay			={0x0,0x0},// {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40},
		.ac_trace_delay ={ 32 + 10,		     32,      0,       0, 0, 32 + 10, 32, 0, 0, 0},
		//.ac_trace_delay ={ 32 + 10,		     32 + 0, 32 + 2, 32 + 2, 32, 32 + 10, 32 + 0, 32 + 2, 32 + 2, 32 + 0},
	#if 0
		.ddr_dmc_remap =
		{
		[0] = (5 | 6 << 5 | 7 << 10 | 8 << 15 | 9 << 20 | 10 << 25),
		[1] = (11 | 0 << 5 | 0 << 10 | 15 << 15 | 16 << 20 | 17 << 25),
		[2] = (18 | 19 << 5 | 20 << 10 | 21 << 15 | 22 << 20 | 23 << 25),
		[3] = (24 | 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25),
		[4] = (30 | 12 << 5 | 13 << 10 | 14 << 15 | 0 << 20 | 0 << 25),
		},
	#endif
	#if 1
		.ddr_dmc_remap =
		{
		[0] = ( 5 |  7 << 5 |  8 << 10 |  9 << 15 | 10 << 20 | 11 << 25 ),
		[1] = ( 12|  0 << 5 |  0 << 10 | 14 << 15 | 15 << 20 | 16 << 25 ),
		[2] = ( 17| 18 << 5 | 19 << 10 | 21 << 15 | 22 << 20 | 23 << 25 ),
		[3] = ( 24| 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25 ),
		[4] = ( 30| 13 << 5 | 20 << 10 |  6 << 15 |  0 << 20 |  0 << 25 ),
		},
	#endif
		.ddr_lpddr34_ca_remap ={ 00,			   00 },
		.ddr_lpddr34_dq_remap ={ 7,			     1,			       5,	0,	 2,  3,	 4,  6,
		  8,  15,			12,	 13,	  9,  10, 11, 14,
		  22, 18,			21,	 20,	  17, 23, 16, 19,
		  29, 30,			28,	 31,	  24, 27, 26, 25 },

		.dram_rtt_nom_wr_park ={ 00,			   00 },

		/* pll ssc config:
		 *
		 *   pll_ssc_mode = (1<<20) | (1<<8) | ([strength] << 4) | [mode],
		 *      ppm = strength * 500
		 *      mode: 0=center, 1=up, 2=down
		 *
		 *   eg:
		 *     1. config 1000ppm center ss. then mode=0, strength=2
		 *        .pll_ssc_mode = (1<<20) | (1<<8) | (2 << 4) | 0,
		 *     2. config 3000ppm down ss. then mode=2, strength=6
		 *        .pll_ssc_mode = (1<<20) | (1<<8) | (6 << 4) | 2,
		 */
		//.pll_ssc_mode			= (1<<20) | (1<<8) | (2<<4) | 0,//center_ssc_1000ppm
		.ddr_func = 0,                          // DDR_FUNC,
		.magic = DRAM_CFG_MAGIC,
		.slt_test_function ={ 0x0,			0x0 }, //{0x1,0x0},enable slt 4 DRAMFreq test;{0x0,0x0},disable slt 4 DRAMFreq test;
		.fast_boot[0] = 0,
		.ac_pinmux ={ 2,	      3,			1,	 0,	  5, 4, 0, 0, 0, 0,
		  1, 3,			       5,	2,	 4, 0, 0, 0, 0 },

		.dfi_pinmux =
		{
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		0,
		10,
		11,
		12,
		13,
		14,
		15,
		16,
		17,
		18,
		19,
		20,
		21,
		22,
		23,
		0,
		0,
		0,
		},
		},

		{
		//.fast_boot[0]=0x000000fd,// 253
		.board_id = CONFIG_BOARD_ID_MASK,
		.version = 1,
		//.dram_rank_config		= CONFIG_DDR0_32BIT_RANK0_CH0,
		.DramType = CONFIG_DDR_TYPE_LPDDR4,
		.enable_lpddr4x_mode = ENABLE_LPDDR4X_MODE,
		.DRAMFreq ={ 1200,		       0,	0,	 0	 },
		.ddr_rfc_type = DDR_RFC_TYPE_LPDDR4_8Gbx1,
		.ddr_base_addr = CFG_DDR_BASE_ADDR,
		.ddr_start_offset = CFG_DDR_START_OFFSET,
		.DisabledDbyte[0] = 0xf0,                       //bit 0 -3 ch0 cs0 ,bit 4-7 ch0 cs1,
		.DisabledDbyte[1] = 0xf0,                       //bit 0 -3 ch1 cs0 ,bit 4-7 ch1 cs1,
		.Is2Ttiming = 0,
		.HdtCtrl = 0xc8,
		//.dram_ch0_size_MB		= 0xffff,
		//.dram_ch1_size_MB		= 0,
#if (DDR_SIZE_TYPE_1 == AN_408_DDR0_1G_DDR1_0G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_0G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_1G_DDR1_1G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_1G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_2G)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX0 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_2G_DDR1_2G_RANK01)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX2 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#elif (DDR_SIZE_TYPE_1 == AN_408_DDR0_4G_DDR1_4G_RANK01)
		.dram_ch0_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
		.dram_ch1_size_MB =
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS0_BYTE_23_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_01_SIZE_256_ID_OFFSET) +
			(DRAM_SIZE_ID_256MBX4 << CONFIG_CS1_BYTE_23_SIZE_256_ID_OFFSET),
#endif
		.training_SequenceCtrl ={ 0x131f,		    0x61 },     //ddr3 0x21f 0x31f
		.phy_odt_config_rank ={ 0x23,			  0x13 }, //use 0x23 0x13  compatibility with 1rank and 2rank //targeting rank 0. [3:0] is used //for write ODT [7:4] is used for //read ODT
		.dfi_odt_config = 0x0d0d,               //use 0d0d compatibility with 1rank and 2rank  //0808
		.PllBypassEn = 0,                       //bit0-ps0,bit1-ps1
		.ddr_rdbi_wr_enable = 0x0,              //bit 0 read-dbi,bit 1 write dbi
		.clk_drv_ohm = 40,
		.cs_drv_ohm = 40,
		.ac_drv_ohm = 40,
		.soc_data_drv_ohm_p = 40,
		.soc_data_drv_ohm_n = 40,
		.soc_data_odt_ohm_p = 40,
		.soc_data_odt_ohm_n = 40,
		.dram_data_drv_ohm = 40,                //48, //34, //ddr4 sdram only 34 or 48, skt board use 34 better
		.dram_data_odt_ohm = 40,                //60,
		.dram_ac_odt_ohm = 240,
		.soc_clk_slew_rate = 0x1ff,
		.soc_cs_slew_rate = 0x1ff,
		.soc_ac_slew_rate = 0x1ff,
		.soc_data_slew_rate = 0x5ff,
		.vref_output_permil = 000,
		.vref_receiver_permil = 0,
		.vref_dram_permil = 200,
		//.vref_reverse			= 0,
		//.ac_trace_delay			={0x0,0x0},// {0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40},
		//.ac_trace_delay ={ 32 + 10,		     32 + 0, 32 + 2, 32 + 2, 32, 32 + 10, 32 + 0, 32 + 2, 32 + 2, 32 + 0},
		.ac_trace_delay ={ 32 + 10,		     32,      0,       0, 0, 32 + 10, 32, 0, 0, 0},
		.ddr_dmc_remap =
		{
		[0] = (5 | 6 << 5 | 7 << 10 | 8 << 15 | 9 << 20 | 10 << 25),
		[1] = (11 | 0 << 5 | 0 << 10 | 15 << 15 | 16 << 20 | 17 << 25),
		[2] = (18 | 19 << 5 | 20 << 10 | 21 << 15 | 22 << 20 | 23 << 25),
		[3] = (24 | 25 << 5 | 26 << 10 | 27 << 15 | 28 << 20 | 29 << 25),
		[4] = (30 | 12 << 5 | 13 << 10 | 14 << 15 | 0 << 20 | 0 << 25),
		},
		.ddr_lpddr34_ca_remap ={ 00,			   00 },
		.ddr_lpddr34_dq_remap ={ 4,			     6,			       5,	0,	 7,  1,	 2,  3,
		  15, 11,			13,	 14,	  12, 9,  8,  10,
		  21, 19,			22,	 16,	  18, 17, 23, 20,
		  29, 30,			26,	 25,	  24, 27, 31, 28 },
		.dram_rtt_nom_wr_park ={ 00,			   00 },

		/* pll ssc config:
		 *
		 *   pll_ssc_mode = (1<<20) | (1<<8) | ([strength] << 4) | [mode],
		 *      ppm = strength * 500
		 *      mode: 0=center, 1=up, 2=down
		 *
		 *   eg:
		 *     1. config 1000ppm center ss. then mode=0, strength=2
		 *        .pll_ssc_mode = (1<<20) | (1<<8) | (2 << 4) | 0,
		 *     2. config 3000ppm down ss. then mode=2, strength=6
		 *        .pll_ssc_mode = (1<<20) | (1<<8) | (6 << 4) | 2,
		 */
		//.pll_ssc_mode			= (1<<20) | (1<<8) | (2<<4) | 0,//center_ssc_1000ppm
		.ddr_func = 0,                          // DDR_FUNC,
		.magic = DRAM_CFG_MAGIC,
		.slt_test_function ={ 0x0,			0x0 }, //{0x1,0x0},enable slt 4 DRAMFreq test;{0x0,0x0},disable slt 4 DRAMFreq test;
		.fast_boot[0] = 0,
		.ac_pinmux ={ 2,	      3,			1,	 0,	  5, 4, 0, 0, 0, 0,
		  1, 3,			       5,	2,	 4, 0, 0, 0, 0 },

		.dfi_pinmux =
		{
		1,
		2,
		3,
		4,
		5,
		6,
		7,
		8,
		0,
		10,
		11,
		12,
		13,
		14,
		15,
		16,
		17,
		18,
		19,
		20,
		21,
		22,
		23,
		0,
		0,
		0,
		},
		},
#endif
};

board_clk_set_t __board_clk_setting
__attribute__ ((section(".clk_param"))) = {
	/* clock settings for bl2 */
	.cpu_clk	= CPU_CLK / 24 * 24,
#ifdef CONFIG_PXP_EMULATOR
	.pxp		= 1,
#else
	.pxp		= 0,
#endif
	.low_console_baud = CONFIG_LOW_CONSOLE_BAUD,
};

#define VCCK_A_VAL                              AML_VCCK_A_INIT_VOLTAGE
#define VCCK_B_VAL                              AML_VCCK_B_INIT_VOLTAGE
#define VDDEE_VAL                               AML_VDDEE_INIT_VOLTAGE
#define VDD_GPU_VAL                             AML_VDDGPU_INIT_VOLTAGE
#define VDD_NPU_VAL                             AML_VDDNPU_INIT_VOLTAGE
#define VDD_DDR_VAL                             AML_VDDDDR_INIT_VOLTAGE

/* VCCK_A PWM table */
#if   (VCCK_A_VAL == 1039)
#define VCCK_A_VAL_REG  0x00000022
#elif (VCCK_A_VAL == 1029)
#define VCCK_A_VAL_REG  0x00010021
#elif (VCCK_A_VAL == 1019)
#define VCCK_A_VAL_REG  0x00020020
#elif (VCCK_A_VAL == 1009)
#define VCCK_A_VAL_REG  0x0003001f
#elif (VCCK_A_VAL == 999)
#define VCCK_A_VAL_REG  0x0004001e
#elif (VCCK_A_VAL == 989)
#define VCCK_A_VAL_REG  0x0005001d
#elif (VCCK_A_VAL == 979)
#define VCCK_A_VAL_REG  0x0006001c
#elif (VCCK_A_VAL == 969)
#define VCCK_A_VAL_REG  0x0007001b
#elif (VCCK_A_VAL == 959)
#define VCCK_A_VAL_REG  0x0008001a
#elif (VCCK_A_VAL == 949)
#define VCCK_A_VAL_REG  0x00090019
#elif (VCCK_A_VAL == 939)
#define VCCK_A_VAL_REG  0x000a0018
#elif (VCCK_A_VAL == 929)
#define VCCK_A_VAL_REG  0x000b0017
#elif (VCCK_A_VAL == 919)
#define VCCK_A_VAL_REG  0x000c0016
#elif (VCCK_A_VAL == 909)
#define VCCK_A_VAL_REG  0x000d0015
#elif (VCCK_A_VAL == 899)
#define VCCK_A_VAL_REG  0x000e0014
#elif (VCCK_A_VAL == 889)
#define VCCK_A_VAL_REG  0x000f0013
#elif (VCCK_A_VAL == 879)
#define VCCK_A_VAL_REG  0x00100012
#elif (VCCK_A_VAL == 869)
#define VCCK_A_VAL_REG  0x00110011
#elif (VCCK_A_VAL == 859)
#define VCCK_A_VAL_REG  0x00120010
#elif (VCCK_A_VAL == 849)
#define VCCK_A_VAL_REG  0x0013000f
#elif (VCCK_A_VAL == 839)
#define VCCK_A_VAL_REG  0x0014000e
#elif (VCCK_A_VAL == 829)
#define VCCK_A_VAL_REG  0x0015000d
#elif (VCCK_A_VAL == 819)
#define VCCK_A_VAL_REG  0x0016000c
#elif (VCCK_A_VAL == 809)
#define VCCK_A_VAL_REG  0x0017000b
#elif (VCCK_A_VAL == 799)
#define VCCK_A_VAL_REG  0x0018000a
#elif (VCCK_A_VAL == 789)
#define VCCK_A_VAL_REG  0x00190009
#elif (VCCK_A_VAL == 779)
#define VCCK_A_VAL_REG  0x001a0008
#elif (VCCK_A_VAL == 769)
#define VCCK_A_VAL_REG  0x001b0007
#elif (VCCK_A_VAL == 759)
#define VCCK_A_VAL_REG  0x001c0006
#elif (VCCK_A_VAL == 749)
#define VCCK_A_VAL_REG  0x001d0005
#elif (VCCK_A_VAL == 739)
#define VCCK_A_VAL_REG  0x001e0004
#elif (VCCK_A_VAL == 729)
#define VCCK_A_VAL_REG  0x001f0003
#elif (VCCK_A_VAL == 719)
#define VCCK_A_VAL_REG  0x00200002
#elif (VCCK_A_VAL == 709)
#define VCCK_A_VAL_REG  0x00210001
#elif (VCCK_A_VAL == 699)
#define VCCK_A_VAL_REG  0x00220000
#else
#error "VCCK_A val out of range\n"
#endif

/* VCCK_B PWM table */
#if   (VCCK_B_VAL == 1039)
#define VCCK_B_VAL_REG  0x00000022
#elif (VCCK_B_VAL == 1029)
#define VCCK_B_VAL_REG  0x00010021
#elif (VCCK_B_VAL == 1019)
#define VCCK_B_VAL_REG  0x00020020
#elif (VCCK_B_VAL == 1009)
#define VCCK_B_VAL_REG  0x0003001f
#elif (VCCK_B_VAL == 999)
#define VCCK_B_VAL_REG  0x0004001e
#elif (VCCK_B_VAL == 989)
#define VCCK_B_VAL_REG  0x0005001d
#elif (VCCK_B_VAL == 979)
#define VCCK_B_VAL_REG  0x0006001c
#elif (VCCK_B_VAL == 969)
#define VCCK_B_VAL_REG  0x0007001b
#elif (VCCK_B_VAL == 959)
#define VCCK_B_VAL_REG  0x0008001a
#elif (VCCK_B_VAL == 949)
#define VCCK_B_VAL_REG  0x00090019
#elif (VCCK_B_VAL == 939)
#define VCCK_B_VAL_REG  0x000a0018
#elif (VCCK_B_VAL == 929)
#define VCCK_B_VAL_REG  0x000b0017
#elif (VCCK_B_VAL == 919)
#define VCCK_B_VAL_REG  0x000c0016
#elif (VCCK_B_VAL == 909)
#define VCCK_B_VAL_REG  0x000d0015
#elif (VCCK_B_VAL == 899)
#define VCCK_B_VAL_REG  0x000e0014
#elif (VCCK_B_VAL == 889)
#define VCCK_B_VAL_REG  0x000f0013
#elif (VCCK_B_VAL == 879)
#define VCCK_B_VAL_REG  0x00100012
#elif (VCCK_B_VAL == 869)
#define VCCK_B_VAL_REG  0x00110011
#elif (VCCK_B_VAL == 859)
#define VCCK_B_VAL_REG  0x00120010
#elif (VCCK_B_VAL == 849)
#define VCCK_B_VAL_REG  0x0013000f
#elif (VCCK_B_VAL == 839)
#define VCCK_B_VAL_REG  0x0014000e
#elif (VCCK_B_VAL == 829)
#define VCCK_B_VAL_REG  0x0015000d
#elif (VCCK_B_VAL == 819)
#define VCCK_B_VAL_REG  0x0016000c
#elif (VCCK_B_VAL == 809)
#define VCCK_B_VAL_REG  0x0017000b
#elif (VCCK_B_VAL == 799)
#define VCCK_B_VAL_REG  0x0018000a
#elif (VCCK_B_VAL == 789)
#define VCCK_B_VAL_REG  0x00190009
#elif (VCCK_B_VAL == 779)
#define VCCK_B_VAL_REG  0x001a0008
#elif (VCCK_B_VAL == 769)
#define VCCK_B_VAL_REG  0x001b0007
#elif (VCCK_B_VAL == 759)
#define VCCK_B_VAL_REG  0x001c0006
#elif (VCCK_B_VAL == 749)
#define VCCK_B_VAL_REG  0x001d0005
#elif (VCCK_B_VAL == 739)
#define VCCK_B_VAL_REG  0x001e0004
#elif (VCCK_B_VAL == 729)
#define VCCK_B_VAL_REG  0x001f0003
#elif (VCCK_B_VAL == 719)
#define VCCK_B_VAL_REG  0x00200002
#elif (VCCK_B_VAL == 709)
#define VCCK_B_VAL_REG  0x00210001
#elif (VCCK_B_VAL == 699)
#define VCCK_B_VAL_REG  0x00220000
#else
#error "VCCK_B val out of range\n"
#endif

/* VDDEE_VAL_REG */
#if   (VDDEE_VAL == 700)
#define VDDEE_VAL_REG   0x120000
#elif (VDDEE_VAL == 710)
#define VDDEE_VAL_REG   0x110001
#elif (VDDEE_VAL == 720)
#define VDDEE_VAL_REG   0x100002
#elif (VDDEE_VAL == 730)
#define VDDEE_VAL_REG   0xf0003
#elif (VDDEE_VAL == 740)
#define VDDEE_VAL_REG   0xe0004
#elif (VDDEE_VAL == 750)
#define VDDEE_VAL_REG   0xd0005
#elif (VDDEE_VAL == 760)
#define VDDEE_VAL_REG   0xc0006
#elif (VDDEE_VAL == 770)
#define VDDEE_VAL_REG   0xb0007
#elif (VDDEE_VAL == 780)
#define VDDEE_VAL_REG   0xa0008
#elif (VDDEE_VAL == 790)
#define VDDEE_VAL_REG   0x90009
#elif (VDDEE_VAL == 800)
#define VDDEE_VAL_REG   0x8000a
#elif (VDDEE_VAL == 810)
#define VDDEE_VAL_REG   0x7000b
#elif (VDDEE_VAL == 820)
#define VDDEE_VAL_REG   0x6000c
#elif (VDDEE_VAL == 830)
#define VDDEE_VAL_REG   0x5000d
#elif (VDDEE_VAL == 840)
#define VDDEE_VAL_REG   0x4000e
#elif (VDDEE_VAL == 850)
#define VDDEE_VAL_REG   0x3000f
#elif (VDDEE_VAL == 860)
#define VDDEE_VAL_REG   0x20010
#elif (VDDEE_VAL == 870)
#define VDDEE_VAL_REG   0x10011
#elif (VDDEE_VAL == 880)
#define VDDEE_VAL_REG   0x12
#else
#error "VDDEE val out of range\n"
#endif

/* VDD_GPU PWM table */
#if   (VDD_GPU_VAL == 700)
#define VDD_GPU_VAL_REG 0x120000
#elif (VDD_GPU_VAL == 710)
#define VDD_GPU_VAL_REG 0x110001
#elif (VDD_GPU_VAL == 720)
#define VDD_GPU_VAL_REG 0x100002
#elif (VDD_GPU_VAL == 730)
#define VDD_GPU_VAL_REG 0xf0003
#elif (VDD_GPU_VAL == 740)
#define VDD_GPU_VAL_REG 0xe0004
#elif (VDD_GPU_VAL == 750)
#define VDD_GPU_VAL_REG 0xd0005
#elif (VDD_GPU_VAL == 760)
#define VDD_GPU_VAL_REG 0xc0006
#elif (VDD_GPU_VAL == 770)
#define VDD_GPU_VAL_REG 0xb0007
#elif (VDD_GPU_VAL == 780)
#define VDD_GPU_VAL_REG 0xa0008
#elif (VDD_GPU_VAL == 790)
#define VDD_GPU_VAL_REG 0x90009
#elif (VDD_GPU_VAL == 800)
#define VDD_GPU_VAL_REG 0x8000a
#elif (VDD_GPU_VAL == 810)
#define VDD_GPU_VAL_REG 0x7000b
#elif (VDD_GPU_VAL == 820)
#define VDD_GPU_VAL_REG 0x6000c
#elif (VDD_GPU_VAL == 830)
#define VDD_GPU_VAL_REG 0x5000d
#elif (VDD_GPU_VAL == 840)
#define VDD_GPU_VAL_REG 0x4000e
#elif (VDD_GPU_VAL == 850)
#define VDD_GPU_VAL_REG 0x3000f
#elif (VDD_GPU_VAL == 860)
#define VDD_GPU_VAL_REG 0x20010
#elif (VDD_GPU_VAL == 870)
#define VDD_GPU_VAL_REG 0x10011
#elif (VDD_GPU_VAL == 880)
#define VDD_GPU_VAL_REG 0x12
#else
#error "VDD_GPU val out of range\n"
#endif

/* VDD_NPU PWM table */
#if   (VDD_NPU_VAL == 700)
#define VDD_NPU_VAL_REG 0x120000
#elif (VDD_NPU_VAL == 710)
#define VDD_NPU_VAL_REG 0x110001
#elif (VDD_NPU_VAL == 720)
#define VDD_NPU_VAL_REG 0x100002
#elif (VDD_NPU_VAL == 730)
#define VDD_NPU_VAL_REG 0xf0003
#elif (VDD_NPU_VAL == 740)
#define VDD_NPU_VAL_REG 0xe0004
#elif (VDD_NPU_VAL == 750)
#define VDD_NPU_VAL_REG 0xd0005
#elif (VDD_NPU_VAL == 760)
#define VDD_NPU_VAL_REG 0xc0006
#elif (VDD_NPU_VAL == 770)
#define VDD_NPU_VAL_REG 0xb0007
#elif (VDD_NPU_VAL == 780)
#define VDD_NPU_VAL_REG 0xa0008
#elif (VDD_NPU_VAL == 790)
#define VDD_NPU_VAL_REG 0x90009
#elif (VDD_NPU_VAL == 800)
#define VDD_NPU_VAL_REG 0x8000a
#elif (VDD_NPU_VAL == 810)
#define VDD_NPU_VAL_REG 0x7000b
#elif (VDD_NPU_VAL == 820)
#define VDD_NPU_VAL_REG 0x6000c
#elif (VDD_NPU_VAL == 830)
#define VDD_NPU_VAL_REG 0x5000d
#elif (VDD_NPU_VAL == 840)
#define VDD_NPU_VAL_REG 0x4000e
#elif (VDD_NPU_VAL == 850)
#define VDD_NPU_VAL_REG 0x3000f
#elif (VDD_NPU_VAL == 860)
#define VDD_NPU_VAL_REG 0x20010
#elif (VDD_NPU_VAL == 870)
#define VDD_NPU_VAL_REG 0x10011
#elif (VDD_NPU_VAL == 880)
#define VDD_NPU_VAL_REG 0x12
#else
#error "VDD_NPU val out of range\n"
#endif

/* VDD_DDR PWM table */
#if   (VDD_DDR_VAL == 700)
#define VDD_DDR_VAL_REG 0x120000
#elif (VDD_DDR_VAL == 710)
#define VDD_DDR_VAL_REG 0x110001
#elif (VDD_DDR_VAL == 720)
#define VDD_DDR_VAL_REG 0x100002
#elif (VDD_DDR_VAL == 730)
#define VDD_DDR_VAL_REG 0xf0003
#elif (VDD_DDR_VAL == 740)
#define VDD_DDR_VAL_REG 0xe0004
#elif (VDD_DDR_VAL == 750)
#define VDD_DDR_VAL_REG 0xd0005
#elif (VDD_DDR_VAL == 760)
#define VDD_DDR_VAL_REG 0xc0006
#elif (VDD_DDR_VAL == 770)
#define VDD_DDR_VAL_REG 0xb0007
#elif (VDD_DDR_VAL == 780)
#define VDD_DDR_VAL_REG 0xa0008
#elif (VDD_DDR_VAL == 790)
#define VDD_DDR_VAL_REG 0x90009
#elif (VDD_DDR_VAL == 800)
#define VDD_DDR_VAL_REG 0x8000a
#elif (VDD_DDR_VAL == 810)
#define VDD_DDR_VAL_REG 0x7000b
#elif (VDD_DDR_VAL == 820)
#define VDD_DDR_VAL_REG 0x6000c
#elif (VDD_DDR_VAL == 830)
#define VDD_DDR_VAL_REG 0x5000d
#elif (VDD_DDR_VAL == 840)
#define VDD_DDR_VAL_REG 0x4000e
#elif (VDD_DDR_VAL == 850)
#define VDD_DDR_VAL_REG 0x3000f
#elif (VDD_DDR_VAL == 860)
#define VDD_DDR_VAL_REG 0x20010
#elif (VDD_DDR_VAL == 870)
#define VDD_DDR_VAL_REG 0x10011
#elif (VDD_DDR_VAL == 880)
#define VDD_DDR_VAL_REG 0x12
#else
#error "VDD_DDR val out of range\n"
#endif

bl2_reg_t __bl2_reg[] __attribute__ ((section(".generic_param"))) = {
	//need fine tune
	{ 0, 0, 0xffffffff, 0, 0, 0 },
};

/* for PWM use */
/*
 * GPIOE_0   PWMAO_A    VDDEE
 * GPIOE_1   PWMAO_B    VCCK_B
 * GPIOE_3   PWMAO_D    VCCK_A
 * GPIOE_4   PWMAO_E    VDDGPU
 * GPIOE_5   PWMAO_F    VDDNPU
 * GPIOE_6   PWMAO_G    VDDDDR
 */
/* gpio/pinmux/pwm init */
register_ops_t __bl2_ops_reg[MAX_REG_OPS_ENTRIES]
__attribute__ ((section(".misc_param"))) = {
	/* demo, user defined override register */
	/* config vddee pwm - pwmao_a */
#ifdef CONFIG_PDVFS_ENABLE
	{PWM_AO_AB_PWM_A, 0x7000b, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_1, 0},
	{PWM_AO_AB_PWM_A, 0x5000d, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_2, 0},
	{PWM_AO_AB_PWM_A, 0x4000e, 0xffffffff, 0, BL2_INIT_STAGE_VDDCORE_CONFIG_3, 0},
#else
	{PWM_AO_AB_PWM_A, VDDEE_VAL_REG,   0xffffffff,    0, 0, 0 },
#endif
	{ PWM_AO_AB_MISC_REG_AB,      (0x1 << 0),      (0x1 << 0),    0, 0, 0 },
	{ CLKCTRL_PWM_CLK_AO_AB_CTRL, (0x1 << 8),      (0x7FF << 0),  0, 0, 0 },
	/* config vcck a pwm - pwmao_d */
	{ PWM_AO_CD_PWM_B,	      VCCK_A_VAL_REG,  0xffffffff,    0, 0, 0 },
	{ PWM_AO_CD_MISC_REG_AB,      (0x1 << 1),      (0x1 << 1),    0, 0, 0 },
	{ CLKCTRL_PWM_CLK_AO_CD_CTRL, (0x1 << 24),     (0x7FF << 16), 0, 0, 0 },
	/* config vcck b pwm - pwmao_b */
	{ PWM_AO_AB_PWM_B,	      VCCK_B_VAL_REG,  0xffffffff,    0, 0, 0 },
	{ PWM_AO_AB_MISC_REG_AB,      (0x1 << 1),      (0x1 << 1),    0, 0, 0 },
	{ CLKCTRL_PWM_CLK_AO_AB_CTRL, (0x1 << 24),     (0x7FF << 16), 0, 0, 0 },
	/* config vddgpu pwm - pwmao_e */
	{ PWM_AO_EF_PWM_A,	      VDD_GPU_VAL_REG, 0xffffffff,    0, 0, 0 },
	{ PWM_AO_EF_MISC_REG_AB,      (0x1 << 0),      (0x1 << 0),    0, 0, 0 },
	{ CLKCTRL_PWM_CLK_AO_EF_CTRL, (0x1 << 8),      (0x7FF << 0),  0, 0, 0 },
	/* config vddnpu pwm - pwmao_f */
	{ PWM_AO_EF_PWM_B,	      VDD_NPU_VAL_REG, 0xffffffff,    0, 0, 0 },
	{ PWM_AO_EF_MISC_REG_AB,      (0x1 << 1),      (0x1 << 1),    0, 0, 0 },
	{ CLKCTRL_PWM_CLK_AO_EF_CTRL, (0x1 << 24),     (0x7FF << 16), 0, 0, 0 },
	/* config vddddr pwm - pwmao_g */
	{ PWM_AO_GH_PWM_A,	      VDD_DDR_VAL_REG, 0xffffffff,    0, 0, 0 },
	{ PWM_AO_GH_MISC_REG_AB,      (0x1 << 0),      (0x1 << 0),    0, 0, 0 },
	{ CLKCTRL_PWM_CLK_AO_GH_CTRL, (0x1 << 8),      (0x7FF << 0),  0, 0, 0 },

	/* enable vddee */
	/* set gpioe_0 ds to max */
	{ PADCTRL_GPIOE_DS,	      (0x3 << 0),      (0x3 << 0),    0, 0, 0 },
	/* mux gpioe_0 to pwmao_a */
	{ PADCTRL_PIN_MUX_REGC,	      (0x1 << 0),      (0xf << 0),    0, 0, 0 },

	/* enable vddgpu */
	/* set gpioe_4 ds to max */
	{ PADCTRL_GPIOE_DS,	      (0x3 << 8),      (0x3 << 8),    0, 0, 0 },
	/* mux gpioe_4 to pwmao_e */
	{ PADCTRL_PIN_MUX_REGC,	      (0x1 << 16),     (0xf << 16),   0, 0, 0 },
	/* set TEST_N to high(default high) */
	//{PADCTRL_TESTN_O,	(0x1 << 0),             (0x1 << 0),	0, 0, 0},
	//{PADCTRL_TESTN_OEN,	(0x0 << 0),             (0x1 << 0),	0, 0, 0},

	/* enable vddnpu */
	/* set gpioe_5 ds to max */
	{ PADCTRL_GPIOE_DS,	      (0x3 << 10),     (0x3 << 10),   0, 0, 0 },
	/* mux gpioe_5 to pwmao_F */
	{ PADCTRL_PIN_MUX_REGC,	      (0x1 << 20),     (0xf << 20),   0, 0, 0 },

	/* enable vcck b */
	/* set gpioe_1 ds to max */
	{ PADCTRL_GPIOE_DS,	      (0x3 << 2),      (0x3 << 2),    0, 0, 0 },
	/* mux gpioe_1 to pwmao_b */
	{ PADCTRL_PIN_MUX_REGC,	      (0x1 << 4),      (0xf << 4),    0, 0, 0 },

	/* enable vcck a */
	/* set gpioe_3 ds to max */
	{ PADCTRL_GPIOE_DS,	      (0x3 << 6),      (0x3 << 6),    0, 0, 0 },
	/* mux gpioe_3 to pwmao_d */
	{ PADCTRL_PIN_MUX_REGC,	      (0x1 << 12),     (0xf << 12),   0, 0, 0 },
	/* set gpiod_2 to high(default high) */
	//{PADCTRL_GPIOD_O,	(0x1 << 2),             (0x1 << 2), 0, 0, 0},
	//{PADCTRL_GPIOD_OEN, (0x0 << 2),               (0x1 << 2), 0, 0, 0},
	//{PADCTRL_PIN_MUX_REGA,	(0x0 << 8),             (0xf << 8), 0, 0, 0},

	/* enable vddddr */
	/* set gpioe_6 ds to max */
	{ PADCTRL_GPIOE_DS,	      (0x3 << 12),     (0x3 << 12),   0, 0, 0 },
	/* mux gpioe_6 to pwmao_g */
	{ PADCTRL_PIN_MUX_REGC,	      (0x1 << 24),     (0xf << 24),   0, 0, 0 },
};

/* for all the storage parameter */
storage_parameter_t __store_para __attribute__ ((section(".store_param"))) = {
	.common					={
		.version			= 0x01,
		.device_fip_container_size	= 0x280000,
		.device_fip_container_copies	= 4,
	},
	.nand					={
		.version			= 0x01,
		.bbt_pages			= 0x1,
		.bbt_start_block		= 0x20,
		.discrete_mode			= 1,
		.setup_data.nand_setup_data	= (2 << 20) |		    \
						  (0 << 19) |			  \
						  (1 << 17) |			  \
						  (4 << 14) |			  \
						  (0 << 13) |			  \
						  (64 << 6) |			  \
						  (4 << 0),
		.reserved_area_blk_cnt		= 48,
		.page_per_block			= 128,
		.use_param_page_list		= 0,
	},
};

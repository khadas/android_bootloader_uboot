/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __BOARD_CFG_H__
#define __BOARD_CFG_H__

#include <asm/arch/cpu.h>
#include <amlogic/base_env.h>

/*
 * platform power init config
 */

#define AML_VCCK_INIT_VOLTAGE	  1009		//VCCK power up voltage
#define AML_VDDEE_INIT_VOLTAGE	800	   // VDDEE power up voltage
/*Distinguish whether to use efuse to adjust vddee*/
#define CONFIG_PDVFS_ENABLE

/* SMP Definitions */
#define CPU_RELEASE_ADDR		secondary_boot_func

/* Serial config */
#define CONFIG_CONS_INDEX 2
#define CONFIG_BAUDRATE  115200

/*if disable uboot console, enable it*/
//#define CONFIG_SILENT_CONSOLE
#ifdef CONFIG_SILENT_CONSOLE
#undef CONFIG_SILENT_CONSOLE_UPDATE_ON_RELOC
#endif

/*low console baudrate*/
#define CONFIG_LOW_CONSOLE_BAUD			0

/* Enable ir remote wake up for bl30 */
#define AML_IR_REMOTE_POWER_UP_KEY_VAL1 0xef10fe01 //amlogic tv ir --- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL2 0XBB44FB04 //amlogic tv ir --- ch+
#define AML_IR_REMOTE_POWER_UP_KEY_VAL3 0xF20DFE01 //amlogic tv ir --- ch-
#define AML_IR_REMOTE_POWER_UP_KEY_VAL4 0XBA45BD02 //amlogic small ir--- power
#define AML_IR_REMOTE_POWER_UP_KEY_VAL5 0xe51afb04
#define AML_IR_REMOTE_POWER_UP_KEY_VAL6 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL7 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL8 0xFFFFFFFF
#define AML_IR_REMOTE_POWER_UP_KEY_VAL9 0xFFFFFFFF

/*config the default parameters for adc power key*/
#define AML_ADC_POWER_KEY_CHAN   2  /*channel range: 0-7*/
#define AML_ADC_POWER_KEY_VAL	0  /*sample value range: 0-1023*/

#ifdef CONFIG_DTB_LOAD
#undef CONFIG_DTB_LOAD
#endif

#define CONFIG_DTB_LOAD \
	"if test ${boot_source} = emmc; then "\
		"echo Load dtb/${fdtfile} from eMMC (1:1) ...;" \
		"load mmc 1:4 ${dtb_mem_addr} dtb/${fdtfile};" \
	"else if test ${boot_source} = sd; then "\
		"echo Load dtb/${fdtfile} from SD (0:1) ...;" \
		"load mmc 0:4 ${dtb_mem_addr} dtb/${fdtfile};" \
	"fi;fi;"

/* Bootloader Control Block function
*  That is used for recovery and the bootloader to talk to each other
*/
#ifndef CONFIG_PXP_DDR
#define CONFIG_BOOTLOADER_CONTROL_BLOCK
#endif// #ifndef CONFIG_PXP_DDR

/* AVB */
#define CONFIG_AML_AVB2_ANTIROLLBACK 1
#define CONFIG_AVB_VERIFY 1
#define CONFIG_SUPPORT_EMMC_RPMB 1
#define CONFIG_AML_DEV_ID 1

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#ifndef BOOT_TARGET_DEVICES
#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)
#endif

#include <config_distro_bootcmd.h>

/* args/envs */
#define CONFIG_SYS_MAXARGS  64
#define CONFIG_EXTRA_ENV_SETTINGS \
	CONFIG_EXTRA_ENV_SETTINGS_BASE \
	"dv_fw_addr=0xa00000\0"\
	"otg_device=1\0" \
	"panel_type=lcd_1\0" \
	"outputmode=1080p60hz\0" \
	"hdmimode=none\0" \
	"colorattribute=444,8bit\0"\
	"cvbsmode=576cvbs\0" \
	"vout_init=enable\0" \
	"display_width=1920\0" \
	"display_height=1080\0" \
	"display_bpp=24\0" \
	"display_color_index=24\0" \
	"display_layer=osd0\0" \
	"display_color_fg=0xffff\0" \
	"display_color_bg=0\0" \
	"dtb_mem_addr=0x01000000\0" \
	"fdt_addr_r=0x01000000\0"\
	"fdtoverlay_addr_r=0x00a00000\0"\
	"fdtaddr=0x01000000\0"\
	"kernel_addr_r=0x01080000\0"\
	"pxefile_addr_r=0x00010000\0"\
	"scriptaddr=0x00010000\0" \
	"ramdisk_addr_r=0x10000000\0"\
	"kernel_comp_addr_r=0x0d080000\0"\
	"kernel_comp_size=0x2000000\0"\
	"pxeuuid=00000000-0000-0000-0000-000000000000\0"\
	"bootfile=\0"\
	"fdtfile=s4_s905y4_ap222_debian.dtb\0" \
	"fb_width=1920\0" \
	"fb_height=1080\0" \
	"hdmichecksum=0x00000000\0" \
	"frac_rate_policy=1\0" \
	"hdr_policy=0\0" \
	"usb_burning=" CONFIG_USB_TOOL_ENTRY "\0" \
	"fdt_high=0x20000000\0"\
	"sdcburncfg=aml_sdc_burn.ini\0"\
	"EnableSelinux=enforcing\0" \
	"recovery_part=recovery\0"\
	"loglevel=8\0" \
	"lock=10101000\0"\
	"cvbs_drv=0\0"\
	"osd_reverse=0\0"\
	"video_reverse=0\0"\
	"board=oppen\0"\
	"upgrade_key_flag=0\0"\
	"initargs="\
		"init=data=writeback rw rootfstype=ext4" CONFIG_KNL_LOG_LEVEL " "\
		"console=ttyS0,921600 console=tty0 no_console_suspend "\
		"earlycon=aml-uart,0xfe07a000 ramoops.pstore_en=1 ramoops.record_size=0x8000 "\
		"ramoops.console_size=0x4000 loop.max_part=4 scramble_reg=0xfe02e030 "\
		"scsi_mod.scan=async xhci_hcd.quirks=0x800000 gamma=0 boot_source=emmc "\
		"cma_first_wm_low=on "\
		"\0"\
	"upgrade_check="\
		"run upgrade_check_base;"\
		"\0"\
	"storeargs="\
		"get_bootloaderversion;" \
		"run storeargs_base;"\
		"setenv bootargs ${bootargs} ${emmc_quirks} kvm-arm.mode=none init_on_alloc=0;"\
		"run cmdline_keys;"\
		"\0"\
	"switch_bootmode="\
		"get_rebootmode;"\
		"if test ${reboot_mode} = factory_reset; then "\
			"run recovery_from_flash;"\
		"else if test ${reboot_mode} = update; then "\
			"run update;"\
		"else if test ${reboot_mode} = quiescent; then "\
			"setenv bootconfig ${bootconfig} androidboot.quiescent=1;"\
			"setenv vout_init enable;"\
		"else if test ${reboot_mode} = recovery_quiescent; then "\
			"setenv bootconfig ${bootconfig} androidboot.quiescent=1;"\
			"setenv vout_init enable;"\
			"run recovery_from_flash;"\
		"else if test ${reboot_mode} = cold_boot; then "\
		"else if test ${reboot_mode} = fastboot; then "\
			"fastboot 0;"\
		"fi;fi;fi;fi;fi;fi;"\
		"\0"\
	"storeboot="\
		"run storeboot_base;"\
		"\0"\
	"update="\
		"run update_base;"\
		"\0"\
	"enter_fastboot="\
		"fastboot 0;"\
		"\0"\
	"recovery_from_fat_dev="\
		"run recovery_from_fat_dev_base;"\
		"\0"\
	"recovery_from_udisk="\
		"run recovery_from_udisk_base;"\
		"\0"\
	"recovery_from_sdcard="\
		"run recovery_from_sdcard_base;"\
		"\0"\
	"recovery_from_flash="\
		"store param;"\
		"setenv bootargs ${bootargs} ${mtdbootparts}; "\
		"run recovery_from_flash_base;"\
		"\0"\
	"bcb_cmd="\
		"get_avb_mode;"\
		"get_valid_slot;"\
		"fdt addr ${fdtaddr}; "\
		"load mmc 1:4 ${fdtaddr} dtb.img;"\
		"\0"\
	"load_bmp_logo="\
		"if load mmc 1:4 ${loadaddr} /usr/share/amlbian/logo/logo.bmp; then "\
			"bmp display ${loadaddr};"\
			"bmp scale;"\
		"fi;"\
		"\0"\
	"init_display="\
		"get_rebootmode;"\
		"echo reboot_mode:::: ${reboot_mode};"\
		"setenv reboot_mode_android ""normal"";"\
		"setenv initconfig androidboot.bootreason=${reboot_mode};"\
		"hdmitx hpd;hdmitx get_parse_edid;"\
		"dovi process;setenv display_layer osd0;osd open;osd clear;run load_bmp_logo;"\
		"bmp scale;vout output ${outputmode};dovi set;dovi pkg;vpp hdrpkt;"\
		"\0"\
	"storage_param="\
		"setenv bootargs ${bootargs} ${emmc_quirks}; "\
		"run storage_param_base;"\
		"\0"\
	"cmdline_keys="\
		"setenv usid ap222${cpu_id};"\
		"run cmdline_keys_base;"\
		"\0"\
	"upgrade_key="\
		"run upgrade_key_base;"\
		"echo usr key;"\
		"\0"\
	BOOTENV\
	"pxe_boot=dhcp; pxe get && pxe boot\0"\
	"bootcmd_storeboot=run storeboot\0"\
	"boot_targets=spi usb0 mmc0 mmc1 storeboot pxe dhcp\0"

#ifndef CONFIG_PXP_DDR
#define CONFIG_PREBOOT  \
"run upgrade_check;"\
"run init_display;"\
"run storeargs;"\
"run upgrade_key;" \
"bcb uboot-command;"\
"run switch_bootmode;"
#else
#define CONFIG_PREBOOT  "echo preboot"
#endif
/* #define CONFIG_ENV_IS_NOWHERE  1 */
#define CONFIG_ENV_SIZE   (64 * 1024)
#define CONFIG_FIT 1
#define CONFIG_OF_LIBFDT 1
#define CONFIG_ANDROID_BOOT_IMAGE 1
#define CONFIG_SYS_BOOTM_LEN (64 << 20) /* Increase max gunzip size*/

/* ATTENTION */
/* DDR configs move to board/amlogic/[board]/firmware/timing.c */

/* running in sram */
//#define UBOOT_RUN_IN_SRAM
#ifdef UBOOT_RUN_IN_SRAM
#define CONFIG_SYS_INIT_SP_ADDR				(0x00200000)
/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN				(256 * 1024)
#else
#define CONFIG_SYS_INIT_SP_ADDR				(0x00200000)
#define CONFIG_SYS_MALLOC_LEN				(96 * 1024 * 1024)
#endif

//#define CONFIG_NR_DRAM_BANKS			1
/* ddr functions */
#define DDR_FULL_TEST			0 //0:disable, 1:enable. ddr full test
#define DDR_LOW_POWER			0 //0:disable, 1:enable. ddr clk gate for lp
#define DDR_ZQ_PD				0 //0:disable, 1:enable. ddr zq power down
#define DDR_USE_EXT_VREF		 0 //0:disable, 1:enable. ddr use external vref
#define DDR4_TIMING_TEST		 0 //0:disable, 1:enable. ddr4 timing test function
#define DDR_PLL_BYPASS		   0 //0:disable, 1:enable. ddr pll bypass function

/* storage: emmc/nand/sd */
#define CONFIG_ENV_OVERWRITE
/* #define	 CONFIG_CMD_SAVEENV */
/* fixme, need fix*/

#if (defined(CONFIG_ENV_IS_IN_AMLNAND) || defined(CONFIG_ENV_IS_IN_MMC)) && \
defined(CONFIG_STORE_COMPATIBLE)
#error env in amlnand/mmc already be compatible;
#endif

/*
 *				storage
 *		|---------|---------|
 *		|					|
 *		emmc<--Compatible-->nand
 *					|-------|-------|
 *					|		|
 *					MTD<-Exclusive->NFTL
 *					|
 *			|***************|***************|
 *			slc-nand	SPI-nand	SPI-nor
 *			(raw nand)
 */
/* axg only support slc nand */
/* swither for mtd nand which is for slc only. */

#if defined(CONFIG_AML_NAND) && defined(CONFIG_MESON_NFC)
#error CONFIG_AML_NAND/CONFIG_MESON_NFC can not support at the sametime;
#endif

#if (defined(CONFIG_AML_NAND) || defined(CONFIG_MESON_NFC)) && defined(CONFIG_MESON_FBOOT)
#error CONFIG_AML_NAND/CONFIG_MESON_NFC CONFIG _MESON_FBOOT can not support at the sametime;
#endif

#if defined(CONFIG_SPI_NAND) && defined(CONFIG_MTD_SPI_NAND) && defined(CONFIG_MESON_NFC)
#error CONFIG_SPI_NAND/CONFIG_MTD_SPI_NAND/CONFIG_MESON_NFC can not support at the sametime;
#endif

/* #define CONFIG_AML_SD_EMMC 1 */
#ifdef CONFIG_AML_SD_EMMC
	#define CONFIG_GENERIC_MMC 1
	#define CONFIG_CMD_MMC 1
	#define CONFIG_CMD_GPT 1
	#define CONFIG_SYS_MMC_ENV_DEV 1
	#define CONFIG_EMMC_DDR52_EN 0
	#define CONFIG_EMMC_DDR52_CLK 35000000
#endif
#define		CONFIG_PARTITIONS 1

#if defined CONFIG_MESON_NFC || defined CONFIG_SPI_NAND || defined CONFIG_MTD_SPI_NAND
	#define CONFIG_SYS_MAX_NAND_DEVICE  2
#endif

/* vpu */
#define AML_VPU_CLK_LEVEL_DFT 7

/* osd */
#define OSD_SCALE_ENABLE
#define AML_OSD_HIGH_VERSION

/* USB
 * Enable CONFIG_MUSB_HCD for Host functionalities MSC, keyboard
 * Enable CONFIG_MUSB_UDD for Device functionalities.
 */
/* #define CONFIG_MUSB_UDC		1 */
/* #define CONFIG_CMD_USB 1 */

#define USB_PHY2_PLL_PARAMETER_1	0x09400414
#define USB_PHY2_PLL_PARAMETER_2	0x927e0000
#define USB_PHY2_PLL_PARAMETER_3	0xAC5F49E5

#define USB_G12x_PHY_PLL_SETTING_1	(0xfe18)
#define USB_G12x_PHY_PLL_SETTING_2	(0xfff)
#define USB_G12x_PHY_PLL_SETTING_3	(0x78000)
#define USB_G12x_PHY_PLL_SETTING_4	(0xe0004)
#define USB_G12x_PHY_PLL_SETTING_5	(0xe000c)

#define AML_TXLX_USB		1
#define AML_USB_V2			 1
#define USB_GENERAL_BIT		 3
#define USB_PHY21_BIT		   4

/* net */
/* #define CONFIG_CMD_NET   1 */
#define CONFIG_ETH_DESIGNWARE
#if defined(CONFIG_CMD_NET)
	#define CONFIG_DESIGNWARE_ETH 1
	#define CONFIG_PHYLIB	1
	#define CONFIG_NET_MULTI 1
	#define CONFIG_CMD_PING 1
	#define CONFIG_CMD_DHCP 1
	#define CONFIG_CMD_RARP 1
	#define CONFIG_HOSTNAME	"arm_gxbb"
	#define CONFIG_ETHADDR		 (00 : 15 : 18 : 01 : 81 : 31)   /* Ethernet address */
	#define CONFIG_IPADDR		  (10.18.9.97)		  /* Our ip address */
	#define CONFIG_GATEWAYIP	   (10.18.9.1)		   /* Our getway ip address */
	#define CONFIG_SERVERIP		(10.18.9.113)		 /* Tftp server ip address */
	#define CONFIG_NETMASK		 (255.255.255.0)
#endif /* (CONFIG_CMD_NET) */

#define MAC_ADDR_NEW  1

/* other devices */
#define CONFIG_SHA1 1
#define CONFIG_MD5 1

/* commands */
/* #define CONFIG_CMD_PLLTEST 1 */

/*file system*/
#define CONFIG_DOS_PARTITION 1
#define CONFIG_EFI_PARTITION 1
/* #define CONFIG_MMC 1 */
#define CONFIG_FS_FAT 1
#define CONFIG_FS_EXT4 1
#define CONFIG_LZO 1

#define CONFIG_FAT_WRITE 1
#define CONFIG_AML_FACTORY_PROVISION 1
#define CONFIG_NAND_FACTORY_PROVISION 1

#define CONFIG_AML_WATERMARK 1

/* Cache Definitions */
/* #define CONFIG_SYS_DCACHE_OFF */
/* #define CONFIG_SYS_ICACHE_OFF */

/* other functions */
#define CONFIG_LIBAVB		1

/* define CONFIG_SYS_MEM_TOP_HIDE 8M space for free buffer */
#define CONFIG_SYS_MEM_TOP_HIDE		0x00800000

#define CONFIG_CPU_ARMV8

/* define CONFIG_UPDATE_MMU_TABLE for need update mmu */
#define CONFIG_UPDATE_MMU_TABLE

//use sha2 command
#define CONFIG_CMD_SHA2

//use hardware sha2
//#define CONFIG_AML_HW_SHA2

//Replace avb2 software SHA256 to utilize armce
#define CONFIG_AVB2_UBOOT_SHA256

/* support secure boot */
#define CONFIG_AML_SECURE_UBOOT   1

#if defined(CONFIG_AML_SECURE_UBOOT)

/* unify build for generate encrypted bootloader "u-boot.bin.encrypt" */
#define CONFIG_AML_CRYPTO_UBOOT   1

#ifdef CONFIG_AB_UPDATE
#define CONFIG_AML_SIGNED_UBOOT   1
#endif

/* unify build for generate encrypted kernel image
 *  SRC : "board/amlogic/(board)/boot.img"
 *  DST : "fip/boot.img.encrypt"
 */
/* #define CONFIG_AML_CRYPTO_IMG	1 */

#endif /* CONFIG_AML_SECURE_UBOOT */

#define CONFIG_FIP_IMG_SUPPORT  1

/* config ramdump to debug kernel panic */
#ifndef CONFIG_AB_UPDATE
#define CONFIG_FULL_RAMDUMP
#endif

#define BL32_SHARE_MEM_SIZE  0x800000
#define CONFIG_AML_KASLR_SEED
#endif

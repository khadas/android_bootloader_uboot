
/*
 * arch/arm/include/asm/arch-txl/bl31_apis.h
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

/*
 * Trustzone API
 *
 * Copyright (C) 2012 Amlogic, Inc.
 *
 * Author: Platform-SH@amlogic.com
 *
 */

#ifndef __GXBB_BL31_APIS_H
#define __GXBB_BL31_APIS_H

#include <asm/arch/io.h>
#include <amlogic/image_check.h>

/*#define SRAM_READ				0x82000010
#define CORE_RD_REV1			0x82000011
#define SRAM_ACS_READ		0x82000012
#define SRAM_ACS_INDIRECT_READ		0x82000013*/

#define GET_SHARE_MEM_INPUT_BASE		0x82000020
#define GET_SHARE_MEM_OUTPUT_BASE		0x82000021
#define GET_REBOOT_REASON				0x82000022
#define GET_SHARE_STORAGE_IN_BASE		0x82000023
#define GET_SHARE_STORAGE_OUT_BASE		0x82000024
#define GET_SHARE_STORAGE_BLOCK_BASE	0x82000025
#define GET_SHARE_STORAGE_MESSAGE_BASE	0x82000026
#define GET_SHARE_STORAGE_BLOCK_SIZE	0x82000027
#define SET_STORAGE_INFO				0x82000028
#define SET_STORAGE_BOOTSEQUENCE		0x82000029
#define SET_REBOOT_REASON				0x82000049

/* Set Reboot Reason then Reboot*/
#define PSCI_SYS_REBOOT		0x84000009

/* SECUREOS DEFINITION*/
/* SMC Identifiers for non-secure world functions */
#define CALL_TRUSTZONE_HAL_API                  0x5
#define GET_SHARE_MEM_INFORMATION		0x8200002a
	#define GET_SHARE_VERSION_ADDR		 0x3	//for secure use
	#define GET_SHARE_VERSION_DATA		 0x4	//for none secure use

/* EFUSE */
#define EFUSE_READ					0x82000030
#define EFUSE_WRITE				0x82000031
#define EFUSE_WRITE_PATTERN		0x82000032
#define EFUSE_USER_MAX    0x82000033
#define EFUSE_OBJ_READ    0x8200003B
#define EFUSE_OBJ_WRITE   0x8200003C
#define EFUSE_READ_CALI    0x8200003D
#define EFUSE_READ_CALI_ITEM    0x8200003E
	#define EFUSE_CALI_SUBITEM_WHOBURN       0x100
	#define EFUSE_CALI_SUBITEM_SENSOR0       0x101
	#define EFUSE_CALI_SUBITEM_SARADC        0x102
	#define EFUSE_CALI_SUBITEM_USBPHY        0x103
	#define EFUSE_CALI_SUBITEM_MIPICSI       0x104
	#define EFUSE_CALI_SUBITEM_HDMIRX        0x105
	#define EFUSE_CALI_SUBITEM_ETHERNET      0x106
	#define EFUSE_CALI_SUBITEM_CVBS          0x107
	#define EFUSE_CALI_SUBITEM_EARCRX        0x108
	#define EFUSE_CALI_SUBITEM_EARCTX        0x109
	#define EFUSE_CALI_SUBITEM_USBCCLOGIC    0x10A
	#define EFUSE_CALI_SUBITEM_BC            0x10B

	#define EFUSE_LOCK_SUBITEM_DGPK1_KEY     0x1000
	#define EFUSE_LOCK_SUBITEM_DGPK2_KEY     0x1001
	#define EFUSE_LOCK_SUBITEM_AUDIO_V_ID    0x1002

#define DEBUG_EFUSE_WRITE_PATTERN	0x820000F0
#define DEBUG_EFUSE_READ_PATTERN	0x820000F1

/* JTAG*/
#define JTAG_ON                                0x82000040
#define JTAG_OFF                               0x82000041

#define SET_USB_BOOT_FUNC	0x82000043
	/* USB BOOT FUNC sub command list*/
	#define CLEAR_USB_BOOT			1
	#define FORCE_USB_BOOT			2
	#define RUN_COMD_USB_BOOT		3
	#define PANIC_DUMP_USB_BOOT	4

#define GET_CHIP_ID			0x82000044

/* tsensor calibration data */
#define TSENSOR_CALI_SET       0x8200004C
#define TSENSOR_CALI_READ      0x82000047

/*oscring efuse value get */
#define OSCRING_EFUSE_GET       0x8200004D
/* Security Key*/
#define SECURITY_KEY_QUERY	0x82000060
#define SECURITY_KEY_READ	0x82000061
#define SECURITY_KEY_WRITE	0x82000062
#define SECURITY_KEY_TELL		0x82000063
#define SECURITY_KEY_VERIFY	0x82000064
#define SECURITY_KEY_STATUS	0x82000065
#define SECURITY_KEY_NOTIFY	0x82000066
#define SECURITY_KEY_LIST		0x82000067
#define SECURITY_KEY_REMOVE	0x82000068
#define SECURITY_KEY_NOTIFY_EX	0x82000069
#define SECURITY_KEY_SET_ENCTYPE	0x8200006A
#define SECURITY_KEY_GET_ENCTYPE	0x8200006B
#define SECURITY_KEY_VERSION		0x8200006C

/*viu probe en*/
#define VIU_PREOBE_EN		0x82000080
/*set boot first timeout*/
#define SET_BOOT_FIRST		0x82000087

/* KEYMASTER */
#define SET_BOOT_PARAMS		0x82000072

/* Secure HAL APIs */
#define TRUSTZONE_HAL_API_SRAM                  0x400

/*start hifi4 */
#define START_HIFI4			0x82000090
#define DSP_SEC_POWERSET		0x82000092


#define SRAM_HAL_API_CHECK_EFUSE 0x403
struct sram_hal_api_arg {
	unsigned int cmd;
	unsigned int req_len;
	unsigned int res_len;
	unsigned long req_phy_addr;
	unsigned long res_phy_addr;
	unsigned long ret_phy_addr;
};

#define JTAG_STATE_ON  0
#define JTAG_STATE_OFF 1
#define JTAG_M3_AO     0
#define JTAG_M3_EE     1
#define JTAG_A53_AO    2
#define JTAG_A53_EE 3
#define CLUSTER_BIT 2


/* AVB2 */
#define GET_AVBKEY_FROM_FIP              0x820000b0

/////////////////////////////////////////////////////////////////////////////////
#define AML_DATA_PROCESS                 (0x820000FF)
	#define AML_D_P_W_EFUSE_SECURE_BOOT  (0x10)
	#define AML_D_P_W_EFUSE_PASSWORD     (0x11)
	#define AML_D_P_W_EFUSE_CUSTOMER_ID  (0x12)
	#define AML_D_P_W_EFUSE_AMLOGIC 	 (0x20)
	#define AML_D_P_IMG_DECRYPT          (0x40)
	#define AML_D_P_UPGRADE_CHECK        (0x80)

#define GXB_EFUSE_PATTERN_SIZE      (0x2 << 9)
#define GXB_IMG_SIZE                (24<<20)
#define GXB_IMG_LOAD_ADDR           (0x1080000)
	#define GXB_IMG_DEC_KNL   (1<<0)
	#define GXB_IMG_DEC_RMD   (1<<1)
	#define GXB_IMG_DEC_DTB   (1<<2)
	#define GXB_IMG_DEC_ALL   (GXB_IMG_DEC_KNL|GXB_IMG_DEC_RMD|GXB_IMG_DEC_DTB)

void aml_set_jtag_state(unsigned state, unsigned select);
unsigned aml_get_reboot_reason(void);
unsigned aml_reboot(uint64_t function_id, uint64_t arg0, uint64_t arg1, uint64_t arg2);
void aml_set_bootsequence(uint32_t val);
void aml_set_reboot_reason(uint64_t function_id, uint64_t arg0, uint64_t arg1, uint64_t arg2);
unsigned long aml_sec_boot_check(unsigned long ,unsigned long ,unsigned long,unsigned long );
long get_sharemem_info(unsigned long);
void set_usb_boot_function(unsigned long command);
void aml_system_off(void);

void bl31_get_chipid(unsigned int *, unsigned int *,
	unsigned int *, unsigned int *);
void set_viu_probe_enable(void);
void wdt_send_cmd_to_bl31(uint64_t cmd, uint64_t value);
void power_set_dsp(unsigned int id, unsigned int powerflag);
void init_dsp(unsigned int id,unsigned int addr,unsigned int cfg0);
void set_boot_first_timeout(uint64_t arg0);
int bl31_get_cornerinfo(uint8_t *outbuf, int size);
int32_t set_boot_params(const keymaster_boot_params*);
int32_t get_avbkey_from_fip(uint8_t *buf, uint32_t buflen);
int32_t aml_get_bootloader_version(uint8_t *outbuf);
#endif

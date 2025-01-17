/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * include/rtk_fw.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __RTLFW_H__
#define __RTLFW_H__

#include <usb.h>
#define log_level  0
#define DEL_CODE 0

#define	ENODEV		19	/* No such device */
#define HCI_CMD_READ_BD_ADDR 0x1009
#define HCI_VENDOR_CHANGE_BDRATE 0xfc17
#define HCI_VENDOR_READ_RTK_ROM_VERSION 0xfc6d
#define HCI_VENDOR_READ_LMP_VERSION 0x1001
#define HCI_VENDOR_CHIP_RESET 0x0C03
#define HCI_VENDOR_SET_WAKE_UP_DEVICE 0xfc7b
#define HCI_VENDOR_FORCE_RESET_AND_PATCHABLE    0xFC66

/*******************************
 **    Reasil patch code
 ********************************/
#define CMD_CMP_EVT		0x0e
#define PKT_LEN			300
#define MSG_TO			1000
#define PATCH_SEG_MAX	252
#define DATA_END		0x80
#define DOWNLOAD_OPCODE	0xfc20
#define BTOFF_OPCODE	0xfc28
#define TRUE			1
#define FALSE			0
#define CMD_HDR_LEN		sizeof(struct hci_command_hdr)
#define EVT_HDR_LEN		sizeof(struct hci_event_hdr)
#define CMD_CMP_LEN		sizeof(struct hci_ev_cmd_complete)
#define RTK_PATCH_LENGTH_MAX 1024 * 24

#define CHIP_TYPE_RTL8723BU "fw_8723b_b.txt"
#define CHIP_TYPE_RTL8723DU "fw_8723du_c.txt"
#define CHIP_TYPE_RTL8822BU "fw_8822b_b.txt"
#define CHIP_TYPE_RTL88X2CU "fw_rtl8822c_d.txt"

#define CODE_MAXSIZE_24K    (1024 * 24)   //24K
#define CODE_MAXSIZE_40K    (1024 * 40)   //40K
#define CODE_MAXSIZE_23K    (23639)   //56 * 10K

enum rtk_endpoit {
	CTRL_EP = 0,
	INTR_EP = 1,
	BULK_EP = 2,
	ISOC_EP = 3
};

#define MAX_URBS_QUEUED 5
struct hci_command_hdr {
	__le16	opcode;		/* OCF & OGF */
	unsigned char	plen;
} __packed;

struct hci_event_hdr {
	unsigned char	evt;
	unsigned char	plen;
} __packed;
#define HCI_EV_CMD_COMPLETE		0x0e
struct hci_ev_cmd_complete {
	unsigned char     ncmd;
	__le16   opcode;
} __packed;

struct rtk_eversion_evt {
	unsigned char status;
	unsigned char version;
} __attribute__ ((packed));

struct rtk_localversion_evt {
	unsigned char status;
	unsigned char hci_version;
	unsigned int hci_revision;
	unsigned char lmp_version;
	unsigned int lmp_manufacture;
	unsigned int lmp_subversion;
} __attribute__ ((packed));

struct rtk_epatch_entry {
	unsigned short chip_id;
	unsigned short patch_length;
	unsigned int start_offset;
	unsigned int coex_version;
	unsigned int svn_version;
	unsigned int fw_version;
} __attribute__ ((packed));

struct rtk_epatch {
	unsigned char signature[8];
	unsigned int fw_version;
	unsigned short number_of_total_patch;
	struct rtk_epatch_entry entry[0];
} __attribute__ ((packed));

struct rtk_extension_entry {
	unsigned char opcode;
	unsigned char length;
	unsigned char *data;
} __attribute__ ((packed));

typedef struct {
	unsigned short	prod_id;
	unsigned short	lmp_sub_default;
	unsigned short	lmp_sub;
	unsigned short	eversion;
	char		*mp_patch_name;
	char		*patch_name;
	char		*config_name;
	unsigned char		*fw_cache;
	int			fw_len;
} patch_info;

typedef struct {
	unsigned char index;
	unsigned char data[PATCH_SEG_MAX];
} __attribute__((packed)) download_cp;

typedef struct {
	unsigned char status;
	unsigned char index;
} __attribute__((packed)) download_rp;

typedef struct {
	struct usb_interface	*intf;
	struct usb_device		*udev;
	patch_info *patch_entry;
	int			pipe_in, pipe_out;
	unsigned char		*send_pkt;
	unsigned char		*rcv_pkt;
	struct hci_command_hdr		*cmd_hdr;
	struct hci_event_hdr		*evt_hdr;
	struct hci_ev_cmd_complete	*cmd_cmp;
	unsigned char		*req_para,	*rsp_para;
	unsigned char		*fw_data;
	int			pkt_len;
	int			fw_len;
} firmware_info;

typedef struct {
	unsigned short idVendor;
	unsigned short idProduct;
	char *usb_module;
	char *fw_name;
	unsigned int fw_size;
	unsigned char *fw_buf;
} bluetooth_dongle;

void load_rtl_firmware(void);
struct usb_device *get_rtl_dev(void);
int load_rtl_firmware_dev(struct usb_device *dev);
struct usb_device *rtk_usb_get_dev_index(int index);

#endif

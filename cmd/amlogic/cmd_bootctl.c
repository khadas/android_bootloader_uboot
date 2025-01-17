// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <malloc.h>
#ifdef CONFIG_AML_MTD
#include <linux/mtd/mtd.h>
#endif
#include <asm/byteorder.h>
#include <config.h>
#include <asm/arch/io.h>
#include <partition_table.h>
#include <version.h>
#ifdef CONFIG_UNIFY_BOOTLOADER
#include "cmd_bootctl_wrapper.h"
#endif
#include "cmd_bootctl_utils.h"

extern int nand_store_write(const char *name, loff_t off, size_t size, void *buf);

#ifdef CONFIG_BOOTLOADER_CONTROL_BLOCK
extern int store_read_ops(
		unsigned char *partition_name,
		unsigned char *buf, uint64_t off, uint64_t size);
extern int store_write_ops(
		unsigned char *partition_name,
		unsigned char *buf, uint64_t off, uint64_t size);

#define COMMANDBUF_SIZE  32
#define STATUSBUF_SIZE   32
#define RECOVERYBUF_SIZE 768

#define BOOTINFO_OFFSET  864
#define SLOTBUF_SIZE     32
#define MISCBUF_SIZE     1088

struct bootloader_message {
	char command[32];
	char status[32];
	char recovery[768];
	// The 'recovery' field used to be 1024 bytes.  It has only ever
	// been used to store the recovery command line, so 768 bytes
	// should be plenty.  We carve off the last 256 bytes to store the
	// stage string (for multistage packages) and possible future
	// expansion.
	char stage[32];
	char slot_suffix[32];
	char reserved[192];
};

typedef struct BrilloSlotInfo {
	uint8_t bootable;
	uint8_t online;
	uint8_t reserved[2];
} BrilloSlotInfo;

typedef struct BrilloBootInfo {
	// Used by fs_mgr. Must be NUL terminated.
	char bootctrl_suffix[4];

	// Magic for identification - must be 'B', 'C', 'c' (short for
	// "boot_control copy" implementation).
	uint8_t magic[3];

	// Version of BrilloBootInfo struct, must be 0 or larger.
	uint8_t version;

	// Currently active slot.
	uint8_t active_slot;

	// Information about each slot.
	BrilloSlotInfo slot_info[2];
	uint8_t attemp_times;

	uint8_t reserved[14];
} BrilloBootInfo;

#ifdef CONFIG_UNIFY_BOOTLOADER
bootctl_func_handles cmd_bootctrl_handles = {0};
#endif

/*static int clear_misc_partition(char *clearbuf, int size)
{
    char *partition = "misc";

    memset(clearbuf, 0, size);
    if (store_write_ops((unsigned char *)partition,
        (unsigned char *)clearbuf, 0, size) < 0) {
        printf("failed to clear %s.\n", partition);
        return -1;
    }

    return 0;
}*/

bool boot_info_validate(BrilloBootInfo *info)
{
	if (info->magic[0] != 'B' || info->magic[1] != 'C' ||
			info->magic[2] != 'c')
		return false;
	if (info->active_slot >= 2)
		return false;
	return true;
}

static void boot_info_reset(BrilloBootInfo *info)
{
	memset(info, '\0', SLOTBUF_SIZE);
	info->magic[0] = 'B';
	info->magic[1] = 'C';
	info->magic[2] = 'c';
	info->active_slot = 0;
	info->slot_info[0].bootable = 1;
	info->slot_info[0].online = 1;
	info->slot_info[1].bootable = 0;
	info->slot_info[1].online = 0;
	info->attemp_times = 0;
	memcpy(info->bootctrl_suffix, "_a", 2);
}

void dump_boot_info(BrilloBootInfo *info)
{
	pr_info("info->attemp_times = %u\n", info->attemp_times);
	pr_info("info->active_slot = %u\n", info->active_slot);
	pr_info("info->slot_info[0].bootable = %u\n", info->slot_info[0].bootable);
	pr_info("info->slot_info[0].online = %u\n", info->slot_info[0].online);
	pr_info("info->slot_info[1].bootable = %u\n", info->slot_info[1].bootable);
	pr_info("info->slot_info[1].online = %u\n", info->slot_info[1].online);
	pr_info("info->attemp_times = %u\n", info->attemp_times);
}


int boot_info_get_attemp_times(BrilloBootInfo *info)
{
	return info->attemp_times;
}

int boot_info_change_online_slot(BrilloBootInfo *info)
{
	BrilloSlotInfo tmp_info;

	memcpy(&tmp_info, &info->slot_info[0], sizeof(BrilloSlotInfo));
	memcpy(&info->slot_info[0], &info->slot_info[1], sizeof(BrilloSlotInfo));
	memcpy(&info->slot_info[1], &tmp_info, sizeof(BrilloSlotInfo));
	info->attemp_times = 0;
	return 0;
}


int boot_info_get_online_slot(BrilloBootInfo *info)
{
	if (info->slot_info[0].online == 1)
		return 0;

	if (info->slot_info[1].online == 1)
		return 1;

	return 0;
}

int boot_info_set_active_slot(BrilloBootInfo *info, int slot)
{
	if (slot == 0) {
		info->active_slot = 0;
		info->slot_info[0].bootable = 1;
		info->slot_info[0].online = 1;
		info->slot_info[1].bootable = 0;
		info->slot_info[1].online = 0;
		info->attemp_times = 0;
		memcpy(info->bootctrl_suffix, "_a", 2);
	} else {
		info->active_slot = 1;
		info->slot_info[0].bootable = 0;
		info->slot_info[0].online = 0;
		info->slot_info[1].bootable = 1;
		info->slot_info[1].online = 1;
		info->attemp_times = 0;
		memcpy(info->bootctrl_suffix, "_b", 2);
	}

	dump_boot_info(info);

	return 0;
}

bool boot_info_load(BrilloBootInfo *out_info, char *miscbuf)
{
	memcpy(out_info, miscbuf + BOOTINFO_OFFSET, SLOTBUF_SIZE);
	dump_boot_info(out_info);
	return true;
}

bool boot_info_save(BrilloBootInfo *info, char *miscbuf)
{
	char *partition = "misc";

	printf("save boot-info\n");
	memcpy(miscbuf + BOOTINFO_OFFSET, info, SLOTBUF_SIZE);
	dump_boot_info(info);
#ifdef CONFIG_AML_MTD
	enum boot_type_e device_boot_flag = store_get_type();

	if (device_boot_flag == BOOT_NAND_NFTL || device_boot_flag == BOOT_NAND_MTD ||
			device_boot_flag == BOOT_SNAND) {
		int ret = 0;

		ret = run_command("store erase misc 0 0x4000", 0);
		if (ret != 0) {
			printf("erase partition misc failed!\n");
			return false;
		}
	}
#endif

	if (store_get_type() == BOOT_SNAND || store_get_type() == BOOT_NAND_MTD) {
#ifdef CONFIG_BOOTLOADER_CONTROL_BLOCK
		nand_store_write((const char *)partition, 0, MISCBUF_SIZE,
							(unsigned char *)miscbuf);
#endif
	} else {
		store_write((const char *)partition, 0, MISCBUF_SIZE, (unsigned char *)miscbuf);
	}
	return true;
}

static int do_GetValidSlot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char miscbuf[4096] = {0};
	BrilloBootInfo info;
	int attemp_times;
	int slot;
	int ret = 0;

	if (argc != 1)
		return cmd_usage(cmdtp);

	ret = boot_info_open_partition(miscbuf);
	if (ret != 0)
		return -1;

	boot_info_load(&info, miscbuf);

#ifndef CONFIG_AB_SYSTEM
	char command[32];

	memcpy(command, miscbuf, 32);
	if (!memcmp(command, "boot-recovery", strlen("boot-recovery"))) {
		run_command("run init_display", 0);
		run_command("run storeargs", 0);
		if (run_command("run recovery_from_flash", 0) < 0) {
			printf("run_command for cmd:run recovery_from_flash failed.\n");
			return -1;
		}
		printf("run command:run recovery_from_flash successful.\n");
		return 0;
	}
#endif

	if (!boot_info_validate(&info)) {
		pr_info("boot-info is invalid. Resetting.\n");
		boot_info_reset(&info);
		boot_info_save(&info, miscbuf);
	}

	attemp_times = boot_info_get_attemp_times(&info);
	pr_info("attemp_times = %d\n", attemp_times);

	if (attemp_times == -1)
		boot_info_change_online_slot(&info);

	//slot = boot_info_get_online_slot(&info);
	slot = info.active_slot;
	pr_info("active slot = %d\n", slot);

#ifdef CONFIG_AML_MTD
	//check if boot_a/b on nand
	enum boot_type_e device_boot_flag = store_get_type();

	if (device_boot_flag == BOOT_NAND_MTD) {
		struct mtd_info *nand;

		nand = get_mtd_device_nm("boot_a");
		if (!IS_ERR(nand))
			has_boot_slot = 1;
		else
			has_boot_slot = 0;
	}
#endif

	if (slot == 0) {
		if (has_boot_slot == 1) {
			env_set("active_slot", "_a");
			env_set("boot_part", "boot_a");
			env_set("recovery_part", "recovery_a");
			env_set("slot-suffixes", "0");
		} else {
			env_set("active_slot", "normal");
			env_set("boot_part", "boot");
			env_set("recovery_part", "recovery");
			env_set("slot-suffixes", "-1");
		}
	} else {
		if (has_boot_slot == 1) {
			env_set("active_slot", "_b");
			env_set("boot_part", "boot_b");
			env_set("recovery_part", "recovery_b");
			env_set("slot-suffixes", "1");
		} else {
			env_set("active_slot", "normal");
			env_set("boot_part", "boot");
			env_set("recovery_part", "recovery");
			env_set("slot-suffixes", "-1");
		}
	}

	if (dynamic_partition)
		env_set("partition_mode", "dynamic");
	else
		env_set("partition_mode", "normal");

	return 0;
}

static int do_SetActiveSlot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char miscbuf[4096] = {0};
	BrilloBootInfo info;
	int ret = 0;

	if (argc != 2)
		return cmd_usage(cmdtp);

	if (has_boot_slot == 0) {
		pr_info("device is not ab mode\n");
		return -1;
	}

	ret = boot_info_open_partition(miscbuf);
	if (ret != 0)
		return -1;

	boot_info_load(&info, miscbuf);

	if (!boot_info_validate(&info)) {
		pr_info("boot-info is invalid. Resetting.\n");
		boot_info_reset(&info);
		boot_info_save(&info, miscbuf);
	}

	if (strcmp(argv[1], "a") == 0) {
		env_set("active_slot", "_a");
		env_set("boot_part", "boot_a");
		env_set("recovery_part", "recovery_a");
		env_set("slot-suffixes", "0");
		pr_info("set active slot a\n");
		boot_info_set_active_slot(&info, 0);
	} else if (strcmp(argv[1], "b") == 0) {
		env_set("active_slot", "_b");
		env_set("boot_part", "boot_b");
		env_set("recovery_part", "recovery_b");
		env_set("slot-suffixes", "1");
		pr_info("set active slot b\n");
		boot_info_set_active_slot(&info, 1);
	} else {
		pr_info("error input slot\n");
		return -1;
	}

	boot_info_save(&info, miscbuf);

	return 0;
}

static int do_GetSystemMode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
#ifdef CONFIG_SYSTEM_AS_ROOT
	env_set("system_mode", "1");
#else
	env_set("system_mode", "0");
#endif

	return 0;
}

static int do_GetAvbMode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	env_set("avb2", "0");

	return 0;
}

#endif /* CONFIG_BOOTLOADER_CONTROL_BLOCK */

#ifdef CONFIG_UNIFY_BOOTLOADER
bootctl_func_handles *get_bootctl_cmd_func(void)
{
	cmd_bootctrl_handles.do_GetValidSlot_func = do_GetValidSlot;
	cmd_bootctrl_handles.do_SetActiveSlot_func = do_SetActiveSlot;
	cmd_bootctrl_handles.do_GetSystemMode_func = do_GetSystemMode;
	cmd_bootctrl_handles.do_GetAvbMode_func = do_GetAvbMode;

	return &cmd_bootctrl_handles;
}
#else
U_BOOT_CMD(
    get_valid_slot, 2, 0, do_GetValidSlot,
    "get_valid_slot",
    "\nThis command will choose valid slot to boot up which saved in misc\n"
    "partition by mark to decide whether execute command!\n"
    "So you can execute command: get_valid_slot"
);

U_BOOT_CMD(
    set_active_slot, 2, 1, do_SetActiveSlot,
    "set_active_slot",
    "\nThis command will set active slot\n"
    "So you can execute command: set_active_slot a"
);

U_BOOT_CMD(
    get_system_as_root_mode, 1,	0, do_GetSystemMode,
    "get_system_as_root_mode",
    "\nThis command will get system_as_root_mode\n"
    "So you can execute command: get_system_as_root_mode"
);

U_BOOT_CMD(
    get_avb_mode, 1,	0, do_GetAvbMode,
    "get_avb_mode",
    "\nThis command will get avb mode\n"
    "So you can execute command: get_avb_mode"
);
#endif


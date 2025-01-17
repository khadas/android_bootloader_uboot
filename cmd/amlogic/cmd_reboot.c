// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/arch/reboot.h>
#include <asm/arch/secure_apb.h>
#include <asm/io.h>
#include <asm/arch/bl31_apis.h>
#include <partition_table.h>
#include <amlogic/storage.h>
#include <asm/arch/cpu_config.h>
#include <asm/arch/stick_mem.h>
#include <amlogic/pm.h>
/*
run get_rebootmode  //set reboot_mode env with current mode
*/

int do_get_rebootmode (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t reboot_mode_val;
	char *quiescent_env_bk;
	uint32_t stick_reboot_flag;
	uint32_t bit_mask;

#ifdef NO_EXTEND_REBOOT_MODE
	bit_mask = 0xf;
#else
	bit_mask = 0x7f;
#endif
	reboot_mode_val = ((readl(AO_SEC_SD_CFG15) >> 12) & bit_mask);
	//this step prevent the reboot mode val stored in sticky register lost
	//during the exceptional reset, such as reset pin disturbance
	stick_reboot_flag = get_stick_reboot_flag();
	if (reboot_mode_val == AMLOGIC_COLD_BOOT &&
			stick_reboot_flag == AMLOGIC_WATCHDOG_REBOOT) {
		printf("Warning: system is reset abnormally!\n");
		printf("Set reboot_mode to watchdog reboot\n");
		reboot_mode_val = AMLOGIC_WATCHDOG_REBOOT;
	}

	debug("reboot_mode(0x%x)=0x%x\n", AO_SEC_SD_CFG15, reboot_mode_val);

	switch (reboot_mode_val)
	{
		case AMLOGIC_COLD_BOOT:
		{
			env_set("reboot_mode","cold_boot");
			break;
		}
		case AMLOGIC_NORMAL_BOOT:
		{
			env_set("reboot_mode","normal");
			break;
		}
		case AMLOGIC_FACTORY_RESET_REBOOT:
		{
			env_set("reboot_mode","factory_reset");
			break;
		}
		case AMLOGIC_UPDATE_REBOOT:
		{
			env_set("reboot_mode","update");
			break;
		}
		case AMLOGIC_FASTBOOT_REBOOT:
		{
			env_set("reboot_mode","fastboot");
			break;
		}
		case AMLOGIC_BOOTLOADER_REBOOT:
		{
			env_set("reboot_mode","bootloader");
			break;
		}
		case AMLOGIC_SUSPEND_REBOOT:
		{
			env_set("reboot_mode","suspend_off");
			break;
		}
		case AMLOGIC_HIBERNATE_REBOOT:
		{
			env_set("reboot_mode","hibernate");
			break;
		}
		case AMLOGIC_SHUTDOWN_REBOOT:
		{
			env_set("reboot_mode","shutdown_reboot");
			break;
		}
		case AMLOGIC_RESCUEPARTY_REBOOT:
		{
			env_set("reboot_mode", "rescueparty");
			break;
		}
		case AMLOGIC_KERNEL_PANIC:
		{
			env_set("reboot_mode","kernel_panic");
			break;
		}
		case AMLOGIC_WATCHDOG_REBOOT:
		{
			env_set("reboot_mode","watchdog_reboot");
			break;
		}
		case AMLOGIC_RPMBP_REBOOT:
		{
			env_set("reboot_mode","rpmbp");
			break;
		}
		case AMLOGIC_QUIESCENT_REBOOT:
		{
			env_set("quiescent_env_bk", "quiescent");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p quiescent_env_bk;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif
			env_set("reboot_mode","quiescent");
			break;
		}
		case AMLOGIC_RECOVERY_QUIESCENT_REBOOT:
		{
			env_set("quiescent_env_bk", "recovery_quiescent");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
			run_command("update_env_part -p quiescent_env_bk;", 0);
#else
			run_command("defenv_reserv; saveenv;", 0);
#endif
			env_set("reboot_mode","recovery_quiescent");
			break;
		}
#ifdef AMLOGIC_FFV_REBOOT
		case AMLOGIC_FFV_REBOOT:
		{
			env_set("reboot_mode", "ffv_reboot");
			break;
		}
#endif
		default:
		{
			env_set("reboot_mode","charging");
			break;
		}
	}

#ifdef CONFIG_CMD_FASTBOOT
	switch (reboot_mode_val) {
		case AMLOGIC_FASTBOOT_REBOOT: {
			env_set("reboot_mode","fastboot");
			break;
		}
		case AMLOGIC_BOOTLOADER_REBOOT: {
			env_set("reboot_mode", "fastboot");
			break;
		}
	}
#endif

	quiescent_env_bk = env_get("quiescent_env_bk");
	if (quiescent_env_bk &&
		((strcmp(quiescent_env_bk, "quiescent") == 0) ||
		(strcmp(quiescent_env_bk, "recovery_quiescent") == 0))) {
		printf("quiescent_env_bk: %s\n", quiescent_env_bk);
		switch (reboot_mode_val) {
		case AMLOGIC_COLD_BOOT:
		case AMLOGIC_KERNEL_PANIC:
		case AMLOGIC_WATCHDOG_REBOOT: {
				printf("set reboot_mode %s\n", quiescent_env_bk);
				env_set("reboot_mode", quiescent_env_bk);
				break;
			}
		default: {
				printf("clear quiescent_env_bk\n");
				env_set("quiescent_env_bk", "0");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
				run_command("update_env_part -p quiescent_env_bk;", 0);
#else
				run_command("defenv_reserv; saveenv;", 0);
#endif
				break;
			}
		}
	}

#if defined(CONFIG_AML_RPMB)
	run_command("rpmb_state",0);
#endif

	return 0;
}

int do_reboot (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t reboot_mode_val = AMLOGIC_NORMAL_BOOT;
	if (argc <= 1) {
		printf("reboot use default mode: normal\n");
	}
	else {
		printf("reboot mode: %s\n", argv[1]);
		char * mode = argv[1];

		if (strcmp(mode, "next") == 0) {
			store_restore_bootidx();
			reboot_mode_val = AMLOGIC_NORMAL_BOOT;
		} else if (strcmp(mode, "next,quiescent") == 0) {
			store_restore_bootidx();
			reboot_mode_val = AMLOGIC_QUIESCENT_REBOOT;
		} else if (strcmp(mode, "next,bootloader") == 0) {
			store_restore_bootidx();
			reboot_mode_val = AMLOGIC_BOOTLOADER_REBOOT;
		} else if (strcmp(mode, "cold_boot") == 0)
			reboot_mode_val = AMLOGIC_COLD_BOOT;
		else if (strcmp(mode, "normal") == 0)
			reboot_mode_val = AMLOGIC_NORMAL_BOOT;
		else if (strcmp(mode, "recovery") == 0) {
			reboot_mode_val = AMLOGIC_FACTORY_RESET_REBOOT;
			run_command("bcb recovery", 0);
		} else if (strcmp(mode, "factory_reset") == 0)
			reboot_mode_val = AMLOGIC_FACTORY_RESET_REBOOT;
		else if (strcmp(mode, "update") == 0)
			reboot_mode_val = AMLOGIC_UPDATE_REBOOT;
		else if (strcmp(mode, "fastboot") == 0) {
			if (dynamic_partition) {
				printf("dynamic partition, enter fastbootd");
				reboot_mode_val = AMLOGIC_FACTORY_RESET_REBOOT;
				run_command("bcb fastbootd",0);
			} else
				reboot_mode_val = AMLOGIC_FASTBOOT_REBOOT;
		} else if (strcmp(mode, "bootloader") == 0)
			reboot_mode_val = AMLOGIC_BOOTLOADER_REBOOT;
		else if (strcmp(mode, "suspend_off") == 0)
			reboot_mode_val = AMLOGIC_SUSPEND_REBOOT;
		else if (strcmp(mode, "hibernate") == 0)
			reboot_mode_val = AMLOGIC_HIBERNATE_REBOOT;
		else if (strcmp(mode, "rescueparty") == 0)
			reboot_mode_val = AMLOGIC_RESCUEPARTY_REBOOT;
		else if (strcmp(mode, "kernel_panic") == 0)
			reboot_mode_val = AMLOGIC_KERNEL_PANIC;
		else if (strcmp(mode, "rpmbp") == 0)
			reboot_mode_val = AMLOGIC_RPMBP_REBOOT;
#ifdef AMLOGIC_FFV_REBOOT
		else if (strcmp(mode, "ffv_reboot") == 0)
			reboot_mode_val = AMLOGIC_FFV_REBOOT;
#endif
		else {
			printf("Can not find match reboot mode, use normal by default\n");
			reboot_mode_val = AMLOGIC_NORMAL_BOOT;
		}
	}
#ifdef CONFIG_USB_DEVICE_V2
#if !(defined AML_USB_V2)
	*P_RESET1_REGISTER |= (1<<17);
	mdelay(200);
#endif
#endif
	dcache_disable();

	aml_reboot (PSCI_SYS_REBOOT, reboot_mode_val, 0, 0);
	return 0;
}

/* USB BOOT FUNC sub command list*/
#define CLEAR_USB_BOOT			1
#define FORCE_USB_BOOT			2
#define RUN_COMD_USB_BOOT		3
#define PANIC_DUMP_USB_BOOT		4

int do_set_usb_boot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int usb_mode = 0;
	if (argc <= 1) {
		printf("usb flag default 0\n");
	}
	else {
		usb_mode = simple_strtoul(argv[1], NULL, 16);
	}
	printf("usb flag: %d\n", usb_mode);
	set_usb_boot_function(usb_mode);

	return 0;
}

U_BOOT_CMD(
	get_rebootmode,	1,	0,	do_get_rebootmode,
	"get reboot mode",
	"/N\n"
	"  This command will get and set env 'reboot_mode'\n"
	"get_rebootmode\n"
);

U_BOOT_CMD(
	reboot,	2,	0,	do_reboot,
	"set reboot mode and reboot system",
	"[rebootmode]/N\n"
	"  This command will set reboot mode and reboot system\n"
	"\n"
	"  support following [rebootmode]:\n"
	"    cold_boot\n"
	"    normal[default]\n"
	"    factory_reset/recovery\n"
	"    update\n"
	"    fastboot\n"
	"    bootloader\n"
	"    suspend_off\n"
	"    hibernate\n"
	"    next <ONLY work for SC2>\n"
	"    crash_dump\n"
);

U_BOOT_CMD(
	set_usb_boot,	2,	0,	do_set_usb_boot,
	"set usb boot mode",
	"[usb boot mode]/N\n"
	"  support following [usb boot mode]:\n"
	"    1: CLEAR_USB_BOOT\n"
	"    2: FORCE_USB_BOOT[default]\n"
	"    3: RUN_COMD_USB_BOOT/recovery\n"
	"    4: PANIC_DUMP_USB_BOOT\n"
);

int do_systemoff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	pm_poweroff();
	return 0;
}


U_BOOT_CMD(
	systemoff,	2,	1,	do_systemoff,
	"system off ",
	"systemoff "
);

int do_systemsuspend(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	pm_suspend();
	return 0;
}

U_BOOT_CMD(systemsuspend, 2, 1,	do_systemsuspend,
	"system suspend ", "systemsuspend ");


// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Boot support
 */
#include <common.h>
#include <bootm.h>
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <nand.h>
#include <asm/byteorder.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <u-boot/zlib.h>
#include <asm/arch/bl31_apis.h>
#include <libavb.h>
#if defined(CONFIG_AML_ANTIROLLBACK) || defined(CONFIG_AML_AVB2_ANTIROLLBACK)
#include <amlogic/anti-rollback.h>
#endif
#include <asm/arch/secure_apb.h>
#include <amlogic/aml_efuse.h>
#include <version.h>
#include <amlogic/image_check.h>
#include <amlogic/aml_rollback.h>
#include <partition_table.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CMD_IMI)
static int image_info(unsigned long addr);
#endif

#if defined(CONFIG_CMD_IMLS)
#include <flash.h>
#include <mtd/cfi_flash.h>
extern flash_info_t flash_info[]; /* info for FLASH chips */
#endif

#if defined(CONFIG_CMD_IMLS) || defined(CONFIG_CMD_IMLS_NAND)
static int do_imls(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif

/* we overload the cmd field with our state machine info instead of a
 * function pointer */
static cmd_tbl_t cmd_bootm_sub[] = {
	U_BOOT_CMD_MKENT(start, 0, 1, (void *)BOOTM_STATE_START, "", ""),
	U_BOOT_CMD_MKENT(loados, 0, 1, (void *)BOOTM_STATE_LOADOS, "", ""),
#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
	U_BOOT_CMD_MKENT(ramdisk, 0, 1, (void *)BOOTM_STATE_RAMDISK, "", ""),
#endif
#ifdef CONFIG_OF_LIBFDT
	U_BOOT_CMD_MKENT(fdt, 0, 1, (void *)BOOTM_STATE_FDT, "", ""),
#endif
	U_BOOT_CMD_MKENT(cmdline, 0, 1, (void *)BOOTM_STATE_OS_CMDLINE, "", ""),
	U_BOOT_CMD_MKENT(bdt, 0, 1, (void *)BOOTM_STATE_OS_BD_T, "", ""),
	U_BOOT_CMD_MKENT(prep, 0, 1, (void *)BOOTM_STATE_OS_PREP, "", ""),
	U_BOOT_CMD_MKENT(fake, 0, 1, (void *)BOOTM_STATE_OS_FAKE_GO, "", ""),
	U_BOOT_CMD_MKENT(go, 0, 1, (void *)BOOTM_STATE_OS_GO, "", ""),
};

static int do_bootm_subcommand(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	int ret = 0;
	long state;
	cmd_tbl_t *c;

	c = find_cmd_tbl(argv[0], &cmd_bootm_sub[0], ARRAY_SIZE(cmd_bootm_sub));
	argc--; argv++;

	if (c) {
		state = (long)c->cmd;
		if (state == BOOTM_STATE_START)
			state |= BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER;
	} else {
		/* Unrecognized command */
		return CMD_RET_USAGE;
	}

	if (((state & BOOTM_STATE_START) != BOOTM_STATE_START) &&
	    images.state >= state) {
		printf("Trying to execute a command out of order\n");
		return CMD_RET_USAGE;
	}

	ret = do_bootm_states(cmdtp, flag, argc, argv, state, &images, 0);

	return ret;
}

static void recovery_mode_process(void)
{
	char *reboot_mode_s = NULL;
	char *upgrade_step_s = NULL;

	reboot_mode_s = env_get("reboot_mode");
	upgrade_step_s = env_get("upgrade_step");
	if ((!reboot_mode_s) || (!upgrade_step_s))
		return;

	if ((!strcmp(reboot_mode_s, "recovery")) || (!strcmp(reboot_mode_s, "update"))
		|| (!strcmp(reboot_mode_s, "factory_reset")) || (!strcmp(upgrade_step_s, "3")))
	{
		run_command("amlbootsta -p -s",0);
	}
}

static void bootloader_wp_check(void)
{
	char *bootloader_wp = env_get("bootloader_wp");
	char *slot = env_get("active_slot");
	char *gpt_mode = env_get("gpt_mode");
	char *nocs_mode = env_get("nocs_mode");

	//support write protect
	if (bootloader_wp && !strcmp(bootloader_wp, "1")) {
		//ab mode
		if (slot && strcmp(slot, "normal")) {
			//gpt or nocs
			if ((gpt_mode && !strcmp(gpt_mode, "true")) ||
				(nocs_mode && !strcmp(nocs_mode, "true"))) {
				//boot1 protect
				run_command("amlmmc boot_wp boot1 poweron", 0);
			} else {
				if (!strcmp(slot, "_a")) {
					//boot0 protect
					run_command("amlmmc boot_wp boot0 poweron", 0);
				} else {
					//boot1 protect
					run_command("amlmmc boot_wp boot1 poweron", 0);
				}
			}
		}
	}
}


/*******************************************************************/
/* bootm - boot application image from image in memory */
/*******************************************************************/
//temp solution for A1, as A1 secure boot not ready yet...
#include <amlogic/cpu_id.h>
//end
int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	/* add reboot_mode in bootargs for kernel command line */
	char *pbootargs = env_get("bootargs");
	char *preboot_mode = env_get("reboot_mode");
	char *recoverystr = "factory_reset";
	char *precovery_mode = env_get("recovery_mode");

	//if recovery mode need set reboot_mode factory_reset
	//for drm driver init recovery by reboot_mode
	if (precovery_mode && !strcmp(precovery_mode, "true"))
		preboot_mode = recoverystr;

	if (pbootargs && preboot_mode) {
		int nlen = strlen(pbootargs) + strlen(preboot_mode) + 16;
		char *pnewbootargs = malloc(nlen);
		//char *pnewbootargs = (char *)0x6000000;
		if (pnewbootargs) {
			memset((void *)pnewbootargs, 0, nlen);
			sprintf(pnewbootargs, "%s reboot_mode=%s\n", pbootargs, preboot_mode);
			env_set("bootargs", pnewbootargs);
			free(pnewbootargs);
			pnewbootargs = NULL;
		} else {
			puts("Error: malloc in pnewbootargs failed!\n");
		}
	} else {
		puts("Error: add reboot_mode in bootargs failed!\n");
	}

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	static int relocated = 0;

	if (!relocated) {
		int i;

		/* relocate names of sub-command table */
		for (i = 0; i < ARRAY_SIZE(cmd_bootm_sub); i++)
			cmd_bootm_sub[i].name += gd->reloc_off;

		relocated = 1;
	}
#endif

	/* determine if we have a sub command */
	argc--; argv++;
	if (argc > 0) {
		char *endp;

		simple_strtoul(argv[0], &endp, 16);
		/* endp pointing to NULL means that argv[0] was just a
		 * valid number, pass it along to the normal bootm processing
		 *
		 * If endp is ':' or '#' assume a FIT identifier so pass
		 * along for normal processing.
		 *
		 * Right now we assume the first arg should never be '-'
		 */
		if ((*endp != 0) && (*endp != ':') && (*endp != '#'))
			return do_bootm_subcommand(cmdtp, flag, argc, argv);
	}
#ifndef CONFIG_SKIP_KERNEL_DTB_SECBOOT_CHECK
	unsigned int loadaddr = GXB_IMG_LOAD_ADDR; //default load address

	if (argc > 0)
	{
		char *endp;
		loadaddr = simple_strtoul(argv[0], &endp, 16);
		//printf("aml log : addr = 0x%x\n",loadaddr);
	}

	if (IS_FEAT_BOOT_VERIFY()) {
		int ret = 0;
		ulong img_addr, ncheckoffset = 0;
		static char argv0_new[12] = {0};
		static char *argv_new;

		argv_new = (char *)&argv0_new;
		img_addr = genimg_get_kernel_addr(argc < 1 ? NULL : argv[0]);
#ifndef CONFIG_IMAGE_CHECK
		ret = aml_sec_boot_check(AML_D_P_IMG_DECRYPT, loadaddr, GXB_IMG_SIZE,
				GXB_IMG_DEC_ALL);

		if (ret) {
			printf("\naml log : Sig Check %d\n", ret);
			return ret;
		}
#ifdef AML_D_Q_IMG_SIG_HDR_SIZE
		ncheckoffset = aml_sec_boot_check(AML_D_Q_IMG_SIG_HDR_SIZE,
				GXB_IMG_LOAD_ADDR, GXB_EFUSE_PATTERN_SIZE, GXB_IMG_DEC_ALL);
		if (AML_D_Q_IMG_SIG_HDR_SIZE == (ncheckoffset & 0xFFFF))
			ncheckoffset = (ncheckoffset >> 16) & 0xFFFF;
		else
			ncheckoffset = 0;
#endif
#else
		ret = secure_image_check((uint8_t *)(unsigned long)loadaddr,
			GXB_IMG_SIZE, GXB_IMG_DEC_ALL);
		if (ret) {
			printf("\naml log : Sig Check %d\n", ret);
			return ret;
		}
		/* Override load address argument to skip secure boot header.
		 * Only skip if secure boot so normal boot can use plain boot.img+
		 */
		ncheckoffset = android_image_check_offset();
#endif
		img_addr += ncheckoffset;
		env_set_hex("loadaddr", img_addr);//android_image_get_ramdisk_v3 need env loadaddr
		snprintf(argv0_new, sizeof(argv0_new), "%lx", img_addr);
		argc = 1;
		argv = (char **)&argv_new;
	}
#endif

	char *fastboot_step = env_get("fastboot_step");

	if (fastboot_step && (strcmp(fastboot_step, "2") == 0)) {
		//come to here, means new burn bootloader.img is OK, reset env
		printf("new burn bootloader.img is OK, write other bootloader\n");
		char *gpt_mode = env_get("gpt_mode");
		char *nocs_mode = env_get("nocs_mode");

		if ((gpt_mode && !strcmp(gpt_mode, "true")) ||
			(nocs_mode && !strcmp(nocs_mode, "true"))) {
			printf("gpt or disable user bootloader mode\n");
			run_command("copy_slot_bootable 2 1", 0);
		} else {
			printf("normal mode\n");
			run_command("copy_slot_bootable 1 0", 0);
			run_command("copy_slot_bootable 1 2", 0);
		}

		env_set("fastboot_step", "0");
#if CONFIG_IS_ENABLED(AML_UPDATE_ENV)
		run_command("update_env_part -p fastboot_step;", 0);
#else
		run_command("defenv_reserve;setenv fastboot_step 0;saveenv;", 0);
#endif
	}

#ifdef CONFIG_CMD_BOOTCTOL_AVB
	int rc = 0;
	char *avb_s = NULL;

	run_command("get_avb_mode;", 0);
	avb_s = env_get("avb2");
	printf("avb2: %s\n", avb_s);
	if (strcmp(avb_s, "1") == 0) {
		AvbSlotVerifyData *out_data = NULL;
		char *bootargs = NULL;
		char *newbootargs = NULL;
		const char *bootstate_o = "androidboot.verifiedbootstate=orange";
		const char *bootstate_g = "androidboot.verifiedbootstate=green";
		const char *bootstate = NULL;
		uint8_t vbmeta_digest[AVB_SHA256_DIGEST_SIZE];

		if (is_device_unlocked()) {
			printf("unlock state, ignore the avb check\n");
			rc = avb_verify(&out_data);
			run_command("setenv bootconfig ${bootconfig} "\
			"androidboot.verifiedbootstate=orange", 0);
		} else {
			printf("lock state, need avb check\n");
			rc = avb_verify(&out_data);
			printf("avb verification: locked = %d, result = %d\n",
						!is_device_unlocked(), rc);
#if defined(CONFIG_AML_ANTIROLLBACK) || defined(CONFIG_AML_AVB2_ANTIROLLBACK)
			if (rc == AVB_SLOT_VERIFY_RESULT_OK && is_avb_arb_available() &&
					!set_successful_boot()) {
				uint32_t i = 0;
				uint32_t version = 0;

				for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
					uint64_t rb_idx = out_data->rollback_indexes[i];

					if (get_avb_antirollback(i, &version) &&
						version < (uint32_t)rb_idx &&
						!set_avb_antirollback(i, (uint32_t)rb_idx)) {
						printf("rollback(%d) = %u failed\n",
							i, (uint32_t)rb_idx);
					}
				}
			}

			if (is_avb_arb_available() &&
				rc == AVB_SLOT_VERIFY_RESULT_ERROR_ROLLBACK_INDEX &&
				has_boot_slot == 1) {
				printf("ab mode\n");
				update_rollback();
				env_set("write_boot", "0");
				run_command("saveenv", 0);
				run_command("reset", 0);
			}
#endif

			if (rc != AVB_SLOT_VERIFY_RESULT_OK) {
				avb_slot_verify_data_free(out_data);
				return rc;
			}
		}

		bootargs = env_get("bootconfig");
		if (!bootargs) {
			bootargs = "\0";
		}

		if (out_data) {
			keymaster_boot_params boot_params;
			const int is_dev_unlocked = is_device_unlocked();
			AvbVBMetaImageHeader toplevel_vbmeta;

			avb_vbmeta_image_header_to_host_byte_order
			((const AvbVBMetaImageHeader *)out_data->vbmeta_images[0].vbmeta_data,
			&toplevel_vbmeta);

			boot_params.boot_patchlevel =
				avb_get_boot_patchlevel_from_vbmeta(out_data);

			create_csrs();

			boot_params.device_locked = is_dev_unlocked? 0: 1;
			if (is_dev_unlocked || (toplevel_vbmeta.flags &
				AVB_VBMETA_IMAGE_FLAGS_VERIFICATION_DISABLED) ||
				(toplevel_vbmeta.flags &
				AVB_VBMETA_IMAGE_FLAGS_HASHTREE_DISABLED)) {
				bootstate = bootstate_o;
				boot_params.verified_boot_state = 2;
			}
			else {
				bootstate = bootstate_g;
				boot_params.verified_boot_state = 0;
			}

			memcpy(boot_params.verified_boot_key, boot_key_hash,
					sizeof(boot_params.verified_boot_key));

			avb_slot_verify_data_calculate_vbmeta_digest(
				out_data, AVB_DIGEST_TYPE_SHA256, vbmeta_digest);
			memcpy(boot_params.verified_boot_hash, vbmeta_digest,
					sizeof(boot_params.verified_boot_hash));

			if (set_boot_params(&boot_params) < 0) {
				printf("failed to set boot params.\n");
			}

			newbootargs = malloc(strlen(bootargs) + strlen(out_data->cmdline) + strlen(bootstate) + 1 + 1 + 1);
			if (!newbootargs) {
				printf("failed to allocate buffer for bootarg\n");
				return -1;
			}
			sprintf(newbootargs, "%s %s %s", bootargs, out_data->cmdline, bootstate);
			env_set("bootconfig", newbootargs);
			free(newbootargs);
			newbootargs = NULL;
			avb_slot_verify_data_free(out_data);
		}
	}else {
		const int is_dev_unlocked = is_device_unlocked();
		if (is_dev_unlocked)
			run_command("setenv bootconfig ${bootconfig} "\
			"androidboot.verifiedbootstate=orange", 0);
		else
			run_command("setenv bootconfig ${bootconfig} "\
			"androidboot.verifiedbootstate=green", 0);
	}
#endif//CONFIG_CMD_BOOTCTOL_AVB

	recovery_mode_process();
	bootloader_wp_check();
	return do_bootm_states(cmdtp, flag, argc, argv, BOOTM_STATE_START |
		BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER |
		BOOTM_STATE_LOADOS |
#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
		BOOTM_STATE_RAMDISK |
#endif
#if defined(CONFIG_PPC) || defined(CONFIG_MIPS)
		BOOTM_STATE_OS_CMDLINE |
#endif
		BOOTM_STATE_OS_PREP | BOOTM_STATE_OS_FAKE_GO |
		BOOTM_STATE_OS_GO, &images, 1);
}

int bootm_maybe_autostart(cmd_tbl_t *cmdtp, const char *cmd)
{
	const char *ep = env_get("autostart");

	if (ep && !strcmp(ep, "yes")) {
		char *local_args[2];
		local_args[0] = (char *)cmd;
		local_args[1] = NULL;
		printf("Automatic boot of image at addr 0x%08lX ...\n", load_addr);
		return do_bootm(cmdtp, 0, 1, local_args);
	}

	return 0;
}

#ifdef CONFIG_SYS_LONGHELP
static char bootm_help_text[] =
	"[addr [arg ...]]\n    - boot application image stored in memory\n"
	"\tpassing arguments 'arg ...'; when booting a Linux kernel,\n"
	"\t'arg' can be the address of an initrd image\n"
#if defined(CONFIG_OF_LIBFDT)
	"\tWhen booting a Linux kernel which requires a flat device-tree\n"
	"\ta third argument is required which is the address of the\n"
	"\tdevice-tree blob. To boot that kernel without an initrd image,\n"
	"\tuse a '-' for the second argument. If you do not pass a third\n"
	"\ta bd_info struct will be passed instead\n"
#endif
#if defined(CONFIG_FIT)
	"\t\nFor the new multi component uImage format (FIT) addresses\n"
	"\tmust be extended to include component or configuration unit name:\n"
	"\taddr:<subimg_uname> - direct component image specification\n"
	"\taddr#<conf_uname>   - configuration specification\n"
	"\tUse iminfo command to get the list of existing component\n"
	"\timages and configurations.\n"
#endif
	"\nSub-commands to do part of the bootm sequence.  The sub-commands "
	"must be\n"
	"issued in the order below (it's ok to not issue all sub-commands):\n"
	"\tstart [addr [arg ...]]\n"
	"\tloados  - load OS image\n"
#if defined(CONFIG_SYS_BOOT_RAMDISK_HIGH)
	"\tramdisk - relocate initrd, set env initrd_start/initrd_end\n"
#endif
#if defined(CONFIG_OF_LIBFDT)
	"\tfdt	- relocate flat device tree\n"
#endif
	"\tcmdline - OS specific command line processing/setup\n"
	"\tbdt	- OS specific bd_t processing\n"
	"\tprep    - OS specific prep before relocation or go\n"
#if defined(CONFIG_TRACE)
	"\tfake    - OS specific fake start without go\n"
#endif
	"\tgo	 - start OS";
#endif

U_BOOT_CMD(
	bootm,	CONFIG_SYS_MAXARGS,	1,	do_bootm,
	"boot application image from memory", bootm_help_text
);

/*******************************************************************/
/* bootd - boot default image */
/*******************************************************************/
#if defined(CONFIG_CMD_BOOTD)
int do_bootd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return run_command(env_get("bootcmd"), flag);
}

U_BOOT_CMD(
	boot,	1,	1,	do_bootd,
	"boot default, i.e., run 'bootcmd'",
	""
);

/* keep old command name "bootd" for backward compatibility */
U_BOOT_CMD(
	bootd, 1,	1,	do_bootd,
	"boot default, i.e., run 'bootcmd'",
	""
);

#endif


/*******************************************************************/
/* iminfo - print header info for a requested image */
/*******************************************************************/
#if defined(CONFIG_CMD_IMI)
static int do_iminfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int	arg;
	ulong	addr;
	int	rcode = 0;

	if (argc < 2) {
		return image_info(load_addr);
	}

	for (arg = 1; arg < argc; ++arg) {
		addr = simple_strtoul(argv[arg], NULL, 16);
		if (image_info(addr) != 0)
			rcode = 1;
	}
	return rcode;
}

static int image_info(ulong addr)
{
	void *hdr = (void *)addr;

	printf("\n## Checking Image at %08lx ...\n", addr);

	switch (genimg_get_format(hdr)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
	case IMAGE_FORMAT_LEGACY:
		puts("   Legacy image found\n");
		if (!image_check_magic(hdr)) {
			puts("   Bad Magic Number\n");
			return 1;
		}

		if (!image_check_hcrc(hdr)) {
			puts("   Bad Header Checksum\n");
			return 1;
		}

		image_print_contents(hdr);

		puts("   Verifying Checksum ... ");
		if (!image_check_dcrc(hdr)) {
			puts("   Bad Data CRC\n");
			return 1;
		}
		puts("OK\n");
		return 0;
#endif
#if defined(CONFIG_ANDROID_BOOT_IMAGE)
	case IMAGE_FORMAT_ANDROID:
		puts("   Android image found\n");
		android_print_contents(hdr);
		return 0;
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		puts("   FIT image found\n");

		if (fit_check_format(hdr, IMAGE_SIZE_INVAL)) {
			puts("Bad FIT image format!\n");
			return 1;
		}

		fit_print_contents(hdr);

		if (!fit_all_image_verify(hdr)) {
			puts("Bad hash in FIT image!\n");
			return 1;
		}

		return 0;
#endif
	default:
		puts("Unknown image format!\n");
		break;
	}

	return 1;
}

U_BOOT_CMD(
	iminfo,	CONFIG_SYS_MAXARGS,	1,	do_iminfo,
	"print header information for application image",
	"addr [addr ...]\n"
	"    - print header information for application image starting at\n"
	"	 address 'addr' in memory; this includes verification of the\n"
	"	 image contents (magic number, header and payload checksums)"
);
#endif


/*******************************************************************/
/* imls - list all images found in flash */
/*******************************************************************/
#if defined(CONFIG_CMD_IMLS)
static int do_imls_nor(void)
{
	flash_info_t *info;
	int i, j;
	void *hdr;

	for (i = 0, info = &flash_info[0];
		i < CONFIG_SYS_MAX_FLASH_BANKS; ++i, ++info) {

		if (info->flash_id == FLASH_UNKNOWN)
			goto next_bank;
		for (j = 0; j < info->sector_count; ++j) {

			hdr = (void *)info->start[j];
			if (!hdr)
				goto next_sector;

			switch (genimg_get_format(hdr)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
			case IMAGE_FORMAT_LEGACY:
				if (!image_check_hcrc(hdr))
					goto next_sector;

				printf("Legacy Image at %08lX:\n", (ulong)hdr);
				image_print_contents(hdr);

				puts("   Verifying Checksum ... ");
				if (!image_check_dcrc(hdr)) {
					puts("Bad Data CRC\n");
				} else {
					puts("OK\n");
				}
				break;
#endif
#if defined(CONFIG_FIT)
			case IMAGE_FORMAT_FIT:
				if (fit_check_format(hdr, IMAGE_SIZE_INVAL))
					goto next_sector;

				printf("FIT Image at %08lX:\n", (ulong)hdr);
				fit_print_contents(hdr);
				break;
#endif
			default:
				goto next_sector;
			}

next_sector:		;
		}
next_bank:	;
	}
	return 0;
}
#endif

#if defined(CONFIG_CMD_IMLS_NAND)
static int nand_imls_legacyimage(struct mtd_info *mtd, int nand_dev,
				 loff_t off, size_t len)
{
	void *imgdata;
	int ret;

	imgdata = malloc(len);
	if (!imgdata) {
		printf("May be a Legacy Image at NAND device %d offset %08llX:\n",
				nand_dev, off);
		printf("   Low memory(cannot allocate memory for image)\n");
		return -ENOMEM;
	}

	ret = nand_read_skip_bad(mtd, off, &len, NULL, mtd->size, imgdata);
	if (ret < 0 && ret != -EUCLEAN) {
		free(imgdata);
		return ret;
	}

	if (!image_check_hcrc(imgdata)) {
		free(imgdata);
		return 0;
	}

	printf("Legacy Image at NAND device %d offset %08llX:\n",
			nand_dev, off);
	image_print_contents(imgdata);

	puts("   Verifying Checksum ... ");
	if (!image_check_dcrc(imgdata))
		puts("Bad Data CRC\n");
	else
		puts("OK\n");

	free(imgdata);

	return 0;
}

static int nand_imls_fitimage(struct mtd_info *mtd, int nand_dev, loff_t off,
				 size_t len)
{
	void *imgdata;
	int ret;

	imgdata = malloc(len);
	if (!imgdata) {
		printf("May be a FIT Image at NAND device %d offset %08llX:\n",
				nand_dev, off);
		printf("   Low memory(cannot allocate memory for image)\n");
		return -ENOMEM;
	}

	ret = nand_read_skip_bad(mtd, off, &len, NULL, mtd->size, imgdata);
	if (ret < 0 && ret != -EUCLEAN) {
		free(imgdata);
		return ret;
	}

	if (fit_check_format(imgdata, IMAGE_SIZE_INVAL)) {
		free(imgdata);
		return 0;
	}

	printf("FIT Image at NAND device %d offset %08llX:\n", nand_dev, off);

	fit_print_contents(imgdata);
	free(imgdata);

	return 0;
}

static int do_imls_nand(void)
{
	struct mtd_info *mtd;
	int nand_dev = nand_curr_device;
	size_t len;
	loff_t off;
	u32 buffer[16];

	if (nand_dev < 0 || nand_dev >= CONFIG_SYS_MAX_NAND_DEVICE) {
		puts("\nNo NAND devices available\n");
		return -ENODEV;
	}

	printf("\n");

	for (nand_dev = 0; nand_dev < CONFIG_SYS_MAX_NAND_DEVICE; nand_dev++) {
		mtd = get_nand_dev_by_index(nand_dev);
		if (!mtd->name || !mtd->size)
			continue;

		for (off = 0; off < mtd->size; off += mtd->erasesize) {
			const image_header_t *header;
			int ret;

			if (nand_block_isbad(mtd, off))
				continue;

			len = sizeof(buffer);

			ret = nand_read(mtd, off, &len, (u8 *)buffer);
			if (ret < 0 && ret != -EUCLEAN) {
				printf("NAND read error %d at offset %08llX\n",
						ret, off);
				continue;
			}

			switch (genimg_get_format(buffer)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
			case IMAGE_FORMAT_LEGACY:
				header = (const image_header_t *)buffer;

				len = image_get_image_size(header);
				nand_imls_legacyimage(mtd, nand_dev, off, len);
				break;
#endif
#if defined(CONFIG_FIT)
			case IMAGE_FORMAT_FIT:
				len = fit_get_size(buffer);
				nand_imls_fitimage(mtd, nand_dev, off, len);
				break;
#endif
			}
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_CMD_IMLS) || defined(CONFIG_CMD_IMLS_NAND)
static int do_imls(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret_nor = 0, ret_nand = 0;

#if defined(CONFIG_CMD_IMLS)
	ret_nor = do_imls_nor();
#endif

#if defined(CONFIG_CMD_IMLS_NAND)
	ret_nand = do_imls_nand();
#endif

	if (ret_nor)
		return ret_nor;

	if (ret_nand)
		return ret_nand;

	return (0);
}

U_BOOT_CMD(
	imls,	1,		1,	do_imls,
	"list all images found in flash",
	"\n"
	"    - Prints information about all images found at sector/block\n"
	"	 boundaries in nor/nand flash."
);
#endif

// SPDX-License-Identifier:     BSD-3-Clause
/*
 * Copyright (c) 2018 The Fuchsia Authors
 */

#include <common.h>
#include <kvstore.h>
#include <zircon/sysconfig.h>
#include <amlogic/storage.h>

static int do_zircon_bootconfig(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	const char *sysconfig;
	const char *zircon_a;
	const char *zircon_b;
	const char *zircon_r;
	const char *zircon_boot;
	struct kvstore kvs;
	int rc;

	if (argc != 5)
		return -1;

	sysconfig = argv[1];
	zircon_a = argv[2];
	zircon_b = argv[3];
	zircon_r = argv[4];

	uint8_t *buffer = NULL;

	buffer = malloc(ZX_SYSCONFIG_KVSTORE_SIZE);
	if (!buffer) {
		pr_info("alloc buffer fail\n");
		return -1;
	}

	rc = kvs_load(&kvs, buffer, ZX_SYSCONFIG_KVSTORE_SIZE);
	if (rc < 0) {
		rc = store_read(sysconfig, ZX_SYSCONFIG_KVSTORE_SIZE,
				ZX_SYSCONFIG_BOOT_DEFAULT_OFFSET, buffer);
		if (rc) {
			printf("do_sysconfig: store_read_ops failed for %s\n", sysconfig);
			return 0;
		}
		rc = kvs_load(&kvs, buffer, ZX_SYSCONFIG_KVSTORE_SIZE);
		if (rc < 0) {
			printf("do_sysconfig: kvs_load failed for %s (%d)\n", sysconfig, rc);
			return 0;
		}
	}

	const char *boot_part = kvs_get(&kvs, "boot", NULL);

	if (!boot_part) {
		printf("boot partition not set\n");
		return -1;
	}
	if (!strcmp(boot_part, "a")) {
		zircon_boot = zircon_a;
	} else if (!strcmp(boot_part, "b")) {
		zircon_boot = zircon_b;
	} else if (!strcmp(boot_part, "r")) {
		zircon_boot = zircon_r;
	} else {
		printf("invalid boot partition %s, booting to recovery\n", boot_part);
		zircon_boot = zircon_r;
	}

	printf("zircon_bootconfig setting boot_part to %s\n", zircon_boot);
	env_set("boot_part", zircon_boot);

	return 0;
}

U_BOOT_CMD(
	zircon_bootconfig, 5, 0, do_zircon_bootconfig,
	"sets Zircon boot partition based on information in sysconfig partition",
	"    argv: zircon_bootconfig <sysconfig partition> <Zircon-A partition> <Zircon-B partition> <Zircon-R partition>\n"
	"    This command reads the Zircon boot configuration from partition <sysconfig partition>,\n"
	"    The environment variable \"boot_part\" is set to one of the partition names\n"
	"    <Zircon-A partition>,  <Zircon-B partition> or <Zircon-R partition> based on the\n"
	"    configuration in the sysconfig partition.\n"
);

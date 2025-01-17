// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/secure_apb.h>

#ifdef CONFIG_CMD_RSVMEM

#if defined(P_AO_SEC_GP_CFG3)
#define REG_RSVMEM_SIZE        P_AO_SEC_GP_CFG3
#define REG_RSVMEM_BL32_START  P_AO_SEC_GP_CFG4
#define REG_RSVMEM_BL31_START  P_AO_SEC_GP_CFG5
#elif defined(SYSCTRL_SEC_STATUS_REG15)
#define REG_RSVMEM_SIZE        SYSCTRL_SEC_STATUS_REG15
#define REG_RSVMEM_BL32_START  SYSCTRL_SEC_STATUS_REG16
#define REG_RSVMEM_BL31_START  SYSCTRL_SEC_STATUS_REG17
#endif

//#define RSVMEM_DEBUG_ENABLE
#ifdef RSVMEM_DEBUG_ENABLE
#define rsvmem_dbg(fmt...)	printf("[rsvmem] "fmt)
#else
#define rsvmem_dbg(fmt...)
#endif
#define rsvmem_info(fmt...)	printf("[rsvmem] "fmt)
#define rsvmem_err(fmt...)	printf("[rsvmem] "fmt)

#ifndef DTB_BIND_KERNEL
#define RSVMEM_NONE -1
#define RSVMEM_RESERVED	0
#define RSVMEM_CMA	1
#define BL31_SHARE_MEM_SIZE  0x100000
#ifndef BL32_SHARE_MEM_SIZE
#define BL32_SHARE_MEM_SIZE  0x400000
#endif

static int do_rsvmem_check(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	unsigned int data = 0;
	unsigned int bl31_rsvmem_size = 0;
	unsigned int bl32_rsvmem_size = 0;
	unsigned int bl31_rsvmem_start = 0;
	unsigned int bl32_rsvmem_start = 0;
	unsigned int alignment = 0;
	unsigned int alignment_temp = 0;
	unsigned int secure_monitor_size = 0;
	unsigned int secure_monitor_size_final = 0;
	unsigned int ramoops_start = 0;
	int reg_flag = 0;
	char cmdbuf[128];
	char *fdtaddr = NULL;
	int ret = 0;
	char *temp_env = NULL;
	int rsvmemtype = RSVMEM_NONE;
	unsigned int aarch32 = 0;

	rsvmem_dbg("reserved memory check!\n");
	data = readl(REG_RSVMEM_SIZE);
	/* workaround for bl3x size */
	if ((data >> 16) & 0xff) {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 16;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 16;
	} else {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 10;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 10;
	}
	bl31_rsvmem_start = readl(REG_RSVMEM_BL31_START);
	bl32_rsvmem_start = readl(REG_RSVMEM_BL32_START);

	fdtaddr = env_get("fdtaddr");
	if (fdtaddr == NULL) {
		rsvmem_err("get fdtaddr NULL!\n");
		return -1;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt addr %s;", fdtaddr);
	rsvmem_dbg("CMD: %s\n", cmdbuf);
	ret = run_command(cmdbuf, 0);
	if (ret != 0 ) {
		rsvmem_err("fdt addr error.\n");
		return -2;
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_env / \\#address-cells;");
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		rsvmem_err("fdt get size #address-cells failed.\n");
		return -2;
	}
	temp_env = env_get("temp_env");
	//if (temp_env && !strcmp(temp_env, "0x01000000"))
	if (temp_env && !strcmp(temp_env, "0x00000001"))
		aarch32 = 1;

	/* Get alignment size
	 * If arm64, alignment has 2 parameters
	 * The second parameter need convert big-endian to little-endian
	 */
	alignment = 0x400000;
	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value temp_alignment /reserved-memory/linux,secmon alignment;");
	ret = run_command(cmdbuf, 0);
	if (!ret) {
		temp_env = env_get("temp_alignment");
		alignment_temp = simple_strtoul(temp_env, NULL, 16);
		if (aarch32) {
			alignment = alignment_temp;
		} else {
			alignment = (alignment_temp & 0xff) << 24;
			alignment |= (alignment_temp & 0xff00) << 8;
			alignment |= (alignment_temp & 0xff0000) >> 8;
			alignment |= (alignment_temp & 0xff000000) >> 24;
		}
	}

	memset(cmdbuf, 0, sizeof(cmdbuf));
	sprintf(cmdbuf, "fdt get value env_compatible /reserved-memory/linux,secmon compatible;");
	ret = run_command(cmdbuf, 0);
	if (ret != 0) {
		rsvmem_err("fdt get prop fail.\n");
		return -2;
	}
	temp_env = env_get("env_compatible");
	if (strcmp(temp_env, "shared-dma-pool") == 0)
		rsvmemtype = RSVMEM_CMA;
	else if (strcmp(temp_env, "amlogic, aml_secmon_memory") == 0)
		rsvmemtype = RSVMEM_RESERVED;
	else
		rsvmemtype = RSVMEM_NONE;
	if (rsvmemtype == RSVMEM_NONE) {
		rsvmem_err("env set fail.\n");
		return -2;
	}
	run_command("setenv env_compatible;", 0);

	secure_monitor_size = ((bl31_rsvmem_size + alignment - 1) / alignment) * alignment;
	secure_monitor_size_final = bl31_rsvmem_size + bl32_rsvmem_size + alignment - 1;
	secure_monitor_size_final = (secure_monitor_size_final / alignment) * alignment;
	ramoops_start = bl31_rsvmem_start + bl31_rsvmem_size + bl32_rsvmem_size + alignment - 1;
	ramoops_start = (ramoops_start / alignment) * alignment;

	if ((bl31_rsvmem_size > 0) && (bl31_rsvmem_start > 0)) {
		if (rsvmemtype == RSVMEM_RESERVED) {
			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
					bl31_rsvmem_start, bl31_rsvmem_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
					bl31_rsvmem_start, bl31_rsvmem_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set addr error.\n");
				return -3;
			}
		}
		if (rsvmemtype == RSVMEM_CMA) {
			/* Check parameter reg, add for linux 5.15 and before */
			reg_flag = 0;
			memset(cmdbuf, 0, sizeof(cmdbuf));
			sprintf(cmdbuf,
					"fdt get value temp_rsv_reg /reserved-memory/linux,secmon reg;");
			ret = run_command(cmdbuf, 0);
			if (!ret) {
				reg_flag = 1;
				if (aarch32)
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
				else
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret) {
					rsvmem_err("bl31 reserved memory set reg error.\n");
					return -3;
				}
			}

			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x%x>;",
						secure_monitor_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x0 0x%x>;",
						secure_monitor_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set size error.\n");
				/*
				 * If reg exist, to modify bl32,
				 * need not return if modify size failed
				 */
				if (!reg_flag)
					return -3;
			}
			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set alloc-ranges error.\n");
				/*
				 * If reg exist, to modify bl32,
				 * need not return if modify size failed
				 */
				if (!reg_flag)
					return -3;
			}
			memset(cmdbuf, 0, sizeof(cmdbuf));
			sprintf(cmdbuf, "fdt set /secmon reserve_mem_size <0x%x>;",
						bl31_rsvmem_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl31 reserved memory set reserve_mem_size error.\n");
				return -3;
			}
		}
	}

	if ((bl32_rsvmem_size > 0) && (bl32_rsvmem_start > 0)) {
		if ((rsvmemtype == RSVMEM_RESERVED)
				|| ((bl31_rsvmem_start + bl31_rsvmem_size != bl32_rsvmem_start)
					&& (rsvmemtype == RSVMEM_CMA))) {
			memset(cmdbuf, 0, sizeof(cmdbuf));
			sprintf(cmdbuf, "fdt set /reserved-memory/linux,secos status okay;");
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl32 reserved memory set status error.\n");
				return -3;
			}
			memset(cmdbuf, 0, sizeof(cmdbuf));
			if (aarch32)
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secos reg <0x%x 0x%x>;",
					bl32_rsvmem_start, bl32_rsvmem_size);
			else
				sprintf(cmdbuf, "fdt set /reserved-memory/linux,secos reg <0x0 0x%x 0x0 0x%x>;",
					bl32_rsvmem_start, bl32_rsvmem_size);
			rsvmem_dbg("CMD: %s\n", cmdbuf);
			ret = run_command(cmdbuf, 0);
			if (ret != 0 ) {
				rsvmem_err("bl32 reserved memory set addr error.\n");
				return -3;
			}
		}
		if ((bl31_rsvmem_start + bl31_rsvmem_size == bl32_rsvmem_start)
				&& (rsvmemtype == RSVMEM_CMA)) {
			/* Modify parameter reg, add for linux 5.15 and before */
			if (reg_flag) {
				if (aarch32)
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				else
					sprintf(cmdbuf,
						"fdt set /reserved-memory/linux,secmon reg <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret) {
					rsvmem_err("bl32 reserved memory set reg error.\n");
					return -3;
				}
			}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				if (aarch32)
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x%x>;",
						secure_monitor_size_final);
				else
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon size <0x0 0x%x>;",
						secure_monitor_size_final);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret != 0 ) {
					rsvmem_err("bl32 reserved memory set size error.\n");
					/*
					 * If reg exist, to modify reserve_mem_size,
					 * need not return if modify size failed
					 */
					if (!reg_flag)
						return -3;
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				if (aarch32)
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x%x 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				else
					sprintf(cmdbuf, "fdt set /reserved-memory/linux,secmon alloc-ranges <0x0 0x%x 0x0 0x%x>;",
						bl31_rsvmem_start, secure_monitor_size_final);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret != 0 ) {
					rsvmem_err("bl32 reserved memory set alloc-ranges error.\n");
					/*
					 * If reg exist, to modify reserve_mem_size,
					 * need not return if modify size failed
					 */
					if (!reg_flag)
						return -3;
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "fdt set /secmon reserve_mem_size <0x%x>;",
						bl31_rsvmem_size + bl32_rsvmem_size);
				rsvmem_dbg("CMD: %s\n", cmdbuf);
				ret = run_command(cmdbuf, 0);
				if (ret != 0 ) {
					rsvmem_err("bl32 reserved memory set reserve_mem_size error.\n");
					return -3;
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "fdt get value ramoops_reg /reserved-memory/ramoops reg;");
				if (run_command(cmdbuf, 0) == 0) {
					memset(cmdbuf, 0, sizeof(cmdbuf));
					if (aarch32)
						sprintf(cmdbuf, "fdt set /reserved-memory/ramoops reg <0x%x 0x%x>;",
								ramoops_start, 0x100000);
					else
						sprintf(cmdbuf, "fdt set /reserved-memory/ramoops reg <0x0 0x%x 0x0 0x%x>;",
								ramoops_start, 0x100000);

					rsvmem_dbg("CMD: %s\n", cmdbuf);
					ret = run_command(cmdbuf, 0);
					if (ret != 0 ) {
						rsvmem_err("fdt set /reserved-memory/ramoops reg  error.\n");
						return -3;
					}
				}

				memset(cmdbuf, 0, sizeof(cmdbuf));
				sprintf(cmdbuf, "fdt get value secmon_clear_range /secmon clear_range;");
				if (run_command(cmdbuf, 0) == 0) {
					memset(cmdbuf, 0, sizeof(cmdbuf));
					sprintf(cmdbuf, "fdt set /secmon clear_range <0x%x 0x%x>;",
							bl31_rsvmem_start + BL31_SHARE_MEM_SIZE , bl31_rsvmem_size + bl32_rsvmem_size
							- BL31_SHARE_MEM_SIZE - BL32_SHARE_MEM_SIZE);
					rsvmem_dbg("CMD: %s\n", cmdbuf);
					ret = run_command(cmdbuf, 0);
					if (ret != 0 ) {
						rsvmem_err("bl32 reserved memory set clear_range error.\n");
						return -3;
					}
				}
		}
	}

	return ret;
}

static int do_rsvmem_dump(cmd_tbl_t *cmdtp, int flag, int argc,
		char *const argv[])
{
	unsigned int data = 0;
	unsigned int bl31_rsvmem_size = 0;
	unsigned int bl32_rsvmem_size = 0;
	unsigned int bl31_rsvmem_start = 0;
	unsigned int bl32_rsvmem_start = 0;

	rsvmem_info("reserved memory:\n");
	data = readl(REG_RSVMEM_SIZE);
	/* workaround for bl3x size */
	if ((data >> 16) & 0xff) {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 16;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 16;
	} else {
		bl31_rsvmem_size =  ((data & 0xffff0000) >> 16) << 10;
		bl32_rsvmem_size =  (data & 0x0000ffff) << 10;
	}
	bl31_rsvmem_start = readl(REG_RSVMEM_BL31_START);
	bl32_rsvmem_start = readl(REG_RSVMEM_BL32_START);

	rsvmem_info("bl31 reserved memory start: 0x%08x\n", bl31_rsvmem_start);
	rsvmem_info("bl31 reserved memory size:  0x%08x\n", bl31_rsvmem_size);
	rsvmem_info("bl32 reserved memory start: 0x%08x\n", bl32_rsvmem_start);
	rsvmem_info("bl32 reserved memory size:  0x%08x\n", bl32_rsvmem_size);

	return 0;
}

static cmd_tbl_t cmd_rsvmem_sub[] = {
	U_BOOT_CMD_MKENT(check, 2, 0, do_rsvmem_check, "", ""),
	U_BOOT_CMD_MKENT(dump, 2, 0, do_rsvmem_dump, "", ""),
};
#endif

static int do_rsvmem(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
#ifdef DTB_BIND_KERNEL
	rsvmem_err("no check for rsvmem, should check int kernel\n");
	return 0;
#else
	cmd_tbl_t *c;

	/* Strip off leading 'rsvmem' command argument */
	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_rsvmem_sub[0], ARRAY_SIZE(cmd_rsvmem_sub));

	if (c) {
		int ret = c->cmd(cmdtp, flag, argc, argv);

		if (ret < 0) {
			/*only negative -1 is identifid as fail */
			ret = -1;
		}
		return ret;
	} else {
		return CMD_RET_USAGE;
	}
#endif
}

U_BOOT_CMD(
	rsvmem, 2, 0,	do_rsvmem,
	"reserve memory",
	"check                   - check reserved memory\n"
	"rsvmem dump                    - dump reserved memory\n"
);
#endif

// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/io.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <amlogic/cpu_id.h>
#include <asm/arch/secure_apb.h>
#include <asm/arch/pinctrl_init.h>
#include <linux/sizes.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#include <asm/armv8/mmu.h>
#include <amlogic/aml_v3_burning.h>
#include <amlogic/aml_v2_burning.h>
#include <linux/mtd/partitions.h>

DECLARE_GLOBAL_DATA_PTR;

void sys_led_init(void)
{
}

int serial_set_pin_port(unsigned long port_base)
{
    return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

unsigned int get_dram_size(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4);
#endif /* CONFIG_SYS_MEM_TOP_HIDE */
}


int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = 0;
	gd->bd->bi_dram[0].size = get_dram_size();
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is is only for compiling pass
 * */
void secondary_boot_func(void)
{
}

int board_eth_init(bd_t *bis)
{
	return 0;
}

int active_clk(void)
{
	struct udevice *clk = NULL;
	int err;

	err = uclass_get_device_by_name(UCLASS_CLK,
			"xtal-clk", &clk);
	if (err) {
		pr_err("Can't find xtal-clk clock (%d)\n", err);
		return err;
	}
	err = uclass_get_device_by_name(UCLASS_CLK,
			"clock-controller@0", &clk);
	if (err) {
		pr_err("Can't find clock-controller@0 clock (%d)\n", err);
		return err;
	}

	return 0;
}

void board_init_mem(void) {
	/* config bootm low size, make sure whole dram/psram space can be used */
	phys_size_t ram_size;
	char *env_tmp;
	env_tmp = env_get("bootm_size");
	if (!env_tmp) {
		ram_size = (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4);
		env_set_hex("bootm_low", 0);
		env_set_hex("bootm_size", ram_size);
	}
}

/* set dts props */
void aml_config_dtb(void)
{
	cpu_id_t cpuid = get_cpu_id();
	if (MESON_CPU_MAJOR_ID_C2 != cpuid.family_id)
		return;

	run_command("fdt address $dtb_mem_addr", 0);
	printf("%s %d\n", __func__, __LINE__);
	if (cpuid.chip_rev == 0xA) {
		printf("%s %d\n", __func__, __LINE__);
		run_command("fdt rm /soc/emmc mmc-hs400-1_8v", 0);
		run_command("fdt rm /soc/emmc fixed-emmc-driver-type", 0);
		run_command("fdt print /soc/emmc", 0);
	} else {
		printf("%s %d\n", __func__, __LINE__);
		run_command("fdt set /soc/bus/pinctrl/emmc/mux drive-strength <4>", 0);
		run_command("fdt set /soc/bus/pinctrl/emmc/mux1 drive-strength <4>", 0);
		run_command("fdt set /soc/bus/pinctrl/emmc_clk_gate/mux drive-strength <4>", 0);
		run_command("fdt print /soc/bus/pinctrl/emmc", 0);
		run_command("fdt print /soc/bus/pinctrl/emmc_clk_gate", 0);
	}
}

int board_init(void)
{
	printf("board init\n");
	//Please keep try usb boot first in board_init, as other init before usb may cause burning failure
#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
	if ((0x1b8ec003 != readl(SYSCTRL_SEC_STICKY_REG2)) && (0x1b8ec004 != readl(SYSCTRL_SEC_STICKY_REG2)))
	{ aml_v3_factory_usb_burning(0, gd->bd); }
#endif//#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)

	pinctrl_devices_active(PIN_CONTROLLER_NUM);
	active_clk();

	run_command("gpio c GPIOA_10", 0);
	run_command("gpio c GPIOA_12", 0);

	//wifi reset
	run_command("gpio c GPIOM_6", 0);
	printf("wifi chip en down\n");
	mdelay(100);
	run_command("gpio s GPIOM_6", 0);
	printf("wifi chip en up\n");

	return 0;
}

int board_late_init(void)
{
	printf("board late init\n");

	//default uboot env need before anyone use it
	if (env_get("default_env")) {
		printf("factory reset, need default all uboot env.\n");
		run_command("defenv_reserv; setenv upgrade_step 2; saveenv;", 0);
	}

	run_command("echo upgrade_step $upgrade_step; if itest ${upgrade_step} == 1; then "\
			"defenv_reserv; setenv upgrade_step 2; saveenv; fi;", 0);
	board_init_mem();

#ifndef CONFIG_SYSTEM_RTOS //pure rtos not need dtb
	if ( run_command("run common_dtb_load", 0) ) {
		printf("Fail in load dtb with cmd[%s]\n", env_get("common_dtb_load"));
	} else {
		//load dtb here then users can directly use 'fdt' command
		run_command("if fdt addr ${dtb_mem_addr}; then else echo no valid dtb at ${dtb_mem_addr};fi;", 0);
	}
#endif//#ifndef CONFIG_SYSTEM_RTOS //pure rtos not need dtb

#ifdef CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE //try auto upgrade from ext-sdcard
	aml_try_factory_sdcard_burning(0, gd->bd);
#endif//#ifdef CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE
	//auto enter usb mode after board_late_init if 'adnl.exe setvar burnsteps 0x1b8ec003'
#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
	if (0x1b8ec003 == readl(SYSCTRL_SEC_STICKY_REG2))
	{ aml_v3_factory_usb_burning(0, gd->bd); }
#endif//#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)

	aml_config_dtb();

	unsigned char chipid[16];

	memset(chipid, 0, 16);

	if (get_chip_id(chipid, 16) != -1) {
		char chipid_str[32];
		int i, j;
		char buf_tmp[4];

		memset(chipid_str, 0, 32);

		char *buff = &chipid_str[0];

		for (i = 0, j = 0; i < 12; ++i) {
			sprintf(&buf_tmp[0], "%02x", chipid[15 - i]);
			if (strcmp(buf_tmp, "00") != 0) {
				sprintf(buff + j, "%02x", chipid[15 - i]);
				j = j + 2;
			}
		}
		env_set("cpu_id", chipid_str);
		printf("buff: %s\n", buff);
	} else {
		env_set("cpu_id", "1234567890");
	}

	return 0;
}


phys_size_t get_effective_memsize(void)
{
#ifdef UBOOT_RUN_IN_SRAM
	return 0x180000; /* SRAM 1.5MB */
#else
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4);
#endif /* CONFIG_SYS_MEM_TOP_HIDE */
#endif /* UBOOT_RUN_IN_SRAM */
}

static struct mm_region bd_mem_map[] = {
	{
		.virt = 0x00000000UL,
		.phys = 0x00000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = bd_mem_map;

int mach_cpu_init(void) {
	printf("\nmach_cpu_init\n");
	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	/* eg: bl31/32 rsv */
	return 0;
}

/*
* mtd nand partition table, only care the size!
* offset will be calculated by nand driver.
*/
#ifdef CONFIG_MESON_NFC
static struct mtd_partition normal_partition_info[] = {
/* MUST NOT CHANGE this part unless u know what you are doing!
* inherent partition for discrete bootloader to store fip
* size is determind by TPL_SIZE_PER_COPY*TPL_COPY_NUM
* name must be same with TPL_PART_NAME
*/
{
	.name = "tpl",
	.offset = 0,
	.size = 0,
},
{
	.name = "recovery",
	.offset = 0,
	.size = 32*SZ_1M,
},
{
	.name = "boot",
	.offset = 0,
	.size = 32*SZ_1M,
},
{
	.name = "system",
	.offset = 0,
	.size = 304*SZ_1M,
},
/* last partition get the rest capacity */
{
	.name = "data",
	.offset = MTDPART_OFS_APPEND,
	.size = MTDPART_SIZ_FULL,
},
};

struct mtd_partition *get_aml_mtd_partition(void)
{
	return normal_partition_info;
}

int get_aml_partition_count(void)
{
	return ARRAY_SIZE(normal_partition_info);
}
#endif

/* partition table */
/* partition table for spinand flash */
#if (defined(CONFIG_SPI_NAND) || defined(CONFIG_MTD_SPI_NAND))
static const struct mtd_partition spinand_partitions[] = {
	{
		.name = "logo",
		.offset = 0,
		.size = 2 * SZ_1M,
	},
	{
		.name = "recovery",
		.offset = 0,
		.size = 16 * SZ_1M,
	},
	{
		.name = "boot",
		.offset = 0,
		.size = 16 * SZ_1M,
	},
	{
		.name = "system",
		.offset = 0,
		.size = 64 * SZ_1M,
	},
	/* last partition get the rest capacity */
	{
		.name = "data",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	}
};
struct mtd_partition *get_spinand_partition_table(int *partitions)
{
	*partitions = ARRAY_SIZE(spinand_partitions);
	return spinand_partitions;
}
#endif /* CONFIG_SPI_NAND */

/* partition table for spinor flash */
#ifdef CONFIG_SPI_FLASH
static const struct mtd_partition spiflash_partitions[] = {
	{
		.name = "env",
		.offset = 0,
		.size = 1 * SZ_256K,
	},
	{
		.name = "misc",
		.offset = 0,
		.size = 8 * SZ_8K,
	},
	/* last partition get the rest capacity */
	{
		.name = "user",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	}
};
const struct mtd_partition *get_spiflash_partition_table(int *partitions)
{
	*partitions = ARRAY_SIZE(spiflash_partitions);
	return spiflash_partitions;
}
#endif /* CONFIG_SPI_FLASH */

int __attribute__((weak)) mmc_initialize(bd_t *bis){ return 0;}

int __attribute__((weak)) do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]){ return 0;}

void __attribute__((weak)) set_working_fdt_addr(ulong addr) {}

int __attribute__((weak)) ofnode_read_u32_default(ofnode node, const char *propname, u32 def) {return 0;}

void __attribute__((weak)) md5_wd (unsigned char *input, int len, unsigned char output[16],	unsigned int chunk_sz){}

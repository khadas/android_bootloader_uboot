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
#include <linux/mtd/partitions.h>
#include <dt-bindings/gpio/meson-a1-gpio.h>
#include <amlogic/saradc.h>

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

#ifdef CONFIG_MULTI_DTB
int checkhw(char *name)
{
	return 0;
}
#endif

/* secondary_boot_func
 * this function should be write with asm, here, it is only for compiling pass
 */
void secondary_boot_func(void)
{
}

int board_eth_init(bd_t *bis)
{
	return 0;
}

int active_a1_clk(void)
{
	struct udevice *a1_clk = NULL;
	int err;

	err = uclass_get_device_by_name(UCLASS_CLK,
			"xtal-clk", &a1_clk);
	if (err) {
		pr_err("Can't find xtal-clk clock (%d)\n", err);
		return err;
	}
	err = uclass_get_device_by_name(UCLASS_CLK,
			"clock-controller@0", &a1_clk);
	if (err) {
		pr_err("Can't find clock-controller@0 clock (%d)\n", err);
		return err;
	}

	return 0;
}

void board_init_mem(void)
{
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

int board_init(void)
{
	pr_info("board init\n");
	/* reset uart A for BT*/
	writel(0x4000000, RESETCTRL_RESET1);
	/*Please keep try usb boot first in board_init*/
	/*as other init before usb may cause burning failure*/
#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
	if ((readl(SYSCTRL_SEC_STICKY_REG2) != 0x1b8ec003) &&
		(readl(SYSCTRL_SEC_STICKY_REG2) != 0x1b8ec004))
		aml_v3_factory_usb_burning(0, gd->bd);
#endif//#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)

	pinctrl_devices_active(PIN_CONTROLLER_NUM);
	active_a1_clk();

	return 0;
}

int read_adc_key(void)
{
#define KEY_NONE 0
#define KEY_USB_UPDATE 1

	struct udevice *dev;
	unsigned int val;
	int ret, sec = 5, key_mode = KEY_NONE;

	env_set("reboot_mode", "cold_boot");
	ret = uclass_get_device_by_name(UCLASS_ADC, "adc", &dev);
	if (ret)
		return ret;
	ret = adc_set_mode(dev, 0, ADC_MODE_AVERAGE);
	if (ret) {
		pr_err("set adc mode fail\n");
		return ret;
	}

	while (sec) {
		ret = adc_channel_single_shot_mode("adc", ADC_MODE_AVERAGE,  0, &val);
		if (ret)
			return ret;

		if (val < 50) {
			key_mode = KEY_USB_UPDATE;
			sec--;
			mdelay(1000);
		} else {
			key_mode = KEY_NONE;
			break;
		}
	}

	if (!sec) {
		if (key_mode == KEY_USB_UPDATE)
			run_command("adnl", 0);
	}
	return 0;
#undef KEY_NONE
#undef KEY_USB_UPDATE
}

int board_late_init(void)
{
#define UPGRADE_CMD "echo upgrade_step $upgrade_step; if itest ${upgrade_step} == 1; then "\
	"defenv_reserv; setenv upgrade_step 2; saveenv; fi;"
#define CHECK_FDT_CMD "if fdt addr ${dtb_mem_addr}; "\
	"then else echo no valid dtb at ${dtb_mem_addr};fi;"

	pr_info("board late init\n");

	//default uboot env need before anyone use it
	if (env_get("default_env")) {
		pr_info("factory reset, need default all uboot env.\n");
		run_command("defenv_reserv; setenv upgrade_step 2; saveenv;", 0);
	}

	run_command(UPGRADE_CMD, 0);
	board_init_mem();

#ifndef CONFIG_SYSTEM_RTOS //pure rtos not need dtb
	if (run_command("run common_dtb_load", 0)) {
		pr_info("Fail in load dtb with cmd[%s]\n", env_get("common_dtb_load"));
	} else {
		//load dtb here then users can directly use 'fdt' command
		run_command(CHECK_FDT_CMD, 0);
	}
#endif//#ifndef CONFIG_SYSTEM_RTOS //pure rtos not need dtb

	//auto enter usb mode after board_late_init if 'adnl.exe setvar burnsteps 0x1b8ec003'
#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)
	if (readl(SYSCTRL_SEC_STICKY_REG2) == 0x1b8ec003)
		aml_v3_factory_usb_burning(0, gd->bd);
#endif//#if defined(CONFIG_AML_V3_FACTORY_BURN) && defined(CONFIG_AML_V3_USB_TOOl)

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
		pr_info("buff: %s\n", buff);
	} else {
		env_set("cpu_id", "1234567890");
	}

	read_adc_key();
	return 0;

#undef UPGRADE_CMD
#undef CHECK_FDT_CMD
}

phys_size_t get_dram_size(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return (((readl(SYSCTRL_SEC_STATUS_REG4)) & 0xFFFF0000) << 4);
#endif /* CONFIG_SYS_MEM_TOP_HIDE */
}

phys_size_t get_effective_memsize(void)
{
#ifdef CONFIG_UBOOT_RUN_IN_SRAM
	return 0x180000; /* SRAM 1.5MB */
#else
	return get_dram_size();
#endif /* CONFIG_UBOOT_RUN_IN_SRAM */
}

#ifdef CONFIG_UBOOT_RUN_IN_SRAM
ulong board_get_usable_ram_top(ulong total_size)
{
	return (PHYS_SDRAM_1_BASE + PHYS_SDRAM_1_SIZE);
}
#endif

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = 0;
	gd->bd->bi_dram[0].size = get_dram_size();
	return 0;
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
		.size = 0x7FE00000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		.virt = 0xFFE00000UL,
		.phys = 0xFFE00000UL,
		.size = 0x00200000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = bd_mem_map;

int mach_cpu_init(void)
{
	pr_info("\nmach_cpu_init\n");
	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	/* eg: bl31/32 rsv */
	return 0;
}

/* partition table */
/* partition table for spinand flash */
#if (defined(CONFIG_SPI_NAND) || defined(CONFIG_MTD_SPI_NAND))
#ifdef CONFIG_SYSTEM_RTOS
static const struct mtd_partition spinand_partitions[] = {
	{
		.name = "logo",
		.offset = 0,
		.size = 2 * SZ_1M,
	},
	{
		.name = "boot",
		.offset = 0,
		.size = 16 * SZ_1M,
	},
	{
		.name = "dspA",
		.offset = 0,
		.size = 16 * SZ_1M,
	},
	{
		.name = "dspB",
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
#else /*CONFIG_SYSTEM_RTOS*/
static const struct mtd_partition spinand_partitions[] = {
	{
		.name = "misc",
		.offset = 0,
		.size = 1 * SZ_256K,
	},
	{
		.name = "recovery",
		.offset = 0,
		.size = 16 * SZ_1M,
	},
	{
		.name = "boot",
		.offset = 0,
		.size = 12 * SZ_1M,
	},
	{
		.name = "system",
		.offset = 0,
		.size = 56 * SZ_1M,
	},
	/* last partition get the rest capacity */
	{
		.name = "data",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
	}
};
#endif /*CONFIG_SYSTEM_RTOS*/
const struct mtd_partition *get_spinand_partition_table(int *partitions)
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
	{
		.name = "boot",
		.offset = 0,
		.size = 3 * SZ_1M,
	},
	{
		.name = "dspA",
		.offset = 0,
		.size = 1 * SZ_512K,
	},
	{
		.name = "dspB",
		.offset = 0,
		.size = 1 * SZ_512K,
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

uint64_t spiflash_bootloader_size(void)
{
	return 1 * SZ_1M;
}
#endif /* CONFIG_SPI_FLASH */

int __attribute__((weak)) mmc_initialize(bd_t *bis) { return 0; }

int __attribute__((weak))
do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) { return 0; }

void __attribute__((weak)) set_working_fdt_addr(ulong addr) {}

int __attribute__((weak))
ofnode_read_u32_default(ofnode node, const char *propname, u32 def) { return 0; }

void __attribute__((weak))
md5_wd(unsigned char *input, int len, unsigned char output[16], unsigned int chunk_sz) {}

// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2012-2014 Daniel Schwierzeck, daniel.schwierzeck@gmail.com
 */

#include <common.h>
#include <malloc.h>
#include <linux/errno.h>
#include <linux/mtd/mtd.h>
#include <spi_flash.h>
#include <linux/log2.h>
#include <amlogic/aml_mtd.h>
#include <amlogic/aml_pageinfo.h>
#include <amlogic/cpu_id.h>

#ifdef CONFIG_AML_MTDPART
extern int spinor_add_partitions(struct mtd_info *mtd);
extern int spinor_del_partitions(struct mtd_info *mtd);
#endif

static struct mtd_info sf_mtd_info;
static bool sf_mtd_registered;
static char sf_mtd_name[8];

#ifdef CONFIG_AML_STORAGE
struct mtd_info *spi_flash_get_mtd(void)
{
	return &sf_mtd_info;
}
#endif

static int spi_flash_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct spi_flash *flash = mtd->priv;
	int err;

	if (!flash)
		return -ENODEV;

	instr->state = MTD_ERASING;

	err = spi_flash_erase(flash, instr->addr, instr->len);
	if (err) {
		instr->state = MTD_ERASE_FAILED;
		instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
		return -EIO;
	}

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static int spi_flash_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct spi_flash *flash = mtd->priv;
	int err;

	if (!flash)
		return -ENODEV;

	err = spi_flash_read(flash, from, len, buf);
	if (!err)
		*retlen = len;

	return err;
}

static int spi_flash_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct spi_flash *flash = mtd->priv;
	cpu_id_t cpu_id = get_cpu_id();
	unsigned char *page_info;
	int err;

	if (!flash)
		return -ENODEV;

	if (to == 512 && cpu_id.family_id == MESON_CPU_MAJOR_ID_A4) {
		page_info = page_info_post_init(mtd, flash->dev);
		err = spi_flash_write(flash, 0, 512, page_info);
		if (err)
			return err;
	}
	err = spi_flash_write(flash, to, len, buf);
	if (!err)
		*retlen = len;

	return err;
}

static void spi_flash_mtd_sync(struct mtd_info *mtd)
{
}

static int spi_flash_mtd_number(void)
{
#ifdef CONFIG_SYS_MAX_FLASH_BANKS
	return CONFIG_SYS_MAX_FLASH_BANKS;
#else
	return 0;
#endif
}

int spi_flash_mtd_register(struct spi_flash *flash)
{
	int ret;

	if (sf_mtd_registered) {
	#ifndef CONFIG_AML_MTDPART
		ret = del_mtd_device(&sf_mtd_info);
	#else
		ret = spinor_del_partitions(&sf_mtd_info);
	#endif
		if (ret)
			return ret;

		sf_mtd_registered = false;
	}

	sf_mtd_registered = false;
	memset(&sf_mtd_info, 0, sizeof(sf_mtd_info));
	sprintf(sf_mtd_name, "nor%d", spi_flash_mtd_number());

	sf_mtd_info.name = sf_mtd_name;
	sf_mtd_info.type = MTD_NORFLASH;
	sf_mtd_info.flags = MTD_CAP_NORFLASH;
	sf_mtd_info.writesize = 1;
	sf_mtd_info.writebufsize = flash->page_size;

	sf_mtd_info._erase = spi_flash_mtd_erase;
	sf_mtd_info._read = spi_flash_mtd_read;
	sf_mtd_info._write = spi_flash_mtd_write;
	sf_mtd_info._sync = spi_flash_mtd_sync;

	sf_mtd_info.size = flash->size;
	sf_mtd_info.priv = flash;

	/* Only uniform flash devices for now */
	sf_mtd_info.numeraseregions = 0;
	sf_mtd_info.erasesize = flash->sector_size;

	ret = add_mtd_device(&sf_mtd_info);
	if (!ret)
		sf_mtd_registered = true;

#ifdef CONFIG_AML_MTDPART
	/*
	 * add_mtd_device must be before spinor_add_partitions
	 * because add_mtd_device will init mtd->partitions
	 */
	if (is_power_of_2(sf_mtd_info.erasesize))
		sf_mtd_info.erasesize_shift = ffs(sf_mtd_info.erasesize) - 1;
	ret = spinor_add_partitions(&sf_mtd_info);
#endif /* CONFIG_AML_MTDPART */
	return ret;
}

void spi_flash_mtd_unregister(void)
{
	int ret;

	if (!sf_mtd_registered)
		return;

	ret = del_mtd_device(&sf_mtd_info);
	if (!ret) {
		sf_mtd_registered = false;
		return;
	}

	/*
	 * Setting mtd->priv to NULL is the best we can do. Thanks to that,
	 * the MTD layer can still call mtd hooks without risking a
	 * use-after-free bug. Still, things should be fixed to prevent the
	 * spi_flash object from being destroyed when del_mtd_device() fails.
	 */
	sf_mtd_info.priv = NULL;
	printf("Failed to unregister MTD %s and the spi_flash object is going away: you're in deep trouble!",
	       sf_mtd_info.name);
}

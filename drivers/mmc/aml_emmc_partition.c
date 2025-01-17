// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <errno.h>
#include <mmc.h>
#include <part.h>
#include <memalign.h>
#include <malloc.h>
#include <linux/list.h>
#include <div64.h>
#include "mmc_private.h"
#include <emmc_partitions.h>
#include <amlogic/cpu_id.h>
#include <part_efi.h>
#include <partition_table.h>
#include <linux/compat.h>

DECLARE_GLOBAL_DATA_PTR;
/* using mbr*/
#if (CONFIG_PTBL_MBR)
	/* cmpare partition name? */
	#define CONFIG_CMP_PARTNAME	(0)
	/* cmpare partition mask */
	#define CONFIG_CMP_PARTMASK	(0)
#else
	#define CONFIG_CMP_PARTNAME	(1)
	#define CONFIG_CMP_PARTMASK	(1)
#endif
/* debug info*/
#define CONFIG_MPT_DEBUG 	(0)
#define GPT_SIZE      0x4400

#define apt_err(fmt, ...) printf( "%s()-%d: " fmt , \
                  __func__, __LINE__, ##__VA_ARGS__)

#define apt_wrn(fmt, ...) printf( "%s()-%d: " fmt , \
                  __func__, __LINE__, ##__VA_ARGS__)
#if (CONFIG_MPT_DEBUG)
/* for detail debug info */
#define apt_info(fmt, ...) printf( "%s()-%d: " fmt , \
                  __func__, __LINE__, ##__VA_ARGS__)
#else
#define apt_info(fmt, ...)
#endif

/* creat MBR for emmc */
#define MAX_PNAME_LEN	MAX_MMC_PART_NAME_LEN
#define MAX_PART_COUNT	MAX_MMC_PART_NUM

/*
  Global offset of reserved partition is 36MBytes
  since MMC_BOOT_PARTITION_RESERVED is 32MBytes and
  MMC_BOOT_DEVICE_SIZE is 4MBytes.
  MMC_RESERVED_SIZE is 64MBytes for now.
  layout detail inside reserved partition.
  0x000000 - 0x003fff: partition table
  0x004000 - 0x03ffff: storage key area	(16k offset & 256k size)
  0x400000 - 0x47ffff: dtb area  (4M offset & 512k size)
  0x480000 - 64MBytes: resv for other usage.
  ...
 */
/*
#define RSV_DTB_OFFSET_GLB	(SZ_1M*40)
#define RSV_DTB_SIZE		(512*1024UL)
#define RSV_PTBL_OFFSET		(SZ_1M*0)
#define RSV_PTBL_SIZE		(16*1024UL)
#define RSV_SKEY_OFFSET		(16*1024UL)
#define RSV_SKEY_SIZE		(256*1024UL)
#define RSV_DTB_OFFSET		(SZ_1M*4)
*/

/* virtual partitions which are in "reserved" */
#define MAX_MMC_VIRTUAL_PART_CNT	(5)

/* BinaryLayout of partition table stored in rsv area */
struct ptbl_rsv {
    char magic[4];				/* MPT */
    unsigned char version[12];	/* binary version */
    int count;	/* partition count in using */
    int checksum;
    struct partitions partitions[MAX_MMC_PART_NUM];
};

/* partition table for innor usage*/
struct _iptbl {
	struct partitions *partitions;
	int count;	/* partition count in use */
};

unsigned device_boot_flag = 0xff;
extern bool is_partition_checked;

#ifndef CONFIG_AML_MMC_INHERENT_PART
/* fixme, name should be changed as aml_inherent_ptbl */
struct partitions emmc_partition_table[] = {
	PARTITION_ELEMENT(MMC_BOOT_NAME, MMC_BOOT_DEVICE_SIZE, 0),
	PARTITION_ELEMENT(MMC_RESERVED_NAME, MMC_RESERVED_SIZE, 0),
	/* prior partitions, same partition name with dts*/
	/* partition size will be override by dts*/
	PARTITION_ELEMENT(MMC_CACHE_NAME, 0, 0),
	PARTITION_ELEMENT(MMC_ENV_NAME, MMC_ENV_SIZE, 0),
#ifdef CONFIG_AB_UPDATE
	PARTITION_ELEMENT(FIP_A_NAME, FIP_SIZE, 0),
	PARTITION_ELEMENT(FIP_B_NAME, FIP_SIZE, 0),
#endif
};

struct virtual_partition virtual_partition_table[] = {
    /* partition for name idx, off & size will not be used! */
#if (CONFIG_PTBL_MBR)
    VIRTUAL_PARTITION_ELEMENT(MMC_MBR_NAME, MMC_MBR_OFFSET, MMC_MBR_SIZE),
#endif
    VIRTUAL_PARTITION_ELEMENT(MMC_BOOT_NAME0, 0, 0),
    VIRTUAL_PARTITION_ELEMENT(MMC_BOOT_NAME1, 0, 0),

    /* virtual partition in reserved partition, take care off and size */
#ifdef CONFIG_AML_PARTITION
	VIRTUAL_PARTITION_ELEMENT(MMC_TABLE_NAME, MMC_TABLE_OFFSET, MMC_TABLE_SIZE),
#endif
	VIRTUAL_PARTITION_ELEMENT(MMC_KEY_NAME, EMMCKEY_RESERVE_OFFSET, MMC_KEY_SIZE),
	VIRTUAL_PARTITION_ELEMENT(MMC_PATTERN_NAME, CALI_PATTERN_OFFSET, CALI_PATTERN_SIZE),
	VIRTUAL_PARTITION_ELEMENT(MMC_MAGIC_NAME, MAGIC_OFFSET, MAGIC_SIZE),
	VIRTUAL_PARTITION_ELEMENT(MMC_RANDOM_NAME, RANDOM_OFFSET, RANDOM_SIZE),
#ifndef DTB_BIND_KERNEL
	VIRTUAL_PARTITION_ELEMENT(MMC_DTB_NAME, DTB_OFFSET, DTB_SIZE),
#endif
	VIRTUAL_PARTITION_ELEMENT(MMC_FASTBOOT_CONTEXT_NAME,
			FASTBOOT_CONTEXT_OFFSET, FASTBOOT_CONTEXT_SIZE),
	VIRTUAL_PARTITION_ELEMENT(MMC_DDR_PARAMETER_NAME, DDR_PARAMETER_OFFSET, DDR_PARAMETER_SIZE),
	VIRTUAL_PARTITION_ELEMENT(MMC_GPT_ALT_NAME, MMC_GPT_ALT_OFFSET, MMC_GPT_ALT_SIZE),
};

int get_emmc_partition_arraysize(void)
{
    return ARRAY_SIZE(emmc_partition_table);
}

int get_emmc_virtual_partition_arraysize(void)
{
    return ARRAY_SIZE(virtual_partition_table);
}

#endif

void __attribute__((unused)) _dump_part_tbl(struct partitions *p, int count)
{
	int i = 0;
	apt_info("count %d\n", count);
	while (i < count) {
		printf("%02d %10s %016llx %016llx\n", i, p[i].name, p[i].offset, p[i].size);
		i++;
	}
	return;
}

static int _get_part_index_by_name(struct partitions *tbl,
					   int cnt, const char *name)
{
	int i = 0;
	struct partitions *part = NULL;

	while (i < cnt) {
		part = &tbl[i];
		if (!strcmp(name, part->name)) {
			apt_info("find %s @ tbl[%d]\n", name, i);
			break;
		}
		i++;
	};

	if (i == cnt) {
		i = -1;
		apt_wrn("do not find match in table %s\n", name);
	}

	if (gpt_partition)
		i += 1;

	return i;
}

static struct partitions *_find_partition_by_name(struct partitions *tbl,
			int cnt, const char *name)
{
	int i = 0;
	struct partitions *part = NULL;

	while (i < cnt) {
		part = &tbl[i];
		if (!strcmp(name, part->name)) {
			apt_info("find %s @ tbl[%d]\n", name, i);
			break;
		}
		i++;
	};
	if (i == cnt) {
		part = NULL;
		apt_wrn("do not find match in table %s\n", name);
	}
	return part;
}

/* fixme, must called after offset was calculated. */
static ulong _get_inherent_offset(const char *name)
{
	struct partitions *part;

	part = _find_partition_by_name(emmc_partition_table,
			get_emmc_partition_arraysize(), name);
	if (NULL == part)
		return -1;
	else
		return part->offset;
}
/* partition table (Emmc Partition Table) */
struct _iptbl *p_iptbl_ept = NULL;

/* trans byte into lba manner for rsv area read/write */
#ifdef CONFIG_AML_PARTITION
static ulong _mmc_rsv_read(struct mmc *mmc, ulong offset, ulong size, void * buffer)
{
	lbaint_t _blk, _cnt;
	if (0 == size)
		return 0;

	_blk = offset / mmc->read_bl_len;
	_cnt = size / mmc->read_bl_len;
	_cnt = blk_dread(mmc_get_blk_desc(mmc), _blk, _cnt, buffer);

	return (ulong)(_cnt * mmc->read_bl_len);
}

static ulong _mmc_rsv_write(struct mmc *mmc, ulong offset, ulong size, void * buffer)
{
	lbaint_t _blk, _cnt;
	if (0 == size)
		return 0;

	_blk = offset / mmc->read_bl_len;
	_cnt = size / mmc->read_bl_len;
	_cnt = blk_dwrite(mmc_get_blk_desc(mmc), _blk, _cnt, buffer);

	return (ulong)(_cnt * mmc->read_bl_len);
}
#endif

int fill_ept_by_gpt(struct mmc *mmc, struct _iptbl *p_iptbl_ept)
{
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);
	gpt_entry *gpt_pte = NULL;
	int i, k;
	size_t efiname_len, dosname_len;
	struct _iptbl *ept = p_iptbl_ept;
	struct partitions *partitions = ept->partitions;
	uint64_t alternate;

	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);

	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
				gpt_head, &gpt_pte) != 1) {
		alternate = get_gpt_alternate(mmc);
		if (alternate != -1) {
			if (is_gpt_valid(dev_desc, alternate, gpt_head, &gpt_pte) != 1) {
				printf("%s: invalid gpt\n", __func__);
				return 1;
			}
		}
		printf("%s: *** Using Backup GPT ***\n", __func__);
	}

	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
		if (!is_pte_valid(&gpt_pte[i]))
			break;

		partitions[i].offset = le64_to_cpu(gpt_pte[i].starting_lba << 9ULL);
		partitions[i].size = ((le64_to_cpu(gpt_pte[i].ending_lba) + 1) -
			le64_to_cpu(gpt_pte[i].starting_lba)) << 9ULL;
		/* mask flag */
		partitions[i].mask_flags =
			(uint32_t)le64_to_cpu(gpt_pte[i].attributes.fields.type_guid_specific);
		/* partition name */
		efiname_len = sizeof(gpt_pte[i].partition_name)
			/ sizeof(efi_char16_t);
		dosname_len = sizeof(partitions[i].name);

		memset(partitions[i].name, 0, sizeof(partitions[i].name));
		for (k = 0; k < min(dosname_len, efiname_len); k++)
			partitions[i].name[k] = (char)gpt_pte[i].partition_name[k];

		if (strcmp(partitions[i].name, "boot_a") == 0) {
			has_boot_slot = 1;
			printf("set has_boot_slot = 1\n");
		} else if (strcmp(partitions[i].name, "boot") == 0) {
			has_boot_slot = 0;
			printf("set has_boot_slot = 0\n");
		}
		if (strcmp(partitions[i].name, "system_a") == 0)
			has_system_slot = 1;
		else if (strcmp(partitions[i].name, "system") == 0)
			has_system_slot = 0;

		if (strcmp(partitions[i].name, "super") == 0) {
			dynamic_partition = true;
			env_set("partition_mode", "dynamic");
			printf("enable dynamic_partition\n");
		}

		if (strncmp(partitions[i].name, "vendor_boot", 11) == 0) {
			vendor_boot_partition = true;
			env_set("vendor_boot_mode", "true");
			printf("enable vendor_boot\n");
		}
	}
	ept->count = i;
	free(gpt_pte);
	return 0;
}

/*
 * 1. gpt is writed on emmc
 * parse gpt and compose ept and part_table
 *
 */
int get_ept_from_gpt(struct mmc *mmc)
{
	struct partitions *ptbl = p_iptbl_ept->partitions;

	if (!fill_ept_by_gpt(mmc, p_iptbl_ept)) {
		printf("get ept from gpt success\n");
		gpt_partition = true;
		return 0;
	} else if (part_table && part_table[0].offset != 0) {
		memcpy(ptbl, part_table, sizeof(struct partitions) * parts_total_num);
		p_iptbl_ept->count = parts_total_num;
		printf("get ept from part_table success\n");
		gpt_partition = true;
		return 0;
	}

	return 1;
}

static struct partitions * get_ptbl_from_dtb(struct mmc *mmc)
{
	struct partitions * ptbl = NULL;
#ifdef CONFIG_AML_PARTITION
#ifndef DTB_BIND_KERNEL
	unsigned char * buffer = NULL;
	ulong ret, offset;
	struct virtual_partition *vpart = aml_get_virtual_partition_by_name(MMC_DTB_NAME);

	/* try get dtb table from ddr, which may exsit while usb burning */
	if (NULL == get_partitions()) {
		/* if failed, try rsv dtb area then. */
		buffer = malloc(vpart->size * DTB_COPIES);
		if (NULL == buffer) {
			apt_err("Can not alloc enough buffer\n");
			goto _err;
		}
		offset = _get_inherent_offset(MMC_RESERVED_NAME) + vpart->offset;
		ret = _mmc_rsv_read(mmc, offset, (vpart->size * DTB_COPIES), buffer);
		if (ret != (vpart->size * DTB_COPIES)) {
			apt_err("Can not alloc enough buffer\n");
			goto _err1;
		}
		/* parse it */
		if (get_partition_from_dts(buffer)) {
			apt_err("get partition table from dts faild\n");
			goto _err1;
		}
		/* double check part_table(glb) */
		if (NULL == get_partitions()) {
			goto _err1;
		}
		apt_info("get partition table from dts successfully\n");

		free(buffer);
		buffer = NULL;
	}
#endif
#endif
	/* asign partition info to *ptbl */
	ptbl = get_partitions();
	return ptbl;
#ifdef CONFIG_AML_PARTITION
#ifndef DTB_BIND_KERNEL
_err1:
	if (buffer)
		free(buffer);
_err:
	free (ptbl);
	return NULL;
#endif
#endif
}

static struct partitions *is_prio_partition(struct _iptbl *list, struct partitions *part)
{
	int i;
	struct partitions *plist = NULL;

	if (list->count == 0)
		goto _out;

	apt_info("count %d\n", list->count);
	for (i=0; i<list->count; i++) {
		plist = &list->partitions[i];
		apt_info("%d: %s, %s\n", i, part->name, plist->name);
		if (!strcmp(plist->name, part->name)) {
			apt_info("%s is prio in list[%d]\n", part->name, i);
			break;
		}
	}
	if (i == list->count)
		plist = NULL;
_out:
	return plist;
}

/*  calculate offset of each partitions.
	bottom is a flag for considering
 */
static int _calculate_offset(struct mmc *mmc, struct _iptbl *itbl, u32 bottom)
{
	int i;
	struct partitions *part;
	ulong  gap = PARTITION_RESERVED;
	int ret = 0;

	if (itbl->count <= 0)
		return -1;
	part = itbl->partitions;
#if (CONFIG_MPT_DEBUG)
	_dump_part_tbl(part, itbl->count);
#endif

	if (!strcmp(part->name, "bootloader")) {
		part->offset = 0;
		gap = MMC_BOOT_PARTITION_RESERVED;
	}
	for (i=1; i<itbl->count; i++) {
		/**/
		part[i].offset = part[i-1].offset + part[i-1].size + gap;
		/* check capacity overflow ?*/
		if (((part[i].offset + part[i].size) > mmc->capacity) ||
				(part[i].size == -1)) {
			part[i].size = mmc->capacity - part[i].offset;
			/* reserv space @ the bottom */
			if (bottom && (part[i].size > MMC_BOTTOM_RSV_SIZE)) {
				apt_info("reserv %d bytes at bottom\n", MMC_BOTTOM_RSV_SIZE);
				part[i].size -= MMC_BOTTOM_RSV_SIZE;
			}
			break;
		}
		if ((part[i].mask_flags & 0x100) != 0)
			gap = PARTITION_MIN_RESERVED;
		else
			gap = PARTITION_RESERVED;
	}

#if (ADD_LAST_PARTITION)
	i += 1;
	if (i == itbl->count - 1 && bottom == 1) {
		part[i - 1].size -= gap + part[i].size;
		part[i].offset = part[i - 1].offset + part[i - 1].size + gap;
	}
#endif

	if (i < (itbl->count - 1)) {
		apt_err("too large partition table for current emmc, overflow!\n");
		ret = -1;
	}
#if (CONFIG_MPT_DEBUG)
	_dump_part_tbl(part, itbl->count);
#endif
	return ret;
}

static void compose_ept(struct _iptbl *dtb, struct _iptbl *inh,
			struct _iptbl *ept)
{
	int i;
	struct partitions *partition = NULL;
	struct partitions *dst, *src, *prio;

	/* override inh info by dts */
	apt_info("dtb %p, inh %p, ept %p\n", dtb, inh, ept);
	apt_info("ept->partitions %p\n", ept->partitions);
	partition = ept->partitions;
	apt_info("partition %p\n", partition);
	for (i=0; i<MAX_PART_COUNT; i++) {
		apt_info("i %d, ept->count %d\n", i, ept->count);
		dst = &partition[ept->count];
		src = (i < inh->count) ? &inh->partitions[i]:&dtb->partitions[i-inh->count];

		prio = is_prio_partition(ept, src);
		if (prio) {
			/* override prio partition by new */
			apt_info("override %d: %s\n", ept->count, prio->name);
			//*prio = *src;
			dst = prio;
		} else
			ept->count ++;
		*dst = *src;
		if (-1 == src->size) {
			apt_info("break! %s\n", src->name);
			break;
		}
	}

#if (ADD_LAST_PARTITION)
	i += 1;
	dst = &partition[ept->count];
	src = &dtb->partitions[i - inh->count];
	ept->count += 1;
	*dst = *src;
#endif

	return;
}
#ifdef CONFIG_AML_PARTITION
static int _get_version(unsigned char * s)
{
	int version = 0;
	if (!strncmp((char *)s, MMC_MPT_VERSION_2, sizeof(MMC_MPT_VERSION_2)))
		version = 2;
	else if (!strncmp((char *)s, MMC_MPT_VERSION_1, sizeof(MMC_MPT_VERSION_1)))
		version = 1;
	else
		version = -1;

	return version;
}

/*  calc checksum.
	there's a bug on v1 which did not calculate all the partitions.
 */
static int _calc_iptbl_check_v2(struct partitions *part, int count)
{
	int ret = 0, i;
	int size = count * sizeof(struct partitions) >> 2;
	int *buf = (int *)part;

	for (i = 0; i < size; i++)
		ret += buf[i];

	return ret;
}

static int _calc_iptbl_check_v1(struct partitions *part, int count)
{
    int i, j;
	u32 checksum = 0, *p;

	for (i = 0; i < count; i++) {
		p = (u32*)part;
		/*BUG here, do not fix it!!*/
		for (j = sizeof(struct partitions)/sizeof(checksum); j > 0; j--) {
			checksum += *p;
			p++;
	    }
    }

	return checksum;
}

static int _calc_iptbl_check(struct partitions * part, int count, int version)
{
	if (1 == version)
		return _calc_iptbl_check_v1(part, count);
	else if (2 == version)
		return _calc_iptbl_check_v2(part, count);
	else
		return -1;
}

/* ept is malloced out side */
static int _cpy_iptbl(struct _iptbl * dst, struct _iptbl * src)
{
	int ret = 0;
	if (!dst || !src) {
		apt_err("invalid arg %s\n", !dst ? "dst" : "src");
		ret = -1;
		goto _out;
	}
	if (!dst->partitions || !src->partitions) {
		apt_err("invalid arg %s->partitions\n", !dst ? "dst" : "src");
		ret = -2;
		goto _out;
	}

	dst->count = src->count;
	memcpy(dst->partitions, src->partitions, sizeof(struct partitions) * src->count);

_out:
	return ret;
}

/* get ptbl from rsv area from emmc */
static int get_ptbl_rsv(struct mmc *mmc, struct _iptbl *rsv)
{
	struct ptbl_rsv * ptbl_rsv = NULL;
	uchar * buffer = NULL;
	ulong size, offset;
	int checksum, version, ret = 0;
	struct virtual_partition *vpart = aml_get_virtual_partition_by_name(MMC_TABLE_NAME);

	size = (sizeof(struct ptbl_rsv) + 511) / 512 * 512;
	if (vpart->size < size) {
		apt_err("too much partitions\n");
		ret = -1;
		goto _out;
	}
	buffer = malloc(size);
	if (NULL == buffer) {
		apt_err("no enough memory for ptbl rsv\n");
		ret = -2;
		goto _out;
	}
	/* read it from emmc. */
	offset = _get_inherent_offset(MMC_RESERVED_NAME) + vpart->offset;
	if (size != _mmc_rsv_read(mmc, offset, size, buffer)) {
		apt_err("read ptbl from rsv failed\n");
		ret = -3;
		goto _out;
	}

	ptbl_rsv = (struct ptbl_rsv *) buffer;
	apt_info("magic %3.3s, version %8.8s, checksum %x\n", ptbl_rsv->magic,
			ptbl_rsv->version, ptbl_rsv->checksum);
	/* fixme, check magic ?*/
	if (strncmp(ptbl_rsv->magic, MMC_PARTITIONS_MAGIC, sizeof(MMC_PARTITIONS_MAGIC))) {
		apt_err("magic faild %s, %3.3s\n", MMC_PARTITIONS_MAGIC, ptbl_rsv->magic);
		ret = -4;
		goto _out;
	}
	/* check version*/
	version = _get_version(ptbl_rsv->version);
	if (version < 0) {
		apt_err("version faild %s, %3.3s\n", MMC_PARTITIONS_MAGIC, ptbl_rsv->magic);
		ret = -5;
		goto _out;
	}
	if (ptbl_rsv->count > MAX_MMC_PART_NUM) {
		apt_err("invalid partition count %d\n", ptbl_rsv->count);
		ret = -1;
		goto _out;
	}
	/* check sum */
	checksum = _calc_iptbl_check(ptbl_rsv->partitions, ptbl_rsv->count, version);
	if (checksum != ptbl_rsv->checksum) {
		apt_err("checksum faild 0x%x, 0x%x\n", ptbl_rsv->checksum, checksum);
		ret = -6;
		goto _out;
	}

	rsv->count = ptbl_rsv->count;
	memcpy(rsv->partitions, ptbl_rsv->partitions, rsv->count * sizeof(struct partitions));

_out:
	if (buffer)
		free (buffer);
	return ret;
}

/* update partition tables from src
	if success, return 0;
	else, return 1
	*/
static int update_ptbl_rsv(struct mmc *mmc, struct _iptbl *src)
{
	struct ptbl_rsv *ptbl_rsv = NULL;
	uchar *buffer;
	ulong size, offset;
	int ret = 0, version;
	struct virtual_partition *vpart = aml_get_virtual_partition_by_name(MMC_TABLE_NAME);

	size = (sizeof(struct ptbl_rsv) + 511) / 512 * 512;
	buffer = malloc(size);
	if (NULL == buffer) {
		apt_err("no enough memory for ptbl rsv\n");
		return -1;
	}
	memset(buffer, 0 , size);
	/* version, magic and checksum */
	ptbl_rsv = (struct ptbl_rsv *) buffer;
	strcpy((char *)ptbl_rsv->version, MMC_MPT_VERSION);
	strcpy(ptbl_rsv->magic, MMC_PARTITIONS_MAGIC);
	if (src->count > MAX_MMC_PART_NUM) {
		apt_err("too much partitions\n");
		ret = -1;
		goto _err;
	}
	ptbl_rsv->count = src->count;
	memcpy(ptbl_rsv->partitions, src->partitions,
		sizeof(struct partitions)*src->count);
	version = _get_version(ptbl_rsv->version);
	ptbl_rsv->checksum = _calc_iptbl_check(src->partitions, src->count, version);
	/* write it to emmc. */
	apt_info("magic %3.3s, version %8.8s, checksum %x\n", ptbl_rsv->magic, ptbl_rsv->version, ptbl_rsv->checksum);
	offset = _get_inherent_offset(MMC_RESERVED_NAME) + vpart->offset;
	if (_mmc_rsv_write(mmc, offset, size, buffer) != size) {
		apt_err("write ptbl to rsv failed\n");
		ret = -1;
		goto _err;
	}
_err:
	free (buffer);
	return ret;
}

static void _free_iptbl(struct _iptbl *iptbl)
{
	if (iptbl && iptbl->partitions) {
		free(iptbl->partitions);
		iptbl->partitions = NULL;
	}
	if (iptbl) {
		free(iptbl);
		iptbl = NULL;
	}

	return;
}

#endif
static int _cmp_partition(struct partitions *dst, struct partitions *src, int override)
{
	int ret = 0;
#if (CONFIG_CMP_PARTNAME)
	if (strncmp(dst->name, src->name, sizeof(src->name)))
		ret = -2;
#endif
	if (dst->size != src->size)
		ret = -3;
	if (dst->offset != src->offset)
		ret = -4;
#if (CONFIG_CMP_PARTMASK)
	if (dst->mask_flags != src->mask_flags)
		ret = -5;
#endif

	if (ret && (!override)) {
		apt_err("name: %10.10s<->%10.10s\n", dst->name, src->name);
		apt_err("size: %llx<->%llx\n", dst->size, src->size);
		apt_err("offset: %llx<->%llx\n", dst->offset, src->offset);
		apt_err("mask: %08x<->%08x\n", dst->mask_flags, src->mask_flags);
	}

	if (override) {
		*dst = *src;
		ret = 0;
	}

	return ret;
}

/* compare partition tables
	if same, do nothing then return 0;
	else, print the diff ones and return -x
	-1:count
	-2:name
	-3:size
	-4:offset
	*/
static int _cmp_iptbl(struct _iptbl * dst, struct _iptbl * src)
{
	int ret = 0, i = 0;
	struct partitions *dstp;
	struct partitions *srcp;

	if (dst->count != src->count) {
		apt_err("partition count is not same %d:%d\n", dst->count, src->count);
		ret = -1;
		goto _out;
	}

	while (i < dst->count) {
		dstp = &dst->partitions[i];
		srcp = &src->partitions[i];
		ret = _cmp_partition(dstp, srcp, 0);
		if (ret) {
			env_set("part_changed", "1");
			apt_err("partition %d has changed\n", i);
			break;
		}
		i++;
	}

_out:
	return ret;
}

/* iptbl buffer opt. */
static int _zalloc_iptbl(struct _iptbl **_iptbl)
{
	int ret = 0;
	struct _iptbl *iptbl;
	struct partitions *partition = NULL;

	partition = malloc(sizeof(struct partitions)*MAX_PART_COUNT);
	if (NULL == partition) {
		ret = -1;
		apt_err("no enough memory for partitions\n");
		goto _out;
	}

	iptbl = malloc(sizeof(struct _iptbl));
	if (NULL == iptbl) {
		ret = -2;
		apt_err("no enough memory for ept\n");
		free(partition);
		goto _out;
	}
	memset(partition, 0, sizeof(struct partitions)*MAX_PART_COUNT);
	memset(iptbl, 0, sizeof(struct _iptbl));

	iptbl->partitions = partition;
	apt_info("iptbl %p, partition %p, iptbl->partitions %p\n",
		iptbl, partition, iptbl->partitions);
	*_iptbl = iptbl;
_out:
	return ret;
}


/*
 * fixme, need check space size later.
 */

static inline int le32_to_int(unsigned char *le32)
{
	return ((le32[3] << 24) +
	    (le32[2] << 16) +
	    (le32[1] << 8) +
	     le32[0]
	   );
}

static int test_block_type(unsigned char *buffer)
{
	int slot;
	struct dos_partition *p;

	if ((buffer[DOS_PART_MAGIC_OFFSET + 0] != 0x55) ||
	    (buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) ) {
		return (-1);
	} /* no DOS Signature at all */
	p = (struct dos_partition *)&buffer[DOS_PART_TBL_OFFSET];
	for (slot = 0; slot < 3; slot++) {
		if (p->boot_ind != 0 && p->boot_ind != 0x80) {
			if (!slot &&
			    (strncmp((char *)&buffer[DOS_PBR_FSTYPE_OFFSET],
				     "FAT", 3) == 0 ||
			     strncmp((char *)&buffer[DOS_PBR32_FSTYPE_OFFSET],
				     "FAT32", 5) == 0)) {
				return DOS_PBR; /* is PBR */
			} else {
				return -1;
			}
		}
	}
	return DOS_MBR;	    /* Is MBR */
}

//DOS_MBR OR DOS_PBR
/*
 * re-constructed iptbl from mbr&ebr infos.
 * memory for  iptbl_mbr must be alloced outside.
 *
 */
static void _construct_ptbl_by_mbr(struct mmc *mmc, struct _iptbl *iptbl_mbr)
{
	int ret,i;
	int flag = 0;
	lbaint_t read_offset = 0;
	int part_num = 0;
	int primary_num = 0;
	uint64_t logic_start = 0;
	uint64_t extended_start = 0;
	struct dos_partition *pt;
	struct partitions *partitions = iptbl_mbr->partitions;

	apt_info("aml MBR&EBR debug...\n");
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, 512);
	for (;;) {
		apt_info("**%02d: read_offset %016llx\n", part_num, (uint64_t)read_offset<<9);
		ret = blk_dread(mmc_get_blk_desc(mmc), read_offset, 1, buffer);
		if (read_offset == 0)
			flag = 1;
		else
			flag = 0;
		/* debug code */
		// print_buffer(0,buffer,1,512,16);
		if (ret != 1) {
			apt_err("ret %d fail to read current ebr&mbr from emmc! \n", ret);
			break;
		}
		ret = test_block_type(buffer);
		if (ret != 0 && ret != 1) {
			apt_err("invalid magic value: 0x%02x%02x\n",
				buffer[DOS_PART_MAGIC_OFFSET], buffer[DOS_PART_MAGIC_OFFSET + 1]);
			break;
		}

		pt = (dos_partition_t *)(&buffer[0] + DOS_PART_TBL_OFFSET);
		for (i = 0; i < 4; i++, pt++) {
			if ( (pt->boot_ind == 0x00 || pt->boot_ind == 0x80) && pt->sys_ind == 0x83 ) {
				//emmc_partition[part_num]->name = NULL;
				partitions[part_num].offset = ((uint64_t)(le32_to_int(pt->start4)+read_offset) << 9ULL);
				partitions[part_num].size = (uint64_t)le32_to_int(pt->size4) << 9ULL;
				partitions[part_num].mask_flags = pt->sys_ind;

				apt_info("--partition[%d]: %016llx, %016llx, 0x%08x \n",
					part_num, partitions[part_num].offset,
					partitions[part_num].size,
					le32_to_int(pt->size4));
				part_num++;
				if ( flag )
					primary_num++;
			}else{/* get the next  extended partition info */
				if ( pt->boot_ind == 0x00 && pt->sys_ind == 0x05) {
					logic_start = (uint64_t)le32_to_int (pt->start4);
					//logic_size = (uint64_t)le32_to_int (pt->size4);
				}
			}
		}
		/* mbr & ebr debug infos */
		apt_info("******%02d: read_offset=%016llx, logic_start=%016llx\n",
			part_num,(uint64_t)read_offset*512ULL,logic_start*512ULL);

		if (part_num == primary_num) {
			extended_start = logic_start;
			read_offset = extended_start;
		}else
			read_offset = extended_start + logic_start;
		if (logic_start == 0)
			break;
		logic_start = 0;

	}
	iptbl_mbr->count = part_num;
	apt_info("iptbl_mbr->count = %d\n", iptbl_mbr->count);

	return;
}

static int __attribute__((unused)) _check_ptbl_mbr(struct mmc *mmc, struct _iptbl *ept)
{
	int ret = 0;
	/* re-constructed by mbr */
	struct _iptbl *iptbl_mbr = NULL;
	struct partitions *partitions = NULL;

	iptbl_mbr = malloc(sizeof(struct _iptbl));
	if (NULL == iptbl_mbr) {
		apt_err("no enough memory for iptbl_mbr\n");
		return -1;
	}
	memset(iptbl_mbr , 0, sizeof(struct _iptbl));
	partitions = (struct partitions *)malloc(sizeof(struct partitions) * DOS_PARTITION_COUNT);
	if (NULL == partitions) {
		apt_err("no enough memory for partitions\n");
		free(iptbl_mbr);
		return -1;
	}
	memset(partitions, 0, sizeof(struct partitions) * DOS_PARTITION_COUNT);
	iptbl_mbr->partitions = partitions;

	_construct_ptbl_by_mbr(mmc, iptbl_mbr);

	ret = _cmp_iptbl(iptbl_mbr, ept);

	if (partitions)
		free(partitions);
	if (iptbl_mbr)
		free(iptbl_mbr);

	apt_wrn("MBR is %s\n", ret?"Improper!":"OK!");
	return ret;
}

/* construct a partition table entry of EBR */
static int _construct_ebr_1st_entry(struct _iptbl *p_iptbl,struct dos_partition *p_ebr, int part_num )
{
	uint64_t start_offset = 0;
	uint64_t logic_size = 0;

	p_ebr->boot_ind = 0x00;
	p_ebr->sys_ind = 0x83;
	/* Starting = relative offset between this EBR sector and the first sector of the logical partition
	* the gap between two partition is a fixed value of PARTITION_RESERVED ,otherwise the emmc partiton
	* is different with reserved */
	start_offset = PARTITION_RESERVED >> 9;
	/* Number of sectors = total count of sectors for this logical partition */
	// logic_size = (p_iptbl->partitions[part_num].size) >> 9ULL;
	logic_size = lldiv(p_iptbl->partitions[part_num].size, 512);
	apt_info("*** %02d: size 0x%016llx, logic_size 0x%016llx\n", part_num, p_iptbl->partitions[part_num].size, logic_size);
	memcpy((unsigned char *)(p_ebr->start4), &start_offset, 4);
	memcpy((unsigned char *)(p_ebr->size4), &logic_size, 4);
	return 0;
}

static int _construct_ebr_2nd_entry(struct _iptbl *p_iptbl, struct dos_partition *p_ebr, int part_num)
{
	uint64_t start_offset = 0;
	uint64_t logic_size = 0;

	if ((part_num+2) > p_iptbl->count)
		return 0;

	p_ebr->boot_ind = 0x00;
	p_ebr->sys_ind = 0x05;
	/* Starting sector = LBA address of next EBR minus LBA address of extended partition's first EBR */
	start_offset = (p_iptbl->partitions[part_num+1].offset - PARTITION_RESERVED -
					(p_iptbl->partitions[3].offset - PARTITION_RESERVED)) >> 9;
	/* total count of sectors for next logical partition, but count starts from the next EBR sector */
	logic_size = (p_iptbl->partitions[part_num+1].size + PARTITION_RESERVED) >> 9;

	memcpy((unsigned char *)(p_ebr->start4), &start_offset, 4);
	memcpy((unsigned char *)(p_ebr->size4), &logic_size, 4);

	return 0;
}

/* construct a partition table entry of MBR OR EBR */
static int _construct_mbr_entry(struct _iptbl *p_iptbl, struct dos_partition *p_entry, int part_num)
{
	uint64_t start_offset = 0;
	uint64_t primary_size = 0;
	uint64_t extended_size = 0;
	int i;
	/* the entry is active or not */
	p_entry->boot_ind = 0x00;

	if (part_num == 3) {/* the logic partion entry */
		/* the entry type */
		p_entry->sys_ind = 0x05;
		start_offset = (p_iptbl->partitions[3].offset - PARTITION_RESERVED) >> 9;
		for ( i = 3;i< p_iptbl->count;i++)
			extended_size = p_iptbl->partitions[i].size >> 9;

		memcpy((unsigned char *)p_entry->start4, &start_offset, 4);
		memcpy((unsigned char *)p_entry->size4, &extended_size, 4);
	}else{/* the primary partion entry */
		/* the entry type */
		p_entry->sys_ind = 0x83;
		start_offset = (p_iptbl->partitions[part_num].offset) >> 9;
		primary_size = (p_iptbl->partitions[part_num].size)>>9;
		memcpy((unsigned char *)p_entry->start4, &start_offset, 4);
		memcpy((unsigned char *)p_entry->size4, &primary_size, 4);
	}

	return 0;
}

static int _construct_mbr_or_ebr(struct _iptbl *p_iptbl, struct dos_mbr_or_ebr *p_br,
	int part_num, int type)
{
	int i;

	if (DOS_MBR == type) {
		/* construct a integral MBR */
		for (i = 0; i<4 ; i++)
			_construct_mbr_entry(p_iptbl, &p_br->part_entry[i], i);

	}else{
		/* construct a integral EBR */
		p_br->bootstart[DOS_PBR32_FSTYPE_OFFSET] = 'F';
		p_br->bootstart[DOS_PBR32_FSTYPE_OFFSET + 1] = 'A';
		p_br->bootstart[DOS_PBR32_FSTYPE_OFFSET + 2] = 'T';
		p_br->bootstart[DOS_PBR32_FSTYPE_OFFSET + 3] = '3';
		p_br->bootstart[DOS_PBR32_FSTYPE_OFFSET + 4] = '2';

		_construct_ebr_1st_entry(p_iptbl, &p_br->part_entry[0], part_num);
		_construct_ebr_2nd_entry(p_iptbl, &p_br->part_entry[1], part_num);
	}

	p_br->magic[0] = 0x55 ;
	p_br->magic[1] = 0xAA ;
	return 0;
}

static __attribute__((unused)) int _update_ptbl_mbr(struct mmc *mmc, struct _iptbl *p_iptbl)
{
	int ret = 0, start_blk = 0, blk_cnt = 1;
	unsigned char *src;
	int i;
	struct dos_mbr_or_ebr *mbr;
	struct _iptbl *ptb ;

	ptb = p_iptbl;
	mbr = malloc(sizeof(struct dos_mbr_or_ebr));

	for (i=0;i<ptb->count;i++) {
		apt_info("-update MBR-: partition[%02d]: %016llx - %016llx\n",i,
			ptb->partitions[i].offset, ptb->partitions[i].size);
	}

	for (i = 0;i < ptb->count;) {
		memset(mbr ,0 ,sizeof(struct dos_mbr_or_ebr));
		if (i == 0) {
			_construct_mbr_or_ebr(ptb, mbr, i, 0);
			i = i+2;
		} else
			_construct_mbr_or_ebr(ptb, mbr, i, 2);
		src = (unsigned char *)mbr;
		apt_info("--%s(): %02d(%02d), off %x\n", __func__, i, ptb->count, start_blk);
		ret = blk_dwrite(mmc_get_blk_desc(mmc), start_blk, blk_cnt, src);
		i++;
		if (ret != blk_cnt) {
			apt_err("write current MBR failed! ret: %d != cnt: %d\n",ret,blk_cnt);
			break;
		}
		start_blk = (ptb->partitions[i].offset - PARTITION_RESERVED) >> 9;
	}
	free(mbr);

	ret = !ret;
	if (ret)
		apt_err("write MBR failed!\n");

	return ret;
}

int check_gpt_change(struct blk_desc *dev_desc, void *buf)
{
	int i, k;
	int m = 0;
	gpt_header *gpt_h;
	gpt_entry *gpt_e;
	u32 calc_crc32;
	u32 entries_num;
	size_t efiname_len;
	int ret = 0;
	bool alternate_flag = false;
	int j = 0;
	int recovery_offset_old = 0, recovery_offset_new = 0;
	int tee_offset_old = 0, tee_offset_new = 0;
	int oem_offset_old = 0, oem_offset_new = 0;
	int data_offset_old = 0, data_offset_new = 0;
	int data_size_old = 0, data_size_new = 0;
	int cache_offset_old = 0, cache_offset_new = 0;
	int cache_size_old = 0, cache_size_new = 0;
	int tee_size_old = 0, tee_size_new = 0;
	int oem_size_old = 0, oem_size_new = 0;
	int metadata_offset_old = 0, metadata_offset_new = 0;
	int metadata_size_old = 0, metadata_size_new = 0;

	struct partitions *partitions = p_iptbl_ept->partitions;
	int parts_num = p_iptbl_ept->count;
	uint64_t offset;
	uint64_t size;
	uint32_t mask_flags;
	char name[PARTNAME_SZ];
	char *update_dts_gpt = NULL;
#if (ADD_LAST_PARTITION)
	ulong gap = GPT_GAP;
#endif

	update_dts_gpt = env_get("update_dts_gpt");

	if (is_valid_gpt_buf(dev_desc, buf))
		return -1;

	gpt_h = buf + (GPT_PRIMARY_PARTITION_TABLE_LBA *
			dev_desc->blksz);

	/* determine start of GPT Entries in the buffer */
	gpt_e = buf + (le64_to_cpu(gpt_h->partition_entry_lba) *
			dev_desc->blksz);
	entries_num = le32_to_cpu(gpt_h->num_partition_entries);

	if (le64_to_cpu(gpt_h->alternate_lba) > dev_desc->lba ||
		le64_to_cpu(gpt_h->alternate_lba) == 0) {
		printf("GPT: alternate_lba: %llX, " LBAF ", reset it\n",
		       le64_to_cpu(gpt_h->alternate_lba), dev_desc->lba);
		gpt_h->alternate_lba = cpu_to_le64(dev_desc->lba - 1);
		alternate_flag = true;
	}

	if (le64_to_cpu(gpt_h->last_usable_lba) > dev_desc->lba) {
		printf("GPT: last_usable_lba incorrect: %llX > " LBAF ", reset it\n",
		       le64_to_cpu(gpt_h->last_usable_lba), dev_desc->lba);
		if (alternate_flag)
			gpt_h->last_usable_lba = cpu_to_le64(dev_desc->lba - 34);
		else
			gpt_h->last_usable_lba = cpu_to_le64(dev_desc->lba - 1);
	}

	for (i = 0; i < entries_num; i++) {
#if (ADD_LAST_PARTITION)
		if (i == entries_num - 1) {
			gpt_e[i - 1].ending_lba -= gpt_e[i].ending_lba + le64_to_cpu(gap) + 1;
			gpt_e[i].starting_lba = gpt_e[i - 1].ending_lba + le64_to_cpu(gap) + 1;
			gpt_e[i].ending_lba = gpt_h->last_usable_lba;
		}

#endif
		if (le64_to_cpu(gpt_e[i].starting_lba) > gpt_h->last_usable_lba) {
			printf("gpt_e[%d].starting_lba: %llX > %llX, writing failed\n", i,
			       le64_to_cpu(gpt_e[i].starting_lba),
			       le64_to_cpu(gpt_h->last_usable_lba));
			return 1;
		}
		if (le64_to_cpu(gpt_e[i].ending_lba) > gpt_h->last_usable_lba) {
			printf("gpt_e[%d].ending_lba: %llX > %llX, reset it\n",
			i, le64_to_cpu(gpt_e[i].ending_lba), le64_to_cpu(gpt_h->last_usable_lba));
			if (alternate_flag)
				gpt_e[i].ending_lba = ((gpt_h->last_usable_lba >> 12) << 12) - 1;
			else
				gpt_e[i].ending_lba = gpt_h->last_usable_lba;
			printf("gpt_e[%d].ending_lba: %llX\n", i, gpt_e[i].ending_lba);
		}
	}

	calc_crc32 = crc32(0, (const unsigned char *)gpt_e,
			entries_num * le32_to_cpu(gpt_h->sizeof_partition_entry));
	gpt_h->partition_entry_array_crc32 = calc_crc32;
	gpt_h->header_crc32 = 0;
	calc_crc32 = crc32(0, (const unsigned char *)gpt_h,
	le32_to_cpu(gpt_h->header_size));
	gpt_h->header_crc32 = calc_crc32;

	if (update_dts_gpt) {
		printf("update_dts_gpt is %s\n", update_dts_gpt);
		m = 1;
		ret = 2;
	}

	if (parts_num != entries_num) {
		printf("parts_num changes\n");
		ret = 2;
	}

	for (j = m; j < parts_num; j++) {
		if (partitions[j].size != 0 &&
				(strcmp(partitions[j].name, "rsv") != 0)) {
			if (!strcmp(partitions[j].name, "recovery")) {
				recovery_offset_old = partitions[j].offset;
				//printf("recovery_offset_old = %d\n", recovery_offset_old);
			} else if (!strcmp(partitions[j].name, "cache")) {
				cache_offset_old = partitions[j].offset;
				cache_size_old = partitions[j].size;
				//printf("cache_offset_old = %d\n", cache_offset_old);
				//printf("cache_size_old = %d\n", cache_size_old);
			} else if (!strcmp(partitions[j].name, "userdata") ||
				!strcmp(partitions[j].name, "data")) {
				data_offset_old = partitions[j].offset;
				data_size_old = partitions[j].size;
				//printf("data_offset_old = %d\n", data_offset_old);
				//printf("data_size_old = %d\n", data_size_old);
			} else if (!strcmp(partitions[j].name, "tee")) {
				tee_offset_old = partitions[j].offset;
				tee_size_old = partitions[j].size;
				//printf("tee_offset_old = %d\n", tee_offset_old);
				//printf("tee_size_old = %d\n", tee_size_old);
			} else if (!strcmp(partitions[j].name, "metadata")) {
				metadata_offset_old = partitions[j].offset;
				metadata_size_old = partitions[j].size;
				//printf("metadata_offset_old = %d\n", metadata_offset_old);
				//printf("metadata_size_old = %d\n", metadata_size_old);
			} else if (!strcmp(partitions[j].name, "oem")) {
				oem_offset_old = partitions[j].offset;
				oem_size_old = partitions[j].size;
				//printf("oem_offset_old = %d\n", tee_offset_old);
				//printf("oem_size_old = %d\n", tee_size_old);
			}
		}
	}

	for (i = 0; i < entries_num; i++) {
		/* partition name */
		efiname_len = sizeof(gpt_e[i].partition_name)
			/ sizeof(efi_char16_t);

		memset(name, 0, PARTNAME_SZ);
		for (k = 0; k < efiname_len; k++)
			name[k] = (char)gpt_e[i].partition_name[k];

		if (strcmp(name, partitions[i].name) != 0) {
			printf("Caution! GPT name [%s] had been changed\n", name);
			ret = 2;
		}

		offset = le64_to_cpu(gpt_e[i].starting_lba << 9ULL);
		size = ((le64_to_cpu(gpt_e[i].ending_lba) + 1) -
				le64_to_cpu(gpt_e[i].starting_lba)) << 9ULL;

		mask_flags =
			(uint32_t)le64_to_cpu(gpt_e[i].attributes.fields.type_guid_specific);

		if (!strcmp(name, "recovery")) {
			recovery_offset_new = offset;
			//printf("recovery_offset_new = %d\n", recovery_offset_new);
		} else if (!strcmp(name, "cache")) {
			cache_offset_new = offset;
			cache_size_new = size;
			///printf("cache_offset_new = %d\n", cache_offset_new);
			//printf("cache_size_new = %d\n", cache_size_new);
		} else if (!strcmp(name, "userdata") ||
			!strcmp(name, "data")) {
			data_offset_new = offset;
			data_size_new = size;
			//printf("data_offset_new = %d\n", data_offset_new);
			//printf("data_size_new = %d\n", data_size_new);
		} else if (!strcmp(name, "tee")) {
			tee_offset_new = offset;
			tee_size_new = size;
			//printf("tee_offset_new = %d\n", tee_offset_new);
			//printf("tee_size_new = %d\n", tee_size_new);
		} else if (!strcmp(name, "metadata")) {
			metadata_offset_new = offset;
			metadata_size_new = size;
			//printf("metadata_offset_new = %d\n", metadata_offset_new);
			//printf("metadata_size_new = %d\n", metadata_size_new);
		} else if (!strcmp(name, "oem")) {
			oem_offset_new = offset;
			oem_size_new = size;
			//printf("oem_offset_new = %d\n", oem_offset_new);
			//printf("oem_size_new = %d\n", oem_size_new);
		}

		for (j = m; j < parts_num; j++) {
			if ((strcmp(partitions[j].name, name) == 0) &&
					(strcmp(partitions[j].name, "rsv") != 0)) {
				if (partitions[j].offset != offset ||
						partitions[j].size != size) {
					printf("%s offset/size had been changed\n",
							name);
					printf("offset: %016llx --> %016llx\n",
							partitions[j].offset,
							offset);
					printf("size: %016llx --> %016llx\n",
							partitions[j].size, size);
					ret = 3;
				}
				if (partitions[j].mask_flags != mask_flags) {
					printf("%s mask_flags had been changed\n",
						name);
					printf("%08x<->%08x\n", partitions[j].mask_flags,
						mask_flags);
					if (ret == 0)
						ret = 2;
				}
			}
		}
	}

	if (data_offset_old != data_offset_new ||
		data_size_old != data_size_new ||
		cache_offset_old != cache_offset_new ||
		cache_size_old != cache_size_new ||
		tee_offset_old != tee_offset_new ||
		tee_size_old != tee_size_new ||
		metadata_offset_old != metadata_offset_new ||
		metadata_size_old != metadata_size_new ||
		oem_offset_old != oem_offset_new ||
		oem_size_old != oem_size_new ||
		recovery_offset_old != recovery_offset_new) {
		printf("null ab critical partition change\n");
		ret = 4;
	}

	return ret;
}

int is_gpt_changed(struct mmc *mmc, struct _iptbl *p_iptbl_ept)
{
	int i, k;
	gpt_entry *gpt_pte = NULL;
	size_t efiname_len;
	struct _iptbl *ept = p_iptbl_ept;
	struct partitions *partitions = ept->partitions;
	int parts_num = ept->count;
	uint64_t offset;
	uint64_t size;
	char name[PARTNAME_SZ];
	int gpt_changed = 0;
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);

	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);

	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
				gpt_head, &gpt_pte) != 1) {
		printf("%s: ***ERROR:Invalid GPT ***\n", __func__);
		if (is_gpt_valid(dev_desc, (dev_desc->lba - 1),
					gpt_head, &gpt_pte) != 1) {
			printf("%s: ***ERROR: Invalid Backup GPT ***\n",
					__func__);
			return 1;
		} else {
			printf("%s: *** Using Backup GPT ***\n",
					__func__);
		}
	}
	for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
		if (!is_pte_valid(&gpt_pte[i]))
			break;

		offset = le64_to_cpu(gpt_pte[i].starting_lba<<9ULL);
		if (partitions[i].offset != offset) {
			printf("Caution! GPT offset had been changed\n");
			gpt_changed = 1;
			break;
		}

		size = ((le64_to_cpu(gpt_pte[i].ending_lba)+1) -
			le64_to_cpu(gpt_pte[i].starting_lba)) << 9ULL;
		if (i == parts_num - 1) {
			if ((partitions[i].size - GPT_TOTAL_SIZE) != size) {
				printf("Caution! GPT size had been changed\n");
				gpt_changed = 1;
				break;
			}
		} else {
		if (partitions[i].size != size) {
			printf("Caution! GPT size had been changed\n");
			gpt_changed = 1;
			break;
			}
		}

		/* partition name */
		efiname_len = sizeof(gpt_pte[i].partition_name)
			/ sizeof(efi_char16_t);

		memset(name, 0, PARTNAME_SZ);
		for (k = 0; k < efiname_len; k++)
			name[k] = (char)gpt_pte[i].partition_name[k];
		if (strcmp(name, partitions[i].name) != 0) {
			printf("Caution! GPT name had been changed\n");
			gpt_changed = 1;
			break;
		}

	}
	if ((i != parts_num) && (gpt_changed == 0)) {
		gpt_changed = 1;
		printf("Caution! GPT number had been changed\n");
	}

	free(gpt_pte);
	return gpt_changed;
}

int is_gpt_broken(struct mmc *mmc)
{
	gpt_entry *gpt_pte = NULL;
	int broken_status = 0;
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);

	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);

	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
				gpt_head, &gpt_pte) != 1) {
		broken_status += 1;
		printf("%s: ***ERROR:Invalid GPT ***\n", __func__);
	}
	if (is_gpt_valid(dev_desc, (dev_desc->lba - 1),
				gpt_head, &gpt_pte) != 1) {
		printf("%s: ***ERROR: Invalid Backup GPT ***\n",
					__func__);
		broken_status += 2;
	}

	if (broken_status != 3)
		free(gpt_pte);
	return broken_status;

}

/*
 * check is gpt is valid
 * if valid return 0
 * else return 1
 */
int aml_gpt_valid(struct mmc *mmc) {
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);
	if (!dev_desc) {
		printf("%s: Invalid Argument(s)\n", __func__);
		return 1;
	} else {
		ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);
		gpt_entry *gpt_pte = NULL;
		if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
				gpt_head, &gpt_pte) != 1) {
			if (is_gpt_valid(dev_desc, (dev_desc->lba - 1),
					gpt_head, &gpt_pte) != 1) {
				printf("gpt is invalid\n");
				return 1;
			} else {
				printf("%s: *** Using Backup GPT ***\n",
					__func__);
			}
		}
	}

	return 0;
}

void trans_ept_to_diskpart(struct _iptbl *ept, disk_partition_t *disk_part) {
	struct partitions *part = ept->partitions;
	int count = ept->count;
	int i;
	for (i = 0; i < count; i++) {
		disk_part[i].start = part[i].offset >> 9;
		strcpy((char *)disk_part[i].name, part[i].name);
		/* store maskflag into type, 8bits ONLY! */
		disk_part[i].type[0] = (uchar)part[i].mask_flags;
#ifdef CONFIG_PARTITION_TYPE_GUID
		strcpy((char *)disk_part[i].type_guid, part[i].name);
#endif
#ifdef CONFIG_RANDOM_UUID
		gen_rand_uuid_str(disk_part[i].uuid, UUID_STR_FORMAT_STD);
#endif
		disk_part[i].bootable = 0;
		if ( i == (count - 1))
			disk_part[i].size = 0;
		else
			disk_part[i].size = (part[i].size) >> 9;
	}
	return;
}

#ifdef CONFIG_AML_PARTITION
/*
 * compare ept and rsv
 *
 * if different:
 *   update rsv write back on emmc
 *
 */
int enable_rsv_part_table(struct mmc *mmc)
{
	struct _iptbl *p_iptbl_rsv = NULL;
	int ret = -1;

	/* try to get partition table from rsv */
	ret = _zalloc_iptbl(&p_iptbl_rsv);
	if (ret)
		return ret;
	if (!get_ptbl_rsv(mmc, p_iptbl_rsv)) {
		if (_cmp_iptbl(p_iptbl_ept, p_iptbl_rsv)) {
			apt_wrn("update rsv with gpt!\n");
			ret = update_ptbl_rsv(mmc, p_iptbl_ept);
			if (ret)
				printf("update rsv with gpt failed\n");
		}
	} else {
		printf("rsv not exist\n");
		ret = update_ptbl_rsv(mmc, p_iptbl_ept);
		if (ret)
			printf("update rsv with gpt failed\n");
	}

	_free_iptbl(p_iptbl_rsv);
	return ret;
}
#endif

void __attribute__((unused)) _update_part_tbl(struct partitions *p, int count)
{
	int i = 0;

	while (i < count) {
		if (strcmp(p[i].name, "boot_a") == 0)
			has_boot_slot = 1;
		else if (strcmp(p[i].name, "boot") == 0)
			has_boot_slot = 0;

		if (strcmp(p[i].name, "system_a") == 0)
			has_system_slot = 1;
		else if (strcmp(p[i].name, "system") == 0)
			has_system_slot = 0;

		if (strcmp(p[i].name, "super") == 0) {
			dynamic_partition = true;
			env_set("partition_mode", "dynamic");
		}

		if (strncmp(p[i].name, "vendor_boot", 11) == 0) {
			vendor_boot_partition = true;
			env_set("vendor_boot_mode", "true");
		}
		i++;
	}
}

lbaint_t get_gpt_alternate(struct mmc *mmc)
{
	gpt_header *gpt_h;
	void *buf;
	int ret;
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);
	lbaint_t alternate;

	buf = malloc(GPT_SIZE);
	if (!buf) {
		printf("not enough space for gpt buffer\n");
		return -1;
	}

	ret = mmc_gpt_read(buf);
	if (ret == 0) {
		/* determine start of GPT Header in the buffer */
		gpt_h = buf + (GPT_PRIMARY_PARTITION_TABLE_LBA *
				dev_desc->blksz);
		alternate = le64_to_cpu(gpt_h->alternate_lba);
		free(buf);
		return alternate;
	} else if (ret == -1) {
		printf("%s: read gpt failed\n", __func__);
		free(buf);
		return -1;
	}
	free(buf);
	return 0;
}

static uint64_t calc_alternate_check(struct gpt_alternate *gpt_alt)
{
	int i;
	uint64_t checksum = 0;
	int size = sizeof(struct gpt_alternate) - sizeof(uint64_t);
	char *buf = (char *)gpt_alt;

	for (i = 0; i < size; i++)
		checksum += buf[i];

	return checksum;
}

int is_gpt_alter_valid(lbaint_t *alternate_lba)
{
	struct gpt_alternate *gpt_alt;
	int ret;
	char *name = NULL;
	loff_t offset = (RESERVED_GPT_OFFSET + MMC_GPT_ALT_OFFSET) >> 9;
	size_t size = MMC_GPT_ALT_SIZE >> 9;

	gpt_alt = malloc(MMC_GPT_ALT_SIZE);
	if (!gpt_alt)
		return -ENOMEM;

	ret = mmc_storage_read(name, offset, size, (void *)gpt_alt);
	if (ret) {
		free(gpt_alt);
		return -1;
	}

	if (strcmp(gpt_alt->magic, "GPT")) {
		printf("gpt_alt->magic %s\n", gpt_alt->magic);
		free(gpt_alt);
		return -1;
	} else if (calc_alternate_check(gpt_alt) != gpt_alt->checksum) {
		printf("gpt_alt->checksum %llx\n", gpt_alt->checksum);
		free(gpt_alt);
		return -1;
	}

	*alternate_lba = gpt_alt->alternate_lba;
	free(gpt_alt);
	return 1;
}

int write_gpt_alternate(lbaint_t gpt_alternate)
{
	struct gpt_alternate *gpt_alt;
	char *name = NULL;
	int ret;
	loff_t offset = (RESERVED_GPT_OFFSET + MMC_GPT_ALT_OFFSET) >> 9;
	size_t size = MMC_GPT_ALT_SIZE >> 9;

	gpt_alt = malloc(MMC_GPT_ALT_SIZE);
	if (!gpt_alt)
		return -ENOMEM;

	memset(gpt_alt, 0, MMC_GPT_ALT_SIZE);

	strcpy(gpt_alt->magic, "GPT");
	gpt_alt->alternate_lba = gpt_alternate;
	gpt_alt->checksum = calc_alternate_check(gpt_alt);

	ret = mmc_storage_write(name, offset, size, (void *)gpt_alt);

	free(gpt_alt);
	return ret;
}

/*
 * check: mbr, first_gpt, secondary_gpt, and alternate addr
 *
 * if first_gpt is valid but one of other part is broken
 *		prepare to repair others
 * else if alternate addr and secondary_gpt is valid
 *  and first_gpt is broken
 *		prepare to repair first_gpt
 * otherwise
 *		do nothing
 */
int _mmc_check_gpt(struct mmc *mmc, lbaint_t *alternate)
{
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);
	gpt_entry *gpt_pte = NULL;
	lbaint_t alternate_lba;
	u32 entries_num;
	int i, k;
	size_t efiname_len;
	int flag = 0;
	char name[PARTNAME_SZ];

	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);

	/* first_gpt is valid, others is broken, repair */
	if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
			gpt_head, &gpt_pte) == 1) {
		alternate_lba = (lbaint_t)le64_to_cpu(gpt_head->alternate_lba);
		entries_num = le32_to_cpu(gpt_head->num_partition_entries);

		for (i = 0; i < entries_num; i++) {
			/* partition name */
			efiname_len = sizeof(gpt_pte[i].partition_name)
				/ sizeof(efi_char16_t);

			memset(name, 0, PARTNAME_SZ);
			for (k = 0; k < efiname_len; k++)
				name[k] = (char)gpt_pte[i].partition_name[k];

			if (alternate_lba > le64_to_cpu(gpt_pte[i].starting_lba) &&
				alternate_lba < le64_to_cpu(gpt_pte[i].ending_lba)) {
				printf("GPT: alternate_lba: %lX during %s, invalid\n",
			       alternate_lba, name);
				flag = 1;
			}
		}

		if (flag == 1) {
			printf("GPT: alternate_lba: %llX invalid, reset it\n",
			       le64_to_cpu(gpt_head->alternate_lba));
			gpt_head->alternate_lba = gpt_pte[1].starting_lba - 1;
			alternate_lba = (lbaint_t)le64_to_cpu(gpt_head->alternate_lba);
			*alternate = alternate_lba;
			free(gpt_pte);
			return 1;
		}

		*alternate = alternate_lba;
		free(gpt_pte);

		if (is_gpt_valid(dev_desc, alternate_lba, gpt_head, &gpt_pte) != 1)
			return 1;

		free(gpt_pte);

		if (part_test_efi(dev_desc) || alternate_lba != *alternate ||
				(is_gpt_alter_valid(&alternate_lba) != 1))
			return 1;
		else
			return 0;
	/* first_gpt is broken, secondary_gpt is valid, repair */
	} else if ((is_gpt_alter_valid(&alternate_lba) == 1) &&
			(is_gpt_valid(dev_desc, alternate_lba, gpt_head, &gpt_pte)) == 1) {
		*alternate = alternate_lba;
		free(gpt_pte);
		return 1;
	/* legacy mode, secondary_gpt at last lba of mmc */
	} else if (is_gpt_valid(dev_desc, dev_desc->lba - 1, gpt_head, &gpt_pte) == 1) {
		*alternate = dev_desc->lba - 1;
		free(gpt_pte);
		return 1;
	}

	printf("%s: gpt is invalid and can't be repair\n", __func__);

	return 0;
}

int mmc_repair_gpt(struct mmc *mmc, lbaint_t alternate)
{
	gpt_entry *gpt_pte = NULL;
	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);
	int ret;
	u64 pri_lba = GPT_PRIMARY_PARTITION_TABLE_LBA;

	ALLOC_CACHE_ALIGN_BUFFER_PAD(gpt_header, gpt_head, 1, dev_desc->blksz);

	if (is_gpt_valid(dev_desc, pri_lba, gpt_head, &gpt_pte) == 1) {
		gpt_head->alternate_lba = alternate;

		ret = write_gpt_table(dev_desc, gpt_head, gpt_pte);
		free(gpt_pte);
		ret |= write_gpt_alternate(alternate);
		return ret;
	} else if (is_gpt_valid(dev_desc, alternate, gpt_head, &gpt_pte) == 1) {
		/* change alternate header to first header */
		prepare_backup_gpt_header(gpt_head);
		ret = write_gpt_table(dev_desc, gpt_head, gpt_pte);
		free(gpt_pte);
		ret |= write_gpt_alternate(alternate);
		return ret;
	}

	return 1;
}

void mmc_check_gpt(struct mmc *mmc)
{
	lbaint_t alternate;
	int ret;

	if (_mmc_check_gpt(mmc, &alternate)) {
		printf("GPT is not complete\n");
		ret = mmc_repair_gpt(mmc, alternate);
		printf("GPT is repaired %s\n", ret ? "failed" : "success");
	}
	printf("gpt is complete\n");
}

int resize_gpt(struct mmc *mmc)
{
	gpt_header *gpt_h;
	void *buf;
	int ret;

	struct blk_desc *dev_desc = mmc_get_blk_desc(mmc);

	buf = malloc(GPT_SIZE);
	if (!buf) {
		printf("not enough space for gpt buffer\n");
		return -1;
	}

	ret = mmc_gpt_read(buf);
	if (ret == 0) {
		/* determine start of GPT Header in the buffer */
		gpt_h = buf + (GPT_PRIMARY_PARTITION_TABLE_LBA *
				dev_desc->blksz);
		if (le64_to_cpu(gpt_h->last_usable_lba) > dev_desc->lba) {
			ret = write_mbr_and_gpt_partitions(dev_desc, (u_char *)buf);
			if (ret) {
				printf("%s: writing GPT partitions failed\n", __func__);
				free(buf);
				return -1;
			}
			printf("resize gpt success\n");
		}
	} else if (ret == -1) {
		printf("%s: read gpt failed\n", __func__);
		free(buf);
		return -1;
	}
	free(buf);
	return 0;
}

/***************************************************
 *	init partition table for emmc device.
 *	returns 0 means ok.
 *			other means failure.
 ***************************************************
 *  work flows:
 *	source of logic partition table(LPT) is from dts
 *	no matter MACRO is on/off
 *		1. try to get LPT from dtb
 *			1.1 if dtb exist, compose ept by LPT&inh
 *			1.2 if not, go ahead
 *      2. try to get ept from emmc rsv partition
 *			2.1 if not:
 * 				2.1.1 when dtb exists
 *					2.1.1.1 check ept with dtb
 *					2.1.1.2 update rsv if needed
 * 				2.1.1 without dtb, exit
 *			2.2 if got:
 *				2.2.1 try to reconstruct ept by MBR
 *				2.2.2 check it with ept
 *				2.2.3 update MBR if needed
 ***************************************************
 *	when normal boot:
 *		without dtb, with rsv, with MBR
 *  when blank emmc:
 *		without dtb, without rsv, without MBR
 *  when burning MBR on a blank emmc:
 *		with dtb, without rsv, without MBR
 *  when burning MBR on a emmc with rsv:
 *		with dtb, with rsv, without MBR
 *  when burning MBR on a emmc with rsv&MBR:
 *		with dtb, with rsv, with MBR
 ***************************************************/
int mmc_device_init (struct mmc *mmc)
{
	int ret = 1;

#ifdef CONFIG_AML_PARTITION
	int update = 1;
	struct _iptbl *p_iptbl_rsv = NULL;
#endif

#if (CONFIG_PTBL_MBR)  || (!CONFIG_AML_PARTITION)
	cpu_id_t cpu_id = get_cpu_id();
#endif
	/* partition table from dtb/code/emmc rsv */
	struct _iptbl iptbl_dtb, iptbl_inh;

	/* For re-entry */
	if (!p_iptbl_ept) {
		ret = _zalloc_iptbl(&p_iptbl_ept);
		if (ret)
			goto _out;
	} else {
		p_iptbl_ept->count = 0;
		memset(p_iptbl_ept->partitions, 0,
				sizeof(struct partitions) * MAX_PART_COUNT);
	}

	mmc_check_gpt(mmc);

	if (resize_gpt(mmc))
		goto _out;

	/* calculate inherent offset */
	iptbl_inh.count = get_emmc_partition_arraysize();
	if (iptbl_inh.count) {
		iptbl_inh.partitions = emmc_partition_table;
		_calculate_offset(mmc, &iptbl_inh, 0);
	}
	apt_info("inh count %d\n",  iptbl_inh.count);

	ret = get_ept_from_gpt(mmc);
	if (!ret) {
#ifdef CONFIG_AML_PARTITION
		/* init part again */
		part_init(mmc_get_blk_desc(mmc));
		return enable_rsv_part_table(mmc);
#else
		part_init(mmc_get_blk_desc(mmc));
		return ret;
#endif
	}

#if (CONFIG_MPT_DEBUG)
	apt_info("inherent partition table\n");
	_dump_part_tbl(iptbl_inh.partitions, iptbl_inh.count);
#endif

	/* try to get partition table from dtb(ddr or emmc) */
	iptbl_dtb.partitions = get_ptbl_from_dtb(mmc);
	/* construct ept by dtb if exist */
	if (iptbl_dtb.partitions) {
		iptbl_dtb.count = get_partition_count();
		apt_info("dtb %p, count %d\n", iptbl_dtb.partitions, iptbl_dtb.count);
		/* reserved partition must exist! */
		if (iptbl_inh.count) {
			compose_ept(&iptbl_dtb, &iptbl_inh, p_iptbl_ept);
			if (0 == p_iptbl_ept->count) {
				apt_err("compose partition table failed!\n");
				goto _out;
			}
			/* calculate offset infos. considering GAPs */
			if (_calculate_offset(mmc, p_iptbl_ept, 1)) {
				goto _out;
			}
		#if (CONFIG_MPT_DEBUG)
			apt_info("ept partition table\n");
			_dump_part_tbl(p_iptbl_ept->partitions, p_iptbl_ept->count);
		#endif
		} else {
			/* report fail, because there is no reserved partitions */
			apt_err("compose partition table failed!\n");
			ret = -1;
			goto _out;
		}
	} else
		apt_wrn("get partition table from dtb failed\n");
#ifndef CONFIG_AML_PARTITION
	if (cpu_id.family_id < MESON_CPU_MAJOR_ID_G12B) {
		printf("CONFIG_AML_PARTITION should define before G12B\n");
		goto _out;
	}
#endif

#ifdef CONFIG_AML_PARTITION
	/* try to get partiton table from rsv */
	ret = _zalloc_iptbl(&p_iptbl_rsv);
	if (ret)
		goto _out;
	ret = get_ptbl_rsv(mmc, p_iptbl_rsv);
	if (p_iptbl_rsv->count) {
		/* dtb exist, p_iptbl_ept already inited */
		if (iptbl_dtb.partitions) {
			ret = _cmp_iptbl(p_iptbl_ept, p_iptbl_rsv);
			if (!ret) {
				update = 0;
			}
		} else {
			/* without dtb, update ept with rsv */
		#if 0
			p_iptbl_ept->count = p_iptbl_rsv->count;
			memcpy(p_iptbl_ept->partitions, p_iptbl_rsv->partitions,
				p_iptbl_ept->count * sizeof(struct partitions));
		#endif
			_cpy_iptbl(p_iptbl_ept, p_iptbl_rsv);
			update = 0;
		}
	} else {
		/* without dtb& rsv */
		if (!iptbl_dtb.partitions) {
			apt_err("dtb&rsv are not exist, no LPT source\n");
			ret = -9;
			goto _out;
		}
	}

	if (update && iptbl_dtb.partitions && (aml_gpt_valid(mmc) != 0)) {
		apt_wrn("update rsv with dtb!\n");
		ret = update_ptbl_rsv(mmc, p_iptbl_ept);
	}
#endif
	//apt_wrn("ept source is %s\n", (ept_source == p_iptbl_ept)?"ept":"rsv");
#if (CONFIG_PTBL_MBR)
	/* 1st sector was reserved by romboot after gxl */
	if (cpu_id.family_id >= MESON_CPU_MAJOR_ID_GXL) {
		if (_check_ptbl_mbr(mmc, p_iptbl_ept)) {
			/*fixme, comaptible for mbr&ebr */
			ret |= _update_ptbl_mbr(mmc, p_iptbl_ept);
			apt_wrn("MBR Updated!\n");
		}
	}
#endif

	_update_part_tbl(p_iptbl_ept->partitions, p_iptbl_ept->count);

	/* init part again */
	part_init(mmc_get_blk_desc(mmc));

_out:
#ifdef CONFIG_AML_PARTITION
	if (p_iptbl_rsv)
		_free_iptbl(p_iptbl_rsv);
#endif
	return ret;
}

struct partitions *find_mmc_partition_by_name (char const *name)
{
	struct partitions *partition = NULL;

	apt_info("p_iptbl_ept %p\n", p_iptbl_ept);
	if (NULL == p_iptbl_ept) {
		goto _out;
	}
	partition = p_iptbl_ept->partitions;
	partition = _find_partition_by_name(partition,
			p_iptbl_ept->count, name);
	apt_info("partition %p\n", partition);
	if (!partition) {
		partition = _find_partition_by_name(emmc_partition_table,
			get_emmc_partition_arraysize(), name);
	}
	apt_info("partition %p\n", partition);
_out:
	return partition;
}

/*
 find virtual partition in inherent table.
*/
int find_virtual_partition_by_name (char const *name, struct partitions *partition)
{
	int ret = 0;
	ulong offset;
	struct virtual_partition *vpart = aml_get_virtual_partition_by_name(MMC_DTB_NAME);
	if (NULL == partition)
		return -1;

	offset = _get_inherent_offset(MMC_RESERVED_NAME);
	if (-1 == offset) {
		apt_err("can't find %s in inherent\n", MMC_RESERVED_NAME);
		return -1;
	}

	if (!strcmp(name, "dtb")) {
		strncpy(partition->name, name, sizeof(partition->name) - 1);
          	partition->name[sizeof(partition->name) - 1] = '\0';
		partition->offset = offset + vpart->offset;
		partition->size = (vpart->size * DTB_COPIES);
	}

	return ret;
}

int find_dev_num_by_partition_name (char const *name)
{
	int dev = -1;

	/* card */
	if (!strcmp(name, MMC_CARD_PARTITION_NAME)) {
		dev = 0;
    } else { /* eMMC OR TSD */
		/* partition name is valid */
		if (find_mmc_partition_by_name(name)) {
			dev = 1;
		}
	}
	return dev;
}

static inline uint64_t get_part_size(struct partitions *part, int num)
{
    return part[num].size;
}

static inline uint64_t get_part_offset(struct partitions *part, int num)
{
    return part[num].offset;
}

static inline char * get_part_name(struct partitions *part, int num)
{
    return (char *)part[num].name;
}

int get_part_info_from_tbl(struct blk_desc *dev_desc,
	int num, disk_partition_t *info)
{
    int ret = 0;
    struct partitions *part;

	if (NULL == p_iptbl_ept)
        return -1;
	if (num > (p_iptbl_ept->count-1))
        return -1;
    part = p_iptbl_ept->partitions;

    /*get partition info by index! */
    info->start = (lbaint_t)(get_part_offset(part, num)/dev_desc->blksz);
    info->size = (lbaint_t)(get_part_size(part, num)/dev_desc->blksz);
	info->blksz = dev_desc->blksz;
    strcpy((char *)info->name, get_part_name(part, num));

    return ret;
}
#if (CONFIG_MPT_DEBUG)
void show_partition_info(disk_partition_t *info)
{
	printf("----------%s----------\n", __func__);
	printf("name %10s\n", info->name);
	printf("blksz " LBAFU "\n", info->blksz);
	printf("sart %ld\n", info->start);
	printf("size %ld\n", info->size);
	printf("----------end----------\n");
}
#endif

struct partitions *aml_get_partition_by_name(const char *name)
{
	struct partitions *partition = NULL;
	partition = _find_partition_by_name(emmc_partition_table,
			get_emmc_partition_arraysize(), name);
	if (partition == NULL)
		apt_wrn("do not find match in inherent table %s\n", name);
	return partition;
}

struct virtual_partition *aml_get_virtual_partition_by_name(const char *name)
{
	int i = 0, cnt;
	struct virtual_partition *part = NULL;
	cnt = get_emmc_virtual_partition_arraysize();
	while (i < cnt) {

		part = &virtual_partition_table[i];
		if (!strcmp(name, part->name)) {
			apt_info("find %10s @ tbl[%d]\n", name, i);
			break;
		}
		i++;
	};
	if (i == cnt) {
		part = NULL;
		apt_wrn("do not find match in table %10s\n", name);
	}
	return part;
}

int get_part_info_by_name(struct blk_desc *dev_desc,
	const char *name, disk_partition_t *info)
{
	struct partitions *partition = NULL;
	struct partitions virtual;
	int ret = 0;
	cpu_id_t cpu_id = get_cpu_id();

	partition = find_mmc_partition_by_name((char *)name);
	if (partition) {
		info->start = (lbaint_t)(partition->offset/dev_desc->blksz);
		info->size = (lbaint_t)(partition->size/dev_desc->blksz);
		info->blksz = dev_desc->blksz;
		strcpy((char *)info->name, partition->name);
	} else if (!find_virtual_partition_by_name((char *)name, &virtual)) {
		/* try virtual partitions */
		apt_wrn("Got %s in virtual table\n", name);
		info->start = (lbaint_t)(virtual.offset/dev_desc->blksz);
		info->size = (lbaint_t)(virtual.size/dev_desc->blksz);
		info->blksz = dev_desc->blksz;
		strcpy((char *)info->name, virtual.name);
	} else {
		/* all partitions were tried, fail */
		ret = -1;
		goto _out;
	}
	/* for bootloader */
	if ((0 == info->start) && (cpu_id.family_id >= MESON_CPU_MAJOR_ID_GXL)) {
		info->start = 1;
		info->size -= 1;
	}

#if (CONFIG_MPT_DEBUG)
	show_partition_info(info);
#endif
_out:
	return ret;
}


/*
 * get the partition number by name
 * return value
 *     < 0 means no partition found
 *     >= 0 means valid partition
 */
__weak int get_partition_num_by_name(char const *name)
{
	   int ret = -1;
	   struct partitions *partition = NULL;

       if (NULL == p_iptbl_ept)
			   goto _out;
	   partition = p_iptbl_ept->partitions;
	   ret = _get_part_index_by_name(partition,
					   p_iptbl_ept->count, name);
_out:
	   return ret;
}

/*
 * get the partition info by number
 * return value
 *     < 0 means no partition found
 *     >= 0 means valid partition
 */
__weak struct partitions *get_partition_info_by_num(const int num)
{
	struct partitions *partition = NULL;

	if ((NULL == p_iptbl_ept)
		|| (num >= p_iptbl_ept->count))
		goto _out;
	partition = &p_iptbl_ept->partitions[num];

_out:
	return partition;
}

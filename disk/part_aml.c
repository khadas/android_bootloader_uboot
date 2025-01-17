// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <command.h>
#include <memalign.h>
#ifdef CONFIG_HAVE_BLOCK_DEVICE
extern int get_part_info_from_tbl(struct blk_desc * dev_desc,
	int part_num, disk_partition_t * info);
int get_part_info_by_name(struct blk_desc *dev_desc,
	const char *name, disk_partition_t *info);
#define	AML_PART_DEBUG	(0)

#if	(AML_PART_DEBUG)
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

#define AML_SEC_SIZE	(512)
#define MAGIC_OFFSET	(1)

/* read back boot partitions */
static int _get_partition_info_aml(struct blk_desc * dev_desc,
	int part_num, disk_partition_t * info, int verb)
{
	int ret = 0;

	if (IF_TYPE_MMC != dev_desc->if_type)
		return -1;

	info->blksz=dev_desc->blksz;
	sprintf ((char *)info->type, "U-Boot");
	/* using partition name in partition tables */
#ifdef CONFIG_MMC
	ret = get_part_info_from_tbl(dev_desc, part_num, info);
#else
	ret = -1;
#endif
	if (ret) {
		printf ("** Partition %d not found on device %d **\n",
			part_num,dev_desc->devnum);
		return -1;
	}

	PRINTF(" part %d found @ %lx size %lx\n",part_num,info->start,info->size);
	return 0;
}

int get_partition_info_aml(struct blk_desc * dev_desc,
	int part_num, disk_partition_t * info)
{
	return(_get_partition_info_aml(dev_desc, part_num, info, 1));
}

int get_partition_info_aml_by_name(struct blk_desc *dev_desc,
	const char *name, disk_partition_t *info)
{
	return (get_part_info_by_name(dev_desc,
		name, info));
}

void print_part_aml(struct blk_desc * dev_desc)
{
	disk_partition_t info;
	int i;
	if (_get_partition_info_aml(dev_desc,0,&info,0) == -1) {
		printf("** No boot partition found on device %d **\n",dev_desc->devnum);
		return;
	}
	printf("Part   Start     Sect x Size Type  name\n");
	i=0;
	do {
		printf(" %02d " LBAFU " " LBAFU " %6ld %.32s %.32s\n",
		       i++, info.start, info.size, info.blksz, info.type, info.name);
	} while (_get_partition_info_aml(dev_desc,i,&info,0)!=-1);
}
#define AML_MPT_OFFSET	(73728)	/* 36M */
/* fix 40Mbyte to check the MPT magic */
int test_part_aml (struct blk_desc *dev_desc)
{
	ALLOC_CACHE_ALIGN_BUFFER(char, buffer, dev_desc->blksz);
	if (blk_dread(dev_desc, AML_MPT_OFFSET, 1, (ulong *) buffer) != 1)
		return -1;
	if (!strncmp(buffer, "MPT", 3))
		return 0;
	return 1;
}


U_BOOT_PART_TYPE(aml) = {
	.name		= "AML",
	.part_type	= PART_TYPE_AML,
	.max_entries	= AML_ENTRY_NUMBERS,
	.get_info	= get_partition_info_aml,
	.print		= print_part_aml,
	.test		= test_part_aml,
};


#endif

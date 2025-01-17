// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <bootm.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <linux/sizes.h>
#include <asm/arch/io.h>
#include <asm/arch/secure_apb.h>
#include <partition_table.h>
#include <bzlib.h>

//#define AML_DT_DEBUG
#ifdef AML_DT_DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...) ((void)0)
#endif

#define  AML_MULTI_DTB_API_NEW
//#define AML_MULTI_DTB_CHECK_CMD //command for multi-dtb test

#ifdef AML_MULTI_DTB_API_NEW

/*for multi-dtb gzip buffer*/
#ifndef GUNZIP_BUF_SIZE
#define GUNZIP_BUF_SIZE         BIT(20)     /*1MB  is enough?*/
#endif
#define BZIP2_BUF_SIZE          BIT(23)     /*8MB  is enough?*/

/*magic for multi-dtb*/
#define MAGIC_GZIP_MASK         (0x0000FFFF)
#define MAGIC_GZIP_ID           (0x00008B1F)
#define MAGIC_BZIP2_ID          (0x00005a42)
#define IS_GZIP_PACKED(nMagic)  (MAGIC_GZIP_ID == (MAGIC_GZIP_MASK & nMagic))
#define IS_BZIP2_PACKED(nMagic) (MAGIC_BZIP2_ID == (MAGIC_BZIP2_ID & (nMagic)))
#define MAGIC_DTB_SGL_ID        (0xedfe0dd0)
#define MAGIC_DTB_MLT_ID        (0x5f4c4d41)

/*amlogic multi-dtb version*/
#define AML_MUL_DTB_VER_1       (1)
#define AML_MUL_DTB_VER_2       (2)

/*max char for dtb name, fixed to soc_package_board format*/
#define AML_MAX_DTB_NAME_SIZE   (128)
#define AML_DTB_TOKEN_MAX_COUNT (AML_MAX_DTB_NAME_SIZE>>1)

#define MAX_DTB_SIZE		SZ_1M
#define MAX_DTB_COUNT		16

typedef struct{
	unsigned int nMagic;
	unsigned int nVersion;
	unsigned int nDTBCount;
}st_dtb_hdr_t,*p_st_dtb_hdr_t;

/*v1,v2 multi-dtb only support max to 3 tokens for each DTB name*/
#define MULTI_DTB_TOKEN_MAX_COUNT      (3)
#define MULTI_DTB_TOKEN_UNIT_SIZE_V1   (4)      //v1 support 4bytes for each token
#define MULTI_DTB_TOKEN_UNIT_SIZE_V2   (16)     //v2 support 16bytes for each token

/*v1 multi-dtb*/
typedef struct{
	unsigned char     szToken[MULTI_DTB_TOKEN_MAX_COUNT][MULTI_DTB_TOKEN_UNIT_SIZE_V1];
	unsigned int      nDTBOffset;
	unsigned int      nDTBIMGSize;
}st_dtb_token_v1_t,*p_st_dtb_token_v1_t;

typedef struct{
	st_dtb_hdr_t      hdr;
	st_dtb_token_v1_t dtb[1];
}st_dtb_v1_t,*p_st_dtb_v1_t;


/*v2 multi-dtb*/
typedef struct{
	unsigned char     szToken[MULTI_DTB_TOKEN_MAX_COUNT][MULTI_DTB_TOKEN_UNIT_SIZE_V2];
	unsigned int      nDTBOffset;
	unsigned int      nDTBIMGSize;
}st_dtb_token_v2_t,*p_st_dtb_token_v2_t;

typedef struct{
	st_dtb_hdr_t      hdr;
	st_dtb_token_v2_t dtb[1];
}st_dtb_v2_t,*p_st_dtb_v2_t;


/*to get the valid DTB index with matched DTB name*/
static int get_dtb_index(const char aml_dt_buf[128],unsigned long fdt_addr)
{
	int nReturn = -1;

	if (aml_dt_buf) {
		p_st_dtb_hdr_t pDTBHdr = (p_st_dtb_hdr_t)fdt_addr;
		char sz_aml_dt_msb[10][MULTI_DTB_TOKEN_UNIT_SIZE_V2];

		memset(sz_aml_dt_msb, 0, sizeof(sz_aml_dt_msb));

		/* split aml_dt with token '_',  e.g "tm2-revb_t962x3_ab301" */
		//printf("		aml_dt : %s\n",aml_dt_buf);

		char *tokens[AML_DTB_TOKEN_MAX_COUNT];
		char sz_temp[AML_MAX_DTB_NAME_SIZE + 4];

		memset(tokens, 0, sizeof(tokens));
		memset(sz_temp, 0, sizeof(sz_temp));
		strncpy(sz_temp, aml_dt_buf, 128);

		int i, j;
		int nLen = strlen(sz_temp);

		sz_temp[nLen] = '_';
		sz_temp[nLen + 1] = '\0';
		nLen += 1;
		tokens[0] = sz_temp;
		for (i = 1; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
			tokens[i] = strstr(tokens[i - 1], "_");
			if (!tokens[i])
				break;

			*tokens[i] = '\0';

			tokens[i] = tokens[i] + 1;

			if (!(*tokens[i])) {
				tokens[i] = 0;
				break;
			}
		}

		//for (i=0;i<10 && tokens[i];++i)
		//	printf("token-%d:%s\n",i,tokens[i]);

		int nTokenLen = 0, nDtbcnt = 0;

		switch (pDTBHdr->nVersion) {
		case AML_MUL_DTB_VER_1: {
			nTokenLen = MULTI_DTB_TOKEN_UNIT_SIZE_V1;
		} break;
		case AML_MUL_DTB_VER_2: {
			nTokenLen = MULTI_DTB_TOKEN_UNIT_SIZE_V2;
		} break;
		default: {
			goto exit; break;
		}
		}

		for (i = 0; i < MULTI_DTB_TOKEN_MAX_COUNT; ++i) {
			if (tokens[i]) {
				char *pbyswap = (char *)sz_aml_dt_msb + (nTokenLen * i);
				unsigned int nValSwap;
				int m;

				strcpy(pbyswap, tokens[i]);
				for (j = 0; j < nTokenLen; j += 4) {
					/*swap byte order with unit@4bytes*/
					nValSwap = *(unsigned int *)(pbyswap + j);
					for (m = 0; m < 4; m++) {
						pbyswap[j + m] = ((nValSwap >>
									((3 - m) << 3)) & 0xFF);
					}

					/*replace 0 with 0x20*/
					for (m = 0 ; m < MULTI_DTB_TOKEN_UNIT_SIZE_V2; ++m)
						//if (pbyswap[m] == 0)
						//	pbyswap[m] = 0x20;
						/* avoid coverity */
						pbyswap[m] == 0 ? pbyswap[m] = 0x20 : 0;
				}
			} else {
				break;
			}
		}

		switch (pDTBHdr->nVersion) {
		case AML_MUL_DTB_VER_1: {
			p_st_dtb_v1_t pDTB_V1 = (p_st_dtb_v1_t)fdt_addr;

			nDtbcnt = pDTB_V1->hdr.nDTBCount;
			for (i = 0; i < pDTB_V1->hdr.nDTBCount; ++i) {
				if (!memcmp(pDTB_V1->dtb[i].szToken, sz_aml_dt_msb,
					MULTI_DTB_TOKEN_MAX_COUNT * nTokenLen)) {
					nReturn = i;
					break;
				}
			}

		} break;
		case AML_MUL_DTB_VER_2: {
			p_st_dtb_v2_t pDTB_V2 = (p_st_dtb_v2_t)fdt_addr;

			nDtbcnt = pDTB_V2->hdr.nDTBCount;
			for (i = 0; i < pDTB_V2->hdr.nDTBCount; ++i) {
				if (!memcmp(pDTB_V2->dtb[i].szToken, sz_aml_dt_msb,
					MULTI_DTB_TOKEN_MAX_COUNT * nTokenLen)) {
					nReturn = i;
					break;
				}
			}

		} break;
		default: {
				goto exit; break;
			}
		}

		/* print dtb */
		char **dt_name;
		unsigned int x = 0, y = 0, z = 0; //loop counter
		unsigned int nDtbSwap;
		unsigned int aml_dtb_header_size = 8 + (nTokenLen * 3);

		dt_name = (char **)malloc(sizeof(char *) * MULTI_DTB_TOKEN_MAX_COUNT);
		for (i = 0; i < MULTI_DTB_TOKEN_MAX_COUNT; i++)
			dt_name[i] = (char *)malloc(sizeof(char) * nTokenLen);

		for (i = 0; i < nDtbcnt; i++) {
			for (x = 0; x < MULTI_DTB_TOKEN_MAX_COUNT; x++) {
				for (y = 0; y < nTokenLen; y += 4) {
					nDtbSwap = *(unsigned int *)(fdt_addr + 12 +
							i * aml_dtb_header_size +
							0 + (x * nTokenLen) + y);
					for (z = 0; z < 4; z++)
						dt_name[x][y + z] = (nDtbSwap >>
							((3 - z) << 3)) & 0xFF;
					/*replace 0 with 0x20*/
					for (z = 0; z < nTokenLen; z++)
						//if (dt_name[x][z] == 0x20)
						//	dt_name[x][z] = '\0';
						/* avoid coverity */
						dt_name[x][z] == 0x20 ? dt_name[x][z] = '\0' : 0;
				}
			}
			if (pDTBHdr->nVersion == 1)
				printf("      dtb %d soc: %.4s	plat: %.4s   vari: %.4s\n",
						i, (char *)(dt_name[0]), (char *)(dt_name[1]),
						(char *)(dt_name[2]));
			else if (pDTBHdr->nVersion == 2)
				printf("      dtb %d soc: %.16s   plat: %.16s	vari: %.16s\n",
						i, (char *)(dt_name[0]), (char *)(dt_name[1]),
						(char *)(dt_name[2]));
		}
		if (dt_name)
			free(dt_name);
	} else {
		goto exit;
	}

exit:

	return nReturn;

}

unsigned long __attribute__((unused))	get_multi_dt_entry(unsigned long fdt_addr)
{
	unsigned long lReturn = 0; //return buffer for valid DTB;
	void * gzip_buf = NULL;
	unsigned long pInputFDT  = fdt_addr;
	p_st_dtb_hdr_t pDTBHdr   = (p_st_dtb_hdr_t)pInputFDT;
	unsigned long unzip_size = GUNZIP_BUF_SIZE;
	unsigned int src_len = BZIP2_BUF_SIZE;
	unsigned int dest_len = BZIP2_BUF_SIZE;

	printf("      Amlogic Multi-DTB tool\n");

	/* first check the blob header, support GZIP format */
	if ( IS_GZIP_PACKED(pDTBHdr->nMagic))
	{
		printf("      GZIP format, decompress...\n");
		gzip_buf = malloc(GUNZIP_BUF_SIZE);
		if (!gzip_buf)
		{
			printf("      ERROR! fail to allocate memory for GUNZIP...\n");
			goto exit;
		}
		memset(gzip_buf, 0, GUNZIP_BUF_SIZE);
		if (gunzip(gzip_buf, GUNZIP_BUF_SIZE, (void *)pInputFDT, &unzip_size) < 0)
		{
			printf("      ERROR! GUNZIP process fail...\n");
			goto exit;
		}
		if (unzip_size > GUNZIP_BUF_SIZE)
		{
			printf("      ERROR! GUNZIP overflow...\n");
			goto exit;
		}
		//memcpy((void*)fdt_addr,gzip_buf,unzip_size);
		pInputFDT = (unsigned long)gzip_buf;
		pDTBHdr   = (p_st_dtb_hdr_t)pInputFDT;
	} else if (IS_BZIP2_PACKED(pDTBHdr->nMagic)) {
		printf("      BZIP2 format, decompress...\n");
		/*
		 * If we've got less than 4 MB of malloc() space,
		 * use slower decompression algorithm which requires
		 * at most 2300 KB of memory.
		 */
		gzip_buf = malloc(BZIP2_BUF_SIZE);
		if (!gzip_buf) {
			printf("      ERROR! fail to allocate memory for GUNZIP...\n");
			goto exit;
		}
		memset(gzip_buf, 0, BZIP2_BUF_SIZE);
		int ret = BZ2_bzBuffToBuffDecompress(gzip_buf, &dest_len,
				(void *)pInputFDT, src_len,
				1, 2);
		if (ret != BZ_OK) {
			printf("BZIP2: uncompress or overwrite error %d must RESET board\n", ret);
			goto exit;
		} else {
			pInputFDT = (unsigned long)gzip_buf;
			pDTBHdr   = (p_st_dtb_hdr_t)pInputFDT;
			unzip_size = dest_len;
		}
	}

	switch (pDTBHdr->nMagic)
	{
	case MAGIC_DTB_SGL_ID:
	{
		printf("      Single DTB detected\n");

		if (fdt_addr != (unsigned long)pInputFDT) //in case of GZIP single DTB
			memcpy((void*)fdt_addr,(void*)pInputFDT,unzip_size);

		lReturn = fdt_addr;

	}break;
	case MAGIC_DTB_MLT_ID:
	{
		printf("      Multi DTB detected.\n");
		printf("      Multi DTB tool version: v%d.\n", pDTBHdr->nVersion);
		printf("      Found %d DTBS.\n", pDTBHdr->nDTBCount);


		/* check and set aml_dt */
		char aml_dt_buf[AML_MAX_DTB_NAME_SIZE+4];
		memset(aml_dt_buf, 0, sizeof(aml_dt_buf));

		/* update 2016.07.27, checkhw and setenv everytime,
		or else aml_dt will set only once if it is reserved */
		extern int checkhw(char * name);
#if 1
		if (checkhw(aml_dt_buf) < 0 || strlen(aml_dt_buf) <= 0)
		{
			printf("      Get env aml_dt failed!\n");
			goto exit;
		}
#else
		char *aml_dt = getenv(AML_DT_UBOOT_ENV);
		/* if aml_dt not exist or env not ready, get correct dtb by name */
		if (NULL == aml_dt)
			checkhw(aml_dt_buf);
		else
			memcpy(aml_dt_buf, aml_dt,
			(strlen(aml_dt)>AML_MAX_DTB_NAME_SIZE?AML_MAX_DTB_NAME_SIZE:(strlen(aml_dt)+1)));
#endif

		int dtb_match_num = get_dtb_index(aml_dt_buf,(unsigned long)pInputFDT);

		/*check valid dtb index*/
		if (dtb_match_num < 0 || dtb_match_num >= pDTBHdr->nDTBCount)
		{
			printf("      NOT found matched DTB for \"%s\"\n",aml_dt_buf);
			goto exit;
		}

		printf("      Matched DTB for \"%s\"\n",aml_dt_buf);

		switch (pDTBHdr->nVersion)
		{
		case AML_MUL_DTB_VER_1:
		{
			p_st_dtb_v1_t pDTB_V1 = (p_st_dtb_v1_t)pInputFDT;

			if (pDTB_V1->hdr.nDTBCount > MAX_DTB_COUNT) {
				printf("Wrong dtb cnt1:%d\n", pDTB_V1->hdr.nDTBCount);
				goto exit;
			}

			if ((pDTB_V1->dtb[dtb_match_num].nDTBIMGSize > MAX_DTB_SIZE) ||
			    pDTB_V1->dtb[dtb_match_num].nDTBOffset >= MAX_DTB_SIZE * (pDTBHdr->nDTBCount)) {
				printf("%s, ERRROR dtb size:%d or offset:%d\n",
					__func__,
					pDTB_V1->dtb[dtb_match_num].nDTBIMGSize,
					pDTB_V1->dtb[dtb_match_num].nDTBOffset);
				goto exit;
			}

			lReturn = pDTB_V1->dtb[dtb_match_num].nDTBOffset + pInputFDT;

			//if (pInputFDT != fdt_addr)
			{
				memcpy((void*)fdt_addr, (void*)lReturn,pDTB_V1->dtb[dtb_match_num].nDTBIMGSize);
				lReturn = fdt_addr;
			}

		}break;
		case AML_MUL_DTB_VER_2:
		{
			p_st_dtb_v2_t pDTB_V2 = (p_st_dtb_v2_t)pInputFDT;

			if (pDTB_V2->hdr.nDTBCount > MAX_DTB_COUNT) {
				printf("Wrong dtb cnt2:%d\n", pDTB_V2->hdr.nDTBCount);
				goto exit;
			}

			if ((pDTB_V2->dtb[dtb_match_num].nDTBIMGSize > MAX_DTB_SIZE) ||
			    pDTB_V2->dtb[dtb_match_num].nDTBOffset >= MAX_DTB_SIZE * (pDTBHdr->nDTBCount)) {
				printf("%s v2, ERRROR dtb size:%d or offset:%d\n",
					__func__,
					pDTB_V2->dtb[dtb_match_num].nDTBIMGSize,
					pDTB_V2->dtb[dtb_match_num].nDTBOffset);
				goto exit;
			}

			lReturn = pDTB_V2->dtb[dtb_match_num].nDTBOffset + pInputFDT;

			//if (pInputFDT != fdt_addr)
			{
				memcpy((void*)fdt_addr, (void*)lReturn,pDTB_V2->dtb[dtb_match_num].nDTBIMGSize);
				lReturn = fdt_addr;
			}

		}break;
		default:
		{
			printf("      Invalid Multi-DTB Version [%d]!\n",
				pDTBHdr->nVersion);
			goto exit;
		}break;
		}

	}break;
	default: goto exit; break;
	}

exit:

	if (gzip_buf)
	{
		free(gzip_buf);
		gzip_buf = 0;
	}

	return lReturn;
}

#else //#ifdef AML_MULTI_DTB_API_NEW

#define AML_DT_IND_LENGTH_V1		4	/*fixed*/
#define AML_DT_IND_LENGTH_V2		16	/*fixed*/

#define AML_DT_IND_LENGTH			16
#define AML_DT_ID_VARI_TOTAL		3	//Total 3 strings
#define AML_DT_EACH_ID_INT			(AML_DT_IND_LENGTH / 4)

/*Latest version: v2*/
#define AML_DT_VERSION_OFFSET		4
#define AML_DT_TOTAL_DTB_OFFSET		8
#define AML_DT_FIRST_DTB_OFFSET		12
//#define AML_DT_DTB_HEADER_SIZE	(8+(AML_DT_IND_LENGTH * AML_DT_ID_VARI_TOTAL))
#define AML_DT_DTB_DT_INFO_OFFSET	0
//#define AML_DT_DTB_OFFSET_OFFSET	(AML_DT_IND_LENGTH * AML_DT_ID_VARI_TOTAL)
//#define AML_DT_DTB_SIZE_OFFSET	16

#define AML_DT_UBOOT_ENV	"aml_dt"
#define DT_HEADER_MAGIC		0xedfe0dd0	/*header of dtb file*/
#define AML_DT_HEADER_MAGIC	0x5f4c4d41	/*"AML_", multi dtbs supported*/

#define IS_GZIP_FORMAT(data)		((data & (0x0000FFFF)) == (0x00008B1F))
#define GUNZIP_BUF_SIZE				(0x500000) /* 5MB */
#define DTB_MAX_SIZE				(AML_DTB_IMG_MAX_SZ)

//#define readl(addr) (*(volatile unsigned int*)(addr))
extern int checkhw(char * name);

unsigned long __attribute__((unused))
	get_multi_dt_entry(unsigned long fdt_addr){
	unsigned int dt_magic = readl(fdt_addr);
	unsigned int dt_total = 0;
	unsigned int dt_tool_version = 0;
	unsigned int gzip_format = 0;
	void * gzip_buf = NULL;
	unsigned long dt_entry = fdt_addr;

	printf("      Amlogic multi-dtb tool\n");

	/* first check the file header, support GZIP format */
	gzip_format = IS_GZIP_FORMAT(dt_magic);
	if (gzip_format) {
		printf("      GZIP format, decompress...\n");
		gzip_buf = malloc(GUNZIP_BUF_SIZE);
		memset(gzip_buf, 0, GUNZIP_BUF_SIZE);
		unsigned long unzip_size = GUNZIP_BUF_SIZE;
		gunzip(gzip_buf, GUNZIP_BUF_SIZE, (void *)fdt_addr, &unzip_size);
		dbg_printf("      DBG: unzip_size: 0x%x\n", (unsigned int)unzip_size);
		if (unzip_size > GUNZIP_BUF_SIZE) {
			printf("      Warning! GUNZIP overflow...\n");
		}
		fdt_addr = (unsigned long)gzip_buf;
		dt_magic = readl(fdt_addr);
	}

	dbg_printf("      DBG: fdt_addr: 0x%x\n", (unsigned int)fdt_addr);
	dbg_printf("      DBG: dt_magic: 0x%x\n", (unsigned int)dt_magic);

	/*printf("      Process device tree. dt magic: %x\n", dt_magic);*/
	if (dt_magic == DT_HEADER_MAGIC) {/*normal dtb*/
		printf("      Single dtb detected\n");
		if (gzip_format) {
			memcpy((void *)dt_entry, (void *)fdt_addr, DTB_MAX_SIZE);
			fdt_addr = dt_entry;
			if (gzip_buf)
				free(gzip_buf);
		}
		return fdt_addr;
	}
	else if (dt_magic == AML_DT_HEADER_MAGIC) {/*multi dtb*/
		printf("      Multi dtb detected\n");
		/* check and set aml_dt */
		int i = 0;
		char *aml_dt_buf;
		aml_dt_buf = (char *)malloc(sizeof(char)*64);
		printf("Multi dtb malloc aml_dt_buf addr = %p\n", aml_dt_buf);
		memset(aml_dt_buf, 0, sizeof(aml_dt_buf));

		/* update 2016.07.27, checkhw and setenv everytime,
		or else aml_dt will set only once if it is reserved */
#if 1
		checkhw(aml_dt_buf);
#else
		char *aml_dt = getenv(AML_DT_UBOOT_ENV);
		/* if aml_dt not exist or env not ready, get correct dtb by name */
		if (NULL == aml_dt)
			checkhw(aml_dt_buf);
		else
			memcpy(aml_dt_buf, aml_dt, (strlen(aml_dt)>64?64:(strlen(aml_dt)+1)));
#endif

		unsigned int aml_dt_len = aml_dt_buf ? strlen(aml_dt_buf) : 0;
		if (aml_dt_len <= 0) {
			printf("      Get env aml_dt failed!\n");
			if (aml_dt_buf)
				free(aml_dt_buf);
			if (gzip_buf)
				free(gzip_buf);
			return fdt_addr;
		}

		/*version control, compatible with v1*/
		dt_tool_version = readl(fdt_addr + AML_DT_VERSION_OFFSET);
		unsigned int aml_each_id_length=0;
		unsigned int aml_dtb_offset_offset;
		unsigned int aml_dtb_header_size;
		if (dt_tool_version == 1)
			aml_each_id_length = 4;
		else if(dt_tool_version == 2)
			aml_each_id_length = 16;

		aml_dtb_offset_offset = aml_each_id_length * AML_DT_ID_VARI_TOTAL;
		aml_dtb_header_size = 8+(aml_each_id_length * AML_DT_ID_VARI_TOTAL);
		printf("      Multi dtb tool version: v%d .\n", dt_tool_version);

		/*fdt_addr + 0x8: num of dtbs*/
		dt_total = readl(fdt_addr + AML_DT_TOTAL_DTB_OFFSET);
		printf("      Support %d dtbs.\n", dt_total);

		/* split aml_dt to 3 strings */
		char *tokens[3] = {NULL, NULL, NULL};
		for (i = 0; i < AML_DT_ID_VARI_TOTAL; i++) {
			tokens[i] = strsep(&aml_dt_buf, "_");
		}
		//if (aml_dt_buf)
			//free(aml_dt_buf);
		printf("        aml_dt soc: %s platform: %s variant: %s\n", tokens[0], tokens[1], tokens[2]);

		/*match and print result*/
		char **dt_info;
		dt_info = (char **)malloc(sizeof(char *)*AML_DT_ID_VARI_TOTAL);
		for (i = 0; i < AML_DT_ID_VARI_TOTAL; i++)
			dt_info[i] = (char *)malloc(sizeof(char)*aml_each_id_length);
		unsigned int dtb_match_num = 0xffff;
		unsigned int x = 0, y = 0, z = 0; //loop counter
		unsigned int read_data;
		for (i = 0; i < dt_total; i++) {
			for (x = 0; x < AML_DT_ID_VARI_TOTAL; x++) {
				for (y = 0; y < aml_each_id_length; y+=4) {
					read_data = readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
						 i * aml_dtb_header_size + AML_DT_DTB_DT_INFO_OFFSET + \
						 (x * aml_each_id_length) + y);
					dt_info[x][y+0] = (read_data >> 24) & 0xff;
					dt_info[x][y+1] = (read_data >> 16) & 0xff;
					dt_info[x][y+2] = (read_data >> 8) & 0xff;
					dt_info[x][y+3] = (read_data >> 0) & 0xff;
				}
				for (z=0; z<aml_each_id_length; z++) {
					/*fix string with \0*/
					if (0x20 == (uint)dt_info[x][z]) {
						dt_info[x][z] = '\0';
					}
				}
				//printf("dt_info[x]: %s\n", dt_info[x]);
				//printf("strlen(dt_info[x]): %d\n", strlen(dt_info[x]));
			}
			if (dt_tool_version == 1)
				printf("        dtb %d soc: %.4s   plat: %.4s   vari: %.4s\n", i, (char *)(dt_info[0]), (char *)(dt_info[1]), (char *)(dt_info[2]));
			else if(dt_tool_version == 2)
				printf("        dtb %d soc: %.16s   plat: %.16s   vari: %.16s\n", i, (char *)(dt_info[0]), (char *)(dt_info[1]), (char *)(dt_info[2]));
			uint match_str_counter = 0;
			for (z=0; z<AML_DT_ID_VARI_TOTAL; z++) {
				/*must match 3 strings*/
				if (!strncmp(tokens[z], (char *)(dt_info[z]), strlen(tokens[z])) && \
					(strlen(tokens[z]) == strlen(dt_info[z])))
					match_str_counter++;
			}
			if (match_str_counter == AML_DT_ID_VARI_TOTAL) {
				//printf("Find match dtb\n");
				dtb_match_num = i;
			}
			for (z=0; z<AML_DT_ID_VARI_TOTAL; z++) {
				/*clear data for next loop*/
				memset(dt_info[z], 0, sizeof(aml_each_id_length));
			}
		}
		/*clean malloc memory*/
		for (i = 0; i < AML_DT_ID_VARI_TOTAL; i++) {
			if (dt_info[i])
				free(dt_info[i]);
		}
		if (dt_info)
			free(dt_info);

		/*if find match dtb, return address, or else return main entrance address*/
		if (0xffff != dtb_match_num) {
			printf("      Find match dtb: %d\n", dtb_match_num);
			/*this offset is based on dtb image package, so should add on base address*/
			fdt_addr = (fdt_addr + readl(fdt_addr + AML_DT_FIRST_DTB_OFFSET + \
				dtb_match_num * aml_dtb_header_size + aml_dtb_offset_offset));
			if (gzip_format) {
				memcpy((void *)dt_entry, (void *)fdt_addr, DTB_MAX_SIZE);
				fdt_addr = dt_entry;
				if (gzip_buf)
					free(gzip_buf);
			}
			return fdt_addr;
		}
		else{
			printf("      Not match any dtb.\n");
			if (gzip_buf)
				free(gzip_buf);
			return fdt_addr;
		}
	}
	else {
		printf("      Cannot find legal dtb!\n");
		if (gzip_buf)
			free(gzip_buf);
		return fdt_addr;
	}
}
#endif //#ifdef AML_MULTI_DTB_API_NEW

extern int check_valid_dts(unsigned char *buffer);
#ifdef 	AML_MULTI_DTB_CHECK_CMD
static int do_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long loadaddr = 0x1080000;

	if (argc > 1)
		loadaddr = simple_strtoul(argv[1],NULL,16);

	check_valid_dts((void*)loadaddr);

	return 0;
}

U_BOOT_CMD(
   dtb_chk,           //command name
   2,                 //maxargs
   0,                 //repeatable
   do_test,           //command function
   "multi-dtb check command",             //description
   "    argv: dtb_chk <dtbLoadaddr> \n"   //usage
   "    do dtb check, which already loaded at <dtbLoadaddr>.\n"
);
#endif //#ifdef 	AML_MULTI_DTB_CHECK_CMD

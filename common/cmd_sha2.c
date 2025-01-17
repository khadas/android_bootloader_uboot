// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/*
 * Functions in this file implement Amlogic SHA2 function
 *
 * Author : zhongfu.luo@amlogic.com
 *
 * Feature: implement uboot command for SHA2 common usage
 *          command : sha2 addr_in len [addr_out]
 *                         addr_in[IN]  : DDR address for input buffer
 *                         len[IN]      : total data size to be processed for SHA2
 *                         addr_out[OUT]: user defined output buffer for the SHA2 (if not set then the SHA2 of input will shown only but not stored)
 *
 * History: 1. 2017.06.01 function init
 *
 *
*/

#include <common.h>
#include <malloc.h>
#include <asm/arch/regs.h>
#include <u-boot/sha256.h>
#include <u-boot/sha1.h>
#include <asm/arch-c1/timer.h>

#define DATA_MAX_LEN    (1 << 31) //max length of SHA2 is 2 GB

static int do_sha2(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	int nReturn=CMD_RET_USAGE;
	ulong addr_in,nLength,addr_out = 0;
	unsigned char szSHA2[32];
	unsigned char *pSHA2 = szSHA2;
	int nSHA2Type = 256;
	char *endp;
	int i = 0;
	unsigned int ntime1,ntime2,ntime;

	/* need at least three arguments */
	if (argc < 3)
		goto exit;

	nReturn = __LINE__;

	if ( 0 == *argv[1] )
	{
		printf("[addr_in] format error! \n" );
		goto exit;
	}
	addr_in = simple_strtoul(argv[1], &endp, 16);
	if ( 0 != *endp )
	{
		printf("[addr_in] format error! \n" );
		goto exit;
	}
	nReturn = __LINE__;

	if ( 0 == *argv[2])
	{
		printf("[Length] format error! \n" );
		goto exit;
	}
	nLength = simple_strtoul(argv[2], &endp, 16);
	if (  0 != *endp )
	{
		printf("[Length] format error! \n" );
		goto exit;
	}
	if ( !nLength || nLength > DATA_MAX_LEN)
		{
			printf("Length range: 0x00000001 ~ 0x%08x Byte! \n", DATA_MAX_LEN );
			goto exit;
		}

	nReturn = __LINE__;


	if (argc > 3)
	{
		if ( 0 == *argv[3] )
		{
			printf("[addr_out] format error! \n" );
			goto exit;
		}
		addr_out = simple_strtoul(argv[3], &endp, 16);
		if ( 0 != *endp )
		{
			printf("[addr_out] format error! \n" );
			goto exit;
		}
		pSHA2=(unsigned char *)addr_out;
	}

	ntime1 = get_time();
	sha256_csum_wd((unsigned char *)addr_in,(unsigned int)nLength,pSHA2,0);
	ntime2 = get_time();
	ntime = ntime2 - ntime1;
	printf("\n cost time: %d us, bandwidth: %d M/s",ntime, (unsigned int)((float)nLength/1024/ntime*1000000/1024));

	if (argc > 3)
	printf("\nSHA%d of addr_in: 0x%08x, len: 0x%08x, addr_out: 0x%08x \n", nSHA2Type, (unsigned int)addr_in, (unsigned int)nLength,(unsigned int)addr_out);
	else
	printf("\nSHA%d of addr_in: 0x%08x, len: 0x%08x \n", nSHA2Type, (unsigned int)addr_in, (unsigned int)nLength);


	for (i=0; i<SHA256_SUM_LEN; i++)
		printf("%02x%s", pSHA2[i], ((i+1) % 16==0) ? "\n" :" ");

	nReturn = 0;

exit:

	return nReturn;
}


#undef DATA_MAX_LEN

U_BOOT_CMD(
	sha2, 4,	1,	do_sha2,
	"SHA2 command",
	"addr_in len [addr_out] \n"
);



/*
 * Feature: implement uboot command for test SHA2 common usage
 *          command : sha2test [writeval] [len]
 *                         writeval[IN]  : the value of data for test(default is 0x01)
 *                         len[IN]      : total data size to be processed for SHA2 (default is 128k bytes)
 *
 * History: 1. 2017.06.01 function init
 *
 *
*/

//#define CONFIG_AML_SHA2_TEST
#if defined(CONFIG_AML_SHA2_TEST)

//#define writel(val,reg) (*((volatile unsigned *)(reg)))=(val)
//#define readl(reg)		(*((volatile unsigned *)(reg)))
#define TEST_MAX_LEN    (10<<20)
#define TEST_MIN_LEN    (64)

static int do_sha2test(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{

	int nReturn = __LINE__;
	unsigned char *pBuffer = 0;

	int i;
	long nLength = TEST_MAX_LEN;
	unsigned int byPattern = 1;
	char *endp;
	int nSHA2Type = 256;
	unsigned int ntime1,ntime2,ntime;
	unsigned char szSHA2[32];
	int testLength[8]={TEST_MAX_LEN,1024*1024,100*1024,10*1024,1024,512,128,TEST_MIN_LEN};
	int c=0;

	#ifdef CONFIG_AML_HW_SHA2
		printf("aml log : Amlogic HW SHA2\n");
	#elif defined(CONFIG_ARMV8_CRYPTO)
		printf("aml log : ARMCE SHA2\n");
	#else
		printf("aml log : Software SHA2\n");
	#endif


	//pattern
	if (argc > 1)
	{
		if ( 0 == *argv[1] )
		{
			printf("[writeval] format error! \n" );
			goto exit;
		}
		byPattern = simple_strtoul(argv[1], &endp, 16);
		if ( 0 != *endp )
		{
			printf("[writeval] format error! \n" );
			goto exit;
		}
		if (  byPattern > 0xff)
		{
			printf("writeval range: 0x00 ~ 0xff! \n" );
			goto exit;
		}

	}

	nReturn = __LINE__;

	//length
	if (argc > 2)
	{
		if ( 0== *argv[2] )
		{
			printf("[Length] format error! \n" );
			goto exit;
		}
		nLength = simple_strtoul(argv[2], &endp, 16);
		if ( 0 != *endp )
		{
			printf("[Length] format error! \n" );
			goto exit;
		}

		if ( !nLength || nLength > TEST_MAX_LEN)
		{
			printf("Length range: 0x00000001 ~ 0x%08x Byte! \n", TEST_MAX_LEN );
			goto exit;
		}
	}

	nReturn = __LINE__;

	pBuffer=(unsigned char*)malloc(nLength);

	if (!pBuffer)
		{
		printf("malloc fail! \n" );
		goto exit;
	}

	//set buffer with dedicated pattern
	memset(pBuffer,byPattern,nLength);


	do
	{
		ntime1=readl(P_ISA_TIMERE);

		sha256_csum_wd(pBuffer,nLength,szSHA2,0);

		ntime2=readl(P_ISA_TIMERE);

		ntime = ntime2 - ntime1;

		printf("\nSHA%d of val:0x%02x, len:0x%08lx, time:%d us, BW:%ld kB/s\n",
			nSHA2Type, (unsigned int)byPattern, nLength, ntime,
			(long)((nLength * 1000000 >> 10) / ntime));

		//SHA2 dump
		for (i=0; i<SHA256_SUM_LEN; i++)
			printf("%02x%s", szSHA2[i], ((i+1) % 16==0) ? "\n" :" ");

		if (argc > 2)
			break;

		nLength =testLength[++c];
	}
	while (nLength > TEST_MIN_LEN) ;

	nReturn = 0;

exit:

	if (pBuffer)
	{
		free(pBuffer);
		pBuffer = 0;
	}

	return nReturn;

}

#undef writel
#undef readl
#undef TEST_MAX_LEN
#undef TEST_MIN_LEN

U_BOOT_CMD(
	sha2test, 3,	1,	do_sha2test,
	"test SHA2 fuction",
	"[writeval] [len] \n"
);

static char *sha1_msg =
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

static uint8_t sha1_hash[20] = {
	0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
	0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1
};

static char *sha256_msg =
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

static uint8_t sha256_hash[] = {
	0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8,
	0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
	0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67,
	0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1
};

static int do_sha_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t test_case = 0;
	char *endp = 0;
	uint8_t output[32];
	int ret = 0;

	if (argc != 2) {
		cmd_usage(cmdtp);
		return 1;
	}

	test_case = ustrtoul(argv[1], &endp, 0);

	switch (test_case) {
	case 0:
		flush_dcache_range((uintptr_t)sha256_msg, strlen(sha256_msg));
		sha256_csum_wd((uint8_t *)sha256_msg, strlen(sha256_msg),
				output, sizeof(output));
		ret = memcmp(sha256_hash, output, 32);
		if (ret)
			printf("test:%s sha256 failed\n", __func__);
		else
			printf("test:%s sha256 passed\n", __func__);
		return 0;
	case 1:
		flush_dcache_range((uintptr_t)sha1_msg, strlen(sha1_msg));
		sha1_csum_wd((uint8_t *)sha1_msg, strlen(sha1_msg),
		  output, sizeof(output));
		ret = memcmp(sha1_hash, output, 20);
		if (ret)
			printf("test:%s sha1 failed\n", __func__);
		else
			printf("test:%s sha1 passed\n", __func__);
	default:
		printf("Unknown test case\n");
		return 0;
	}

	return 0;
}

U_BOOT_CMD(shatest, 2, 0, do_sha_test,
		"shatest <test case>",
		"run small sha test cases\n"
	  );
#endif //#if defined(CONFIG_AML_SHA2_TEST)

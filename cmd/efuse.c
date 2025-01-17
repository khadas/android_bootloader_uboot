// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <asm/arch/efuse.h>
#include <asm/arch/bl31_apis.h>
#include <amlogic/aml_efuse.h>

#define CMD_EFUSE_WRITE            0
#define CMD_EFUSE_READ             1
#define CMD_EFUSE_READ_CALI        2
#define CMD_EFUSE_READ_CALI_ITEM   3
#define CMD_EFUSE_SECURE_BOOT_SET  6
#define CMD_EFUSE_PASSWORD_SET     7
#define CMD_EFUSE_CUSTOMER_ID_SET  8

#define CMD_EFUSE_CHECK_PATTERN_ITEM   10

#define CMD_EFUSE_AMLOGIC_SET      20

#define setenv env_set
#define getenv env_get

int cmd_efuse(int argc, char * const argv[], char *buf)
{
	int i, action = -1;
	u64 offset;
	u32 size = 0, max_size;
	char *end;
	char *s;
	int ret;
	long lAddr1, lAddr2;
	unsigned long nType = 0;

	if (strncmp(argv[1], "mac", 3) == 0) {
		char mac[6] = {0};
		char mac_addr[17] = {0};
		int size = 6;
		uint32_t mac_offset = 0;
		ret = efuse_read_usr(mac, size, (loff_t *)&mac_offset);

		if (ret == -1) {
			printf("ERROR: efuse read mac information fail!\n");
			return -1;
		}

		if (ret != size)
			printf("ERROR: read %d byte(s) not %d byte(s) data\n",
			       ret, size);

		/*for(i = 0; i < 6; i++){
			if(i != 5){
				printf("%02x:", mac[i]);
			}else{
				printf("%02x", mac[i]);
			}
		}
		printf("\n");
		*/
		sprintf(mac_addr,"%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		setenv("eth_mac", mac_addr);
		return 0;
	}
	if (strncmp(argv[1], "usid", 4) == 0) {
		char usid[11] = {0};
		char usid_write[11] = {0};
		int size = 10;
		uint32_t usid_offset = 18;
		ret = efuse_read_usr(usid, size, (loff_t *)&usid_offset);

		if (ret == -1) {
			printf("ERROR: efuse read mac information fail!\n");
			return -1;
		}

		if (ret != size)
			printf("ERROR: read %d byte(s) not %d byte(s) data\n",
			       ret, size);

		/*for(i = 0; i < 10; i++){
			printf("wjh=%c", usid[i]);
		}
		printf("\n");
		*/
		sprintf(usid_write,"%x%x%x%x%x%x%x%x%x%x", usid[0],usid[1],usid[2],usid[3],usid[4],usid[5],usid[6],usid[7],usid[8],usid[9]);
		setenv("usid", usid_write);
		return 0;
	}
	if (strncmp(argv[1], "read", 4) == 0) {
		action = CMD_EFUSE_READ;
	} else if (strncmp(argv[1], "cali_read", 9) == 0) {
		action = CMD_EFUSE_READ_CALI;
	} else if (strncmp(argv[1], "item_read", 9) == 0) {
		action = CMD_EFUSE_READ_CALI_ITEM;
		goto efuse_action;
	} else if (strncmp(argv[1], "check", 9) == 0) {
		action = CMD_EFUSE_CHECK_PATTERN_ITEM;
		goto efuse_action;
	} else if (strncmp(argv[1], "write", 5) == 0) {
		action = CMD_EFUSE_WRITE;
	} else if (strncmp(argv[1], "secure_boot_set", 15) == 0) {
		action = CMD_EFUSE_SECURE_BOOT_SET;
		goto efuse_action;
	} else if (strncmp(argv[1], "password_set", 12) == 0) {
		action = CMD_EFUSE_PASSWORD_SET;
		goto efuse_action;
	} else if (strncmp(argv[1], "customer_id_set", 15) == 0) {
		action = CMD_EFUSE_CUSTOMER_ID_SET;
		goto efuse_action;
	} else if (strncmp(argv[1], "amlogic_set", 11) == 0) {
		action = CMD_EFUSE_AMLOGIC_SET;
		goto efuse_action;
	} else{
		printf("%s arg error\n", argv[1]);
		return CMD_RET_USAGE;
	}

	if (argc < 4)
		return CMD_RET_USAGE;
	/*check efuse user data max size*/
	offset = simple_strtoul(argv[2], &end, 16);
	size = simple_strtoul(argv[3], &end, 16);
	printf("%s: offset is %lld  size is  %d\n", __func__, offset, size);
	max_size = efuse_get_max();
	if (!size) {
		printf("\n error: size is zero!!!\n");
		return -1;
	}
	if (offset > max_size) {
		printf("\n error: offset is too large!!!\n");
		printf("\n offset should be less than %d!\n", max_size);
		return -1;
	}
	if (offset + size > max_size) {
		printf("\n error: offset + size is too large!!!\n");
		printf("\n offset + size should be less than %d!\n", max_size);
		return -1;
	}

efuse_action:

	/* efuse read */
	if (action == CMD_EFUSE_READ) {
		memset(buf, 0, size);
		ret = efuse_read_usr(buf, size, (loff_t *)&offset);
		if (ret == -1) {
			printf("ERROR: efuse read user data fail!\n");
			return -1;
		}

		if (ret != size)
			printf("ERROR: read %d byte(s) not %d byte(s) data\n",
			       ret, size);
		printf("efuse read data");
		for (i = 0; i < size; i++) {
			if (i % 16 == 0)
				printf("\n");
			printf(":%02x", buf[i]);
		}
		printf("\n");
	}
	else if (action == CMD_EFUSE_READ_CALI) {
		memset(buf, 0, size);
		ret = efuse_read_cali(buf, size, offset);
		if (ret == -1) {
			printf("ERROR: efuse read cali data fail!\n");
			return -1;
		}

		if (ret != size)
			printf("ERROR: read %d byte(s) not %d byte(s) data\n",
			       ret, size);
		printf("efuse read cali data");
		for (i = 0; i < size; i++) {
			if (i % 16 == 0)
				printf("\n");
			printf(":%02x", buf[i]);
		}
		printf("\n");
	}
	else if (action == CMD_EFUSE_READ_CALI_ITEM) {
		s = argv[2];
		ret = efuse_get_cali_item(s);
		if (ret < 0) {
			printf("ERROR: efuse read cali item data fail!\n");
			return -1;
		}
		printf("efuse %s cali data=0x%x\n",s,ret);
	} else if (action == CMD_EFUSE_CHECK_PATTERN_ITEM) {
		s = argv[2];
		ret = efuse_check_pattern_item(s);
		if (ret < 0) {
			printf("ERROR: efuse check pattern fail!\n");
			return -1;
		}
		printf("efuse %s %s\n", s, ret > 0 ? "has been written" : "is not write");
		return ret == 0 ? 1 : 0; //cmd return 0: written, 1: not write
	}

	/* efuse write */
	else if (action == CMD_EFUSE_WRITE) {
		if (argc < 5) {
			printf("arg count error\n");
			return CMD_RET_USAGE;
		}
		memset(buf, 0, size);

		s = argv[4];
		memcpy(buf, s, strlen(s));
		if (efuse_write_usr(buf, size, (loff_t *)&offset) < 0) {
			printf("error: efuse write fail.\n");
			return -1;
		} else {
			printf("%s written done.\n", __func__);
		}

	} else if (CMD_EFUSE_SECURE_BOOT_SET == action || CMD_EFUSE_AMLOGIC_SET == action \
				|| CMD_EFUSE_PASSWORD_SET == action || CMD_EFUSE_CUSTOMER_ID_SET == action) {
		lAddr1 = GXB_IMG_LOAD_ADDR;

		if (argc > 2)
			lAddr1 = simple_strtoul(argv[2], &end, 16);

		lAddr2 = get_sharemem_info(GET_SHARE_MEM_INPUT_BASE);
		memcpy((void *)lAddr2, (void *)lAddr1, GXB_EFUSE_PATTERN_SIZE<<1);
		flush_cache(lAddr2, GXB_EFUSE_PATTERN_SIZE<<1);

		switch (action) {
		case CMD_EFUSE_SECURE_BOOT_SET:
			nType = AML_D_P_W_EFUSE_SECURE_BOOT;
			break;
		case CMD_EFUSE_AMLOGIC_SET:
			nType = AML_D_P_W_EFUSE_AMLOGIC;
			break;
		case CMD_EFUSE_PASSWORD_SET:
			nType = AML_D_P_W_EFUSE_PASSWORD;
			break;
		case CMD_EFUSE_CUSTOMER_ID_SET:
			nType = AML_D_P_W_EFUSE_CUSTOMER_ID;
			break;
		}

		ret = aml_sec_boot_check(nType, lAddr2, GXB_EFUSE_PATTERN_SIZE<<1, 0);
		if (ret)
			printf("aml log : EFUSE pattern programming fail [%d]!\n",
			       ret);
		else
			printf("aml log : EFUSE pattern programming success!\n");

		return ret;
	} else {
		/*
		 * This part of the code is necessary to prevent unknown errors
		 */
		/* coverity[dead_error_begin] */
		printf("arg error\n");
		return CMD_RET_USAGE;
	}

	return 0;
}

int do_efuse(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char buf[EFUSE_BYTES];

	memset(buf, 0, sizeof(buf));

	if (argc < 2)
		return CMD_RET_USAGE;

	return cmd_efuse(argc, argv, buf);
}

static char efuse_help_text[] =
	"[read/write offset size [data]]\n"
	"  [cali_read]  -read from cali\n"
	"                   example: [efuse cali_read 0 7]; offset is 0,size is 7. \n"
	"  [item_read]  -read cali item\n"
	"                   [item_read sensor/saradc/mipicsi/hdmirx/eth/cvbs/earcrx/earctx]\n"
	"  [check]      -check if pattern is write\n"
	"                  [check dgpk1|dgpk2|aud_id]\n"
	"  [read/write]  - read or write 'size' data from\n"
	"                  'offset' from efuse user data ;\n"
	"  [offset]      - the offset byte from the beginning\n"
	"                  of efuse user data zone;\n"
	"  [size]        - data size\n"
	"  [data]        - the optional argument for 'write',\n"
	"                  data is treated as characters\n"
	"  examples: efuse write 0xc 0xd abcdABCD1234\n"
	"[amlogic_set addr]\n";

U_BOOT_CMD(
	efuse,  5,  1,  do_efuse,
	"efuse commands", efuse_help_text
);

#ifdef CONFIG_EFUSE_OBJ_API
static char *efuse_obj_err_parse(uint32_t  efuse_obj_err_status)
{
	char *err_char = NULL;

	switch (efuse_obj_err_status) {
	case EFUSE_OBJ_ERR_INVALID_DATA:
		err_char = "invalid data";
		break;
	case EFUSE_OBJ_ERR_NOT_FOUND:
		err_char = "field not found";
		break;
	case EFUSE_OBJ_ERR_SIZE:
		err_char = "size not match";
		break;
	case EFUSE_OBJ_ERR_NOT_SUPPORT:
		err_char = "not support";
		break;
	case EFUSE_OBJ_ERR_ACCESS:
		err_char = "access denied";
		break;
	case EFUSE_OBJ_ERR_WRITE_PROTECTED:
		err_char = "write protected";
		break;
	case EFUSE_OBJ_ERR_INTERNAL:
	case EFUSE_OBJ_ERR_OTHER_INTERNAL:
		err_char = "internal error";
		break;
	default:
		err_char = "unknown error";
		break;
		}

	return err_char;
}

int do_efuse_obj(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	char *name = NULL;

	if (argc < 3 || argc > 4) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	memset(&efuse_field, 0, sizeof(efuse_field));

	if (strcmp(argv[1], "get") == 0) {
		// $0 get field
		if (argc == 3) {
			name = argv[2];
			rc = efuse_obj_get_data(name);
			if (rc == EFUSE_OBJ_SUCCESS) {
				int i;

				for (i = 0; i < efuse_field.size; i++)
					printf("%02x%s", efuse_field.data[i],
						((i && i % 16 == 15) || (i == efuse_field.size - 1)
						? "\n" : " "));
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error getting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too many arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else if (strcmp(argv[1], "set") == 0) {
		// $0 set field data
		if (argc == 4) {
			name = argv[2];
			char *data = argv[3];

			rc = efuse_obj_set_data(name, data);
			if (rc == EFUSE_OBJ_SUCCESS) {
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error setting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too few arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else if (strcmp(argv[1], "lock") == 0) {
		// $0 lock field
		if (argc == 3) {
			name = argv[2];
			rc = efuse_obj_lock(name);
			if (rc == EFUSE_OBJ_SUCCESS) {
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error setting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too many arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else if (strcmp(argv[1], "get_lock") == 0) {
		// $0 get_lock field
		if (argc == 3) {
			name = argv[2];
			rc = efuse_obj_get_lock(name);
			if (rc == EFUSE_OBJ_SUCCESS) {
				int i;

				for (i = 0; i < efuse_field.size; i++)
					printf("%02x%s", efuse_field.data[i],
						((i && i % 16 == 15) || (i == efuse_field.size - 1)
						? "\n" : " "));
				rc = CMD_RET_SUCCESS;
			} else {
				printf("Error getting eFUSE object: %s: %d\n",
					efuse_obj_err_parse(rc), rc);
				rc = CMD_RET_FAILURE;
			}
		} else {
			printf("Error: too many arguments %d\n", argc);
			rc = CMD_RET_USAGE;
		}
	} else {
		printf("Error: cmd %s not supported\n", argv[1]);
		rc = CMD_RET_USAGE;
	}

	return rc;
}

static char efuse_obj_help_text[] =
	"[set | get | lock | get_lock] <FIELD> {<field-value-hexdump-string>}\n"
	"\n"
	"get FIELD         Get field value. FIELD is the field name\n"
	"                          expected_data is an optional expected data\n"
	"set FIELD DATA    Set field to data.  DATA is in continuous\n"
	"                          hexdump format, e.g. aabb1122\n"
	"lock FIELD        Lock field. Program exits with status 0 if success\n"
	"get_lock FIELD    Check if field is locked.  Program exits with\n"
	"                          status 1 if locked, or 0 if unlocked\n"
	"                          expected_result is 00 or 01, an optional expected lock result\n"
	"\n";

U_BOOT_CMD(efuse_obj,	4,	0,	do_efuse_obj,
	"eFUSE object program commands", efuse_obj_help_text
);
#endif /* CONFIG_EFUSE_OBJ_API */

#ifdef CONFIG_EFUSE_MRK_GET_CHECKNUM
int do_efuse_mrk(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	uint32_t rc = CMD_RET_FAILURE;
	char name[16];
	uint32_t checknum = 0;

	if (argc != 2) {
		printf("Invalid number of arguments %d\n", argc);
		return CMD_RET_USAGE;
	}

	memset(name, 0, sizeof(name));
	strncpy(name, argv[1], sizeof(name) - 1);
	if (!efuse_mrk_get_checknum(name, &checknum)) {
		printf("%s: 0x%08x\n", argv[1], checknum);
		rc = CMD_RET_SUCCESS;
	} else {
		printf("get mrk checknum for %s failed\n", argv[1]);
	}

	return rc;
}

static char efuse_mrk_help_text[] =
	"<MRK>\n"
	"Supported MRK includes: DVGK, DVUK,\n"
	"DGPK1, DGPK2,\n"
	"ETSI_SCK_0, ETSI_SCK_1, ETSI_SCK_2,\n"
	"SCPU_ETSI_SCK_0, SCPU_ETSI_SCK_1, SEGK\n";

U_BOOT_CMD(efuse_mrk,	2,	0,	do_efuse_mrk,
	"eFUSE mrk checknum", efuse_mrk_help_text
);
#endif /* CONFIG_EFUSE_MRK_GET_CHECKNUM */

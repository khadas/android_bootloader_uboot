// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "../v2_sdc_burn/optimus_sdc_burn_i.h"

typedef int __hFileHdl;

#define BURN_DBG 0
#if BURN_DBG
#define SDC_DBG(fmt...) printf(fmt)
#else
#define SDC_DBG(fmt...)
#endif//if BURN_DBG

#define SDC_MSG         DWN_MSG
#define SDC_ERR         DWN_ERR

//static char _errInfo[512] = "";
// count usb start,to reduce the time-consuming of excuting usb start
static int usb_start_count = 0;

//step 1: get script file size, and get script file contents
//step 2: read image file
//"Usage: usb_update partition image_file_path [imgFmt, verifyFile]\n"   //usage
int do_usb_update(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rcode = 0;

    const char* partName    = argv[1];
    const char* imgItemPath = argv[2];
    const char* fileFmt     = argc > 3 ? argv[3] : NULL;
    const char* verifyFile  = argc > 4 ? argv[4] : NULL;

    setenv("usb_update", "1");
    printf("usb_start_count %d\n", usb_start_count);
#if BURN_DBG
    printf("argc %d, %s, %s\n", argc, argv[0], argv[1]);
    if (argc < 3)
    {
        partName    = "system";
        imgItemPath = "rec.img";
        fileFmt     = "normal";
        verifyFile  = "recovery.verify";
    }
#else
    if (argc < 3) {
        cmd_usage(cmdtp);
        return __LINE__;
    }
#endif//#if BURN_DBG

    if (usb_start_count == 0)
    {
        //usb start to ensure usb host inserted and inited
        rcode = run_command("usb start", 0);
        usb_start_count++;
        if (rcode) {
            SDC_ERR("Fail in init usb, Does usb host not plugged in?\n");
            return __LINE__;
        }
    }

    rcode = optimus_device_probe("usb", "0");

    if (!fileFmt)
    {
        rcode = do_fat_get_file_format(imgItemPath, (u8*)OPTIMUS_DOWNLOAD_TRANSFER_BUF_ADDR, (8*1024));
        if (rcode < 0) {
            SDC_ERR("Fail when parse file format\n");
            return __LINE__;
        }
        fileFmt = rcode ? "sparse" : "normal";
    }

    rcode = optimus_burn_partition_image(partName, imgItemPath, fileFmt, verifyFile, 0);
    if (rcode) {
        SDC_ERR("Fail to burn partition (%s) with image file (%s) in format (%s)\n", partName, imgItemPath, fileFmt);
    }

    return rcode;
}

U_BOOT_CMD(
   usb_update,      //command name
   5,               //maxargs
   0,               //repeatable
   do_usb_update,   //command function
   "Burning a partition with image file in usb host",           //description
   "    argv: <part_name> <image_file_path> <[,fileFmt]> <[,verify_file]> \n"   //usage
   "    - <fileFmt> parameter is optional, if you know it you can specify it.\n"
   "        for Android, system.img and data.img is \"sparse\" format, other is \"normal\"\n"   //usage
   "    - <verify_file> parameter is optional, if you have it you can specify it.\n"
   "    - e.g. \n"
   "        to burn partition boot with boot.img of usb 0 : \"usb_update boot boot.img\"\n"   //usage
   "        to burn partition system with system.img of usb 0 : \"usb_update system system.img\"\n"   //usage
);

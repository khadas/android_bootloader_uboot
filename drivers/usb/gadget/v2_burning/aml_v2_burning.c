// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "v2_burning_i.h"
#include <mmc.h>
#include <amlogic/cpu_id.h>

#ifndef BOOT_DEVICE_USB
#define BOOT_DEVICE_SD                  4
#define BOOT_DEVICE_USB                 5
#endif// #ifndef BOOT_DEVICE_USB

extern int v2_usbburning(unsigned timeout);
extern int optimus_burn_package_in_sdmmc(const char* sdc_cfg_file);
extern void close_usb_phy_clock(int cfg);

//check ${sdcburncfg} exist in external mmc and internal flash not burned yet!
int aml_check_is_ready_for_sdc_produce(void)
{
    char* sdc_cfg_file = NULL;
    const char* cmd = NULL;
    int ret = 0;

    //if reboot_mode is MESON_SDC_BURNER_REBOOT, then this booting is skip_booted for sdc_burning
    if (/*MESON_SDC_BURNER_REBOOT != reboot_mode*/ 0) {
            /*DWN_MSG("reboot_mode=0x%x\n", (unsigned int)reboot_mode);*/
            return 1;//not ready
    }

    cmd = "mmcinfo";
    ret = run_command(cmd, 0);
    if (ret) {
        DWN_MSG("mmcinfo failed!\n");
        return 0;//not ready
    }

    sdc_cfg_file = getenv("sdcburncfg");
    if (!sdc_cfg_file) {
        sdc_cfg_file = "aml_sdc_burn.ini";
        setenv("sdcburncfg", sdc_cfg_file); }

    cmd = "fatsize mmc 0 ${sdcburncfg}";
    ret = run_command(cmd, 0);
    if (ret) {
        DWN_DBG("%s not exist\n", sdc_cfg_file);
        return 0;
    }

    return 1;//is ready for sdcard producing
}

#ifdef CONFIG_AML_V3_FACTORY_BURN
extern unsigned _get_romcode_boot_id(void);
#else
static unsigned _get_romcode_boot_id(void)
{
    unsigned boot_id = 0;
#ifdef SYSCTRL_SEC_STATUS_REG2
    cpu_id_t cpuid = get_cpu_id();
	if (MESON_CPU_MAJOR_ID_SC2 <= cpuid.family_id) {
		boot_id = readl(SYSCTRL_SEC_STATUS_REG2);
        DWN_DBG("boot_id 0x%x\n", boot_id);
		boot_id = (boot_id>>4) & 0xf;
	} else
#elif defined(P_AO_SEC_GP_CFG0)
    {
		DWN_DBG("cfg0 0x%08x\n", readl(P_AO_SEC_GP_CFG0));
		boot_id = readl(P_AO_SEC_GP_CFG0) & 0xf;
	}
#else
    DWN_MSG("both P_AO_SEC_GP_CFG0 and SYSCTRL_SEC_STATUS_REG2 undefined\n");
#endif//#ifdef SYSCTRL_SEC_STATUS_REG2
    DWN_MSG("boot_id 1x%x\n", boot_id);
    return boot_id;
}
#endif

//is the uboot loaded from usb otg
int is_tpl_loaded_from_usb(void)
{
        const int boot_id  = _get_romcode_boot_id();
        return (BOOT_DEVICE_USB == boot_id);
}

//is the uboot loaded from sdcard mmc 0
//note only sdmmc supported by romcode when external device boot
int is_tpl_loaded_from_ext_sdmmc(void)
{
    return (BOOT_DEVICE_SD == _get_romcode_boot_id());
}

//Check if uboot loaded from external sdmmc or usb otg
int aml_burn_check_uboot_loaded_for_burn(int flag)
{
    int usb_boot = is_tpl_loaded_from_usb();
    int sdc_boot = is_tpl_loaded_from_ext_sdmmc();

    return usb_boot || sdc_boot;
}

//1, is booted from external sdmmc: check if aml_sdc_burn.ini existed
//2, if loaded from usb, ready for burn
int aml_burn_check_is_ready_for_burn(int flag, bd_t* bis)
{
        if (is_tpl_loaded_from_usb()) {
                return 1;
        }

        if (is_tpl_loaded_from_ext_sdmmc())
        {
                return aml_check_is_ready_for_sdc_produce();
        }

        return 0;
}

int aml_burn_sdc_producing(int flag, bd_t* bis)
{
    optimus_work_mode_set(OPTIMUS_WORK_MODE_SDC_PRODUCE);

    return optimus_burn_package_in_sdmmc(getenv("sdcburncfg"));
}

#ifdef CONFIG_USB_BURNING_TOOL
//producing mode means boot from raw flash, i.e, uboot is loaded from usb
static int aml_burn_usb_producing(int flag, bd_t* bis)
{
    flag = flag; bis = bis;//avoid compile warning

    set_usb_boot_function(CLEAR_USB_BOOT);
#if (defined AML_USB_BURN_TOOL)
    optimus_work_mode_set(OPTIMUS_WORK_MODE_USB_PRODUCE);
#endif//#if (defined AML_USB_BURN_TOOL)

    close_usb_phy_clock(0);//disconnect before re-connect to enhance pc compatibility
    optimus_clear_ovd_register();//clear OVD register for normal reboot
    return v2_usbburning(20000);
}

extern void serial_initialize(void);
int aml_try_factory_usb_burning(int flag, bd_t* bis)
{
    if (!is_tpl_loaded_from_usb()) return 1;

    if ( !flag ) {
        serial_initialize();//init for write memory
#ifdef CONFIG_MMC
        DWN_MSG("MMC init in usb\n");
        mmc_initialize(bis);
#endif
    }
    return aml_burn_usb_producing(flag, bis);
}
#endif// #ifdef CONFIG_USB_BURNING_TOOL

//burning flash from romboot stage
int aml_burn_factory_producing(int flag, bd_t* bis)
{
#ifdef CONFIG_USB_BURNING_TOOL
        if (is_tpl_loaded_from_usb())
        {
                return aml_burn_usb_producing(flag, bis);
        }
#endif// #ifdef CONFIG_USB_BURNING_TOOL

        if (is_tpl_loaded_from_ext_sdmmc())
        {
                return aml_burn_sdc_producing(flag, bis);
        }

        DWN_ERR("Shouldnot reach here!\n");
        return 0;
}


int aml_try_factory_sdcard_burning(int flag, bd_t* bis)
{
        if (!is_tpl_loaded_from_ext_sdmmc()) return 1;
#ifdef CONFIG_SILENT_CONSOLE
        /* enable console output */
        gd->flags &= ~GD_FLG_SILENT;
#endif
	optimus_clear_ovd_register();
        if ( aml_check_is_ready_for_sdc_produce() )
        {
            return aml_burn_sdc_producing(flag, bis);
        }

        return 0;
}


# SPDX-License-Identifier: GPL-2.0+
#

obj-$(CONFIG_AML_VPU) += cmd_vpu.o
obj-$(CONFIG_AML_VPP) += cmd_vpp.o
obj-$(CONFIG_AML_VOUT) += cmd_vout.o
obj-$(CONFIG_AML_CVBS) += cmd_cvbs.o
obj-$(CONFIG_AML_HDMITX20) += cmd_hdmitx20.o
obj-$(CONFIG_AML_HDMITX21) += cmd_hdmitx21.o
obj-$(CONFIG_AML_LCD) += cmd_lcd.o
obj-$(CONFIG_AML_OSD) += cmd_osd.o
obj-$(CONFIG_AML_DOLBY) += cmd_dolbyvision.o
obj-$(CONFIG_CMD_PLLTEST) += cmd_plltest.o

obj-$(CONFIG_AML_STORE_WRAPPER) += store_wrapper.o
obj-$(CONFIG_AML_IMG_READ) += imgread.o
obj-$(CONFIG_AML_DEFENV) += defenv_without.o
obj-$(CONFIG_AML_ROBUST) += bootloader_status.o

obj-$(CONFIG_AML_REBOOT) += cmd_reboot.o
obj-$(CONFIG_CMD_RSVMEM) += cmd_rsvmem.o

obj-y += cmd_version.o
obj-y += cmd_delay.o

obj-$(CONFIG_CMD_DMCRW) += cmd_dmc.o

obj-$(CONFIG_CMD_CAR_PARAMS) += cmd_car_param.o
obj-$(CONFIG_CMD_CAR_PARAMS) += rvc_interface.o
obj-$(CONFIG_NOVERBOSE_BUILD) += cmd_bl30_print_set.o

ifdef CONFIG_BOOTLOADER_CONTROL_BLOCK
obj-y += cmd_bcb.o
obj-y += get_bcb_boot_part.o
obj-y += cmd_dynamic.o
obj-y += cmd_bootctl_utils.o
ifdef CONFIG_UNIFY_BOOTLOADER
obj-y += cmd_bootctl_vab.o
obj-y += cmd_bootctl_avb.o
obj-y += cmd_bootctl.o
obj-y += cmd_bootctl_wrapper.o
else
ifdef CONFIG_CMD_BOOTCTOL_AVB
ifdef CONFIG_CMD_BOOTCTOL_VAB
obj-y += cmd_bootctl_vab.o
else
obj-y += cmd_bootctl_avb.o
endif
else
obj-y += cmd_bootctl.o
endif
endif
endif
obj-$(CONFIG_AML_RPMB) += cmd_rpmb.o
obj-$(CONFIG_CMD_BOOTCTOL_AVB) += cmd_avb.o
obj-$(CONFIG_CMD_DDR_D2PLL) += cmd_d2pll.o

ifdef CONFIG_NASC_NAGRA_TIER_1
else

ifdef CONFIG_MESON_P1
obj-y += cmd_ddr_test_p1.o
else

ifdef CONFIG_MESON_S5
obj-y += cmd_ddr_test_s5.o
else
ifdef CONFIG_MESON_T3X
obj-y += cmd_ddr_test_t3x.o
else
ifdef CONFIG_MESON_A4
obj-y += cmd_ddr_test_t3x.o
else
ifdef CONFIG_MESON_T5M
obj-y += cmd_ddr_test_t5m.o
else
ifdef CONFIG_MESON_TXHD2
obj-y += cmd_ddr_test_txhd2.o
else
obj-y += cmd_ddr_test_g12.o
endif
endif
endif
endif
endif
endif

endif

obj-$(CONFIG_CMD_OS_IDENT) += os_ident.o
obj-$(CONFIG_AML_FACTORY_PROVISION) += cmd_factory_provision.o
obj-$(CONFIG_AML_WATERMARK) += cmd_watermark.o
obj-$(CONFIG_SECURE_POWER_CONTROL) += cmd_power.o
obj-$(CONFIG_CMD_HDMIRX) += cmd_hdmirx.o
obj-$(CONFIG_CMD_CEC) += cmd_cec.o
obj-$(CONFIG_CMD_PMU) += cmd_pmic.o
obj-$(CONFIG_CMD_RMA) += cmd_rma.o
# khadas commands
obj-$(CONFIG_KHADAS_KBI) += cmd_kbi.o

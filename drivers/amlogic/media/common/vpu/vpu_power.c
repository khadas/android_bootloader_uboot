// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <config.h>
#include <linux/kernel.h>
#include <linux/arm-smccc.h>
#include <amlogic/media/vpu/vpu.h>
#include "vpu_reg.h"
#include "vpu.h"

#ifdef CONFIG_AMLOGIC_TEE
//flag:(forward compatible)
// 0=set vpu sec without debug print
// 1=set vpu sec with after debug print
// 2=set vpu sec with before and after debug print
// 3=only debug print
unsigned long viu_init_psci_smc(unsigned long flag)
{
	struct arm_smccc_res res;

	VPUPR("%s\n", __func__);
	arm_smccc_smc(0x82000080, flag, 0, 0,
		      0, 0, 0, 0, &res);
	return res.a0;
}
#endif

void vpu_mem_pd_init_off(void)
{
	return;
#ifdef VPU_DEBUG_PRINT
	VPUPR("%s\n", __func__);
#endif
}

void vpu_module_init_config(void)
{
	struct vpu_ctrl_s *ctrl_table;
	unsigned int _reg, _val, _bit, _len;
	int i = 0, cnt;

	/* vpu clk gate init off */
	cnt = vpu_conf.data->module_init_table_cnt;
	ctrl_table = vpu_conf.data->module_init_table;
	if (ctrl_table) {
		while (i < cnt) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_val = ctrl_table[i].val;
			_bit = ctrl_table[i].bit;
			_len = ctrl_table[i].len;
			vpu_vcbus_setb(_reg, _val, _bit, _len);
			i++;
		}
	}
	vpu_hiu_setb(vpu_conf.data->vid_clk_reg, 0, 0, 8);

	/* dmc_arb_config */
	if (vpu_conf.data->chip_type < VPU_CHIP_T3) {
		vpu_vcbus_write(VPU_RDARB_MODE_L1C1, 0x0); //0x210000
		vpu_vcbus_write(VPU_RDARB_MODE_L1C2, 0x10000);
		vpu_vcbus_write(VPU_WRARB_MODE_L2C1, 0x20000);
	}
	if (vpu_conf.data->chip_type == VPU_CHIP_T5M) {
		vpu_vcbus_write(VPU_RDARB_MODE_L2C1, 0x0);
		vpu_vcbus_write(VPU_WRARB_MODE_L2C1, 0x20000);
	} else if (vpu_conf.data->chip_type == VPU_CHIP_T3X) {
		//bit[27:26]=0, bit[23:22]=0, tcon read p1 & p3 on arb0, default value
		vpu_vcbus_write(VPU_RDARB_MODE_L2C1, 0x100000);
		//bit[20]=0, bit[19]=0, tcon write p1 & p3 on arb0
		vpu_vcbus_write(VPU_WRARB_MODE_L2C1, 0x20000); //default 0x20000

		//[15:14]=3, bit[13:12]=3, bit[11:10]=3, tcon p1/p2/p3 read urgent
		vpu_vcbus_write(VPU_RDARB_UGT_L2C1, 0xfc00); //default 0x0
		//[9:8]=3, bit[7:6]=3, tcon p1 & p3 write urgent
		vpu_vcbus_write(VPU_WRARB_UGT_L2C1, 0x00f003c0); //default 0x00f00000

		vpu_vcbus_write(DI_WRARB_UGT_L1C1, 0x1154);//di write urgent
	} else if (vpu_conf.data->chip_type == VPU_CHIP_T7 ||
		vpu_conf.data->vpu_read_type == ONLY_READ0) {
		init_arb_urgent_table();
	} else {
		vpu_vcbus_write(VPU_RDARB_MODE_L2C1, 0x900000);
		vpu_vcbus_write(VPU_WRARB_MODE_L2C1, 0x20000);
	}

#ifdef CONFIG_AMLOGIC_TEE
	if (vpu_conf.data->chip_type == VPU_CHIP_T3 ||
	    vpu_conf.data->chip_type == VPU_CHIP_T5W ||
		vpu_conf.data->chip_type == VPU_CHIP_T5M ||
		vpu_conf.data->chip_type == VPU_CHIP_T3X)
		viu_init_psci_smc(0);
#endif
	/* S5 new add registers */
	if (vpu_conf.data->chip_type == VPU_CHIP_S5)
		vpu_sysctrl_write(SYSCTRL_SYS_CLK_VPU_EN, 0xFFFFFFFF);
	VPUPR("%s\n", __func__);
}

void vpu_power_on(void)
{
	struct vpu_ctrl_s *ctrl_table;
	struct vpu_reset_s *reset_table;
	unsigned int _reg, _val, _start, _end, _len, mask;
	int i, j;

	/* power on VPU_HDMI */
	ctrl_table = vpu_conf.data->power_table;
	if (ctrl_table) {
		i = 0;
		while (i < VPU_PWR_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_val = 0;
			_start = ctrl_table[i].bit;
			_len = ctrl_table[i].len;
			vpu_ao_setb(_reg, _val, _start, _len);
			i++;
		}
	}
	udelay(20);

	/* power up memories */
	ctrl_table = vpu_conf.data->mem_pd_table;
	if (ctrl_table) {
		i = 0;
		while (i < VPU_MEM_PD_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_start = ctrl_table[i].bit;
			_end = ctrl_table[i].len + ctrl_table[i].bit;
			for (j = _start; j < _end; j += 2) {
				vpu_hiu_setb(_reg, 0, j, 2);
				udelay(5);
			}
			i++;
		}
		for (i = 8; i < 16; i++) {
			vpu_hiu_setb(HHI_MEM_PD_REG0, 0, i, 1);
			udelay(5);
		}
		udelay(20);
	}

	/* Reset VIU + VENC */
	/* Reset VENCI + VENCP + VADC + VENCL */
	/* Reset HDMI-APB + HDMI-SYS + HDMI-TX + HDMI-CEC */
	reset_table = vpu_conf.data->reset_table;
	if (reset_table) {
		i = 0;
		while (i < VPU_RESET_CNT_MAX) {
			if (reset_table[i].reg == VPU_REG_END)
				break;
			_reg = reset_table[i].reg;
			mask = reset_table[i].mask;
			vpu_cbus_clr_mask(_reg, mask);
			i++;
		}
		udelay(5);
		/* release Reset */
		i = 0;
		while (i < VPU_RESET_CNT_MAX) {
			if (reset_table[i].reg == VPU_REG_END)
				break;
			_reg = reset_table[i].reg;
			mask = reset_table[i].mask;
			vpu_cbus_set_mask(_reg, mask);
			i++;
		}
	}

	/* Remove VPU_HDMI ISO */
	ctrl_table = vpu_conf.data->iso_table;
	if (ctrl_table) {
		i = 0;
		while (i < VPU_ISO_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_val = 0;
			_start = ctrl_table[i].bit;
			_len = ctrl_table[i].len;
			vpu_ao_setb(_reg, _val, _start, _len);
			i++;
		}
	}

	VPUPR("%s\n", __func__);
}

void vpu_power_off(void)
{
	struct vpu_ctrl_s *ctrl_table;
	unsigned int _reg, _start, _end, _len, _val;
	int i, j;

	/* Enable Isolation */
	ctrl_table = vpu_conf.data->iso_table;
	if (ctrl_table) {
		i = 0;
		while (i < VPU_ISO_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_val = 1;
			_start = ctrl_table[i].bit;
			_len = ctrl_table[i].len;
			vpu_ao_setb(_reg, _val, _start, _len);
			i++;
		}
		udelay(20);
	}

	/* power down memories */
	ctrl_table = vpu_conf.data->mem_pd_table;
	if (ctrl_table) {
		i = 0;
		while (i < VPU_MEM_PD_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_start = ctrl_table[i].bit;
			_end = ctrl_table[i].len + ctrl_table[i].bit;
			for (j = _start; j < _end; j += 2) {
				vpu_hiu_setb(_reg, 0x3, j, 2);
				udelay(5);
			}
			i++;
		}
		for (i = 8; i < 16; i++) {
			vpu_hiu_setb(HHI_MEM_PD_REG0, 0x1, i, 1);
			udelay(5);
		}
		udelay(20);
	}

	/* Power down VPU domain */
	ctrl_table = vpu_conf.data->power_table;
	if (ctrl_table) {
		i = 0;
		while (i < VPU_PWR_CNT_MAX) {
			if (ctrl_table[i].reg == VPU_REG_END)
				break;
			_reg = ctrl_table[i].reg;
			_val = 1;
			_start = ctrl_table[i].bit;
			_len = ctrl_table[i].len;
			vpu_ao_setb(_reg, _val, _start, _len);
			i++;
		}
	}

	vpu_hiu_setb(vpu_conf.data->vapb_clk_reg, 0, 8, 1);
	vpu_hiu_setb(vpu_conf.data->vpu_clk_reg, 0, 8, 1);

	VPUPR("%s\n", __func__);
}

void vpu_power_on_new(void)
{
#ifdef CONFIG_SECURE_POWER_CONTROL
	unsigned int pwr_id;
	int i = 0;

	if (!vpu_conf.data->pwrctrl_id_table)
		return;

	while (i < VPU_PWR_ID_MAX) {
		pwr_id = vpu_conf.data->pwrctrl_id_table[i];
		if (pwr_id == VPU_PWR_ID_END)
			break;
#ifdef VPU_DEBUG_PRINT
		VPUPR("%s: pwr_id=%d\n", __func__, pwr_id);
#endif
		pwr_ctrl_psci_smc(pwr_id, 1);
		i++;
	}
	VPUPR("%s\n", __func__);
#else
	VPUERR("%s: no CONFIG_SECURE_POWER_CONTROL\n", __func__);
#endif
}

void vpu_power_off_new(void)
{
#ifdef CONFIG_SECURE_POWER_CONTROL
	unsigned int pwr_id;
	int i = 0;

	VPUPR("%s\n", __func__);
	if (!vpu_conf.data->pwrctrl_id_table)
		return;

	while (i < VPU_PWR_ID_MAX) {
		pwr_id = vpu_conf.data->pwrctrl_id_table[i];
		if (pwr_id == VPU_PWR_ID_END)
			break;
#ifdef VPU_DEBUG_PRINT
		VPUPR("%s: pwr_id=%d\n", __func__, pwr_id);
#endif
		pwr_ctrl_psci_smc(pwr_id, 0);
		i++;
	}
#else
	VPUERR("%s: no CONFIG_SECURE_POWER_CONTROL\n", __func__);
#endif

	vpu_hiu_setb(vpu_conf.data->vapb_clk_reg, 0, 8, 1);
	vpu_hiu_setb(vpu_conf.data->vpu_clk_reg, 0, 8, 1);
}

void vpu_power_off_c3(void)
{
	vpu_hiu_setb(vpu_conf.data->vpu_clk_reg, 0, 8, 1);
}

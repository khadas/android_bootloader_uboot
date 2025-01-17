// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <amlogic/cpu_id.h>
#include <fdtdec.h>
#ifdef CONFIG_SECURE_POWER_CONTROL
#include <asm/arch/pwr_ctrl.h>
#endif
#include <amlogic/keyunify.h>
#include <amlogic/media/vout/aml_vout.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#ifdef CONFIG_AML_LCD_EXTERN
#include <amlogic/media/vout/lcd/lcd_extern.h>
#endif
#include "lcd_reg.h"
#include "lcd_common.h"
#include <amlogic/pm.h>

//DECLARE_GLOBAL_DATA_PTR;

unsigned int lcd_debug_print_flag;
static unsigned int lcd_debug_test_flag;
struct aml_lcd_data_s *lcd_data;
static struct aml_lcd_drv_s *lcd_driver[LCD_MAX_DRV];
static struct lcd_debug_ctrl_s debug_ctrl;
static char *g_dt_addr = (char *)0x01000000;
//static int lcd_poweron_suspend = 1;
char *lcd_pm_name[LCD_MAX_DRV] = {"lcd_drv0_pm", "lcd_drv1_pm", "lcd_drv2_pm"};

static void lcd_update_ctrl_bootargs(struct aml_lcd_drv_s *pdrv);

static struct aml_lcd_data_s lcd_data_g12a = {
	.chip_type = LCD_CHIP_G12A,
	.chip_name = "g12a",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_g12b = {
	.chip_type = LCD_CHIP_G12B,
	.chip_name = "g12b",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_tl1 = {
	.chip_type = LCD_CHIP_TL1,
	.chip_name = "tl1",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_sm1 = {
	.chip_type = LCD_CHIP_SM1,
	.chip_name = "sm1",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_tm2 = {
	.chip_type = LCD_CHIP_TM2,
	.chip_name = "tm2",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t5 = {
	.chip_type = LCD_CHIP_T5,
	.chip_name = "t5",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t5d = {
	.chip_type = LCD_CHIP_T5D,
	.chip_name = "t5d",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t5w = {
	.chip_type = LCD_CHIP_T5W,
	.chip_name = "t5w",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t7 = {
	.chip_type = LCD_CHIP_T7,
	.chip_name = "t7",
	.rev_type = 0,
	.drv_max = 3,
	.offset_venc = {0x0, (0x600 << 2), (0x800 << 2)},
	.offset_venc_if = {0x0, (0x500 << 2), (0x600 << 2)},
	.offset_venc_data = {0x0, (0x100 << 2), (0x200 << 2)},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t3 = {
	.chip_type = LCD_CHIP_T3,
	.chip_name = "t3",
	.rev_type = 0,
	.drv_max = 2,
	.offset_venc = {0x0, (0x600 << 2), 0},
	.offset_venc_if = {0x0, (0x500 << 2), 0},
	.offset_venc_data = {0x0, (0x100 << 2), 0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_c3 = {
	.chip_type = LCD_CHIP_C3,
	.chip_name = "c3",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t5m = {
	.chip_type = LCD_CHIP_T5M,
	.chip_name = "t5m",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0x0},
	.offset_venc_if = {0x0},
	.offset_venc_data = {0x0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_t3x = {
	.chip_type = LCD_CHIP_T3X,
	.chip_name = "t3x",
	.rev_type = 0,
	.drv_max = 2,
	.offset_venc = {0x0, (0x100 << 2), 0},
	.offset_venc_if = {0x0, (0x500 << 2), 0},
	.offset_venc_data = {0x0, (0x100 << 2), 0},
};

static __maybe_unused struct aml_lcd_data_s lcd_data_a4 = {
	.chip_type = LCD_CHIP_A4,
	.chip_name = "a4",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0},
	.offset_venc_if = {0},
	.offset_venc_data = {0},
	.dft_conf = {NULL, NULL, NULL},
};

static struct aml_lcd_data_s lcd_data_txhd2 = {
	.chip_type = LCD_CHIP_TXHD2,
	.chip_name = "txhd2",
	.rev_type = 0,
	.drv_max = 1,
	.offset_venc = {0x0},
	.offset_venc_if = {0x0},
	.offset_venc_data = {0x0},
	.dft_conf = {NULL, NULL, NULL},
};

static void lcd_chip_detect(void)
{
#if 1
	unsigned int cpu_type;
	unsigned int rev_type;

	cpu_type = get_cpu_id().family_id;
	rev_type = get_cpu_id().chip_rev;
	switch (cpu_type) {
	case MESON_CPU_MAJOR_ID_G12A:
		lcd_data = &lcd_data_g12a;
		break;
	case MESON_CPU_MAJOR_ID_G12B:
		lcd_data = &lcd_data_g12b;
		break;
	case MESON_CPU_MAJOR_ID_TL1:
		lcd_data = &lcd_data_tl1;
		break;
	case MESON_CPU_MAJOR_ID_SM1:
		lcd_data = &lcd_data_sm1;
		break;
	case MESON_CPU_MAJOR_ID_TM2:
		lcd_data = &lcd_data_tm2;
		break;
	case MESON_CPU_MAJOR_ID_T5:
		lcd_data = &lcd_data_t5;
		break;
	case MESON_CPU_MAJOR_ID_T5D:
		lcd_data = &lcd_data_t5d;
		break;
	case MESON_CPU_MAJOR_ID_T5W:
		lcd_data = &lcd_data_t5w;
		break;
	case MESON_CPU_MAJOR_ID_T7:
		lcd_data = &lcd_data_t7;
		break;
	case MESON_CPU_MAJOR_ID_T3:
		lcd_data = &lcd_data_t3;
		break;
	case MESON_CPU_MAJOR_ID_C3:
		lcd_data = &lcd_data_c3;
		break;
	case MESON_CPU_MAJOR_ID_T5M:
		lcd_data = &lcd_data_t5m;
		break;
	case MESON_CPU_MAJOR_ID_T3X:
		lcd_data = &lcd_data_t3x;
		break;
	case MESON_CPU_MAJOR_ID_A4:
		lcd_data = &lcd_data_a4;
		break;
	case MESON_CPU_MAJOR_ID_TXHD2:
		lcd_data = &lcd_data_txhd2;
		break;
	default:
		lcd_data = NULL;
		return;
	}
	lcd_data->rev_type = rev_type;
#else
	lcd_data = &lcd_data_txhd2;
#endif
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("check chip: %d %s\n",
			lcd_data->chip_type, lcd_data->chip_name);
	}
}

struct aml_lcd_data_s *aml_lcd_get_data(void)
{
	return lcd_data;
}

static struct aml_lcd_drv_s *lcd_driver_check_valid(int index)
{
	if (index >= LCD_MAX_DRV)
		return NULL;

	if (!lcd_driver[index] || !lcd_driver[index]->probe_done) {
		LCDERR("invalid lcd%d config\n", index);
		return NULL;
	}
	return lcd_driver[index];
}

struct aml_lcd_drv_s *aml_lcd_get_driver(int index)
{
	return lcd_driver_check_valid(index);
}

static void lcd_power_ctrl(struct aml_lcd_drv_s *pdrv, int status)
{
	struct lcd_power_ctrl_s *lcd_power;
	struct lcd_power_step_s *power_step;
	char *str;
	unsigned int i, wait, gpio;
	int value = LCD_PMU_GPIO_NUM_MAX;
#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_driver_s *edrv;
	struct lcd_extern_dev_s *edev;
#endif

#ifdef CONFIG_AML_LCD_PXP
	LCDPR("[%d]: %s: lcd_pxp bypass\n", pdrv->index, __func__);
	return;
#endif

	i = 0;
	lcd_power = &pdrv->config.power;
	if (status) {
		/* check if factory test */
		if (pdrv->factory_lcd_power_on_step) {
			LCDPR("[%d]: %s: factory test power_on_step!\n",
			      pdrv->index, __func__);
			power_step = pdrv->factory_lcd_power_on_step;
		} else {
			power_step = &lcd_power->power_on_step[0];
		}
	} else {
		power_step = &lcd_power->power_off_step[0];
	}

	while (i < LCD_PWR_STEP_MAX) {
		if (power_step->type >= LCD_POWER_TYPE_MAX)
			break;
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
			LCDPR("[%d]: power_ctrl: %d, step %d: type=%d, index=%d, value=%d, delay=%d\n",
			      pdrv->index, status, i,
			      power_step->type, power_step->index,
			      power_step->value, power_step->delay);
		}
		switch (power_step->type) {
		case LCD_POWER_TYPE_CPU:
			if (power_step->index < LCD_CPU_GPIO_NUM_MAX) {
				str = lcd_power->cpu_gpio[power_step->index];
				gpio = lcd_gpio_name_map_num(str);
				lcd_gpio_set(gpio, power_step->value);
			} else {
				LCDERR("[%d]: invalid cpu_gpio index: %d\n",
				       pdrv->index, power_step->index);
			}
			break;
		case LCD_POWER_TYPE_PMU:
			if (power_step->index < LCD_PMU_GPIO_NUM_MAX)
				LCDPR("to do\n");
			else
				LCDERR("pmu_gpio invalid index: %d\n",
				       power_step->index);
			break;
		case LCD_POWER_TYPE_SIGNAL:
			if (status)
				pdrv->driver_init(pdrv);
			else
				pdrv->driver_disable(pdrv);
			break;
#ifdef CONFIG_AML_LCD_EXTERN
		case LCD_POWER_TYPE_EXTERN:
			edrv = lcd_extern_get_driver(pdrv->index);
			edev = lcd_extern_get_dev(edrv, power_step->index);
			if (!edrv || !edev) {
				LCDERR("no ext_dev\n");
				break;
			}
			if (status) {
				if (edev->power_on)
					edev->power_on(edrv, edev);
				else
					LCDERR("no ext power on\n");
			} else {
				if (edev->power_off)
					edev->power_off(edrv, edev);
				else
					LCDERR("no ext power off\n");
			}
			break;
#endif
		case LCD_POWER_TYPE_WAIT_GPIO:
			if (power_step->index < LCD_CPU_GPIO_NUM_MAX) {
				str = lcd_power->cpu_gpio[power_step->index];
				gpio = lcd_gpio_name_map_num(str);
				lcd_gpio_set(gpio, LCD_GPIO_INPUT);
			} else {
				LCDERR("[%d]: wait_gpio index: %d\n",
				       pdrv->index, power_step->index);
				break;
			}
			LCDPR("[%d]: lcd_power_type_wait_gpio wait\n", pdrv->index);
			for (wait = 0; wait < power_step->delay; wait++) {
				value = lcd_gpio_input_get(gpio);
				if (value == power_step->value) {
					LCDPR("[%d]: get value: %d, wait ok\n",
					      pdrv->index, value);
					break;
				}
				mdelay(1);
			}
			if (wait == power_step->delay) {
				LCDERR("[%d]: get value: %d, wait timeout!\n",
				       pdrv->index, value);
			}
			break;
		case LCD_POWER_TYPE_CLK_SS:
			break;
#ifdef CONFIG_AML_LCD_TCON
		case LCD_POWER_TYPE_TCON_SPI_DATA_LOAD:
			if (!pdrv->tcon_spi_data_load) {
				LCDERR("[%d]: %s: tcon_spi_data_load is null\n",
				       pdrv->index, __func__);
				break;
			}
			pdrv->tcon_spi_data_load();
			break;
#endif
		default:
			break;
		}

		if (power_step->type != LCD_POWER_TYPE_WAIT_GPIO) {
#ifndef CONFIG_AML_LCD_PXP
			if (power_step->delay > 0)
				mdelay(power_step->delay);
#endif
		}
		i++;
		power_step++;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("[%d]: %s: %d\n", pdrv->index, __func__, status);
}

void lcd_encl_on(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	if (pdrv->config_check_en == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("[%d]: config_check disabled\n", pdrv->index);
	} else {
		ret = lcd_config_timing_check(pdrv, &pdrv->config.timing.act_timing);
		if (ret & 0x55) {
			LCDERR("[%d]: %s: config timing check fatal error!\n",
				pdrv->index, __func__);
			return;
		}
	}

	pdrv->driver_init_pre(pdrv);
	if (lcd_debug_test_flag)
		lcd_debug_test(pdrv, lcd_debug_test_flag);

	pdrv->status |= LCD_STATUS_ENCL_ON;
}

static void lcd_interface_on(struct aml_lcd_drv_s *pdrv)
{
	lcd_power_ctrl(pdrv, 1);
#ifndef CONFIG_AML_LCD_PXP
	pdrv->config.retry_enable_cnt = 0;
	while (pdrv->config.retry_enable_flag) {
		if (pdrv->config.retry_enable_cnt++ >= LCD_ENABLE_RETRY_MAX)
			break;
		LCDPR("[%d]: retry enable...%d\n",
		      pdrv->index, pdrv->config.retry_enable_cnt);
		lcd_power_ctrl(pdrv, 0);
		mdelay(1000);
		lcd_power_ctrl(pdrv, 1);
	}
	pdrv->config.retry_enable_cnt = 0;
#endif
	pdrv->status |= LCD_STATUS_IF_ON;
}

__maybe_unused static void lcd_update_outputmode(struct aml_lcd_drv_s *pdrv)
{
	char curr_mode[20];
	char mode_env_name[20] = "outputmode\0\0";
	unsigned short framerate, dur_index;

	if (!pdrv->vmode_mgr.cur_vmode_info) {
		LCDERR("[%d]: curr vmode info is NULL\n", pdrv->index);
		return;
	}

	dur_index = pdrv->vmode_mgr.cur_vmode_info->duration_index;
	if (dur_index != 0xff)
		framerate = pdrv->std_duration[dur_index].frame_rate;
	else
		framerate = pdrv->vmode_mgr.cur_vmode_info->base_fr;

	memset(curr_mode, 0, 20 * sizeof(char));
	str_add_vmode(curr_mode, pdrv->vmode_mgr.cur_vmode_info, framerate);

	if (pdrv->viu_sel == 2)
		mode_env_name[10] = '2';
	else if (pdrv->viu_sel == 3)
		mode_env_name[10] = '3';

	env_set(mode_env_name, curr_mode);
}

static void lcd_module_enable(struct aml_lcd_drv_s *pdrv, char *mode)
{
	unsigned int sync_duration;
	struct lcd_config_s *pconf;
	int ret;

	pconf = &pdrv->config;
	ret = pdrv->config_valid(pdrv, mode);
	if (ret) {
		LCDERR("[%d]: init exit\n", pdrv->index);
		return;
	}

	sync_duration = pconf->timing.act_timing.sync_duration_num;
	sync_duration = (sync_duration * 100) / pconf->timing.act_timing.sync_duration_den;
	LCDPR("[%d]: enable: %s, %s, %ux%u@%u.%02uHz\n",
	      pdrv->index, pconf->basic.model_name,
	      lcd_type_type_to_str(pconf->basic.lcd_type),
	      pconf->timing.act_timing.h_active,
	      pconf->timing.act_timing.v_active,
	      (sync_duration / 100), (sync_duration % 100));

	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0)
		lcd_encl_on(pdrv);
	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0) {
		LCDERR("[%d]: %s: encl_on failed!\n", pdrv->index, __func__);
		return;
	}
	if ((pdrv->status & LCD_STATUS_IF_ON) == 0) {
#ifdef CONFIG_AML_LCD_TABLET
		if (unlikely(pdrv->mode == LCD_MODE_TABLET &&
			     pdrv->config.control.mipi_cfg.panel_det_attr & 0x01))
			lcd_power_ctrl(pdrv, 1);
#endif
		if (pdrv->boot_ctrl.init_level == LCD_INIT_LEVEL_NORMAL) {
			lcd_interface_on(pdrv);
#ifdef CONFIG_AML_LCD_BACKLIGHT
#ifndef CONFIG_AML_LCD_PXP
			aml_bl_driver_enable(pdrv->index);
#endif
#endif
		} else {
			LCDPR("[%d]: bypass interface for init_level %d\n",
			      pdrv->index, pdrv->boot_ctrl.init_level);
		}
	}

	lcd_update_ctrl_bootargs(pdrv);
}

static void lcd_module_disable(struct aml_lcd_drv_s *pdrv)
{
	LCDPR("[%d]: disable: %s\n", pdrv->index, pdrv->config.basic.model_name);

	if (pdrv->status & LCD_STATUS_IF_ON) {
#ifdef CONFIG_AML_LCD_BACKLIGHT
#ifndef CONFIG_AML_LCD_PXP
		aml_bl_driver_disable(pdrv->index);
#endif
#endif
		lcd_power_ctrl(pdrv, 0);
	}

	lcd_venc_enable(pdrv, 0);
	lcd_disable_clk(pdrv);
	pdrv->status = 0;
}

static void lcd_module_prepare(struct aml_lcd_drv_s *pdrv, char *mode)
{
	int ret;

	if (!mode)
		return;

	ret = pdrv->config_valid(pdrv, mode);
	if (ret) {
		LCDERR("[%d]: prepare exit\n", pdrv->index);
		return;
	}

	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0)
		lcd_encl_on(pdrv);

	lcd_update_ctrl_bootargs(pdrv);
}

static int lcd_mode_init(struct aml_lcd_drv_s *pdrv)
{
	int ret = -1;

	if (debug_ctrl.debug_lcd_mode == 1) {
		LCDPR("[%d]: lcd_debug_mode: 1,tv\n", pdrv->index);
		pdrv->mode = LCD_MODE_TV;
	} else if (debug_ctrl.debug_lcd_mode == 2) {
		LCDPR("[%d]: lcd_debug_mode: 2,tablet\n", pdrv->index);
		pdrv->mode = LCD_MODE_TABLET;
	}

	switch (pdrv->mode) {
#ifdef CONFIG_AML_LCD_TV
	case LCD_MODE_TV:
		ret = lcd_mode_tv_init(pdrv);
		break;
#endif
#ifdef CONFIG_AML_LCD_TABLET
	case LCD_MODE_TABLET:
		ret = lcd_mode_tablet_init(pdrv);
		break;
#endif
	default:
		LCDERR("[%d]: invalid lcd mode: %d\n", pdrv->index, pdrv->mode);
		break;
	}

	if (ret) {
		pdrv->probe_done = 0;
		LCDERR("[%d]: %s: invalid config\n", pdrv->index, __func__);
		return -1;
	}

	return 0;
}

static unsigned int lcd_get_drv_cnt_flag_from_dts(char *dt_addr)
{
#ifdef CONFIG_OF_LIBFDT
	int parent_offset;
	char str[10];
	unsigned int i, flag = 0;

	for (i = 0; i < lcd_data->drv_max; i++) {
		if (i == 0)
			sprintf(str, "/lcd");
		else
			sprintf(str, "/lcd%d", i);

		parent_offset = fdt_path_offset(dt_addr, str);
		if (parent_offset < 0) {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("not find %s node\n", str);
		} else {
			flag |= (1 << i);
		}
	}

	return flag;
#else
	return 0;
#endif
}

static unsigned int lcd_get_drv_cnt_flag_from_bsp(void)
{
	unsigned int i, flag = 0;

	for (i = 0; i < lcd_data->drv_max; i++) {
		if (!lcd_data->dft_conf[i])
			continue;
		if (lcd_data->dft_conf[i]->ext_lcd) {
			flag |= (1 << i);
		} else {
			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
				LCDPR("not find ext_lcd[%d]\n", i);
		}
	}

	return flag;
}

static struct aml_lcd_drv_s *lcd_driver_add(int index)
{
	struct aml_lcd_drv_s *pdrv;
	int init_once = 0;

	if (index >= lcd_data->drv_max) {
		LCDERR("%s: invalid index: %d\n", __func__, index);
		return NULL;
	}
	if (!lcd_data->dft_conf[index]) {
		LCDERR("%s: no lcd%d dft_conf\n", __func__, index);
		return NULL;
	}
	if (!lcd_driver[index]) {
		lcd_driver[index] = (struct aml_lcd_drv_s *)
			malloc(sizeof(struct aml_lcd_drv_s));
		if (!lcd_driver[index]) {
			LCDERR("%s: Not enough memory\n", __func__);
			return NULL;
		}
		init_once = 1;
	}

	pdrv = lcd_driver[index];
	memset(pdrv, 0, sizeof(struct aml_lcd_drv_s));
	pdrv->index = index;

	if (init_once) {
		pdrv->power_on_suspend = 1;
		pdrv->dev_pm_ops = dev_register_pm(lcd_pm_name[index],
							&aml_lcd_driver_suspend,
							&aml_lcd_driver_resume,
							&aml_lcd_driver_poweroff);
	}

	/* default config */
	pdrv->data = lcd_data;
	pdrv->config.basic.lcd_type = LCD_TYPE_MAX;
	pdrv->config.power.power_on_step[0].type = LCD_POWER_TYPE_MAX;
	pdrv->config.power.power_off_step[0].type = LCD_POWER_TYPE_MAX;
	pdrv->config.pinmux_set[0][0] = LCD_PINMUX_END;
	pdrv->config.pinmux_set[0][1] = 0x0;
	pdrv->config.pinmux_clr[0][0] = LCD_PINMUX_END;
	pdrv->config.pinmux_clr[0][1] = 0x0;
	pdrv->config.backlight_index = 0xff;

	/* default setting */
	pdrv->config.retry_enable_flag = 0;
	pdrv->config.retry_enable_cnt = 0;

	return pdrv;
}

static int lcd_driver_remove(int index)
{
	if (index >= lcd_data->drv_max)
		return 0;

	if (!lcd_driver[index])
		return 0;

	if (lcd_driver[index]->dev_pm_ops)
		dev_unregister_pm(lcd_driver[index]->dev_pm_ops);

	free(lcd_driver[index]);
	lcd_driver[index] = NULL;

	return 0;
}

static void lcd_update_ctrl_bootargs(struct aml_lcd_drv_s *pdrv)
{
	unsigned int val = 0;
	char env_str[15], ctrl_str[20];

	pdrv->boot_ctrl.lcd_type = pdrv->config.basic.lcd_type;
	pdrv->boot_ctrl.lcd_bits = pdrv->config.basic.lcd_bits;
	pdrv->boot_ctrl.clk_mode = pdrv->config.timing.clk_mode;
	pdrv->boot_ctrl.base_frame_rate = pdrv->config.timing.base_timing.frame_rate;
	switch (pdrv->config.timing.ppc) {
	case 2:
		pdrv->boot_ctrl.ppc = LCD_VENC_2PPC;
		break;
	case 4:
		pdrv->boot_ctrl.ppc = LCD_VENC_4PPC;
		break;
	case 1:
	default:
		pdrv->boot_ctrl.ppc = LCD_VENC_1PPC;
		break;
	}
	switch (pdrv->config.basic.lcd_type) {
	case LCD_RGB:
		pdrv->boot_ctrl.advanced_flag =
			(pdrv->config.control.rgb_cfg.sync_valid << 1) |
			(pdrv->config.control.rgb_cfg.de_valid << 0);
		break;
	case LCD_P2P:
		pdrv->boot_ctrl.advanced_flag =
			pdrv->config.control.p2p_cfg.p2p_type;
		break;
	default:
		break;
	}
	pdrv->boot_ctrl.custom_pinmux = pdrv->config.custom_pinmux ? 1 : 0;
	pdrv->boot_ctrl.init_level = env_get_ulong("lcd_init_level", 10, 0);

	/*
	 *bit[31:23]: base frame rate
	 *bit[23:22]: clk_mode
	 *bit[21:20]: ppc
	 *bit[19:18]: lcd_init_level
	 *bit[17]: reserved
	 *bit[16]: custom pinmux flag
	 *bit[15:8]: advanced flag(p2p_type when lcd_type=p2p)
	 *bit[7:4]: lcd bits
	 *bit[3:0]: lcd_type
	 */
	val |= (pdrv->boot_ctrl.lcd_type & 0xf);
	val |= (pdrv->boot_ctrl.lcd_bits & 0xf) << 4;
	val |= (pdrv->boot_ctrl.advanced_flag & 0xff) << 8;
	val |= (pdrv->boot_ctrl.custom_pinmux & 0x1) << 16;
	val |= (pdrv->boot_ctrl.init_level & 0x3) << 18;
	val |= (pdrv->boot_ctrl.ppc & 0x3) << 20;
	val |= (pdrv->boot_ctrl.clk_mode & 0x3) << 22;
	val |= (pdrv->boot_ctrl.base_frame_rate & 0xff) << 24;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("[%d]: %s: ppc=%d, clk_mode=%d, base_fr=%d, bootctrl val=0x%x\n",
			pdrv->index, __func__,
			pdrv->config.timing.ppc,
			pdrv->config.timing.clk_mode,
			pdrv->config.timing.base_timing.frame_rate, val);
	}

	sprintf(ctrl_str, "0x%08x", val);

	if (strlen(pdrv->config.basic.model_name) > 0) {
		if (pdrv->index == 0)
			sprintf(env_str, "panel_name");
		else
			sprintf(env_str, "panel%d_name", pdrv->index);
		env_set(env_str, pdrv->config.basic.model_name);
	}

	if (pdrv->index == 0)
		sprintf(env_str, "lcd_ctrl");
	else
		sprintf(env_str, "lcd%d_ctrl", pdrv->index);
	env_set(env_str, ctrl_str);
}

static void lcd_update_debug_bootargs(void)
{
	unsigned int val = 0;
	char dbg_str[20];

	debug_ctrl.debug_print_flag = lcd_debug_print_flag;
	debug_ctrl.debug_test_pattern = lcd_debug_test_flag;
	debug_ctrl.debug_para_source = env_get_ulong("lcd_debug_para", 10, 0);
	debug_ctrl.debug_lcd_mode = env_get_ulong("lcd_debug_mode", 10, 0);

	/*
	 *bit[31:30]: lcd mode(0=normal, 1=tv; 2=tablet, 3=TBD)
	 *bit[29:28]: lcd debug para source(0=normal, 1=dts, 2=unifykey, 3=bsp for uboot)
	 *bit[27:20]: reserved
	 *bit[19:16]: lcd test pattern
	 *bit[15:0]:  lcd debug print flag
	 */
	val |= (debug_ctrl.debug_print_flag & 0xffff);
	val |= (debug_ctrl.debug_test_pattern & 0xf) << 16;
	val |= (debug_ctrl.debug_para_source & 0x3) << 28;
	val |= (debug_ctrl.debug_lcd_mode & 0x3) << 30;
	sprintf(dbg_str, "0x%08x", val);
	env_set("lcd_debug", dbg_str);
}

void lcd_panel_config_load_to_drv(struct aml_lcd_drv_s *pdrv)
{
	lcd_clk_config_probe(pdrv);
	lcd_phy_probe(pdrv);
	lcd_debug_probe(pdrv);
	lcd_mode_init(pdrv);
	lcd_update_ctrl_bootargs(pdrv);
}

char *lcd_get_dt_addr(void)
{
	return g_dt_addr;
}

static int lcd_config_probe(void)
{
	int load_id = 0, load_id_lcd, load_id_temp;
	struct aml_lcd_drv_s *pdrv;
	unsigned int drv_cnt_flag;
	int i, ret;

#ifdef CONFIG_DTB_MEM_ADDR
	g_dt_addr = (char *)CONFIG_DTB_MEM_ADDR;
#endif
#ifdef CONFIG_AML_LCD_PXP
	g_dt_addr = (char *)0x06000000;
#endif

#ifdef CONFIG_OF_LIBFDT
	if (fdt_check_header(g_dt_addr) < 0) {
		LCDERR("check dts: %s, load default lcd parameters\n",
			fdt_strerror(fdt_check_header(g_dt_addr)));
	} else {
		load_id = 0x1;
	}
#endif
	/*load_id: bit[8]:debug_force, bit[4]:key, bit[0]:dts*/
	switch (debug_ctrl.debug_para_source) {
	case 1:
		LCDPR("lcd_debug_para: 1,dts\n");
		load_id = 0x101;
		break;
	case 2:
		LCDPR("lcd_debug_para: 2,unifykey\n");
		load_id = 0x111;
		break;
	case 3:
		LCDPR("lcd_debug_para: 3,bsp\n");
		load_id = 0x100;
		break;
	default:
		break;
	}
	load_id_lcd = load_id;

	if (load_id_lcd & 0x1) {
		drv_cnt_flag = lcd_get_drv_cnt_flag_from_dts(g_dt_addr);
		if (drv_cnt_flag == 0) {
			LCDPR("not find /lcd node\n");
			drv_cnt_flag = lcd_get_drv_cnt_flag_from_bsp();
			load_id_lcd &= ~(1 << 0);
		}
	} else {
		drv_cnt_flag = lcd_get_drv_cnt_flag_from_bsp();
	}

	for (i = 0; i < lcd_data->drv_max; i++) {
		if ((drv_cnt_flag & (1 << i)) == 0)
			continue;
		pdrv = lcd_driver_add(i);
		if (!pdrv)
			continue;

		if (load_id_lcd & 0x1)
			ret = lcd_base_config_load_from_dts(g_dt_addr, pdrv);
		else
			ret = lcd_base_config_load_from_bsp(pdrv);

		if (ret) {
			lcd_driver_remove(i);
			continue;
		}
		load_id_temp = load_id_lcd & 0xff;
		if ((load_id_lcd & (1 << 8)) == 0) {
			if (pdrv->key_valid)
				load_id_temp |= (1 << 4);
			else
				load_id_temp &= ~(1 << 4);
		}

		if (lcd_get_panel_config(g_dt_addr, load_id_temp, pdrv)) {
			lcd_driver_remove(pdrv->index);
			continue;
		}
		lcd_panel_config_load_to_drv(pdrv);
#ifdef CONFIG_AML_LCD_BACKLIGHT
		aml_bl_probe_single(i, load_id_temp);
#endif
	}

#ifdef CONFIG_AML_LCD_EXTERN
	lcd_extern_probe(g_dt_addr, load_id);
#endif

	return 0;
}

void lcd_power_domain_off(struct aml_lcd_data_s *lcd_data_p)
{
#ifdef CONFIG_SECURE_POWER_CONTROL
	switch (lcd_data_p->chip_type) {
	case LCD_CHIP_T7:
		pwr_ctrl_psci_smc(PM_MIPI_DSI0, 0);
		pwr_ctrl_psci_smc(PM_MIPI_DSI1, 0);
		pwr_ctrl_psci_smc(PM_EDP0, 0);
		pwr_ctrl_psci_smc(PM_EDP1, 0);
		break;
	default:
		return;
	}
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("lcd power domain off\n");
#endif
}

int lcd_probe(void)
{
	int ret = 0;

	lcd_debug_print_flag = env_get_ulong("lcd_debug_print", 16, 0);
	LCDPR("lcd_debug_print flag: 0x%x\n", lcd_debug_print_flag);

	lcd_debug_test_flag = env_get_ulong("lcd_debug_test", 10, 0);

	debug_ctrl.debug_print_flag = lcd_debug_print_flag;
	debug_ctrl.debug_test_pattern = lcd_debug_test_flag;
	debug_ctrl.debug_para_source = env_get_ulong("lcd_debug_para", 10, 0);
	debug_ctrl.debug_lcd_mode = env_get_ulong("lcd_debug_mode", 10, 0);

	lcd_chip_detect();
	if (!lcd_data) {
		LCDERR("%s: invalid lcd data\n", __func__);
		return -1;
	}

#ifdef CONFIG_AML_LCD_EXTERN
	lcd_extern_init();
#endif
#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_init();
#endif

	lcd_config_bsp_init();

	lcd_phy_config_init(lcd_data);
	lcd_venc_probe(lcd_data);
	ret = lcd_config_probe();
	if (ret)
		return -1;

	lcd_power_domain_off(lcd_data);

	lcd_update_debug_bootargs();

	return 0;
}

int lcd_remove(void)
{
	int i;

	if (!lcd_data)
		return 0;

	aml_bl_remove_all();
#ifdef CONFIG_AML_LCD_EXTERN
	lcd_extern_remove();
#endif

	for (i = 0; i < LCD_MAX_DRV; i++) {
		if (lcd_driver[i]) {
			if (lcd_driver[i]->dev_pm_ops)
				dev_unregister_pm(lcd_driver[i]->dev_pm_ops);
			free(lcd_driver[i]);
			lcd_driver[i] = NULL;
		}
	}

	return 0;
}

/* ********************************************** *
  lcd driver API
 * ********************************************** */
int aml_lcd_driver_probe(int index)
{
	return lcd_probe();
}

/***********************************************
 * use for vout
 ************************************************/
void aml_lcd_driver_list_support_mode(void)
{
	struct aml_lcd_drv_s *pdrv;
	int index;

	for (index = 0; index < LCD_MAX_DRV; index++) {
		pdrv = lcd_driver_check_valid(index);
		if (!pdrv)
			continue;

		if (pdrv->list_support_mode)
			pdrv->list_support_mode(pdrv);
	}
}

/***********************************************
 * use for vout
 * parameters:  mode, such as 1080p60hz...
 * return:      viu_mux
 ************************************************/
unsigned int aml_lcd_driver_outputmode_check(unsigned char lcd_idx, char *mode)
{
	struct aml_lcd_drv_s *pdrv;
	int ret;

	if (!mode) {
		LCDERR("%s: mode is NULL\n", __func__);
		return VIU_MUX_MAX;
	}

	pdrv = lcd_driver_check_valid(lcd_idx);
	if (!pdrv)
		return VIU_MUX_MAX;

	if (pdrv->outputmode_check) {
		ret = pdrv->outputmode_check(pdrv, mode);
		if (ret == 0)
			return VIU_MUX_ENCL;
	}

	return VIU_MUX_MAX;
}

void aml_lcd_driver_prepare(int index, char *mode)
{
	struct aml_lcd_drv_s *pdrv;

	if (!mode) {
		LCDERR("%s: mode is NULL\n", __func__);
		return;
	}

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	if (pdrv->status & LCD_STATUS_ENCL_ON) {
		LCDPR("[%d]: already enabled\n", pdrv->index);
		return;
	}

	lcd_module_prepare(pdrv, mode);
}

void aml_lcd_driver_enable(int index, char *mode)
{
	struct aml_lcd_drv_s *pdrv;
	char *ddr_resume = NULL;

	if (!mode) {
		LCDERR("%s: mode is NULL\n", __func__);
		return;
	}

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	ddr_resume = env_get("ddr_resume");
	if (ddr_resume && ddr_resume[0] == '1' && pdrv->power_on_suspend == 1) {
		pdrv->power_on_suspend = 0;
		sprintf(pdrv->init_mode, "%s", mode);
		pdrv->init_mode[strlen(mode)] = '\0';
		LCDPR("%s drv mode=%s\n", __func__, pdrv->init_mode);
		return;
	}

	if (pdrv->status & LCD_STATUS_IF_ON) {
		LCDPR("[%d]: already enabled\n", pdrv->index);
		return;
	}

	lcd_module_enable(pdrv, mode);
}

void aml_lcd_driver_disable(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0) {
		LCDPR("[%d]: already disabled\n", pdrv->index);
		return;
	}

	lcd_module_disable(pdrv);
}

void aml_lcd_driver_set_ss(int index, unsigned int level, unsigned int freq,
			   unsigned int mode)
{
	struct aml_lcd_drv_s *pdrv;
	int ret;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0) {
		LCDPR("[%d]: already disabled\n", pdrv->index);
		return;
	}

	ret = lcd_set_ss(pdrv, level, freq, mode);
	if (ret == 0) {
		if (level < 0xff)
			pdrv->config.timing.ss_level = level;
		if (freq < 0xff)
			pdrv->config.timing.ss_freq = freq;
		if (mode < 0xff)
			pdrv->config.timing.ss_mode = mode;
	}
}

void aml_lcd_driver_get_ss(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	if ((pdrv->status & LCD_STATUS_ENCL_ON) == 0) {
		LCDPR("[%d]: already disabled\n", pdrv->index);
		return;
	}

	lcd_get_ss(pdrv);
}

void aml_lcd_driver_test(int index, int num)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	if (num == 10) {
		lcd_display_init_test(pdrv);
		return;
	} else if (num == 20) {
		lcd_display_init_reg_dump(pdrv);
		return;
	}

	if ((pdrv->status & LCD_STATUS_IF_ON) == 0) {
		LCDPR("[%d]: already disabled\n", pdrv->index);
		return;
	}

	lcd_debug_test(pdrv, num);
}

void aml_lcd_driver_clk_info(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	lcd_clk_config_print(pdrv);
}

void aml_lcd_driver_debug_print(int index, unsigned int val)
{
	char str[32];

	lcd_debug_print_flag = val;
	snprintf(str, 32, "setenv lcd_debug_print %d", val);
	run_command(str, 0);
	LCDPR("set debug_print_flag: %d\n", val);
}

void aml_lcd_driver_info(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	lcd_info_print(pdrv);
}

void aml_lcd_driver_reg_info(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	lcd_reg_print(pdrv);
}

void aml_lcd_config_check(int index)
{
	struct aml_lcd_drv_s *pdrv;
	int ret;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	ret = lcd_config_timing_check(pdrv, &pdrv->config.timing.act_timing);
	if (ret == 0)
		printf("lcd config_timing_check: PASS\n");
	printf("disp_tmg_min_req:\n"
		"  alert_lvl  %d\n"
		"  hswbp  %d\n"
		"  hfp    %d\n"
		"  vswbp  %d\n"
		"  vfp    %d\n\n",
		pdrv->disp_req.alert_level,
		pdrv->disp_req.hswbp_vid, pdrv->disp_req.hfp_vid,
		pdrv->disp_req.vswbp_vid, pdrv->disp_req.vfp_vid);
#ifdef CONFIG_AML_LCD_TCON
	if (pdrv->config.basic.lcd_type == LCD_MLVDS ||
	    pdrv->config.basic.lcd_type == LCD_P2P) {
		lcd_tcon_dbg_check(pdrv, &pdrv->config.timing.act_timing);
	}
#endif
	printf("config_check_glb: %d, config_check: 0x%x, config_check_en: %d\n\n",
		pdrv->config_check_glb, pdrv->config.basic.config_check, pdrv->config_check_en);
}

void aml_lcd_vbyone_rst(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	lcd_vbyone_rst(pdrv);
}

int aml_lcd_vbyone_cdr(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return -1;

	return lcd_vbyone_cdr(pdrv);
}

int aml_lcd_vbyone_lock(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return -1;

	return lcd_vbyone_lock(pdrv);
}

int aml_lcd_edp_debug(int index, char *str, int num)
{
#ifdef CONFIG_AML_LCD_TABLET
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return -1;

	return eDP_debug_test(pdrv, str, num);
#endif
	return -1;
}

void aml_lcd_mipi_dsi_mode(int index, u8 mode)
{
#ifdef CONFIG_AML_LCD_TABLET
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	lcd_dsi_set_operation_mode(pdrv, mode);
#endif
}

void aml_lcd_mipi_dsi_cmd(int index, u8 *payload)
{
#ifdef CONFIG_AML_LCD_TABLET
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;

	lcd_dsi_write_cmd(pdrv, payload);
#endif
}

int aml_lcd_mipi_dsi_read(int index, u8 *payload, u8 *rd_data, u8 rd_byte_len)
{
#ifdef CONFIG_AML_LCD_TABLET
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return -1;

	return lcd_dsi_read(pdrv, payload, rd_data, rd_byte_len);
#endif
	return -1;
}

void aml_lcd_driver_ext_info(int index)
{
	struct aml_lcd_drv_s *pdrv;
#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_driver_s *edrv;
#endif

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_EXTERN
	edrv = lcd_extern_get_driver(pdrv->index);
	if (edrv) {
		if (edrv->info_print)
			edrv->info_print(edrv);
	}
#endif
}

void aml_lcd_driver_ext_power_on(int index)
{
	struct aml_lcd_drv_s *pdrv;
#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_driver_s *edrv;
#endif

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_EXTERN
	edrv = lcd_extern_get_driver(pdrv->index);
	if (edrv) {
		if (edrv->power_ctrl)
			edrv->power_ctrl(edrv, 1);
	}
#endif
}

void aml_lcd_driver_ext_power_off(int index)
{
	struct aml_lcd_drv_s *pdrv;
#ifdef CONFIG_AML_LCD_EXTERN
	struct lcd_extern_driver_s *edrv;
#endif

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_EXTERN
	edrv = lcd_extern_get_driver(pdrv->index);
	if (edrv) {
		if (edrv->power_ctrl)
			edrv->power_ctrl(edrv, 0);
	}
#endif
}

void aml_lcd_driver_bl_on(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_driver_enable(pdrv->index);
#endif
}

void aml_lcd_driver_bl_off(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_driver_disable(pdrv->index);
#endif
}

void aml_lcd_driver_set_bl_level(int index, int level)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_set_level(pdrv->index, level);
#endif
}

unsigned int aml_lcd_driver_get_bl_level(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return 0;
#ifdef CONFIG_AML_LCD_BACKLIGHT
	return aml_bl_get_level(pdrv->index);
#endif
	return 0;
}

void aml_lcd_driver_bl_config_print(int index)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return;
#ifdef CONFIG_AML_LCD_BACKLIGHT
	aml_bl_config_print(pdrv->index);
#endif
}

int aml_lcd_driver_prbs(int index, unsigned int ms, unsigned int mode_flag)
{
	struct aml_lcd_drv_s *pdrv;

	pdrv = lcd_driver_check_valid(index);
	if (!pdrv)
		return 0;

	return aml_lcd_prbs_test(pdrv, ms, mode_flag);
}

void aml_lcd_driver_unifykey_dump(int index, unsigned int flag)
{
	unsigned int key_flag = LCD_UKEY_DEBUG_NORMAL;

	if (flag & (1 << 0)) {
		key_flag = LCD_UKEY_DEBUG_NORMAL;
	} else if (flag & (1 << 1)) {
#ifdef CONFIG_AML_LCD_TCON
		key_flag = (LCD_UKEY_DEBUG_TCON | LCD_UKEY_TCON_SIZE_NEW);
#endif
	}
	lcd_unifykey_dump(index, key_flag);
}

int aml_lcd_driver_suspend(void *pm_ops)
{
	int i = 0;
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;

	for (i = 0; i < LCD_MAX_DRV; i++) {
		printf("%s %d: pm->name=%s\n", __func__, __LINE__, pm->name);
		if (strcmp(pm->name, lcd_pm_name[i]) == 0)
			break;
	}

	if (i >= LCD_MAX_DRV || i < 0) {
		LCDERR("lcd_drv%d is not allowed\n", i);
		return 0;
	}

	aml_lcd_driver_disable(i);
	LCDPR("%s driver disabled\n", __func__);

	return 0;
}

int aml_lcd_driver_resume(void *pm_ops)
{
	int i = 0;
	struct dev_pm_ops *pm = (struct dev_pm_ops *)pm_ops;
	struct aml_lcd_drv_s *pdrv;

	for (i = 0; i < LCD_MAX_DRV; i++)
		if (strcmp(pm->name, lcd_pm_name[i]) == 0)
			break;

	pdrv = lcd_driver_check_valid(i);
	if (!pdrv)
		return -1;

	aml_lcd_driver_enable(i, pdrv->init_mode);
	LCDPR("%s driver enable\n", __func__);

	return 0;
}

int aml_lcd_driver_poweroff(void *pm_ops)
{
	aml_lcd_driver_suspend(pm_ops);

	return 0;
}

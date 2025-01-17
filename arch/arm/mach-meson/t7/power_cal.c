// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/arch/secure_apb.h>
#include <asm/arch/io.h>
#include <amlogic/saradc.h>
#include <asm/arch/mailbox.h>

#define P_EE_TIMER_E		(*((volatile unsigned *)(0xffd00000 + (0x3c62 << 2))))
#define vcck_adc_channel	0x4
#define ee_adc_channel		0x5
#define default_ref_val		1800

extern const int pwm_cal_voltage_table[][2];
extern const int pwm_cal_voltage_table_ee[][2];
extern int pwm_cal_voltage_table_size;
extern int pwm_cal_voltage_table_ee_size;


enum pwm_id {
    pwm_vcck = 0,
    pwm_ee,
};

unsigned int _get_time(void)
{
	return P_EE_TIMER_E;
}

void _udelay_(unsigned int us)
{
	unsigned int t0 = _get_time();

	while (_get_time() - t0 <= us)
		;
}

int32_t aml_delt_get(int adc_val, unsigned int voltage)
{
	unsigned int adc_volt;
	int32_t delt;
	int32_t div = 10;	/*10mv is min step*/

	if (adc_val != -1) {
		adc_volt = default_ref_val*adc_val/1024;
		printf("aml pwm cal adc_val = %x, adc_voltage = %d, def_voltage = %d\n",
				adc_val, adc_volt, voltage);
	} else {
		adc_volt = voltage;
		printf("warning:aml pwm cal adc get voltage error\n");
		return 0;
	}
	delt = voltage - adc_volt;
	delt = delt / div;
	return delt;
}

void aml_set_voltage(unsigned int id, unsigned int voltage, int delt)
{
	int to;

	switch (id) {
	case pwm_vcck:
		for (to = 0; to < pwm_cal_voltage_table_size; to++) {
			if (pwm_cal_voltage_table[to][1] >= voltage) {
				break;
			}
		}
		to +=delt;
		if (to >= pwm_cal_voltage_table_size) {
			to = pwm_cal_voltage_table_size - 1;
		}
		/*vcck volt set by dvfs and avs*/
		//writel(pwm_voltage_table[to][0], PWM_PWM_A_ADDRESS);
		_udelay_(200);
		break;

	case pwm_ee:
		for (to = 0; to < pwm_cal_voltage_table_ee_size; to++) {
			if (pwm_cal_voltage_table_ee[to][1] >= voltage) {
				break;
				}
		}
		to +=delt;
		if (to >= pwm_cal_voltage_table_ee_size) {
			to = pwm_cal_voltage_table_ee_size - 1;
		}
		printf("aml pwm cal before ee_address: %x, ee_voltage: %x\n",
				AO_PWM_PWM_B, readl(AO_PWM_PWM_B));
		writel(pwm_cal_voltage_table_ee[to][0],AO_PWM_PWM_B);
		_udelay_(1000);
		printf("aml pwm cal after ee_address: %x, ee_voltage: %x\n",
				AO_PWM_PWM_B, readl(AO_PWM_PWM_B));
		break;
	default:
		break;
	}
	_udelay_(200);
}

int aml_cal_pwm(unsigned int ee_voltage, unsigned int vcck_voltage)
{
	int32_t ee_delt, vcck_delt;
	unsigned int ee_val, vcck_val;
	int ret;

	/*txlx vcck ch4,vddee ch5*/
	ret = adc_channel_single_shot_mode("adc", ADC_MODE_HIGH_PRECISION,
			vcck_adc_channel, &vcck_val);
	if (ret)
		return ret;

	ret = adc_channel_single_shot_mode("adc", ADC_MODE_HIGH_PRECISION,
			ee_adc_channel, &ee_val);
	if (ret)
		return ret;

	vcck_delt = aml_delt_get(vcck_val, vcck_voltage);
	ee_delt = aml_delt_get(ee_val, ee_voltage);
	send_pwm_delt(vcck_delt, ee_delt);
	aml_set_voltage(pwm_ee, AML_VDDEE_INIT_VOLTAGE, ee_delt);
	//aml_set_voltage(pwm_vcck, CONFIG_VCCK_INIT_VOLTAGE, vcck_delt);
	printf("aml board pwm vcck: %x, ee: %x\n", vcck_delt, ee_delt);

	return 0;
}

void aml_pwm_cal_init(int mode)
{
	printf("aml pwm cal init\n");
	aml_cal_pwm(AML_VDDEE_INIT_VOLTAGE, AML_VCCK_INIT_VOLTAGE);
}

/* SPDX-License-Identifier:     GPL-2.0+
 *
 * Copyright 2018 The Fuchsia Authors. All rights reserved.
 */

#pragma once

// BOOTDATA_KERNEL_DRIVER bootdata types
#define KDRV_ARM_PSCI               0x49435350  // 'PSCI'
#define KDRV_ARM_GIC_V2             0x32434947  // 'GIC2'
#define KDRV_ARM_GIC_V3             0x33434947  // 'GIC3'
#define KDRV_ARM_GENERIC_TIMER      0x4D495441  // 'ATIM'
#define KDRV_PL011_UART             0x55304C50  // 'PL0U'
#define KDRV_AMLOGIC_UART           0x554C4D41  // 'AMLU'
#define KDRV_NXP_IMX_UART           0x55584D49  // 'IMXU'
#define KDRV_HISILICON_POWER        0x4F505348  // 'HSPO'
#define KDRV_AMLOGIC_HDCP           0x484C4D41  // 'AMLH'
#define KDRV_GENERIC_32BIT_WATCHDOG 0x32334457  // 'WD32'
// 'RMLH' (typoed, originally intended to by 'AMLR')
#define KDRV_AMLOGIC_RNG_V1         0x484C4D52  // 'RMLH'
#define KDRV_AMLOGIC_RNG_V2         0x524c4d41  // 'AMLR'

// kernel driver struct that can be used for simple drivers
// used by KDRV_PL011_UART, KDRV_AMLOGIC_UART and KDRV_NXP_IMX_UART
typedef struct {
	uint64_t mmio_phys;
	uint32_t irq;
} dcfg_simple_t;

// for KDRV_ARM_PSCI
typedef struct {
	bool use_hvc;
	uint64_t shutdown_args[3];
	uint64_t reboot_args[3];
	uint64_t reboot_bootloader_args[3];
	uint64_t reboot_recovery_args[3];
} dcfg_arm_psci_driver_t;

// for KDRV_ARM_GIC_V2
typedef struct {
	uint64_t mmio_phys;
	uint64_t msi_frame_phys;
	uint64_t gicd_offset;
	uint64_t gicc_offset;
	uint64_t gich_offset;
	uint64_t gicv_offset;
	uint32_t ipi_base;
	bool optional;
	bool use_msi;
} dcfg_arm_gicv2_driver_t;

// for KDRV_ARM_GIC_V3
typedef struct {
	uint64_t mmio_phys;
	uint64_t gicd_offset;
	uint64_t gicr_offset;
	uint64_t gicr_stride;
	uint64_t mx8_gpr_phys;
	uint32_t ipi_base;
	bool optional;
} dcfg_arm_gicv3_driver_t;

// for KDRV_ARM_GENERIC_TIMER
typedef struct {
	uint32_t irq_phys;
	uint32_t irq_virt;
	uint32_t irq_sphys;
	uint32_t freq_override;
} dcfg_arm_generic_timer_driver_t;

// for KDRV_HISILICON_POWER
typedef struct {
	uint64_t sctrl_phys;
	uint64_t pmu_phys;
} dcfg_hisilicon_power_driver_t;

// for KDRV_AMLOGIC_HDCP
typedef struct {
	uint64_t preset_phys;
	uint64_t hiu_phys;
	uint64_t hdmitx_phys;
} dcfg_amlogic_hdcp_driver_t;

// for KDRV_GENERIC_32BIT_WATCHDOG
typedef struct {
	uint64_t addr;
	uint32_t clr_mask;
	uint32_t set_mask;
} dcfg_generic_32bit_watchdog_action_t;

#define KDRV_GENERIC_32BIT_WATCHDOG_FLAG_ENABLED ((uint32_t)0x00000001)

// Align with fuchsia code base.
// Since all zbi items are 8-byte(64bits) aligned,
// it's okay to add 'reserved' field.
typedef struct {
	dcfg_generic_32bit_watchdog_action_t pet_action;
	dcfg_generic_32bit_watchdog_action_t enable_action;
	dcfg_generic_32bit_watchdog_action_t disable_action;
	int64_t watchdog_period_nsec;
	uint32_t flags;
	uint32_t reserved;
} dcfg_generic_32bit_watchdog_t;

// for KDRV_AMLOGIC_RNG_V1
// for KDRV_AMLOGIC_RNG_V2
typedef struct {
	uint64_t rng_data_phys;
	uint64_t rng_status_phys;
	uint64_t rng_refresh_interval_usec;
} dcfg_amlogic_rng_driver_t;

/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * arch/arm/include/asm/arch-t5d/romboot.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __BOOT_ROM_H_
#define __BOOT_ROM_H_
#ifndef __ASSEMBLY__
//#include <stdint.h>
//uint8_t simple_i2c(uint8_t adr);
//void spi_pin_mux(void);
//void spi_init(void);
//uint32_t spi_read(uint32_t src, uint32_t mem, uint32_t size);
//void udelay(uint32_t usec);
//void boot_des_decrypt(uint8_t *ct, uint8_t *pt, uint32_t size);

#endif /* ! __ASSEMBLY__ */
#include "config.h"

/* Magic number to "boot" up A53 */
#define AO_SEC_SD_CFG10_CB			0x80000000

/*BOOT device and ddr size*/
/*31-28: boot device id, 27-24: boot device para, 23-20: reserved*/
/*19-8: ddr size, 7-0: board revision*/
//#define P_AO_SEC_GP_CFG0                                     0xDA100240 //defined in secure_apb.h
#define AO_SEC_GP_CFG7_W0_BIT			8
#define AO_SEC_GP_CFG7_W0			0x100

#define BOOT_ID_RESERVED	0
#define BOOT_ID_EMMC		1
#define BOOT_ID_NAND		2
#define BOOT_ID_SPI		3
#define BOOT_ID_SDCARD		4
#define BOOT_ID_USB		5

#endif /* __BOOT_ROM_H_ */

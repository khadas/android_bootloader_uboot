/*
 * arch/arm/include/asm/arch-t7/mailbox.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

 /*
  *
 * Copyright (C) 2012 Amlogic, Inc.
 *
 * Author: Platform-SH@amlogic.com
 *
 */

#ifndef __MAILBOX_H__
#define __MAILBOX_H__
#include <asm/arch/secure_apb.h>

#define REE2AO_SET_ADDR		MAILBOX_SET_MBOX03
#define REE2AO_STS_ADDR		MAILBOX_STS_MBOX03
#define REE2AO_CLR_ADDR		MAILBOX_CLR_MBOX03
#define REE2AO_WR_ADDR		MAILBOX_BUF_MBOX03
#define REE2AO_RD_ADDR		MAILBOX_BUF_MBOX03
#define REE2AO_IRQCLR_ADDR	MAILBOX_IRQA_CLR0

#define MAILBOX_USER_DATA_SIZE	96

#define MHU_SYNC		(1 << 26)
#define MHU_CMD_BUILD(command, size) \
	(((command) & 0xffff) | (((size) & 0x1ff) << 16) | MHU_SYNC)
#define MHU_ACK_MASK(mbox)	(1 << ((mbox)*2 + 1))

#define MHU_PAYLOAD_SIZE	0x80
#define MHU_DATA_OFFSET		0x1c
#define REE2AO_MBOX_ID		0x3
/* ...Message composition with module(6bits), function(10bits) */
#define __MBX_COMPOSE_MSG(mod, func)    (((mod) << 10) | ((func) & 0x3FF))

/*******************************************************************************
 * Define module type here, 6bits valid
 ******************************************************************************/
#define MBX_SYSTEM              0x0

/*******************************************************************************
 * Define function here, 10bits valid
 ******************************************************************************/
         /*SYSTEM*/
#define CMD_UNDEFINE            0x0
#define CMD_TEST		0x6
#define CMD_GET_STICK_REBOOT_FLAG	0x40
#define CMD_LED_INFO            0xF7
#define CMD_SET_BL30_PRINT	0x6A
#define CMD_GET_BL30_VERSION	0x6B

/*******************************************************************************
 * TimerE definition
 ******************************************************************************/
#define TIMERE_LOW_REG SYSCTRL_TIMERE
#define TIMERE_HIG_REG SYSCTRL_TIMERE_HI

/*******************************************************************************
 * AOCPU tick count address
 ******************************************************************************/
#define AOCPU_TICK_CNT_RD_ADDR  (MAILBOX_BUF_MBOX04)

/*******************************************************************************
 * Mssage composition
 ******************************************************************************/
#define MBX_CMD_TEST    __MBX_COMPOSE_MSG(MBX_SYSTEM, CMD_TEST)
enum {
        HIFIA_REE_CHANNEL = 0,
        HIFIB_REE_CHANNEL = 1,
        SECPU_REE_CHANNEL = 2,
        AOCPU_REE_CHANNEL = 3,
};

void mhu_init(void);
/*message max size MAILBOX_USER_DATA_SIZE 96 byte*/
int scpi_send_data(uint32_t chan, uint32_t command,
		   void *sendmessage, uint32_t sendsize,
		   void *revmessage, uint32_t revsize);
#endif	/* __SCPI_FIFO_H__ */

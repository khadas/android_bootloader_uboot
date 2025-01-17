/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __ANTI_ROLLBACK_
#define __ANTI_ROLLBACK_

#include <linux/types.h>

#define AVB_UNLOCK_STATE        (0)
#define AVB_LOCK_STATE          (1)

bool check_antirollback(uint32_t kernel_version);

bool is_avb_arb_available(void);
bool set_avb_antirollback(uint32_t index, uint32_t version);
bool get_avb_antirollback(uint32_t index, uint32_t* version);
bool get_avb_lock_state(uint32_t* lock_state);
bool avb_lock(void);
bool avb_unlock(void);

#endif

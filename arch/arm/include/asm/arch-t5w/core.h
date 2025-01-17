/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * arch/arm/include/asm/arch-t5d/core.h
 *
 * Copyright (C) 2020 Amlogic, Inc. All rights reserved.
 *
 */

#ifndef __CORE_H__
#define __CORE_H__

int get_core_mpidr(unsigned int cpuid);
int get_core_idx(unsigned int mpidr);
int get_core_max(void);

#endif

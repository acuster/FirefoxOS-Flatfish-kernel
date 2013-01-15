/*
 * arch/arm/mach-sun6i/dram-freq/mdfs/mdfs.h
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __MDFS_H__
#define __MDFS_H__

#include <mach/dram-freq-common.h>

#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

typedef signed int          __s32;
typedef unsigned int        __u32;

#if 0
    #define DEBUG_LINE  writel(__LINE__, 0xf1f00100)
#else
    #define DEBUG_LINE  do{}while(0)
#endif

#endif  //__MDFS_H__


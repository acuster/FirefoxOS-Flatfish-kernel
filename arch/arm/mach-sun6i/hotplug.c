/*
 *  linux/arch/arm/mach-sun6i/hotplug.c
 *
 *  Copyright (C) 2012-2016 Allwinner Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/smp.h>

#include <asm/cacheflush.h>
#include <asm/smp_plat.h>

extern volatile int pen_release;

static inline void cpu_enter_lowpower(void)
{
	printk("[%s] Not implemented!\n", __FUNCTION__);
}

static inline void cpu_leave_lowpower(void)
{
	printk("[%s] Not implemented!\n", __FUNCTION__);
}

static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
	printk("[%s] Not implemented!\n", __FUNCTION__);
}

int platform_cpu_kill(unsigned int cpu)
{
	printk("[%s] kill cpu%d\n", __FUNCTION__, cpu);
	return 1;
}

void platform_cpu_die(unsigned int cpu)
{
	printk("[%s] cpu%d die\n", __FUNCTION__, cpu);
	asm("wfi");
}

int platform_cpu_disable(unsigned int cpu)
{
	printk("[%s] Not implemented!\n", __FUNCTION__);
	return -1;
}

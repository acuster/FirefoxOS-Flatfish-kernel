/*
 *  linux/arch/arm/mach-aw163x/platsmp.c
 *
 *  Copyright (C) 2012-2016 Allwinner Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/smp.h>

#include <mach/hardware.h>
#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>

#include <mach/platform.h>

#include "core.h"

extern void aw163x_secondary_startup(void);

static DEFINE_SPINLOCK(boot_lock);

/*
 * Initialise the CPU possible map early - this describes the CPUs
 * which may be present or become present in the system.
 *
 * Note: for arch/arm/kernel/setup.csetup_arch(..)
 */
static void __iomem *scu_base_addr(void)
{
	printk("[%s] enter\n", __FUNCTION__);
	return __io_address(AW_SCU_BASE);
}

#define AW_R_CPUCFG_BASE 0x01f01c00

static u32 get_nr_cores(void)
{
        u32 cores;

        /* Read current CP15 Cache Size ID Register */
        asm volatile ("mrc p15, 1, %0, c9, c0, 2" : "=r" (cores));

	printk("[%s] cores=%x\n", __FUNCTION__, cores);
	cores = ((cores >> 24) & 0x3) + 1;
        return cores;
}

void enable_all_cpus(void)
{
	printk("[%s] enter\n", __FUNCTION__);

	//let reset go
	writel(0x1, 0xf0000000 + AW_R_CPUCFG_BASE + 0x80);
	writel(0x1, 0xf0000000 + AW_R_CPUCFG_BASE + 0xc0);
	writel(0x1, 0xf0000000 + AW_R_CPUCFG_BASE + 0x100);
	printk("[%s] leave\n", __FUNCTION__);
}

void enable_aw_cpu(int cpu)
{
        printk("[%s] switchs on cpu%d\n", __FUNCTION__, cpu);

        //let reset go

	if (cpu == 1) {
	        writel(0x1, 0xf0000000 + AW_R_CPUCFG_BASE + 0x80);
	} else if (cpu == 2) {
	        writel(0x1, 0xf0000000 + AW_R_CPUCFG_BASE + 0xc0);
	} else if (cpu == 3) {
	        writel(0x1, 0xf0000000 + AW_R_CPUCFG_BASE + 0x100);
	} else {
		printk("[%s] error cpu%d\n", __FUNCTION__, cpu);
	}

        printk("[%s] leave\n", __FUNCTION__);
}


void __init smp_init_cpus(void)
{
	unsigned int i, ncores;

	ncores =  get_nr_cores();
	printk("[%s] ncores=%d\n", __FUNCTION__, ncores);

	for (i = 0; i < ncores; i++)
                set_cpu_possible(i, true);

	set_smp_cross_call(gic_raise_softirq);
}

/*
 * for arch/arm/kernel/smp.c:smp_prepare_cpus(unsigned int max_cpus)
 */
void __init platform_smp_prepare_cpus(unsigned int max_cpus)
{
	void __iomem *scu_base;

	printk("[%s] enter\n", __FUNCTION__);
	scu_base = scu_base_addr();
	scu_enable(scu_base);
}

/*
 * for linux/arch/arm/kernel/smp.c:secondary_start_kernel(void)
 */
void __cpuinit platform_secondary_init(unsigned int cpu)
{
	printk("[%s] enter\n", __FUNCTION__);
	gic_secondary_init(0);
}

/*
 * for linux/arch/arm/kernel/smp.c:__cpu_up(..)
 */
int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	long paddr;

	printk("[%s] enter\n", __FUNCTION__);

	paddr = virt_to_phys(aw163x_secondary_startup);
	writel(paddr, IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_P_REG0);
	printk("set startup address to 0x%x\n", (unsigned int)paddr);
	smp_wmb();

	enable_aw_cpu(cpu);

	printk("[%s] leave\n", __FUNCTION__);
	return 0;
}

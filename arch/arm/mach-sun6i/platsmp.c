/*
 *  linux/arch/arm/mach-sun6i/platsmp.c
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

extern void sun6i_secondary_startup(void);

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

void enable_aw_cpu(int cpu)
{
	long paddr;
	volatile long reg1 = 0x0;

	paddr = virt_to_phys(sun6i_secondary_startup);
        writel(paddr, IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_P_REG0);

	/* let cpus go */
	writel(1, IO_ADDRESS(AW_R_CPUCFG_BASE) + 0x80);

	reg1 = readl(IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_P_REG1);

	if (cpu == 1) {
		reg1 |= 0x2;
	} else if (cpu == 2) {
		reg1 |= 0x4;
	} else if (cpu == 3) {
		reg1 |= 0x8;
	}

	writel(reg1, IO_ADDRESS(AW_R_CPUCFG_BASE) + AW_CPUCFG_P_REG1);
}


void __init smp_init_cpus(void)
{
	unsigned int i, ncores;

	ncores =  scu_get_core_count(NULL);
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

        spin_lock(&boot_lock);
        spin_unlock(&boot_lock);
	printk("[%s] leave\n", __FUNCTION__);
}

/*
 * for linux/arch/arm/kernel/smp.c:__cpu_up(..)
 */
int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	printk("[%s] enter\n", __FUNCTION__);

	spin_lock(&boot_lock);
	enable_aw_cpu(cpu);
	spin_unlock(&boot_lock);

	printk("[%s] leave\n", __FUNCTION__);
	return 0;
}

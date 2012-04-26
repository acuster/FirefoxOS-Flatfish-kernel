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

void __init smp_init_cpus(void)
{
	void __iomem *scu_base = scu_base_addr();
	unsigned int i, ncores;

	ncores =  scu_get_core_count(scu_base);
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

#define AW_R_PRCM_BASE 0x01f01400
#define AW_R_CPUCFG_BASE 0x01f01c00

static void __iomem *r_prcm_base = NULL;
static void __iomem *r_cpucfg_base = NULL;
void switch_on_cpu1()
{
	volatile unsigned long prcm;

	printk("[%s] enter\n", __FUNCTION__);

	r_prcm_base = ioremap_nocache(AW_R_PRCM_BASE, 0x800);
	r_cpucfg_base = ioremap_nocache(AW_R_CPUCFG_BASE, 0x800);


	printk("[%s] %p, %p\n", __FUNCTION__, r_prcm_base, r_cpucfg_base);


	prcm = readl(r_prcm_base+0x144);
	prcm |= 0x1f;
	writel(prcm, r_prcm_base+0x144);


	smp_wmb();
	flush_cache_all();

	//let reset go
	writel(0x1, r_cpucfg_base+0x80);

	iounmap(r_prcm_base);
	iounmap(r_cpucfg_base);
}
/*
 * for linux/arch/arm/kernel/smp.c:__cpu_up(..)
 */
int __cpuinit boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	long paddr;
	volatile long *sram_a2_base = NULL;

	printk("[%s] enter\n", __FUNCTION__);

	paddr = virt_to_phys(aw163x_secondary_startup);
	sram_a2_base = ioremap_nocache(AW_SRAM_A2_BASE, 0x4);
	if (!sram_a2_base) {
		printk("[%s] remap failed\n", __FUNCTION__);
		return -1;
	}

	sram_a2_base[0] = paddr;
	printk("set %p to 0x%x\n", sram_a2_base, paddr);
	smp_wmb();

	switch_on_cpu1();
	printk("[%s] leave\n", __FUNCTION__);
	return 0;
}

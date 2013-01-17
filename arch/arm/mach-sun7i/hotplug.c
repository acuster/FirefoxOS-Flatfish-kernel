/*
 *  linux/arch/arm/mach-sun7i/hotplug.c
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
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <mach/platform.h>
#include <mach/hardware.h>


extern volatile int pen_release;
static cpumask_t dead_cpus;

static inline void cpu_enter_lowpower(void)
{
	printk("[%s] need implement\n", __func__);
}

static inline void cpu_leave_lowpower(void)
{
	printk("[%s] need implement\n", __func__);
}

static inline void platform_do_lowpower(unsigned int cpu, int *spurious)
{
	printk("[%s] cpu%d, need implement\n", __func__, cpu);
}

int platform_cpu_kill(unsigned int cpu)
{
	int k;
	u32 pwr_reg;

	int tmp_cpu = get_cpu();
	put_cpu();
	pr_info("[hotplug]: cpu(%d) try to kill cpu(%d)\n", tmp_cpu, cpu);

	for (k = 0; k < 1000; k++) {
		if (cpumask_test_cpu(cpu, &dead_cpus)) {
			/*
			 * Clear the reset Control
			 */
			writel(0, IO_ADDRESS(SW_PA_CPUCFG_IO_BASE) + CPUX_RESET_CTL(cpu));

			/*
			 * Set the poweroff gateing.
			 */
			pwr_reg = readl(IO_ADDRESS(SW_PA_CPUCFG_IO_BASE) + AW_CPUCFG_PWROFF_REG);
			pwr_reg |= (1<<cpu);
			writel(pwr_reg, IO_ADDRESS(SW_PA_CPUCFG_IO_BASE) + AW_CPUCFG_PWROFF_REG);

			/*
			 * Set the clamp control.
			 */
			writel(0xff, IO_ADDRESS(SW_PA_CPUCFG_IO_BASE) + AW_CPUCFG_PWR_CLAMP);
			pr_info("[hotplug]: cpu%d is killed!\n", cpu);

		    return 1;
		}

		mdelay(1);
	}

	return 0;
}

void platform_cpu_die(unsigned int cpu)
{
	/* hardware shutdown code running on the CPU that is being offlined */
	flush_cache_all();
	dsb();

	/* notify platform_cpu_kill() that hardware shutdown is finished */
	cpumask_set_cpu(cpu, &dead_cpus);

	while(1) {
		asm("wfi" : : : "memory", "cc");
	}
}

int platform_cpu_disable(unsigned int cpu)
{
	cpumask_clear_cpu(cpu, &dead_cpus);
	/*
	 * we don't allow CPU 0 to be shutdown (it is still too special
	 * e.g. clock tick interrupts)
	 */
	return cpu == 0 ? -EPERM : 0;
}

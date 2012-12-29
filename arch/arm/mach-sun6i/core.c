/*
 *  arch/arm/mach-sun6i/core.c
 *
 *  Copyright (C) 2012 - 2016 Allwinner Limited
 *  Benn Huang (benn@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/amba/bus.h>
#include <linux/amba/pl061.h>
#include <linux/amba/mmci.h>
#include <linux/memblock.h>
#include <linux/amba/pl022.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/pmu.h>
#include <asm/smp_twd.h>
#include <asm/pgtable.h>
#include <asm/hardware/gic.h>
#include <linux/clockchips.h>
#include <asm/hardware/cache-l2x0.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <mach/io.h>
#include <mach/timer.h>

#include "core.h"

static struct map_desc sun6i_io_desc[] __initdata = {
	{IO_ADDRESS(AW_SRAM_A1_BASE), __phys_to_pfn(AW_SRAM_A1_BASE),  AW_SRAM_A1_SIZE, MT_MEMORY_ITCM},
	{IO_ADDRESS(AW_SRAM_A2_BASE), __phys_to_pfn(AW_SRAM_A2_BASE),  AW_SRAM_A2_SIZE, MT_DEVICE_NONSHARED},
	{IO_ADDRESS(AW_IO_PHYS_BASE), __phys_to_pfn(AW_IO_PHYS_BASE),  AW_IO_SIZE, MT_DEVICE_NONSHARED},
	{IO_ADDRESS(AW_BROM_BASE),    __phys_to_pfn(AW_BROM_BASE),     AW_BROM_SIZE, MT_DEVICE_NONSHARED},
};

static void __init sun6i_map_io(void)
{
	iotable_init(sun6i_io_desc, ARRAY_SIZE(sun6i_io_desc));
}


static void __init gic_init_irq(void)
{
	gic_init(0, 29, (void *)IO_ADDRESS(AW_GIC_DIST_BASE), (void *)IO_ADDRESS(AW_GIC_CPU_BASE));
}


void __init sun6i_clkevt_init(void);
int __init sun6i_clksrc_init(void);
int __init arch_timer_sched_clock_init(void);
int arch_timer_common_register(void);

static void __init sun6i_timer_init(void)
{
	sun6i_clkevt_init();
	sun6i_clksrc_init();
	arch_timer_common_register();
}


static struct sys_timer sun6i_timer = {
	.init		= sun6i_timer_init,
};

static void sun6i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
	pr_debug("[%s] enter\n", __func__);
	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = HW_RESERVED_MEM_BASE - meminfo->bank[0].start;
	meminfo->bank[1].start = HW_RESERVED_MEM_BASE + HW_RESERVED_MEM_SIZE;
	meminfo->bank[1].size = (PLAT_PHYS_OFFSET+PLAT_MEM_SIZE) - meminfo->bank[1].start;

	/* for sys_config */
	memblock_reserve(SYS_CONFIG_MEMBASE, SYS_CONFIG_MEMSIZE);
	/* for standby: 0x4600,0000-0x4600,0000+1k; */
	memblock_reserve(SUPER_STANDBY_MEM_BASE, SUPER_STANDBY_MEM_SIZE);
#if defined(CONFIG_ION) || defined(CONFIG_ION_MODULE)
	/* for ion carveout heap */
	memblock_reserve(ION_CARVEOUT_MEM_BASE, ION_CARVEOUT_MEM_SIZE);
#endif

	meminfo->nr_banks = 2;
}

static void sun6i_restart(char mode, const char *cmd)
{
	pr_debug("[%s] enter\n", __func__);
	writel(0, (AW_VIR_R_WDOG_BASE + AW_WDOG0_IRQ_EN_REG));
	writel(1, (AW_VIR_R_WDOG_BASE + AW_WDOG0_CFG_REG));
	writel(1, (AW_VIR_R_WDOG_BASE + AW_WDOG0_MODE_REG)); /* interval is 0.5 sec */
	while(1); /* never return */
}

extern void sw_pdev_init(void);
static void __init sun6i_init(void)
{
	pr_debug("[%s] enter\n", __func__);
	sw_pdev_init();
	/* Register platform devices here!! */
}

void __init sun6i_init_early(void)
{
	pr_debug("[%s] enter\n", __func__);
}

MACHINE_START(SUN6I, "sun6i")
	.atag_offset	= 0x100,
	.fixup		= sun6i_fixup,
	.map_io		= sun6i_map_io,
	.init_early	= sun6i_init_early,
	.init_irq	= gic_init_irq,
	.timer		= &sun6i_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= sun6i_init,
#ifdef CONFIG_ZONE_DMA
	.dma_zone_size	= SZ_256M,
#endif
	.restart	= sun6i_restart,
MACHINE_END

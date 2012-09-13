/*
 *  arch/arm/mach-sun7i/common.c
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

#include <mach/includes.h>

#include "core.h"

#define MEM_RESERVE_20120816 /* liugang, reserve memory for de/mp/sys_config/ve, 2012-8-16 */

static void __iomem *timer_cpu_base = 0;

#define TIMER0_VALUE (AW_CLOCK_SRC / (AW_CLOCK_DIV*100))

static struct map_desc sun7i_io_desc[] __initdata = {
	{IO_ADDRESS(AW_SRAM_A1_BASE), __phys_to_pfn(AW_SRAM_A1_BASE),  AW_SRAM_A1_SIZE, MT_MEMORY_ITCM},	/*16K bytes*/
	{IO_ADDRESS(AW_SRAM_A2_BASE), __phys_to_pfn(AW_SRAM_A2_BASE),  AW_SRAM_A2_SIZE, MT_MEMORY_ITCM},	/*16K bytes*/
	{IO_ADDRESS(AW_SRAM_A3_A4_BASE), __phys_to_pfn(AW_SRAM_A3_A4_SIZE),  AW_SRAM_A2_SIZE, MT_MEMORY_ITCM}, 	/*16K bytes*/
	{IO_ADDRESS(AW_IO_PHYS_BASE), __phys_to_pfn(AW_IO_PHYS_BASE),  AW_IO_SIZE, MT_DEVICE_NONSHARED},
};

static void __init sun7i_map_io(void)
{
	iotable_init(sun7i_io_desc, ARRAY_SIZE(sun7i_io_desc));
}

static void __init gic_init_irq(void)
{
	gic_init(0, 29, (void *)IO_ADDRESS(AW_GIC_DIST_BASE), (void *)IO_ADDRESS(AW_GIC_CPU_BASE));
}

static void timer_set_mode(enum clock_event_mode mode, struct clock_event_device *clk)
{
        volatile u32 ctrl = 0;

        switch (mode) {
        case CLOCK_EVT_MODE_PERIODIC:
                writel(TIMER0_VALUE, timer_cpu_base + AW_TMR0_INTV_VALUE_REG); /* interval (999+1) */
                ctrl = readl(timer_cpu_base + AW_TMR0_CTRL_REG);
                ctrl &= ~(1<<7);    /* Continuous mode */
                ctrl |= 1;  /* Enable this timer */
                break;
        case CLOCK_EVT_MODE_ONESHOT:
                break;
        case CLOCK_EVT_MODE_UNUSED:
        case CLOCK_EVT_MODE_SHUTDOWN:
        default:
                ctrl = readl(timer_cpu_base + AW_TMR0_CTRL_REG);
                ctrl &= ~(1<<0);    /* Disable timer0 */
                break;
        }

        writel(ctrl, timer_cpu_base + AW_TMR0_CTRL_REG);
}

static struct clock_event_device sun7i_timer0_clockevent = {
        .name = "timer0",
        .shift = 32,
        .rating = 100,
        .features = CLOCK_EVT_FEAT_PERIODIC,
        .set_mode = timer_set_mode,
};

static irqreturn_t sun7i_timer_interrupt(int irq, void *dev_id)
{
        struct clock_event_device *evt = (struct clock_event_device *)dev_id;

        /* Clear interrupt */
        writel(0x1, timer_cpu_base + AW_TMR_IRQ_STA_REG);

        /*
         * timer_set_next_event will be called only in ONESHOT mode
         */
        evt->event_handler(evt);
        return IRQ_HANDLED;
}

static struct irqaction sun7i_timer_irq = {
        .name = "timer0",
        .flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
        .handler = sun7i_timer_interrupt,
        .dev_id = &sun7i_timer0_clockevent,
        .irq = 36,
};

static void __init sun7i_timer_init(void)
{
	int ret;

	timer_cpu_base = ioremap_nocache(AW_TIMER_BASE, 0x1000);
	printk("[%s] base=%p\n", __FUNCTION__,timer_cpu_base);

        /* Disable & clear all timers */
	writel(0x3f, timer_cpu_base + AW_TMR_IRQ_EN_REG);
        writel(0x3f, timer_cpu_base + AW_TMR_IRQ_STA_REG);

        /* Init timer0 */
        writel(TIMER0_VALUE, timer_cpu_base + AW_TMR0_INTV_VALUE_REG);
        writel(0x66, timer_cpu_base + AW_TMR0_CTRL_REG);

        ret = setup_irq(36, &sun7i_timer_irq);
        if (ret) {
                early_printk("failed to setup irq %d\n", 36);
        }

        /* Enable timer0 */
        writel(0x1, timer_cpu_base + AW_TMR_IRQ_EN_REG);

        sun7i_timer0_clockevent.mult = div_sc(AW_CLOCK_SRC/AW_CLOCK_DIV, NSEC_PER_SEC, sun7i_timer0_clockevent.shift);
        sun7i_timer0_clockevent.max_delta_ns = clockevent_delta2ns(0xff, &sun7i_timer0_clockevent);
        sun7i_timer0_clockevent.min_delta_ns = clockevent_delta2ns(0x1, &sun7i_timer0_clockevent);
        sun7i_timer0_clockevent.cpumask = cpu_all_mask;
        sun7i_timer0_clockevent.irq = sun7i_timer_irq.irq;
        clockevents_register_device(&sun7i_timer0_clockevent);
}

static struct sys_timer sun7i_timer = {
	.init		= sun7i_timer_init,
};

static void sun7i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
	printk("[%s] enter\n", __FUNCTION__);
	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = PLAT_MEM_SIZE - SW_FB_MEM_SIZE;

	meminfo->nr_banks = 1;
}

#ifdef MEM_RESERVE_20120816
u32 g_mem_resv[][2] = {
	{SW_SCRIPT_MEM_BASE, 	SW_SCRIPT_MEM_SIZE	},
	//{SW_FB_MEM_BASE, 	SW_FB_MEM_SIZE		},
	{SW_G2D_MEM_BASE, 	SW_G2D_MEM_SIZE		},
	{SW_CSI_MEM_BASE, 	SW_CSI_MEM_SIZE		},
	{SW_GPU_MEM_BASE, 	SW_GPU_MEM_SIZE		},
	{SW_VE_MEM_BASE, 	SW_VE_MEM_SIZE		},
	{SUPER_STANDBY_BASE,	SUPER_STANDBY_SIZE	},	//for standby: 0x5200,0000-0x5200,0000+64k;

};

static void __init sun7i_reserve(void)
{
	u32 	i = 0;

	pr_info("Memory Reserved(in bytes):\n");

	for(i = 0; i < ARRAY_SIZE(g_mem_resv); i++) {
		if(0 != memblock_reserve(g_mem_resv[i][0], g_mem_resv[i][1]))
			printk("%s err, line %d, base 0x%08x, size 0x%08x\n", __FUNCTION__,
				__LINE__, g_mem_resv[i][0], g_mem_resv[i][1]);
		else
			pr_info("\t: 0x%08x, 0x%08x\n", g_mem_resv[i][0], g_mem_resv[i][1]);
	}
}
#endif /* MEM_RESERVE_20120816 */

static void sun7i_restart(char mode, const char *cmd)
{
	printk("[%s] enter\n", __FUNCTION__);
}

extern void sw_pdev_init(void);
static void __init sun7i_init(void)
{
	printk("[%s] enter\n", __FUNCTION__);
	sw_pdev_init();
	/* Register platform devices here!! */
}

void __init sun7i_init_early(void)
{
	printk("[%s] enter\n", __FUNCTION__);
}

int sw_get_chip_id(struct sw_chip_id *chip_id)
{
    chip_id->sid_rkey0 = readl(SW_VA_SID_IO_BASE);
    chip_id->sid_rkey1 = readl(SW_VA_SID_IO_BASE+0x04);
    chip_id->sid_rkey2 = readl(SW_VA_SID_IO_BASE+0x08);
    chip_id->sid_rkey3 = readl(SW_VA_SID_IO_BASE+0x0C);

    return 0;
}
EXPORT_SYMBOL(sw_get_chip_id);

MACHINE_START(SUN7I, "Allwinner AW165x")
	.atag_offset	= 0x100,
	.fixup		= sun7i_fixup,
	.map_io		= sun7i_map_io,
	.init_early	= sun7i_init_early,
	.init_irq	= gic_init_irq,
	.timer		= &sun7i_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= sun7i_init,
#ifdef CONFIG_ZONE_DMA
	.dma_zone_size	= SZ_256M,
#endif
	.restart	= sun7i_restart,
#ifdef MEM_RESERVE_20120816
	.reserve        = sun7i_reserve,
#endif /* MEM_RESERVE_20120816 */
MACHINE_END

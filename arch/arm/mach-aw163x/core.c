/*
 *  arch/arm/mach-aw163x/common.c
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

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/platform.h>
#include <mach/io.h>

#include "core.h"

static struct map_desc aw163x_io_desc[] __initdata = {
	{IO_ADDRESS(AW_IO_PHYS_BASE), __phys_to_pfn(AW_IO_PHYS_BASE),  AW_IO_SIZE, MT_DEVICE_NONSHARED},
};

static void __init aw163x_map_io(void)
{
	iotable_init(aw163x_io_desc, ARRAY_SIZE(aw163x_io_desc));
}

static void __iomem *timer_cpu_base = 0;

#if 0
static struct amba_device *amba_devs[] __initdata = {
};
#endif

static void __init gic_init_irq(void)
{
	void __iomem *gic_dist_base;
	void __iomem *gic_cpu_base;
	AW_UART_LOG("1");

	gic_dist_base = ioremap_nocache(AW_GIC_DIST_BASE, 0x1000);
	gic_cpu_base = ioremap_nocache(AW_GIC_CPU_BASE, 0x1000);

	printk("[%s] gic_dist=%p, gic_cpu=%p\n", __FUNCTION__, gic_dist_base, gic_cpu_base);
	gic_init(0, AW_IRQ_GIC_START, gic_dist_base, gic_cpu_base);
	//gic_init(0, 16, gic_dist_base, gic_cpu_base);

}

#define TIMER0_VALUE (AW_CLOCK_SRC / (AW_CLOCK_DIV*100))

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

static struct clock_event_device aw163x_timer0_clockevent = {
        .name = "timer0",
        .shift = 32,
        .rating = 100,
        .features = CLOCK_EVT_FEAT_PERIODIC,
        .set_mode = timer_set_mode,
};

static irqreturn_t aw163x_timer_interrupt(int irq, void *dev_id)
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

static struct irqaction aw163x_timer_irq = {
        .name = "timer0",
        .flags = IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
        .handler = aw163x_timer_interrupt,
        .dev_id = &aw163x_timer0_clockevent,
        .irq = 36,
};

static void __init aw163x_timer_init(void)
{
	int ret;

	AW_UART_LOG("1");

	timer_cpu_base = ioremap_nocache(AW_TIMER_BASE, 0x1000);
	printk("[%s] base=%p\n", __FUNCTION__,timer_cpu_base);

        /* Disable & clear all timers */
	writel(0x3f, timer_cpu_base + AW_TMR_IRQ_EN_REG);
        writel(0x3f, timer_cpu_base + AW_TMR_IRQ_STA_REG);

        /* Init timer0 */
        writel(TIMER0_VALUE, timer_cpu_base + AW_TMR0_INTV_VALUE_REG);
        writel(0x66, timer_cpu_base + AW_TMR0_CTRL_REG);

        ret = setup_irq(36, &aw163x_timer_irq);
        if (ret) {
		AW_UART_LOG("2");
                early_printk("failed to setup irq %d\n", 36);
        }

        /* Enable timer0 */
        writel(0x1, timer_cpu_base + AW_TMR_IRQ_EN_REG);

        aw163x_timer0_clockevent.mult = div_sc(AW_CLOCK_SRC/AW_CLOCK_DIV, NSEC_PER_SEC, aw163x_timer0_clockevent.shift);
        aw163x_timer0_clockevent.max_delta_ns = clockevent_delta2ns(0xff, &aw163x_timer0_clockevent);
        aw163x_timer0_clockevent.min_delta_ns = clockevent_delta2ns(0x1, &aw163x_timer0_clockevent);
        aw163x_timer0_clockevent.cpumask = cpumask_of(0);
        aw163x_timer0_clockevent.irq = aw163x_timer_irq.irq;
        clockevents_register_device(&aw163x_timer0_clockevent);
	AW_UART_LOG("3");
}

static struct sys_timer aw163x_timer = {
	.init		= aw163x_timer_init,
};

static void aw163x_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
	AW_UART_LOG("1");
	meminfo->bank[0].start = 0x40000000;
	meminfo->bank[0].size = SZ_64M;
	meminfo->nr_banks = 1;
	AW_UART_LOG("2");
}

static void aw163x_restart(char mode, const char *cmd)
{
	AW_UART_LOG("1");
}

extern void sw_pdev_init(void);
static void __init aw163x_init(void)
{
	AW_UART_LOG("1");
	sw_pdev_init();
	AW_UART_LOG("2");
	/* Register platform devices here!! */
}

void __init aw163x_init_early(void)
{
	AW_UART_LOG("1");
}

MACHINE_START(AW163X, "Allwinner AW163x")
	.atag_offset	= 0x100,
	.fixup		= aw163x_fixup,
	.map_io		= aw163x_map_io,
	.init_early	= aw163x_init_early,
	.init_irq	= gic_init_irq,
	.timer		= &aw163x_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= aw163x_init,
#ifdef CONFIG_ZONE_DMA
	.dma_zone_size	= SZ_256M,
#endif
	.restart	= aw163x_restart,
MACHINE_END

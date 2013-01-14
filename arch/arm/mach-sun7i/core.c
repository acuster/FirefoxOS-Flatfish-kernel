/*
 *  arch/arm/mach-sun7i/core.c
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
#include <linux/clockchips.h>
#include <linux/module.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/mach-types.h>
#include <asm/pmu.h>
#include <asm/smp_twd.h>
#include <asm/pgtable.h>
#include <asm/hardware/gic.h>

#include <asm/hardware/cache-l2x0.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include <mach/includes.h>
#include <mach/timer.h>

#include "core.h"

void sw_pdev_init(void);
int arch_timer_common_register(void);

static struct map_desc sun7i_io_desc[] __initdata = {
	{IO_ADDRESS(SW_PA_IO_BASE), __phys_to_pfn(SW_PA_IO_BASE),  SW_IO_SIZE, MT_DEVICE_NONSHARED},
	{IO_ADDRESS(SW_PA_SRAM_A1_BASE), __phys_to_pfn(SW_PA_SRAM_A1_BASE),  SW_SRAM_A1_SIZE, MT_MEMORY_ITCM},
	{IO_ADDRESS(SW_PA_SRAM_A2_BASE), __phys_to_pfn(SW_PA_SRAM_A2_BASE),  SW_SRAM_A2_SIZE, MT_DEVICE_NONSHARED},
	{IO_ADDRESS(SW_PA_SRAM_A3_BASE), __phys_to_pfn(SW_PA_SRAM_A3_BASE),  SW_SRAM_A3_SIZE + SW_SRAM_A4_SIZE, MT_MEMORY_ITCM},
	//{IO_ADDRESS(SW_PA_SRAM_A4_BASE), __phys_to_pfn(SW_PA_SRAM_A4_BASE),  SW_SRAM_A4_SIZE, MT_MEMORY_ITCM}, /* not page align, cause sun7i_map_io err,2013-1-10 */
	{IO_ADDRESS(SW_PA_BROM_START), __phys_to_pfn(SW_PA_BROM_START), SW_BROM_SIZE, MT_DEVICE_NONSHARED},
};

static void __init sun7i_map_io(void)
{
	iotable_init(sun7i_io_desc, ARRAY_SIZE(sun7i_io_desc));
}

static void __init gic_init_irq(void)
{
	gic_init(0, 29, (void *)IO_ADDRESS(AW_GIC_DIST_BASE), (void *)IO_ADDRESS(AW_GIC_CPU_BASE));
}

static void __init sun7i_timer_init(void)
{
	aw_clkevt_init();
	/* to fix, 2013-1-14 */
	//aw_clksrc_init();
	//arch_timer_common_register();
}

static struct sys_timer sun7i_timer = {
	.init		= sun7i_timer_init,
};

static void sun7i_fixup(struct tag *tags, char **from,
			struct meminfo *meminfo)
{
	pr_info("%s(%d)\n", __func__, __LINE__);
	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = PLAT_MEM_SIZE - (SW_FB_MEM_SIZE + SW_GPU_MEM_SIZE + SW_G2D_MEM_SIZE);

	meminfo->nr_banks = 1;
}

u32 g_mem_resv[][2] = {
	{SYS_CONFIG_MEMBASE, SYS_CONFIG_MEMSIZE},
	//{SW_FB_MEM_BASE, 	SW_FB_MEM_SIZE		},
	//{SW_GPU_MEM_BASE, 	SW_GPU_MEM_SIZE		},
	//{SW_G2D_MEM_BASE, 	SW_G2D_MEM_SIZE		},
	{SW_CSI_MEM_BASE, 	SW_CSI_MEM_SIZE		},
	{SW_VE_MEM_BASE, 	SW_VE_MEM_SIZE		},
	{SUPER_STANDBY_BASE,	SUPER_STANDBY_SIZE	},	//for standby: 0x5200,0000-0x5200,0000+64k;
};

static void __init sun7i_reserve(void)
{
	u32 	i = 0;

	pr_info("memory reserved(in bytes):\n");
	for(i = 0; i < ARRAY_SIZE(g_mem_resv); i++) {
		if(0 != memblock_reserve(g_mem_resv[i][0], g_mem_resv[i][1]))
			printk("%s err, line %d, base 0x%08x, size 0x%08x\n", __func__,
				__LINE__, g_mem_resv[i][0], g_mem_resv[i][1]);
		else
			pr_info("\t: 0x%08x, 0x%08x\n", g_mem_resv[i][0], g_mem_resv[i][1]);
	}
}

static void sun7i_restart(char mode, const char *cmd)
{
	pr_info("%s: enter\n", __func__);
}

static void __init sun7i_init(void)
{
	pr_info("%s: enter\n", __func__);
	sw_pdev_init();
	/* Register platform devices here!! */
}

void __init sun7i_init_early(void)
{
	pr_info("%s: enter\n", __func__);
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
	.reserve        = sun7i_reserve,
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
MACHINE_END

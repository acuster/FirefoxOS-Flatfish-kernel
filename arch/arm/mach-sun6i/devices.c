/*
 *  arch/arm/mach-$chip/devices.c
 *
 *  Copyright (C) 2012 AllWinner Limited
 *  Benn Huang <benn@allwinnertech.com>
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


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/pda_power.h>
#include <linux/io.h>
#include <linux/i2c.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/setup.h>
#include <mach/hardware.h>
#include <mach/system.h>
#include <mach/platform.h>

#define ADD_DMA_BY_LIUGANG /* add dma to platform device */

#ifdef ADD_DMA_BY_LIUGANG
/* uart */
static struct plat_serial8250_port debug_uart_platform_data[] = {
	{
		.membase        = (void __iomem *)(IO_ADDRESS(AW_UART0_BASE)),
		.mapbase        = (resource_size_t)AW_UART0_BASE,
		.irq            = 33,
		.flags          = UPF_BOOT_AUTOCONF|UPF_IOREMAP,
		.iotype         = UPIO_MEM32,
		.regshift       = 2,
		.uartclk        = 24000000,
	}, {
		.flags          = 0,
	}
 };

static struct platform_device debug_uart = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &debug_uart_platform_data[0],
	},
};

/* dma */
static u64 sw_dmac_dmamask = DMA_BIT_MASK(32);

static struct resource sw_dmac_resources[] = {
	[0] = {
		.start 	= AW_DMA_BASE,
		.end 	= AW_DMA_BASE + 0x1000, /* lead to boot_secondary->enable_aw_cpu halt? no */
		.flags 	= IORESOURCE_MEM,
	},
	[1] = {
		.start 	= AW_IRQ_DMA,
		.end 	= AW_IRQ_DMA,
		.flags 	= IORESOURCE_IRQ
	}
};

static struct platform_device sw_dmac_device = {
	.name 		= "sw_dmac",	/* must be same as sw_dmac_driver's name */
	.id 		= 0, 		/* there is only one device for sw_dmac dirver, so id is 0 */
	.num_resources 	= ARRAY_SIZE(sw_dmac_resources),
	.resource 	= sw_dmac_resources,
	.dev 		= {
				.dma_mask = &sw_dmac_dmamask,
				.coherent_dma_mask = DMA_BIT_MASK(32),	/* for validate dma_pool_alloc */
				// .platform_data = (void *) &sw_dmac_pdata,
			  },
};

static struct platform_device *sw_pdevs[] __initdata = {
	&debug_uart,
	&sw_dmac_device,
};

void sw_pdev_init(void)
{
	AW_UART_LOG("enter");
	platform_add_devices(sw_pdevs, ARRAY_SIZE(sw_pdevs));
	AW_UART_LOG("leave");
}
#else
/* uart */
static struct plat_serial8250_port debug_uart_platform_data[] = {
	{
		.membase        = (void __iomem *)(IO_ADDRESS(AW_UART0_BASE)),
		.mapbase        = (resource_size_t)AW_UART0_BASE,
		.irq            = 33,
		.flags          = UPF_BOOT_AUTOCONF|UPF_IOREMAP,
		.iotype         = UPIO_MEM32,
		.regshift       = 2,
		.uartclk        = 24000000,
	}, {
		.flags          = 0,
	}
 };

static struct platform_device debug_uart = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &debug_uart_platform_data[0],
	},
};

void sw_pdev_init(void)
{
	AW_UART_LOG("enter");
	platform_device_register(&debug_uart);
	AW_UART_LOG("leave");
}
#endif /* ADD_DMA_BY_LIUGANG */

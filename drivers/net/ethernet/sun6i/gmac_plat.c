/*******************************************************************************
  This contains the functions to handle the platform driver.

  Copyright (C) 2012 Shuge

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/mii.h>
#include <linux/phy.h>

#include "sun6i_gmac.h"
#include "mach/hardware.h"

#ifdef CONFIG_PM
int gmac_suspend(struct net_device *ndev);
int gmac_resume(struct net_device *ndev);
int gmac_freeze(struct net_device *ndev);
int gmac_restore(struct net_device *ndev);

#endif
static int gmac_system_init(struct gmac_priv *priv)
{
	int reg_value;
	int phy_interface = priv->plat->phy_interface;

	if(priv->clkbase){
		reg_value = readl(priv->clkbase + AHB1_GATING);
		writel((reg_value | GMAC_AHB_BIT), priv->clkbase + AHB1_GATING);
	}

	if(phy_interface == PHY_INTERFACE_MODE_RGMII){
		reg_value = readl(priv->clkbase + GMAC_CLK_REG);
		reg_value |= 1<<2;
		writel(reg_value, priv->clkbase + GMAC_CLK_REG);
	} else {
		reg_value = readl(priv->clkbase + GMAC_CLK_REG);
		reg_value &= ~(1<<2);
	}

	/* configure system io */
	if(priv->gpiobase){
		writel(0x22222222, priv->gpiobase + PA_CFG0);

		writel(0x22222222, priv->gpiobase + PA_CFG1);

		reg_value = readl(priv->gpiobase + PA_CFG2);
		reg_value &= 0xffffffaa;
		reg_value |= 0x00000022;
		writel(reg_value, priv->gpiobase + PA_CFG2);
	}

#ifdef SUN7i_GMAC_FPGA
	reg_value = readl(IO_ADDRESS(GPIO_BASE + 0x108));
	reg_value |= 0x1<<20;
	writel(reg_value, IO_ADDRESS(GPIO_BASE + 0x108));

	reg_value = readl(IO_ADDRESS(GPIO_BASE + 0x10c));
	reg_value &= ~(0x1<<29);
	writel(reg_value, IO_ADDRESS(GPIO_BASE + 0x10c));

	mdelay(200);

	reg_value = readl(IO_ADDRESS(GPIO_BASE + 0x10c));
	reg_value |= 0x1<<29;
	writel(reg_value, IO_ADDRESS(GPIO_BASE + 0x10c));
#endif

	return 0;
}

static int gmac_sys_request(struct platform_device *pdev, struct gmac_priv *priv)
{
	int ret = 0;
	struct resource *io_clk, *io_gpio;

	io_clk = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!io_clk)
		return -ENODEV;

	if (!request_mem_region(io_clk->start, resource_size(io_clk), pdev->name)) {
		printk(KERN_ERR "%s: ERROR: memory allocation failed"
		       "cannot get the I/O addr 0x%x\n",
		       __func__, (unsigned int)io_clk->start);
		return -EBUSY;
	}

	priv->clkbase = ioremap(io_clk->start, resource_size(io_clk));
	if (!priv->clkbase) {
		printk(KERN_ERR "%s: ERROR: memory mapping failed", __func__);
		ret = -ENOMEM;
		goto out_release_clk;
	}

	io_gpio = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	if (!io_gpio){
		ret = -ENODEV;
		goto out_unmap_clk;
	}

	if (!request_mem_region(io_gpio->start, resource_size(io_gpio), pdev->name)) {
		printk(KERN_ERR "%s: ERROR: memory allocation failed"
		       "cannot get the I/O addr 0x%x\n",
		       __func__, (unsigned int)io_gpio->start);
		ret = -EBUSY;
		goto out_unmap_clk;
	}

	priv->gpiobase = ioremap(io_gpio->start, resource_size(io_gpio));
	if (!priv->clkbase) {
		printk(KERN_ERR "%s: ERROR: memory mapping failed", __func__);
		ret = -ENOMEM;
		goto out_release_gpio;
	}

	return 0;

out_release_gpio:
	release_mem_region(io_gpio->start, resource_size(io_gpio));
out_unmap_clk:
	iounmap(priv->clkbase);
out_release_clk:
	release_mem_region(io_clk->start, resource_size(io_clk));

	return ret;
}

static void gmac_sys_release(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct gmac_priv *priv = netdev_priv(ndev);
	struct resource *res;

	iounmap((void *)priv->gpiobase);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	release_mem_region(res->start, resource_size(res));

	iounmap((void *)priv->clkbase);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	release_mem_region(res->start, resource_size(res));
}

static int gmac_pltfr_probe(struct platform_device *pdev)
{
	int ret = 0;
	int irq = 0;
	struct resource *io_gmac;
	void __iomem *addr = NULL;
	struct gmac_priv *priv = NULL;
	//struct gmac_plat_data *plat_dat;

	io_gmac = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!io_gmac)
		return -ENODEV;

	if (!request_mem_region(io_gmac->start, resource_size(io_gmac), pdev->name)) {
		pr_err("%s: ERROR: memory allocation failed"
		       "cannot get the I/O addr 0x%x\n",
		       __func__, (unsigned int)io_gmac->start);
		return -EBUSY;
	}

	addr = ioremap(io_gmac->start, resource_size(io_gmac));
	if (!addr) {
		pr_err("%s: ERROR: memory mapping failed", __func__);
		ret = -ENOMEM;
		goto out_release_region;
	}

	/* Get the MAC information */
	irq = platform_get_irq_byname(pdev, "gmacirq");
	if (irq == -ENXIO) {
		printk(KERN_ERR "%s: ERROR: MAC IRQ configuration "
		       "information not found\n", __func__);
		ret = -ENXIO;
		goto out_unmap;
	}

	priv = gmac_dvr_probe(&(pdev->dev), addr, irq);
	if (!priv) {
		printk("[gmac]: %s: main driver probe failed", __func__);
		goto out_unmap;
	}

	if(gmac_sys_request(pdev, priv))
		goto out_unmap;

	gmac_system_init(priv);
	platform_set_drvdata(pdev, priv->ndev);

	printk("[gmac]: sun6i_gmac platform driver registration completed");

	return 0;

out_unmap:
	iounmap(addr);
	platform_set_drvdata(pdev, NULL);

out_release_region:
	release_mem_region(io_gmac->start, resource_size(io_gmac));

	return ret;
}

static int gmac_pltfr_remove(struct platform_device *pdev)
{
	struct net_device *ndev = platform_get_drvdata(pdev);
	struct gmac_priv *priv = netdev_priv(ndev);
	struct resource *res;
	int ret = gmac_dvr_remove(ndev);


	platform_set_drvdata(pdev, NULL);

	iounmap((void *)priv->ioaddr);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));

	gmac_sys_release(pdev);

	return ret;
}

#ifdef CONFIG_PM
static int gmac_pltfr_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	return gmac_suspend(ndev);
}

static int gmac_pltfr_resume(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	return gmac_resume(ndev);
}

int gmac_pltfr_freeze(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	return gmac_freeze(ndev);
}

int gmac_pltfr_restore(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);

	return gmac_restore(ndev);
}

static const struct dev_pm_ops gmac_pltfr_pm_ops = {
	.suspend = gmac_pltfr_suspend,
	.resume = gmac_pltfr_resume,
	.freeze = gmac_pltfr_freeze,
	.thaw = gmac_pltfr_restore,
	.restore = gmac_pltfr_restore,
};
#else
static const struct dev_pm_ops gmac_pltfr_pm_ops;
#endif /* CONFIG_PM */

struct platform_driver gmac_driver = {
	.probe	= gmac_pltfr_probe,
	.remove = gmac_pltfr_remove,
	.driver = {
		   .name = GMAC_RESOURCE_NAME,
		   .owner = THIS_MODULE,
		   .pm = &gmac_pltfr_pm_ops,
		   },
};

static struct resource gmac_resources[] = {
	[0] = {
		.name	= "gmacio",
		.start	= GMAC_BASE,
		.end	= GMAC_BASE + 0x1054,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.name	= "clkbus",
		.start	= CCMU_BASE,
		.end	= CCMU_BASE + GMAC_CLK_REG,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.name	= "gpio",
		.start	= GPIO_BASE,
		.end	= GPIO_BASE + 0x0c,
		.flags	= IORESOURCE_MEM,
	},
	[3] = {
		.name	= "gmacirq",
		.start	= AW_IRQ_GMAC,
		.end	= AW_IRQ_GMAC,
		.flags	= IORESOURCE_IRQ,
	}
};

static struct gmac_mdio_bus_data gmac_mdio_data = {
	.bus_id = 0,
	.phy_reset  = NULL,
	.phy_mask = 0,
	.irqs = NULL,
	.probed_phy_irq = 0,
};

static struct gmac_plat_data gmac_platdata ={
	.bus_id = 0,
	.phy_addr = -1,
	.phy_interface = PHY_INTERFACE_MODE_RGMII,
	.clk_csr = 2,

	.tx_coe = 1,
	.bugged_jumbo = 0,
	.force_sf_dma_mode = 1,
	.pbl = 2,
	.mdio_bus_data = &gmac_mdio_data,
};

struct platform_device gmac_device = {
	.name = GMAC_RESOURCE_NAME,
	.id = -1,
	.resource = gmac_resources,
	.num_resources = ARRAY_SIZE(gmac_resources),
	.dev = {
		.platform_data = &gmac_platdata,
	},
};

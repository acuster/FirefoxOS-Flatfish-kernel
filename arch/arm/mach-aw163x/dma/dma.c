/*
 * arch/arm/mach-aw163x/dma/dma.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma driver interface
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

/**
 * dma_drv_probe - dma driver inital function.
 * @dev:	XXXXXXX
 *
 * Returns 0 if success, otherwise return the err line number.
 */
static int __devinit dma_drv_probe(struct platform_device *dev)
{
	int 	ret = 0;

	DMA_DBG_FUN_LINE;
	ret = dma_init(dev);
	if (ret) {
		DMA_ERR_FUN_LINE;
	} else {
		DMA_DBG_FUN_LINE;
	}

	return ret;
}

/**
 * dma_drv_remove - dma driver deinital function.
 * @dev:	XXXXXXX
 *
 * Returns 0 if success, otherwise means err.
 */
static int __devexit dma_drv_remove(struct platform_device *dev)
{
	int 	ret = 0;

	DMA_DBG_FUN_LINE;
	ret = dma_deinit();
	if (ret) {
		DMA_ERR_FUN_LINE;
	} else {
		DMA_DBG_FUN_LINE;
	}

	return ret;
}

/**
 * dma_drv_suspend - dma driver suspend function.
 * @dev:	XXXXXXX
 * @state:	XXXXXXX
 *
 * Returns 0 if success, otherwise means err.
 */
static int dma_drv_suspend(struct platform_device *dev, pm_message_t state)
{
DMA_DBG_FUN_LINE;
return 0;
}

/**
 * dma_drv_resume - dma driver resume function.
 * @dev:	XXXXXXX
 *
 * Returns 0 if success, otherwise means err.
 */
static int dma_drv_resume(struct platform_device *dev)
{
DMA_DBG_FUN_LINE;
return 0;
}

static struct platform_driver sw_dmac_driver = {
.probe          = dma_drv_probe,
.remove         = __devexit_p(dma_drv_remove),
.suspend        = dma_drv_suspend,
.resume         = dma_drv_resume,
.driver         = {
.name   = "sw_dmac",
.owner  = THIS_MODULE,
},
};

/**
 * drv_dma_init - dma driver register function
 *
 * Returns 0 if success, otherwise means err.
 */
static int __init drv_dma_init(void)
{
	platform_driver_register(&sw_dmac_driver);
	return 0;
}


arch_initcall(drv_dma_init);

/*
 * arch/arm/mach-sun6i/dma/dma_include.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_INCLUDE_H
#define __DMA_INCLUDE_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <mach/platform.h>

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/types.h>

#define USE_UNCACHED_FOR_DESMGR /* use uncached for des manager, 2012-6-20 */

//#define TEST_LOCK_IMPROVE_FOR_ALLOC /* unlock when dma_pool_alloc/free called, for singe mode only, 2012-6-22 */

#include <mach/dma.h>
#include "dma_regs.h"
#include "dma_common.h"
#include "dma_csp.h"
#include "dma_ctrl.h"
#include "dma_enqueue.h"
#include "dma_init.h"
#include "dma_interface.h"
#include "dma_irq_hd.h"
#include "dma_single.h"

#ifdef DBG_DMA
#include <linux/delay.h>
#endif /* DBG_DMA */

#endif  /* __DMA_INCLUDE_H */

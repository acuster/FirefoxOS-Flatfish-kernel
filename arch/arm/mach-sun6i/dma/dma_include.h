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
#include <linux/clk.h>
#include <linux/pm.h>

//#define TEMP_FOR_XJF_20121121 	/* alloc des buf in request, temp for xujinfeng, 2012-11-21 */

#include <mach/dma.h>
#include <mach/clock.h>
#include "dma_regs.h"
#include "dma_common.h"
#include "dma_csp.h"
#include "dma_interface.h"
#include "dma_single.h"
#include "dma_chain.h"

#ifdef TEMP_FOR_XJF_20121121
#define TEMP_DES_CNT	20
extern u32 index_get, index_put;
extern u32 v_addr, p_addr;
#endif /* TEMP_FOR_XJF_20121121 */

#ifdef DBG_DMA
#include <linux/delay.h>
#endif /* DBG_DMA */

#endif  /* __DMA_INCLUDE_H */


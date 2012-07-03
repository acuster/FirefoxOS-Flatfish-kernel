/*
 * arch/arm/mach-sun6i/dma/dma_interface.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sun6i dma header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_INTERFACE_H
#define __DMA_INTERFACE_H

extern struct dma_mgr_t  g_dma_mgr;
#ifdef USE_UNCACHED_FOR_DESMGR
extern struct dma_pool	 *g_pdes_mgr;
#else
extern struct kmem_cache *g_pdes_mgr;
#endif /* USE_UNCACHED_FOR_DESMGR */
extern struct dma_pool	 *g_pool_ch;

extern struct dma_pool	 *g_pool_sg;

u32 dma_check_handle(dm_hdl_t dma_hdl);
extern unsigned long addrtype_arr[];
extern unsigned long xfer_arr[];

#endif  /* __DMA_INTERFACE_H */

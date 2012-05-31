/*
 * arch/arm/mach-aw163x/dma/dma_interface.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma header file
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
extern struct kmem_cache *g_pdma_des_mgr;
extern struct kmem_cache *g_pdma_des;


#endif  /* __DMA_INTERFACE_H */

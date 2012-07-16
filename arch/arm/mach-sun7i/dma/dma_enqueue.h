/*
 * arch/arm/mach-sun7i/dma/dma_enqueue.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun7i dma header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_ENQUEUE_H
#define __DMA_ENQUEUE_H

u32 dma_clean_des(struct dma_channel_t *pchan);
u32 dma_chan_init_main_des(struct dma_channel_t *pchan);
u32 dma_enqueue(dm_hdl_t dma_hdl, struct cofig_des_t *pdes, enum dma_enque_phase_e phase);


#endif  /* __DMA_ENQUEUE_H */

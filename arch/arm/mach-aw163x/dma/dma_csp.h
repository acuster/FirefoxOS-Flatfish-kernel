/*
 * arch/arm/mach-aw163x/dma/dma_csp.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma csp header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_CSP_H
#define __DMA_CSP_H

void csp_dma_init(void);
u32  csp_dma_clear_irqpend(u32 index);

void csp_dma_chan_set_startaddr(struct dma_channel_t * pchan, u32 ustart_addr);
void csp_dma_chan_start(struct dma_channel_t * pchan);
void csp_dma_chan_pause(struct dma_channel_t * pchan);
void csp_dma_chan_resume(struct dma_channel_t * pchan);
void csp_dma_chan_stop(struct dma_channel_t * pchan);
u32  csp_dma_chan_get_startaddr(struct dma_channel_t *pchan);
u32  csp_dma_chan_get_left_bytecnt(struct dma_channel_t * pchan);
u32  csp_dma_chan_get_cur_dstaddr(struct dma_channel_t * pchan);
u32  csp_dma_chan_get_cur_srcaddr(struct dma_channel_t * pchan);
u32  csp_dma_chan_get_status(struct dma_channel_t * pchan);
void csp_dma_chan_irq_enable(struct dma_channel_t * pchan, u32 irq_type);
void csp_dma_chan_clear_irqpend(struct dma_channel_t * pchan, u32 irq_type);
u32  csp_dma_chan_get_irqpend(struct dma_channel_t * pchan);

#endif  /* __DMA_CSP_H */

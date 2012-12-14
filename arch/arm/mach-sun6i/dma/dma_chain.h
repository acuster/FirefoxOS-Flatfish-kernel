/*
 * arch/arm/mach-sun6i/dma/dma_chain_new.h
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

#ifndef __DMA_CHAIN_NEW_H
#define __DMA_CHAIN_NEW_H

void dma_irq_hdl_chain(struct dma_channel_t *pchan, u32 upend_bits);
u32 dma_request_init_chain(struct dma_channel_t *pchan);
void dma_dump_chain(struct dma_channel_t *pchan);
u32 dma_enqueue_chain(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt);
u32 dma_config_chain(dm_hdl_t dma_hdl, struct dma_config_t *pcfg);
u32 dma_ctrl_chain(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg);
u32 dma_release_chain(dm_hdl_t dma_hdl);

#endif  /* __DMA_CHAIN_NEW_H */


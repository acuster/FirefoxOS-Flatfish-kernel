/*
 * arch/arm/mach-sun6i/dma/dma_single.h
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

#ifndef __DMA_SINGLE_H
#define __DMA_SINGLE_H

/*
 * XXX
 */
struct des_item_t {
	struct cofig_des_t	des;     	/* XXX */
	u32 			paddr;		/* paddr of des_item_t */
	struct list_head	list;     	/* XXX */
};

u32 dma_irq_hdl_sgmd(struct dma_channel_t *pchan, u32 upend_bits);

u32 dma_enqueue_single(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt,
				enum dma_enque_phase_e phase);
u32 dma_config_single(dm_hdl_t dma_hdl, struct dma_config_t *pcfg, enum dma_enque_phase_e phase);
u32 dma_ctrl_single(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg);
u32 dma_release_single(dm_hdl_t dma_hdl);

#endif  /* __DMA_SINGLE_H */

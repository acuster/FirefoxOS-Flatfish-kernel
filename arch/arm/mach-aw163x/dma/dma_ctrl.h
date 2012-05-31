/*
 * arch/arm/mach-aw163x/dma/dma_ctrl.h
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

#ifndef __DMA_CTRL_H
#define __DMA_CTRL_H

u32 dma_start(dm_hdl_t dma_hdl);
u32 dma_pause(dm_hdl_t dma_hdl);
u32 dma_resume(dm_hdl_t dma_hdl);
u32 dma_stop(dm_hdl_t dma_hdl);
u32 dma_get_status(dm_hdl_t dma_hdl, u32 *pval);
u32 dma_get_cur_src_addr(dm_hdl_t dma_hdl, u32 *pval);
u32 dma_get_cur_dst_addr(dm_hdl_t dma_hdl, u32 *pval);
u32 dma_get_left_bytecnt(dm_hdl_t dma_hdl, u32 *pval);
u32 dma_set_op_cb(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb);
u32 dma_set_hd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb);
u32 dma_set_fd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb);
u32 dma_set_qd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb);


#endif  /* __DMA_CTRL_H */

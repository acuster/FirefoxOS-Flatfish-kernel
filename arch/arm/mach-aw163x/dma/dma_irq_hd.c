/*
 * arch/arm/mach-aw163x/dma/dma_irq_hd.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

/**
 * __dma_chan_handle_hd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_handle_hd(struct dma_channel_t *pchan)
{
	dma_cb 		func = pchan->hd_cb.func;
	void 		*parg = pchan->hd_cb.parg;

	if(NULL != func)
		return func((dm_hdl_t)pchan, parg, DMA_CB_OK);

	return 0;
}

/**
 * __dma_chan_handle_fd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_handle_fd(struct dma_channel_t *pchan)
{
	dma_cb 		func = pchan->fd_cb.func;
	void 		*parg = pchan->fd_cb.parg;

	if(NULL != func)
		return func((dm_hdl_t)pchan, parg, DMA_CB_OK);

	return 0;
}

/**
 * __dma_chan_handle_qd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_handle_qd(struct dma_channel_t *pchan)
{
	u32 		uRet = 0;
	unsigned long	flags = 0;

	/*
	 * should not queue done in continue mode
	 */
	if(true == pchan->bconti_mode) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	/*
	 * call the callback first
	 * we cannot lock fd_cb function, in case sw_dma_enqueue
	 * called and locked agin, which may lead to BUG somewhere
	 */
	//DMA_CHAN_LOCK(&pchan->lock, flags);
	if(NULL != pchan->qd_cb.func) {
		if(0 != pchan->qd_cb.func((dm_hdl_t)pchan, pchan->qd_cb.parg, DMA_CB_OK)) {
			DMA_ERR_FUN_LINE;
			return 0;
		}
	}
	//DMA_CHAN_UNLOCK(&pchan->lock, flags);

	DMA_CHAN_LOCK(&pchan->lock, flags);

	/*
	 * stopped when hd_cb calling?
	 */
	if(DMA_CHAN_STA_IDLE == pchan->state) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR("%s err, line %d, state idle, stopped during hd_cb/fd_cb/qd_cb or somewhere else? just return ok\n", \
			__FUNCTION__, __LINE__);
		return 0;
	}

	/*
	 * no changed to wait_qd in normal/hd_cb/fd_cb/qd_cb queueing, the des is old,
	 * clear des and change state to done
	 */
	if(DMA_CHAN_STA_RUNING == pchan->state) {
		/* free the extra des, clear des */
		if(0 != dma_clean_des(pchan)) {
			uRet = __LINE__;
			goto End;
		}

		/* change state to done */
		pchan->state = DMA_CHAN_STA_DONE;

		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		return 0;
	}

	/*
	 * changed to wait_qd before(in normal/hd_cb/fd_cb/qd_cb queueing), the des is new,
	 * run the new buffer chain
	 */
	if(DMA_CHAN_STA_WAIT_QD == pchan->state) {
		DMA_DBG_FUN_LINE;
		/* start the new queue(from begin) */
		if(NULL != pchan->op_cb.func) {
			if(0 != pchan->op_cb.func((dm_hdl_t)pchan, pchan->op_cb.parg, DMA_OP_START)) {
				uRet = __LINE__;
				goto End;
			}
		}
		if(0 != dma_start(pchan)) {
			uRet = __LINE__;
			goto End;
		}

		/* change state to running */
		//pchan->state = DMA_CHAN_STA_RUNING; /* done in dma_start */

		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		return 0;
	}

	/*
	 * should never be done
	 */
	if(DMA_CHAN_STA_DONE == pchan->state) {
		uRet = __LINE__;
		goto End;
	}

End:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
	}

	return 0;
}

/**
 * dma_irq_hdl - dma irq process function
 * @irq:	dma physical irq num
 * @dev:	XXXXXXX
 *
 * Returns 0 if sucess, the err line number if failed.
 *
 * we cannot lock dma_irq_hdl through,
 * because sw_dma_enqueue maybe called in cb,
 * which will result in deadlock
 */
irqreturn_t dma_irq_hdl(int irq, void *dev)
{
	u32 		i = 0;
	u32 		uline = 0;
	u32 		upend_bits = 0;
	u32 		uirq_spt = 0;
	struct dma_channel_t *pchan = NULL;
	struct dma_mgr_t *pdma_mgr = NULL;

	pdma_mgr = (struct dma_mgr_t *)dev;
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &pdma_mgr->chnl[i];
		uirq_spt = pdma_mgr->chnl[i].irq_spt;

		/* get channel irq pend bits */
		upend_bits = csp_dma_chan_get_irqpend(pchan);
		if(0 == upend_bits)
			continue;

		/* deal half done */
		if((upend_bits & CHAN_IRQ_HD) && (uirq_spt & CHAN_IRQ_HD)) {
			csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD);
			if(0 != __dma_chan_handle_hd(pchan)) {
				uline = __LINE__;
				goto End;
			}
		}

		/* deal full done */
		if((upend_bits & CHAN_IRQ_FD) && (uirq_spt & CHAN_IRQ_FD)) {
			csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_FD);
			if(0 != __dma_chan_handle_fd(pchan)) {
				uline = __LINE__;
				goto End;
			}
		}

		/* deal queue done */
		if((upend_bits & CHAN_IRQ_QD) && (uirq_spt & CHAN_IRQ_QD)) {
			csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_QD);
			if(0 != __dma_chan_handle_qd(pchan)) {
				uline = __LINE__;
				goto End;
			}
		}
	}

End:
	if(0 != uline) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uline);
	}

	return IRQ_HANDLED;
}

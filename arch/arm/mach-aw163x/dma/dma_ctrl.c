/*
 * arch/arm/mach-aw163x/dma/dma_ctrl.c
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
 * __dma_dump_buf_chain - dump dma buf chain
 * @pchan:	dma handle
 */
void __dma_dump_buf_chain(struct dma_channel_t *pchan)
{
	u32 	i = 0;
	struct des_mgr_t *pdes_mgr = pchan->pdes_mgr;

	DMA_INF("+++++++++++%s+++++++++++\n", __FUNCTION__);

	while(NULL != pdes_mgr) {
		if(0 == pdes_mgr->des_num) {
			DMA_DBG_FUN_LINE; /* des_num never be 0 since pdes_mgr valid */
			break;
		}

		DMA_INF("pdes_mgr 0x%08x, des_num %d\n", (u32)pdes_mgr, pdes_mgr->des_num);
		for(i = 0; i < pdes_mgr->des_num; i++) {
			DMA_INF("cofig/saddr/daddr/bcnt/param/pnext:0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", \
				pdes_mgr->pdes[i].cofig, pdes_mgr->pdes[i].saddr, pdes_mgr->pdes[i].daddr, \
				pdes_mgr->pdes[i].bcnt, pdes_mgr->pdes[i].param, (u32)pdes_mgr->pdes[i].pnext);
		}
		DMA_INF("\n");

		pdes_mgr = pdes_mgr->pnext;
	}

	DMA_INF("-----------%s-----------\n", __FUNCTION__);
}

/**
 * dma_start - start the dma des queue, change state to running
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 *
 * start op can in these state:
 * idle: 	the first start, or after stop. make sure queue is valid.
 * wait_qd: 	need restart so state from running to wait_qd in hd_cb/fd_cb/qd_cb/normal queueing, des is new.
 *  		  so let __dma_chan_handle_qd start the new queue(from begin of des)
 * done: 	__dma_chan_handle_qd has done the queue, and no new buffer queued, handle_qd
 * 		 will clear des, change state to done, if sw_dma_enqueue(normal) queued new buffer after
 *		 that, it will start the buffer.
 */
u32 dma_start(dm_hdl_t dma_hdl)
{
	u32	uRet = 0;
	u32 	udes_paddr = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(NULL == pchan || NULL == pchan->pdes_mgr
		|| NULL == pchan->pdes_mgr->pdes
		|| 0 == pchan->pdes_mgr->des_pa) {
		uRet = __LINE__;
		goto End;
	}
	if(0 == pchan->pdes_mgr->des_num) {
		uRet = __LINE__;
		goto End;
	}

	/* virt to phys */
	//udes_paddr = virt_to_phys((void *)pchan->pdes_mgr->pdes); /* err */
	udes_paddr = pchan->pdes_mgr->des_pa;
	DMA_WRITE_REG(udes_paddr, pchan->reg_base + DMA_OFF_REG_START);

	//DMA_DBG("%s: write 0x%08x to reg 0x%08x\n", __FUNCTION__, udes_paddr, pchan->reg_base + DMA_OFF_REG_START);

	/* start dma */
	csp_dma_chan_start(pchan);

	/* change state to running */
	pchan->state = DMA_CHAN_STA_RUNING;

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __FUNCTION__, uRet, (u32)dma_hdl);
	}

	return uRet;
}

/**
 * dma_pause - pause a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_pause(dm_hdl_t dma_hdl)
{
	/*
	 * do something if needed...
	 */

	csp_dma_chan_pause((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_resume - resume a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_resume(dm_hdl_t dma_hdl)
{
	/*
	 * do something if needed...
	 */

	csp_dma_chan_resume((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_stop - stop a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_stop(dm_hdl_t dma_hdl)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

#ifdef DBG_DMA
	DMA_INF("%s: state %d, buf chain: \n", __FUNCTION__, (u32)pchan->state);
	__dma_dump_buf_chain(pchan);

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_INF("%s: state idle, maybe before start or already stopped\n", __FUNCTION__);
		break;
	case DMA_CHAN_STA_RUNING:
		DMA_INF("%s: state running, so stop the channel, abort the transfer, and free extra des, \
			init main des\n", __FUNCTION__);
		break;
	case DMA_CHAN_STA_DONE:
		DMA_INF("%s: state done, just stop the channel(des already cleared and inited in qd handle)\n", \
			__FUNCTION__);
		break;
	case DMA_CHAN_STA_WAIT_QD:
		DMA_INF("%s: state wait_qd, so stop the channel, abort the transfer, and free extra des, \
			init main des\n", __FUNCTION__);
		break;
	default:
		DMA_ERR_FUN_LINE;
		break;
	}
#endif /* DBG_DMA */

	/*
	 * abort dma transfer if state is running or wait_qd
	 */
	DMA_DBG_FUN_LINE_TOCHECK;
	if(DMA_CHAN_STA_RUNING == pchan->state
		|| DMA_CHAN_STA_WAIT_QD == pchan->state) {
		if(NULL != pchan->qd_cb.func) {
			if(0 != pchan->qd_cb.func(dma_hdl, pchan->qd_cb.parg, DMA_CB_ABORT)) {
				uret = __LINE__;
				goto End;
			}
		}
	}

	/*
	 * stop dma channle and clear irq pending
	 */
	csp_dma_chan_stop(pchan);
	csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD);

	/*
	 * free the extra des(except the main des)
	 */
	if(0 != dma_clean_des(pchan)) {
		uret = __LINE__;
		goto End;
	}

	/* change channel state to idle */
	pchan->state = DMA_CHAN_STA_IDLE;

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * dma_get_status - get hardware status of dma channel
 * @dma_hdl:	dma handle
 * @pval:	value to get
 *
 * return 0.
 */
u32 dma_get_status(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_status((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_get_cur_src_addr - get dma channel's cur src addr reg value
 * @dma_hdl:	dma handle
 * @pval:	value to get
 *
 * return 0.
 */
u32 dma_get_cur_src_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_cur_srcaddr((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_get_cur_dst_addr - get dma channel's cur dst addr reg value
 * @dma_hdl:	dma handle
 * @pval:	value to get
 *
 * return 0.
 */
u32 dma_get_cur_dst_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_cur_dstaddr((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_get_left_bytecnt - get dma channel's left byte cnt
 * @dma_hdl:	dma handle
 * @pval:	value to get
 *
 * return 0.
 */
u32 dma_get_left_bytecnt(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_left_bytecnt((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_set_op_cb - set dma operation callback function
 * @dma_hdl:	dma handle
 * @pCb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_op_cb(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->op_cb.func = pcb->func;
	pchan->op_cb.parg = pcb->parg;
	return 0;
}

/**
 * dma_set_hd_cb - set dma half done callback function
 * @dma_hdl:	dma handle
 * @pCb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_hd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->hd_cb.func = pcb->func;
	pchan->hd_cb.parg = pcb->parg;
	return 0;
}

/**
 * dma_set_fd_cb - set dma full done callback function
 * @dma_hdl:	dma handle
 * @pCb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_fd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->fd_cb.func = pcb->func;
	pchan->fd_cb.parg = pcb->parg;
	return 0;
}

/**
 * dma_set_qd_cb - set dma queue done callback function
 * @dma_hdl:	dma handle
 * @pcb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_qd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->qd_cb.func = pcb->func;
	pchan->qd_cb.parg = pcb->parg;
	return 0;
}

/*
 * arch/arm/mach-sun6i/dma/dma_single.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

/* 找到链上第一个buffer, 移出队列, start它 */
u32 __dma_start(dm_hdl_t dma_hdl)
{
	u32	uret = 0;
	struct des_item_t	*pdes_item = NULL;
	struct dma_channel_t 	*pchan = (struct dma_channel_t *)dma_hdl;

	if(list_empty(&pchan->buf_list_head)) {
		uret = __LINE__;
		goto End;
	}

	/* remove from list */
	pdes_item = list_entry(pchan->buf_list_head.next, struct des_item_t, list);
	list_del(&pdes_item->list); /* 只是从链表移除, 未释放空间 */

	//udes_paddr = pchan->pdes_mgr->des_pa;
	//DMA_WRITE_REG(udes_paddr, pchan->reg_base + DMA_OFF_REG_START);
	DMA_WRITE_REG(pdes_item->paddr, pchan->reg_base + DMA_OFF_REG_START);

	csp_dma_chan_start(pchan);

	STATE_SGL(pchan) = SINGLE_STA_RUNING;

	pchan->pcur_des = pdes_item;

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __FUNCTION__, uret, (u32)dma_hdl);
	}
	return uret;
}

u32 __dma_pause(dm_hdl_t dma_hdl)
{
	csp_dma_chan_pause((struct dma_channel_t *)dma_hdl);
	return 0;
}

u32 __dma_resume(dm_hdl_t dma_hdl)
{
	csp_dma_chan_resume((struct dma_channel_t *)dma_hdl);
	return 0;
}

/* not include cur buf */
u32 __dma_free_buflist(struct dma_channel_t *pchan)
{
	u32	utemp = 0;
	struct des_item_t *pdes_item = NULL;

	while (!list_empty(&pchan->buf_list_head)) {
		pdes_item = list_entry(pchan->buf_list_head.next, struct des_item_t, list);
		utemp = pdes_item->paddr;
		list_del(&pdes_item->list);
		dma_pool_free(g_pool_sg, pdes_item, utemp);
	}
	return 0;
}

/* include cur buf */
u32 __dma_free_allbuf(struct dma_channel_t *pchan)
{
	u32	utemp = 0;

	if(NULL != pchan->pcur_des) {
		utemp = pchan->pcur_des->paddr;
		dma_pool_free(g_pool_sg, pchan->pcur_des, utemp);
		pchan->pcur_des = NULL;
	}

	__dma_free_buflist(pchan);
	return 0;
}

void __dump_buf_list_sgmd(struct dma_channel_t *pchan)
{
	struct list_head *p, *n;
	struct des_item_t *pitem = NULL;

	DMA_INF("+++++++++++%s+++++++++++\n", __FUNCTION__);
	list_for_each_safe(p, n, &pchan->buf_list_head) {
		pitem = list_entry(p, struct des_item_t, list);
		DMA_INF("des[%d]: 0x%08x\n", i++, (u32)&pitem->des);
	}
	DMA_INF("-----------%s-----------\n", __FUNCTION__);
}

/* abort回调, 硬件上stop, 释放当前buffer(如果有), 释放list buffer */
u32 __dma_stop(dm_hdl_t dma_hdl)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	DMA_INF("%s: state %d, buf chain: \n", __FUNCTION__, (u32)STATE_SGL(pchan));
	__dump_buf_list_sgmd(pchan);

	switch(STATE_SGL(pchan)) {
	case SINGLE_STA_IDLE:
		DMA_INF("%s: state idle, maybe before start or after stop, so stop the channel, free all buf list\n", __FUNCTION__);
		DMA_ASSERT(NULL == pchan->pcur_des);
		break;
	case SINGLE_STA_RUNING:
		DMA_INF("%s: state running, so stop the channel, abort the cur buf, and free extra buf\n", __FUNCTION__);
		DMA_ASSERT(NULL != pchan->pcur_des);
		break;
	case SINGLE_STA_LAST_DONE:
		DMA_INF("%s: state last done, so stop the channel, buffer already freed all, to check\n", __FUNCTION__);
		DMA_ASSERT(NULL == pchan->pcur_des && list_empty(&pchan->buf_list_head));
		break;
	default:
		uret = __LINE__;
		goto End;
	}

	/*
	 * stop dma channle and clear irq pending
	 */
	csp_dma_chan_stop(pchan);
	csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD);

	/*
	 * abort dma transfer if state is running or last done
	 */
	if(SINGLE_STA_RUNING == STATE_SGL(pchan)
		|| SINGLE_STA_LAST_DONE == STATE_SGL(pchan)) {
		if(NULL != pchan->qd_cb.func) {
			if(0 != pchan->qd_cb.func(dma_hdl, pchan->qd_cb.parg, DMA_CB_ABORT)) {
				uret = __LINE__;
				goto End;
			}
		}
	}

	/* free buffer list */
	__dma_free_allbuf(pchan);

	/* change channel state to idle */
	STATE_SGL(pchan) = SINGLE_STA_IDLE;

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}

u32 __dma_get_status(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_status((struct dma_channel_t *)dma_hdl);
	return 0;
}

u32 __dma_get_cur_src_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_cur_srcaddr((struct dma_channel_t *)dma_hdl);
	return 0;
}

u32 __dma_get_cur_dst_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_cur_dstaddr((struct dma_channel_t *)dma_hdl);
	return 0;
}

u32 __dma_get_left_bytecnt(dm_hdl_t dma_hdl, u32 *pval)
{
	*pval = csp_dma_chan_get_left_bytecnt((struct dma_channel_t *)dma_hdl);
	return 0;
}

u32 __dma_set_op_cb(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(SINGLE_STA_IDLE != STATE_SGL(pchan)) {
		DMA_ERR_FUN_LINE;
	}
	pchan->op_cb.func = pcb->func;
	pchan->op_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_set_hd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(SINGLE_STA_IDLE != STATE_SGL(pchan)) {
		DMA_ERR_FUN_LINE;
	}
	pchan->hd_cb.func = pcb->func;
	pchan->hd_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_set_fd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(SINGLE_STA_IDLE != STATE_SGL(pchan)) {
		DMA_ERR_FUN_LINE;
	}
	pchan->fd_cb.func = pcb->func;
	pchan->fd_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_set_qd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(SINGLE_STA_IDLE != STATE_SGL(pchan)) {
		DMA_ERR_FUN_LINE;
	}
	pchan->qd_cb.func = pcb->func;
	pchan->qd_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_enqueue(dm_hdl_t dma_hdl, struct cofig_des_t *pdes, enum dma_enque_phase_e phase)
{
	u32 	uret = 0;
	u32 	utemp = 0;
	struct dma_channel_t 	*pchan = (struct dma_channel_t *)dma_hdl;
	struct des_item_t	*pdes_itm = NULL;

	pdes_itm = (struct des_item_t *)dma_pool_alloc(g_pool_sg, GFP_ATOMIC, &utemp);
	if (NULL == pdes_itm) {
		uret = __LINE__;
		goto End;
	}
	pdes_itm->des = *pdes;
	pdes_itm->paddr = utemp;

//	不管状态怎样, enqueue to list
	list_add_tail(&pdes_itm->list, &pchan->buf_list_head);

	if(SINGLE_STA_LAST_DONE == STATE_SGL(pchan)) {
		if(ENQUE_PHASE_NORMAL != phase)
			DMA_ERR_FUN_LINE;
		DMA_DBG_FUN_LINE;
		if(0 != __dma_start(dma_hdl)) {
			uret = __LINE__;
			goto End;
		}
	}
End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}

u32 __handle_hd_sgmd(struct dma_channel_t *pchan)
{
	dma_cb 		func = pchan->hd_cb.func;
	void 		*parg = pchan->hd_cb.parg;

	if(NULL != func)
		return func((dm_hdl_t)pchan, parg, DMA_CB_OK);

	return 0;
}

u32 __handle_fd_sgmd(struct dma_channel_t *pchan)
{
	dma_cb 		func = pchan->fd_cb.func;
	void 		*parg = pchan->fd_cb.parg;

	if(NULL != func)
		return func((dm_hdl_t)pchan, parg, DMA_CB_OK);

	return 0;
}

u32 __handle_qd_sgmd(struct dma_channel_t *pchan)
{
	u32 		uret = 0;
	u32 		utemp = 0;
	unsigned long	flags = 0;
	enum st_md_single_e cur_state = 0;

	/* cannot lock fd_cb function, in case sw_dma_enqueue called and locked agin */
	if(NULL != pchan->qd_cb.func) {
		if(0 != pchan->qd_cb.func((dm_hdl_t)pchan, pchan->qd_cb.parg, DMA_CB_OK)) {
			DMA_ERR_FUN_LINE;
			return 0;
		}
	}

	DMA_CHAN_LOCK(&pchan->lock, flags);

	cur_state = STATE_SGL(pchan);

	/* stopped when hd_cb/fd_cb/qd_cb/somewhere calling? */
	if(SINGLE_STA_IDLE == cur_state) {
		DMA_ASSERT(NULL == pchan->pcur_des);
		DMA_INF("%s: state idle, stopped in cb before? just return ok!\n", __FUNCTION__);
		goto End;
	} else if(SINGLE_STA_RUNING == cur_state) {
		/* for continue mode, just re start the cur buffer */
		if(unlikely(true == pchan->bconti_mode)) {
			DMA_ASSERT(true == list_empty(&pchan->buf_list_head));
			list_add_tail(&pchan->pcur_des->list, &pchan->buf_list_head);
			uret = __dma_start((dm_hdl_t)pchan);
			goto End;
		}

		/* for no-continue mode, free cur buf and start the next buf in chain */
		DMA_ASSERT(NULL != pchan->pcur_des);
		utemp = pchan->pcur_des->paddr;
		dma_pool_free(g_pool_sg, pchan->pcur_des, utemp);
		pchan->pcur_des = NULL;

		/* start next if there is, or change to last done */
		if(!list_empty(&pchan->buf_list_head)) {
			if(0 != __dma_start((dm_hdl_t)pchan)) {
				uret = __LINE__;
				goto End;
			}
		} else {
			DMA_DBG_FUN_LINE;
			STATE_CHAIN(pchan) = SINGLE_STA_LAST_DONE; /* change state to done */
		}
		goto End;
	} else { /* should never be last done) */
		uret = __LINE__;
		goto End;
	}

End:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * dma_release_single - release dma channel, for single mode
 * @dma_hdl:	dma handle
 *
 * return 0 if success, the err line number if not
 */
u32 dma_release_single(dm_hdl_t dma_hdl)
{
	unsigned long 	flags = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	DMA_CHAN_LOCK(&pchan->lock, flags);

#ifdef DBG_DMA
	if(0 != dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* if not idle, call stop first */
	if(SINGLE_STA_IDLE != STATE_SGL(pchan)) {
		DMA_INF("%s maybe err: line %d, state(%d) not idle, call stop dma first!\n", \
			__FUNCTION__, __LINE__, STATE_SGL(pchan));
		if(0 != __dma_stop(dma_hdl)) {
			DMA_ERR_FUN_LINE;
		}
	}

	//memset(pchan, 0, sizeof(*pchan)); /* donot do that, because id...shouldnot be cleared */
	pchan->used 	= 0;
	memset(pchan->owner, 0, sizeof(pchan->owner));

	pchan->irq_spt 	= CHAN_IRQ_NO;
	pchan->bconti_mode = false;
	memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));

	pchan->work_mode = DMA_WORK_MODE_INVALID;

	memset(&pchan->op_cb, 0, sizeof(pchan->op_cb));
	memset(&pchan->hd_cb, 0, sizeof(pchan->hd_cb));
	memset(&pchan->fd_cb, 0, sizeof(pchan->fd_cb));
	memset(&pchan->qd_cb, 0, sizeof(pchan->qd_cb));

	/* maybe enqueued but not started, so free buf */
	DMA_ASSERT(NULL == pchan->pcur_des);
	__dma_free_buflist(pchan);

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	return 0;
}

/**
 * dma_ctrl_single - dma ctrl, for single mode
 * @dma_hdl:	dma handle
 * @op:		dma operation type
 * @parg:	arg for the op
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_ctrl_single(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg)
{
	u32		uret = 0;
	unsigned long	flags = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in start/stop/pause/resume case can parg be NULL  */
	if((NULL == parg)
		&& (DMA_OP_START != op)
		&& (DMA_OP_PAUSE != op)
		&& (DMA_OP_RESUME != op)
		&& (DMA_OP_STOP != op)) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	DMA_CHAN_LOCK(&pchan->lock, flags);

#ifdef DBG_DMA
	if(0 != dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* let the caller to do some operation before op */
	if((DMA_OP_SET_OP_CB != op) && (NULL != pchan->op_cb.func)) {
		if(0 != pchan->op_cb.func(dma_hdl, pchan->op_cb.parg, op))
			DMA_ERR_FUN_LINE;
	}

	switch(op) {
	case DMA_OP_START:
		uret = __dma_start(dma_hdl);
		break;
	case DMA_OP_PAUSE:
		uret = __dma_pause(dma_hdl);
		break;
	case DMA_OP_RESUME:
		uret = __dma_resume(dma_hdl);
		break;
	case DMA_OP_STOP:
		uret = __dma_stop(dma_hdl);
		break;

	case DMA_OP_GET_STATUS:
		dma_get_status(dma_hdl, parg);
		break;
	case DMA_OP_GET_CUR_SRC_ADDR:
		dma_get_cur_src_addr(dma_hdl, parg);
		break;
	case DMA_OP_GET_CUR_DST_ADDR:
		dma_get_cur_dst_addr(dma_hdl, parg);
		break;
	case DMA_OP_GET_BYTECNT_LEFT:
		dma_get_left_bytecnt(dma_hdl, parg);
		break;

	case DMA_OP_SET_OP_CB:
		uret = __dma_set_op_cb(dma_hdl, (struct dma_op_cb_t *)parg);
		break;
	case DMA_OP_SET_HD_CB:
		uret = __dma_set_hd_cb(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_FD_CB:
		uret = __dma_set_fd_cb(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_QD_CB:
		uret = __dma_set_qd_cb(dma_hdl, (struct dma_cb_t *)parg);
		break;
	default:
		uret = __LINE__;
		goto End;
	}

End:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uret) {
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __FUNCTION__, uret, (u32)dma_hdl);
	}
	return uret;
}

/**
 * dma_config_single - config dma channel, enqueue the buffer, for single mode only
 * @dma_hdl:	dma handle
 * @pcfg:	dma cofig para
 * @phase:	dma enqueue phase
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_config_single(dm_hdl_t dma_hdl, struct dma_config_t *pcfg, enum dma_enque_phase_e phase)
{
	u32 		uret = 0;
	u32		uConfig = 0;
	unsigned long	flags = 0;
	struct cofig_des_t	des;
	struct dma_channel_t	*pchan = (struct dma_channel_t *)dma_hdl;

	/* get dma config val */
	uConfig |= xfer_arr[pcfg->xfer_type]; /* src/dst burst length and data width */
	uConfig |= addrtype_arr[pcfg->address_type]; /* src/dst address mode */
	uConfig |= (pcfg->src_drq_type << DMA_OFF_BITS_SDRQ)
			| (pcfg->dst_drq_type << DMA_OFF_BITS_DDRQ); /* src/dst drq type */

	/* fill cofig_des_t struct */
	memset(&des, 0, sizeof(des));
	des.cofig = uConfig;
	des.saddr = pcfg->src_addr;
	des.daddr = pcfg->dst_addr;
	des.bcnt  = pcfg->byte_cnt;
	des.param = pcfg->para;
	des.pnext = (struct cofig_des_t *)DMA_END_DES_LINK;

	/* get continue mode flag */
	pchan->bconti_mode = pcfg->bconti_mode;

	/* get irq surport type for channel handle */
	pchan->irq_spt = pcfg->irq_spt;

	/* bkup config/param */
	pchan->des_info_save.cofig = uConfig;
	pchan->des_info_save.param = pcfg->para;

	/*
	 * when called in irq, just use spin_lock, not spin_lock_irqsave
	 */
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_LOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_LOCK_IN_IRQHD(&pchan->lock);
	}

	/* cannot enqueue more than one buffer in single_continue mode */
	if(true == pcfg->bconti_mode
		&& !list_empty(&pchan->buf_list_head)) {
		uret = __LINE__;
		goto End;
	}

	/* irq enable */
	csp_dma_chan_irq_enable(pchan, pcfg->irq_spt);

	/* des enqueue */
	if(0 != __dma_enqueue(dma_hdl, &des, phase)) {
		uret = __LINE__;
		goto End;
	}

End:
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_UNLOCK_IN_IRQHD(&pchan->lock);
	}

	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}

/**
 * sw_dma_enqueue - enqueue the buffer, for single mode only
 * @dma_hdl:	dma handle
 * @src_addr:	buffer src phys addr
 * @dst_addr:	buffer dst phys addr
 * @byte_cnt:	buffer byte cnt
 * @phase:	enqueue phase
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_enqueue_single(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt,
				enum dma_enque_phase_e phase)
{
	u32 			uret = 0;
	unsigned long		flags = 0;
	struct cofig_des_t	des;
	struct dma_channel_t 	*pchan = (struct dma_channel_t *)dma_hdl;

	memset(&des, 0, sizeof(des));
	des.saddr 	= src_addr;
	des.daddr 	= dst_addr;
	des.bcnt 	= byte_cnt;
	des.cofig 	= pchan->des_info_save.cofig;
	des.param 	= pchan->des_info_save.param;
	des.pnext	= (struct cofig_des_t *)DMA_END_DES_LINK;

	/*
	 * when called in irq, just use spin_lock, not spin_lock_irqsave
	 */
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_LOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_LOCK_IN_IRQHD(&pchan->lock);
	}

	/* cannot enqueue more than one buffer in single_continue mode */
	if(true == pchan->bconti_mode
		&& !list_empty(&pchan->buf_list_head)) {
		uret = __LINE__;
		goto End;
	}

	if(0 != __dma_enqueue(dma_hdl, &des, phase)) {
		uret = __LINE__;
		goto End;
	}

End:
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_UNLOCK_IN_IRQHD(&pchan->lock);
	}

	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}

/**
 * dma_irq_hdl_sgmd - dma irq handle, for single mode only
 * @pchan:	dma channel handle
 * @upend_bits:	irq pending for the channel
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_irq_hdl_sgmd(struct dma_channel_t *pchan, u32 upend_bits)
{
	u32	uirq_spt = 0;
	u32	uret = 0;

	DMA_ASSERT(0 != upend_bits);

	uirq_spt = pchan->irq_spt;

	/* deal half done */
	if(upend_bits & CHAN_IRQ_HD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD);
		if(uirq_spt & CHAN_IRQ_HD) {
			if(0 != __handle_hd_sgmd(pchan)) {
				uret = __LINE__;
				goto End;
			}
		}
	}

	/* deal full done */
	if(upend_bits & CHAN_IRQ_FD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_FD);
		if(uirq_spt & CHAN_IRQ_FD) {
			if(0 != __handle_fd_sgmd(pchan)) {
				uret = __LINE__;
				goto End;
			}
		}
	}

	/* deal queue done */
	if(upend_bits & CHAN_IRQ_QD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_QD);
		if(uirq_spt & CHAN_IRQ_QD) {
			if(0 != __handle_qd_sgmd(pchan)) {
				uret = __LINE__;
				goto End;
			}
		}
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}

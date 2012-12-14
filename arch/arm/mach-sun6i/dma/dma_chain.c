/*
 * arch/arm/mach-sun6i/dma/dma_chain_new.c
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

void __free_dest_list(struct list_head *plist)
{
 	des_item *pcur = NULL, *n = NULL;

	/* free all buf on the list */
	list_for_each_entry_safe(pcur, n, plist, list) {
		//printk(KERN_DEBUG "%s: free buf 0x%08x(phys: 0x%08x)\n", __func__, (u32)pcur, (u32)pcur->paddr);
		dma_pool_free(g_des_pool, pcur, pcur->paddr);
	}
	/* init list */
	INIT_LIST_HEAD(plist);
}

/* before start: cur_list is not empty, next_list is empty */
void __dma_start_chain(dm_hdl_t dma_hdl)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;
	des_item *pitem = NULL;

	BUG_ON(list_empty(&pchan->cur_list) || !list_empty(&pchan->next_list));

	/* get the first item from cur_list */
	pitem = list_first_entry(&pchan->cur_list, des_item, list);

	/* write the item's paddr to start reg; start dma */
	csp_dma_chan_set_startaddr(pchan, pitem->paddr);
	csp_dma_chan_start(pchan);
	STATE_CHAIN(pchan) = SINGLE_STA_RUNING;
}

void __dma_stop_chain(dm_hdl_t dma_hdl)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	//dma_dump_chain(dma_hdl);

	/* stop dma channle and clear irq pending */
	csp_dma_chan_stop(pchan);
	csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD);

	/* free all buffer */
	__free_dest_list(&pchan->cur_list);
	__free_dest_list(&pchan->next_list);

	/* change channel state to idle */
	STATE_CHAIN(pchan) = DMA_CHAN_STA_IDLE;
}

void __dma_pause_chain(dm_hdl_t dma_hdl)
{
	csp_dma_chan_pause((struct dma_channel_t *)dma_hdl);
}

void __dma_resume_chain(dm_hdl_t dma_hdl)
{
	csp_dma_chan_resume((struct dma_channel_t *)dma_hdl);
}

u32 __dma_set_op_cb_chain(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	WARN_ON(DMA_CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->op_cb.func = pcb->func;
	pchan->op_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_set_hd_cb_chain(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	WARN_ON(DMA_CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->hd_cb.func = pcb->func;
	pchan->hd_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_set_fd_cb_chain(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	WARN_ON(DMA_CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->fd_cb.func = pcb->func;
	pchan->fd_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_set_qd_cb_chain(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	WARN_ON(DMA_CHAN_STA_IDLE != STATE_CHAIN(pchan));
	pchan->qd_cb.func = pcb->func;
	pchan->qd_cb.parg = pcb->parg;
	return 0;
}

u32 __dma_chain_enqueue(struct dma_channel_t *pchain, struct cofig_des_t *pdes)
{
	u32 paddr = 0, uret = 0;
	des_item *pdes_temp = NULL, *pdes_last = NULL;

	/* cannot enqueue more than one buffer in continue mode */
	if(pchain->des_info_save.bconti_mode && (!list_empty(&pchain->cur_list) || !list_empty(&pchain->next_list))) {
		uret = __LINE__;
		goto end;
	}

	/* create new des */
	pdes_temp = (des_item *)dma_pool_alloc(g_des_pool, GFP_ATOMIC, &paddr);
	if(NULL == pdes_temp) {
		uret = __LINE__;
		goto end;
	}
	pdes_temp->des = *pdes;
	pdes_temp->paddr = paddr;
	/* if continue mode, des link to itself */
	if(pchain->des_info_save.bconti_mode)
		pdes_temp->des.pnext = (struct cofig_des_t *)paddr;

	switch(STATE_CHAIN(pchain)) {
	case DMA_CHAN_STA_IDLE:
		/* change last->link to cur */
		if(!list_empty(&pchain->cur_list)) {
			pdes_last = (des_item *)list_entry(pchain->cur_list.prev, des_item, list);
			pdes_last->des.pnext = (struct cofig_des_t *)paddr;
		}
		/* add to cur list end */
		list_add_tail(&pdes_temp->list, &pchain->cur_list);
		break;
	case DMA_CHAN_STA_RUNING:
		/* assert cur list not empty */
		WARN_ON(list_empty(&pchain->cur_list));
		/* change next.last->link to cur */
		if(!list_empty(&pchain->next_list)) {
			pdes_last = list_entry(pchain->next_list.prev, des_item, list);
			pdes_last->des.pnext = (struct cofig_des_t *)paddr;
		}
		/* add to next list end */
		list_add_tail(&pdes_temp->list, &pchain->next_list);
		break;
	case DMA_CHAN_STA_DONE:
		/* assert cur/next list empty */
		WARN_ON(!list_empty(&pchain->cur_list) || !list_empty(&pchain->next_list));
		/* add to cur list */
		list_add_tail(&pdes_temp->list, &pchain->cur_list);
		/* start cur list */
		__dma_start_chain(pchain);
		break;
	default:
		uret = __LINE__;
		break;
	}
end:
	if(0 != uret) {
		DMA_ERR("%s(%d) err, pchain 0x%08x, pdes 0x%08x\n", __func__, uret, (u32)pchain, (u32)pdes);
		if(NULL != pdes_temp)
			dma_pool_free(g_des_pool, pdes_temp, paddr);
	}
	return uret;
}

void dma_dump_chain(struct dma_channel_t *pchan)
{
	des_item *pitem = NULL;

	if(NULL == pchan) {
		DMA_ERR("%s(%d) err, para is NULL\n", __func__, __LINE__);
		return;
	}

	printk("+++++++++++%s+++++++++++\n", __func__);
	printk("  channel id:        %d\n", pchan->id);
	printk("  channel used:      %d\n", pchan->used);
	printk("  channel owner:     %s\n", pchan->owner);
	printk("  bconti_mode:       %d\n", pchan->bconti_mode);
	printk("  channel irq_spt:   0x%08x\n", pchan->irq_spt);
	printk("  channel reg_base:  0x%08x\n", pchan->reg_base);
	printk("          EN REG:             0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_EN));
	printk("          PAUSE REG:          0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_PAUSE));
	printk("          START REG:          0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_START));
	printk("          CONFIG REG:         0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CFG));
	printk("          CUR SRC REG:        0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_SRC));
	printk("          CUR DST REG:        0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_DST));
	printk("          BYTE CNT LEFT REG:  0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_BCNT_LEFT));
	printk("          PARA REG:           0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_PARA));
	printk("  channel hd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->hd_cb.func, (u32)pchan->hd_cb.parg);
	printk("  channel fd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->fd_cb.func, (u32)pchan->fd_cb.parg);
	printk("  channel qd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->qd_cb.func, (u32)pchan->qd_cb.parg);
	printk("  channel op_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->op_cb.func, (u32)pchan->op_cb.parg);
	printk("  channel des_info_save:  (cofig: 0x%08x, param: 0x%08x, bconti_mode %d)\n", pchan->des_info_save.cofig,
		pchan->des_info_save.param, pchan->des_info_save.bconti_mode);
	if(DMA_WORK_MODE_CHAIN == pchan->work_mode) { /* chain mode */
		/* dump cur des chain */
		printk("  channel cur des buf chain:\n");
		list_for_each_entry(pitem, &pchan->cur_list, list) {
			printk("   pitem: 0x%08x, &pitem->list 0x%08x, &pchan->cur_list 0x%08x\n", (u32)pitem, (u32)&pitem->list, (u32)&pchan->cur_list);
			printk("   paddr:        0x%08x\n", (u32)pitem->paddr);
			printk("   cofig/saddr/daddr/bcnt/param/pnext: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/\n",
				pitem->des.cofig, pitem->des.saddr, pitem->des.daddr, pitem->des.bcnt, pitem->des.param, (u32)pitem->des.pnext);
		}
		/* dump next des chain */
		printk("  channel next des buf chain:\n");
		list_for_each_entry(pitem, &pchan->next_list, list) {
			printk("   pitem: 0x%08x, &pitem->list 0x%08x, &pchan->next_list 0x%08x\n", (u32)pitem, (u32)&pitem->list, (u32)&pchan->next_list);
			printk("   paddr:        0x%08x\n", (u32)pitem->paddr);
			printk("   cofig/saddr/daddr/bcnt/param/pnext: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/\n",
				pitem->des.cofig, pitem->des.saddr, pitem->des.daddr, pitem->des.bcnt, pitem->des.param, (u32)pitem->des.pnext);
		}
		printk("  channel state:     0x%08x\n", (u32)STATE_CHAIN(pchan));
	} else { /* single mode */
		list_for_each_entry(pitem, &pchan->buf_list_head, list) {
			printk("   pitem: 0x%08x, &pitem->list 0x%08x, &pchan->cur_list 0x%08x\n", (u32)pitem, (u32)&pitem->list, (u32)&pchan->cur_list);
			printk("   paddr:        0x%08x\n", (u32)pitem->paddr);
			printk("   cofig/saddr/daddr/bcnt/param/pnext: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/\n",
				pitem->des.cofig, pitem->des.saddr, pitem->des.daddr, pitem->des.bcnt, pitem->des.param, (u32)pitem->des.pnext);
		}
		printk("  channel state:     0x%08x\n", (u32)STATE_SGL(pchan));
	}
	printk("-----------%s-----------\n", __func__);
}

u32 dma_enqueue_chain(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt)
{
	u32 uret = 0;
	unsigned long flags = 0;
	struct cofig_des_t des;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	memset(&des, 0, sizeof(des));
	des.saddr 	= src_addr;
	des.daddr 	= dst_addr;
	des.bcnt 	= byte_cnt;
	des.cofig 	= pchan->des_info_save.cofig;
	des.param 	= pchan->des_info_save.param;
	des.pnext	= (struct cofig_des_t *)DMA_END_DES_LINK;

	DMA_CHAN_LOCK(&pchan->lock, flags);
	if(0 != __dma_chain_enqueue(dma_hdl, &des)) {
		uret = __LINE__;
		goto end;
	}

end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uret)
		DMA_ERR("%s err, line %d\n", __func__, uret);
	return uret;
}

u32 dma_config_chain(dm_hdl_t dma_hdl, struct dma_config_t *pcfg)
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
	pchan->des_info_save.bconti_mode = pcfg->bconti_mode;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	/* irq enable */
	csp_dma_chan_irq_enable(pchan, pcfg->irq_spt);
	/* des enqueue */
	if(0 != __dma_chain_enqueue(dma_hdl, &des)) {
		uret = __LINE__;
		goto end;
	}
end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uret)
		DMA_ERR("%s err, line %d\n", __func__, uret);
	return uret;
}

u32 dma_ctrl_chain(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg)
{
	u32		uret = 0;
	unsigned long	flags = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in start/stop/pause/resume case can parg be NULL  */
	if((NULL == parg) && (DMA_OP_START != op) && (DMA_OP_PAUSE != op) && (DMA_OP_RESUME != op) && (DMA_OP_STOP != op)) {
		WARN_ON(1);
		return __LINE__;
	}

	DMA_CHAN_LOCK(&pchan->lock, flags);

	/* let the caller to do some operation before op */
	if((DMA_OP_SET_OP_CB != op) && (NULL != pchan->op_cb.func))
		if(0 != pchan->op_cb.func(dma_hdl, pchan->op_cb.parg, op))
			DMA_ERR("%s err, line %d\n", __func__, __LINE__);

	switch(op) {
	/* dma hw control */
	case DMA_OP_START:
		__dma_start_chain(dma_hdl);
		break;
	case DMA_OP_PAUSE:
		__dma_pause_chain(dma_hdl);
		break;
	case DMA_OP_RESUME:
		__dma_resume_chain(dma_hdl);
		break;
	case DMA_OP_STOP:
		__dma_stop_chain(dma_hdl);
		break;
	case DMA_OP_GET_STATUS:
		*(u32 *)parg = csp_dma_chan_get_status(pchan);
		break;
	case DMA_OP_GET_CUR_SRC_ADDR:
		*(u32 *)parg = csp_dma_chan_get_cur_srcaddr(pchan);
		break;
	case DMA_OP_GET_CUR_DST_ADDR:
		*(u32 *)parg = csp_dma_chan_get_cur_dstaddr(pchan);
		break;
	case DMA_OP_GET_BYTECNT_LEFT:
		*(u32 *)parg = csp_dma_chan_get_left_bytecnt(pchan);
		break;
	/* set callback */
	case DMA_OP_SET_OP_CB:
		uret = __dma_set_op_cb_chain(dma_hdl, (struct dma_op_cb_t *)parg);
		break;
	case DMA_OP_SET_HD_CB:
		uret = __dma_set_hd_cb_chain(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_FD_CB:
		uret = __dma_set_fd_cb_chain(dma_hdl, (struct dma_cb_t *)parg);
		break;
	case DMA_OP_SET_QD_CB:
		uret = __dma_set_qd_cb_chain(dma_hdl, (struct dma_cb_t *)parg);
		break;
	default:
		uret = __LINE__;
		goto end;
	}

end:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uret)
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __func__, uret, (u32)dma_hdl);
	return uret;
}

u32 dma_release_chain(dm_hdl_t dma_hdl)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;
	unsigned long	flags = 0;

	DMA_CHAN_LOCK(&pchan->lock, flags);

	/* if not idle, call stop */
	if(DMA_CHAN_STA_IDLE != STATE_CHAIN(pchan))
		__dma_stop_chain(dma_hdl);

	/* assert all buf freed */
	WARN_ON(!list_empty(&pchan->cur_list) || !list_empty(&pchan->next_list));

	//memset(pchan, 0, sizeof(*pchan)); /* donot do that, because id...shouldnot be cleared */
	pchan->used = 0;
	pchan->irq_spt = CHAN_IRQ_NO;
	pchan->bconti_mode = false;
	memset(pchan->owner, 0, sizeof(pchan->owner));
	memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));
	memset(&pchan->op_cb, 0, sizeof(pchan->op_cb));
	memset(&pchan->hd_cb, 0, sizeof(pchan->hd_cb));
	memset(&pchan->fd_cb, 0, sizeof(pchan->fd_cb));
	memset(&pchan->qd_cb, 0, sizeof(pchan->qd_cb));

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	return 0;
}

u32 dma_request_init_chain(struct dma_channel_t *pchan)
{
	INIT_LIST_HEAD(&pchan->cur_list);
	INIT_LIST_HEAD(&pchan->next_list);
	STATE_CHAIN(pchan) = DMA_CHAN_STA_IDLE;

	/* init for single mode, incase err access by someone */
	INIT_LIST_HEAD(&pchan->buf_list_head);
	return 0;
}

void __handle_qd_chain(struct dma_channel_t *pchan)
{
	unsigned long	flags = 0;

	/* cannot lock fd_cb function, in case sw_dma_enqueue called and locked agin */
	if(NULL != pchan->qd_cb.func)
		pchan->qd_cb.func((dm_hdl_t)pchan, pchan->qd_cb.parg, DMA_CB_OK);

	DMA_CHAN_LOCK(&pchan->lock, flags);

	switch(STATE_CHAIN(pchan)) {
	case DMA_CHAN_STA_IDLE:
		/* stopped in cb before? or normal stopped */
		WARN_ON(!list_empty(&pchan->cur_list) || !list_empty(&pchan->next_list));
		break;
	case DMA_CHAN_STA_RUNING:
		if(likely(false == pchan->des_info_save.bconti_mode)) {
			//dma_dump_chain(pchan);
			/* free cur buf list */
			__free_dest_list(&pchan->cur_list);
			if(!list_empty(&pchan->next_list)) {
				/* change list head */
				list_replace_init(&pchan->next_list, &pchan->cur_list);
				/* start cur list */
				__dma_start_chain(pchan);
			} else
				STATE_CHAIN(pchan) = DMA_CHAN_STA_DONE;
		}
		break;
	case DMA_CHAN_STA_DONE:
		/* never here */
		BUG_ON(1);
		break;
	default:
		BUG_ON(1);
		break;
	}

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
}

void dma_irq_hdl_chain(struct dma_channel_t *pchan, u32 upend_bits)
{
	u32	uirq_spt = 0;

	WARN(0 == upend_bits, "%s err, line %d!\n", __func__, __LINE__);
	uirq_spt = pchan->irq_spt;

	/* deal half done */
	if(upend_bits & CHAN_IRQ_HD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD);
		if((uirq_spt & CHAN_IRQ_HD) && NULL != pchan->hd_cb.func)
			pchan->hd_cb.func((dm_hdl_t)pchan, pchan->hd_cb.parg, DMA_CB_OK);
	}
	/* deal full done */
	if(upend_bits & CHAN_IRQ_FD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_FD);
		if((uirq_spt & CHAN_IRQ_FD) && NULL != pchan->fd_cb.func)
				pchan->fd_cb.func((dm_hdl_t)pchan, pchan->fd_cb.parg, DMA_CB_OK);
	}
	/* deal queue done */
	if(upend_bits & CHAN_IRQ_QD) {
		csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_QD);
		if(uirq_spt & CHAN_IRQ_QD)
			__handle_qd_chain(pchan);
	}
}


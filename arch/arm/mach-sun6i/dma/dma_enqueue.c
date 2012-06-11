/*
 * arch/arm/mach-sun6i/dma/dma_enqueue.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
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

/**
 * __need_restart_dma - check if we must restart dma for the new enqueued buffer,
 * 	or if we can just pause->resume
 * @pchan:	dma handle
 *
 * return true if we must restart dma, false if not
 */
bool __need_restart_dma(struct dma_channel_t *pchan)
{
	u32 	utemp = 0;
	bool 	bret = false;

	if(DMA_END_DES_LINK == csp_dma_chan_get_startaddr(pchan))
		bret = true;
	else
		bret = false;

#ifdef DBG_DMA
	utemp = csp_dma_chan_get_status(pchan);
	DMA_DBG("%s: channel status %s\n", __FUNCTION__, (utemp == 0 ? "idle" : "busy"));
#endif /* DBG_DMA */

	DMA_DBG("%s: return %d\n", __FUNCTION__, (u32)bret);
	return bret;
}

/**
 * __des_chain_is_empty - check if the channel's descriptor is empty
 * @pchan:	dma handle
 *
 * return true if empty, false if not
 */
bool __des_chain_is_empty(struct dma_channel_t *pchan)
{
#ifdef DBG_DMA
	if(NULL == pchan->pdes_mgr
		|| NULL == pchan->pdes_mgr->pdes
		|| 0 == pchan->pdes_mgr->des_pa) {
		DMA_ERR_FUN_LINE;
		return true;
	}
#endif /* DBG_DMA */

	if(0 == pchan->pdes_mgr->des_num) {
#ifdef DBG_DMA
		if(NULL != pchan->pdes_mgr->pnext)
			DMA_ERR_FUN_LINE;
#endif /* DBG_DMA */
		return true;
	} else {
		return false;
	}
}

/**
 * __add_des_to_chain - add new descriptor to descriptor chain, common for continue/no_continue mode
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __add_des_to_chain(struct dma_channel_t *pchan, struct cofig_des_t *pdes)
{
	u32		uret = 0;
	u32		cur_des_paddr = 0;
	u32		des_num = 0;
	dma_addr_t		des_new_paddr = 0; /* dma bus addr for pdes_new */
	struct des_mgr_t 	*pdes_mgr = NULL;
	struct cofig_des_t	*pdes_new = NULL;
	struct des_mgr_t 	*pdes_mgr_new = NULL;

	/*
	 * add a des to the end of the chain.
	 * NOTE: not consider pdes->pnext. so it's common for conti-mod/not_conti-mod.
	 *	the lase des's pnext is 0xFFFFF800(not_continue_mode) or not_0xFFFFF800(continue_mode)
	 *
	 * (1) des chain is empty, queue to pchan->pdes_mgr->pdes[0], pchan->pdes_mgr->des_num++
	 * (2) des chain not empty:
	 * 	1. there are empty space in cur pdes_mgr, just enqueue to end, des_num++
	 *		a. install pdes to end(memcpy)
	 *		b. change prev_des's pnext to pdes(phys addr)
	 *		c. des_num++
	 *	2. there is not empty space in cur pdes_mgr:
	 *		a. alloc new_des & new_des_mgr, init them
	 *		b. new_des_mgr->pdes = new_des
	 *		c. last_des_mgr->pnext = new_des_mgr
	 *		d. last_des_mgr->des[last]->pnext = &new_des[0]
	 *		e. install pdes to head(memcpy): new_des[0] = pdes(phys addr);
	 */

#ifdef DBG_DMA
	if(NULL == pchan->pdes_mgr
		|| NULL == pchan->pdes_mgr->pdes
		|| 0 == pchan->pdes_mgr->des_pa) {
		uret = __LINE__;
		goto End;
	}
#endif /* DBG_DMA */

	/* deal with empty chain */
	if(0 == pchan->pdes_mgr->des_num) {
		//DMA_DBG_FUN_LINE;
		memcpy(&pchan->pdes_mgr->pdes[0], pdes, sizeof(struct cofig_des_t));
		pchan->pdes_mgr->des_num = 1;
		return 0;
	}

	//DMA_DBG_FUN_LINE;

	/* move to the last des mgr */
	pdes_mgr = pchan->pdes_mgr;
	while(NULL != pdes_mgr->pnext)
		pdes_mgr = pdes_mgr->pnext;

	/* check if there is space left */
	des_num = pdes_mgr->des_num;
#ifdef DBG_DMA
	if(0 == des_num || des_num > MAX_DES_ITEM_NUM) {
		uret = __LINE__;
		goto End;
	}
#endif /* DBG_DMA */
	if(des_num < MAX_DES_ITEM_NUM) {
		/* add des to the end */
		memcpy(&pdes_mgr->pdes[des_num], pdes, sizeof(struct cofig_des_t));

		/* change the last des's link to pdes */
		//cur_des_paddr = virt_to_phys(&pdes_mgr->pdes[des_num]); /* err */
		//cur_des_paddr = pdes_mgr->des_pa + des_num * sizeof(struct cofig_des_t *);
		cur_des_paddr = pdes_mgr->des_pa + (des_num << 4) + (des_num << 3); /* sizeof(struct cofig_des_t *) = 24 */
		//DMA_DBG("%s: the last des's link(0x%08x) to 0x%08x\n", __FUNCTION__, \
		//	(u32)pdes_mgr->pdes[des_num - 1].pnext, cur_des_paddr);
		pdes_mgr->pdes[des_num - 1].pnext = (struct cofig_des_t *)cur_des_paddr;

		/* increase the des num */
		pdes_mgr->des_num++;
	} else {
		/* alloc new des and des mgr */
		pdes_new = (struct cofig_des_t *)dma_pool_alloc(g_pdma_pool, GFP_ATOMIC, &des_new_paddr);
		if (NULL == pdes_new) {
			uret = __LINE__;
			goto End;
		}

		DMA_INF("%s: dma_pool_alloc return va 0x%08x, pa 0x%08x, virt_to_phys(va) 0x%08x\n", \
			__FUNCTION__, (u32)pdes_new, des_new_paddr, (u32)virt_to_phys(pdes_new));

		pdes_mgr_new = kmem_cache_alloc(g_pdma_des_mgr, GFP_ATOMIC);
		if (NULL == pdes_mgr_new) {
			uret = __LINE__;
			goto End;
		}
#ifdef DBG_DMA
		/* init des and des mgr */
		memset(pdes_new, 0, DES_AREA_LEN);
		pdes_mgr_new->des_num = 0;
		pdes_mgr_new->pnext = NULL;
#endif /* DBG_DMA */

		/* install des to the pdes_mgr_new's head */
		memcpy(&pdes_new[0], pdes, sizeof(struct cofig_des_t));

		/* last des link to pdes_new[0] */
		//cur_des_paddr = virt_to_phys(&pdes_new[0]);
		cur_des_paddr = des_new_paddr;
		//DMA_DBG("%s: the last des's link(0x%08x) to 0x%08x\n", __FUNCTION__, \
		//	(u32)pdes_mgr->pdes[MAX_DES_ITEM_NUM - 1].pnext, cur_des_paddr);
		pdes_mgr->pdes[MAX_DES_ITEM_NUM - 1].pnext = (struct cofig_des_t *)cur_des_paddr;

		/* pdes_mgr->pnext point to pdes_mgr_new */
		pdes_mgr->pnext = pdes_mgr_new;

		/* pdes_mgr_new's pdes point to pdes_new */
		pdes_mgr_new->pdes = pdes_new;
		pdes_mgr_new->des_pa = des_new_paddr;

		/* increase des_num++ */
		pdes_mgr_new->des_num++;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);

		DMA_DBG_FUN_LINE_TODO;

		/* free des and des mgr alloced */
		if (NULL != pdes_new) {
			dma_pool_free(g_pdma_pool, pdes_new, des_new_paddr);
			pdes_new = NULL;
			des_new_paddr = 0;
		}
		if (NULL != pdes_mgr_new) {
			kmem_cache_free(g_pdma_des_mgr, pdes_mgr_new);
			pdes_mgr_new = NULL;
		}
	}

	return uret;
}

/**
 * dma_chan_init_main_des - init the main des
 * @pchan:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_chan_init_main_des(struct dma_channel_t *pchan)
{
//#ifdef DBG_DMA
#if 0	/* already checked outside */
	if(NULL == pchan
		|| NULL == pchan->pdes_mgr
		|| NULL == pchan->pdes_mgr->pdes
		|| 0 == pchan->pdes_mgr->des_pa) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* clear main des */
	DMA_DBG_FUN_LINE_TOCHECK; /* todo: just clear the first des */
	memset(pchan->pdes_mgr->pdes, 0, DES_AREA_LEN);

	/* init main des_mgr */
	pchan->pdes_mgr->des_num = 0;
	pchan->pdes_mgr->pnext = NULL;
	return 0;
}

/**
 * dma_clean_des - free extra des/des_mgr if there is, and init main des/des_mgr
 * @pchan:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_clean_des(struct dma_channel_t *pchan)
{
	u32	uRet = 0;
	struct des_mgr_t *pcur = NULL;
	struct des_mgr_t *pnext = NULL;

#ifdef DBG_DMA
	/* para check */
	if(NULL == pchan
		|| NULL == pchan->pdes_mgr
		|| NULL == pchan->pdes_mgr->pdes
		|| 0 == pchan->pdes_mgr->des_pa) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	if(0 == pchan->pdes_mgr->des_num) {
		DMA_INF("%s: des is empty, maybe cleared in qd handle!\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if(NULL == pchan->pdes_mgr->pnext) { /* there are only one des mgr in chain */
		DMA_DBG_FUN_LINE;

		/* init main des/des_mgr */
		dma_chan_init_main_des(pchan);
		return 0;
	} else {
		DMA_DBG_FUN_LINE;

		pcur = pchan->pdes_mgr->pnext;
		pnext = pcur->pnext;

		/* free extra des/desmgr */
		do {
			if(NULL == pnext) {
				DMA_INF("%s, line %d\n", __FUNCTION__, __LINE__);

				/* free pcur */
				dma_pool_free(g_pdma_pool, pcur->pdes, pcur->des_pa);
				kmem_cache_free(g_pdma_des_mgr, pcur);
				break;
			} else {
				DMA_INF("%s, line %d\n", __FUNCTION__, __LINE__);

				/* free pcur */
				dma_pool_free(g_pdma_pool, pcur->pdes, pcur->des_pa);
				kmem_cache_free(g_pdma_des_mgr, pcur);

				pcur = pnext;
				pnext = pcur->pnext;
			}
		}while(1);

		/* init main des/des_mgr */
		dma_chan_init_main_des(pchan);
	}

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
	}

	return uRet;
}

/**
 * __dma_enqueue_contimode - enqueue buffer while dma channel works in continue mode
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __dma_enqueue_contimode(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	u32 	upaddr = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * when continue mode: never queue done, start address reg never be 0xfffff800,
	 * so just pause->queue->resume
	 */

	/*
	 * change pdes->pnext to physcal address of &pchan->pdes_mgr->pdes[0]
	 */
#ifdef DBG_DMA
	if(NULL == pdes) {
		uret = __LINE__;
		goto End;
	}
#endif /*DBG_DMA*/
	//upaddr = virt_to_phys(&pchan->pdes_mgr->pdes[0]);
	upaddr = pchan->pdes_mgr->des_pa;
	DMA_DBG("%s: change cur_des's link from 0x%08x to des chain head 0x%08x\n", __FUNCTION__, \
		(u32)pdes->pnext, upaddr);
	pdes->pnext = (struct cofig_des_t *)upaddr;

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_DBG("%s: state idle, maybe before start or after stop, just enqueue to end\n", __FUNCTION__);
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __need_restart_dma(pchan)) { /* start addr reg never be 0xfffff800 */
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		DMA_DBG("%s: pause->queue to end->resume\n", __FUNCTION__);
		/* pause dma */
		if(0 != dma_pause(dma_hdl)) {
			uret = __LINE__;
			goto End;
		}

		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}

		/* resume dma */
		if(0 != dma_resume(dma_hdl)) {
			uret = __LINE__;
			goto End;
		}

		//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		break;

	default: /* never be wait_qd/done state */
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_normal - enqueue buffer in normal phase
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __dma_enqueue_phase_normal(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from app
	 * so state:
	 * 	idle: 	  maybe before start or after stop
	 * 	running:
	 *		(1) need restart(start addr reg is 0xfffff800): clear/free des, enqueue the first new des, state -> wait_qd,
	 *			new des will be start in __dma_chan_handle_qd
	 *		(2) not need start(reg not 0xfffff800): pause -> enqueue -> resume
	 * 	wait_qd:
	 *		assert des not idle, just enqueue to end, not need pause
	 * 	done:
	 *		assert hw idle, assert des idle, queue and start the first des, state -> running
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_DBG("%s: state idle, maybe before start or after stop, just enqueue to end\n", __FUNCTION__);
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
		if(true == __need_restart_dma(pchan)) { /* cannot queue to end, need restart but qd irq not yet handled */
			DMA_DBG("%s: need restart dma, just clean des and queue to head\n", __FUNCTION__);
			/* save last config/param, clean des */
			if(0 != dma_clean_des(pchan)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* change state to wait_qd */
			pchan->state = DMA_CHAN_STA_WAIT_QD;
		} else { /* transferring, so pause, add des to end, and resume */
			DMA_DBG("%s: not need restart dma, so pause->queue to end->resume\n", __FUNCTION__);
			/* pause dma */
			if(0 != dma_pause(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* resume dma */
			if(0 != dma_resume(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		}
		break;

	case DMA_CHAN_STA_WAIT_QD:
		DMA_DBG("%s: state wait_qd, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __need_restart_dma(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_DBG("%s: state done, make sure des chain is empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}

		/* start dma */
		if(NULL != pchan->op_cb.func) {
			if(0 != pchan->op_cb.func((dm_hdl_t)pchan, pchan->op_cb.parg, DMA_OP_START)) {
				uret = __LINE__;
				goto End;
			}
		}
		if(0 != dma_start(dma_hdl)) { /* state changed to running */
			uret = __LINE__;
			goto End;
		}
		break;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_hd - enqueue buffer in half done callback
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __dma_enqueue_phase_hd(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from hd_cb
	 * so state:
	 * 	idle: 	  maybe stopped somewhere, err
	 * 	running:
	 *		(1) need restart(start addr reg is 0xfffff800): clear/free des, enqueue the first new des, state -> wait_qd,
	 *		(2) not need start(reg not 0xfffff800): pause -> enqueue -> resume
	 * 	wait_qd:
	 *		assert des not idle, just enqueue to end, not need pause/start
	 * 	done:
	 *		print err
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_ERR("%s: state idle, maybe stopped somewhere, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		if(true == __need_restart_dma(pchan)) { /* cannot queue to end, need restart but qd irq not yet handled */
			DMA_DBG("%s: need restart dma, just clean des and queue to head\n", __FUNCTION__);
			/* save last config/param, clean des */
			if(0 != dma_clean_des(pchan)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* change state to wait_qd */
			pchan->state = DMA_CHAN_STA_WAIT_QD;
		} else { /* transferring, so pause, add des to end, and resume */
			DMA_DBG("%s: not need restart dma, so pause->queue to end->resume\n", __FUNCTION__);
			/* pause dma */
			if(0 != dma_pause(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* resume dma */
			if(0 != dma_resume(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		}
		break;

	case DMA_CHAN_STA_WAIT_QD:
		DMA_DBG("%s: state wait_qd, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __need_restart_dma(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_ERR("%s: state done, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_fd - enqueue buffer in full done callback
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __dma_enqueue_phase_fd(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from fd_cb
	 * so state:
	 * 	idle: 	  maybe stopped somewhere, err
	 * 	running:
	 *		(1) need restart(start addr reg is 0xfffff800): clear/free des, enqueue the first new des, state -> wait_qd,
	 *		(2) not need start(reg not 0xfffff800): pause -> enqueue -> resume
	 * 	wait_qd:
	 *		assert des not idle, just enqueue to end, not need pause
	 * 	done:
	 *		print err
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_ERR("%s: state idle, maybe stopped somewhere, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		if(true == __need_restart_dma(pchan)) { /* cannot queue to end, need restart but qd irq not yet handled */
			DMA_DBG("%s: need restart dma, just clean des and queue to head\n", __FUNCTION__);
			/* save last config/param, clean des */
			if(0 != dma_clean_des(pchan)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* change state to wait_qd */
			pchan->state = DMA_CHAN_STA_WAIT_QD;
		} else { /* transferring, so pause, add des to end, and resume */
			DMA_DBG("%s: not need restart dma, so pause->queue to end->resume\n", __FUNCTION__);
			/* pause dma */
			if(0 != dma_pause(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* resume dma */
			if(0 != dma_resume(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		}
		break;

	case DMA_CHAN_STA_WAIT_QD:
		DMA_DBG("%s: state wait_qd, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __need_restart_dma(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_ERR("%s: state done, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_qd - enqueue buffer in queue done callback
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __dma_enqueue_phase_qd(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from qd_cb
	 * so state:
	 * 	idle: 	  maybe stopped somewhere, err
	 * 	running(normal case, not changed to wait_qd before):
	 *		assert des not idle: clear/free des, enqueue the first new des, state -> wait_qd,
	 * 	wait_qd(maybe from above(running) or from hd_cb/fd_cb/normal queueing):
	 *		assert des not idle, just enqueue to end, not need start, will start behind
	 * 	done:
	 *		print err
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_ERR("%s: state idle, maybe stopped somewhere, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true != __need_restart_dma(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* save last config/param, clean des */
		if(0 != dma_clean_des(pchan)) {
			uret = __LINE__;
			goto End;
		}

		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}

		/* change state to wait_qd */
		pchan->state = DMA_CHAN_STA_WAIT_QD;
		break;

	case DMA_CHAN_STA_WAIT_QD:
		DMA_DBG("%s: state wait_qd, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __need_restart_dma(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_ERR("%s: state done, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * dma_enqueue - dma enqueue function, i.e. add buffer to des chain
 * @pchan:	dma handle
 * @pdes:	descriptor to add
 * @phase:	phase when enqueueing
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_enqueue(dm_hdl_t dma_hdl, struct cofig_des_t *pdes, enum dma_enque_phase_e phase)
{
	/*
	 * when continue mode: never queue done, start address reg never be 0xfffff800,
	 * so just pause->queue->resume
	 */
	if(true == ((struct dma_channel_t *)dma_hdl)->bconti_mode) {
		DMA_DBG_FUN_LINE;
		return __dma_enqueue_contimode(dma_hdl, pdes);
	}

	switch(phase) {
	case ENQUE_PHASE_NORMAL:
		return __dma_enqueue_phase_normal(dma_hdl, pdes);
	case ENQUE_PHASE_HD:
		return __dma_enqueue_phase_hd(dma_hdl, pdes);
	case ENQUE_PHASE_FD:
		return __dma_enqueue_phase_fd(dma_hdl, pdes);
	case ENQUE_PHASE_QD:
		return __dma_enqueue_phase_qd(dma_hdl, pdes);
	default:
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
}

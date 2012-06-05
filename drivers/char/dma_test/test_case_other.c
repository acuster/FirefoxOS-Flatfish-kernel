/*
 * drivers/char/dma_test/test_case_other.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "aw163x_dma_test.h"
#include <linux/random.h>

/*
 * src/dst addr for loop dma transfer
 */
//#define DTC_1T_TOTAL_LEN	SIZE_1M /* may lead to dma_pool_alloc failed in sw_dma_request */
//#define DTC_1T_ONE_LEN		SIZE_128K
#define DTC_1T_TOTAL_LEN	SIZE_512K
#define DTC_1T_ONE_LEN		SIZE_64K
static u32 g_src_addr = 0, g_dst_addr = 0;
static atomic_t g_acur_cnt = ATOMIC_INIT(0);

extern wait_queue_head_t g_dtc_queue[];

//#define pr_info

/**
 * __CB_qd_stopcmd - XXX
 *
 * XXX
 */
u32 __CB_qd_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	//pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) {
			//DBG_FUN_LINE;
			uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_QD))
				ERR_FUN_LINE;
		/*
		 * we have complete enqueueing, but not means it's the last qd irq,
		 * in test, we found sometimes never meet if(ucur_cnt == uloop_cnt...
		 * that is, enqueue complete during hd/fd callback.
		 */
		} else if(ucur_cnt == uloop_cnt){
			//DBG_FUN_LINE;

			//sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
		} else {
			//DBG_FUN_LINE;
			//sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
		}
		break;

	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;

	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_fd_stopcmd - dma full done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;

	//pr_info("%s: called!\n", __FUNCTION__);

/*	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		break;

	case DMA_CB_ABORT:
		//pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;

	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	*/
	return uRet;
}

/**
 * __CB_hd_stopcmd - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_stopcmd(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;

/*	//pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		break;

	case DMA_CB_ABORT:
		//pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;

	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	*/
	return uRet;
}

/**
 * __CB_op_stopcmd - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_stopcmd(dm_hdl_t dma_hdl, enum dma_op_type_e op)
{
/*	u32 		result = 0;
	static u32 	enq_cnt = 30;

	//pr_info("%s: called!\n", __FUNCTION__);

	switch(op) {
	case DMA_OP_START:
		//pr_info("%s: op DMA_OP_START!\n", __FUNCTION__);
		break;
	case DMA_OP_STOP:
		//pr_info("%s: op DMA_OP_STOP!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_HD_CB:
		//pr_info("%s: op DMA_OP_SET_HD_CB!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_FD_CB:
		//pr_info("%s: op DMA_OP_SET_FD_CB!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_OP_CB:
		//pr_info("%s: op DMA_OP_SET_OP_CB!\n", __FUNCTION__);
		break;
	default:
		ERR_FUN_LINE;
		return __LINE__;
	}*/

	return 0;
}

/**
 * __Waitdone_stopcmd - wait dma done for case stop_cmd
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_stopcmd(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50 */

	//DBG_FUN_LINE;
        ret = wait_event_interruptible_timeout(g_dtc_queue[0], \
		atomic_read(&g_adma_done)== 1, timeout);
	atomic_set(&g_adma_done, 0);

	if(-ERESTARTSYS == ret) {
		pr_info("%s success!\n", __FUNCTION__);
		return 0;
	} else if(0 == ret) {
		pr_info("%s err, time out!\n", __FUNCTION__);
		return __LINE__;
	} else {
		pr_info("%s success with condition match, ret %d!\n", __FUNCTION__, (int)ret);
		return 0;
	}
}

/**
 * __dtc_stopcmd - app and callback enqueue simutanously, also test last done case
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_stopcmd(void)
{
	u32 	uRet = 0;
	u32 	i = 0;
	void 	*pSrcV = NULL, *pDstV = NULL;
	u32 	uSrcP = 0, uDstP = 0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	struct dma_config_t DmaConfig;
	u32 	src_addr = 0, dst_addr = 0, byte_cnt = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/*
	 * prepare the buffer and data
	 */
	pSrcV = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&uSrcP, GFP_KERNEL);
	if(NULL == pSrcV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pSrcV 0x%08x, uSrcP 0x%08x\n", __FUNCTION__, (u32)pSrcV, uSrcP);
	pDstV = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&uDstP, GFP_KERNEL);
	if(NULL == pDstV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pDstV 0x%08x, uDstP 0x%08x\n", __FUNCTION__, (u32)pDstV, uDstP);

	/*
	 * dump the init src buffer
	 */
	get_random_bytes(pSrcV, DTC_1T_TOTAL_LEN);
	memset(pDstV, 0x54, DTC_1T_TOTAL_LEN);

	/*
	 * init for loop transfer
	 */
	atomic_set(&g_acur_cnt, 0);
	g_src_addr = uSrcP;
	g_dst_addr = uDstP;

	/*
	 * start data transfer
	 */
	dma_hdl = sw_dma_request("case_stp_dma");
	if(NULL == dma_hdl) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __FUNCTION__, (u32)dma_hdl);

	/*
	 * set callback
	 */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = __CB_qd_stopcmd;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_stopcmd;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_stopcmd;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_stopcmd;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set op_cb success\n", __FUNCTION__);

	/*
	 * config
	 */
	memset(&DmaConfig, 0, sizeof(DmaConfig));
	DmaConfig.src_drq_type = DRQSRC_SDRAM;
	DmaConfig.dst_drq_type = DRQDST_SDRAM;

	DmaConfig.bconti_mode = false; /* must be 0, otherwise irq will come again and again */
	DmaConfig.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	DmaConfig.address_type = DMAADDRT_D_LN_S_LN; /* change with dma type */
	DmaConfig.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	DmaConfig.src_addr = uSrcP;
	DmaConfig.dst_addr = uDstP;
	DmaConfig.byte_cnt = DTC_1T_ONE_LEN;
	DmaConfig.para = 0; /* to check here */
	if(0 != sw_dma_config(dma_hdl, &DmaConfig, ENQUE_PHASE_NORMAL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_config success\n", __FUNCTION__);

	sw_dma_dump_chan(dma_hdl);

	atomic_set(&g_adma_done, 0); /* must here, or __Waitdone_stopcmd time out */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_start success\n", __FUNCTION__);

	/*
	 * last done. app and fd_cb enqueue simutanously
	 */
#if 1
	i = 0;
	while(i++ < 100) {
		u32 	uCurSrc = 0, uCurDst = 0;
		u32	uindex = 0;

		uindex  = get_random_int() % (DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN); /* the data section to be transfered */
		uCurSrc = g_src_addr + uindex * DTC_1T_ONE_LEN;
		uCurDst = g_dst_addr + uindex * DTC_1T_ONE_LEN;
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}

		//pr_info("%s: i %d, uindex %d\n", __FUNCTION__, i, uindex);
		//msleep(1);
	}
#endif
	//msleep(5);

#if 0	/* for meet stop which des not null  */
	/*
	 * wait dma done
	 */
	if(0 != __Waitdone_stopcmd()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_stopcmd sucess\n", __FUNCTION__);
#endif

	/*
	 * stop and free dma channel
	 */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_stop success\n", __FUNCTION__);

	if(0 != sw_dma_release(dma_hdl)) {
		uRet = __LINE__;
		goto End;
	}
	dma_hdl = (dm_hdl_t)NULL;
	pr_info("%s: sw_dma_release success\n", __FUNCTION__);

	/*
	 * check if data ok
	 */
	if(0 == memcmp(pSrcV, pDstV, DTC_1T_TOTAL_LEN)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
#if 0 /* we donnot need data ok, just test stop cmd */
		uRet = __LINE__; /* return err */
		goto End;
#endif
	}

End:
	/*
	 * print err line
	 */
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	else
		pr_info("%s success!\n", __FUNCTION__);

	/*
	 * stop and free dma channel, if need
	 */
	if((dm_hdl_t)NULL != dma_hdl) {
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
			pr_err("%s err, line %d!\n", __FUNCTION__, __LINE__);
		}
		if(0 != sw_dma_release(dma_hdl)) {
			pr_err("%s err, line %d!\n", __FUNCTION__, __LINE__);
		}
	}

	/*
	 * free dma memory
	 */
	if(NULL != pSrcV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pSrcV, uSrcP);

	if(NULL != pDstV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pDstV, uDstP);

	return uRet;
}

/**
 * __CB_qd_many_enq - XXX
 *
 * XXX
 */
u32 __CB_qd_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) {
			pr_info("%s, line %d, ucur_cnt %d\n", __FUNCTION__, __LINE__, ucur_cnt);
			//uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN; /* BUG: data check maybe err */
			uCurSrc = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_QD))
				ERR_FUN_LINE;
		} else if(ucur_cnt == uloop_cnt){
		/*
		 * we have complete enqueueing, but not means it's the last qd irq,
		 * in test, we found sometimes never meet if(ucur_cnt == uloop_cnt...
		 * that is, enqueue complete during hd/fd callback.
		 */
			DBG_FUN_LINE;

#if 0		/* NOTE: cannot sigal g_adma_done here, because maybe it's NOT the last qd irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
#endif
		} else {
		/*
		 * NOTE: cannot sigal g_adma_done here, because maybe:
		 * (1) it's the last irq, in this case, we can sigal g_adma_done
		 * (2) it's the last but one irq. maybe ucur_cnt already > uloop_cnt before(eg: in hd/fd cb),
		 * 	so, at this time, it's the last but one irq, after this, __dma_chan_handle_qd will start
		 *	the rest buffer.
		 * in test, we find here(DBG_FUN_LINE below) will meet 2 times.
		 */
			DBG_FUN_LINE;

#if 0
			sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
#endif
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_fd_many_enq - dma full done callback for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			pr_info("%s, line %d, ucur_cnt %d\n", __FUNCTION__, __LINE__, ucur_cnt);
			uCurSrc = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_FD))
				ERR_FUN_LINE;
		} else {
			/* do nothing */
			DBG_FUN_LINE;
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_hd_many_enq - dma half done callback for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			pr_info("%s, line %d, ucur_cnt %d\n", __FUNCTION__, __LINE__, ucur_cnt);
			uCurSrc = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_HD))
				ERR_FUN_LINE;
		} else {
			/* do nothing */
			DBG_FUN_LINE;
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_op_many_enq - dma op callback for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_many_enq(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __FUNCTION__);

	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __FUNCTION__);
		atomic_set(&g_adma_done, 0);
		break;
	case DMA_OP_STOP:
		pr_info("%s: op DMA_OP_STOP!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_HD_CB:
		pr_info("%s: op DMA_OP_SET_HD_CB!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_FD_CB:
		pr_info("%s: op DMA_OP_SET_FD_CB!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_OP_CB:
		pr_info("%s: op DMA_OP_SET_OP_CB!\n", __FUNCTION__);
		break;
	default:
		DBG_FUN_LINE;
		break;
	}

	return 0;
}

/**
 * __Waitdone_many_enq - wait dma done for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_many_enq(void)
{
	long 	ret = 0;
	long 	timeout = 3 * HZ; /* 3s */

        ret = wait_event_interruptible_timeout(g_dtc_queue[0], \
		atomic_read(&g_adma_done)== 1, timeout);

	atomic_set(&g_adma_done, 0);

	if(-ERESTARTSYS == ret) {
		pr_info("%s success!\n", __FUNCTION__);
		return 0;
	} else if(0 == ret) {
		pr_info("%s err, time out!\n", __FUNCTION__);
		return __LINE__;
	} else {
		pr_info("%s success with condition match, ret %d!\n", __FUNCTION__, (int)ret);
		return 0;
	}
}

/**
 * __dtc_many_enq - XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_many_enq(void)
{
	u32 	uRet = 0;
	u32 	i = 0;
	void 	*pSrcV = NULL, *pDstV = NULL;
	u32 	uSrcP = 0, uDstP = 0;
	u32 	usrcp_temp = 0, udstp_temp = 0;
	u32 	src_addr = 0, dst_addr = 0, byte_cnt = 0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	struct dma_config_t DmaConfig;

	pr_info("%s enter\n", __FUNCTION__);

	/*
	 * prepare the buffer and data
	 */
	pSrcV = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&uSrcP, GFP_KERNEL);
	if(NULL == pSrcV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pSrcV 0x%08x, uSrcP 0x%08x\n", __FUNCTION__, (u32)pSrcV, uSrcP);
	pDstV = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&uDstP, GFP_KERNEL);
	if(NULL == pDstV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pDstV 0x%08x, uDstP 0x%08x\n", __FUNCTION__, (u32)pDstV, uDstP);

	/*
	 * dump the init src buffer
	 */
	get_random_bytes(pSrcV, DTC_1T_TOTAL_LEN);
	memset(pDstV, 0x54, DTC_1T_TOTAL_LEN);

	/*
	 * init for loop transfer
	 */
	atomic_set(&g_acur_cnt, 0);
	g_src_addr = uSrcP;
	g_dst_addr = uDstP;

	/*
	 * start data transfer
	 */
	dma_hdl = sw_dma_request("m2m_dma");
	if(NULL == dma_hdl) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __FUNCTION__, (u32)dma_hdl);

	/*
	 * set callback
	 */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = __CB_qd_many_enq;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_many_enq;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_many_enq;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_many_enq;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set op_cb success\n", __FUNCTION__);

	memset(&DmaConfig, 0, sizeof(DmaConfig));

	DmaConfig.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	DmaConfig.address_type = DMAADDRT_D_LN_S_LN;
	DmaConfig.para = 0;
	DmaConfig.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;

	DmaConfig.src_addr = uSrcP;
	DmaConfig.dst_addr = uDstP;
	DmaConfig.byte_cnt = DTC_1T_ONE_LEN;

	DmaConfig.bconti_mode = false;
	DmaConfig.src_drq_type = DRQSRC_SDRAM;
	DmaConfig.dst_drq_type = DRQDST_SDRAM;

	/*
	 * config
	 */
	if(0 != sw_dma_config(dma_hdl, &DmaConfig, ENQUE_PHASE_NORMAL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_config success\n", __FUNCTION__);

	/*
	 * dump chain
	 */
	sw_dma_dump_chan(dma_hdl);

	/*
	 * start
	 */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	//pr_info("%s: sw_dma_start success\n", __FUNCTION__);

	/*
	 * normal enqueue and callback enqueue simutanously
	 */
	{
		u32 	ucur_cnt = 0, uCurSrc = 0, uCurDst = 0;
		u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;

		while(1) {
			//DBG_FUN_LINE;
			ucur_cnt = atomic_add_return(1, &g_acur_cnt);
			if(ucur_cnt < uloop_cnt){
				pr_info("%s, line %d, ucur_cnt %d\n", __FUNCTION__, __LINE__, ucur_cnt);
				uCurSrc = g_src_addr + ucur_cnt * DTC_1T_ONE_LEN;
				uCurDst = g_dst_addr + ucur_cnt * DTC_1T_ONE_LEN;
				if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL))
					ERR_FUN_LINE;

				/* in order to meet cb/normal enqueue simutanously */
				msleep(0);
			} else {
				DBG_FUN_LINE;
				break;
			}
		}
	}
	DBG_FUN_LINE;

#if 1
	/*
	 * we just delay 3s, and check data ok. because we don't know witch is the last
	 *  qd irq to sigal g_adma_done
	 */
	 __Waitdone_many_enq();
#else
	/*
	 * wait dma done
	 */
	if(0 != __Waitdone_many_enq()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_many_enq sucess\n", __FUNCTION__);
#endif

	/*
	 * check if data ok
	 */
	if(0 == memcmp(pSrcV, pDstV, DTC_1T_TOTAL_LEN)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
		uRet = __LINE__; /* return err */
		goto End;
	}

	/*
	 * stop and free dma channel
	 */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_stop success\n", __FUNCTION__);

	if(0 != sw_dma_release(dma_hdl)) {
		uRet = __LINE__;
		goto End;
	}
	dma_hdl = (dm_hdl_t)NULL;

	pr_info("%s: sw_dma_release success\n", __FUNCTION__);

End:
	/*
	 * print err line
	 */
	if(0 != uRet) {
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	} else {
		pr_info("%s, success!\n", __FUNCTION__);
	}

	/*
	 * stop and free dma channel, if need
	 */
	if((dm_hdl_t)NULL != dma_hdl) {
		pr_err("%s, stop and release dma handle now!\n", __FUNCTION__);
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
			pr_err("%s err, line %d!\n", __FUNCTION__, __LINE__);
		}
		if(0 != sw_dma_release(dma_hdl)) {
			pr_err("%s err, line %d!\n", __FUNCTION__, __LINE__);
		}
	}

	/*
	 * free dma memory
	 */
	if(NULL != pSrcV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pSrcV, uSrcP);

	if(NULL != pDstV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pDstV, uDstP);

	pr_info("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

/**
 * __CB_qd_conti_mode - XXX
 *
 * XXX
 */
u32 __CB_qd_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
#if 0		/* never queue done in continue mode */
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) {
			DBG_FUN_LINE;
			uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_QD))
				ERR_FUN_LINE;
		/*
		 * we have complete enqueueing, but not means it's the last qd irq,
		 * in test, we found sometimes never meet if(ucur_cnt == uloop_cnt...
		 * that is, enqueue complete during hd/fd callback.
		 */
		} else if(ucur_cnt == uloop_cnt){
			DBG_FUN_LINE;

		} else {
			DBG_FUN_LINE;
			sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
		}
#endif
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_fd_conti_mode - dma full done callback for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			DBG_FUN_LINE;
			uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_FD))
				ERR_FUN_LINE;
		} else {
			/* do nothing */
			DBG_FUN_LINE;
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_hd_conti_mode - dma half done callback for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		/* enqueue if not done */
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt){
			DBG_FUN_LINE;
			uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			uCurDst = g_dst_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_HD))
				ERR_FUN_LINE;
		} else {
			/* do nothing */
			DBG_FUN_LINE;
		}
		break;
	case DMA_CB_ABORT:
		pr_info("%s: DMA_CB_ABORT!\n", __FUNCTION__);
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);

	return uRet;
}

/**
 * __CB_op_conti_mode - dma op callback for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_conti_mode(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __FUNCTION__);

	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __FUNCTION__);
		atomic_set(&g_adma_done, 0);
		break;
	case DMA_OP_STOP:
		pr_info("%s: op DMA_OP_STOP!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_HD_CB:
		pr_info("%s: op DMA_OP_SET_HD_CB!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_FD_CB:
		pr_info("%s: op DMA_OP_SET_FD_CB!\n", __FUNCTION__);
		break;
	case DMA_OP_SET_OP_CB:
		pr_info("%s: op DMA_OP_SET_OP_CB!\n", __FUNCTION__);
		break;
	default:
		DBG_FUN_LINE;
		break;
	}

	return 0;
}

/**
 * __Waitdone_conti_mode - wait dma done for XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_conti_mode(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50s */

        ret = wait_event_interruptible_timeout(g_dtc_queue[0], \
		atomic_read(&g_adma_done)== 1, timeout);

	atomic_set(&g_adma_done, 0);

	if(-ERESTARTSYS == ret) {
		pr_info("%s success!\n", __FUNCTION__);
		return 0;
	} else if(0 == ret) {
		pr_info("%s err, time out!\n", __FUNCTION__);
		return __LINE__;
	} else {
		pr_info("%s success with condition match, ret %d!\n", __FUNCTION__, (int)ret);
		return 0;
	}
}

/**
 * __dtc_conti_mode - XXX
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_conti_mode(void)
{
	u32 	uRet = 0;
	u32 	i = 0;
	void 	*pSrcV = NULL, *pDstV = NULL;
	u32 	uSrcP = 0, uDstP = 0;
	u32 	usrcp_temp = 0, udstp_temp = 0;
	u32 	src_addr = 0, dst_addr = 0, byte_cnt = 0;
	struct dma_cb_t done_cb;
	struct dma_op_cb_t op_cb;

	dm_hdl_t	dma_hdl = (dm_hdl_t)NULL;
	struct dma_config_t DmaConfig;

	pr_info("%s enter\n", __FUNCTION__);

	/*
	 * prepare the buffer and data
	 */
	pSrcV = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&uSrcP, GFP_KERNEL);
	if(NULL == pSrcV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pSrcV 0x%08x, uSrcP 0x%08x\n", __FUNCTION__, (u32)pSrcV, uSrcP);
	pDstV = dma_alloc_coherent(NULL, DTC_1T_TOTAL_LEN, (dma_addr_t *)&uDstP, GFP_KERNEL);
	if(NULL == pDstV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pDstV 0x%08x, uDstP 0x%08x\n", __FUNCTION__, (u32)pDstV, uDstP);

	/*
	 * dump the init src buffer
	 */
	get_random_bytes(pSrcV, DTC_1T_TOTAL_LEN);
	memset(pDstV, 0x54, DTC_1T_TOTAL_LEN);

	/*
	 * init for loop transfer
	 */
	atomic_set(&g_acur_cnt, 0);
	g_src_addr = uSrcP;
	g_dst_addr = uDstP;

	/*
	 * start data transfer
	 */
	dma_hdl = sw_dma_request("m2m_dma");
	if(NULL == dma_hdl) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __FUNCTION__, (u32)dma_hdl);

	/*
	 * set callback
	 */
	memset(&done_cb, 0, sizeof(done_cb));
	memset(&op_cb, 0, sizeof(op_cb));

	done_cb.func = __CB_qd_conti_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_conti_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_conti_mode;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_conti_mode;
	op_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_OP_CB, (void *)&op_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set op_cb success\n", __FUNCTION__);

	memset(&DmaConfig, 0, sizeof(DmaConfig));

	DmaConfig.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	DmaConfig.address_type = DMAADDRT_D_LN_S_LN;
	DmaConfig.para = 0;
	DmaConfig.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;

	DmaConfig.src_addr = uSrcP;
	DmaConfig.dst_addr = uDstP;
	DmaConfig.byte_cnt = DTC_1T_ONE_LEN;

	DmaConfig.bconti_mode = true;
	DmaConfig.src_drq_type = DRQSRC_SDRAM;
	DmaConfig.dst_drq_type = DRQDST_SDRAM;

	/*
	 * config
	 */
	if(0 != sw_dma_config(dma_hdl, &DmaConfig, ENQUE_PHASE_NORMAL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_config success\n", __FUNCTION__);

	/*
	 * dump chain
	 */
	sw_dma_dump_chan(dma_hdl);

	/*
	 * start
	 */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	//pr_info("%s: sw_dma_start success\n", __FUNCTION__);

#if 0	/* may lead to irq(hd/fd) block the main app */
	{
		u32 	ucur_cnt = 0, uCurSrc = 0, uCurDst = 0;
		u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;

		while(1) {
			//DBG_FUN_LINE;
			ucur_cnt = atomic_add_return(1, &g_acur_cnt);
			if(ucur_cnt < uloop_cnt){
				DBG_FUN_LINE;
				uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
				uCurDst = g_dst_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
				if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL))
					ERR_FUN_LINE;

				/* in order to meet cb/normal enqueue simutanously */
				msleep(0);
			} else {
				DBG_FUN_LINE;
				break;
			}
		}
	}
#endif

	/*
	 * wait dma done
	 */
/*	if(0 != __Waitdone_conti_mode()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_conti_mode sucess\n", __FUNCTION__);*/
	msleep(0);

	/*
	 * check if data ok
	 */
	if(0 == memcmp(pSrcV, pDstV, DTC_1T_TOTAL_LEN)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
		uRet = __LINE__; /* return err */
		goto End;
	}

	/*
	 * stop and free dma channel
	 */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_stop success\n", __FUNCTION__);

	if(0 != sw_dma_release(dma_hdl)) {
		uRet = __LINE__;
		goto End;
	}
	dma_hdl = (dm_hdl_t)NULL;

	pr_info("%s: sw_dma_release success\n", __FUNCTION__);

End:
	/*
	 * print err line
	 */
	if(0 != uRet) {
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	} else {
		pr_info("%s, success!\n", __FUNCTION__);
	}

	/*
	 * stop and free dma channel, if need
	 */
	if((dm_hdl_t)NULL != dma_hdl) {
		pr_err("%s, stop and release dma handle now!\n", __FUNCTION__);
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
			pr_err("%s err, line %d!\n", __FUNCTION__, __LINE__);
		}
		if(0 != sw_dma_release(dma_hdl)) {
			pr_err("%s err, line %d!\n", __FUNCTION__, __LINE__);
		}
	}

	/*
	 * free dma memory
	 */
	if(NULL != pSrcV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pSrcV, uSrcP);

	if(NULL != pDstV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pDstV, uDstP);

	pr_info("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

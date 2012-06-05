/*
 * drivers/char/dma_test/test_case_1t_mem_2_mem.c
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
#define DTC_1T_TOTAL_LEN	SIZE_512K
//#define DTC_1T_TOTAL_LEN	SIZE_16K
#define DTC_1T_ONE_LEN		SIZE_4K
static u32 g_src_addr = 0, g_dst_addr = 0;
static atomic_t g_acur_cnt = ATOMIC_INIT(0);

/**
 * __CB_qd_1t_mem_2_mem - XXX
 *
 * XXX
 */
u32 __CB_qd_1t_mem_2_mem(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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

#if 0
			sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[DTC_1T_MEM_2_MEM]);
#endif
		} else {
			DBG_FUN_LINE;
			sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[DTC_1T_MEM_2_MEM]);
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
 * __CB_fd_1t_mem_2_mem - dma full done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_1t_mem_2_mem(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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
 * __CB_hd_1t_mem_2_mem - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_1t_mem_2_mem(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
#if 0
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
 * __CB_op_1t_mem_2_mem - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_1t_mem_2_mem(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
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
 * __Waitdone_1t_mem_2_mem - wait dma done for DTC_1T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_1t_mem_2_mem(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50s */

        ret = wait_event_interruptible_timeout(g_dtc_queue[DTC_1T_MEM_2_MEM], \
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
 * __dtc_1t_mem_2_mem - dma test case one-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_1t_mem_2_mem(void)
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

	done_cb.func = __CB_qd_1t_mem_2_mem;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_1t_mem_2_mem;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_1t_mem_2_mem;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_1t_mem_2_mem;
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

	//DmaConfig.conti_mode = 1;
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
	 * enqueue to full
	 */
	/*usrcp_temp = uSrcP;
	udstp_temp = uDstP;
	for(i = 0; i < DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN - 1; i++) {
		pr_info("%s: sw_dma_enqueue i %d\n", __FUNCTION__, i);
		usrcp_temp += DTC_1T_ONE_LEN;
		udstp_temp += DTC_1T_ONE_LEN;
		if(0 != sw_dma_enqueue(dma_hdl, usrcp_temp, udstp_temp, \
				DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL))
			ERR_FUN_LINE;
	}*/

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
				//DBG_FUN_LINE;
				uCurSrc = g_src_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
				uCurDst = g_dst_addr + atomic_read(&g_acur_cnt) * DTC_1T_ONE_LEN;
				if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL))
					ERR_FUN_LINE;

				/* in order to meet cb/normal enqueue simutanously */
				//msleep(1);
			} else {
				DBG_FUN_LINE;
				break;
			}
		}
	}
	DBG_FUN_LINE;

	/*
	 * wait dma done
	 */
	//msleep(5000);
	if(0 != __Waitdone_1t_mem_2_mem()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_1t_mem_2_mem sucess\n", __FUNCTION__);

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

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	/*
	 * free dma memory
	 */
	if(NULL != pSrcV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pSrcV, uSrcP);

	if(NULL != pDstV)
		dma_free_coherent(NULL, DTC_1T_TOTAL_LEN, pDstV, uDstP);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

/**
 * __CB_qd_case_enq_aftdone - XXX
 *
 * XXX
 */
u32 __CB_qd_case_enq_aftdone(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32 	ucur_cnt = 0;
	u32	uloop_cnt = DTC_1T_TOTAL_LEN / DTC_1T_ONE_LEN;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
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

			sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[DTC_1T_MEM_2_MEM]);
		} else {
			DBG_FUN_LINE;
			sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[DTC_1T_MEM_2_MEM]);
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
 * __CB_fd_case_enq_aftdone - dma full done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_case_enq_aftdone(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
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
 * __CB_hd_case_enq_aftdone - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_case_enq_aftdone(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
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
 * __CB_op_case_enq_aftdone - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_case_enq_aftdone(dm_hdl_t dma_hdl, void *parg, enum dma_op_type_e op)
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
		ERR_FUN_LINE;
		return __LINE__;
	}

	return 0;
}

/**
 * __Waitdone_case_enq_aftdone - wait dma done for DTC_1T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_case_enq_aftdone(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50 */

        ret = wait_event_interruptible_timeout(g_dtc_queue[DTC_1T_MEM_2_MEM], \
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
 * __dtc_case_enq_aftdone - app and callback enqueue simutanously, also test done case
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_case_enq_aftdone(void)
{
	u32 	uRet = 0;
	u32 	i = 0;
	void 	*pSrcV = NULL, *pDstV = NULL;
	u32 	uSrcP = 0, uDstP = 0;
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

	done_cb.func = __CB_qd_case_enq_aftdone;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_case_enq_aftdone;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_case_enq_aftdone;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_case_enq_aftdone;
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
	//DmaConfig.conti_mode = 1;
	DmaConfig.bconti_mode = false; /* must be 0, otherwise irq will come again and again */
	DmaConfig.xfer_type = DMAXFER_D_BWORD_S_BWORD;
	DmaConfig.address_type = DMAADDRT_D_LN_S_LN; /* change with dma type */
	DmaConfig.irq_spt = CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD;
	DmaConfig.src_addr = uSrcP;
	DmaConfig.dst_addr = uDstP;
	DmaConfig.byte_cnt = DTC_1T_ONE_LEN;
	DmaConfig.para = 0;
	if(0 != sw_dma_config(dma_hdl, &DmaConfig, ENQUE_PHASE_NORMAL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_config success\n", __FUNCTION__);

	sw_dma_dump_chan(dma_hdl);

	//atomic_set(&g_adma_done, 0);
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_start success\n", __FUNCTION__);

	/*
	 * wait dma done
	 */
	if(0 != __Waitdone_case_enq_aftdone()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_case_enq_aftdone sucess\n", __FUNCTION__);

	/*
	 * done. app and fd_cb enqueue simutanously
	 */
	i = 0;
	while(i++ < 30) {
		u32 uCurSrc = 0, uCurDst = 0;

		pr_info("%s: i %d\n", __FUNCTION__, i);

		uCurSrc = g_src_addr + 0 * DTC_1T_ONE_LEN;
		uCurDst = g_dst_addr + 0 * DTC_1T_ONE_LEN;
		if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_1T_ONE_LEN, ENQUE_PHASE_NORMAL)) {
			uRet = __LINE__;
			goto End;
		}

		msleep(1);
	}
	msleep(5000);

	/*
	 * check if data ok
	 */
	if(0 == memcmp(pSrcV, pDstV, DTC_1T_TOTAL_LEN)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);

		/*
		 * dump the dst mem
		 */
		pr_info("%s: first bytes in src mem is:\n", __FUNCTION__);
		for(i = 0; i < 100; i++) {
			pr_info("%d, ", *(u8 *)((u8 *)pSrcV + i));
		}
		pr_info("\n");
		pr_info("%s: first bytes in dst mem is:\n", __FUNCTION__);
		for(i = 0; i < 100; i++) {
			pr_info("%d, ", *(u8 *)((u8 *)pDstV + i));
		}
		pr_info("\n");

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


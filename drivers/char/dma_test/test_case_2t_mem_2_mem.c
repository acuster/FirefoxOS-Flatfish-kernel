/*
 * drivers/char/dma_test/test_case_2t_mem_2_mem.c
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

#define THREAD1_DMA_NAME 	"m2m_dma_thread1"
#define THREAD2_DMA_NAME 	"m2m_dma_thread2"

#define DTC_2T_TOTAL_LEN	SIZE_64K
#define DTC_2T_ONE_LEN		SIZE_8K

#define TEST_TIME_THREAD1 	0x0fffffff	/* ms */
#define TEST_TIME_THREAD2 	0x0fffffff

static atomic_t 	g_adma_done1 = ATOMIC_INIT(0);	/* dma done flag */
static atomic_t 	g_adma_done2 = ATOMIC_INIT(0);	/* dma done flag */
static u32 		g_sadr1 = 0, g_dadr1 = 0;
static u32 		g_sadr2 = 0, g_dadr2 = 0;
static atomic_t 	g_acur_cnt1 = ATOMIC_INIT(0);
static atomic_t 	g_acur_cnt2 = ATOMIC_INIT(0);

/**
 * __dump_cur_mem_info - dump current mem info
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dump_cur_mem_info(void)
{
	struct sysinfo s_info;
	int 	error;

	memset(&s_info, 0, sizeof(s_info));
	error = do_sysinfo(&s_info);

	pr_info("%s: cur time 0x%08xs, total mem %dM, free mem %dM, total high %dM, free high %dM\n", \
		__FUNCTION__, (u32)s_info.uptime, 	\
		(u32)(s_info.totalram / 1024 / 1024), 	\
		(u32)(s_info.freeram / 1024 / 1024), 	\
		(u32)(s_info.totalhigh / 1024 / 1024), 	\
		(u32)(s_info.freehigh / 1024 / 1024));

	return 0;
}

/**
 * __CB_qd_2 - XXX
 *
 * XXX
 */
u32 __CB_qd_2(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_2T_TOTAL_LEN / DTC_2T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		ucur_cnt = atomic_add_return(1, &g_acur_cnt2);
		if(ucur_cnt < uloop_cnt) {
			//DBG_FUN_LINE;
			uCurSrc = g_sadr2 + atomic_read(&g_acur_cnt2) * DTC_2T_ONE_LEN;
			uCurDst = g_dadr2 + atomic_read(&g_acur_cnt2) * DTC_2T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_2T_ONE_LEN, ENQUE_PHASE_QD))
				ERR_FUN_LINE;
		} else if(ucur_cnt == uloop_cnt){
			//DBG_FUN_LINE;

			//sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done2, 1);
			wake_up_interruptible(&g_dtc_queue[2]);
		} else {
			//DBG_FUN_LINE;
			//sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done2, 1);
			wake_up_interruptible(&g_dtc_queue[2]);
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
 * __CB_fd_2 - dma full done callback for __dtc_2t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_2(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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
 * __CB_hd_2 - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_2(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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
 * __CB_op_2 - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_2(dm_hdl_t dma_hdl, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __FUNCTION__);

	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __FUNCTION__);
		atomic_set(&g_adma_done2, 0);
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
 * __Waitdone_2 - wait dma done for DTC_2T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_2(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50 */

        ret = wait_event_interruptible_timeout(g_dtc_queue[2], \
		atomic_read(&g_adma_done2)== 1, timeout);

	atomic_set(&g_adma_done2, 0);

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
 * _thread2_proc - thread2 test func
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 _thread2_proc(void)
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
	pSrcV = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&uSrcP, GFP_KERNEL);
	if(NULL == pSrcV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pSrcV 0x%08x, uSrcP 0x%08x\n", __FUNCTION__, (u32)pSrcV, uSrcP);
	pDstV = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&uDstP, GFP_KERNEL);
	if(NULL == pDstV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pDstV 0x%08x, uDstP 0x%08x\n", __FUNCTION__, (u32)pDstV, uDstP);

	/*
	 * dump the init src buffer
	 */
	get_random_bytes(pSrcV, DTC_2T_TOTAL_LEN);
	memset(pDstV, 0x53, DTC_2T_TOTAL_LEN);

	/*
	 * init for loop transfer
	 */
	atomic_set(&g_acur_cnt2, 0);
	g_sadr2 = uSrcP;
	g_dadr2 = uDstP;

	/*
	 * start data transfer
	 */
	dma_hdl = sw_dma_request(THREAD2_DMA_NAME);
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

	done_cb.func = __CB_qd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_2;
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
	DmaConfig.byte_cnt = DTC_2T_ONE_LEN;
	DmaConfig.para = 0;
	if(0 != sw_dma_config(dma_hdl, &DmaConfig, ENQUE_PHASE_NORMAL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_config success\n", __FUNCTION__);

	sw_dma_dump_chan(dma_hdl);

	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_start success\n", __FUNCTION__);

	/*
	 * wait dma done
	 */
	if(0 != __Waitdone_2()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_2 sucess\n", __FUNCTION__);

	/*
	 * check if data ok
	 */
	if(0 == memcmp(pSrcV, pDstV, DTC_2T_TOTAL_LEN)) {
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
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, pSrcV, uSrcP);

	if(NULL != pDstV)
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, pDstV, uDstP);

	return uRet;
}

/**
 * __CB_qd_1 - XXX
 *
 * XXX
 */
u32 __CB_qd_1(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
{
	u32 	uRet = 0;
	u32	uCurSrc = 0, uCurDst = 0;
	u32	uloop_cnt = DTC_2T_TOTAL_LEN / DTC_2T_ONE_LEN;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __FUNCTION__);

	switch(cause) {
	case DMA_CB_OK:
		//pr_info("%s: DMA_CB_OK!\n", __FUNCTION__);
		ucur_cnt = atomic_add_return(1, &g_acur_cnt1);
		if(ucur_cnt < uloop_cnt) {
			//DBG_FUN_LINE;
			uCurSrc = g_sadr1 + atomic_read(&g_acur_cnt1) * DTC_2T_ONE_LEN;
			uCurDst = g_dadr1 + atomic_read(&g_acur_cnt1) * DTC_2T_ONE_LEN;
			if(0 != sw_dma_enqueue(dma_hdl, uCurSrc, uCurDst, DTC_2T_ONE_LEN, ENQUE_PHASE_QD))
				ERR_FUN_LINE;
		} else if(ucur_cnt == uloop_cnt){
			//DBG_FUN_LINE;

			//sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq; or next will be the last irq, need think about */
			atomic_set(&g_adma_done1, 1);
			wake_up_interruptible(&g_dtc_queue[1]);
		} else {
			//DBG_FUN_LINE;
			//sw_dma_dump_chan(dma_hdl); /* for debug */

			/* maybe it's the last irq */
			atomic_set(&g_adma_done1, 1);
			wake_up_interruptible(&g_dtc_queue[1]);
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
 * __CB_fd_1 - dma full done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_fd_1(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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
 * __CB_hd_1 - dma half done callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_hd_1(dm_hdl_t dma_hdl, void *parg, enum dma_cb_cause_e cause)
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
 * __CB_op_1 - dma op callback for __dtc_1t_mem_2_mem
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __CB_op_1(dm_hdl_t dma_hdl, enum dma_op_type_e op)
{
	pr_info("%s: called!\n", __FUNCTION__);

	switch(op) {
	case DMA_OP_START:
		pr_info("%s: op DMA_OP_START!\n", __FUNCTION__);
		atomic_set(&g_adma_done1, 0);
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
 * __Waitdone_1 - wait dma done for DTC_2T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __Waitdone_1(void)
{
	long 	ret = 0;
	long 	timeout = 50 * HZ; /* 50 */

        ret = wait_event_interruptible_timeout(g_dtc_queue[1], \
		atomic_read(&g_adma_done1)== 1, timeout);

	atomic_set(&g_adma_done1, 0);

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
 * _thread1_proc - thread2 test func
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 _thread1_proc(void)
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
	pSrcV = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&uSrcP, GFP_KERNEL);
	if(NULL == pSrcV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pSrcV 0x%08x, uSrcP 0x%08x\n", __FUNCTION__, (u32)pSrcV, uSrcP);
	pDstV = dma_alloc_coherent(NULL, DTC_2T_TOTAL_LEN, (dma_addr_t *)&uDstP, GFP_KERNEL);
	if(NULL == pDstV) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pDstV 0x%08x, uDstP 0x%08x\n", __FUNCTION__, (u32)pDstV, uDstP);

	/*
	 * dump the init src buffer
	 */
	get_random_bytes(pSrcV, DTC_2T_TOTAL_LEN);
	memset(pDstV, 0x56, DTC_2T_TOTAL_LEN);

	/*
	 * init for loop transfer
	 */
	atomic_set(&g_acur_cnt1, 0);
	g_sadr1 = uSrcP;
	g_dadr1 = uDstP;

	/*
	 * start data transfer
	 */
	dma_hdl = sw_dma_request(THREAD1_DMA_NAME);
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

	done_cb.func = __CB_qd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_QD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set queuedone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_fd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set fulldone_cb success\n", __FUNCTION__);

	done_cb.func = __CB_hd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: set halfdone_cb success\n", __FUNCTION__);

	op_cb.func = __CB_op_1;
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
	DmaConfig.byte_cnt = DTC_2T_ONE_LEN;
	DmaConfig.para = 0;
	if(0 != sw_dma_config(dma_hdl, &DmaConfig, ENQUE_PHASE_NORMAL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_config success\n", __FUNCTION__);

	sw_dma_dump_chan(dma_hdl);

	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: sw_dma_start success\n", __FUNCTION__);

	/*
	 * wait dma done
	 */
	if(0 != __Waitdone_1()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: __Waitdone_1 sucess\n", __FUNCTION__);

	/*
	 * check if data ok
	 */
	if(0 == memcmp(pSrcV, pDstV, DTC_2T_TOTAL_LEN)) {
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
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, pSrcV, uSrcP);

	if(NULL != pDstV)
		dma_free_coherent(NULL, DTC_2T_TOTAL_LEN, pDstV, uDstP);

	return uRet;
}
/**
 * __test_thread1 - thread test proc1
 * @arg:	thread arg, no use
 *
 * Returns 0 if success, the err line number if failed.
 */
int __test_thread1(void * arg)
{
	u32 	uRet = 0;
	u32 	begin_ms = 0, end_ms = 0;

	begin_ms = (jiffies * 1000) / HZ;
	pr_info("%s: begin_ms 0x%08x\n", __FUNCTION__, begin_ms);
	while(1) {
		__dump_cur_mem_info();

		if(0 != _thread1_proc()) {
			uRet = __LINE__;
			goto End;
		}

		//msleep(20);

		end_ms = (jiffies * 1000) / HZ;
		pr_info("%s: cur_ms 0x%08x\n", __FUNCTION__, end_ms);
		if(end_ms - begin_ms >= TEST_TIME_THREAD1) {
			pr_info("%s: time passed! ok!\n", __FUNCTION__);
			break;
		}
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	else
		pr_info("%s success!\n", __FUNCTION__);

	return uRet;
}

/**
 * __test_thread2 - thread test proc2
 * @arg:	thread arg, no use
 *
 * Returns 0 if success, the err line number if failed.
 */
int __test_thread2(void * arg)
{
	u32 	uRet = 0;
	u32 	begin_ms = 0, end_ms = 0;

	begin_ms = (jiffies * 1000) / HZ;
	while(1) {
		__dump_cur_mem_info();

		if(0 != _thread2_proc()) {
			uRet = __LINE__;
			goto End;
		}

		//msleep(10);

		end_ms = (jiffies * 1000) / HZ;
		if(end_ms - begin_ms >= TEST_TIME_THREAD2) {
			pr_info("%s: time passed! ok!\n", __FUNCTION__);
			break;
		}
	}

End:
	if(0 != uRet)
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	else
		pr_info("%s success!\n", __FUNCTION__);

	return uRet;
}

/**
 * __dtc_2t_mem_2_mem - dma test case two-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_2t_mem_2_mem(void)
{
	__dump_cur_mem_info();

	kernel_thread(__test_thread1, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(__test_thread2, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

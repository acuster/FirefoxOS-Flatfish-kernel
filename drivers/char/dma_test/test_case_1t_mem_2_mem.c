/*
 * drivers/char/dma_test/test_case_1t_mem_2_mem.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun7i dma test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "sun7i_dma_test.h"
#include <linux/random.h>

/*
 * one buffer size and total transfer size
 */
#define TOTAL_LEN_CASE_1TM2M	SZ_512K
#define ONE_LEN_CASE_1TM2M	TOTAL_LEN_CASE_1TM2M

#define TOTAL_LEN_CASE_ENQAFTDONE	SZ_512K
#define ONE_LEN_CASE_ENQAFTDONE		TOTAL_LEN_CASE_ENQAFTDONE

/*
 * src/dst addr for loop dma transfer
 */
static u32 	g_src_addr = 0, g_dst_addr = 0;
static atomic_t g_acur_cnt = ATOMIC_INIT(0);

/**
 * fd_cb_case_1tm2m - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_case_1tm2m(struct sw_dma_chan * ch, void *buf,
				int size, enum sw_dma_buffresult result)
{
	u32 	usign = 0;
	u32	ucur_src = 0, ucur_dst = 0;
	u32	uloop_cnt = TOTAL_LEN_CASE_1TM2M / ONE_LEN_CASE_1TM2M;
	u32 	ucur_cnt = 0;

	printk("%s: ch %d, buf 0x%08x, size %d, result %d\n", __FUNCTION__, ch->number,
		(u32)buf, size, (u32)result);

	switch(result) {
	case SW_RES_OK:
		pr_info("%s: SW_RES_OK!\n", __FUNCTION__);
		ucur_cnt = atomic_add_return(1, &g_acur_cnt);
		if(ucur_cnt < uloop_cnt) { /* enqueue if not done */
			ucur_src = g_src_addr + atomic_read(&g_acur_cnt) * ONE_LEN_CASE_1TM2M;
			ucur_dst = g_dst_addr + atomic_read(&g_acur_cnt) * ONE_LEN_CASE_1TM2M;
			ERR_FUN_LINE; /* only input src_phys??? how can we input dst_phys??? */
			if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0], (dma_addr_t)ucur_src, ONE_LEN_CASE_1TM2M))
				ERR_FUN_LINE;
		} else { /* we have complete enqueueing, but not means it's the last qd irq?? */
			DBG_FUN_LINE;
			if(ucur_cnt == uloop_cnt)
				DBG_FUN_LINE;

			/* maybe it's the last irq */
			atomic_set(&g_adma_done, 1);
			wake_up_interruptible(&g_dtc_queue[0]);
		}
		break;
	default:
		usign = __LINE__;
		goto End;
	}

End:
	if(0 != usign)
		pr_err("%s err, line %d!\n", __FUNCTION__, usign);
}

/**
 * wd_case_1tm2m - wait dma done for DTC_1T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_case_1tm2m(void)
{
	long 	ret = 0;
	long 	timeout = 5 * HZ; /* 5s */

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
 * __dtc_1t_mem_2_mem - dma test case one-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_1t_mem_2_mem(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_LEN_CASE_1TM2M, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_LEN_CASE_1TM2M, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_LEN_CASE_1TM2M);
	memset(pdst_v, 0x54, TOTAL_LEN_CASE_1TM2M);

	/* init for loop transfer */
	atomic_set(&g_acur_cnt, 0);
	g_src_addr = psrc_p;
	g_dst_addr = pdst_p;

	/* request dma */
	g_dma_hle = sw_dma_request(DMACH_DSDRAM, &g_client, NULL);
	if(g_dma_hle < 0) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_request return g_dma_hle %d\n", __FUNCTION__, g_dma_hle);

	/* set auto start flag */
	if(0 != sw_dma_setflags(g_dma_hle, SW_DMAF_AUTOSTART)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_setflags success\n", __FUNCTION__);

	/* config dma */
	g_confs[0].xfer_type 	= DMAXFER_D_BWORD_S_BWORD;
	g_confs[0].hf_irq 	= SW_DMA_IRQ_FULL;
	g_confs[0].dir		= SW_DMA_M2M;
	g_confs[0].to 		= pdst_p;
	g_confs[0].from 	= psrc_p;
	g_confs[0].address_type = DMAADDRT_D_INC_S_INC;;
	g_confs[0].drqsrc_type 	= DRQ_TYPE_SDRAM;
	g_confs[0].drqdst_type 	= DRQ_TYPE_SDRAM;
	if(0 != sw_dma_config(g_dma_hle, &g_confs[0])) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_config success\n", __FUNCTION__);

	/* set full done callback */
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle, fd_cb_case_1tm2m)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0], (dma_addr_t)psrc_p, ONE_LEN_CASE_1TM2M)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

	/* wait dma done */
	if(0 != wd_case_1tm2m()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_case_1tm2m success\n", __FUNCTION__);

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_LEN_CASE_1TM2M)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
		uRet = __LINE__;
		goto End;
	}

	/* stop and free dma channel */
	if(0 != sw_dma_free(g_dma_hle, &g_client)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_free success\n", __FUNCTION__);

End:
	if(0 != uRet) {
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	} else {
		pr_info("%s, success!\n", __FUNCTION__);
	}

	/* free dma memory */
	if(NULL != psrc_v)
		dma_free_coherent(NULL, TOTAL_LEN_CASE_1TM2M, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_LEN_CASE_1TM2M, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

/**
 * fd_cb_case_enqaftdone - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_case_enqaftdone(struct sw_dma_chan * ch, void *buf,
				int size, enum sw_dma_buffresult result)
{
	u32 	usign = 0;

	printk("%s: ch %d, buf 0x%08x, size %d, result %d\n", __FUNCTION__, ch->number,
		(u32)buf, size, (u32)result);

	switch(result) {
	case SW_RES_OK:
		pr_info("%s: SW_RES_OK!\n", __FUNCTION__);
		atomic_set(&g_adma_done, 1);
		wake_up_interruptible(&g_dtc_queue[0]);
		break;
	default:
		usign = __LINE__;
		goto End;
	}

End:
	if(0 != usign)
		pr_err("%s err, line %d!\n", __FUNCTION__, usign);
}

/**
 * wd_case_enqaftdone - wait dma done for DTC_1T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_case_enqaftdone(void)
{
	long 	ret = 0;
	long 	timeout = 5 * HZ; /* 5s */

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
 * __dtc_case_enq_aftdone - dma test case enqueue after done
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_case_enq_aftdone(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_LEN_CASE_ENQAFTDONE, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_LEN_CASE_ENQAFTDONE, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_LEN_CASE_ENQAFTDONE);
	memset(pdst_v, 0x54, TOTAL_LEN_CASE_ENQAFTDONE);

	/* init for loop transfer */
	atomic_set(&g_acur_cnt, 0);
	g_src_addr = psrc_p;
	g_dst_addr = pdst_p;

	/* request dma */
	g_dma_hle = sw_dma_request(DMACH_DSDRAM, &g_client, NULL);
	if(g_dma_hle < 0) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_request return g_dma_hle %d\n", __FUNCTION__, g_dma_hle);

	/* set auto start flag */
	if(0 != sw_dma_setflags(g_dma_hle, SW_DMAF_AUTOSTART)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_setflags success\n", __FUNCTION__);

	/* config dma */
	g_confs[0].xfer_type 	= DMAXFER_D_BWORD_S_BWORD;
	g_confs[0].hf_irq 	= SW_DMA_IRQ_FULL;
	g_confs[0].dir		= SW_DMA_M2M;
	g_confs[0].to 		= pdst_p;
	g_confs[0].from 	= psrc_p;
	g_confs[0].address_type = DMAADDRT_D_INC_S_INC;;
	g_confs[0].drqsrc_type 	= DRQ_TYPE_SDRAM;
	g_confs[0].drqdst_type 	= DRQ_TYPE_SDRAM;
	if(0 != sw_dma_config(g_dma_hle, &g_confs[0])) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_config success\n", __FUNCTION__);

	/* set full done callback */
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle, fd_cb_case_enqaftdone)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0], (dma_addr_t)psrc_p, ONE_LEN_CASE_ENQAFTDONE)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

	/* wait dma done */
	if(0 != wd_case_enqaftdone()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_case_enqaftdone success\n", __FUNCTION__);

	/* enqueue the rest buffer */
#if 0	/* NOTE: cannot enqueue more than one buf, for sw_dma_enqueue only has one addr para */
	{
		u32	ucur_src = 0, ucur_dst = 0;
		u32	uloop_cnt = TOTAL_LEN_CASE_ENQAFTDONE / ONE_LEN_CASE_ENQAFTDONE;
		u32 	ucur_cnt = 0;

		while(++ucur_cnt < uloop_cnt) {
			ucur_src = g_src_addr + ucur_cnt * ONE_LEN_CASE_ENQAFTDONE;
			ucur_dst = g_dst_addr + ucur_cnt * ONE_LEN_CASE_ENQAFTDONE;
			if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0], (dma_addr_t)ucur_src, ONE_LEN_CASE_ENQAFTDONE))
				ERR_FUN_LINE;
		}
	}
#else
	{
		u32 	ucur_cnt = 0;

		pr_info("%s maybe err: NOTE - cannot enqueue more than one buf, for sw_dma_enqueue only has one\
			addr para in sun7i.\n", __FUNCTION__);
		while(ucur_cnt++ < 5) {
			if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0], (dma_addr_t)psrc_p, ONE_LEN_CASE_ENQAFTDONE))
				ERR_FUN_LINE;
		}
	}
#endif

	msleep_interruptible(3000); /* wait all done */

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_LEN_CASE_ENQAFTDONE)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
		uRet = __LINE__;
		goto End;
	}

	/* stop and free dma channel */
	if(0 != sw_dma_free(g_dma_hle, &g_client)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_free success\n", __FUNCTION__);

End:
	if(0 != uRet) {
		pr_err("%s err, line %d!\n", __FUNCTION__, uRet);
	} else {
		pr_info("%s, success!\n", __FUNCTION__);
	}

	/* free dma memory */
	if(NULL != psrc_v)
		dma_free_coherent(NULL, TOTAL_LEN_CASE_ENQAFTDONE, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_LEN_CASE_ENQAFTDONE, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}


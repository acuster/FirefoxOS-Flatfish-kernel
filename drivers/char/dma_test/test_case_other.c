/*
 * drivers/char/dma_test/test_case_other.c
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
#define TOTAL_TRANS_BYTES	SZ_512K
#define ONCE_TRANS_BYTES	TOTAL_TRANS_BYTES

//#define DTC_1T_TOTAL_LEN	SIZE_1M /* may lead to dma_pool_alloc failed in sw_dma_request */

static u32 g_src_addr = 0, g_dst_addr = 0;
static atomic_t g_acur_cnt = ATOMIC_INIT(0);

extern wait_queue_head_t g_dtc_queue[];

/**
 * fd_cb_case_stopcmd - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_case_stopcmd(struct sw_dma_chan * ch, void *buf,
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
	case SW_RES_ABORT:
		pr_info("%s: SW_RES_ABORT!\n", __FUNCTION__);
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
 * wd_case_stopcmd - wait dma done function.
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_case_stopcmd(void)
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
 * __dtc_stop_cmd - dma test case one-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_stop_cmd(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_TRANS_BYTES);
	memset(pdst_v, 0x54, TOTAL_TRANS_BYTES);

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
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle, fd_cb_case_stopcmd)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	{
		u32 	ucnt = 0;
		while(ucnt++ < 50) {
			if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0],
					(dma_addr_t)psrc_p, ONCE_TRANS_BYTES)) {
				uRet = __LINE__;
				goto End;
			}
		}
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

#if 1
	msleep_interruptible(1);
	if(0 != sw_dma_ctrl(g_dma_hle, SW_DMAOP_FLUSH)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_ctrl - SW_DMAOP_FLUSH success\n", __FUNCTION__);
#else
	/* wait dma done */
	if(0 != wd_case_stopcmd()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_case_stopcmd success\n", __FUNCTION__);
#endif

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_TRANS_BYTES)) {
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
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

/**
 * fd_cb_case_manyenq - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_case_manyenq(struct sw_dma_chan * ch, void *buf,
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
 * wd_case_manyenq - wait dma done function.
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_case_manyenq(void)
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
 * __dtc_many_enq - dma test case one-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_many_enq(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_TRANS_BYTES);
	memset(pdst_v, 0x54, TOTAL_TRANS_BYTES);

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
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle, fd_cb_case_manyenq)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	{
		u32 	ucnt = 0;
		while(ucnt++ < 50) {
			if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0],
					(dma_addr_t)psrc_p, ONCE_TRANS_BYTES)) {
				uRet = __LINE__;
				goto End;
			}
		}
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

	/* wait dma done */
	if(0 != wd_case_manyenq()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_case_manyenq success\n", __FUNCTION__);

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_TRANS_BYTES)) {
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
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

/**
 * fd_cb_case_contimod - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_case_contimod(struct sw_dma_chan * ch, void *buf,
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
 * wd_case_contimod - wait dma done function.
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_case_contimod(void)
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
 * __dtc_conti_mod - dma test case one-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_conti_mod(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_TRANS_BYTES);
	memset(pdst_v, 0x54, TOTAL_TRANS_BYTES);

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

	/*
	 * NOTE: we want the buffer auto restart by hardware, and fulldone callback entered continuously.
	 * but when buf done, dma driver auto stop the channel.
	 * securcrt print "dma8: end of transfer, stopping channel (-27163)".
	 * why can continue mode work in a10?
	 */
	g_confs[0].reload	= 1;

	if(0 != sw_dma_config(g_dma_hle, &g_confs[0])) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_config success\n", __FUNCTION__);

	/* set full done callback */
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle, fd_cb_case_contimod)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	if(0 != sw_dma_enqueue(g_dma_hle, &g_confs[0], (dma_addr_t)psrc_p, ONCE_TRANS_BYTES)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

	/* wait dma done */
	if(0 != wd_case_contimod()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_case_contimod success\n", __FUNCTION__);

	msleep_interruptible(4000);

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_TRANS_BYTES)) {
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
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

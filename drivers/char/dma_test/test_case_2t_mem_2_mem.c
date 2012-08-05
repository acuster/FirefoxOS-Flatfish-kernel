/*
 * drivers/char/dma_test/test_case_2t_mem_2_mem.c
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

#define THREAD1_DMA_NAME 	"m2m_dma_thread1"
#define THREAD2_DMA_NAME 	"m2m_dma_thread2"

#define TOTAL_TRANS_BYTES_1	SZ_512K
#define ONCE_TRANS_BYTES_1	TOTAL_TRANS_BYTES_1

#define TOTAL_TRANS_BYTES_2	SZ_32K
#define ONCE_TRANS_BYTES_2	TOTAL_TRANS_BYTES_2

#define TEST_TIME_THREAD1 	0x0fffffff	/* ms */
#define TEST_TIME_THREAD2 	0x0fffffff

static atomic_t 	g_adma_done1 = ATOMIC_INIT(0);	/* dma done flag */
static atomic_t 	g_adma_done2 = ATOMIC_INIT(0);	/* dma done flag */
static u32 		g_sadr1 = 0, g_dadr1 = 0;
static u32 		g_sadr2 = 0, g_dadr2 = 0;
//static atomic_t 	g_acur_cnt1 = ATOMIC_INIT(0);
//static atomic_t 	g_acur_cnt2 = ATOMIC_INIT(0);

struct sw_dma_client 	g_client1 = {
	.name = "sun7i_dma_test",
};
struct dma_hw_conf 	g_confs1[1];

struct sw_dma_client 	g_client2 = {
	.name = "sun7i_dma_test",
};
struct dma_hw_conf 	g_confs2[1];

int 			g_dma_hle1 = -1;
int 			g_dma_hle2 = -1;

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
 * fd_cb_thread2 - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_thread2(struct sw_dma_chan * ch, void *buf,
				int size, enum sw_dma_buffresult result)
{
	u32 	usign = 0;

	printk("%s: ch %d, buf 0x%08x, size %d, result %d\n", __FUNCTION__, ch->number,
		(u32)buf, size, (u32)result);

	switch(result) {
	case SW_RES_OK:
		pr_info("%s: SW_RES_OK!\n", __FUNCTION__);
		atomic_set(&g_adma_done2, 1);
		wake_up_interruptible(&g_dtc_queue[2]);
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
 * wd_thread2 - wait dma done func.
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_thread2(void)
{
	long 	ret = 0;
	long 	timeout = 5 * HZ; /* 5s */

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
 * __thread2_proc - dma test case.
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __thread2_proc(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES_2, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES_2, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_TRANS_BYTES_2);
	memset(pdst_v, 0x54, TOTAL_TRANS_BYTES_2);

	/* init for loop transfer */
	g_sadr2 = psrc_p;
	g_dadr2 = pdst_p;

	/* request dma */
	g_dma_hle2 = sw_dma_request(DMACH_DSDRAM, &g_client2, NULL);
	if(g_dma_hle2 < 0) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_request return g_dma_hle2 %d\n", __FUNCTION__, g_dma_hle2);

	/* set auto start flag */
	if(0 != sw_dma_setflags(g_dma_hle2, SW_DMAF_AUTOSTART)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_setflags success\n", __FUNCTION__);

	/* config dma */
	g_confs2[0].xfer_type 	= DMAXFER_D_BWORD_S_BWORD;
	g_confs2[0].hf_irq 	= SW_DMA_IRQ_FULL;
	g_confs2[0].dir		= SW_DMA_M2M;
	g_confs2[0].to 		= pdst_p;
	g_confs2[0].from 	= psrc_p;
	g_confs2[0].address_type = DMAADDRT_D_INC_S_INC;;
	g_confs2[0].drqsrc_type	= DRQ_TYPE_SDRAM;
	g_confs2[0].drqdst_type	= DRQ_TYPE_SDRAM;
	if(0 != sw_dma_config(g_dma_hle2, &g_confs2[0])) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_config success\n", __FUNCTION__);

	/* set full done callback */
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle2, fd_cb_thread2)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	if(0 != sw_dma_enqueue(g_dma_hle2, &g_confs2[0], (dma_addr_t)psrc_p, ONCE_TRANS_BYTES_2)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

	/* wait dma done */
	if(0 != wd_thread2()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_thread2 success\n", __FUNCTION__);

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_TRANS_BYTES_2)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
		uRet = __LINE__;
		goto End;
	}

	/* stop and free dma channel */
	if(0 != sw_dma_free(g_dma_hle2, &g_client2)) {
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
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES_2, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES_2, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
	return uRet;
}

/**
 * fd_cb_thread1 - full done callback
 *
 * Returns 0 if sucess, the err line number if failed.
 */
void fd_cb_thread1(struct sw_dma_chan * ch, void *buf,
				int size, enum sw_dma_buffresult result)
{
	u32 	usign = 0;

	printk("%s: ch %d, buf 0x%08x, size %d, result %d\n", __FUNCTION__, ch->number,
		(u32)buf, size, (u32)result);

	switch(result) {
	case SW_RES_OK:
		pr_info("%s: SW_RES_OK!\n", __FUNCTION__);
		atomic_set(&g_adma_done1, 1);
		wake_up_interruptible(&g_dtc_queue[1]);
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
 * wd_thread1 - wait dma done for DTC_1T_MEM_2_MEM
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 wd_thread1(void)
{
	long 	ret = 0;
	long 	timeout = 5 * HZ; /* 5s */

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
 * __thread1_proc - dma test case.
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __thread1_proc(void)
{
	u32 	uRet = 0;
	void 	*psrc_v = NULL, *pdst_v = NULL;
	u32 	psrc_p = 0, pdst_p = 0;

	pr_info("%s enter\n", __FUNCTION__);

	/* prepare the buffer and data */
	psrc_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES_1, (dma_addr_t *)&psrc_p, GFP_KERNEL);
	if(NULL == psrc_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: psrc_v 0x%08x, psrc_p 0x%08x\n", __FUNCTION__, (u32)psrc_v, psrc_p);
	pdst_v = dma_alloc_coherent(NULL, TOTAL_TRANS_BYTES_1, (dma_addr_t *)&pdst_p, GFP_KERNEL);
	if(NULL == pdst_v) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s: pdst_v 0x%08x, pdst_p 0x%08x\n", __FUNCTION__, (u32)pdst_v, pdst_p);

	/* get random bytes in src buffer */
	get_random_bytes(psrc_v, TOTAL_TRANS_BYTES_1);
	memset(pdst_v, 0x54, TOTAL_TRANS_BYTES_1);

	/* init for loop transfer */
	g_sadr1 = psrc_p;
	g_dadr1 = pdst_p;

	/* request dma */
	g_dma_hle1 = sw_dma_request(DMACH_DSDRAM, &g_client1, NULL);
	if(g_dma_hle1 < 0) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_request return g_dma_hle1 %d\n", __FUNCTION__, g_dma_hle1);

	/* set auto start flag */
	if(0 != sw_dma_setflags(g_dma_hle1, SW_DMAF_AUTOSTART)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_setflags success\n", __FUNCTION__);

	/* config dma */
	g_confs1[0].xfer_type 	= DMAXFER_D_BWORD_S_BWORD;
	g_confs1[0].hf_irq 	= SW_DMA_IRQ_FULL;
	g_confs1[0].dir		= SW_DMA_M2M;
	g_confs1[0].to 		= pdst_p;
	g_confs1[0].from 	= psrc_p;
	g_confs1[0].address_type = DMAADDRT_D_INC_S_INC;;
	g_confs1[0].drqsrc_type	= DRQ_TYPE_SDRAM;
	g_confs1[0].drqdst_type	= DRQ_TYPE_SDRAM;
	if(0 != sw_dma_config(g_dma_hle1, &g_confs1[0])) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_config success\n", __FUNCTION__);

	/* set full done callback */
	if(0 != sw_dma_set_buffdone_fn(g_dma_hle1, fd_cb_thread1)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_set_buffdone_fn success\n", __FUNCTION__);

	/* enqueue buffer */
	if(0 != sw_dma_enqueue(g_dma_hle1, &g_confs1[0], (dma_addr_t)psrc_p, ONCE_TRANS_BYTES_1)) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s sw_dma_enqueue success\n", __FUNCTION__);

	/* wait dma done */
	if(0 != wd_thread1()) {
		uRet = __LINE__;
		goto End;
	}
	pr_info("%s wd_thread1 success\n", __FUNCTION__);

	/* check if data ok */
	if(0 == memcmp(psrc_v, pdst_v, TOTAL_TRANS_BYTES_1)) {
		pr_info("%s: data check ok!\n", __FUNCTION__);
	} else {
		pr_err("%s: data check err!\n", __FUNCTION__);
		uRet = __LINE__;
		goto End;
	}

	/* stop and free dma channel */
	if(0 != sw_dma_free(g_dma_hle1, &g_client1)) {
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
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES_1, psrc_v, psrc_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);

	if(NULL != pdst_v)
		dma_free_coherent(NULL, TOTAL_TRANS_BYTES_1, pdst_v, pdst_p);

	pr_err("%s, line %d!\n", __FUNCTION__, __LINE__);
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

		if(0 != __thread1_proc()) {
			uRet = __LINE__;
			goto End;
		}

		//msleep(20);

		end_ms = (jiffies * 1000) / HZ;
		pr_info("%s: cur_ms 0x%08x\n", __FUNCTION__, end_ms);
	/*	if(end_ms - begin_ms >= TEST_TIME_THREAD1) {
			pr_info("%s: time passed! ok!\n", __FUNCTION__);
			break;
		}*/
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

		if(0 != __thread2_proc()) {
			uRet = __LINE__;
			goto End;
		}

		//msleep(10);

		end_ms = (jiffies * 1000) / HZ;
		pr_info("%s: cur_ms 0x%08x\n", __FUNCTION__, end_ms);
	/*	if(end_ms - begin_ms >= TEST_TIME_THREAD2) {
			pr_info("%s: time passed! ok!\n", __FUNCTION__);
			break;
		}*/
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

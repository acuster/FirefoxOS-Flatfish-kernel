/*
 * drivers/char/dma_test/test_case_two_thread.c
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

/* dma name for request */
#define THREAD1_DMA_NAME 	"m2m_dma_thread1"
#define THREAD2_DMA_NAME 	"m2m_dma_thread2"
/* test time for each thread */
//#define TEST_TIME_THREAD1 	0x0fffffff	/* ms */
//#define TEST_TIME_THREAD2 	0x0fffffff
#define TEST_TIME_THREAD1 	20000	/* ms */
#define TEST_TIME_THREAD2 	20000

/* dma done flag */
static atomic_t 	g_adma_done1 = ATOMIC_INIT(0);
static atomic_t 	g_adma_done2 = ATOMIC_INIT(0);
/* src/dst buffer physical addr */
static u32 		g_sadr1 = 0, g_dadr1 = 0;
static u32 		g_sadr2 = 0, g_dadr2 = 0;
/* cur buffer index */
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
	int error;

	/* get cur system memory info */
	memset(&s_info, 0, sizeof(s_info));
	error = do_sysinfo(&s_info);
	pr_info("%s: cur time 0x%08xs, total mem %dM, free mem %dM, total high %dM, free high %dM\n", \
		__func__, (u32)s_info.uptime,
		(u32)(s_info.totalram / 1024 / 1024),
		(u32)(s_info.freeram / 1024 / 1024),
		(u32)(s_info.totalhigh / 1024 / 1024),
		(u32)(s_info.freehigh / 1024 / 1024));
	return 0;
}

void __cb_fd_2(dma_hdl_t dma_hdl, void *parg)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = TOTAL_LEN_2 / ONE_LEN_2;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);

	/* enqueue if not done */
	ucur_cnt = atomic_add_return(1, &g_acur_cnt2);
	if(ucur_cnt < uloop_cnt) {
		printk("%s, line %d\n", __func__, __LINE__);
		/* NOTE: fatal err, when read here, g_acur_cnt2 has changed by other place, 2012-12-2 */
		//ucur_saddr = g_sadr2 + atomic_read(&g_acur_cnt2) * ONE_LEN_2;
		ucur_saddr = g_sadr2 + ucur_cnt * ONE_LEN_2;
		ucur_daddr = g_dadr2 + ucur_cnt * ONE_LEN_2;
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, ONE_LEN_2))
			printk("%s err, line %d\n", __func__, __LINE__);
	} else { /* buf enqueue complete */
		printk("%s, line %d\n", __func__, __LINE__);
		//sw_dma_dump_chan(dma_hdl); /* for debug */

		/* maybe it's the last irq */
		atomic_set(&g_adma_done2, 1);
		wake_up_interruptible(&g_dtc_queue[2]);
	}

	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
}

void __cb_hd_2(dma_hdl_t dma_hdl, void *parg)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = TOTAL_LEN_2 / ONE_LEN_2;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);

	/* enqueue if not done */
	ucur_cnt = atomic_add_return(1, &g_acur_cnt2);
	if(ucur_cnt < uloop_cnt) {
		printk("%s, line %d\n", __func__, __LINE__);
		/* NOTE: fatal err, when read here, g_acur_cnt2 has changed by other place, 2012-12-2 */
		//ucur_saddr = g_sadr2 + atomic_read(&g_acur_cnt2) * ONE_LEN_2;
		ucur_saddr = g_sadr2 + ucur_cnt * ONE_LEN_2;
		ucur_daddr = g_dadr2 + ucur_cnt * ONE_LEN_2;
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, ONE_LEN_2))
			printk("%s err, line %d\n", __func__, __LINE__);
	}

	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
}

u32 __waitdone_2(void)
{
	long 	ret = 0;
	long 	timeout = 10 * HZ; /* 10s */

	/* wait dma done */
	ret = wait_event_interruptible_timeout(g_dtc_queue[2], \
		atomic_read(&g_adma_done2)== 1, timeout);
	/* reset dma done flag to 0 */
	atomic_set(&g_adma_done2, 0);

	if(-ERESTARTSYS == ret) {
		pr_info("%s success!\n", __func__);
		return 0;
	} else if(0 == ret) {
		pr_info("%s err, time out!\n", __func__);
		return __LINE__;
	} else {
		pr_info("%s success with condition match, ret %d!\n", __func__, (int)ret);
		return 0;
	}
}

u32 _thread2_proc(void)
{
	u32 	uret = 0, tmp = 0;
	void 	*src_vaddr = NULL, *dst_vaddr = NULL;
	u32 	src_paddr = 0, dst_paddr = 0;
	dma_hdl_t dma_hdl = (dma_hdl_t)NULL;
	dma_cb_t done_cb;
	dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	/* prepare the buffer and data */
	src_vaddr = dma_alloc_coherent(NULL, TOTAL_LEN_2, (dma_addr_t *)&src_paddr, GFP_KERNEL);
	if(NULL == src_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: src_vaddr 0x%08x, src_paddr 0x%08x\n", __func__, (u32)src_vaddr, src_paddr);
	dst_vaddr = dma_alloc_coherent(NULL, TOTAL_LEN_2, (dma_addr_t *)&dst_paddr, GFP_KERNEL);
	if(NULL == dst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: dst_vaddr 0x%08x, dst_paddr 0x%08x\n", __func__, (u32)dst_vaddr, dst_paddr);

	/* init src buffer */
	get_random_bytes(src_vaddr, TOTAL_LEN_2);
	memset(dst_vaddr, 0x54, TOTAL_LEN_2);

	/* init loop para */
	atomic_set(&g_acur_cnt2, 0);
	g_sadr2 = src_paddr;
	g_dadr2 = dst_paddr;

	/* request dma channel */
	dma_hdl = sw_dma_request(THREAD2_DMA_NAME, CHAN_DEDICATE);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set full done callback */
	done_cb.func = __cb_fd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	/* set half done callback */
	done_cb.func = __cb_hd_2;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);

	/* config para */
	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.xfer_type.src_data_width 	= DATA_WIDTH_32BIT;
	dma_config.xfer_type.src_bst_len 	= DATA_BRST_4;
	dma_config.xfer_type.dst_data_width 	= DATA_WIDTH_32BIT;
	dma_config.xfer_type.dst_bst_len 	= DATA_BRST_4;
	dma_config.address_type.src_addr_mode 	= DDMA_ADDR_LINEAR;
	dma_config.address_type.dst_addr_mode 	= DDMA_ADDR_LINEAR;
	dma_config.src_drq_type 	= D_SRC_SDRAM;
	dma_config.dst_drq_type 	= D_DST_SDRAM;
	dma_config.bconti_mode 		= false;
	dma_config.irq_spt 		= CHAN_IRQ_HD | CHAN_IRQ_FD;
	if(0 != sw_dma_config(dma_hdl, &dma_config)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_config success\n", __func__);

	/* set src/dst secu */
	tmp = SRC_SECU_DST_SECU;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_SECURITY, &tmp)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: DMA_OP_SET_SECURITY success\n", __func__);

	/* set para reg, ddma only */
	{
		dma_para_t para;
		para.src_blk_sz 	= 0;
		para.src_wait_cyc 	= 0;
		para.dst_blk_sz 	= 0;
		para.dst_wait_cyc 	= 0;
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_PARA_REG, &tmp)) {
			uret = __LINE__;
			goto end;
		}
		pr_info("%s: DMA_OP_SET_PARA_REG success\n", __func__);
	}

	/* enqueue first buf */
	if(0 != sw_dma_enqueue(dma_hdl, g_sadr2, g_dadr2, ONE_LEN_2)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_enqueue first buf success\n", __func__);

	/* dump chain */
	sw_dma_dump_chan(dma_hdl);

	/* start dma */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uret = __LINE__;
		goto end;
	}

	/* wait dma done */
	if(0 != __waitdone_2()) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: __waitdone_2 sucess\n", __func__);

	/*
	 * NOTE: must sleep here, becase when __waitdone_2 return, buffer enqueue complete, but
	 * data might not transfer complete, 2012-11-14
	 */
	msleep(2000);

	/* check if data ok */
	if(0 == memcmp(src_vaddr, dst_vaddr, TOTAL_LEN_2))
		pr_info("%s: data check ok!\n", __func__);
	else {
		pr_err("%s: data check err!\n", __func__);
		uret = __LINE__; /* return err */
		goto end;
	}

	/* stop and release dma channel */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_stop success\n", __func__);
	if(0 != sw_dma_release(dma_hdl)) {
		uret = __LINE__;
		goto end;
	}
	dma_hdl = (dma_hdl_t)NULL;
	pr_info("%s: sw_dma_release success\n", __func__);

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret); /* print err line */
	else
		pr_info("%s, success!\n", __func__);

	/* stop and free dma channel, if need */
	if((dma_hdl_t)NULL != dma_hdl) {
		pr_err("%s, stop and release dma handle now!\n", __func__);
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		if(0 != sw_dma_release(dma_hdl))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
	}
	pr_info("%s, line %d!\n", __func__, __LINE__);

	/* free dma memory */
	if(NULL != src_vaddr)
		dma_free_coherent(NULL, TOTAL_LEN_2, src_vaddr, src_paddr);
	if(NULL != dst_vaddr)
		dma_free_coherent(NULL, TOTAL_LEN_2, dst_vaddr, dst_paddr);

	pr_info("%s, end!\n", __func__);
	return uret;
}

void __cb_fd_1(dma_hdl_t dma_hdl, void *parg)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = TOTAL_LEN_1 / ONE_LEN_1;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);

	/* enqueue if not done */
	ucur_cnt = atomic_add_return(1, &g_acur_cnt1);
	if(ucur_cnt < uloop_cnt) {
		printk("%s, line %d\n", __func__, __LINE__);
		/* NOTE: fatal err, when read here, g_acur_cnt1 has changed by other place, 2012-12-2 */
		//ucur_saddr = g_sadr1 + atomic_read(&g_acur_cnt1) * ONE_LEN_1;
		ucur_saddr = g_sadr1 + ucur_cnt * ONE_LEN_1;
		ucur_daddr = g_dadr1 + ucur_cnt * ONE_LEN_1;
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, ONE_LEN_1))
			printk("%s err, line %d\n", __func__, __LINE__);
	} else { /* buf enqueue complete */
		printk("%s, line %d\n", __func__, __LINE__);
		//sw_dma_dump_chan(dma_hdl); /* for debug */

		/* maybe it's the last irq */
		atomic_set(&g_adma_done1, 1);
		wake_up_interruptible(&g_dtc_queue[1]);
	}

	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
}

void __cb_hd_1(dma_hdl_t dma_hdl, void *parg)
{
	u32 	uret = 0;
	u32	ucur_saddr = 0, ucur_daddr = 0;
	u32	uloop_cnt = TOTAL_LEN_1 / ONE_LEN_1;
	u32 	ucur_cnt = 0;

	pr_info("%s: called!\n", __func__);

	/* enqueue if not done */
	ucur_cnt = atomic_add_return(1, &g_acur_cnt1);
	if(ucur_cnt < uloop_cnt) {
		printk("%s, line %d\n", __func__, __LINE__);
		/* NOTE: fatal err, when read here, g_acur_cnt1 has changed by other place, 2012-12-2 */
		//ucur_saddr = g_sadr1 + atomic_read(&g_acur_cnt1) * ONE_LEN_1;
		ucur_saddr = g_sadr1 + ucur_cnt * ONE_LEN_1;
		ucur_daddr = g_dadr1 + ucur_cnt * ONE_LEN_1;
		if(0 != sw_dma_enqueue(dma_hdl, ucur_saddr, ucur_daddr, ONE_LEN_1))
			printk("%s err, line %d\n", __func__, __LINE__);
	}

	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
}

u32 __waitdone_1(void)
{
	long 	ret = 0;
	long 	timeout = 10 * HZ; /* 10s */

	/* wait dma done */
	ret = wait_event_interruptible_timeout(g_dtc_queue[1], \
		atomic_read(&g_adma_done1)== 1, timeout);
	/* reset dma done flag to 0 */
	atomic_set(&g_adma_done1, 0);

	if(-ERESTARTSYS == ret) {
		pr_info("%s success!\n", __func__);
		return 0;
	} else if(0 == ret) {
		pr_info("%s err, time out!\n", __func__);
		return __LINE__;
	} else {
		pr_info("%s success with condition match, ret %d!\n", __func__, (int)ret);
		return 0;
	}
}

u32 _thread1_proc(void)
{
	u32 	uret = 0, tmp = 0;
	void 	*src_vaddr = NULL, *dst_vaddr = NULL;
	u32 	src_paddr = 0, dst_paddr = 0;
	dma_hdl_t dma_hdl = (dma_hdl_t)NULL;
	dma_cb_t done_cb;
	dma_config_t dma_config;

	pr_info("%s enter\n", __func__);

	/* prepare the buffer and data */
	src_vaddr = dma_alloc_coherent(NULL, TOTAL_LEN_1, (dma_addr_t *)&src_paddr, GFP_KERNEL);
	if(NULL == src_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: src_vaddr 0x%08x, src_paddr 0x%08x\n", __func__, (u32)src_vaddr, src_paddr);
	dst_vaddr = dma_alloc_coherent(NULL, TOTAL_LEN_1, (dma_addr_t *)&dst_paddr, GFP_KERNEL);
	if(NULL == dst_vaddr) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: dst_vaddr 0x%08x, dst_paddr 0x%08x\n", __func__, (u32)dst_vaddr, dst_paddr);

	/* init src buffer */
	get_random_bytes(src_vaddr, TOTAL_LEN_1);
	memset(dst_vaddr, 0x54, TOTAL_LEN_1);

	/* init loop para */
	atomic_set(&g_acur_cnt1, 0);
	g_sadr1 = src_paddr;
	g_dadr1 = dst_paddr;

	/* request dma channel */
	dma_hdl = sw_dma_request(THREAD1_DMA_NAME, CHAN_DEDICATE);
	if(NULL == dma_hdl) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_request success, dma_hdl 0x%08x\n", __func__, (u32)dma_hdl);

	/* set full done callback */
	done_cb.func = __cb_fd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set fulldone_cb success\n", __func__);
	/* set half done callback */
	done_cb.func = __cb_hd_1;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_HD_CB, (void *)&done_cb)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: set halfdone_cb success\n", __func__);

	/* config para */
	memset(&dma_config, 0, sizeof(dma_config));
	dma_config.xfer_type.src_data_width 	= DATA_WIDTH_32BIT;
	dma_config.xfer_type.src_bst_len 	= DATA_BRST_4;
	dma_config.xfer_type.dst_data_width 	= DATA_WIDTH_32BIT;
	dma_config.xfer_type.dst_bst_len 	= DATA_BRST_4;
	dma_config.address_type.src_addr_mode 	= DDMA_ADDR_LINEAR;
	dma_config.address_type.dst_addr_mode 	= DDMA_ADDR_LINEAR;
	dma_config.src_drq_type 	= D_SRC_SDRAM;
	dma_config.dst_drq_type 	= D_DST_SDRAM;
	dma_config.bconti_mode 		= false;
	dma_config.irq_spt 		= CHAN_IRQ_HD | CHAN_IRQ_FD;
	if(0 != sw_dma_config(dma_hdl, &dma_config)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_config success\n", __func__);

	/* set src/dst secu */
	tmp = SRC_SECU_DST_SECU;
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_SECURITY, &tmp)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: DMA_OP_SET_SECURITY success\n", __func__);

	/* set para reg, ddma only */
	{
		dma_para_t para;
		para.src_blk_sz 	= 0;
		para.src_wait_cyc 	= 0;
		para.dst_blk_sz 	= 0;
		para.dst_wait_cyc 	= 0;
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_SET_PARA_REG, &tmp)) {
			uret = __LINE__;
			goto end;
		}
		pr_info("%s: DMA_OP_SET_PARA_REG success\n", __func__);
	}

	/* enqueue first buf */
	if(0 != sw_dma_enqueue(dma_hdl, g_sadr1, g_dadr1, ONE_LEN_1)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_enqueue first buf success\n", __func__);

	/* dump chain */
	sw_dma_dump_chan(dma_hdl);

	/* start dma */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_START, NULL)) {
		uret = __LINE__;
		goto end;
	}

	/* wait dma done */
	if(0 != __waitdone_1()) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: __waitdone_1 sucess\n", __func__);

	/*
	 * NOTE: must sleep here, becase when __waitdone_1 return, buffer enqueue complete, but
	 * data might not transfer complete, 2012-11-14
	 */
	msleep(2000);

	/* check if data ok */
	if(0 == memcmp(src_vaddr, dst_vaddr, TOTAL_LEN_1))
		pr_info("%s: data check ok!\n", __func__);
	else {
		pr_err("%s: data check err!\n", __func__);
		uret = __LINE__; /* return err */
		goto end;
	}

	/* stop and release dma channel */
	if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL)) {
		uret = __LINE__;
		goto end;
	}
	pr_info("%s: sw_dma_stop success\n", __func__);
	if(0 != sw_dma_release(dma_hdl)) {
		uret = __LINE__;
		goto end;
	}
	dma_hdl = (dma_hdl_t)NULL;
	pr_info("%s: sw_dma_release success\n", __func__);

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret); /* print err line */
	else
		pr_info("%s, success!\n", __func__);

	/* stop and free dma channel, if need */
	if((dma_hdl_t)NULL != dma_hdl) {
		pr_err("%s, stop and release dma handle now!\n", __func__);
		if(0 != sw_dma_ctl(dma_hdl, DMA_OP_STOP, NULL))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
		if(0 != sw_dma_release(dma_hdl))
			pr_err("%s err, line %d!\n", __func__, __LINE__);
	}
	pr_info("%s, line %d!\n", __func__, __LINE__);

	/* free dma memory */
	if(NULL != src_vaddr)
		dma_free_coherent(NULL, TOTAL_LEN_1, src_vaddr, src_paddr);
	if(NULL != dst_vaddr)
		dma_free_coherent(NULL, TOTAL_LEN_1, dst_vaddr, dst_paddr);

	pr_info("%s, end!\n", __func__);
	return uret;
}

int __test_thread1(void * arg)
{
	u32 	uret = 0;
	u32 	begin_ms = 0, end_ms = 0;

	begin_ms = (jiffies * 1000) / HZ;
	pr_info("%s: begin_ms 0x%08x\n", __func__, begin_ms);
	/* loop test until time passed */
	while(1) {
		__dump_cur_mem_info();
		if(0 != _thread1_proc()) {
			uret = __LINE__;
			goto end;
		}

		/* calculate time passed */
		end_ms = (jiffies * 1000) / HZ;
		pr_info("%s: cur_ms 0x%08x\n", __func__, end_ms);
		if(end_ms - begin_ms >= TEST_TIME_THREAD1) {
			pr_info("%s: time passed! ok!\n", __func__);
			break;
		}
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	else
		pr_info("%s success!\n", __func__);

	return uret;
}

int __test_thread2(void * arg)
{
	u32 	uret = 0;
	u32 	begin_ms = 0, end_ms = 0;

	begin_ms = (jiffies * 1000) / HZ;
	while(1) {
		__dump_cur_mem_info();
		if(0 != _thread2_proc()) {
			uret = __LINE__;
			goto end;
		}

		/* calculate time passed */
		end_ms = (jiffies * 1000) / HZ;
		if(end_ms - begin_ms >= TEST_TIME_THREAD2) {
			pr_info("%s: time passed! ok!\n", __func__);
			break;
		}
	}

end:
	if(0 != uret)
		pr_err("%s err, line %d!\n", __func__, uret);
	else
		pr_info("%s success!\n", __func__);

	return uret;
}

/**
 * __dtc_two_thread - dma test case two-thread from memory to memory
 *
 * Returns 0 if success, the err line number if failed.
 */
u32 __dtc_two_thread(void)
{
	/* dump the initial memory status, for test memory leak */
	__dump_cur_mem_info();

	kernel_thread(__test_thread1, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(__test_thread2, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

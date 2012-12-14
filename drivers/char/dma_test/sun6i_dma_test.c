/*
 * drivers/char/dma_test/sun6i_dma_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "sun6i_dma_test.h"

#include "test_case_1t_mem_2_mem.h"
#include "test_case_2t_mem_2_mem.h"
#include "test_case_other.h"
#include "test_case_single_md.h"
#include "test_case_single_conti_md.h"

/* cur test case */
static enum dma_test_case_e g_cur_test_case = DTC_SINGLE_MODE;

/* wait dma done queue, used for wait dma done */
wait_queue_head_t	g_dtc_queue[DTC_MAX];
atomic_t 		g_adma_done = ATOMIC_INIT(0);	/* dma done flag */

/**
 * __dma_test_init_waitqueue - init dma wait queue
 */
static void __dma_test_init_waitqueue(void)
{
	u32 	i = 0;

	for(i = 0; i < DTC_MAX; i++)
		init_waitqueue_head(&g_dtc_queue[i]);
}

/**
 * __dma_test_thread - dma test main thread
 * @arg:	thread arg, not used
 *
 * Returns 0 if success, the err line number if failed.
 */
static int __dma_test_thread(void * arg)
{
	u32 uResult = 0;

	/* init dma wait queue */
	__dma_test_init_waitqueue();

	switch(g_cur_test_case) {
	case DTC_1T_MEM_2_MEM:
		uResult = __dtc_1t_mem_2_mem();
		break;
	case DTC_SINGLE_MODE:
		uResult = __dtc_single_mode();
		break;
	case DTC_SINGLE_CONT_MODE:
		uResult = __dtc_sgct_mode();
		break;
	case DTC_1T_ENQ_AFT_DONE:
		uResult = __dtc_case_enq_aftdone();
		break;
	case DTC_1TM2M_MANY_ENQ:
		uResult = __dtc_many_enq();
		break;
	case DTC_1TM2M_CONTI_MOD:
		uResult = __dtc_conti_mode();
		break;
	case DTC_1T_CMD_STOP:
		uResult = __dtc_stopcmd();
		break;
	case DTC_2T_MEM_2_MEM:
		uResult = __dtc_2t_mem_2_mem();
		break;
	default:
		uResult = __LINE__;
		break;
	}

	if(0 == uResult)
		printk("%s: test success!\n", __func__);
	else
		printk("%s: test failed!\n", __func__);
	return uResult;
}

/**
 * sw_dma_test_init - enter the dma test module
 * return 0
 */
static int __init sw_dma_test_init(void)
{
	pr_info("%s enter\n", __func__);

	/* create the test thread */
	kernel_thread(__dma_test_thread, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

/**
 * sw_dma_test_exit - exit the dma test module
 */
static void __exit sw_dma_test_exit(void)
{
	pr_info("sw_dma_test_exit: enter\n");
}

#ifdef MODULE
module_init(sw_dma_test_init);
module_exit(sw_dma_test_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("liugang");
MODULE_DESCRIPTION ("sun6i Dma Test driver code");
#else
__initcall(sw_dma_test_init);
#endif /* MODULE */

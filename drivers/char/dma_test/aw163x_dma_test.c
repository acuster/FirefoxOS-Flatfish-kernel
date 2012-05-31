/*
 * drivers/char/dma_test/aw163x_dma_test.c
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

#include "test_case_1t_mem_2_mem.h"
#include "test_case_2t_mem_2_mem.h"
#include "test_case_other.h"

/*
 * cur test case
 */
enum dma_test_case_e g_cur_test_case = DTC_1T_MEM_2_MEM;
//enum dma_test_case_e g_cur_test_case = DTC_MAX;

/*
 * wait dma done queue, used for wait dma done
 */
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
	u32 	uResult = 0;

	/*
	 * init dma wait queue
	 */
	__dma_test_init_waitqueue();

	switch(g_cur_test_case) {
	case DTC_1T_MEM_2_MEM:
		uResult = __dtc_1t_mem_2_mem();
		break;
	case DTC_1T_ENQ_AFTER_LASTDONE:
		uResult = __dtc_enque_after_done();
		break;
	case DTC_1T_CMD_STOP:
		uResult = __dtc_stopcmd();
		break;
	case DTC_1T_NAND_DMA_RW:
		pr_err("%s err, cannot test, because open(/dev/nande... linked failed, \
			please use nand_init -> __dma_rw_thread instead!\n", __FUNCTION__);
		break;
	case DTC_2T_MEM_2_MEM:
		uResult = __dtc_2t_mem_2_mem();
		break;

	case DTC_2T_USB_COPY_MANUAL:
		/*
		 * dma test case two-thread usb read/write nand
		 * eg: one thread copy files from PC to nand-udisk, the other thread
		 * copy files from nand to PC, use beyond compare to check if transfer correct.
		 *
		 * Returns 0 if success, the err line number if failed.
		 */
		printk("%s: please test manually, make sure that usb and nand driver is ok!\n", __FUNCTION__);
		break;
	case DTC_2T_M2M_N2M_LOOP:
		/*
		 * dma test case two-thread loop, eg: one
		 * thread memory to memory, the other thread nand to memory,
		 * and loop the operation.
		 *
		 * Returns 0 if success, the err line number if failed.
		 */
		DBG_FUN_LINE_TODO;
		break;
	case DTC_1T_APP_CB_ENQUE:
		printk("%s err: DTC_1T_APP_CB_ENQUE, not support yet!\n", __FUNCTION__);
		uResult = __LINE__;
		break;
	default:
		uResult = __LINE__;
		break;
	}

	if(0 == uResult)
		printk("%s: test success!\n", __FUNCTION__);
	else
		printk("%s: test failed!\n", __FUNCTION__);

	return uResult;
}

/**
 * sw_dma_test_init - enter the dma test module
 */
static int __init sw_dma_test_init(void)
{
	pr_info("%s enter\n", __FUNCTION__);

	/*
	 * create the test thread
	 */
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
MODULE_DESCRIPTION ("aw163x Dma Test driver code");
#else
__initcall(sw_dma_test_init);
#endif /* MODULE */

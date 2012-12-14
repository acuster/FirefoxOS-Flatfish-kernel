/*
 * drivers/char/dma_test/sun6i_dma_test.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma test head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUN6I_DMA_TEST_H
#define __SUN6I_DMA_TEST_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/dma.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/dma-mapping.h>
#include <linux/wait.h>
#include <linux/random.h>

#include <mach/dma.h>

/*
 * dma test case id
 */
enum dma_test_case_e {
	DTC_1T_MEM_2_MEM,	/* dma test case one-thread from memory to memory */
	DTC_SINGLE_MODE,	/* dma test case for single mode */
	DTC_SINGLE_CONT_MODE,	/* dma test case for single mode & continue mode */
	DTC_2T_MEM_2_MEM,	/* dma test case two-thread from memory to memory,
				 * memory range should not be conflict, eg: thread one
				 * from memory-A to memory-B, thread two from C to D.
				 */
	DTC_1TM2M_MANY_ENQ, 	/* dma test case one-thread memory to memory, many enqueue */
	DTC_1TM2M_CONTI_MOD,	/* dma test case one-thread memory to memory, continue mode */
	DTC_1T_ENQ_AFT_DONE,	/* check if dma driver can continue transfer new enqueued buffer after done */
	DTC_1T_CMD_STOP,	/* stop when dma running */
	DTC_MAX
};

extern wait_queue_head_t g_dtc_queue[];
extern atomic_t g_adma_done;

#endif /* __SUN6I_DMA_TEST_H */


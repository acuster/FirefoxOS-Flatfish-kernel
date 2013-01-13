/*
 * drivers/char/dma_test/sun7i_dma_test.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun7i dma test head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUN7I_DMA_TEST_H
#define __SUN7I_DMA_TEST_H

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

#include "test_case_normal.h"
#include "test_case_two_thread.h"
#include "test_case_other.h"
#include "test_case_dedicate.h"

/*
 * dma test case id
 */
enum dma_test_case_e {
	DTC_NORMAL = 0,		/* case for normal channel */
	DTC_NORMAL_CONT_MODE,	/* case for normal channel continue mode */
	DTC_DEDICATE,		/* case for dedicate channel */
	DTC_DEDICATE_CONT_MODE,	/* case for dedicate channel continue mode */
	/*
	 * for dedicate below
	 */
	DTC_ENQ_AFT_DONE,	/* enqueued buffer after dma last done, to see if can cotinue auto start */
	DTC_MANY_ENQ, 		/* many buffer enqueued, function test */
	DTC_CMD_STOP,		/* stop when dma running */
	DTC_M2M_TWO_THREAD,	/* two-thread run simutalously, pressure test and memory leak test */
	DTC_MAX
};

extern wait_queue_head_t g_dtc_queue[];
extern atomic_t g_adma_done;

#endif /* __SUN7I_DMA_TEST_H */

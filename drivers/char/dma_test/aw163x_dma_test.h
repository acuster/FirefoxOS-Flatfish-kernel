/*
 * drivers/char/dma_test/aw163x_dma_test.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma test head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __AW163X_DMA_TEST_H
#define __AW163X_DMA_TEST_H

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

#include <mach/dma.h> /* how to search arch/arm/mach-aw163x/include/mach/dma.h? */

/*
 * dma test case id
 */
enum dma_test_case_e {
	DTC_1T_MEM_2_MEM, 	/* dma test case one-thread from memory to memory */
	DTC_1T_NAND_DMA_RW, 	/* dma test case one-thread from memory to nand,
				 * 2012-5-16 09:13, as we cannot compile open(/dev/nanda...), read(...),
				 * we cannot open nand dev to raw read/write, so we
				 * test in nand driver (nand_init -> __dma_rw_thread) */
	DTC_2T_MEM_2_MEM, 	/* dma test case two-thread from memory to memory,
				 * memory range should not be conflict, eg: thread one
				 * from memory-A to memory-B, thread two from C to D.
				 */

	DTC_2T_USB_COPY_MANUAL, /* dma test case two-thread usb read/write nand, eg: one
				 * thread copy files from PC to nand-udisk, the other thread
				 * copy files from nand to PC, use beyond compare to check if
				 * transfer correct.
				 */

	DTC_2T_M2M_N2M_LOOP,    /* dma test case two-thread loop, eg: one
				 * thread memory to memory, the other thread nand to memory,
				 * and loop the operation. test in nand_init as DTC_1T_NAND_DMA_RW case
				 */
	DTC_1T_APP_CB_ENQUE,	/* app and callback enqueue simutanously */
	DTC_1T_ENQ_AFTER_LASTDONE, /* test enqueue after done */
	DTC_1T_CMD_STOP, 	/* stop when dma running */
	DTC_MAX
};

extern wait_queue_head_t g_dtc_queue[];
extern atomic_t g_adma_done;

#define DBG_FUN_LINE_TODO	printk("%s, line %d, todo############\n", __FUNCTION__, __LINE__)
#define DBG_FUN_LINE 		printk("%s, line %d\n", __FUNCTION__, __LINE__)
#define ERR_FUN_LINE 		printk("%s err, line %d\n", __FUNCTION__, __LINE__)

#endif /* __AW163X_DMA_TEST_H */

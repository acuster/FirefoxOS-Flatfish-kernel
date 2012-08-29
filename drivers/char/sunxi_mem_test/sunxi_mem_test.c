/*
 * drivers/char/sunxi_mem_test/sunxi_mem_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi sunxi_mem test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <linux/sunxi_physmem.h>

/*
 * usage:
 * 	func_name: function name to defination
 *      test_sec:  test time in seconds, 0 means test forever.
 */
#define DEFINE_SUNXIMEMTEST_FUNC(func_name, test_sec)								\
static int func_name(void * arg)										\
{														\
	do {													\
		u32 	va = 0, pa = 0, rest_size = 0, temp = 0;						\
		u32	size = (get_random_int() % 32) * SZ_1M;							\
		struct timeval start_time, end_time;								\
														\
		printk("%s start: size %d Mbytes, test_sec %d\n", __func__, size / SZ_1M, test_sec);		\
														\
		do_gettimeofday(&start_time);									\
		while(1) {											\
			do_gettimeofday(&end_time);								\
			if(0 == test_sec || end_time.tv_sec - start_time.tv_sec <= test_sec) {			\
				if(false == sunxi_mem_alloc(size, &va, &pa))					\
					printk("%s: out of memory! size 0x%08x\n", __func__, size);		\
				else {										\
					temp = (get_random_int() % 10) * 100; msleep(temp);			\
					rest_size = sunxi_mem_get_rest_size();					\
					printk("%s: alloc %d Mbytes success, va 0x%08x, pa 0x%08x, sleep %d ms, rest %d Mbytes\n",\
						__func__, size / SZ_1M, va, pa, temp, rest_size / SZ_1M);	\
					sunxi_mem_free(va, pa);							\
					va = 0;									\
					pa = 0;									\
				}										\
				msleep(100);									\
			} else {										\
				printk("%s: time passed!\n", __func__);						\
				break;										\
			}											\
		}												\
														\
		rest_size = sunxi_mem_get_rest_size();								\
		printk("%s end, rest size %d Mbytes\n", __func__, rest_size / SZ_1M);				\
		return 0;											\
	}while(0);												\
}

/*
 * function defination below, NOTE: cannot add ";" in end,
 * eg: "DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread1)" ok but
 *     "DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread1);" is err.
 *
 * the second para is test time in seconds, 0 means test forever.
 */
DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread1, 0)
DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread2, 0)
DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread3, 0)
DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread4, 0)
DEFINE_SUNXIMEMTEST_FUNC(sunxi_mem_test_thread5, 0)

static int __init sunxi_mem_test_init(void)
{
	printk("%s enter\n", __func__);

	kernel_thread(sunxi_mem_test_thread1, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunxi_mem_test_thread2, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunxi_mem_test_thread3, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunxi_mem_test_thread4, NULL, CLONE_FS | CLONE_SIGHAND);
	kernel_thread(sunxi_mem_test_thread5, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

static void __exit sunxi_mem_test_exit(void)
{
	printk("%s enter\n", __func__);
}

module_init(sunxi_mem_test_init);
module_exit(sunxi_mem_test_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("liugang");
MODULE_DESCRIPTION ("sun7i sunxi_mem test driver code");

/*
 * drivers/char/dma_test/sun7i_dma_test.c
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

/* wait dma done queue, used for wait dma done */
wait_queue_head_t	g_dtc_queue[DTC_MAX];
atomic_t 		g_adma_done = ATOMIC_INIT(0);	/* dma done flag */

const char *case_name[] = {
	"DTC_NORMAL",
	"DTC_NORMAL_CONT_MODE",
	"DTC_DEDICATE",
	"DTC_DEDICATE_CONT_MODE",
	"DTC_ENQ_AFT_DONE",
	"DTC_MANY_ENQ",
	"DTC_CMD_STOP",
	"DTC_M2M_TWO_THREAD",
};

/**
 * __dma_test_init_waitqueue - init dma wait queue
 */
static void __dma_test_init_waitqueue(void)
{
	u32 i = (u32)DTC_MAX;

	while(i--)
		init_waitqueue_head(&g_dtc_queue[i]);
}

static int dma_test_main(int id)
{
	enum dma_test_case_e cur_test = (enum dma_test_case_e)id;
	u32 ret = 0;

	switch(cur_test) {
	case DTC_NORMAL:
		ret = __dtc_normal();
		break;
	case DTC_NORMAL_CONT_MODE:
		ret = __dtc_normal_conti();
		break;
	case DTC_DEDICATE:
		ret = __dtc_dedicate();
		break;
	case DTC_DEDICATE_CONT_MODE:
		ret = __dtc_dedicate_conti();
		break;
	case DTC_ENQ_AFT_DONE:
		ret = __dtc_enq_aftdone();
		break;
	case DTC_MANY_ENQ:
		ret = __dtc_many_enq();
		break;
	case DTC_CMD_STOP:
		ret = __dtc_stop();
		break;
	case DTC_M2M_TWO_THREAD:
		ret = __dtc_two_thread();
		break;
	default:
		ret = __LINE__;
		break;
	}

	if(0 == ret)
		printk("%s: test success!\n", __func__);
	else
		printk("%s: test failed!\n", __func__);
	return ret;
}

ssize_t test_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	int id = 0;

	/* get test id */
	if(strict_strtoul(buf, 10, (long unsigned int *)&id)) {
		pr_err("%s: invalid string %s\n", __func__, buf);
		return -EINVAL;
	}
	pr_info("%s: string %s, test case %s\n", __func__, buf, case_name[id]);

	if(0 != dma_test_main(id))
		pr_err("%s: dma_test_main failed! id %d\n", __func__, id);
	else
		pr_info("%s: dma_test_main success! id %d\n", __func__, id);
	return size;
}

ssize_t help_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t cnt = 0;

	cnt += sprintf(buf + cnt, "usage: echo id > test\n");
	cnt += sprintf(buf + cnt, "     id for case DTC_NORMAL             is %d\n", (int)DTC_NORMAL);
	cnt += sprintf(buf + cnt, "     id for case DTC_NORMAL_CONT_MODE   is %d\n", (int)DTC_NORMAL_CONT_MODE);
	cnt += sprintf(buf + cnt, "     id for case DTC_DEDICATE           is %d\n", (int)DTC_DEDICATE);
	cnt += sprintf(buf + cnt, "     id for case DTC_DEDICATE_CONT_MODE is %d\n", (int)DTC_DEDICATE_CONT_MODE);
	cnt += sprintf(buf + cnt, "     id for case DTC_ENQ_AFT_DONE       is %d\n", (int)DTC_ENQ_AFT_DONE);
	cnt += sprintf(buf + cnt, "     id for case DTC_MANY_ENQ           is %d\n", (int)DTC_MANY_ENQ);
	cnt += sprintf(buf + cnt, "     id for case DTC_CMD_STOP           is %d\n", (int)DTC_CMD_STOP);
	cnt += sprintf(buf + cnt, "     id for case DTC_M2M_TWO_THREAD     is %d\n", (int)DTC_M2M_TWO_THREAD);
	cnt += sprintf(buf + cnt, "case description:\n");
	cnt += sprintf(buf + cnt, "     DTC_NORMAL:             case for normal channel\n");
	cnt += sprintf(buf + cnt, "     DTC_NORMAL_CONT_MODE:   case for normal channel continue mode\n");
	cnt += sprintf(buf + cnt, "     DTC_DEDICATE:           case for dedicate channel\n");
	cnt += sprintf(buf + cnt, "     DTC_DEDICATE_CONT_MODE: case for dedicate channel continue mode\n");
	cnt += sprintf(buf + cnt, "   below is for dedicate:\n");
	cnt += sprintf(buf + cnt, "     DTC_ENQ_AFT_DONE:       enqueued buffer after dma last done, to see if can cotinue auto start\n");
	cnt += sprintf(buf + cnt, "     DTC_MANY_ENQ:           many buffer enqueued, function test\n");
	cnt += sprintf(buf + cnt, "     DTC_CMD_STOP:           stop when dma running\n");
	cnt += sprintf(buf + cnt, "     DTC_M2M_TWO_THREAD:     two-thread run simutalously, pressure test and memory leak test\n");
	return cnt;
}

static struct class_attribute dma_test_class_attrs[] = {
	__ATTR(test, 0220, NULL, test_store), /* not 222, for CTS, other group cannot have write permission, 2013-1-11 */
	__ATTR(help, 0444, help_show, NULL),
	__ATTR_NULL,
};

static struct class dma_test_class = {
	.name		= "sunxi_dma_test",
	.owner		= THIS_MODULE,
	.class_attrs	= dma_test_class_attrs,
};

static int __init sw_dma_test_init(void)
{
	int	status;

	pr_info("%s enter\n", __func__);

	/* init dma wait queue */
	__dma_test_init_waitqueue();

	/* register sys class */
	status = class_register(&dma_test_class);
	if(status < 0)
		pr_info("%s err, status %d\n", __func__, status);
	else
		pr_info("%s success\n", __func__);
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
MODULE_DESCRIPTION ("sun7i Dma Test driver code");
#else
__initcall(sw_dma_test_init);
#endif /* MODULE */

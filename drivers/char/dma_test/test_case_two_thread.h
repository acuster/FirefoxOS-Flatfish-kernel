/*
 * drivers/char/dma_test/test_case_two_thread.h
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

#ifndef __TEST_CASE_TWO_THREAD_H
#define __TEST_CASE_TWO_THREAD_H

/* total length and one buf length */
#define TOTAL_LEN_2		SZ_512K
#define ONE_LEN_2		SZ_16K

#define TOTAL_LEN_1		SZ_512K
#define ONE_LEN_1		SZ_64K

u32 __dtc_two_thread(void);

#endif /* __TEST_CASE_TWO_THREAD_H */

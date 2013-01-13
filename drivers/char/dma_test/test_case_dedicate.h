/*
 * drivers/char/dma_test/test_case_dedicate.h
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

#ifndef __TEST_CASE_DEDICATE
#define __TEST_CASE_DEDICATE

/* total length and one buf length */
#define TOTAL_LEN_DEDICATE	SZ_256K
#define ONE_LEN_DEDICATE	SZ_16K

u32 __dtc_dedicate(void);
u32 __dtc_dedicate_conti(void);
u32 __dtc_app_cb_eque(void);
u32 __dtc_case_enq_aftdone(void);


#endif /* __TEST_CASE_DEDICATE */

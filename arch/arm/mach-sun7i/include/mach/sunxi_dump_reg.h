/*
 * arch/arm/mach-sun7i/include/mach/sunxi_dump_reg.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi dump reg head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_DUMP_REG_H
#define __SUNXI_DUMP_REG_H

#define MAX_COMPARE_ITEM 	256

/**
 * compare_item - reg compare item struct
 * @reg_addr:	reg physical address.
 * @val_expect: expected value, provided by caller.
 * @val_mask:   mask value, provided by caller. only mask bits will be compared.
 */
struct compare_item {
	u32 	reg_addr;
	u32	val_expect;
	u32	val_mask;
};

/**
 * compare_group - reg compare group struct
 * @num:	pitem element count.
 * @pitem: 	items that will be compared, provided by caller.
 */
struct compare_group {
	u32	num;
	struct compare_item *pitem;
};

void sunxi_dump_reg(u32 start_reg, u32 end_reg);
void sunxi_dump_compare_regs(struct compare_group *pgroup);

#endif /* __SUNXI_DUMP_REG_H */

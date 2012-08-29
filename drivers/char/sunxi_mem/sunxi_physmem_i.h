/*
 * drivers/char/sunxi_mem/sunxi_physmem_i.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi physical memory allocator head file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SUNXI_PHYSMEM_I_H
#define __SUNXI_PHYSMEM_I_H

#define SUNXIMEM_DBG_FUN_LINE_TODO	printk("%s, line %d, todo############\n", __func__, __LINE__)
#define SUNXIMEM_DBG_FUN_LINE 		printk("%s, line %d\n", __func__, __LINE__)
#define SUNXIMEM_ERR_FUN_LINE 		printk("%s err, line %d\n", __func__, __LINE__)

struct mem_list {
	u32	   	virt_addr;
	u32	   	phys_addr;
	u32	   	size;
	struct mem_list *next;
	struct mem_list *prev;
};

struct sunxi_mem_allocator {
	u32 (*init)(struct sunxi_mem_allocator *this, u32 size, u32 va, u32 pa);
	void (*deinit)(struct sunxi_mem_allocator *this);

	bool (*allocate)(struct sunxi_mem_allocator *this, const u32 size_to_alloc,
		u32* const pvirt_adr, u32* const pphy_adr);
	void (*free)(struct sunxi_mem_allocator *this, const u32 virtAddr, const u32 physAddr);

	bool 		(*add_node_to_freelist)(struct sunxi_mem_allocator *this, struct mem_list * pNode);
	bool 		(*delete_node)(struct sunxi_mem_allocator *this, struct mem_list * pNode);
	struct mem_list * (*create_new_node)(struct sunxi_mem_allocator *this, u32 size,
					u32 virt_addr, u32 phys_addr);
	struct mem_list * (*find_free_block)(struct sunxi_mem_allocator *this, u32 size);
	bool 		(*free_list)(struct sunxi_mem_allocator *this, struct mem_list * *ppHead);

	u32	   	normal_size;
	struct mem_list *node_free_listh;
	struct mem_list *free_listh;
	struct mem_list *inuse_listh;
};

#endif /* __SUNXI_PHYSMEM_I_H */
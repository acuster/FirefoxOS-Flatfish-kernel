/*
 * drivers/gpu/ion/ion_carveout_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/spinlock.h>

#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "ion_priv.h"

#include <asm/mach/map.h>

//#define DEBUG_HEAP

#ifdef DEBUG_HEAP
u32 total_alloc = 0;
u32 alloc_cnt = 0, free_cnt = 0;
u32 max_single_len = 0, max_total_alloc = 0;
static DEFINE_RWLOCK(heap_lock);
#endif

struct ion_carveout_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	ion_phys_addr_t base;
};

ion_phys_addr_t ion_carveout_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);
	unsigned long offset = gen_pool_alloc(carveout_heap->pool, size);

#ifdef DEBUG_HEAP
	u32 tmp_a_c, tmp_f_c, tmp_t_a, max_s_l, max_t_a;
	unsigned long flags;
#endif

	if (!offset) {
#ifdef DEBUG_HEAP
		read_lock_irqsave(&heap_lock, flags);
		tmp_a_c = alloc_cnt;
		tmp_f_c = free_cnt;
		tmp_t_a = total_alloc;
		read_unlock_irqrestore(&heap_lock, flags);
		printk("%s(%d) err: size 0x%08x, align 0x%08x. alloc_cnt %d, free_cnt %d, total_alloc 0x%08x\n",
			__func__, __LINE__, (int)size, (int)align, tmp_a_c, tmp_f_c, tmp_t_a);
#endif
		return ION_CARVEOUT_ALLOCATE_FAIL;
	}
#ifdef DEBUG_HEAP
	else {
		write_lock_irqsave(&heap_lock, flags);
		alloc_cnt++;
		total_alloc += size;
		if(max_single_len < size)
			max_single_len = size;
		if(max_total_alloc < total_alloc)
			max_total_alloc = total_alloc;
		tmp_a_c = alloc_cnt;
		tmp_f_c = free_cnt;
		tmp_t_a = total_alloc;
		max_s_l = max_single_len;
		max_t_a = max_total_alloc;
		write_unlock_irqrestore(&heap_lock, flags);
		printk("%s(%d) success: size 0x%08x, align 0x%08x, ret 0x%08x, alloc_cnt %d, free_cnt %d,"
			"max_single_len 0x%08x, total_alloc 0x%08x, max_total_alloc 0x%08x\n", __func__, __LINE__,
			(int)size, (int)align, (int)offset, tmp_a_c, tmp_f_c, max_s_l, tmp_t_a, max_t_a);
	}
#endif
	return offset;
}

void ion_carveout_free(struct ion_heap *heap, ion_phys_addr_t addr, unsigned long size)
{
	struct ion_carveout_heap *carveout_heap =
		container_of(heap, struct ion_carveout_heap, heap);

#ifdef DEBUG_HEAP
	u32 tmp_a_c, tmp_f_c, tmp_t_a, max_s_l, max_t_a;
	unsigned long flags;
#endif

	if (addr == ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(carveout_heap->pool, addr, size);
#ifdef DEBUG_HEAP
	write_lock_irqsave(&heap_lock, flags);
	free_cnt++;
	total_alloc -= size;
	tmp_a_c = alloc_cnt;
	tmp_f_c = free_cnt;
	tmp_t_a = total_alloc;
	max_s_l = max_single_len;
	max_t_a = max_total_alloc;
	write_unlock_irqrestore(&heap_lock, flags);
	printk("%s(%d): addr 0x%08x, size 0x%08x, alloc_cnt %d, free_cnt %d, max_single_len 0x%08x,"
		"total_alloc 0x%08x, max_total_alloc 0x%08x\n", __func__, __LINE__,
		(int)addr, (int)size, tmp_a_c, tmp_f_c, max_s_l, tmp_t_a, max_t_a);
#endif
}

static int ion_carveout_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	*addr = buffer->priv_phys;
	*len = buffer->size;
	return 0;
}

static int ion_carveout_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	buffer->priv_phys = ion_carveout_allocate(heap, size, align);
	return buffer->priv_phys == ION_CARVEOUT_ALLOCATE_FAIL ? -ENOMEM : 0;
}

static void ion_carveout_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;

	ion_carveout_free(heap, buffer->priv_phys, buffer->size);
	buffer->priv_phys = ION_CARVEOUT_ALLOCATE_FAIL;
}

struct sg_table *ion_carveout_heap_map_dma(struct ion_heap *heap,
					      struct ion_buffer *buffer)
{
	struct sg_table *table;
	int ret;

	table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return ERR_PTR(-ENOMEM);
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret) {
		kfree(table);
		return ERR_PTR(ret);
	}
	sg_set_page(table->sgl, phys_to_page(buffer->priv_phys), buffer->size,
		    0);
	return table;
}

void ion_carveout_heap_unmap_dma(struct ion_heap *heap,
				 struct ion_buffer *buffer)
{
	sg_free_table(buffer->sg_table);
	/* liugang add */
	kfree(buffer->sg_table);
	buffer->sg_table = NULL;
}

void *ion_carveout_heap_map_kernel(struct ion_heap *heap,
				   struct ion_buffer *buffer)
{
	int mtype = MT_MEMORY_NONCACHED;

	if (buffer->flags & ION_FLAG_CACHED)
		mtype = MT_MEMORY;

	return __arm_ioremap(buffer->priv_phys, buffer->size,
			      mtype);
}

void ion_carveout_heap_unmap_kernel(struct ion_heap *heap,
				    struct ion_buffer *buffer)
{
	__iounmap(buffer->vaddr);
	buffer->vaddr = NULL;
	return;
}

int ion_carveout_heap_map_user(struct ion_heap *heap, struct ion_buffer *buffer,
			       struct vm_area_struct *vma)
{	
	return remap_pfn_range(vma, vma->vm_start,
			       __phys_to_pfn(buffer->priv_phys) + vma->vm_pgoff,
			       vma->vm_end - vma->vm_start,
			       __pgprot_modify(vma->vm_page_prot, L_PTE_MT_MASK, L_PTE_MT_BUFFERABLE));
			       //pgprot_noncached(vma->vm_page_prot));

}

static struct ion_heap_ops carveout_heap_ops = {
	.allocate = ion_carveout_heap_allocate,
	.free = ion_carveout_heap_free,
	.phys = ion_carveout_heap_phys,
	.map_dma = ion_carveout_heap_map_dma,
	.unmap_dma = ion_carveout_heap_unmap_dma,
	.map_user = ion_carveout_heap_map_user,
	.map_kernel = ion_carveout_heap_map_kernel,
	.unmap_kernel = ion_carveout_heap_unmap_kernel,
};

#include <asm/setup.h>
extern struct tag_mem32 ion_mem;
struct ion_heap *ion_carveout_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_carveout_heap *carveout_heap;

	carveout_heap = kzalloc(sizeof(struct ion_carveout_heap), GFP_KERNEL);
	if (!carveout_heap)
		return ERR_PTR(-ENOMEM);

	carveout_heap->pool = gen_pool_create(12, -1);
	if (!carveout_heap->pool) {
		kfree(carveout_heap);
		return ERR_PTR(-ENOMEM);
	}
	//carveout_heap->base = heap_data->base;
	//gen_pool_add(carveout_heap->pool, carveout_heap->base, heap_data->size, -1);
	carveout_heap->base = ion_mem.start;
	gen_pool_add(carveout_heap->pool, carveout_heap->base, ion_mem.size, -1);
	carveout_heap->heap.ops = &carveout_heap_ops;
	carveout_heap->heap.type = ION_HEAP_TYPE_CARVEOUT;

	return &carveout_heap->heap;
}

void ion_carveout_heap_destroy(struct ion_heap *heap)
{
	struct ion_carveout_heap *carveout_heap =
	     container_of(heap, struct  ion_carveout_heap, heap);

	gen_pool_destroy(carveout_heap->pool);
	kfree(carveout_heap);
	carveout_heap = NULL;
}

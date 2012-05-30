/*
 * arch/arm/mach-sun4i/dma/dma_common.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * SUN4I dma common header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_COMMON_H
#define __DMA_COMMON_H

#define DBG_DMA /* debug dma driver */

#include <linux/spinlock.h>

#define FROM_SD_TESTCODE	/* refrence the 1633 dma test code from sd */

/*
 * test mem leak, trace kmem_cache_alloc/kmem_cache_free
 */
//#define DMA_TRACE_KALLOC_FREE

/*
 * dma print macro
 */
#define DMA_DBG_LEVEL	1

#if (DMA_DBG_LEVEL == 1)
	#define DMA_DBG(format,args...)   printk("[dma-dbg] "format,##args)
	#define DMA_INF(format,args...)   printk("[dma-inf] "format,##args)
	#define DMA_ERR(format,args...)   printk("[dma-err] "format,##args)
#elif (DMA_DBG_LEVEL == 2)
	#define DMA_DBG(format,args...)
	#define DMA_INF(format,args...)   printk("[dma-inf] "format,##args)
	#define DMA_ERR(format,args...)   printk("[dma-err] "format,##args)
#elif (DMA_DBG_LEVEL == 3)
	#define DMA_DBG(format,args...)
	#define DMA_INF(format,args...)
	#define DMA_ERR(format,args...)   printk("[dma-err] "format,##args)
#endif

#define DMA_DBG_FUN_LINE   		DMA_DBG("%s(%d)\n", __FUNCTION__, __LINE__)
#define DMA_ERR_FUN_LINE   		DMA_ERR("%s(%d)\n", __FUNCTION__, __LINE__)
#define DMA_DBG_FUN_LINE_TODO  		DMA_DBG("%s(%d) - todo########\n", __FUNCTION__, __LINE__)
#define DMA_DBG_FUN_LINE_TOCHECK 	DMA_DBG("%s(%d) - tocheck########\n", __FUNCTION__, __LINE__)

/*
 * dma channel total
 */
#define DMA_CHAN_TOTAL	(16)

/*
 * dma channel owner name max len
 */
#define MAX_OWNER_NAME_LEN	32

/*
 * dma end des link
 */
#define DMA_END_DES_LINK	0xFFFFF800

/*
 * XXX
 */
enum dma_chan_sta_e {
	DMA_CHAN_STA_IDLE,  	/* maybe before start or after stop */
	DMA_CHAN_STA_RUNING,	/* transferring */
	DMA_CHAN_STA_DONE_WAIT,	/* running -> hw done, but __dma_chan_handle_qd not already dealed it, so clear the des,
				 * if queueing now, just queued to des end, but not start, which done in __dma_chan_handle_qd
				 * only normal/hd_cb/fd_cb/qd_cb queueing can change state from running to done wait, in
				 * __dma_chan_handle_qd meet done wait:
				 * 	(1) if there are new des(des is not null), start the des, and state -> running
				 * 	(2) if there arenot new des(des is null), state -> done
				 */
	DMA_CHAN_STA_DONE	/* it is the case all des done, hw idle, des queue is empty */
};

/*
 * define dma config descriptor struct for hardware
 */
struct cofig_des_t {
	u32		cofig;     	/* dma configuration reg */
	u32		saddr;     	/* dma src addr reg */
	u32		daddr;     	/* dma dst addr reg */
	u32		bcnt;     	/* dma byte cnt reg */
	u32		param;     	/* dma param reg */
	struct cofig_des_t *pnext;    	/* next descriptor address */
};

/*
 * define dma config/param info, for dma_channel_t.des_info_save
 */
struct des_save_info_t {
	u32		cofig;     	/* dma configuration reg */
	u32		param;     	/* dma param reg */
};

/*
 * define dma config descriptor manager
 */
struct des_mgr_t {
	struct cofig_des_t	*pdes;     	/* pointer to descriptor struct */
	u32			des_num;     	/* the vaild des num in pdes[], max is MAX_DES_ITEM_NUM. can be used to:
						 * (1) get the last des, bkup it's des_save_info_t for next queueing
						 * (2) when queue new buffer, queued to pdes[des_num], des_num++
						 */
	struct des_mgr_t 	*pnext;    	/* next des mgr in chain */
};

/*
 * max dma descriptor item num for one des_mgr_t.pdes
 */
#define MAX_DES_ITEM_NUM	256

/*
 * define dma channel struct
 */
struct dma_channel_t {
	u32		used;     	/* 1 used, 0 unuse */
	u32		id;     	/* channel id, 0~15 */
	char 		owner[MAX_OWNER_NAME_LEN];	/* dma chnnnel owner name */
	s32		hash;   	/* owner hash value, for fast search without string compare */
	u32		reg_base;	/* regs base addr */

	/* channel irq supprot type, used for irq handler
	 * only enabled then can call irq callback function
	 */
	u32 		irq_spt;

	struct dma_cb_t		hd_cb;		/* half done call back func */
	struct dma_cb_t		fd_cb;		/* full done call back func */
	struct dma_cb_t		qd_cb;		/* queue done call back func */
	struct dma_op_cb_t	op_cb;		/* dma operation call back func */

	struct des_mgr_t	*pdes_mgr;	/* dma config descriptor manager */
	struct des_save_info_t	des_info_save;	/* save the last descriptor for enqueue, when enqueue with no
						 * no config/para:
						 * (1) state is running or idle, use the above des's config/para
						 * (2) state is done, des are all cleared, use des_info_save, so
						 *	we should bkup des_info_save when done.
						 */

	enum dma_chan_sta_e 	state;	/* XXXX */

	spinlock_t 	lock;		/* XXXX */

#ifdef DMA_TRACE_KALLOC_FREE
	atomic_t 	alloc_cnt;
	atomic_t 	free_cnt;
#endif /* DMA_TRACE_KALLOC_FREE */
};

/*
 * dma manager struct
 */
struct dma_mgr_t {
	struct dma_channel_t chnl[DMA_CHAN_TOTAL];
};


#endif  /* __DMA_COMMON_H */

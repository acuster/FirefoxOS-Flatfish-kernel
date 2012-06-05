/*
 * arch/arm/mach-aw163x/dma/dma_common.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma common header file
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
 * dma print macro
 */
#define DMA_DBG_LEVEL		3

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
	DMA_CHAN_STA_WAIT_QD,	/* running -> last buffer has been retrived(the start addr reg is 0xfffff800), now when new buf
				 * queueing, we cannot pause->queue->resume to run the new buffer, must restart dma. since
				 * __dma_chan_handle_qd not already dealed, so we donnot restart in queueing func, just clear the des,
				 * queued to des end, the new buffer will be-restarted in __dma_chan_handle_qd.
				 * only normal/hd_cb/fd_cb/qd_cb queueing can change state from running to wait_qd, when
				 * __dma_chan_handle_qd meet wait_qd:
				 * 	(1) if there are new des(des is not null), start the des, and state -> running
				 * 	(2) if there arenot new des(des is null), print err
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
	dma_addr_t		des_pa;		/* bus-specific DMA address for pdes, used by dma_pool_free */
	u32			des_num;     	/* the vaild des num in pdes[], max is MAX_DES_ITEM_NUM. can be used to:
						 * (1) get the last des, bkup it's des_save_info_t for next queueing
						 * (2) when queue new buffer, queued to pdes[des_num], des_num++
						 */
	struct des_mgr_t 	*pnext;    	/* next des mgr in chain */
};

/*
 * max dma descriptor item num for one des_mgr_t.pdes
 */
#define MAX_DES_ITEM_NUM	64

/*
 * descriptor area length
 */
#define DES_AREA_LEN 		(MAX_DES_ITEM_NUM * sizeof(struct cofig_des_t))

/*
 * define dma channel struct
 */
struct dma_channel_t {
	u32		used;     	/* 1 used, 0 unuse */
	u32		id;     	/* channel id, 0~15 */
	char 		owner[MAX_OWNER_NAME_LEN];	/* dma chnnnel owner name */
	u32		reg_base;	/* regs base addr */
	u32		bconti_mode;	/* cotinue mode */

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
	enum dma_chan_sta_e 	state;		/* XXXX */
	spinlock_t 		lock;		/* XXXX */
};

/*
 * dma manager struct
 */
struct dma_mgr_t {
	struct dma_channel_t chnl[DMA_CHAN_TOTAL];
};

/*
 * dma channel lock
 */
/* init lock */
#define DMA_CHAN_LOCK_INIT(lock)	spin_lock_init((lock))
#define DMA_CHAN_LOCK_DEINIT(lock)	do{}while(0)

/* lock in process contex */
#define DMA_CHAN_LOCK(lock, flag)	spin_lock_irqsave((lock), (flag))
#define DMA_CHAN_UNLOCK(lock, flag)	spin_unlock_irqrestore((lock), (flag))

/* lock in irq contex */
#define DMA_CHAN_LOCK_IN_IRQHD(lock)	spin_lock(lock)
#define DMA_CHAN_UNLOCK_IN_IRQHD(lock)	spin_unlock(lock)

#endif  /* __DMA_COMMON_H */

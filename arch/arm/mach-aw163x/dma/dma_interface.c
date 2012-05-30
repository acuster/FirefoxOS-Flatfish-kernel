/*
 * arch/arm/mach-sun4i/dma/dma_interface.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * SUN4I dma interface function
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
//#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
//#include <mach/system.h>
#include <mach/platform.h>

#include <linux/slab.h>
#include <linux/spinlock.h>

#include <mach/dma.h>
#include "dma_regs.h"
#include "dma_common.h"
#include "dma_csp.h"
#include "dma_interface.h"

#ifdef DBG_DMA
#include <linux/delay.h>
#endif /* DBG_DMA */

/*
 * temp, check if we alloc free are equal, to del
 */
//#define TEMP_TRANCE_ALLOC_FREE
#ifdef TEMP_TRANCE_ALLOC_FREE
static atomic_t g_acnt = ATOMIC_INIT(0);
static atomic_t g_fcnt = ATOMIC_INIT(0);
#endif /* TEMP_TRANCE_ALLOC_FREE */

/*
 * dma channel lock
 */
#define DMA_CHAN_LOCK_INIT(lock)	spin_lock_init((lock))
#define DMA_CHAN_LOCK_DEINIT(lock)
#define DMA_CHAN_LOCK(lock, flag)	spin_lock_irqsave((lock), (flag))
#define DMA_CHAN_UNLOCK(lock, flag)	spin_unlock_irqrestore((lock), (flag))

/*
 * test mem leak, trace kmem_cache_alloc/kmem_cache_free
 */
#ifdef DMA_TRACE_KALLOC_FREE
#define DMA_ALLOC(hchan, kcache, flag)	dma_alloc_trace((hchan), (kcache), (flag))
#define DMA_FREE(hchan, kcache, objp)	dma_free_trace((hchan), (kcache), (objp))
#else
#define DMA_ALLOC(hchan, kcache, flag)	kmem_cache_alloc((kcache), (flag))
#define DMA_FREE(hchan, kcache, objp)	kmem_cache_free((kcache), (objp))
#endif /* DMA_TRACE_KALLOC_FREE */

/*
 * dma manager
 */
void __iomem *g_dma_reg_vbase = 0;

/*
 * dma manager
 */
static struct dma_mgr_t g_dma_mgr; /* compile warining if "g_dma_mgr = {0}" */

/*
 * XXX
 */
static struct kmem_cache *g_pdma_des_mgr = NULL;

/*
 * XXX
 */
static struct kmem_cache *g_pdma_des;

/*
 * data length and burst length value in config reg
 */
unsigned long xfer_arr[DMAXFER_MAX] =
{
	/* des:X_SIGLE  src:X_SIGLE */
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_WORD << 9),

	/* des:X_SIGLE   src:X_BURST */
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_WORD << 9),

	/* des:X_SIGLE   src:X_TIPPL */
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_SIGLE << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_WORD << 9),

	/* des:X_BURST  src:X_BURST */
	(X_BURST << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_WORD << 9),

	/* des:X_BURST   src:X_SIGLE */
	(X_BURST << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_WORD << 9),

	/* des:X_BURST   src:X_TIPPL */
	(X_BURST << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_BURST << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_WORD << 9),

	/* des:X_TIPPL   src:X_TIPPL */
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_TIPPL <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_TIPPL <<7) | (X_WORD << 9),

	/* des:X_TIPPL   src:X_SIGLE */
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_SIGLE <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_SIGLE <<7) | (X_WORD << 9),

	/* des:X_TIPPL   src:X_BURST */
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_BYTE << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_HALF << 25) | (X_BURST <<7) | (X_WORD << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_BYTE << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_HALF << 9),
	(X_TIPPL << 23) | (X_WORD << 25) | (X_BURST <<7) | (X_WORD << 9),
};

/*
 * src/dst address type value in config reg
 */
unsigned long addrtype_arr[DMAADDRT_MAX] =
{
	(A_LN  << 21) | (A_LN  << 5),
	(A_LN  << 21) | (A_IO  << 5),
	(A_IO  << 21) | (A_LN  << 5),
	(A_IO  << 21) | (A_IO  << 5),
};

#ifdef DMA_TRACE_KALLOC_FREE
/**
 * dma_alloc_trace - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
void *dma_alloc_trace(dm_hdl_t dma_hdl, struct kmem_cache *s, gfp_t gfpflags)
{
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	atomic_add(1, &pdma_chan->alloc_cnt);
	return kmem_cache_alloc(s, gfpflags);
}

/**
 * dma_alloc_trace - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
void dma_free_trace(dm_hdl_t dma_hdl, struct kmem_cache *cachep, void *objp)
{
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	atomic_add(1, &pdma_chan->free_cnt);
	kmem_cache_free(cachep, objp);
}

/**
 * __dma_trace_init_cnt - XXXXXX
 *
 * XXXXXX
 */
void __dma_trace_init_cnt(dm_hdl_t dma_hdl)
{
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	atomic_set(&pdma_chan->alloc_cnt, 0);
	atomic_set(&pdma_chan->free_cnt, 0);
}

/**
 * __dma_trace_init_cnt - XXXXXX
 *
 * XXXXXX
 */
void __dma_trace_dump_result(dm_hdl_t dma_hdl)
{
	u32 	alloc_cnt, free_cnt;
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	alloc_cnt = atomic_read(&pdma_chan->alloc_cnt);
	free_cnt = atomic_read(&pdma_chan->free_cnt);

	DMA_INF("%s-%s: chan id %d, alloc cnt %d, free cnt %d\n", __FUNCTION__, \
		(alloc_cnt == free_cnt ? "ok" : "err"), pdma_chan->id, alloc_cnt, free_cnt);
}
#endif /* DMA_TRACE_KALLOC_FREE */

/**
 * __des_mgr_cache_ctor - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
static void __des_mgr_cache_ctor(void *p)
{
	memset(p, 0, sizeof(struct des_mgr_t));
}

/**
 * __des_cache_ctor - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
static void __des_cache_ctor(void *p)
{
	memset(p, 0, MAX_DES_ITEM_NUM * sizeof(struct cofig_des_t));
}

/**
 * __dma_calc_hash - calculate hash value of a string
 * @string:	string whose hash value need be calculate
 *
 * Returns hash value of the string.
 */
static inline s32 __dma_calc_hash(char *string)
{
    s32   tmpLen, i, tmpHash = 0;

    if(!string) {
        return 0;
    }

    tmpLen = strlen(string);
    for(i = 0; i < tmpLen; i++) {
        tmpHash += string[i];
    }

    return tmpHash;
}

/**
 * __dma_dump_config_para - dump dma_config_t struct
 * @para:	dma_config_t struct to dump
 */
static void __dma_dump_config_para(struct dma_config_t *para)
{
	if(NULL == para) {
		DMA_ERR_FUN_LINE;
		return;
	}

	DMA_DBG("+++++++++++%s+++++++++++\n", __FUNCTION__);
	DMA_DBG("  src_drq_type:      %d\n", para->src_drq_type);
	DMA_DBG("  dst_drq_type:      %d\n", para->dst_drq_type);
	DMA_DBG("  xfer_type:         %d\n", para->xfer_type);
	DMA_DBG("  address_type:      %d\n", para->address_type);
	DMA_DBG("  irq_spt:           %d\n", para->irq_spt);
	DMA_DBG("  dcmbk:             0x%08x\n", para->para);
	DMA_DBG("-----------%s-----------\n", __FUNCTION__);
}

/**
 * __dma_dump_channel - dump dma channel info
 * @dma_hdl:	dma handle
 */
void __dma_dump_channel(struct dma_channel_t *pchan)
{
	if(NULL == pchan) {
		DMA_ERR_FUN_LINE;
		return;
	}

	DMA_DBG("+++++++++++%s+++++++++++\n", __FUNCTION__);
	DMA_DBG("  channel id:        %d\n", pchan->id);
	DMA_DBG("  channel used:      %d\n", pchan->used);
	DMA_DBG("  channel owner:     %s\n", pchan->owner);
	DMA_DBG("  channel hash:      %d\n", pchan->hash);
	DMA_DBG("  channel irq_spt:   0x%08x\n", (u32)pchan->irq_spt);
	DMA_DBG("  channel reg_base:  0x%08x\n", pchan->reg_base);
	DMA_DBG("          EN REG:             0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_EN));
	DMA_DBG("          PAUSE REG:          0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_PAUSE));
	DMA_DBG("          START REG:          0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_START));
	DMA_DBG("          CONFIG REG:         0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CFG));
	DMA_DBG("          CUR SRC REG:        0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_SRC));
	DMA_DBG("          CUR DST REG:        0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_DST));
	DMA_DBG("          BYTE CNT LEFT REG:  0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_BCNT_LEFT));
	DMA_DBG("          PARA REG:           0x%08x\n", DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_PARA));
	DMA_DBG("  channel hd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->hd_cb.func, (u32)pchan->hd_cb.parg);
	DMA_DBG("  channel fd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->fd_cb.func, (u32)pchan->fd_cb.parg);
	DMA_DBG("  channel qd_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->qd_cb.func, (u32)pchan->qd_cb.parg);
	DMA_DBG("  channel op_cb:     (func: 0x%08x, parg: 0x%08x)\n", (u32)pchan->op_cb.func, (u32)pchan->op_cb.parg);
	DMA_DBG("  channel pdes_mgr:  0x%08x\n", (u32)pchan->pdes_mgr);
	{
		struct des_mgr_t *pdes_mgr = pchan->pdes_mgr;
		while(NULL != pdes_mgr) {
			DMA_DBG("     pdes:        0x%08x\n", (u32)pdes_mgr->pdes);
			DMA_DBG("     des_num:     0x%08x\n", pdes_mgr->des_num);
			DMA_DBG("     pnext:       0x%08x\n", (u32)pdes_mgr->pnext);
			DMA_DBG("\n");
			pdes_mgr = pdes_mgr->pnext;
		}
	}
	DMA_DBG("  channel des_info_save:  (cofig: 0x%08x, param: 0x%08x)\n", pchan->des_info_save.cofig, \
		pchan->des_info_save.param);
	DMA_DBG("  channel state:     0x%08x\n", (u32)pchan->state);
	DMA_DBG("-----------%s-----------\n", __FUNCTION__);
}

/**
 * __dma_check_handle - XXX
 * @dma_hdl:	dma handle
 */
static u32 __dma_check_handle(dm_hdl_t dma_hdl)
{
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	if(NULL == pdma_chan) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
	if(0 == pdma_chan->used) {	/* already released? */
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	return 0;
}

/**
 * __dma_check_channel_free - check if channel is free
 * @dma_hdl:	dma handle
 *
 * return true if channel is free, false if not
 *
 * NOTE: can only be called in sw_dma_request recently, becase
 * should be locked
 */
static u32 __dma_check_channel_free(struct dma_channel_t *pchan)
{
	if(0 == pchan->used
		&& 0 == pchan->owner[0]
		&& 0 == pchan->hash
		&& 0 == pchan->reg_base
		//&& CHAN_IRQ_NO == pchan->irq_spt /* maybe not use dma irq? */
		&& NULL == pchan->hd_cb.func
		&& NULL == pchan->fd_cb.func
		&& NULL == pchan->qd_cb.func
		&& NULL == pchan->op_cb.func
		&& NULL == pchan->pdes_mgr
		&& DMA_CHAN_STA_IDLE == pchan->state
		) {
		return true;
	} else {
		__dma_dump_channel(pchan);
		return false;
	}

	return false;
}

/**
 * __dma_dump_buf_chain - XXX
 * @pchan:	dma handle
 */
void __dma_dump_buf_chain(struct dma_channel_t *pchan)
{
	u32 	i = 0;
	struct des_mgr_t *pdes_mgr = pchan->pdes_mgr;

	DMA_DBG("+++++++++++%s+++++++++++\n", __FUNCTION__);

	while(NULL != pdes_mgr) {
		for(i = 0; i < pdes_mgr->des_num; i++) {
			DMA_DBG("  cofig/saddr/daddr/bcnt/param/pnext:  0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n", \
				pdes_mgr->pdes[i].cofig, pdes_mgr->pdes[i].saddr, pdes_mgr->pdes[i].daddr, \
				pdes_mgr->pdes[i].bcnt, pdes_mgr->pdes[i].param, (u32)pdes_mgr->pdes[i].pnext);
		}
		DMA_DBG("\n");
		pdes_mgr = pdes_mgr->pnext;
	}

	DMA_DBG("-----------%s-----------\n", __FUNCTION__);
}

/**
 * __dma_chan_init_main_des - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_init_main_des(struct dma_channel_t *pchan)
{
	if(NULL == pchan || NULL == pchan->pdes_mgr) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	/* clear main des */
	memset(pchan->pdes_mgr->pdes, 0, MAX_DES_ITEM_NUM * sizeof(struct cofig_des_t));

	/* init main des_mgr */
	pchan->pdes_mgr->des_num = 0;
	pchan->pdes_mgr->pnext = NULL;
	return 0;
}

/**
 * dma_chan_des_mgr_init - init the channel's descriptor manager
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 dma_chan_des_mgr_init(struct dma_channel_t *pchan)
{
	u32		uRet = 0;
	struct cofig_des_t	*pdes = NULL;
	struct des_mgr_t	*pdes_mgr = NULL;

	//use dma_alloc_coherent?
	DMA_DBG_FUN_LINE_TOCHECK;

	/* XXX */
	pdes = kmem_cache_alloc(g_pdma_des, GFP_ATOMIC);
	if (NULL == pdes) {
		uRet = __LINE__;
		goto End;
	}

	/* XXX */
	pdes_mgr = kmem_cache_alloc(g_pdma_des_mgr, GFP_ATOMIC);
	if (NULL == pdes_mgr) {
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
		if (NULL != pdes) {
			kmem_cache_free(g_pdma_des, pdes);
			pdes = NULL;
		}
		if (NULL != pdes_mgr) {
			kmem_cache_free(g_pdma_des_mgr, pdes_mgr);
			pdes_mgr = NULL;
		}
	} else {
		pchan->pdes_mgr = pdes_mgr;
		pchan->pdes_mgr->pdes = pdes;
	}

	return uRet;
}

/**
 * dma_save_and_clean_des - bkup the last config/para, free extra des/des_mgr if there is, init main des/des_mgr
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 dma_save_and_clean_des(struct dma_channel_t *pchan)
{
	u32	uRet = 0;
	struct des_mgr_t *pcur = NULL;
	struct des_mgr_t *pnext = NULL;

	/* para check */
	if(NULL == pchan->pdes_mgr || NULL == pchan->pdes_mgr->pdes
		|| 0 == pchan->pdes_mgr->des_num) {
		uRet = __LINE__;
		goto End;
	}

	if(NULL == pchan->pdes_mgr->pnext) { /* there are only one des mgr in chain */
		DMA_DBG_FUN_LINE;
		/* bkup the last config/para */
		pchan->des_info_save.cofig = pchan->pdes_mgr->pdes[pchan->pdes_mgr->des_num - 1].cofig;
		pchan->des_info_save.param = pchan->pdes_mgr->pdes[pchan->pdes_mgr->des_num - 1].param;

		/* init main des/des_mgr */
		__dma_chan_init_main_des(pchan);
		return 0;
	} else {
		pcur = pchan->pdes_mgr->pnext;
		pnext = pcur->pnext;

		/* free extra des/desmgr */
		do {
			if(NULL == pnext) {
				DMA_DBG_FUN_LINE;
				/* bkup the last config/para */
				pchan->des_info_save.cofig = pcur->pdes[pcur->des_num - 1].cofig;
				pchan->des_info_save.param = pcur->pdes[pcur->des_num - 1].param;

				/* free pcur */
				kmem_cache_free(g_pdma_des, pcur->pdes);
				kmem_cache_free(g_pdma_des_mgr, pcur);
				break;
			} else {
				/* free pcur */
				kmem_cache_free(g_pdma_des, pcur->pdes);
				kmem_cache_free(g_pdma_des_mgr, pcur);

				pcur = pnext;
				pnext = pcur->pnext;
			}
		}while(1);

		/* init main des/des_mgr */
		__dma_chan_init_main_des(pchan);
	}

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
	}

	return uRet;
}

/**
 * dma_chan_des_mgr_deinit - deinit the channel's descriptor manager
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 dma_chan_des_mgr_deinit(struct dma_channel_t *pchan)
{
	u32	uRet = 0;

	//use dma_alloc_coherent?
	DMA_DBG_FUN_LINE_TOCHECK;

	if (NULL != pchan->pdes_mgr->pdes) {
		kmem_cache_free(g_pdma_des, pchan->pdes_mgr->pdes);
		pchan->pdes_mgr->pdes = NULL;
	}

	if (NULL != pchan->pdes_mgr) {
		kmem_cache_free(g_pdma_des_mgr, pchan->pdes_mgr);
		pchan->pdes_mgr = NULL;
	}

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
	}

	return uRet;
}

/**
 * sw_dma_request - request a dma channel
 * @name:	dma channel name
 *
 * Returns handle to the channel if success, NULL if failed.
 */
dm_hdl_t sw_dma_request(char * name)
{
	u32		i = 0;
	s32 		shash = __dma_calc_hash(name);
	unsigned long	flags = 0;
	struct dma_channel_t	*pchan = NULL;

	DMA_DBG("%s: name %s\n", __FUNCTION__, name);

	/* para check */
	if(strlen(name) >= MAX_OWNER_NAME_LEN) {
		DMA_ERR("%s err: name %s exceed MAX_OWNER_NAME_LEN(32)\n", __FUNCTION__, name);
		return NULL;
	}

	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &g_dma_mgr.chnl[i];

		DMA_CHAN_LOCK(&pchan->lock, flags);

		/* check if already exist */
		if(1 == pchan->used
			&& shash == pchan->hash
			&& !strcmp(pchan->owner, name)) {

			DMA_CHAN_UNLOCK(&pchan->lock, flags);

			DMA_ERR("%s err, channel \"%s\" already exists\n", __FUNCTION__, name);
			return (dm_hdl_t)NULL;
		}

		/* get a free channel */
		if(0 == pchan->used) {
#ifdef DBG_DMA
			/* check if dma channel free */
			if(true != __dma_check_channel_free(pchan)) {
				DMA_ERR_FUN_LINE;
			}
#endif /* DBG_DMA */

			/* init channel */
			pchan->used = 1;
			strcpy(pchan->owner, name);
			pchan->hash = shash;

			if(0 != dma_chan_des_mgr_init(pchan)) {
				DMA_ERR_FUN_LINE;
			}

			DMA_CHAN_UNLOCK(&pchan->lock, flags);

			DMA_DBG("%s: success, channel id %d\n", __FUNCTION__, i);
			return (dm_hdl_t)pchan;
		}

		DMA_CHAN_UNLOCK(&pchan->lock, flags);
	}

	DMA_ERR("%s err: can not find a free channel!\n", __FUNCTION__);
	return (dm_hdl_t)NULL;
}
EXPORT_SYMBOL(sw_dma_request);

/**
 * sw_dma_release - free a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, other value if failed.
 */
u32 sw_dma_release(dm_hdl_t dma_hdl)
{
	unsigned long 	flags = 0;
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	DMA_CHAN_LOCK(&pdma_chan->lock, flags);

#ifdef DBG_DMA
	if(0 != __dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pdma_chan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	/* must in idle state */
	if(DMA_CHAN_STA_IDLE != pdma_chan->state) {
		DMA_CHAN_UNLOCK(&pdma_chan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	//memset(pdma_chan, 0, sizeof(*pdma_chan)); /* donot do that, because id...shouldnot be cleared */
	pdma_chan->used 	= 0;
	pdma_chan->hash 	= 0;
	memset(pdma_chan->owner, 0, sizeof(pdma_chan->owner));

	pdma_chan->irq_spt 	= CHAN_IRQ_NO;
	memset(&pdma_chan->des_info_save, 0, sizeof(pdma_chan->des_info_save));

	memset(&pdma_chan->op_cb, 0, sizeof(pdma_chan->op_cb));
	memset(&pdma_chan->hd_cb, 0, sizeof(pdma_chan->hd_cb));
	memset(&pdma_chan->fd_cb, 0, sizeof(pdma_chan->fd_cb));
	memset(&pdma_chan->qd_cb, 0, sizeof(pdma_chan->qd_cb));

	if(0 != dma_chan_des_mgr_deinit(pdma_chan))
		DMA_ERR_FUN_LINE;

	DMA_CHAN_UNLOCK(&pdma_chan->lock, flags);
	return 0;
}
EXPORT_SYMBOL(sw_dma_release);

/**
 * dma_start - start the dma des queue, change state to running
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 *
 * start op can in these state:
 * idle: 	the first start, or after stop. make sure queue is valid.
 * done wait: 	hw already done in hd/fd/qd cb queueing or nomal queueing, and des is new.
 *  		  so let __dma_chan_handle_qd start the new queue(from begin of des)
 * done: 	__dma_chan_handle_qd has done the queue, and no new buffer queued, handle_qd
 * 		 will clear des, change state to done, if sw_dma_enqueue(normal) queued new buffer after
 *		 that, it will start the buffer.
 */
u32 dma_start(dm_hdl_t dma_hdl)
{
	u32	uRet = 0;
	u32 	udes_paddr = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(NULL == pchan || NULL == pchan->pdes_mgr
		|| NULL == pchan->pdes_mgr->pdes) {
		uRet = __LINE__;
		goto End;
	}

	/* virt to phys */
	udes_paddr = virt_to_phys((void *)pchan->pdes_mgr->pdes);
	DMA_WRITE_REG(pchan->pdes_mgr->pdes, pchan->reg_base + DMA_OFF_REG_START);

	/* start dma */
	csp_dma_chan_start(pchan);

	/* change state to running */
	pchan->state = DMA_CHAN_STA_RUNING;

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __FUNCTION__, uRet, (u32)dma_hdl);
	}

	return uRet;
}

/**
 * dma_pause - pause a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_pause(dm_hdl_t dma_hdl)
{
	/*
	 * do something if needed...
	 */

	csp_dma_chan_pause((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_resume - resume a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_resume(dm_hdl_t dma_hdl)
{
	/*
	 * do something if needed...
	 */

	csp_dma_chan_resume((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_stop - stop a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_stop(dm_hdl_t dma_hdl)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

//#ifdef DBG_DMA
#if 0	/* maybe enqueued but NOT start, so we can free the enqueued buf in stop op */
	if(DMA_CHAN_STA_IDLE == pdma_chan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

#ifdef DBG_DMA
	DMA_INF("%s: state %d, buf chain: \n", __FUNCTION__, (u32)pchan->state);
	__dma_dump_buf_chain(pchan);

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_INF("%s: state idle, maybe before start or already stopped\n", __FUNCTION__);
		break;
	case DMA_CHAN_STA_RUNING:
		DMA_INF("%s: state running, so stop the channel, abort the transfer, and free extra des, \
			init main des\n", __FUNCTION__);
		break;
	case DMA_CHAN_STA_DONE:
		DMA_INF("%s: state done, just stop the channel\n", __FUNCTION__);
		break;
	case DMA_CHAN_STA_DONE_WAIT:
		DMA_INF("%s: state done wait, so stop the channel, abort the transfer, and free extra des, \
			init main des\n", __FUNCTION__);
		break;
	default:
		DMA_ERR_FUN_LINE;
		break;
	}
#endif /* DBG_DMA */

	/*
	 * stop dma channle and clear irq pending
	 */
	csp_dma_chan_stop(pchan);
	csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD | CHAN_IRQ_FD | CHAN_IRQ_QD);

	/*
	 * abort dma transfer
	 */
	DMA_DBG_FUN_LINE_TOCHECK; /* only in transferring(buffer running) can we abort? or we can abort all the time? */
	if(NULL != pchan->qd_cb.func) {
		if(0 != pchan->qd_cb.func(dma_hdl, pchan->qd_cb.parg, DMA_CB_ABORT)) {
			uret = __LINE__;
			goto End;
		}
	}

	/*
	 * free the extra des(except the main des)
	 */
	if(0 != dma_save_and_clean_des(pchan)) {
		uret = __LINE__;
		goto End;
	}

	/* change channel state to idle */
	pchan->state = DMA_CHAN_STA_IDLE;

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * XXXX - XXXX
 * @XXXX:	XXXX
 *
 * XXXX
 */
u32 dma_get_status(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_status((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * XXXX - XXXX
 * @XXXX:	XXXX
 *
 * XXXX
 */
u32 dma_get_cur_src_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_cur_srcaddr((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * XXXX - XXXX
 * @XXXX:	XXXX
 *
 * XXXX
 */
u32 dma_get_cur_dst_addr(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_cur_dstaddr((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * XXXX - XXXX
 * @XXXX:	XXXX
 *
 * XXXX
 */
u32 dma_get_left_bytecnt(dm_hdl_t dma_hdl, u32 *pval)
{
	/*
	 * do something if needed...
	 */

	*pval = csp_dma_chan_get_left_bytecnt((struct dma_channel_t *)dma_hdl);
	return 0;
}

/**
 * dma_set_op_cb - set dma operation callback function
 * @dma_hdl:	dma handle
 * @pCb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_op_cb(dm_hdl_t dma_hdl, struct dma_op_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->op_cb.func = pcb->func;
	pchan->op_cb.parg = pcb->parg;
	return 0;
}

/**
 * dma_set_hd_cb - set dma half done callback function
 * @dma_hdl:	dma handle
 * @pCb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_hd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->hd_cb.func = pcb->func;
	pchan->hd_cb.parg = pcb->parg;
	return 0;
}

/**
 * dma_set_fd_cb - set dma full done callback function
 * @dma_hdl:	dma handle
 * @pCb:	call back function to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 dma_set_fd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->fd_cb.func = pcb->func;
	pchan->fd_cb.parg = pcb->parg;
	return 0;
}

/**
 * XXXX - XXXX
 * @XXXX:	XXXX
 *
 * XXXX
 */
u32 dma_set_qd_cb(dm_hdl_t dma_hdl, struct dma_cb_t *pcb)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* only in idle state can set callback */
	if(DMA_CHAN_STA_IDLE != pchan->state) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	pchan->qd_cb.func = pcb->func;
	pchan->qd_cb.parg = pcb->parg;
	return 0;
}

/**
 * sw_dma_ctl - start a dma channel
 * @dma_hdl:	dma handle
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_dma_ctl(dm_hdl_t dma_hdl, enum dma_op_type_e op, void *parg)
{
	u32		uRet = 0;
	unsigned long	flags = 0;
	struct dma_channel_t *pdma_chan = (struct dma_channel_t *)dma_hdl;

	/* only in start/stop case can parg be NULL  */
	if((NULL == parg)
		&& (DMA_OP_START != op)
		&& (DMA_OP_PAUSE != op)
		&& (DMA_OP_RESUME != op)
		&& (DMA_OP_STOP != op)) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	DMA_CHAN_LOCK(&pdma_chan->lock, flags);

#ifdef DBG_DMA
	if(0 != __dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pdma_chan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* let the caller to do some operation before op */
	if((DMA_OP_SET_OP_CB != op) && (NULL != pdma_chan->op_cb.func)) {
		if(0 != pdma_chan->op_cb.func(dma_hdl, pdma_chan->op_cb.parg, op))
			DMA_ERR_FUN_LINE;
	}

	switch(op) {
	/*
	 * dma ctrl operation
	 */
	case DMA_OP_START:
		if(0 != dma_start(dma_hdl)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	case DMA_OP_PAUSE:
		if(0 != dma_pause(dma_hdl)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	case DMA_OP_RESUME:
		if(0 != dma_resume(dma_hdl)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	case DMA_OP_STOP:
		if(0 != dma_stop(dma_hdl)) {
			uRet = __LINE__;
			goto End;
		}
#ifdef DMA_TRACE_KALLOC_FREE
		__dma_trace_dump_result(dma_hdl);
#endif /* DMA_TRACE_KALLOC_FREE */
#ifdef TEMP_TRANCE_ALLOC_FREE
		DMA_INF("%s: g_acnt %d, g_fcnt %d\n", __FUNCTION__, atomic_read(&g_acnt), atomic_read(&g_fcnt));
#endif /* TEMP_TRANCE_ALLOC_FREE */
		break;

	/*
	 * dma get status operation
	 */
	case DMA_OP_GET_STATUS:
		dma_get_status(dma_hdl, parg);
		break;
	case DMA_OP_GET_CUR_SRC_ADDR:
		dma_get_cur_src_addr(dma_hdl, parg);
		break;
	case DMA_OP_GET_CUR_DST_ADDR:
		dma_get_cur_dst_addr(dma_hdl, parg);
		break;
	case DMA_OP_GET_BYTECNT_LEFT:
		dma_get_left_bytecnt(dma_hdl, parg);
		break;

	/*
	 * dma set callback operation
	 */
	case DMA_OP_SET_OP_CB:
		if(0 != dma_set_op_cb(dma_hdl, (struct dma_op_cb_t *)parg)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	case DMA_OP_SET_HD_CB:
		if(0 != dma_set_hd_cb(dma_hdl, (struct dma_cb_t *)parg)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	case DMA_OP_SET_FD_CB:
		if(0 != dma_set_fd_cb(dma_hdl, (struct dma_cb_t *)parg)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	case DMA_OP_SET_QD_CB:
		if(0 != dma_set_qd_cb(dma_hdl, (struct dma_cb_t *)parg)) {
			uRet = __LINE__;
			goto End;
		}
		break;
	default:
		uRet = __LINE__;
		goto End;
	}

End:
	DMA_CHAN_UNLOCK(&pdma_chan->lock, flags);
	if(0 != uRet) {
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __FUNCTION__, uRet, (u32)dma_hdl);
	}

	return uRet;
}
EXPORT_SYMBOL(sw_dma_ctl);

/**
 * __hw_transfer_done - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
bool __hw_transfer_done(struct dma_channel_t *pchan)
{
	u32 	utemp = 0;
	bool 	bidle = false;
	bool 	bqdpend = false;

	if(0 == csp_dma_chan_get_status(pchan))
		bidle = true;
	else
		bidle = false;

#ifdef DBG_DMA
	utemp = csp_dma_chan_get_irqpend(pchan);
	if(utemp & CHAN_IRQ_QD)
		bqdpend = true;
	else
		bqdpend = false;
	DMA_DBG("%s: bidle %d, bqdpend %d, channel irq pend %d\n", __FUNCTION__, bidle, bqdpend, utemp);
#endif /* DBG_DMA */

	/*
	 * for hd/fd callback, we can check qd irqpend bit for tranfer done judge,
	 * because it not yet cleared by dma_irq_hdl.
	 */
	DMA_DBG_FUN_LINE_TOCHECK;

	if(true == bidle)
		return true;
	else
		return false;
}

/**
 * __des_chain_is_empty - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
bool __des_chain_is_empty(struct dma_channel_t *pchan)
{
#ifdef DBG_DMA
	if(NULL == pchan->pdes_mgr || NULL == pchan->pdes_mgr->pdes) {
		DMA_ERR_FUN_LINE;
		return true;
	}
#endif /* DBG_DMA */

	if(0 == pchan->pdes_mgr->des_num) {
#ifdef DBG_DMA
		if(NULL != pchan->pdes_mgr->pnext)
			DMA_ERR_FUN_LINE;
#endif /* DBG_DMA */
		return true;
	} else {
		return false;
	}
}

/**
 * __add_des_to_chain - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __add_des_to_chain(struct dma_channel_t *pchan, struct cofig_des_t *pdes)
{
	u32		uret = 0;
	u32		des_paddr = 0;
	u32		des_num = 0;
	struct des_mgr_t 	*pdes_mgr = NULL;
	struct cofig_des_t	*pdes_new = NULL;
	struct des_mgr_t 	*pdes_mgr_new = NULL;

	/*
	 * add a des to the end of the chain.
	 * if there is not empty space in chain, alloc new des & des_mgr
	 * change the lase des's link from 0xFFFFF800 to pdes's phys addr
	 */

#ifdef DBG_DMA
	if(NULL == pchan->pdes_mgr || NULL == pchan->pdes_mgr->pdes) {
		uret = __LINE__;
		goto End;
	}
#endif /* DBG_DMA */

	/* move to the last des mgr */
	pdes_mgr = pchan->pdes_mgr;
	while(NULL != pdes_mgr->pnext)
		pdes_mgr = pdes_mgr->pnext;

	/* get physical address for hw link */
	des_paddr = virt_to_phys(pdes);

	/* check if there is space left */
	des_num = pdes_mgr->des_num;
#ifdef DBG_DMA
	if(0 == des_num || des_num > MAX_DES_ITEM_NUM) {
		uret = __LINE__;
		goto End;
	}
#endif /* DBG_DMA */
	if(des_num < MAX_DES_ITEM_NUM) {
		/* add des to the end */
		memcpy(&pdes_mgr->pdes[des_num], pdes, sizeof(struct cofig_des_t));

		/* change the last des's link to pdes */
		DMA_DBG("%s: the last des's link(0x%08x) should be 0xFFFFF800\n", __FUNCTION__, \
			(u32)pdes_mgr->pdes[des_num - 1].pnext);
		pdes_mgr->pdes[des_num - 1].pnext = (struct cofig_des_t *)des_paddr;

		/* increase the des num */
		pdes_mgr->des_num++;
	} else {
		/* alloc new des and des mgr */
		pdes_new = kmem_cache_alloc(g_pdma_des, GFP_ATOMIC);
		if (NULL == pdes_new) {
			uret = __LINE__;
			goto End;
		}
		pdes_mgr_new = kmem_cache_alloc(g_pdma_des_mgr, GFP_ATOMIC);
		if (NULL == pdes_mgr_new) {
			uret = __LINE__;
			goto End;
		}
#ifdef DBG_DMA
		/* init des and des mgr */
		memset(pdes_new, 0, MAX_DES_ITEM_NUM * sizeof(struct cofig_des_t));
		pdes_mgr_new->des_num = 0;
		pdes_mgr_new->pnext = NULL;
#endif /* DBG_DMA */

		/* install des to the pdes_mgr_new's head */
		memcpy(&pdes_new[0], pdes, sizeof(struct cofig_des_t));

		/* last des link to pdes_new[0] */
		pdes_mgr->pdes[MAX_DES_ITEM_NUM - 1].pnext = (struct cofig_des_t *)des_paddr;

		/* pdes_mgr->pnext point to pdes_mgr_new */
		pdes_mgr->pnext = pdes_mgr_new;

		/* pdes_mgr_new's pdes point to pdes_new */
		pdes_mgr_new->pdes = pdes_new;

		/* increase des_num++ */
		pdes_mgr_new->des_num++;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);

		/* free des and des mgr alloced */
		if (NULL != pdes_new) {
			kmem_cache_free(g_pdma_des, pdes_new);
			pdes_new = NULL;
		}
		if (NULL != pdes_mgr_new) {
			kmem_cache_free(g_pdma_des_mgr, pdes_mgr_new);
			pdes_mgr_new = NULL;
		}
	}

	return uret;

}

/**
 * __dma_enqueue_phase_normal - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_enqueue_phase_normal(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from app
	 * so state:
	 * 	idle: 	  maybe before start or after stop
	 * 	running:
	 *		(1) hw queue done(idle & qd pending): clear/free des, enqueue the first new des, state -> done wait,
	 *			new des will be start in __dma_chan_handle_qd
	 *		(2) hw queue not done: pause -> enqueue -> resume
	 * 	done wait:
	 *		assert hw idle(already done), just enqueue to end, not need pause
	 * 	done:
	 *		assert hw idle, assert des idle, queue the firsst des, state -> running
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_DBG("%s: state idle, maybe before start or after stop, just enqueue to end\n", __FUNCTION__);
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
		if(true == __hw_transfer_done(pchan)) { /* queue done but __dma_chan_handle_qd not yet handled */
			DMA_DBG("%s: hw done\n", __FUNCTION__);
			/* save last config/param, clean des */
			if(0 != dma_save_and_clean_des(pchan)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* change state to done wait */
			pchan->state = DMA_CHAN_STA_DONE_WAIT;
		} else { /* transferring, so pause, add des to end, and resume */
			DMA_DBG("%s: hw not done\n", __FUNCTION__);
			/* pause dma */
			if(0 != dma_pause(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* resume dma */
			if(0 != dma_resume(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		}
		break;

	case DMA_CHAN_STA_DONE_WAIT:
		DMA_DBG("%s: state done wait, make sure hw done and des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __hw_transfer_done(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_DBG("%s: state done, make sure des chain is empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}

		/* start dma */
		if(NULL != pchan->op_cb.func) {
			if(0 != pchan->op_cb.func((dm_hdl_t)pchan, pchan->op_cb.parg, DMA_OP_START)) {
				uret = __LINE__;
				goto End;
			}
		}
		if(0 != dma_start(dma_hdl)) { /* state changed to running */
			uret = __LINE__;
			goto End;
		}
		break;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_hd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_enqueue_phase_hd(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from hd_cb
	 * so state:
	 * 	idle: 	  maybe stopped somewhere, err
	 * 	running:
	 *		(1) hw queue done(idle & qd pending): clear/free des, enqueue the first new des, state -> done wait,
	 *		(2) hw queue not done: pause -> enqueue -> resume
	 * 	done wait:
	 *		assert hw idle(already done), just enqueue to end, not need pause/start
	 * 	done:
	 *		print err
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_ERR("%s: state idle, maybe stopped somewhere, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		if(true == __hw_transfer_done(pchan)) { /* queue done but __dma_chan_handle_qd not yet handled */
			DMA_DBG("%s: hw done\n", __FUNCTION__);
			/* save last config/param, clean des */
			if(0 != dma_save_and_clean_des(pchan)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* change state to done wait */
			pchan->state = DMA_CHAN_STA_DONE_WAIT;
		} else { /* transferring, so pause, add des to end, and resume */
			DMA_DBG("%s: hw not done\n", __FUNCTION__);
			/* pause dma */
			if(0 != dma_pause(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* resume dma */
			if(0 != dma_resume(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		}
		break;

	case DMA_CHAN_STA_DONE_WAIT:
		DMA_DBG("%s: state done wait, make sure hw done and des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __hw_transfer_done(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_ERR("%s: state done, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_fd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_enqueue_phase_fd(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from fd_cb
	 * so state:
	 * 	idle: 	  maybe stopped somewhere, err
	 * 	running:
	 *		(1) hw queue done(idle & qd pending): clear/free des, enqueue the first new des, state -> done wait,
	 *		(2) hw queue not done: pause -> enqueue -> resume
	 * 	done wait:
	 *		assert hw idle(already done), just enqueue to end, not need pause
	 * 	done:
	 *		print err
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_ERR("%s: state idle, maybe stopped somewhere, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		if(true == __hw_transfer_done(pchan)) { /* queue done but __dma_chan_handle_qd not yet handled */
			DMA_DBG("%s: hw done\n", __FUNCTION__);
			/* save last config/param, clean des */
			if(0 != dma_save_and_clean_des(pchan)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* change state to done wait */
			pchan->state = DMA_CHAN_STA_DONE_WAIT;
		} else { /* transferring, so pause, add des to end, and resume */
			DMA_DBG("%s: hw not done\n", __FUNCTION__);
			/* pause dma */
			if(0 != dma_pause(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			/* add des to chain */
			if(0 != __add_des_to_chain(pchan, pdes)) {
				uret = __LINE__;
				goto End;
			}

			/* resume dma */
			if(0 != dma_resume(dma_hdl)) {
				uret = __LINE__;
				goto End;
			}

			//pchan->state = DMA_CHAN_STA_RUNING; /* state remain running */
		}
		break;

	case DMA_CHAN_STA_DONE_WAIT:
		DMA_DBG("%s: state done wait, make sure hw done and des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __hw_transfer_done(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_ERR("%s: state done, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * __dma_enqueue_phase_qd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_enqueue_phase_qd(dm_hdl_t dma_hdl, struct cofig_des_t *pdes)
{
	u32 	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/*
	 * called from qd_cb
	 * so state:
	 * 	idle: 	  maybe stopped somewhere, err
	 * 	running(normal case, not done before):
	 *		assert hw idle(already done): clear/free des, enqueue the first new des, state -> done wait,
	 * 	done wait(maybe from above(running) or from hd_cb/fd_cb...):
	 *		assert hw idle(already done), just enqueue to end, not need pause/start
	 * 	done:
	 *		print err
	 * Returns 0 if sucess, the err line number if failed.
	 */

	switch(pchan->state) {
	case DMA_CHAN_STA_IDLE:
		DMA_ERR("%s: state idle, maybe stopped somewhere, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	case DMA_CHAN_STA_RUNING:
		DMA_DBG("%s: state running, make sure des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true != __hw_transfer_done(pchan)) { /* assert hw idle(already done) */
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* save last config/param, clean des */
		if(0 != dma_save_and_clean_des(pchan)) {
			uret = __LINE__;
			goto End;
		}

		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}

		/* change state to done wait */
		pchan->state = DMA_CHAN_STA_DONE_WAIT;
		break;

	case DMA_CHAN_STA_DONE_WAIT:
		DMA_DBG("%s: state done wait, make sure hw done and des chain is NOT empty\n", __FUNCTION__);
#ifdef DBG_DMA
		if(true != __hw_transfer_done(pchan)) {
			uret = __LINE__;
			goto End;
		}
		if(true == __des_chain_is_empty(pchan)) {
			uret = __LINE__;
			goto End;
		}
#endif /* DBG_DMA */
		/* add des to chain */
		if(0 != __add_des_to_chain(pchan, pdes)) {
			uret = __LINE__;
			goto End;
		}
		break;

	case DMA_CHAN_STA_DONE:
		DMA_ERR("%s: state done, err\n", __FUNCTION__);
		uret = __LINE__;
		goto End;

	default:
		uret = __LINE__;
		goto End;
	}

End:
	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

/**
 * dma_enqueue - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 dma_enqueue(dm_hdl_t dma_hdl, struct cofig_des_t *pdes, enum dma_enque_phase_e phase)
{
	switch(phase) {
	case ENQUE_PHASE_NORMAL:
		return __dma_enqueue_phase_normal(dma_hdl, pdes);
	case ENQUE_PHASE_HD:
		return __dma_enqueue_phase_hd(dma_hdl, pdes);
	case ENQUE_PHASE_FD:
		return __dma_enqueue_phase_fd(dma_hdl, pdes);
	case ENQUE_PHASE_QD:
		return __dma_enqueue_phase_qd(dma_hdl, pdes);
	default:
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
}

/**
 * sw_dma_config - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 sw_dma_config(dm_hdl_t dma_hdl, struct dma_config_t *pcfg, enum dma_enque_phase_e phase)
{
	u32		uConfig = 0;
	struct cofig_des_t	des;
	struct dma_channel_t	*pchan = (struct dma_channel_t *)dma_hdl;

#ifdef DBG_DMA
	if(NULL == pchan) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* get irq surpport type for channel handle */
	pchan->irq_spt = pcfg->irq_spt;

	/* irq enable */
	csp_dma_chan_irq_enable(pchan, pcfg->irq_spt);

	/* get dma config val */
	uConfig |= xfer_arr[pcfg->xfer_type]; /* src/dst burst length and data width */
	uConfig |= addrtype_arr[pcfg->address_type]; /* src/dst address mode */
	uConfig |= (pcfg->src_drq_type << DMA_OFF_BITS_SDRQ)
			| (pcfg->dst_drq_type << DMA_OFF_BITS_DDRQ); /* src/dst drq type */

	/* fill cofig_des_t struct */
	memset(&des, 0, sizeof(des));
	des.cofig = uConfig;
	des.saddr = pcfg->src_addr;
	des.daddr = pcfg->dst_addr;
	des.bcnt  = pcfg->byte_cnt;
	des.param = pcfg->para;
	des.pnext = DMA_END_DES_LINK;

	/* des enqueue */
	if(0 != dma_enqueue(dma_hdl, &des, phase)) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	return 0;
}
EXPORT_SYMBOL(sw_dma_config);

/**
 * sw_dma_enqueue - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 sw_dma_enqueue(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt,
				enum dma_enque_phase_e phase)
{
	struct cofig_des_t	des;
	struct dma_channel_t 	*pdma_chan = (struct dma_channel_t *)dma_hdl;

	memset(&des, 0, sizeof(des));
	des.saddr 	= src_addr;
	des.daddr 	= dst_addr;
	des.bcnt 	= byte_cnt;
	des.cofig 	= pdma_chan->des_info_save.cofig;
	des.param 	= pdma_chan->des_info_save.param;
	des.pnext	= NULL;

	if(0 != dma_enqueue(dma_hdl, &des, phase)) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	return 0;
}
EXPORT_SYMBOL(sw_dma_enqueue);

/**
 * sw_dma_getposition - get the src and dst address from the reg
 * @dma_hdl:	dma handle
 * @pSrc:	pointed to src addr that will be got
 * @pDst:	pointed to dst addr that will be got
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int sw_dma_getposition(dm_hdl_t dma_hdl, u32 *pSrc, u32 *pDst)
{
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(NULL == dma_hdl || NULL == pSrc || NULL == pDst) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	DMA_DBG_FUN_LINE_TOCHECK;
	*pSrc = csp_dma_chan_get_cur_srcaddr(pchan);
	*pDst = csp_dma_chan_get_cur_dstaddr(pchan);

	DMA_DBG("%s: get *pSrc 0x%08x, *pDst 0x%08x\n", __FUNCTION__, *pSrc, *pDst);
	return 0;
}
EXPORT_SYMBOL(sw_dma_getposition);

/**
 * sw_dma_getcurposition - ????
 * @dma_hdl:	dma handle
 * @pSrc:	????
 * @pDst:	????
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int sw_dma_getcurposition(dm_hdl_t dma_hdl, u32 *pSrc, u32 *pDst)
{
	u32	usrc_addr = 0, udst_addr = 0, uleft_bcnt = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(NULL == dma_hdl || NULL == pSrc || NULL == pDst) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	usrc_addr = csp_dma_chan_get_cur_srcaddr(pchan);
	udst_addr = csp_dma_chan_get_cur_dstaddr(pchan);
	uleft_bcnt = csp_dma_chan_get_left_bytecnt(pchan);
	DMA_DBG_FUN_LINE_TOCHECK;
	*pSrc = usrc_addr + uleft_bcnt;
	*pDst = udst_addr + uleft_bcnt;

	DMA_DBG("%s: get *pSrc 0x%08x, *pDst 0x%08x\n", __FUNCTION__, (u32)*pSrc, (u32)*pDst);
	return 0;
}
EXPORT_SYMBOL(sw_dma_getcurposition);

/**
 * sw_dma_dump_chan - XXX
 * XXX		XXX
 *
 * XXX
 */
void sw_dma_dump_chan(dm_hdl_t dma_hdl)
{
	__dma_dump_channel((struct dma_channel_t *)dma_hdl);
	return;
}
EXPORT_SYMBOL(sw_dma_dump_chan);

/**
 * sw_dma_get_cur_bytes - get byte cnt for current buf
 * @dma_hdl:	dma handle
 *
 * Returns 0 if failed, the cur bytes cnt if success.
 */
u32 sw_dma_get_cur_bytes(dm_hdl_t dma_hdl)
{
	/* how to get cur buf byte cnt? */
	DMA_DBG_FUN_LINE_TOCHECK;
	return 0;
}
EXPORT_SYMBOL(sw_dma_get_cur_bytes);

/**
 * __dma_chan_handle_hd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_handle_hd(struct dma_channel_t *pchan)
{
	dma_cb 		func = pchan->hd_cb.func;
	void 		*parg = pchan->hd_cb.parg;

	if(NULL != func)
		return func((dm_hdl_t)pchan, parg, DMA_CB_OK);

	return 0;
}

/**
 * __dma_chan_handle_fd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_handle_fd(struct dma_channel_t *pchan)
{
	dma_cb 		func = pchan->fd_cb.func;
	void 		*parg = pchan->fd_cb.parg;

	if(NULL != func)
		return func((dm_hdl_t)pchan, parg, DMA_CB_OK);

	return 0;
}

/**
 * __dma_chan_handle_qd - XXXXXX
 * XXXXXX
 * XXXXXX
 *
 * XXXXXX
 */
u32 __dma_chan_handle_qd(struct dma_channel_t *pchan)
{
	u32 		uRet = 0;
	unsigned long	flags = 0;
	enum dma_chan_sta_e state;

	/*
	 * call the callback first
	 * we cannot lock fd_cb function, in case sw_dma_enqueue
	 * called and locked agin, which may lead to BUG somewhere
	 */
	//DMA_CHAN_LOCK(&pchan->lock, flags);
	if(NULL != pchan->qd_cb.func) {
		if(0 != pchan->qd_cb.func((dm_hdl_t)pchan, pchan->qd_cb.parg, DMA_CB_OK)) {
			DMA_ERR_FUN_LINE;
			return 0;
		}
	}
	//DMA_CHAN_UNLOCK(&pchan->lock, flags);

	DMA_CHAN_LOCK(&pchan->lock, flags);

	/*
	 * stopped when hd_cb calling?
	 */
	if(DMA_CHAN_STA_IDLE == pchan->state) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR("%s err, line %d, state idle, stopped during hd_cb/fd_cb/qd_cb or somewhere else? just return ok\n", \
			__FUNCTION__, __LINE__);
		return 0;
	}

	/*
	 * no enqueueing in hd_cb/fd_cb/qd_cb, the des is old,
	 * clear des and change state to done
	 */
	if(DMA_CHAN_STA_RUNING == pchan->state) {
		/* free the extra des, clear des */
		if(0 != dma_save_and_clean_des(pchan)) {
			uRet = __LINE__;
			goto End;
		}

		/* change state to done */
		pchan->state = DMA_CHAN_STA_DONE;

		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		return 0;
	}

	/*
	 * done before(in hd_cb/fd_cb/qd_cb queueing or normal queueing), the des is new,
	 * run the new buffer queue
	 */
	if(DMA_CHAN_STA_DONE_WAIT == pchan->state) {
		DMA_DBG_FUN_LINE;
		/* start the new queue(from begin) */
		if(NULL != pchan->op_cb.func) {
			if(0 != pchan->op_cb.func((dm_hdl_t)pchan, pchan->op_cb.parg, DMA_OP_START)) {
				uRet = __LINE__;
				goto End;
			}
		}
		if(0 != dma_start(pchan)) {
			uRet = __LINE__;
			goto End;
		}

		/* change state to running */
		//pchan->state = DMA_CHAN_STA_RUNING; /* done in dma_start */

		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		return 0;
	}

	/*
	 * should never be done
	 */
	if(DMA_CHAN_STA_DONE == pchan->state) {
		uRet = __LINE__;
		goto End;
	}

End:
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
	}

	return 0;
}

/**
 * dma_irq_hdl - dma irq process function
 * @irq:	dma physical irq num
 * @dev:	XXXXXXX
 *
 * Returns 0 if sucess, the err line number if failed.
 *
 * we cannot lock dma_irq_hdl through,
 * because sw_dma_enqueue maybe called in cb,
 * which will result in deadlock
 */
static irqreturn_t dma_irq_hdl(int irq, void *dev)
{
	u32 		i = 0;
	u32 		uline = 0;
	u32 		upend_bits = 0;
	u32 		uirq_spt = 0;
	struct dma_channel_t *pchan = NULL;
	struct dma_mgr_t *pdma_mgr = NULL;

	pdma_mgr = (struct dma_mgr_t *)dev;
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &pdma_mgr->chnl[i];
		uirq_spt = pdma_mgr->chnl[i].irq_spt;

		/* get channel irq pend bits */
		upend_bits = csp_dma_chan_get_irqpend(pchan);
		if(0 == upend_bits)
			continue;

		/* deal half done */
		if((upend_bits & CHAN_IRQ_HD) && (uirq_spt & CHAN_IRQ_HD)) {
			csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_HD);
			if(0 != __dma_chan_handle_hd(pchan)) {
				uline = __LINE__;
				goto End;
			}
		}

		/* deal full done */
		if((upend_bits & CHAN_IRQ_FD) && (uirq_spt & CHAN_IRQ_FD)) {
			csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_FD);
			if(0 != __dma_chan_handle_fd(pchan)) {
				uline = __LINE__;
				goto End;
			}
		}

		/* deal queue done */
		if((upend_bits & CHAN_IRQ_QD) && (uirq_spt & CHAN_IRQ_QD)) {
			csp_dma_chan_clear_irqpend(pchan, CHAN_IRQ_QD);
			if(0 != __dma_chan_handle_qd(pchan)) {
				uline = __LINE__;
				goto End;
			}
		}
	}

End:
	if(0 != uline) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uline);
	}

	return IRQ_HANDLED;
}

/**
 * dma_init - initial the dma manager, request irq
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int dma_init(void)
{
	int 		ret = 0;
	int 		i = 0;
	struct dma_channel_t *pchan = NULL;

        DMA_DBG_FUN_LINE;

	/* map dma regbase */
	g_dma_reg_vbase = ioremap_nocache(AW_DMA_BASE, 0x1000);

	/* init dma controller */
	csp_dma_init();

	/* initial the dma manager */
	memset(&g_dma_mgr, 0, sizeof(g_dma_mgr));
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &g_dma_mgr.chnl[i];

		pchan->used 	= 0;
		pchan->id 	= i;
		pchan->reg_base = DMA_EN_REG(i);
		pchan->irq_spt 	= CHAN_IRQ_NO;
		pchan->state = DMA_CHAN_STA_IDLE;
		DMA_CHAN_LOCK_INIT(&pchan->lock);

		/* these has cleared in memset-g_dma_mgr-0 */
		/*pchan->hash = 0;
		memset(pchan->owner, 0, sizeof(pchan->owner));
		memset(&pchan->hd_cb, 0, sizeof(struct dma_cb_t));
		memset(&pchan->fd_cb, 0, sizeof(struct dma_cb_t));
		memset(&pchan->qd_cb, 0, sizeof(struct dma_cb_t));
		memset(&pchan->op_cb, 0, sizeof(struct dma_op_cb_t));
		memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));
		pchan->pdes_mgr = NULL;*/
	}

	//use dma_alloc_coherent?
	DMA_DBG_FUN_LINE_TOCHECK;

	/* alloc des area */
	g_pdma_des = kmem_cache_create("dma_des", MAX_DES_ITEM_NUM * sizeof(struct cofig_des_t), 0,
					SLAB_HWCACHE_ALIGN | SLAB_CACHE_DMA, __des_cache_ctor);
	if (NULL == g_pdma_des) {
		ret = __LINE__;
		goto End;
	}

	/* alloc des mgr area */
	g_pdma_des_mgr = kmem_cache_create("dma_des_mgr", sizeof(struct des_mgr_t), 0,
					SLAB_HWCACHE_ALIGN, __des_mgr_cache_ctor);
	if (NULL == g_pdma_des_mgr) {
		ret = __LINE__;
		goto End;
	}

	/* register dma interrupt */
	ret = request_irq(DMA_IRQ_PHYS_NUM, dma_irq_hdl, IRQF_DISABLED, "dma_irq", (void *)&g_dma_mgr);
	if(ret) {
		DMA_ERR("%s err: request_irq return %d\n", __FUNCTION__, ret);
		ret = __LINE__;
		goto End;
	}

End:
	if(0 != ret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, ret);

		/* destory cache mem */
		if (NULL != g_pdma_des) {
			kmem_cache_destroy(g_pdma_des);
			g_pdma_des = NULL;
		}
		if (NULL != g_pdma_des_mgr) {
			kmem_cache_destroy(g_pdma_des_mgr);
			g_pdma_des_mgr = NULL;
		}

		/* deinit lock for each channel */
		for(i = 0; i < DMA_CHAN_TOTAL; i++) {
			DMA_CHAN_LOCK_DEINIT(&g_dma_mgr.chnl[i].lock);
		}
	}

	return ret;
}

/**
 * dma_deinit - deinit the dma manager, free irq
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int dma_deinit(void)
{
	int 	ret = 0;
	u32 	i = 0;

        DMA_DBG_FUN_LINE_TOCHECK;

	/* free dma irq */
	free_irq(DMA_IRQ_PHYS_NUM, (void *)&g_dma_mgr);

	/* destory cache mem */
	if (NULL != g_pdma_des) {
		kmem_cache_destroy(g_pdma_des);
		g_pdma_des = NULL;
	}

	if (NULL != g_pdma_des_mgr) {
		kmem_cache_destroy(g_pdma_des_mgr);
		g_pdma_des_mgr = NULL;
	}

	/* deinit lock for each channel */
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		DMA_CHAN_LOCK_DEINIT(&g_dma_mgr.chnl[i].lock);
	}

	/* clear dma manager */
	memset(&g_dma_mgr, 0, sizeof(g_dma_mgr));

	/* unmap dma reg base */
	iounmap(g_dma_reg_vbase);
	g_dma_reg_vbase = 0;

	return ret;
}

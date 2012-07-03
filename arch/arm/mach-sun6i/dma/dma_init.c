/*
 * arch/arm/mach-sun6i/dma/dma_init.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sun6i dma driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

/**
 * __des_mgr_cache_ctor - init function for g_pdes_mgr
 * @p:	pointer to g_pdes_mgr
 */
#ifndef USE_UNCACHED_FOR_DESMGR
static void __des_mgr_cache_ctor(void *p)
{
	memset(p, 0, sizeof(struct des_mgr_t));
}
#endif /* USE_UNCACHED_FOR_DESMGR */

/**
 * dma_init - initial the dma manager, request irq
 * @device:	platform device pointer
 *
 * Returns 0 if sucess, the err line number if failed.
 */
int dma_init(struct platform_device *device)
{
	int 		ret = 0;
	int 		i = 0;
	struct dma_channel_t *pchan = NULL;

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
		pchan->reg_base = (u32)DMA_EN_REG(i);
		pchan->irq_spt 	= CHAN_IRQ_NO;
		pchan->bconti_mode = false;
		//pchan->state.st_md_ch = DMA_CHAN_STA_IDLE;
		STATE_CHAIN(pchan) = 0; /* to check */
		pchan->work_mode = DMA_WORK_MODE_INVALID;
		DMA_CHAN_LOCK_INIT(&pchan->lock);

		/* these has cleared in memset-g_dma_mgr-0 */
		/*memset(pchan->owner, 0, sizeof(pchan->owner));
		memset(&pchan->hd_cb, 0, sizeof(struct dma_cb_t));
		memset(&pchan->fd_cb, 0, sizeof(struct dma_cb_t));
		memset(&pchan->qd_cb, 0, sizeof(struct dma_cb_t));
		memset(&pchan->op_cb, 0, sizeof(struct dma_op_cb_t));
		memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));
		pchan->pdes_mgr = NULL;*/
	}

	/* alloc des mgr area */
#ifdef USE_UNCACHED_FOR_DESMGR
	g_pdes_mgr = dmam_pool_create("dma_des_mgr", &device->dev, sizeof(struct des_mgr_t), 4, 0); /* DWORD align */
#else
	g_pdes_mgr = kmem_cache_create("dma_des_mgr", sizeof(struct des_mgr_t), 0,
					SLAB_HWCACHE_ALIGN, __des_mgr_cache_ctor);
#endif /* USE_UNCACHED_FOR_DESMGR */
	if(NULL == g_pdes_mgr) {
		ret = __LINE__;
		goto End;
	}

	/* alloc dma pool for des area */
	g_pool_ch = dmam_pool_create("dma_des", &device->dev, DES_AREA_LEN, 4, 0); /* DWORD align */
	if(NULL == g_pool_ch) {
		ret = __LINE__;
		goto End;
	}

	/* alloc dma pool for des list, for single mode only */
	g_pool_sg = dmam_pool_create("dma_deslist", &device->dev, sizeof(struct des_item_t), 4, 0); /* DWORD align */
	if(NULL == g_pool_sg) {
		ret = __LINE__;
		goto End;
	}

	/* register dma interrupt */
	ret = request_irq(AW_IRQ_DMA, dma_irq_hdl, IRQF_DISABLED, "dma_irq", (void *)&g_dma_mgr);
	if(ret) {
		DMA_ERR("%s err: request_irq return %d\n", __FUNCTION__, ret);
		ret = __LINE__;
		goto End;
	}
	DMA_DBG_FUN_LINE;

End:
	if(0 != ret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, ret);

		if (NULL != g_pool_sg) {
			dma_pool_destroy(g_pool_sg);
			g_pool_sg = NULL;
		}

		if (NULL != g_pool_ch) {
			dma_pool_destroy(g_pool_ch);
			g_pool_ch = NULL;
		}

#ifdef USE_UNCACHED_FOR_DESMGR
		if (NULL != g_pdes_mgr) {
			dma_pool_destroy(g_pdes_mgr);
			g_pdes_mgr = NULL;
		}
#else
		if (NULL != g_pdes_mgr) {
			kmem_cache_destroy(g_pdes_mgr);
			g_pdes_mgr = NULL;
		}
#endif /* USE_UNCACHED_FOR_DESMGR */

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
	free_irq(AW_IRQ_DMA, (void *)&g_dma_mgr);

	if (NULL != g_pool_sg) {
		dma_pool_destroy(g_pool_sg);
		g_pool_sg = NULL;
	}

	if (NULL != g_pool_ch) {
		dma_pool_destroy(g_pool_ch);
		g_pool_ch = NULL;
	}
#ifdef USE_UNCACHED_FOR_DESMGR
	if (NULL != g_pdes_mgr) {
		dma_pool_destroy(g_pdes_mgr);
		g_pdes_mgr = NULL;
	}
#else
	if (NULL != g_pdes_mgr) {
		kmem_cache_destroy(g_pdes_mgr);
		g_pdes_mgr = NULL;
	}
#endif /* USE_UNCACHED_FOR_DESMGR */

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

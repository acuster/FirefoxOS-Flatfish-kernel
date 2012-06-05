/*
 * arch/arm/mach-aw163x/dma/dma_init.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

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
 * dma_init - initial the dma manager, request irq
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
		pchan->reg_base = DMA_EN_REG(i);
		pchan->irq_spt 	= CHAN_IRQ_NO;
		pchan->bconti_mode = false;
		pchan->state = DMA_CHAN_STA_IDLE;
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

	DMA_DBG_FUN_LINE_TOCHECK;

	/* alloc des mgr area */
	g_pdma_des_mgr = kmem_cache_create("dma_des_mgr", sizeof(struct des_mgr_t), 0,
					SLAB_HWCACHE_ALIGN, __des_mgr_cache_ctor);
	if(NULL == g_pdma_des_mgr) {
		ret = __LINE__;
		goto End;
	}

	/* alloc dma pool for des area */
	g_pdma_pool = dmam_pool_create("dma_des", &device->dev, DES_AREA_LEN, 4, 0); /* DWORD align */
	if(NULL == g_pdma_pool) {
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

		if (NULL != g_pdma_pool) {
			dma_pool_destroy(g_pdma_pool);
			g_pdma_pool = NULL;
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
	free_irq(AW_IRQ_DMA, (void *)&g_dma_mgr);

	if (NULL != g_pdma_pool) {
		dma_pool_destroy(g_pdma_pool);
		g_pdma_pool = NULL;
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

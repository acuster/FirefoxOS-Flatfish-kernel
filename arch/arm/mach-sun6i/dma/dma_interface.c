/*
 * arch/arm/mach-sun6i/dma/dma_interface.c
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

/*
 * dma manager
 */
void __iomem *g_dma_reg_vbase = 0;

/*
 * dma manager
 */
struct dma_mgr_t g_dma_mgr; /* compile warining if "g_dma_mgr = {0}" */

/*
 * lock for request
 */
static DEFINE_MUTEX(dma_mutex);

/*
 * global ptr for dma descriptor and descriptor manager
 */
#ifdef USE_UNCACHED_FOR_DESMGR
struct dma_pool 	*g_pdes_mgr = NULL;
#else
struct kmem_cache 	*g_pdes_mgr = NULL;
#endif /* USE_UNCACHED_FOR_DESMGR */
struct dma_pool		*g_pool_ch = NULL;

/*
 * XXX, for single mode only
 */
struct dma_pool		*g_pool_sg = NULL;

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

#if 0 /* remove warning: defined but not used */
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
	DMA_DBG("  xfer_type:         %d\n", para->xfer_type);
	DMA_DBG("  address_type:      %d\n", para->address_type);
	DMA_DBG("  para:              0x%08x\n", para->para);
	DMA_DBG("  irq_spt:           %d\n", para->irq_spt);
	DMA_DBG("  src_addr:          0x%08x\n", para->src_addr);
	DMA_DBG("  dst_addr:          0x%08x\n", para->dst_addr);
	DMA_DBG("  byte_cnt:          0x%08x\n", para->byte_cnt);
	DMA_DBG("  bconti_mode:       %d\n", para->bconti_mode);
	DMA_DBG("  src_drq_type:      %d\n", para->src_drq_type);
	DMA_DBG("  dst_drq_type:      %d\n", para->dst_drq_type);
	DMA_DBG("-----------%s-----------\n", __FUNCTION__);
}
#endif

/**
 * __dma_dump_channel - dump dma channel info
 * @dma_hdl:	dma handle
 */
void __dma_dump_channel(struct dma_channel_t *pchan)
{
	u32 	i = 0;
	struct des_mgr_t *pdes_mgr = NULL;

	if(NULL == pchan) {
		DMA_ERR_FUN_LINE;
		return;
	}

	DMA_DBG("+++++++++++%s+++++++++++\n", __FUNCTION__);
	DMA_DBG("  channel id:        %d\n", pchan->id);
	DMA_DBG("  channel used:      %d\n", pchan->used);
	DMA_DBG("  channel owner:     %s\n", pchan->owner);
	DMA_DBG("  bconti_mode:       %d\n", pchan->bconti_mode);
	DMA_DBG("  channel irq_spt:   0x%08x\n", pchan->irq_spt);
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

	/* dump the des chain */
	pdes_mgr = pchan->pdes_mgr;
	while(NULL != pdes_mgr) {
		DMA_DBG("     pdes:        0x%08x\n", (u32)pdes_mgr->pdes);
		DMA_DBG("     des_pa:      0x%08x\n", (u32)pdes_mgr->des_pa);
		DMA_DBG("     des_num:     0x%08x\n", pdes_mgr->des_num);

		/* dump chain for cur des mgr */
		for(i = 0; i < pdes_mgr->des_num; i++) {
		DMA_DBG(" cofig/saddr/daddr/bcnt/param/pnext: 0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/0x%08x/\n", \
			pdes_mgr->pdes[i].cofig, pdes_mgr->pdes[i].saddr, pdes_mgr->pdes[i].daddr, \
			pdes_mgr->pdes[i].bcnt, pdes_mgr->pdes[i].param, (u32)pdes_mgr->pdes[i].pnext);
		}

		DMA_DBG("     pnext:       0x%08x\n", (u32)pdes_mgr->pnext);
		DMA_DBG("\n");
		pdes_mgr = pdes_mgr->pnext;
	}

	DMA_DBG("  channel des_info_save:  (cofig: 0x%08x, param: 0x%08x)\n", pchan->des_info_save.cofig, \
		pchan->des_info_save.param);
	DMA_DBG("  channel state:     0x%08x\n", (u32)STATE_CHAIN(pchan));
	DMA_DBG("-----------%s-----------\n", __FUNCTION__);
}

/**
 * dma_check_handle - check if dma handle is valid
 * @dma_hdl:	dma handle
 *
 * return 0 if vaild, the err line number if not vaild
 */
u32 dma_check_handle(dm_hdl_t dma_hdl)
{
	u32	uret = 0;
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	if(NULL == pchan) {
		uret = __LINE__;
		goto End;
	}
	if(0 == pchan->used) {	/* already released? */
		uret = __LINE__;
		goto End;
	}
	if(DMA_WORK_MODE_INVALID == pchan->work_mode) {
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
 * __dma_check_channel_free - check if channel is free
 * @pchan:	dma handle
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
		//&& CHAN_IRQ_NO == pchan->irq_spt /* maybe not use dma irq? */
		&& NULL == pchan->hd_cb.func
		&& NULL == pchan->fd_cb.func
		&& NULL == pchan->qd_cb.func
		&& NULL == pchan->op_cb.func
		&& NULL == pchan->pdes_mgr
		&& DMA_CHAN_STA_IDLE == STATE_CHAIN(pchan)
		&& DMA_WORK_MODE_INVALID == pchan->work_mode
		) {
		return true;
	} else {
		__dma_dump_channel(pchan);
		return false;
	}

	return false;
}

/**
 * __dma_channel_already_exist - check if channel already requested by others
 * @name:	channel name
 *
 * return true if channel already requested, false if not
 */
bool __dma_channel_already_exist(char *name)
{
	u32		i = 0;
	struct dma_channel_t	*pchan = NULL;

	if(NULL == name) {
		DMA_ERR_FUN_LINE;
		return false;
	}

	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &g_dma_mgr.chnl[i];
		if(1 == pchan->used && !strcmp(pchan->owner, name)) {
			return true;
		}
	}

	return false;
}

/**
 * dma_chan_des_mgr_init - init the channel's descriptor manager
 * @pchan:	dma handle
 *
 * return 0 if success, the err line number if not
 */
u32 dma_chan_des_mgr_init(struct dma_channel_t *pchan)
{
#ifdef USE_UNCACHED_FOR_DESMGR
	dma_addr_t		dma_addr_temp = 0;
#endif /* USE_UNCACHED_FOR_DESMGR */
	u32		uRet = 0;
	dma_addr_t		des_paddr = 0; /* dma bus addr for pdes */
	struct cofig_des_t	*pdes = NULL;
	struct des_mgr_t	*pdes_mgr = NULL;

	DMA_DBG_FUN_LINE_TOCHECK;

	/* use dma_pool_alloc to alloc uncached mem for descriptor, which will be set to hw */
	pdes = (struct cofig_des_t *)dma_pool_alloc(g_pool_ch, GFP_ATOMIC, &des_paddr);
	if (NULL == pdes) {
		uRet = __LINE__;
		goto End;
	}

	DMA_DBG("%s: dma_pool_alloc return va 0x%08x, pa 0x%08x, virt_to_phys(va) 0x%08x\n", __FUNCTION__, \
		(u32)pdes, des_paddr, (u32)virt_to_phys(pdes));

	/* use kmem_cache_alloc to alloc cached mem, for des mgr not need to be uncached */
#ifdef USE_UNCACHED_FOR_DESMGR
	pdes_mgr = (struct des_mgr_t *)dma_pool_alloc(g_pdes_mgr, GFP_ATOMIC, &dma_addr_temp);
#else
	pdes_mgr = kmem_cache_alloc(g_pdes_mgr, GFP_ATOMIC);
#endif /* USE_UNCACHED_FOR_DESMGR */
	if (NULL == pdes_mgr) {
		uRet = __LINE__;
		goto End;
	}

End:
	if(0 != uRet) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uRet);
		if (NULL != pdes) {
			dma_pool_free(g_pool_ch, pdes, des_paddr);
			pdes = NULL;
			des_paddr = 0;
		}
		if (NULL != pdes_mgr) {
#ifdef USE_UNCACHED_FOR_DESMGR
			dma_pool_free(g_pdes_mgr, pdes_mgr, dma_addr_temp);
#else
			kmem_cache_free(g_pdes_mgr, pdes_mgr);
#endif /* USE_UNCACHED_FOR_DESMGR */
			pdes_mgr = NULL;
		}
	} else {
		DMA_DBG("%s: pdes_mgr 0x%08x, pdes 0x%08x, des_paddr 0x%08x\n", __FUNCTION__, \
			(u32)pdes_mgr, (u32)pdes, des_paddr);
		pchan->pdes_mgr = pdes_mgr;
		pchan->pdes_mgr->pdes = pdes;
		pchan->pdes_mgr->des_pa = des_paddr;
#ifdef USE_UNCACHED_FOR_DESMGR
		pchan->pdes_mgr->des_mgr_pa = dma_addr_temp;
#endif /* USE_UNCACHED_FOR_DESMGR */
	}

	return uRet;
}

/**
 * dma_chan_des_mgr_deinit - deinit the channel's descriptor manager
 * @pchan:	dma handle
 *
 * return 0 if success, the err line number if not
 */
u32 dma_chan_des_mgr_deinit(struct dma_channel_t *pchan)
{
#ifdef USE_UNCACHED_FOR_DESMGR
	dma_addr_t		dma_addr_temp = 0;
#endif /* USE_UNCACHED_FOR_DESMGR */

	DMA_INF("%s: pdes_mgr 0x%08x, pdes 0x%08x, des_pa 0x%08x\n", __FUNCTION__, \
		(u32)pchan->pdes_mgr, (u32)pchan->pdes_mgr->pdes, pchan->pdes_mgr->des_pa);

	if (NULL != pchan->pdes_mgr->pdes) {
		dma_pool_free(g_pool_ch, pchan->pdes_mgr->pdes, pchan->pdes_mgr->des_pa);
		pchan->pdes_mgr->pdes = NULL;
		pchan->pdes_mgr->des_pa = 0;
	}

	if (NULL != pchan->pdes_mgr) {
#ifdef USE_UNCACHED_FOR_DESMGR
		dma_addr_temp = pchan->pdes_mgr->des_mgr_pa;
		dma_pool_free(g_pdes_mgr, pchan->pdes_mgr, dma_addr_temp);
#else
		kmem_cache_free(g_pdes_mgr, pchan->pdes_mgr);
#endif /* USE_UNCACHED_FOR_DESMGR */
		pchan->pdes_mgr = NULL;
	}

	return 0;
}

/**
 * sw_dma_request - request a dma channel
 * @name:	dma channel name
 *
 * Returns handle to the channel if success, NULL if failed.
 */
dm_hdl_t sw_dma_request(char * name, enum dma_work_mode_e work_mode)
{
	u32	i = 0;
	u32	usign = 0;
	struct dma_channel_t	*pchan = NULL;

	DMA_DBG("%s: name %s\n", __FUNCTION__, name);

	/* para check */
	if(strlen(name) >= MAX_OWNER_NAME_LEN
		|| (work_mode != DMA_WORK_MODE_CHAIN
			&& work_mode != DMA_WORK_MODE_SINGLE)) {
		DMA_ERR("%s: para err, name %s, work mode %d\n", __FUNCTION__, name, (u32)work_mode);
		return NULL;
	}

	mutex_lock(&dma_mutex);

	/* check if already exist */
	if(NULL != name) {
		if(true == __dma_channel_already_exist(name)) {
			usign = __LINE__;
			goto End;
		}
	}

	/* get a free channel */
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		pchan = &g_dma_mgr.chnl[i];
		if(0 == pchan->used) {
#ifdef DBG_DMA
			/* check if dma channel free */
			if(true != __dma_check_channel_free(pchan)) {
				DMA_ERR_FUN_LINE;
			}
#endif /* DBG_DMA */
			break;
		}
	}

	/* cannot get a free channel */
	if(DMA_CHAN_TOTAL == i) {
		usign = __LINE__;
		goto End;
	}

	/* init channel */
	if(DMA_WORK_MODE_CHAIN == work_mode) {
		if(0 != dma_chan_des_mgr_init(pchan)) {
			usign = __LINE__;
			goto End;
		}
	} else if(DMA_WORK_MODE_SINGLE == work_mode) {
		INIT_LIST_HEAD(&pchan->buf_list_head);
	}
	pchan->used = 1;
	if(NULL != name)
		strcpy(pchan->owner, name);
	pchan->work_mode = work_mode;

End:
	mutex_unlock(&dma_mutex);

	if(0 != usign) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, usign);
		return (dm_hdl_t)NULL;
	} else {
		DMA_DBG("%s: success, channel id %d\n", __FUNCTION__, i);
		return (dm_hdl_t)pchan;
	}
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
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* XXX */
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		return dma_release_single(dma_hdl);

	DMA_CHAN_LOCK(&pchan->lock, flags);

#ifdef DBG_DMA
	if(0 != dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* if not idle, call stop first */
	if(DMA_CHAN_STA_IDLE != STATE_CHAIN(pchan)) {
		DMA_INF("%s maybe err: line %d, state(%d) not idle, call stop dma first!\n", \
			__FUNCTION__, __LINE__, STATE_CHAIN(pchan));

		if(0 != dma_stop(dma_hdl)) {
			DMA_ERR_FUN_LINE;
		}
	}

	//memset(pchan, 0, sizeof(*pchan)); /* donot do that, because id...shouldnot be cleared */
	pchan->used 	= 0;
	memset(pchan->owner, 0, sizeof(pchan->owner));

	pchan->irq_spt 	= CHAN_IRQ_NO;
	pchan->bconti_mode = false;
	memset(&pchan->des_info_save, 0, sizeof(pchan->des_info_save));

	memset(&pchan->op_cb, 0, sizeof(pchan->op_cb));
	memset(&pchan->hd_cb, 0, sizeof(pchan->hd_cb));
	memset(&pchan->fd_cb, 0, sizeof(pchan->fd_cb));
	memset(&pchan->qd_cb, 0, sizeof(pchan->qd_cb));

	/* maybe enqueued but not started, so free and clean des */
	if(0 != dma_clean_des(pchan))
		DMA_ERR_FUN_LINE;

	if(0 != dma_chan_des_mgr_deinit(pchan))
		DMA_ERR_FUN_LINE;

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	return 0;
}
EXPORT_SYMBOL(sw_dma_release);

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
	struct dma_channel_t *pchan = (struct dma_channel_t *)dma_hdl;

	/* XXX */
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		return dma_ctrl_single(dma_hdl, op, parg);

	/* only in start/stop case can parg be NULL  */
	if((NULL == parg)
		&& (DMA_OP_START != op)
		&& (DMA_OP_PAUSE != op)
		&& (DMA_OP_RESUME != op)
		&& (DMA_OP_STOP != op)) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}

	DMA_CHAN_LOCK(&pchan->lock, flags);

#ifdef DBG_DMA
	if(0 != dma_check_handle(dma_hdl)) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* let the caller to do some operation before op */
	if((DMA_OP_SET_OP_CB != op) && (NULL != pchan->op_cb.func)) {
		if(0 != pchan->op_cb.func(dma_hdl, pchan->op_cb.parg, op))
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
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	if(0 != uRet) {
		DMA_ERR("%s err, line %d, dma_hdl 0x%08x\n", __FUNCTION__, uRet, (u32)dma_hdl);
	}

	return uRet;
}
EXPORT_SYMBOL(sw_dma_ctl);

/**
 * sw_dma_config - config dma channel, enqueue the buffer
 * @dma_hdl:	dma handle
 * @pcfg:	dma cofig para
 * @phase:	dma enqueue phase
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_dma_config(dm_hdl_t dma_hdl, struct dma_config_t *pcfg, enum dma_enque_phase_e phase)
{
	u32 		uret = 0;
	u32		uConfig = 0;
	unsigned long	flags = 0;
	struct cofig_des_t	des;
	struct dma_channel_t	*pchan = (struct dma_channel_t *)dma_hdl;

#ifdef DBG_DMA
	if(NULL == pchan) {
		DMA_ERR_FUN_LINE;
		return __LINE__;
	}
#endif /* DBG_DMA */

	/* XXX */
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode) {
		if(true == pcfg->bconti_mode) {
			DMA_ERR_FUN_LINE;
			return __LINE__;
		}
		return dma_config_single(dma_hdl, pcfg, phase);
	}

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
#ifdef DBG_DMA
	if(true == pcfg->bconti_mode) {
		DMA_INF("%s: continue mode, temply set pnext to 0xFFFFF800, will changed to \
			pdes_mgr->pdes's pa in __dma_enqueue_contimode\n", __FUNCTION__);
	}
#endif /*DBG_DMA*/
	des.pnext = (struct cofig_des_t *)DMA_END_DES_LINK;

	/* get continue mode flag */
	pchan->bconti_mode = pcfg->bconti_mode;

	/* get irq surport type for channel handle */
	pchan->irq_spt = pcfg->irq_spt;

	/* bkup config/param */
	pchan->des_info_save.cofig = uConfig;
	pchan->des_info_save.param = pcfg->para;

	/*
	 * when called in irq, just use spin_lock, not spin_lock_irqsave
	 */
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_LOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_LOCK_IN_IRQHD(&pchan->lock);
	}

	/* irq enable */
	csp_dma_chan_irq_enable(pchan, pcfg->irq_spt);

	/* des enqueue */
	if(0 != dma_enqueue(dma_hdl, &des, phase)) {
		uret = __LINE__;
		goto End;
	}

End:
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_UNLOCK_IN_IRQHD(&pchan->lock);
	}

	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}
EXPORT_SYMBOL(sw_dma_config);

/**
 * sw_dma_enqueue - enqueue the buffer to des chain
 * @dma_hdl:	dma handle
 * @src_addr:	buffer src phys addr
 * @dst_addr:	buffer dst phys addr
 * @byte_cnt:	buffer byte cnt
 * @phase:	enqueue phase
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_dma_enqueue(dm_hdl_t dma_hdl, u32 src_addr, u32 dst_addr, u32 byte_cnt,
				enum dma_enque_phase_e phase)
{
	u32 			uret = 0;
	unsigned long		flags = 0;
	struct cofig_des_t	des;
	struct dma_channel_t 	*pchan = (struct dma_channel_t *)dma_hdl;

	/* XXX */
	if(DMA_WORK_MODE_SINGLE == pchan->work_mode)
		return dma_enqueue_single(dma_hdl, src_addr, dst_addr, byte_cnt, phase);

	memset(&des, 0, sizeof(des));
	des.saddr 	= src_addr;
	des.daddr 	= dst_addr;
	des.bcnt 	= byte_cnt;
	des.cofig 	= pchan->des_info_save.cofig;
	des.param 	= pchan->des_info_save.param;
#ifdef DBG_DMA
	if(true == pchan->bconti_mode) {
		DMA_INF("%s: continue mode, temply set pnext to 0xFFFFF800, will changed to \
			pdes_mgr->pdes's pa in __dma_enqueue_contimode\n", __FUNCTION__);
	}
#endif /* DBG_DMA */
	des.pnext	= (struct cofig_des_t *)DMA_END_DES_LINK;

	/*
	 * when called in irq, just use spin_lock, not spin_lock_irqsave
	 */
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_LOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_LOCK_IN_IRQHD(&pchan->lock);
	}

	if(0 != dma_enqueue(dma_hdl, &des, phase)) {
		uret = __LINE__;
		goto End;
	}

End:
	if(ENQUE_PHASE_NORMAL == phase) {
		DMA_CHAN_UNLOCK(&pchan->lock, flags);
	} else {
		DMA_CHAN_UNLOCK_IN_IRQHD(&pchan->lock);
	}

	if(0 != uret) {
		DMA_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}
	return uret;
}
EXPORT_SYMBOL(sw_dma_enqueue);

/**
 * XXX - XXX
 * @XXX:	XXX
 *
 * XXX
 */
u32 sw_dma_getsoftsta(dm_hdl_t dma_hdl)
{
	u32 	uret = 0xFFFFFFFF;
	unsigned long	flags = 0;
	struct dma_channel_t 	*pchan = (struct dma_channel_t *)dma_hdl;

	/* to check, can we call spin_lock_irqsave in irq? */
	DMA_DBG_FUN_LINE_TOCHECK;
	DMA_CHAN_LOCK(&pchan->lock, flags);

	if(DMA_WORK_MODE_CHAIN == pchan->work_mode) {
		uret =  pchan->state.st_md_ch;
	} else if(DMA_WORK_MODE_SINGLE == pchan->work_mode) {
		uret =  pchan->state.st_md_sg;
	}

	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	return uret;
}
EXPORT_SYMBOL(sw_dma_getsoftsta);

/**
 * XXX - XXX
 * @XXX:	XXX
 *
 * XXX
 */
u32 sw_dma_sgmd_buflist_empty(dm_hdl_t dma_hdl)
{
	bool 	bret = false;
	unsigned long	flags = 0;
	struct dma_channel_t 	*pchan = (struct dma_channel_t *)dma_hdl;

	/* to check, can we call spin_lock_irqsave in irq? */
	DMA_CHAN_LOCK(&pchan->lock, flags);
	if(list_empty(&pchan->buf_list_head))
		bret = true;
	else
		bret = false;
	DMA_CHAN_UNLOCK(&pchan->lock, flags);
	return bret;
}
EXPORT_SYMBOL(sw_dma_sgmd_buflist_empty);

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
	if(0 == pchan->used) {
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
 * sw_dma_dump_chan - dump dma chain
 * @dma_hdl:	dma handle
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

/*
 * arch/arm/mach-sun6i/dma/dma_csp.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i dma csp functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

struct clk 	*g_dma_ahb_clk = NULL;
struct clk 	*g_dma_mod_clk = NULL;

u32 dma_clk_init(void)
{
	if(NULL != g_dma_mod_clk || NULL != g_dma_ahb_clk) {
		DMA_INF("%s maybe err: g_dma_mod_clk(0x%08x)/g_dma_ahb_clk(0x%08x) not NULL, line %d\n",
			__func__, (u32)g_dma_mod_clk, (u32)g_dma_ahb_clk, __LINE__);
	}

	/* config dma module clock */
	g_dma_mod_clk = clk_get(NULL, CLK_MOD_DMA);
	DMA_DBG("%s: get g_dma_mod_clk 0x%08x\n", __func__, (u32)g_dma_mod_clk);
	if(NULL == g_dma_mod_clk || IS_ERR(g_dma_mod_clk)) {
		DMA_ERR("%s err: clk_get %s failed\n", __func__, CLK_MOD_DMA);
		return -EPERM;
	} else {
		if(0 != clk_enable(g_dma_mod_clk)) {
			DMA_ERR("%s err: clk_enable failed, line %d\n", __func__, __LINE__);
			return -EPERM;
		}
		DMA_DBG("%s: clk_enable g_dma_mod_clk success\n", __func__);

		if(0 != clk_reset(g_dma_mod_clk, AW_CCU_CLK_NRESET)) {
			DMA_ERR("%s err: clk_reset failed, line %d\n", __func__, __LINE__);
			return -EPERM;
		}
		DMA_DBG("%s: clk_reset g_dma_mod_clk-AW_CCU_CLK_NRESET success\n", __func__);
	}

	/* config dma ahb clock */
	g_dma_ahb_clk = clk_get(NULL, CLK_AHB_DMA);
	DMA_DBG("%s: get g_dma_ahb_clk 0x%08x\n", __func__, (u32)g_dma_ahb_clk);
	if(NULL == g_dma_ahb_clk || IS_ERR(g_dma_ahb_clk)) {
		printk("%s err: clk_get %s failed\n", __func__, CLK_AHB_DMA);
		return -EPERM;
	} else {
		if(0 != clk_enable(g_dma_ahb_clk)) {
			DMA_ERR("%s err: clk_enable failed, line %d\n", __func__, __LINE__);
			return -EPERM;
		}
		DMA_DBG("%s: clk_enable g_dma_ahb_clk success\n", __func__);
	}

	DMA_DBG("%s success\n", __func__);
	return 0;
}

u32 dma_clk_deinit(void)
{
	DMA_DBG("%s: g_dma_mod_clk 0x%08x, g_dma_ahb_clk 0x%08x\n",
		__func__, (u32)g_dma_mod_clk, (u32)g_dma_ahb_clk);

	/* release dma mode clock */
	if(NULL == g_dma_mod_clk || IS_ERR(g_dma_mod_clk)) {
		DMA_INF("%s: g_dma_mod_clk 0x%08x invalid, just return\n", __func__, (u32)g_dma_mod_clk);
		return 0;
	} else {
		if(0 != clk_reset(g_dma_mod_clk, AW_CCU_CLK_RESET)) {
			DMA_ERR("%s err: clk_reset failed\n", __func__);
		}
		DMA_DBG("%s: clk_reset g_dma_mod_clk-AW_CCU_CLK_RESET success\n", __func__);
		clk_disable(g_dma_mod_clk);
		clk_put(g_dma_mod_clk);
		g_dma_mod_clk = NULL;
	}

	/* release dma ahb clock */
	if(NULL == g_dma_ahb_clk || IS_ERR(g_dma_ahb_clk)) {
		DMA_INF("%s: g_dma_ahb_clk 0x%08x invalid, just return\n", __func__, (u32)g_dma_ahb_clk);
		return 0;
	} else {
		clk_disable(g_dma_ahb_clk);
		clk_put(g_dma_ahb_clk);
		g_dma_ahb_clk = NULL;
	}

	DMA_DBG("%s success\n", __func__);
	return 0;
}

/**
 * csp_dma_init - init dmac
 */
void csp_dma_init(void)
{
	u32 	i = 0;

	/* init dma clock */
	if(0 != dma_clk_init())
		DMA_ERR("%s err, dma_clk_init failed, line %d\n", __func__, __LINE__);

	/* Disable & clear all interrupts */
	DMA_WRITE_REG(0, DMA_IRQ_EN_REG0);
	DMA_WRITE_REG(0, DMA_IRQ_EN_REG1);
	DMA_WRITE_REG(0xffffffff, DMA_IRQ_PEND_REG0);
	DMA_WRITE_REG(0xffffffff, DMA_IRQ_PEND_REG1);

	/* init enable reg */
	for(i = 0; i < DMA_CHAN_TOTAL; i++)
		DMA_WRITE_REG(0, DMA_EN_REG(i));
}

/**
 * csp_dma_chan_set_startaddr - set the des's start phys addr to start reg, then we can start the dma
 * @pchan:	dma channel handle
 * @ustart_addr: dma channel start physcal address to set
 */
void csp_dma_chan_set_startaddr(struct dma_channel_t * pchan, u32 ustart_addr)
{
	DMA_WRITE_REG(ustart_addr, pchan->reg_base + DMA_OFF_REG_START);
}

/**
 * csp_dma_chan_start - start the dma channel
 * @pchan:	dma channel handle
 */
void csp_dma_chan_start(struct dma_channel_t * pchan)
{
	DMA_WRITE_REG(1, pchan->reg_base + DMA_OFF_REG_EN);
}

/**
 * csp_dma_chan_pause - pause the dma channel
 * @pchan:	dma channel handle
 */
void csp_dma_chan_pause(struct dma_channel_t * pchan)
{
	DMA_WRITE_REG(1, pchan->reg_base + DMA_OFF_REG_PAUSE);
}

/**
 * csp_dma_chan_resume - resume the dma channel
 * @pchan:	dma channel handle
 */
void csp_dma_chan_resume(struct dma_channel_t * pchan)
{
	DMA_WRITE_REG(0, pchan->reg_base + DMA_OFF_REG_PAUSE);
}

/**
 * csp_dma_chan_stop - stop the dma channel
 * @pchan:	dma channel handle
 */
void csp_dma_chan_stop(struct dma_channel_t * pchan)
{
	DMA_WRITE_REG(0, pchan->reg_base + DMA_OFF_REG_EN);
}

/**
 * csp_dma_chan_get_status - get dma channel status
 * @pchan:	dma channel handle
 *
 * Returns 1 indicate channel is busy, 0 idle
 */
u32 csp_dma_chan_get_status(struct dma_channel_t * pchan)
{
	return ((DMA_READ_REG(DMA_STATE_REG) >> pchan->id) & 1);
}

/**
 * csp_dma_chan_get_cur_srcaddr - get dma channel's cur src addr reg value
 *
 * Returns the channel's cur src addr reg value
 */
u32 csp_dma_chan_get_cur_srcaddr(struct dma_channel_t * pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_SRC);
}

/**
 * csp_dma_chan_get_cur_dstaddr - get dma channel's cur dst addr reg value
 * @pchan:	dma channel handle
 *
 * Returns the channel's cur dst addr reg value
 */
u32 csp_dma_chan_get_cur_dstaddr(struct dma_channel_t * pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_CUR_DST);
}

/**
 * csp_dma_chan_get_left_bytecnt - get dma channel's left byte cnt
 * @pchan:	dma channel handle
 *
 * Returns the channel's left byte cnt
 */
u32 csp_dma_chan_get_left_bytecnt(struct dma_channel_t * pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_BCNT_LEFT);
}

/**
 * csp_dma_chan_get_startaddr - get dma channel's start address reg value
 * @pchan:	dma channel handle
 *
 * Returns the dma channel's start address reg value
 */
u32 csp_dma_chan_get_startaddr(struct dma_channel_t *pchan)
{
	return DMA_READ_REG(pchan->reg_base + DMA_OFF_REG_START);
}

/**
 * csp_dma_chan_irq_enable - enable dma channel irq
 * @pchan:	dma channel handle
 * @irq_type:	irq type that will be enabled
 */
void csp_dma_chan_irq_enable(struct dma_channel_t * pchan, u32 irq_type)
{
	u32 	uTemp = 0;

	if(pchan->id < 8) {
		uTemp = DMA_READ_REG(DMA_IRQ_EN_REG0);
		uTemp &= (~(0xf << (pchan->id << 2)));
		uTemp |= (irq_type << (pchan->id << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_EN_REG0);
	} else {
		uTemp = DMA_READ_REG(DMA_IRQ_EN_REG1);
		uTemp &= (~(0xf << ((pchan->id - 8) << 2)));
		uTemp |= (irq_type << ((pchan->id - 8) << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_EN_REG1);
	}
}

/**
 * csp_dma_chan_get_status - get dma channel irq pending val
 * @pchan:	dma channel handle
 *
 * Returns the irq pend value, eg: 0b101
 */
u32 csp_dma_chan_get_irqpend(struct dma_channel_t * pchan)
{
	u32 	uret = 0;
	u32 	utemp = 0;

	if(pchan->id < 8) {
		utemp = DMA_READ_REG(DMA_IRQ_PEND_REG0);
		uret = (utemp >> (pchan->id << 2)) & 0x7;
	} else {
		utemp = DMA_READ_REG(DMA_IRQ_PEND_REG1);
		uret = (utemp >> ((pchan->id - 8) << 2)) & 0x7;
	}
	return uret;
}

/**
 * csp_dma_chan_clear_irqpend - clear the dma channel irq pending
 * @pchan:	dma channel handle
 * @irq_type:	irq type that willbe cleared, eg: CHAN_IRQ_HD|CHAN_IRQ_FD
 */
void csp_dma_chan_clear_irqpend(struct dma_channel_t * pchan, u32 irq_type)
{
	u32 	uTemp = 0;

	if(pchan->id < 8) {
		uTemp = DMA_READ_REG(DMA_IRQ_PEND_REG0);
		uTemp &= (irq_type << (pchan->id << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_PEND_REG0);
	} else {
		uTemp = DMA_READ_REG(DMA_IRQ_PEND_REG1);
		uTemp &= (irq_type << ((pchan->id - 8) << 2));
		DMA_WRITE_REG(uTemp, DMA_IRQ_PEND_REG1);
	}
}

/**
 * csp_dma_clear_irqpend - clear dma irq pending register
 * @index:	irq pend reg index, 0 or 1
 *
 * Returns the irq pend value before cleared, 0xffffffff if failed
 */
u32 csp_dma_clear_irqpend(u32 index)
{
	u32 	uret = 0;
	u32 	ureg_addr = 0;

	if(0 == index)
		ureg_addr = (u32)DMA_IRQ_PEND_REG0;
	else if(1 == index)
		ureg_addr = (u32)DMA_IRQ_PEND_REG1;
	else {
		DMA_ERR("%s err, line %d\n", __func__, __LINE__);
		return 0xffffffff;
	}

	uret = DMA_READ_REG(ureg_addr);
	DMA_WRITE_REG(uret, ureg_addr);
	return uret;
}


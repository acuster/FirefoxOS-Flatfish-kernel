/*
 * arch/arm/mach-aw163x/dma/dma_csp.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * aw163x dma csp functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "dma_include.h"

#ifdef FROM_SD_TESTCODE
#include "dma_csp_from_sdtest.c"
#endif /* FROM_SD_TESTCODE */

/**
 * csp_dma_init - init dmac
 */
void csp_dma_init(void)
{
	u32 	i = 0;

#ifdef FROM_SD_TESTCODE
	ccm_module_reset(DMA_CKID);
	ccm_clock_enable(DMA_CKID);
#endif /* FROM_SD_TESTCODE */

	/* Disable & clear all interrupts */
	DMA_WRITE_REG(0, DMA_IRQ_EN_REG0);
	DMA_WRITE_REG(0, DMA_IRQ_EN_REG1);
	DMA_WRITE_REG(0xffffffff, DMA_IRQ_PEND_REG0);
	DMA_WRITE_REG(0xffffffff, DMA_IRQ_PEND_REG1);

	/* init enable reg */
	for(i = 0; i < DMA_CHAN_TOTAL; i++) {
		DMA_WRITE_REG(0, DMA_EN_REG(i));
	}
}

/**
 * csp_dma_chan_set_startaddr - XXX
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

#if 0
/**
 * csp_dma_chan_config - config the dma channel, then can start dma
 * @pchan:	dma channel handle
 * @pCfg:	dma config para
 *
 * should be called before start dma.
 */
void csp_dma_chan_config(struct dma_channel_t * pchan, struct dma_config_t * pCfg)
{
	u32	uConfig = 0;

	/* src/dst burst length and data width */
	uConfig |= xfer_arr[pCfg->xfer_type];

	/* src/dst address mode */
	uConfig |= addrtype_arr[pCfg->address_type];

	/* src/dst drq type */
	uConfig |= (pCfg->src_drq_type << DMA_OFF_BITS_SDRQ) | (pCfg->dst_drq_type << DMA_OFF_BITS_DDRQ);

	DMA_WRITE_REG(uConfig, pchan->reg_base + DMA_OFF_REG_CFG);

	/* irq enable */
	csp_dma_chan_irq_enable(pchan, pCfg->irq_spt);

	/* para reg */
	DMA_WRITE_REG(pCfg->para, pchan->reg_base + DMA_OFF_REG_PARA);
}
#endif

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

	if(0 == index) {
		ureg_addr = (u32)DMA_IRQ_PEND_REG0;
	} else if(1 == index){
		ureg_addr = (u32)DMA_IRQ_PEND_REG1;
	} else {
		DMA_ERR_FUN_LINE;
		return 0xffffffff;
	}

	uret = DMA_READ_REG(ureg_addr);
	DMA_WRITE_REG(uret, ureg_addr);

	return uret;
}

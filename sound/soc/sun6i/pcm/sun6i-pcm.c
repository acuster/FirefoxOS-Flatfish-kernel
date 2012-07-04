/*
 * sound\soc\sun6i\pcm\sun6i-pcm.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * chenpailin <chenpailin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include <mach/clock.h>

#include <mach/hardware.h>
#include <asm/dma.h>
#include <mach/dma.h>

#include "sun6i-pcmdma.h"
#include "sun6i-pcm.h"

//=============mode selection====================
//#define I2S_COMMUNICATION
#define PCM_COMMUNICATION
//-----------------------------------------------
static unsigned long over_sample_rate = 256;		//128fs/192fs/256fs/384fs/512fs/768fs
static unsigned long sample_resolution = 16;		//16bits/20bits/24bits
static unsigned long word_select_size = 32;		//16bits/20bits/24bits/32bits
static unsigned long pcm_sync_period = 256;		//16/32/64/128/256
static unsigned long msb_lsb_first = 0;			//0: msb first; 1: lsb first
static unsigned long slot_index = 0;				//slot index: 0: the 1st slot - 3: the 4th slot
static unsigned long slot_width = 16;				//8 bit width / 16 bit width
static unsigned long frame_width = 1;				//0: long frame = 2 clock width;  1: short frame
static unsigned long tx_data_mode = 0;				//0: 16bit linear PCM; 1: 8bit linear PCM; 2: 8bit u-law; 3: 8bit a-law
static unsigned long rx_data_mode = 0;				//0: 16bit linear PCM; 1: 8bit linear PCM; 2: 8bit u-law; 3: 8bit a-law
//===============================================

static int regsave[8];
static int pcm_used = 1;//0;

static struct sun6i_dma_params sun6i_pcm_pcm_stereo_out = {
	.name		= "pcm_play",
	.dma_addr	= SUN6I_PCMBASE + SUN6I_PCMTXFIFO,//send data address
};

static struct sun6i_dma_params sun6i_pcm_pcm_stereo_in = {
	.name   	= "pcm_capture",
	.dma_addr	=SUN6I_PCMBASE + SUN6I_PCMRXFIFO,//accept data address
};

struct sun6i_pcm_info sun6i_pcm;
static struct clk *pcm_apbclk, *pcm_pll2clk, *pcm_pllx8, *pcm_moduleclk;

static void sun6i_snd_txctrl_pcm(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMTXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUN6I_PCMTXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMTXCHSEL);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMTXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x3200;
	} else {
		reg_val = 0x3210;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMTXCHMAP);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= SUN6I_PCMCTL_SDO0EN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	//flush TX FIFO
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMFCTL);
	reg_val |= SUN6I_PCMFCTL_FTX;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFCTL);

	//clear TX counter
	writel(0, sun6i_pcm.regs + SUN6I_PCMTXCNT);

	if (on) {
		/* PCM TX ENABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val |= SUN6I_PCMCTL_TXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

		/* enable DMA DRQ mode for play */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val |= SUN6I_PCMINT_TXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);

		//Global Enable Digital Audio Interface
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val |= SUN6I_PCMCTL_GEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	} else {
		/* PCM TX DISABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val &= ~SUN6I_PCMCTL_TXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val &= ~SUN6I_PCMINT_TXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);

		//Global disable Digital Audio Interface
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val &= ~SUN6I_PCMCTL_GEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
	}
}

static void sun6i_snd_rxctrl_pcm(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;
	//printk("Enter %s, line = %d, on = %d\n", __func__, __LINE__, on);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMRXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUN6I_PCMRXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMRXCHSEL);

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMRXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x00003200;
	} else {
		reg_val = 0x00003210;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMRXCHMAP);

	//flush RX FIFO
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMFCTL);
	reg_val |= SUN6I_PCMFCTL_FRX;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFCTL);

	//clear RX counter
	writel(0, sun6i_pcm.regs + SUN6I_PCMRXCNT);

	if (on) {
		/* PCM RX ENABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val |= SUN6I_PCMCTL_RXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

		/* enable DMA DRQ mode for record */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val |= SUN6I_PCMINT_RXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);

		//Global Enable Digital Audio Interface
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val |= SUN6I_PCMCTL_GEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	} else {
		/* PCM RX DISABLE */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val &= ~SUN6I_PCMCTL_RXEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

		/* DISBALE dma DRQ mode */
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMINT);
		reg_val &= ~SUN6I_PCMINT_RXDRQEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMINT);

		//Global disable Digital Audio Interface
		reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
		reg_val &= ~SUN6I_PCMCTL_GEN;
		writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
	}
}

static inline int sun6i_snd_is_clkmaster(void)
{
	return ((readl(sun6i_pcm.regs + SUN6I_PCMCTL) & SUN6I_PCMCTL_MS) ? 0 : 1);
}

static int sun6i_pcm_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val;
	u32 reg_val1;

	//SDO ON
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= (SUN6I_PCMCTL_SDO0EN);
#ifdef PCM_COMMUNICATION
	reg_val |= SUN6I_PCMCTL_PCM;
#endif
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	/* master or slave selection */
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master */
			reg_val |= SUN6I_PCMCTL_MS;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave */
			reg_val &= ~SUN6I_PCMCTL_MS;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	/* pcm or pcm mode selection */
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val1 = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	reg_val1 &= ~SUN6I_PCMFAT0_FMT_RVD;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S mode */
			reg_val &= ~SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_FMT_I2S;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val &= ~SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_FMT_RGT;
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val &= ~SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_FMT_LFT;
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val |= SUN6I_PCMCTL_PCM;
			reg_val1 &= ~SUN6I_PCMFAT0_LRCP;
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val |= SUN6I_PCMCTL_PCM;
			reg_val1 |= SUN6I_PCMFAT0_LRCP;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);
	writel(reg_val1, sun6i_pcm.regs + SUN6I_PCMFAT0);

	/* DAI signal inversions */
	reg_val1 = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUN6I_PCMFAT0_LRCP;
			reg_val1 &= ~SUN6I_PCMFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUN6I_PCMFAT0_LRCP;
			reg_val1 &= ~SUN6I_PCMFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUN6I_PCMFAT0_LRCP;
			reg_val1 |= SUN6I_PCMFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUN6I_PCMFAT0_LRCP;
			reg_val1 |= SUN6I_PCMFAT0_BCP;
			break;
	}
	writel(reg_val1, sun6i_pcm.regs + SUN6I_PCMFAT0);

	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUN6I_PCMFCTL_RXTL(0x1f);				//RX FIFO trigger level
	reg_val |= SUN6I_PCMFCTL_TXTL(0x40);				//TX FIFO empty trigger level
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFCTL);
	return 0;
}

static int sun6i_pcm_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun6i_dma_params *dma_data;

	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun6i_pcm_pcm_stereo_out;
	else
		dma_data = &sun6i_pcm_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sun6i_pcm_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	//struct snd_soc_pcm_runtime *rtd = substream->private_data;
	//struct sun6i_dma_params *dma_data =
	//				snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_pcm(substream, 1);
			} else {
				sun6i_snd_txctrl_pcm(substream, 1);
			}
		#if 0
			printk("[PCM] 0x01c22400+0x00 = %#x, line= %d\n", readl(0xf1c22400+0x00), __LINE__);
			printk("[PCM] 0x01c22400+0x04 = %#x, line= %d\n", readl(0xf1c22400+0x04), __LINE__);
			printk("[PCM] 0x01c22400+0x14 = %#x, line= %d\n", readl(0xf1c22400+0x14), __LINE__);
			printk("[PCM] 0x01c22400+0x18 = %#x, line= %d\n", readl(0xf1c22400+0x18), __LINE__);
			printk("[PCM] 0x01c22400+0x1c = %#x, line= %d\n", readl(0xf1c22400+0x1c), __LINE__);
			printk("[PCM] 0x01c22400+0x20 = %#x, line= %d\n", readl(0xf1c22400+0x20), __LINE__);
			printk("[PCM] 0x01c22400+0x2c = %#x, line= %d\n", readl(0xf1c22400+0x2c), __LINE__);
			printk("[PCM] 0x01c22400+0x38 = %#x, line= %d\n", readl(0xf1c22400+0x38), __LINE__);
			printk("[PCM] 0x01c22400+0x3c = %#x, line= %d\n", readl(0xf1c22400+0x3c), __LINE__);
		#endif
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_pcm(substream, 0);
			} else {
			  sun6i_snd_txctrl_pcm(substream, 0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int sun6i_pcm_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
                                 unsigned int freq, int dir)
{
	clk_set_rate(pcm_pll2clk, freq);

	return 0;
}
static int sun6i_pcm_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	u32 reg_val;
	u32 fs;
	u32 mclk;
	u32 mclk_div = 0;
	u32 bclk_div = 0;
	u32 wss;

	fs = div;
	mclk = over_sample_rate;

#ifdef PCM_COMMUNICATION
	wss = word_select_size;
#endif

#ifdef PCM_COMMUNICATION
	//mclk div caculate
	switch(fs)
	{
		case 8000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 24;
							break;
				case 192:	mclk_div = 16;
							break;
				case 256:	mclk_div = 12;
							break;
				case 384:	mclk_div = 8;
							break;
				case 512:	mclk_div = 6;
							break;
				case 768:	mclk_div = 4;
							break;
			}
			break;
		}

		case 16000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 12;
							break;
				case 192:	mclk_div = 8;
							break;
				case 256:	mclk_div = 6;
							break;
				case 384:	mclk_div = 4;
							break;
				case 768:	mclk_div = 2;
							break;
			}
			break;
		}

		case 32000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 6;
							break;
				case 192:	mclk_div = 4;
							break;
				case 384:	mclk_div = 2;
							break;
				case 768:	mclk_div = 1;
							break;
			}
			break;
		}

		case 64000:
		{
			switch(mclk)
			{
				case 192:	mclk_div = 2;
							break;
				case 384:	mclk_div = 1;
							break;
			}
			break;
		}

		case 128000:
		{
			switch(mclk)
			{
				case 192:	mclk_div = 1;
							break;
			}
			break;
		}

		case 11025:
		case 12000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 16;
							break;
				case 256:	mclk_div = 8;
							break;
				case 512:	mclk_div = 4;
							break;
			}
			break;
		}

		case 22050:
		case 24000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 8;
							break;
				case 256:	mclk_div = 4;
							break;
				case 512:	mclk_div = 2;
							break;
			}
			break;
		}

		case 44100:
		case 48000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 4;
							break;
				case 256:	mclk_div = 2;
							break;
				case 512:	mclk_div = 1;
							break;
			}
			break;
		}

		case 88200:
		case 96000:
		{
			switch(mclk)
			{
				case 128:	mclk_div = 2;
							break;
				case 256:	mclk_div = 1;
							break;
			}
			break;
		}

		case 176400:
		case 192000:
		{
			mclk_div = 1;
			break;
		}

	}

	//bclk div caculate
	bclk_div = mclk/(2*wss);
#else
	mclk_div = 2;
	bclk_div = 6;
#endif

	//calculate MCLK Divide Ratio
	switch(mclk_div)
	{
		case 1: mclk_div = 0;
				break;
		case 2: mclk_div = 1;
				break;
		case 4: mclk_div = 2;
				break;
		case 6: mclk_div = 3;
				break;
		case 8: mclk_div = 4;
				break;
		case 12: mclk_div = 5;
				 break;
		case 16: mclk_div = 6;
				 break;
		case 24: mclk_div = 7;
				 break;
		case 32: mclk_div = 8;
				 break;
		case 48: mclk_div = 9;
				 break;
		case 64: mclk_div = 0xA;
				 break;
	}
	mclk_div &= 0xf;

	//calculate BCLK Divide Ratio
	switch(bclk_div)
	{
		case 2: bclk_div = 0;
				break;
		case 4: bclk_div = 1;
				break;
		case 6: bclk_div = 2;
				break;
		case 8: bclk_div = 3;
				break;
		case 12: bclk_div = 4;
				 break;
		case 16: bclk_div = 5;
				 break;
		case 32: bclk_div = 6;
				 break;
		case 64: bclk_div = 7;
				 break;
	}
	bclk_div &= 0x7;

	//set mclk and bclk dividor register
	reg_val = mclk_div;
	reg_val |= (bclk_div<<4);
	reg_val |= (0x1<<7);
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCLKD);

	/* word select size */
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	sun6i_pcm.ws_size = word_select_size;
	reg_val &= ~SUN6I_PCMFAT0_WSS_32BCLK;
	if(sun6i_pcm.ws_size == 16)
		reg_val |= SUN6I_PCMFAT0_WSS_16BCLK;
	else if(sun6i_pcm.ws_size == 20)
		reg_val |= SUN6I_PCMFAT0_WSS_20BCLK;
	else if(sun6i_pcm.ws_size == 24)
		reg_val |= SUN6I_PCMFAT0_WSS_24BCLK;
	else
		reg_val |= SUN6I_PCMFAT0_WSS_32BCLK;

	sun6i_pcm.samp_res = sample_resolution;
	reg_val &= ~SUN6I_PCMFAT0_SR_RVD;
	if(sun6i_pcm.samp_res == 16)
		reg_val |= SUN6I_PCMFAT0_SR_16BIT;
	else if(sun6i_pcm.samp_res == 20)
		reg_val |= SUN6I_PCMFAT0_SR_20BIT;
	else
		reg_val |= SUN6I_PCMFAT0_SR_24BIT;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFAT0);

	/* PCM REGISTER setup */
	sun6i_pcm.pcm_txtype = tx_data_mode;
	sun6i_pcm.pcm_rxtype = rx_data_mode;
	reg_val = sun6i_pcm.pcm_txtype&0x3;
	reg_val |= sun6i_pcm.pcm_rxtype<<2;

	sun6i_pcm.pcm_sync_type = frame_width;
	if(sun6i_pcm.pcm_sync_type)
		reg_val |= SUN6I_PCMFAT1_SSYNC;

	sun6i_pcm.pcm_sw = slot_width;
	if(sun6i_pcm.pcm_sw == 16)
		reg_val |= SUN6I_PCMFAT1_SW;

	sun6i_pcm.pcm_start_slot = slot_index;
	reg_val |=(sun6i_pcm.pcm_start_slot & 0x3)<<6;

	sun6i_pcm.pcm_lsb_first = msb_lsb_first;
	reg_val |= sun6i_pcm.pcm_lsb_first<<9;

	sun6i_pcm.pcm_sync_period = pcm_sync_period;
	if(sun6i_pcm.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sun6i_pcm.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sun6i_pcm.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sun6i_pcm.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMFAT1);

	return 0;
}

static int sun6i_pcm_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}
static int sun6i_pcm_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void pcmregsave(void)
{
	regsave[0] = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	regsave[1] = readl(sun6i_pcm.regs + SUN6I_PCMFAT0);
	regsave[2] = readl(sun6i_pcm.regs + SUN6I_PCMFAT1);
	regsave[3] = readl(sun6i_pcm.regs + SUN6I_PCMFCTL) | (0x3<<24);
	regsave[4] = readl(sun6i_pcm.regs + SUN6I_PCMINT);
	regsave[5] = readl(sun6i_pcm.regs + SUN6I_PCMCLKD);
	regsave[6] = readl(sun6i_pcm.regs + SUN6I_PCMTXCHSEL);
	regsave[7] = readl(sun6i_pcm.regs + SUN6I_PCMTXCHMAP);
}

static void pcmregrestore(void)
{
	writel(regsave[0], sun6i_pcm.regs + SUN6I_PCMCTL);
	writel(regsave[1], sun6i_pcm.regs + SUN6I_PCMFAT0);
	writel(regsave[2], sun6i_pcm.regs + SUN6I_PCMFAT1);
	writel(regsave[3], sun6i_pcm.regs + SUN6I_PCMFCTL);
	writel(regsave[4], sun6i_pcm.regs + SUN6I_PCMINT);
	writel(regsave[5], sun6i_pcm.regs + SUN6I_PCMCLKD);
	writel(regsave[6], sun6i_pcm.regs + SUN6I_PCMTXCHSEL);
	writel(regsave[7], sun6i_pcm.regs + SUN6I_PCMTXCHMAP);
}

static int sun6i_pcm_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	printk("[PCM]Entered %s\n", __func__);

	//Global Enable Digital Audio Interface
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val &= ~SUN6I_PCMCTL_GEN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	pcmregsave();

	//release the module clock
	clk_disable(pcm_moduleclk);

	clk_disable(pcm_apbclk);

	return 0;
}
static int sun6i_pcm_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	printk("[PCM]Entered %s\n", __func__);

	//release the module clock
	clk_enable(pcm_apbclk);

	//release the module clock
	clk_enable(pcm_moduleclk);

	pcmregrestore();

	//Global Enable Digital Audio Interface
	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= SUN6I_PCMCTL_GEN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	return 0;
}

#define SUN6I_PCM_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun6i_pcm_dai_ops = {
	.trigger 	= sun6i_pcm_trigger,
	.hw_params 	= sun6i_pcm_hw_params,
	.set_fmt 	= sun6i_pcm_set_fmt,
	.set_clkdiv = sun6i_pcm_set_clkdiv,
	.set_sysclk = sun6i_pcm_set_sysclk,
};

static struct snd_soc_dai_driver sun6i_pcm_dai = {
	.probe 		= sun6i_pcm_dai_probe,
	.suspend 	= sun6i_pcm_suspend,
	.resume 	= sun6i_pcm_resume,
	.remove 	= sun6i_pcm_dai_remove,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_PCM_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops 		= &sun6i_pcm_dai_ops,
};

static int __devinit sun6i_pcm_dev_probe(struct platform_device *pdev)
{
	int reg_val = 0;
	int ret;

	sun6i_pcm.regs = ioremap(SUN6I_PCMBASE, 0x100);
	if (sun6i_pcm.regs == NULL)
		return -ENXIO;

	//pcm apbclk
	pcm_apbclk = clk_get(NULL, "apb_pcm");
	if(-1 == clk_enable(pcm_apbclk)){
		printk("pcm_apbclk failed! line = %d\n", __LINE__);
	}

	pcm_pllx8 = clk_get(NULL, "audio_pllx8");

	//pcm pll2clk
	pcm_pll2clk = clk_get(NULL, "audio_pll");

	//pcm module clk
	pcm_moduleclk = clk_get(NULL, "pcm");

	if(clk_set_parent(pcm_moduleclk, pcm_pll2clk)){
		printk("try to set parent of pcm_moduleclk to pcm_pll2ck failed! line = %d\n",__LINE__);
	}

	if(clk_set_rate(pcm_moduleclk, 24576000/8)){
		printk("set pcm_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}

	if(-1 == clk_enable(pcm_moduleclk)){
		printk("open pcm_moduleclk failed! line = %d\n", __LINE__);
	}

	reg_val = readl(sun6i_pcm.regs + SUN6I_PCMCTL);
	reg_val |= SUN6I_PCMCTL_GEN;
	writel(reg_val, sun6i_pcm.regs + SUN6I_PCMCTL);

	iounmap(sun6i_pcm.ioregs);
	ret = snd_soc_register_dai(&pdev->dev, &sun6i_pcm_dai);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __devexit sun6i_pcm_dev_remove(struct platform_device *pdev)
{
	if(pcm_used) {
		pcm_used = 0;
		//release the module clock
		clk_disable(pcm_moduleclk);

		//release pllx8clk
		clk_put(pcm_pllx8);

		//release pll2clk
		clk_put(pcm_pll2clk);

		//release apbclk
		clk_put(pcm_apbclk);

		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

/*data relating*/
static struct platform_device sun6i_pcm_device = {
	.name = "sun6i-pcm",
};

/*method relating*/
static struct platform_driver sun6i_pcm_driver = {
	.probe = sun6i_pcm_dev_probe,
	.remove = __devexit_p(sun6i_pcm_dev_remove),
	.driver = {
		.name = "sun6i-pcm",
		.owner = THIS_MODULE,
	},
};

static int __init sun6i_pcm_init(void)
{
	int err = 0;

	if (pcm_used) {
		if((err = platform_device_register(&sun6i_pcm_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sun6i_pcm_driver)) < 0)
			return err;
	} else {
        printk("[PCM]sun6i-pcm cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }
	return 0;
}
module_init(sun6i_pcm_init);

static void __exit sun6i_pcm_exit(void)
{
	platform_driver_unregister(&sun6i_pcm_driver);
}
module_exit(sun6i_pcm_exit);

module_platform_driver(sun6i_pcm_driver);
/* Module information */
MODULE_AUTHOR("ALLWINNER");
MODULE_DESCRIPTION("sun6i PCM SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun6i-pcm");

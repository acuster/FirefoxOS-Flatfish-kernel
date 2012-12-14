/*
 * sound\soc\sun6i\i2s\sun6i-i2s.c
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * chenpailin <chenpailin@Reuuimllatech.com>
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

#include <asm/dma.h>
#include <mach/clock.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/gpio.h>

#include "sun6i-i2sdma.h"
#include "sun6i-i2s.h"

/*=============mode selection====================*/
//#define I2S_COMMUNICATION
#define PCM_COMMUNICATION
/*-----------------------------------------------*/
unsigned long over_sample_rate 	= 256;		/*128fs/192fs/256fs/384fs/512fs/768fs*/
unsigned long sample_resolution = 16;		/*16bits/20bits/24bits*/
unsigned long word_select_size 	= 32;		/*16bits/20bits/24bits/32bits*/
unsigned long pcm_sync_period 	= 256;		/*16/32/64/128/256*/
unsigned long msb_lsb_first 	= 0;		/*0: msb first; 1: lsb first*/
unsigned long sign_extend 		= 0;		/*0: zero pending; 1: sign extend*/
unsigned long slot_index 		= 0;		/*slot index: 0: the 1st slot - 3: the 4th slot*/
unsigned long slot_width 		= 16;		/*8 bit width / 16 bit width*/
unsigned long frame_width 		= 1;		/*0: long frame = 2 clock width;  1: short frame*/
unsigned long tx_data_mode 		= 0;		/*0: 16bit linear PCM; 1: 8bit linear PCM; 2: 8bit u-law; 3: 8bit a-law*/
unsigned long rx_data_mode 		= 0;		/*0: 16bit linear PCM; 1: 8bit linear PCM; 2: 8bit u-law; 3: 8bit a-law*/
/*===============================================*/

static int regsave[8];
static int i2s_used = 0;
static struct sun6i_dma_params sun6i_i2s_pcm_stereo_out = {
	.name		= "i2s_play",	
	.dma_addr	= SUN6I_IISBASE + SUN6I_IISTXFIFO,/*send data address	*/
};

static struct sun6i_dma_params sun6i_i2s_pcm_stereo_in = {
	.name   	= "i2s_capture",
	.dma_addr	=SUN6I_IISBASE + SUN6I_IISRXFIFO,/*accept data address	*/
};

struct sun6i_i2s_info sun6i_iis;
static struct clk *i2s_apbclk 		= NULL;
static struct clk *i2s_pll2clk 		= NULL;
static struct clk *i2s_pllx8 		= NULL;
static struct clk *i2s_moduleclk	= NULL;

void sun6i_snd_txctrl_i2s(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;

	reg_val = readl(sun6i_iis.regs + SUN6I_TXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUN6I_TXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sun6i_iis.regs + SUN6I_TXCHSEL);

	reg_val = readl(sun6i_iis.regs + SUN6I_TXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x76543200;
	} else {
		reg_val = 0x76543210;
	}
	writel(reg_val, sun6i_iis.regs + SUN6I_TXCHMAP);

	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	reg_val &= ~SUN6I_IISCTL_SDO3EN;
	reg_val &= ~SUN6I_IISCTL_SDO2EN;
	reg_val &= ~SUN6I_IISCTL_SDO1EN;	
	reg_val &= ~SUN6I_IISCTL_SDO0EN;
	switch(substream->runtime->channels) {
		case 1:
		case 2:
			reg_val |= SUN6I_IISCTL_SDO0EN; break;
	#ifdef I2S_COMMUNICATION
		case 3:
		case 4:
			reg_val |= SUN6I_IISCTL_SDO0EN | SUN6I_IISCTL_SDO1EN; break;
		case 5:
		case 6:
			reg_val |= SUN6I_IISCTL_SDO0EN | SUN6I_IISCTL_SDO1EN | SUN6I_IISCTL_SDO2EN; break;
		case 7:
		case 8:
			reg_val |= SUN6I_IISCTL_SDO0EN | SUN6I_IISCTL_SDO1EN | SUN6I_IISCTL_SDO2EN | SUN6I_IISCTL_SDO3EN; break;	
	#endif
		default:
			reg_val |= SUN6I_IISCTL_SDO0EN; break;
	}
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
	
	/*flush TX FIFO*/
	reg_val = readl(sun6i_iis.regs + SUN6I_IISFCTL);
	reg_val |= SUN6I_IISFCTL_FTX;	
	writel(reg_val, sun6i_iis.regs + SUN6I_IISFCTL);
	
	/*clear TX counter*/
	writel(0, sun6i_iis.regs + SUN6I_IISTXCNT);

	if (on) {
		/* IIS TX ENABLE */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
		reg_val |= SUN6I_IISCTL_TXEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
		
		/* enable DMA DRQ mode for play */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISINT);
		reg_val |= SUN6I_IISINT_TXDRQEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISINT);
		
		/*Global Enable Digital Audio Interface*/
		reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
		reg_val |= SUN6I_IISCTL_GEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);

	} else {
		/* IIS TX DISABLE */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
		reg_val &= ~SUN6I_IISCTL_TXEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISINT);
		reg_val &= ~SUN6I_IISINT_TXDRQEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISINT);
	}		
}

void sun6i_snd_rxctrl_i2s(struct snd_pcm_substream *substream, int on)
{
	u32 reg_val;	

	reg_val = readl(sun6i_iis.regs + SUN6I_RXCHSEL);
	reg_val &= ~0x7;
	reg_val |= SUN6I_RXCHSEL_CHNUM(substream->runtime->channels);
	writel(reg_val, sun6i_iis.regs + SUN6I_RXCHSEL);

	reg_val = readl(sun6i_iis.regs + SUN6I_RXCHMAP);
	reg_val = 0;
	if(substream->runtime->channels == 1) {
		reg_val = 0x00003200;
	} else {
		reg_val = 0x00003210;
	}
	writel(reg_val, sun6i_iis.regs + SUN6I_RXCHMAP);
	
	/*flush RX FIFO*/
	reg_val = readl(sun6i_iis.regs + SUN6I_IISFCTL);
	reg_val |= SUN6I_IISFCTL_FRX;	
	writel(reg_val, sun6i_iis.regs + SUN6I_IISFCTL);

	/*clear RX counter*/
	writel(0, sun6i_iis.regs + SUN6I_IISRXCNT);
	
	if (on) {
		/* IIS RX ENABLE */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
		reg_val |= SUN6I_IISCTL_RXEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
			
		/* enable DMA DRQ mode for record */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISINT);
		reg_val |= SUN6I_IISINT_RXDRQEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISINT);
			
		/*Global Enable Digital Audio Interface*/
		reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
		reg_val |= SUN6I_IISCTL_GEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
			
	} else {
		/* IIS RX DISABLE */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
		reg_val &= ~SUN6I_IISCTL_RXEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
			
		/* DISBALE dma DRQ mode */
		reg_val = readl(sun6i_iis.regs + SUN6I_IISINT);
		reg_val &= ~SUN6I_IISINT_RXDRQEN;
		writel(reg_val, sun6i_iis.regs + SUN6I_IISINT);
	}		
}

static inline int sun6i_snd_is_clkmaster(void)
{
	return ((readl(sun6i_iis.regs + SUN6I_IISCTL) & SUN6I_IISCTL_MS) ? 0 : 1);
}

static int sun6i_i2s_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	u32 reg_val;
	u32 reg_val1;

	/*SDO ON*/
	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	reg_val |= (SUN6I_IISCTL_SDO0EN | SUN6I_IISCTL_SDO1EN | SUN6I_IISCTL_SDO2EN | SUN6I_IISCTL_SDO3EN); 
#ifdef PCM_COMMUNICATION
	reg_val |= SUN6I_IISCTL_PCM;
#endif
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);

	/* master or slave selection */
	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	switch(fmt & SND_SOC_DAIFMT_MASTER_MASK){
		case SND_SOC_DAIFMT_CBM_CFM:   /* codec clk & frm master */
			reg_val |= SUN6I_IISCTL_MS;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:   /* codec clk & frm slave */
			reg_val &= ~SUN6I_IISCTL_MS;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
	
	/* pcm or i2s mode selection */
	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	reg_val1 = readl(sun6i_iis.regs + SUN6I_IISFAT0);
	reg_val1 &= ~SUN6I_IISFAT0_FMT_RVD;
	switch(fmt & SND_SOC_DAIFMT_FORMAT_MASK){
		case SND_SOC_DAIFMT_I2S:        /* I2S mode */
			reg_val &= ~SUN6I_IISCTL_PCM;
			reg_val1 |= SUN6I_IISFAT0_FMT_I2S;
			break;
		case SND_SOC_DAIFMT_RIGHT_J:    /* Right Justified mode */
			reg_val &= ~SUN6I_IISCTL_PCM;
			reg_val1 |= SUN6I_IISFAT0_FMT_RGT;
			break;
		case SND_SOC_DAIFMT_LEFT_J:     /* Left Justified mode */
			reg_val &= ~SUN6I_IISCTL_PCM;
			reg_val1 |= SUN6I_IISFAT0_FMT_LFT;
			break;
		case SND_SOC_DAIFMT_DSP_A:      /* L data msb after FRM LRC */
			reg_val |= SUN6I_IISCTL_PCM;
			reg_val1 &= ~SUN6I_IISFAT0_LRCP;
			break;
		case SND_SOC_DAIFMT_DSP_B:      /* L data msb during FRM LRC */
			reg_val |= SUN6I_IISCTL_PCM;
			reg_val1 |= SUN6I_IISFAT0_LRCP;
			break;
		default:
			return -EINVAL;
	}
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
	writel(reg_val1, sun6i_iis.regs + SUN6I_IISFAT0);
	
	/* DAI signal inversions */
	reg_val1 = readl(sun6i_iis.regs + SUN6I_IISFAT0);
	switch(fmt & SND_SOC_DAIFMT_INV_MASK){
		case SND_SOC_DAIFMT_NB_NF:     /* normal bit clock + frame */
			reg_val1 &= ~SUN6I_IISFAT0_LRCP;
			reg_val1 &= ~SUN6I_IISFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_NB_IF:     /* normal bclk + inv frm */
			reg_val1 |= SUN6I_IISFAT0_LRCP;
			reg_val1 &= ~SUN6I_IISFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_NF:     /* invert bclk + nor frm */
			reg_val1 &= ~SUN6I_IISFAT0_LRCP;
			reg_val1 |= SUN6I_IISFAT0_BCP;
			break;
		case SND_SOC_DAIFMT_IB_IF:     /* invert bclk + frm */
			reg_val1 |= SUN6I_IISFAT0_LRCP;
			reg_val1 |= SUN6I_IISFAT0_BCP;
			break;
	}
	writel(reg_val1, sun6i_iis.regs + SUN6I_IISFAT0);
	
	/* set FIFO control register */
	reg_val = 1 & 0x3;
	reg_val |= (1 & 0x1)<<2;
	reg_val |= SUN6I_IISFCTL_RXTL(0x1f);				/*RX FIFO trigger level*/
	reg_val |= SUN6I_IISFCTL_TXTL(0x40);				/*TX FIFO empty trigger level*/
	writel(reg_val, sun6i_iis.regs + SUN6I_IISFCTL);
	return 0;
}

static int sun6i_i2s_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun6i_dma_params *dma_data;
	
	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun6i_i2s_pcm_stereo_out;
	else
		dma_data = &sun6i_i2s_pcm_stereo_in;
	
	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);
	return 0;
}

static int sun6i_i2s_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_i2s(substream, 1);
			} else {
				sun6i_snd_txctrl_i2s(substream, 1);
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_i2s(substream, 0);
			} else {
			  sun6i_snd_txctrl_i2s(substream, 0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}

static int sun6i_i2s_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id, 
                                 unsigned int freq, int dir)
{
	if (clk_set_rate(i2s_pll2clk, freq)) {
		printk("try to set the i2s_pll2clk failed!\n");
	}

	return 0;
}
static int sun6i_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	u32 reg_val;
	u32 fs;
	u32 mclk;
	u32 mclk_div = 0;
	u32 bclk_div = 0;

	fs = div;
	mclk = over_sample_rate;
	
#ifdef I2S_COMMUNICATION
	wss = word_select_size;
#endif

#ifdef I2S_COMMUNICATION
	/*mclk div caculate*/
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
	
	/*bclk div caculate*/
	bclk_div = mclk/(2*wss);
#else
	mclk_div = 2;
	bclk_div = 6;
#endif

	/*calculate MCLK Divide Ratio*/
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

	/*calculate BCLK Divide Ratio*/
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
	
	/*set mclk and bclk dividor register*/
	reg_val = mclk_div;
	reg_val |= (bclk_div<<4);
	reg_val |= (0x1<<7);
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCLKD);

	/* word select size */
	reg_val = readl(sun6i_iis.regs + SUN6I_IISFAT0);
	sun6i_iis.ws_size = word_select_size;
	reg_val &= ~SUN6I_IISFAT0_WSS_32BCLK;
	if(sun6i_iis.ws_size == 16)
		reg_val |= SUN6I_IISFAT0_WSS_16BCLK;
	else if(sun6i_iis.ws_size == 20) 
		reg_val |= SUN6I_IISFAT0_WSS_20BCLK;
	else if(sun6i_iis.ws_size == 24)
		reg_val |= SUN6I_IISFAT0_WSS_24BCLK;
	else
		reg_val |= SUN6I_IISFAT0_WSS_32BCLK;
	
	sun6i_iis.samp_res = sample_resolution;
	reg_val &= ~SUN6I_IISFAT0_SR_RVD;
	if(sun6i_iis.samp_res == 16)
		reg_val |= SUN6I_IISFAT0_SR_16BIT;
	else if(sun6i_iis.samp_res == 20) 
		reg_val |= SUN6I_IISFAT0_SR_20BIT;
	else
		reg_val |= SUN6I_IISFAT0_SR_24BIT;
	writel(reg_val, sun6i_iis.regs + SUN6I_IISFAT0);

	/* PCM REGISTER setup */
	sun6i_iis.pcm_txtype = tx_data_mode;
	sun6i_iis.pcm_rxtype = rx_data_mode;
	reg_val = sun6i_iis.pcm_txtype&0x3;
	reg_val |= sun6i_iis.pcm_rxtype<<2;

	sun6i_iis.pcm_sync_type = frame_width;
	if(sun6i_iis.pcm_sync_type)
		reg_val |= SUN6I_IISFAT1_SSYNC;	

	sun6i_iis.pcm_sw = slot_width;
	if(sun6i_iis.pcm_sw == 16)
		reg_val |= SUN6I_IISFAT1_SW;

	sun6i_iis.pcm_start_slot = slot_index;
	reg_val |=(sun6i_iis.pcm_start_slot & 0x3)<<6;		

	sun6i_iis.pcm_lsb_first = msb_lsb_first;
	reg_val |= sun6i_iis.pcm_lsb_first<<9;			

	sun6i_iis.pcm_sync_period = pcm_sync_period;
	if(sun6i_iis.pcm_sync_period == 256)
		reg_val |= 0x4<<12;
	else if (sun6i_iis.pcm_sync_period == 128)
		reg_val |= 0x3<<12;
	else if (sun6i_iis.pcm_sync_period == 64)
		reg_val |= 0x2<<12;
	else if (sun6i_iis.pcm_sync_period == 32)
		reg_val |= 0x1<<12;
	writel(reg_val, sun6i_iis.regs + SUN6I_IISFAT1);
	
	return 0;
}

static int sun6i_i2s_dai_probe(struct snd_soc_dai *dai)
{			
	return 0;
}
static int sun6i_i2s_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static void iisregsave(void)
{
	regsave[0] = readl(sun6i_iis.regs + SUN6I_IISCTL);
	regsave[1] = readl(sun6i_iis.regs + SUN6I_IISFAT0);
	regsave[2] = readl(sun6i_iis.regs + SUN6I_IISFAT1);
	regsave[3] = readl(sun6i_iis.regs + SUN6I_IISFCTL) | (0x3<<24);
	regsave[4] = readl(sun6i_iis.regs + SUN6I_IISINT);
	regsave[5] = readl(sun6i_iis.regs + SUN6I_IISCLKD);
	regsave[6] = readl(sun6i_iis.regs + SUN6I_TXCHSEL);
	regsave[7] = readl(sun6i_iis.regs + SUN6I_TXCHMAP);
}

static void iisregrestore(void)
{
	writel(regsave[0], sun6i_iis.regs + SUN6I_IISCTL);
	writel(regsave[1], sun6i_iis.regs + SUN6I_IISFAT0);
	writel(regsave[2], sun6i_iis.regs + SUN6I_IISFAT1);
	writel(regsave[3], sun6i_iis.regs + SUN6I_IISFCTL);
	writel(regsave[4], sun6i_iis.regs + SUN6I_IISINT);
	writel(regsave[5], sun6i_iis.regs + SUN6I_IISCLKD);
	writel(regsave[6], sun6i_iis.regs + SUN6I_TXCHSEL);
	writel(regsave[7], sun6i_iis.regs + SUN6I_TXCHMAP);
}

static int sun6i_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	printk("[IIS]Entered %s\n", __func__);

	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	reg_val &= ~SUN6I_IISCTL_GEN;
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);

	iisregsave();
	if ((NULL == i2s_moduleclk) ||(IS_ERR(i2s_moduleclk))) {
		printk("i2s_moduleclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		/*release the module clock*/
		clk_disable(i2s_moduleclk);
	}
	if ((NULL == i2s_apbclk) ||(IS_ERR(i2s_apbclk))) {
		printk("i2s_apbclk handle is invalid, just return\n");
		return -EFAULT;
	} else {
		clk_disable(i2s_apbclk);
	}
	return 0;
}
static int sun6i_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	u32 reg_val;
	printk("[IIS]Entered %s\n", __func__);

	/*release the module clock*/
	if (clk_enable(i2s_apbclk)) {
		printk("try to enable i2s_apbclk output failed!\n");
	}

	/*release the module clock*/
	if (clk_enable(i2s_moduleclk)) {
		printk("try to enable i2s_moduleclk output failed!\n");
	}

	iisregrestore();
	
	/*Global Enable Digital Audio Interface*/
	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	reg_val |= SUN6I_IISCTL_GEN;
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
	
	return 0;
}

#define SUN6I_I2S_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun6i_iis_dai_ops = {
	.trigger 	= sun6i_i2s_trigger,
	.hw_params 	= sun6i_i2s_hw_params,
	.set_fmt 	= sun6i_i2s_set_fmt,
	.set_clkdiv = sun6i_i2s_set_clkdiv,
	.set_sysclk = sun6i_i2s_set_sysclk, 
};

static struct snd_soc_dai_driver sun6i_iis_dai = {	
	.probe 		= sun6i_i2s_dai_probe,
	.suspend 	= sun6i_i2s_suspend,
	.resume 	= sun6i_i2s_resume,
	.remove 	= sun6i_i2s_dai_remove,
	.playback 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture 	= {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
	.ops 		= &sun6i_iis_dai_ops,	
};		

static int __devinit sun6i_i2s_dev_probe(struct platform_device *pdev)
{
	int reg_val = 0;
	int ret;
	
	sun6i_iis.regs = ioremap(SUN6I_IISBASE, 0x100);
	if (sun6i_iis.regs == NULL)
		return -ENXIO;

	/*i2s apbclk*/
	i2s_apbclk = clk_get(NULL, CLK_APB_I2S0);
	if ((!i2s_apbclk)||(IS_ERR(i2s_apbclk))) {
		printk("try to get i2s_apbclk failed\n");
	}
	if (clk_enable(i2s_apbclk)) {
		printk("i2s_apbclk failed! line = %d\n", __LINE__);
	}
	
	i2s_pllx8 = clk_get(NULL, CLK_SYS_PLL2X8);
	if ((!i2s_pllx8)||(IS_ERR(i2s_pllx8))) {
		printk("try to get i2s_pllx8 failed\n");
	}

	/*i2s pll2clk*/
	i2s_pll2clk = clk_get(NULL, CLK_SYS_PLL2);
	if ((!i2s_pll2clk)||(IS_ERR(i2s_pll2clk))) {
		printk("try to get i2s_pll2clk failed\n");
	}
	
	/*i2s module clk*/
	i2s_moduleclk = clk_get(NULL, CLK_MOD_I2S0);
	if ((!i2s_moduleclk)||(IS_ERR(i2s_moduleclk))) {
		printk("try to get i2s_moduleclk failed\n");
	}
	
	if(clk_set_parent(i2s_moduleclk, i2s_pll2clk)){
		printk("try to set parent of i2s_moduleclk to i2s_pll2ck failed! line = %d\n",__LINE__);
	}
	
	if(clk_set_rate(i2s_moduleclk, 24576000/8)){
		printk("set i2s_moduleclk clock freq to 24576000 failed! line = %d\n", __LINE__);
	}
	
	if (clk_enable(i2s_moduleclk)) {
		printk("open i2s_moduleclk failed! line = %d\n", __LINE__);
	}
	
	reg_val = readl(sun6i_iis.regs + SUN6I_IISCTL);
	reg_val |= SUN6I_IISCTL_GEN;
	writel(reg_val, sun6i_iis.regs + SUN6I_IISCTL);
	
	ret = snd_soc_register_dai(&pdev->dev, &sun6i_iis_dai);	
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
	}

	return 0;
}

static int __devexit sun6i_i2s_dev_remove(struct platform_device *pdev)
{
	if (i2s_used) {
		i2s_used = 0;

		if ((NULL == i2s_moduleclk) ||(IS_ERR(i2s_moduleclk))) {
			printk("i2s_moduleclk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release the module clock*/
			clk_disable(i2s_moduleclk);
		}
		if ((NULL == i2s_pllx8) ||(IS_ERR(i2s_pllx8))) {
			printk("i2s_pllx8 handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*reease pllx8clk*/
			clk_put(i2s_pllx8);
		}
		if ((NULL == i2s_pll2clk) ||(IS_ERR(i2s_pll2clk))) {
			printk("i2s_pll2clk handle is invalid, just return\n");
			return -EFAULT;
		} else {
			/*release pll2clk*/
			clk_put(i2s_pll2clk);
		}
		if ((NULL == i2s_apbclk) ||(IS_ERR(i2s_apbclk))) {
			printk("i2s_apbclk handle is invalid, just return\n");
			return -EFAULT;
		} else {		
			/*release apbclk*/
			clk_put(i2s_apbclk);
		}
		snd_soc_unregister_dai(&pdev->dev);
		platform_set_drvdata(pdev, NULL);
	}
	return 0;
}

/*data relating*/
static struct platform_device sun6i_i2s_device = {
	.name = "sun6i-i2s",
};

/*method relating*/
static struct platform_driver sun6i_i2s_driver = {
	.probe = sun6i_i2s_dev_probe,
	.remove = __devexit_p(sun6i_i2s_dev_remove),
	.driver = {
		.name = "sun6i-i2s",
		.owner = THIS_MODULE,
	},
};

static int __init sun6i_i2s_init(void)
{	
	int err = 0;
	int cnt = 0;
	int i 	= 0;
	script_item_u val;
	script_item_u *list = NULL;
	script_item_value_type_e  type;

	type = script_get_item("i2s_para", "i2s_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[I2S] type err!\n");
    }

	i2s_used = val.val;
 	if (i2s_used) {
		/* get gpio list */
		cnt = script_get_pio_list("i2s_para", &list);
		if (0 == cnt) {
			printk("get i2s_para gpio list failed\n");
			return -EFAULT;
		}
	/* req gpio */
	for (i = 0; i < cnt; i++) {
		if (0 != gpio_request(list[i].gpio.gpio, NULL)) {
			printk("[i2s] request some gpio fail\n");
			goto end;
		}
	}
	/* config gpio list */
	if (0 != sw_gpio_setall_range(&list[0].gpio, cnt)) {
		printk("sw_gpio_setall_range failed\n");
	}

	if((err = platform_device_register(&sun6i_i2s_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sun6i_i2s_driver)) < 0)
			return err;	
	} else {
        printk("[I2S]sun6i-i2s cannot find any using configuration for controllers, return directly!\n");
        return 0;
    }

end:
	/* release gpio */
	while(i--)
		gpio_free(list[i].gpio.gpio);

	return 0;
}
module_init(sun6i_i2s_init);

static void __exit sun6i_i2s_exit(void)
{	
	platform_driver_unregister(&sun6i_i2s_driver);
}
module_exit(sun6i_i2s_exit);

/* Module information */
MODULE_AUTHOR("REUUIMLLA");
MODULE_DESCRIPTION("sun6i I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun6i-i2s");


/*
 * sound\soc\sun6i\sun6i-codec.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#define DEBUG
#ifndef CONFIG_PM
#define CONFIG_PM
#endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/dma.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#include <asm/mach-types.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include "sun6i-codec.h"

struct clk *codec_apbclk,*codec_pll2clk,*codec_moduleclk;

static bool play_buffdone_flag = false;
static bool capture_buffdone_flag = false;
static unsigned int capture_dmadst = 0;
static unsigned int play_dmasrc = 0;

struct sun6i_codec {
	long samplerate;
	struct snd_card *card;
	struct snd_pcm *pcm;
};

/*------------- Structure/enum declaration ------------------- */
typedef struct codec_board_info {
	struct device	*dev;	     		/* parent device */
	struct resource	*codec_base_res;   /* resources found */
	struct resource	*codec_base_req;   /* resources found */

	spinlock_t	lock;
} codec_board_info_t;

static struct sun6i_pcm_dma_params sun6i_codec_pcm_stereo_play = {
	.name		= "audio_play",
	.dma_addr	= CODEC_BASSADDRESS + SUN6I_DAC_TXDATA,//send data address
};

static struct sun6i_pcm_dma_params sun6i_codec_pcm_stereo_capture = {
	.name   	= "audio_capture",
	.dma_addr	= CODEC_BASSADDRESS + SUN6I_ADC_RXDATA,//accept data address
};

struct sun6i_playback_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t   dma_start;
	dma_addr_t   dma_pos;
	dma_addr_t	 dma_end;
	dm_hdl_t	dma_hdl;
	struct dma_cb_t play_done_cb;
	struct sun6i_pcm_dma_params	*params;
};

struct sun6i_capture_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t   dma_start;
	dma_addr_t   dma_pos;
	dma_addr_t	 dma_end;
	dm_hdl_t	dma_hdl;
	struct dma_cb_t capture_done_cb;
	struct sun6i_pcm_dma_params	*params;
};

/*播放设备硬件定义*/
static struct snd_pcm_hardware sun6i_pcm_playback_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 |SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,//最大的缓冲区大小
	.period_bytes_min	= 1024*4,//最小周期大小
	.period_bytes_max	= 1024*32,//最大周期大小
	.periods_min		= 4,//最小周期数
	.periods_max		= 8,//最大周期数
	.fifo_size	     	= 32,//fifo字节数
};

/*录音设备硬件定义*/
static struct snd_pcm_hardware sun6i_pcm_capture_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |SNDRV_PCM_RATE_192000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,//最大的缓冲区大小
	.period_bytes_min	= 1024*4,//最小周期大小
	.period_bytes_max	= 1024*32,//最大周期大小
	.periods_min		= 4,//最小周期数
	.periods_max		= 8,//最大周期数
	.fifo_size	     	= 32,//fifo字节数
};

static unsigned int rates[] = {
	8000,11025,12000,16000,
	22050,24000,24000,32000,
	44100,48000,96000,192000
};

static struct snd_pcm_hw_constraint_list hw_constraints_rates = {
	.count	= ARRAY_SIZE(rates),
	.list	= rates,
	.mask	= 0,
};

/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
int codec_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	int change;
	unsigned int old, new;

	old	=	codec_rdreg(reg);
	new	=	(old & ~mask) | value;
	change = old != new;

	if (change){
		codec_wrreg(reg,new);
	}

	return change;
}

/**
*	snd_codec_info_volsw	-	single	mixer	info	callback
*	@kcontrol:	mixer control
*	@uinfo:	control	element	information
*	Callback to provide information about a single mixer control
*
* 	info()函数用于获得该control的详细信息，该函数必须填充传递给它的第二个参数snd_ctl_elem_info结构体
*
*	Returns 0 for success
*/
int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo)
{
	struct	codec_mixer_control *mc	= (struct codec_mixer_control*)kcontrol->private_value;
	int	max	=	mc->max;
	unsigned int shift  = mc->shift;
	unsigned int rshift = mc->rshift;

	if(max	== 1)
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;//the info of type
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;

	uinfo->count = shift ==	rshift	?	1:	2;	//the info of elem count
	uinfo->value.integer.min = 0;				//the info of min value
	uinfo->value.integer.max = max;				//the info of max value
	return	0;
}

/**
*	snd_codec_get_volsw	-	single	mixer	get	callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	Callback to get the value of a single mixer control
*	get()函数用于得到control的目前值并返回用户空间
*	return 0 for success.
*/
int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int	max = mc->max;
	/*fls(7) = 3,fls(1)=1,fls(0)=0,fls(15)=4,fls(3)=2,fls(23)=5*/
	unsigned int mask = (1 << fls(max)) -1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;

	ucontrol->value.integer.value[0] =
		(codec_rdreg(reg)>>	shift) & mask;
	if(shift != rshift)
		ucontrol->value.integer.value[1] =
			(codec_rdreg(reg) >> rshift) & mask;

	/*将获得的值写入snd_ctl_elem_value*/
	if(invert){
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if(shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
		}

		return 0;
}

/**
*	snd_codec_put_volsw	-	single	mixer put callback
*	@kcontrol:	mixer	control
*	@ucontrol:	control	element	information
*
*	put()用于从用户空间写入值，如果值被改变，该函数返回1，否则返回0.
*	Callback to put the value of a single mixer control
*
* return 0 for success.
*/
int snd_codec_put_volsw(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1<<fls(max))-1;
	unsigned int invert = mc->invert;
	unsigned int	val, val2, val_mask;

	val = (ucontrol->value.integer.value[0] & mask);
	if(invert)
		val = max - val;
	val <<= shift;
	val_mask = mask << shift;
	if(shift != rshift){
		val2	= (ucontrol->value.integer.value[1] & mask);
		if(invert)
			val2	=	max	- val2;
		val_mask |= mask <<rshift;
		val |= val2 <<rshift;
	}

	return codec_wrreg_bits(reg,val_mask,val);
}

int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}

int codec_rd_control(u32 reg, u32 bit, u32 *val)
{
	return 0;
}

/**
*	codec_reset - reset the codec
* @codec	SoC Audio Codec
* Reset the codec, set the register of codec default value
* Return 0 for success
*/
static  int codec_init(void)
{
	/*mute l_pa and r_pa.耳机，听筒，喇叭默认已经是关闭状态，初始化的时候，不需要重新关闭*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	/*enable pa*/
	codec_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x1);
	/*when TX FIFO available room less than or equal N,
	* DRQ Requeest will be de-asserted.
	*/
	codec_wr_control(SUN6I_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
	/*set HPVOL volume*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x6, VOLUME, 0x3b);
	return 0;
}

static int codec_play_open(struct snd_pcm_substream *substream)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	/*enable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC, 0x1, DAC_EN, 0x1);

	/*set TX FIFO send drq level*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x4, TX_TRI_LEVEL, 0xf);
	/*set TX FIFO MODE*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);

	//send last sample when dac fifo under run
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, LAST_SE, 0x0);

	/*enable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x1);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x1);

	/*set the default output is HPOUTL/R*/
	codec_wr_control(SUN6I_PA_CTRL, 0x1, LTRNMUTE, 0x1);
	codec_wr_control(SUN6I_PA_CTRL, 0x1, RTLNMUTE, 0x1);
	return 0;
}

static int codec_capture_open(void)
{
	 /*mic1 gain 30dB*/
	 codec_wr_control(SUN6I_MIC_CTRL, 0x7,MIC1BOOST,0x3);//30db
	 /*enable Master microphone bias*/
	 codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x1);
	 /*enable mic1 pa*/
	 codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x1);

	 /*enable Right MIC1 Boost stage*/
	 codec_wr_control(SUN6I_ADC_ACTL, 0x7f, RADCMIXMUTE, 0x40);//移到外部控制
	 /*enable Left MIC1 Boost stage*/
	 codec_wr_control(SUN6I_ADC_ACTL, 0x7f, LADCMIXMUTE, 0x40);//移到外部控制
	 /*enable adc_r adc_l analog*/
	 codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x1);////移到外部控制
	 codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x1);////移到外部控制
	 /*set RX FIFO mode*/
	 codec_wr_control(SUN6I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	 /*set RX FIFO rec drq level*/
	 codec_wr_control(SUN6I_ADC_FIFOC, 0xf, RX_TRI_LEVEL, 0x7);
	 /*enable adc digital part*/
	 codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);

	 return 0;
}

static int codec_play_start(void)
{
	/*DAC FIFO Flush,Write '1' to flush TX FIFO, self clear to '0'*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
	/*enable dac drq*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_DRQ, 0x1);
	return 0;
}

static int codec_play_stop(void)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);

	/*disable dac drq*/
	codec_wr_control(SUN6I_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);

	/*disable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);

	/*disable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC ,  0x1, DAC_EN, 0x0);	// it will cause noise

	return 0;
}

static int codec_capture_start(void)
{
	/*flush RX FIFO*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
	/*enable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	return 0;
}

static int codec_capture_stop(void)
{
	/*disable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x0);
	/*disable adc drq*/
	codec_wr_control(SUN6I_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
	/*disable mic1 pa*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);//移到外部控制
	/*disable Master microphone bias*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);//移到外部控制
	/*disable adc_r adc_l analog*/
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, ADCREN, 0x0);//移到外部控制
	codec_wr_control(SUN6I_ADC_ACTL, 0x1, ADCLEN, 0x0);//移到外部控制

	return 0;
}

static int codec_dev_free(struct snd_device *device)
{
	return 0;
};

/*	对sun6i-codec.c各寄存器的各种设定，或读取。主要实现函数有三个.
* 	.info = snd_codec_info_volsw, .get = snd_codec_get_volsw,\.put = snd_codec_put_volsw,
*/
static const struct snd_kcontrol_new codec_snd_controls[] = {
	/*SUN6I_DAC_ACTL = 0x10,PAVOL*/
	CODEC_SINGLE("Master Playback Volume", SUN6I_DAC_ACTL,0,0x3f,0),
	/*total output switch PAMUTE, if set this bit to 0, the voice is mute*/
	CODEC_SINGLE("Playback LPAMUTE SWITCH", SUN6I_DAC_ACTL,6,0x1,0),
	CODEC_SINGLE("Playback RPAMUTE SWITCH", SUN6I_DAC_ACTL,7,0x1,0),
	CODEC_SINGLE("Left Headphone PA input src select", SUN6I_DAC_ACTL,8,0x1,0),
	CODEC_SINGLE("Right Headphone PA input src select", SUN6I_DAC_ACTL,9,0x1,0),
	CODEC_SINGLE("Left output mixer mute control", SUN6I_DAC_ACTL,10,0x1,0),
	CODEC_SINGLE("Right output mixer mute control", SUN6I_DAC_ACTL,17,0x1,0),
	CODEC_SINGLE("Left analog output mixer en", SUN6I_DAC_ACTL,28,0x1,0),
	CODEC_SINGLE("Right analog output mixer en", SUN6I_DAC_ACTL,29,0x1,0),
	CODEC_SINGLE("Inter DAC analog left channel en", SUN6I_DAC_ACTL,30,0x1,0),
	CODEC_SINGLE("Inter DAC analog right channel en", SUN6I_DAC_ACTL,31,0x1,0),
	/*SUN6I_PA_CTRL = 0x24*/
	CODEC_SINGLE("r_and_l Headphone Power amplifier en", SUN6I_PA_CTRL,29,0x3,0),
	CODEC_SINGLE("HPCOM output protection en", SUN6I_PA_CTRL,28,0x1,0),
	CODEC_SINGLE("L_to_R Headphone apmplifier output mute", SUN6I_PA_CTRL,25,0x1,0),
	CODEC_SINGLE("R_to_L Headphone apmplifier output mute", SUN6I_PA_CTRL,24,0x1,0),
	CODEC_SINGLE("MIC1_G boost stage output mixer control", SUN6I_PA_CTRL,15,0x7,0),
	CODEC_SINGLE("MIC2_G boost stage output mixer control", SUN6I_PA_CTRL,12,0x7,0),
	CODEC_SINGLE("LINEIN_G boost stage output mixer control", SUN6I_PA_CTRL,9,0x7,0),
	CODEC_SINGLE("PHONE_G boost stage output mixer control", SUN6I_PA_CTRL,6,0x7,0),
	CODEC_SINGLE("PHONE_PG boost stage output mixer control", SUN6I_PA_CTRL,3,0x7,0),
	CODEC_SINGLE("PHONE_NG boost stage output mixer control", SUN6I_PA_CTRL,0,0x7,0),

	/*SUN6I_MIC_CTRL = 0x28*/
	CODEC_SINGLE("Headset microphone bias enable", SUN6I_MIC_CTRL,31,0x1,0),
	CODEC_SINGLE("Master microphone bias enable", SUN6I_MIC_CTRL,30,0x1,0),
	CODEC_SINGLE("Headset MIC bias_cur_sen and ADC enable", SUN6I_MIC_CTRL,29,0x1,0),
	CODEC_SINGLE("MIC1 boost AMP enable", SUN6I_MIC_CTRL,28,0x1,0),
	CODEC_SINGLE("MIC1 boost AMP gain control", SUN6I_MIC_CTRL,25,0x7,0),
	CODEC_SINGLE("MIC2 boost AMP enable", SUN6I_MIC_CTRL,24,0x1,0),
	CODEC_SINGLE("MIC2 boost AMP gain control", SUN6I_MIC_CTRL,21,0x7,0),
	CODEC_SINGLE("MIC2 source select", SUN6I_MIC_CTRL,20,0x1,0),
	CODEC_SINGLE("Lineout left enable", SUN6I_MIC_CTRL,19,0x1,0),
	CODEC_SINGLE("Lineout right enable", SUN6I_MIC_CTRL,18,0x1,0),
	CODEC_SINGLE("Left lineout source select", SUN6I_MIC_CTRL,17,0x1,0),
	CODEC_SINGLE("Right lineout source select", SUN6I_MIC_CTRL,16,0x1,0),
	CODEC_SINGLE("Lineout volume control", SUN6I_MIC_CTRL,11,0x1f,0),
	CODEC_SINGLE("PHONEP-PHONEN pre-amp gain control", SUN6I_MIC_CTRL,8,0x7,0),
	CODEC_SINGLE("Phoneout gain control", SUN6I_MIC_CTRL,5,0x7,0),
	CODEC_SINGLE("PHONEOUT en", SUN6I_MIC_CTRL,4,0x1,0),
	CODEC_SINGLE("MIC1 boost stage to phone out mute", SUN6I_MIC_CTRL,3,0x1,0),
	CODEC_SINGLE("MIC2 boost stage to phone out mute", SUN6I_MIC_CTRL,2,0x1,0),
	CODEC_SINGLE("Right output mixer to phone out mute", SUN6I_MIC_CTRL,1,0x1,0),
	CODEC_SINGLE("Left output mixer to phone out mute", SUN6I_MIC_CTRL,1,0x1,0),

	/*SUN6I_ADC_ACTL = 0x2c*/
	CODEC_SINGLE("ADC Right channel en", SUN6I_MIC_CTRL,31,0x1,0),
	CODEC_SINGLE("ADC Left channel en", SUN6I_MIC_CTRL,30,0x1,0),
	CODEC_SINGLE("ADC Left channel en", SUN6I_MIC_CTRL,30,0x1,0),
	CODEC_SINGLE("ADC right channel input gain ctrl", SUN6I_MIC_CTRL,27,0x7,0),
	CODEC_SINGLE("ADC left channel input gain ctrl", SUN6I_MIC_CTRL,24,0x7,0),
	CODEC_SINGLE("Right ADC mixer mute ctrl", SUN6I_MIC_CTRL,7,0x7f,0),
	CODEC_SINGLE("Left ADC mixer mute ctrl", SUN6I_MIC_CTRL,0,0x7f,0),
	/*SUN6I_ADDAC_TUNE = 0x30*/
	CODEC_SINGLE("ADC dither on_off ctrl", SUN6I_ADDAC_TUNE,25,0x7f,0),

	/*SUN6I_HMIC_CTL = 0x50
	* warning:
	* the key and earphone should be check in the switch driver,
	* can't be used in this mixer control.
	* you should be careful while use the key and earphone check in the mixer control
	* it may be confilcted with the key and earphone switch driver.
	*/
	CODEC_SINGLE("Hmic_M debounce key down_up", SUN6I_HMIC_CTL,28,0xf,0),
	CODEC_SINGLE("Hmic_N debounce earphone plug in_out", SUN6I_HMIC_CTL,24,0xf,0),

	/*SUN6I_DAC_DAP_CTL = 0x60
	* warning:the DAP should be realize in a DAP driver?
	* it may be strange using the mixer control to realize the DAP function.
	*/
	CODEC_SINGLE("DAP enable", SUN6I_DAC_DAP_CTL,31,0x1,0),
	CODEC_SINGLE("DAP start control", SUN6I_DAC_DAP_CTL,30,0x1,0),
	CODEC_SINGLE("DAP state", SUN6I_DAC_DAP_CTL,29,0x1,0),
	CODEC_SINGLE("BQ enable control", SUN6I_DAC_DAP_CTL,16,0x1,0),
	CODEC_SINGLE("DRC enable control", SUN6I_DAC_DAP_CTL,15,0x1,0),
	CODEC_SINGLE("HPF enable control", SUN6I_DAC_DAP_CTL,14,0x1,0),
	CODEC_SINGLE("DE function control", SUN6I_DAC_DAP_CTL,12,0x3,0),
	CODEC_SINGLE("Ram address", SUN6I_DAC_DAP_CTL,0,0x7f,0),

	/*SUN6I_DAC_DAP_VOL = 0x64*/
	CODEC_SINGLE("DAP DAC left chan soft mute ctrl", SUN6I_DAC_DAP_VOL,30,0x1,0),
	CODEC_SINGLE("DAP DAC right chan soft mute ctrl", SUN6I_DAC_DAP_VOL,29,0x1,0),
	CODEC_SINGLE("DAP DAC master soft mute ctrl", SUN6I_DAC_DAP_VOL,28,0x1,0),
	CODEC_SINGLE("DAP DAC vol skew time ctrl", SUN6I_DAC_DAP_VOL,24,0x3,0),
	CODEC_SINGLE("DAP DAC master volume", SUN6I_DAC_DAP_VOL,16,0xff,0),
	CODEC_SINGLE("DAP DAC left chan volume", SUN6I_DAC_DAP_VOL,8,0xff,0),
	CODEC_SINGLE("DAP DAC right chan volume", SUN6I_DAC_DAP_VOL,0,0xff,0),

	/*SUN6I_ADC_DAP_CTL = 0x70*/
	CODEC_SINGLE("DAP for ADC en", SUN6I_ADC_DAP_CTL,31,0x1,0),
	CODEC_SINGLE("DAP for ADC start up", SUN6I_ADC_DAP_CTL,30,0x1,0),
	CODEC_SINGLE("DAP left AGC saturation flag", SUN6I_ADC_DAP_CTL,21,0x1,0),
	CODEC_SINGLE("DAP left AGC noise-threshold flag", SUN6I_ADC_DAP_CTL,20,0x1,0),
	CODEC_SINGLE("DAP left gain applied by AGC", SUN6I_ADC_DAP_CTL,12,0xff,0),
	CODEC_SINGLE("DAP right AGC saturation flag", SUN6I_ADC_DAP_CTL,9,0x1,0),
	CODEC_SINGLE("DAP right AGC noise-threshold flag", SUN6I_ADC_DAP_CTL,8,0x1,0),
	CODEC_SINGLE("DAP right gain applied by AGC", SUN6I_ADC_DAP_CTL,0,0xff,0),

	/*SUN6I_ADC_DAP_VOL = 0x74*/
	CODEC_SINGLE("DAP ADC left chan vol mute", SUN6I_ADC_DAP_VOL,18,0x1,0),
	CODEC_SINGLE("DAP ADC right chan vol mute", SUN6I_ADC_DAP_VOL,17,0x1,0),
	CODEC_SINGLE("DAP ADC volume skew mute", SUN6I_ADC_DAP_VOL,16,0x1,0),
	CODEC_SINGLE("DAP ADC left chan vol set", SUN6I_ADC_DAP_VOL,8,0x3f,0),
	CODEC_SINGLE("DAP ADC right chan vol set", SUN6I_ADC_DAP_VOL,0,0x3f,0),

	/*SUN6I_ADC_DAP_LCTL = 0x78*/
	CODEC_SINGLE("DAP ADC Left chan noise-threshold set", SUN6I_ADC_DAP_VOL,16,0xff,0),
	CODEC_SINGLE("DAP Left AGC en", SUN6I_ADC_DAP_VOL,14,0x1,0),
	CODEC_SINGLE("DAP Left HPF en", SUN6I_ADC_DAP_VOL,13,0x1,0),
	CODEC_SINGLE("DAP Left noise-detect en", SUN6I_ADC_DAP_VOL,12,0x1,0),
	CODEC_SINGLE("DAP Left hysteresis setting", SUN6I_ADC_DAP_VOL,8,0x3,0),
	CODEC_SINGLE("DAP Left noise-debounce time", SUN6I_ADC_DAP_VOL,4,0xf,0),
	CODEC_SINGLE("DAP Left signal-debounce time", SUN6I_ADC_DAP_VOL,0,0xf,0),

	/*SUN6I_ADC_DAP_RCTL = 0x7c*/
	CODEC_SINGLE("DAP ADC right chan noise-threshold set", SUN6I_ADC_DAP_RCTL,0,0xff,0),
	CODEC_SINGLE("DAP Right AGC en", SUN6I_ADC_DAP_VOL,14,0x1,0),
	CODEC_SINGLE("DAP Right HPF en", SUN6I_ADC_DAP_VOL,13,0x1,0),
	CODEC_SINGLE("DAP Right noise-detect en", SUN6I_ADC_DAP_VOL,12,0x1,0),
	CODEC_SINGLE("DAP Right hysteresis setting", SUN6I_ADC_DAP_VOL,8,0x3,0),
	CODEC_SINGLE("DAP Right noise-debounce time", SUN6I_ADC_DAP_VOL,4,0xf,0),
	CODEC_SINGLE("DAP Right signal-debounce time", SUN6I_ADC_DAP_VOL,0,0xf,0),
};

int __init snd_chip_codec_mixer_new(struct snd_card *card)
{
	unsigned char *clnt = "codec";
	int idx, err;

	static struct snd_device_ops ops = {
		.dev_free	=	codec_dev_free,
	};

	for (idx = 0; idx < ARRAY_SIZE(codec_snd_controls); idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&codec_snd_controls[idx],clnt))) < 0) {
			return err;
		}
	}

	if ((err = snd_device_new(card, SNDRV_DEV_CODEC, clnt, &ops)) < 0) {
		return err;
	}

	strcpy(card->mixername, "codec Mixer");

	return 0;
}

static void sun6i_pcm_enqueue(struct snd_pcm_substream *substream)
{
	int play_ret = 0, capture_ret = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	dma_addr_t play_pos = 0, capture_pos = 0;
	unsigned long play_len = 0, capture_len = 0;
	unsigned int play_limit = 0, capture_limit = 0;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		play_pos = play_prtd->dma_pos;
		play_len = play_prtd->dma_period;
		play_limit = play_prtd->dma_limit;
		while (play_prtd->dma_loaded < play_limit) {
			if ((play_pos + play_len) > play_prtd->dma_end) {
				play_len  = play_prtd->dma_end - play_pos;
			}
			if (play_buffdone_flag) {
				play_ret = sw_dma_enqueue(play_prtd->dma_hdl, play_pos, play_prtd->params->dma_addr, play_len, ENQUE_PHASE_QD);
			} else {
				play_ret = sw_dma_enqueue(play_prtd->dma_hdl, play_pos, play_prtd->params->dma_addr, play_len, ENQUE_PHASE_NORMAL);
			}

			if (play_ret == 0) {
				play_prtd->dma_loaded++;
				play_pos += play_prtd->dma_period;
				if(play_pos >= play_prtd->dma_end)
					play_pos = play_prtd->dma_start;
			} else {
				break;
			}
		}
		play_prtd->dma_pos = play_pos;
	} else {
		capture_prtd = substream->runtime->private_data;
		capture_pos = capture_prtd->dma_pos;
		capture_len = capture_prtd->dma_period;
		capture_limit = capture_prtd->dma_limit;
		while (capture_prtd->dma_loaded < capture_limit) {
			if ((capture_pos + capture_len) > capture_prtd->dma_end) {
				capture_len  = capture_prtd->dma_end - capture_pos;
			}
			if (capture_buffdone_flag) {
				capture_ret = sw_dma_enqueue(capture_prtd->dma_hdl, capture_prtd->params->dma_addr, capture_pos, capture_len, ENQUE_PHASE_QD);
			} else {
				capture_ret = sw_dma_enqueue(capture_prtd->dma_hdl, capture_prtd->params->dma_addr, capture_pos, capture_len, ENQUE_PHASE_NORMAL);
			}

			if (capture_ret == 0) {
			capture_prtd->dma_loaded++;
			capture_pos += capture_prtd->dma_period;
			if (capture_pos >= capture_prtd->dma_end)
				capture_pos = capture_prtd->dma_start;
			} else {
				break;
			}
		}
		capture_prtd->dma_pos = capture_pos;
	}
}

static u32 sun6i_audio_capture_buffdone(dm_hdl_t dma_hdl, void *parg,
		                                  enum dma_cb_cause_e result)
{
	struct sun6i_capture_runtime_data *capture_prtd;
	struct snd_pcm_substream *substream = parg;

	if (result == DMA_CB_ABORT)
		return -EINVAL;

	capture_buffdone_flag = true;
	capture_prtd = substream->runtime->private_data;
	if (substream) {
		snd_pcm_period_elapsed(substream);
	}

	spin_lock(&capture_prtd->lock);
	{
		capture_prtd->dma_loaded--;

		if(true == capture_buffdone_flag) {
			sun6i_pcm_enqueue(substream);
		} else {
			printk("%s err, line %d\n", __func__, __LINE__);
		}
	}
	spin_unlock(&capture_prtd->lock);

	capture_buffdone_flag = false;
	return 0;
}

static u32 sun6i_audio_play_buffdone(dm_hdl_t dma_hdl, void *parg,
		                                  enum dma_cb_cause_e result)
{
	struct sun6i_playback_runtime_data *play_prtd;
	struct snd_pcm_substream *substream = parg;

	if (result == DMA_CB_ABORT)
		return -EINVAL;

	play_buffdone_flag = true;
	play_prtd = substream->runtime->private_data;
	if (substream) {
		snd_pcm_period_elapsed(substream);
	}

	spin_lock(&play_prtd->lock);
	{
		play_prtd->dma_loaded--;
		if(true == play_buffdone_flag) {
			sun6i_pcm_enqueue(substream);
		} else {
			printk("%s err, line %d\n", __func__, __LINE__);
		}
	}
	spin_unlock(&play_prtd->lock);
	play_buffdone_flag = false;

	return 0;
}

static snd_pcm_uframes_t snd_sun6i_codec_pointer(struct snd_pcm_substream *substream)
{
	unsigned long capture_res = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_runtime *play_runtime = NULL;
    snd_pcm_uframes_t play_offset = 0;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	play_prtd = substream->runtime->private_data;
		spin_lock(&play_prtd->lock);

		if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_GET_CUR_SRC_ADDR, &play_dmasrc)) {
			printk("%s,err\n", __func__);
		}

		play_offset = bytes_to_frames(play_runtime, play_dmasrc - play_runtime->dma_addr);

		spin_unlock(&play_prtd->lock);
		if (play_offset >= play_runtime->buffer_size) {
			play_offset = 0;
		}
		return play_offset;
    } else {
	capture_prtd = substream->runtime->private_data;
	spin_lock(&capture_prtd->lock);

	if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_GET_CUR_DST_ADDR, &capture_dmadst)) {
			printk("%s,err\n", __func__);
		}

	capture_res = capture_dmadst - capture_prtd->dma_start;

	if (capture_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (capture_res == snd_pcm_lib_buffer_bytes(substream))
				capture_res = 0;
		}
		spin_unlock(&capture_prtd->lock);

		return bytes_to_frames(substream->runtime, capture_res);
    }
}

static int sun6i_codec_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{
    struct snd_pcm_runtime *play_runtime = NULL, *capture_runtime = NULL;
    struct sun6i_playback_runtime_data *play_prtd = NULL;
    struct sun6i_capture_runtime_data *capture_prtd = NULL;
    unsigned long play_totbytes = 0, capture_totbytes = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;
		play_prtd = play_runtime->private_data;
		play_totbytes = params_buffer_bytes(params);
		snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (play_prtd->params == NULL) {
			play_prtd->params = &sun6i_codec_pcm_stereo_play;
			/*
			 * requeset audio dma handle(we don't care about the channel!)
			 */
			play_prtd->dma_hdl = sw_dma_request(play_prtd->params->name, DMA_WORK_MODE_SINGLE);
			if (NULL == play_prtd->dma_hdl) {
				printk(KERN_ERR "failed to request audio_play dma handle\n");
				return -EINVAL;
			}
			/*
			* set callback
			*/
			memset(&play_prtd->play_done_cb, 0, sizeof(play_prtd->play_done_cb));
			play_prtd->play_done_cb.func = sun6i_audio_play_buffdone;
			play_prtd->play_done_cb.parg = substream;
			/*use the full buffer callback, maybe we should use the half buffer callback?*/
			if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_SET_QD_CB, (void *)&(play_prtd->play_done_cb))) {
				sw_dma_release(play_prtd->dma_hdl);
				return -EINVAL;
			}

			snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
			play_runtime->dma_bytes = play_totbytes;
			spin_lock_irq(&play_prtd->lock);
			play_prtd->dma_loaded = 0;
			play_prtd->dma_limit = play_runtime->hw.periods_min;
			play_prtd->dma_period = params_period_bytes(params);
			play_prtd->dma_start = play_runtime->dma_addr;

			play_dmasrc = play_prtd->dma_start;
			play_prtd->dma_pos = play_prtd->dma_start;
			play_prtd->dma_end = play_prtd->dma_start + play_totbytes;
			spin_unlock_irq(&play_prtd->lock);
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		capture_runtime = substream->runtime;
		capture_prtd = capture_runtime->private_data;
		capture_totbytes = params_buffer_bytes(params);
		snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (capture_prtd->params == NULL) {
			capture_prtd->params = &sun6i_codec_pcm_stereo_capture;
			/*
			 * requeset audio_capture dma handle(we don't care about the channel!)
			 */
			capture_prtd->dma_hdl = sw_dma_request(capture_prtd->params->name, DMA_WORK_MODE_SINGLE);
			if (NULL == capture_prtd->dma_hdl) {
				printk(KERN_ERR "failed to request audio_capture dma handle\n");
				return -EINVAL;
			}
			/*
			* set callback
			*/
			memset(&capture_prtd->capture_done_cb, 0, sizeof(capture_prtd->capture_done_cb));
			capture_prtd->capture_done_cb.func = sun6i_audio_capture_buffdone;
			capture_prtd->capture_done_cb.parg = substream;
			/*use the full buffer callback, maybe we should use the half buffer callback?*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_SET_QD_CB, (void *)&(capture_prtd->capture_done_cb))) {
				sw_dma_release(capture_prtd->dma_hdl);
				return -EINVAL;
			}

			snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
			capture_runtime->dma_bytes = capture_totbytes;
			spin_lock_irq(&capture_prtd->lock);
			capture_prtd->dma_loaded = 0;
			capture_prtd->dma_limit = capture_runtime->hw.periods_min;
			capture_prtd->dma_period = params_period_bytes(params);
			capture_prtd->dma_start = capture_runtime->dma_addr;

			capture_dmadst = capture_prtd->dma_start;
			capture_prtd->dma_pos = capture_prtd->dma_start;
			capture_prtd->dma_end = capture_prtd->dma_start + capture_totbytes;

			spin_unlock_irq(&capture_prtd->lock);
		}
	} else {
		return -EINVAL;
	}
	return 0;
}

static int snd_sun6i_codec_hw_free(struct snd_pcm_substream *substream)
{
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		snd_pcm_set_runtime_buffer(substream, NULL);
		if (play_prtd->params) {
			/*
			 * stop play dma transfer
			 */
			if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			/*
			*	release play dma handle
			*/
			if (0 != sw_dma_release(play_prtd->dma_hdl)) {
				return -EINVAL;
			}
			play_prtd->dma_hdl = (dm_hdl_t)NULL;
			play_prtd->params = NULL;
		}
	} else {
		capture_prtd = substream->runtime->private_data;
		snd_pcm_set_runtime_buffer(substream, NULL);
		if (capture_prtd->params) {
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			/*
			*	release capture dma handle
			*/
			if (0 != sw_dma_release(capture_prtd->dma_hdl)) {
				return -EINVAL;
			}
			capture_prtd->dma_hdl = (dm_hdl_t)NULL;
			capture_prtd->params = NULL;
		}
	}
	return 0;
}

static int snd_sun6i_codec_prepare(struct snd_pcm_substream	*substream)
{
	struct dma_config_t play_dma_config;
	struct dma_config_t capture_dma_config;
	int play_ret = 0, capture_ret = 0;
	unsigned int reg_val;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (substream->runtime->rate) {
			case 44100:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 22050:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 11025:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 48000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 96000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(7<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 192000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(6<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 32000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 24000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 16000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 12000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 8000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			default:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val |=(1<<6);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			case 2:
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
			default:
				reg_val = readl(baseaddr + SUN6I_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUN6I_DAC_FIFOC);
				break;
		}
	} else {
		switch (substream->runtime->rate) {
			case 44100:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 22050:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 11025:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 48000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 32000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 24000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 16000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 12000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			case 8000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
			default:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(7<<29);
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val |=(1<<7);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
			break;
			case 2:
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
			break;
			default:
				reg_val = readl(baseaddr + SUN6I_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUN6I_ADC_FIFOC);
			break;
		}
	}
   if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		play_prtd = substream->runtime->private_data;
		/* return if this is a bufferless transfer e.g.
		* codec <--> BT codec or GSM modem -- lg FIXME */
		if (!play_prtd->params) {
			return 0;
		}
		//open the dac channel register
		codec_play_open(substream);

		memset(&play_dma_config, 0, sizeof(play_dma_config));
		play_dma_config.xfer_type = DMAXFER_D_BHALF_S_BHALF;
		play_dma_config.address_type = DMAADDRT_D_IO_S_LN;
		play_dma_config.para = 0;
		play_dma_config.irq_spt = CHAN_IRQ_QD;
		play_dma_config.src_addr = play_prtd->dma_start;
		play_dma_config.dst_addr = play_prtd->params->dma_addr;
		play_dma_config.byte_cnt = play_prtd->dma_period;//transfer buffer counter
		play_dma_config.bconti_mode = false;
		play_dma_config.src_drq_type = DRQSRC_SDRAM;
		play_dma_config.dst_drq_type = DRQDST_AUDIO_CODEC;

		if (0 != sw_dma_config(play_prtd->dma_hdl, &play_dma_config, ENQUE_PHASE_NORMAL)) {
			return -EINVAL;
		}

		play_prtd->dma_loaded = 0;
		play_prtd->dma_pos = play_prtd->dma_start;
		/* enqueue dma buffers */
		sun6i_pcm_enqueue(substream);
		return play_ret;
	} else {
		capture_prtd = substream->runtime->private_data;
		/* return if this is a bufferless transfer e.g.
		 * codec <--> BT codec or GSM modem -- lg FIXME */
		if (!capture_prtd->params) {
			return 0;
		}
		//open the adc channel register
		codec_capture_open();

		memset(&capture_dma_config, 0, sizeof(capture_dma_config));
		capture_dma_config.xfer_type = DMAXFER_D_BHALF_S_BHALF;
		capture_dma_config.address_type = DMAADDRT_D_LN_S_IO;
		capture_dma_config.para = 0;
		capture_dma_config.irq_spt = CHAN_IRQ_QD;
		capture_dma_config.src_addr = capture_prtd->params->dma_addr;
		capture_dma_config.dst_addr = capture_prtd->dma_start;
		capture_dma_config.byte_cnt = capture_prtd->dma_period;//transfer buffer counter
		capture_dma_config.bconti_mode = false;
		capture_dma_config.src_drq_type = DRQSRC_AUDIO_CODEC;
		capture_dma_config.dst_drq_type = DRQDST_SDRAM;

		if (0 != sw_dma_config(capture_prtd->dma_hdl, &capture_dma_config, ENQUE_PHASE_NORMAL)) {
			return -EINVAL;
		}

		capture_prtd->dma_loaded = 0;
		capture_prtd->dma_pos = capture_prtd->dma_start;

		/* enqueue dma buffers */
		sun6i_pcm_enqueue(substream);
		return capture_ret;
	}
}

static int snd_sun6i_codec_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int play_ret = 0, capture_ret = 0;
	struct sun6i_playback_runtime_data *play_prtd = NULL;
	struct sun6i_capture_runtime_data *capture_prtd = NULL;
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		play_prtd = substream->runtime->private_data;
		spin_lock(&play_prtd->lock);
		switch (cmd) {
			case SNDRV_PCM_TRIGGER_START:
			case SNDRV_PCM_TRIGGER_RESUME:
			case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
				play_prtd->state |= ST_RUNNING;
				codec_play_start();
				/*
				* start dma transfer
				*/
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_START, NULL)) {
					return -EINVAL;
				}
				/*pa unmute*/
				codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x1);
				codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x1);
				break;
			case SNDRV_PCM_TRIGGER_SUSPEND:
				codec_play_stop();
				/*
				 * stop play dma transfer
				 */
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					return -EINVAL;
				}
				break;
			case SNDRV_PCM_TRIGGER_STOP:
				play_prtd->state &= ~ST_RUNNING;
				codec_play_stop();
				/*
				 * stop play dma transfer
				 */
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					return -EINVAL;
				}
				break;
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
				play_prtd->state &= ~ST_RUNNING;
				/*
				 * stop play dma transfer
				 */
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					return -EINVAL;
				}
				break;
			default:
				printk("error:%s,%d\n", __func__, __LINE__);
				play_ret = -EINVAL;
				break;
			}
		spin_unlock(&play_prtd->lock);
	}else{
		capture_prtd = substream->runtime->private_data;
		spin_lock(&capture_prtd->lock);
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			capture_prtd->state |= ST_RUNNING;
			codec_capture_start();
			/*hardware fifo delay*/
			codec_wr_control(SUN6I_ADC_FIFOC, 0x1, ADCDFEN, 0x1);
			/*
			* start dma transfer
			*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_START, NULL)) {
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
			codec_capture_stop();
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_STOP:
			capture_prtd->state &= ~ST_RUNNING;
			codec_capture_stop();
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			capture_prtd->state &= ~ST_RUNNING;
			/*
			 * stop capture dma transfer
			 */
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				return -EINVAL;
			}
			break;
		default:
			printk("error:%s,%d\n", __func__, __LINE__);
			capture_ret = -EINVAL;
			break;
		}
		spin_unlock(&capture_prtd->lock);
	}
	return 0;
}

static int snd_sun6icard_capture_open(struct snd_pcm_substream *substream)
{
	/*获得PCM运行时信息指针*/
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sun6i_capture_runtime_data *capture_prtd;

	capture_prtd = kzalloc(sizeof(struct sun6i_capture_runtime_data), GFP_KERNEL);
	if (capture_prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&capture_prtd->lock);
	runtime->private_data = capture_prtd;
	runtime->hw = sun6i_pcm_capture_hardware;

	/* ensure that buffer size is a multiple of period size */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;

	return 0;
}

static int snd_sun6icard_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	kfree(runtime->private_data);
	return 0;
}

static int snd_sun6icard_playback_open(struct snd_pcm_substream *substream)
{
	/*获得PCM运行时信息指针*/
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sun6i_playback_runtime_data *play_prtd;

	play_prtd = kzalloc(sizeof(struct sun6i_playback_runtime_data), GFP_KERNEL);
	if (play_prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&play_prtd->lock);
	runtime->private_data = play_prtd;
	runtime->hw = sun6i_pcm_playback_hardware;

	/* ensure that buffer size is a multiple of period size */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;

	return 0;
}

static int snd_sun6icard_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	kfree(runtime->private_data);
	return 0;
}

static struct snd_pcm_ops sun6i_pcm_playback_ops = {
	.open			= snd_sun6icard_playback_open,//打开
	.close			= snd_sun6icard_playback_close,//关闭
	.ioctl			= snd_pcm_lib_ioctl,//I/O控制
	.hw_params	    = sun6i_codec_pcm_hw_params,//硬件参数
	.hw_free	    = snd_sun6i_codec_hw_free,//资源释放
	.prepare		= snd_sun6i_codec_prepare,//准备
	.trigger		= snd_sun6i_codec_trigger,//在pcm被开始、停止或暂停时调用
	.pointer		= snd_sun6i_codec_pointer,//当前缓冲区的硬件位置
};

static struct snd_pcm_ops sun6i_pcm_capture_ops = {
	.open			= snd_sun6icard_capture_open,//打开
	.close			= snd_sun6icard_capture_close,//关闭
	.ioctl			= snd_pcm_lib_ioctl,//I/O控制
	.hw_params	    = sun6i_codec_pcm_hw_params,//硬件参数
	.hw_free	    = snd_sun6i_codec_hw_free,//资源释放
	.prepare		= snd_sun6i_codec_prepare,//准备
	.trigger		= snd_sun6i_codec_trigger,//在pcm被开始、停止或暂停时调用
	.pointer		= snd_sun6i_codec_pointer,//当前缓冲区的硬件位置
};

static int __init snd_card_sun6i_codec_pcm(struct sun6i_codec *sun6i_codec, int device)
{
	struct snd_pcm *pcm;
	int err;

	if ((err = snd_pcm_new(sun6i_codec->card, "M1 PCM", device, 1, 1, &pcm)) < 0){
		printk("error,the func is: %s,the line is:%d\n", __func__, __LINE__);
		return err;
	}

	/*
	 * this sets up our initial buffers and sets the dma_type to isa.
	 * isa works but I'm not sure why (or if) it's the right choice
	 * this may be too large, trying it for now
	 */

	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV,
					      snd_dma_isa_data(),
					      32*1024, 32*1024);

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &sun6i_pcm_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &sun6i_pcm_capture_ops);
	pcm->private_data = sun6i_codec;
	pcm->info_flags = 0;
	strcpy(pcm->name, "sun6i PCM");
	/* setup DMA controller */
	return 0;
}

void snd_sun6i_codec_free(struct snd_card *card)
{

}

static int __init sun6i_codec_probe(struct platform_device *pdev)
{
	int err;
	int ret;
	struct snd_card *card;
	struct sun6i_codec *chip;
	struct codec_board_info  *db;
    printk("enter sun6i Audio codec!!!\n");
	/* register the soundcard */
	ret = snd_card_create(SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1, THIS_MODULE, sizeof(struct sun6i_codec), &card);
	if (ret != 0) {
		return -ENOMEM;
	}

	chip = card->private_data;
	card->private_free = snd_sun6i_codec_free;
	chip->card = card;
	chip->samplerate = AUDIO_RATE_DEFAULT;

	if ((err = snd_chip_codec_mixer_new(card)))
		goto nodev;
	if ((err = snd_card_sun6i_codec_pcm(chip, 0)) < 0)
	    goto nodev;

	strcpy(card->driver, "sun6i-CODEC");
	strcpy(card->shortname, "audiocodec");
	sprintf(card->longname, "sun6i-CODEC  Audio Codec");

	snd_card_set_dev(card, &pdev->dev);

	if ((err = snd_card_register(card)) == 0) {
		printk( KERN_INFO "sun6i audio support initialized\n" );
		platform_set_drvdata(pdev, card);
	}else{
      return err;
	}

	db = kzalloc(sizeof(*db), GFP_KERNEL);
	if (!db)
		return -ENOMEM;
	/* codec_apbclk */
	codec_apbclk = clk_get(NULL,"apb_audio_codec");
	if (-1 == clk_enable(codec_apbclk)) {
		printk("codec_apbclk failed; \n");
	}
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL,"audio_pll");
	/* codec_moduleclk */
	codec_moduleclk = clk_get(NULL,"audio_codec");

	if (clk_set_parent(codec_moduleclk, codec_pll2clk)) {
		printk("try to set parent of codec_moduleclk to codec_pll2clk failed!\n");
	}
	if (clk_set_rate(codec_moduleclk, 24576000)) {
		printk("set codec_moduleclk clock freq 24576000 failed!\n");
	}
	if (-1 == clk_enable(codec_moduleclk)){
		printk("open codec_moduleclk failed; \n");
	}

	db->codec_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	db->dev = &pdev->dev;
	if (db->codec_base_res == NULL) {
		ret = -ENOENT;
		printk("codec insufficient resources\n");
		goto out;
	}
	 /* codec address remap */
	 db->codec_base_req = request_mem_region(db->codec_base_res->start, 0x40,
					   pdev->name);
	 if (db->codec_base_req == NULL) {
		 ret = -EIO;
		 printk("cannot claim codec address reg area\n");
		 goto out;
	 }

	 baseaddr = ioremap(db->codec_base_res->start, 0x40);
	 if (baseaddr == NULL) {
		 ret = -EINVAL;
		 dev_err(db->dev,"failed to ioremap codec address reg\n");
		 goto out;
	 }

	 kfree(db);
	 codec_init();

	 printk("sun6i Audio codec successfully loaded..\n");
	 return 0;
	 out:
		 dev_err(db->dev, "not found (%d).\n", ret);

	 nodev:
		snd_card_free(card);
		return err;
}

#ifdef CONFIG_PM
static int snd_sun6i_codec_suspend(struct platform_device *pdev,pm_message_t state)
{
	printk("[audio codec]:suspend\n");
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	/*disable pa*/
	codec_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x0);
	/*disable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC , 0x1, DAC_EN, 0x0);

	/*disable Master microphone bias*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);
	/*disable mic1 pa*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x0);//移到外部控制
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x0);//移到外部控制
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);

	clk_disable(codec_moduleclk);
	printk("[audio codec]:suspend end\n");
	return 0;
}

static int snd_sun6i_codec_resume(struct platform_device *pdev)
{
	printk("[audio codec]:resume start\n");
	if (-1 == clk_enable(codec_moduleclk)){
		printk("open codec_moduleclk failed; \n");
	}
	codec_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x1);
	printk("[audio codec]:resume end\n");
	return 0;
}
#endif

static int __devexit sun6i_codec_remove(struct platform_device *devptr)
{
	clk_disable(codec_moduleclk);
	clk_put(codec_pll2clk);
	clk_put(codec_apbclk);

	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	return 0;
}

static void sun6i_codec_shutdown(struct platform_device *devptr)
{
	/*mute l_pa and r_pa*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, LHPPA_MUTE, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, RHPPA_MUTE, 0x0);
	/*disable pa*/
	codec_wr_control(SUN6I_PA_CTRL, 0x1, HPPAEN, 0x0);
	/*disable dac_l and dac_r*/
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACALEN, 0x0);
	codec_wr_control(SUN6I_DAC_ACTL, 0x1, DACAREN, 0x0);
	/*disable dac digital*/
	codec_wr_control(SUN6I_DAC_DPC , 0x1, DAC_EN, 0x0);

	/*disable Master microphone bias*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MBIASEN, 0x0);
	/*disable mic1 pa*/
	codec_wr_control(SUN6I_MIC_CTRL, 0x1, MIC1AMPEN, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCREN, 0x0);
	codec_wr_control(SUN6I_ADC_ACTL, 0x1,  ADCLEN, 0x0);
	/*enable adc digital part*/
	codec_wr_control(SUN6I_ADC_FIFOC, 0x1,ADC_EN, 0x1);
}

static struct resource sun6i_codec_resource[] = {
	[0] = {
	.start = CODEC_BASSADDRESS,
        .end   = CODEC_BASSADDRESS + 0x40,
		.flags = IORESOURCE_MEM,
	},
};

/*data relating*/
static struct platform_device sun6i_device_codec = {
	.name = "sun6i-codec",
	.id = -1,
	.num_resources = ARRAY_SIZE(sun6i_codec_resource),
	.resource = sun6i_codec_resource,
};

/*method relating*/
static struct platform_driver sun6i_codec_driver = {
	.probe		= sun6i_codec_probe,
	.remove		= sun6i_codec_remove,
	.shutdown   = sun6i_codec_shutdown,
#ifdef CONFIG_PM
	.suspend	= snd_sun6i_codec_suspend,
	.resume		= snd_sun6i_codec_resume,
#endif
	.driver		= {
		.name	= "sun6i-codec",
	},
};

static int __init sun6i_codec_init(void)
{
	int err = 0;
	if((platform_device_register(&sun6i_device_codec))<0)
		return err;

	if ((err = platform_driver_register(&sun6i_codec_driver)) < 0)
		return err;

	return 0;
}

static void __exit sun6i_codec_exit(void)
{
	platform_driver_unregister(&sun6i_codec_driver);
}

module_init(sun6i_codec_init);
module_exit(sun6i_codec_exit);

MODULE_DESCRIPTION("sun6i CODEC ALSA codec driver");
MODULE_AUTHOR("huangxin");
MODULE_LICENSE("GPL");

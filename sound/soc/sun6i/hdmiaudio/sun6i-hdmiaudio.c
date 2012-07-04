/*
 * sound\soc\sun6i\hdmiaudio\sun6i-hdmiaudio.c
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

#include "sun6i-hdmipcm.h"
#include "sun6i-hdmiaudio.h"

static struct sun6i_dma_params sun6i_hdmiaudio_pcm_stereo_out = {
	.name 			= "hdmiaudio_play",
	.dma_addr 	=	0,
	.dma_size 	=   4,               /* dma transfer 32bits */
};

static struct sun6i_dma_params sun6i_hdmiaudio_pcm_stereo_in = {
	.name 			= "hdmiaudio_capture",
	.dma_addr 	=	0,
	.dma_size 	=   4,               /* dma transfer 32bits */
};

struct sun6i_hdmiaudio_info sun6i_hdmiaudio;

void sun6i_snd_txctrl_hdmiaudio(struct snd_pcm_substream *substream, int on)
{
}

void sun6i_snd_rxctrl_hdmiaudio(struct snd_pcm_substream *substream, int on)
{
}

static inline int sun6i_snd_is_clkmaster(void)
{
	return 0;
}

static int sun6i_hdmiaudio_set_fmt(struct snd_soc_dai *cpu_dai, unsigned int fmt)
{
	return 0;
}

static int sun6i_hdmiaudio_hw_params(struct snd_pcm_substream *substream,
																struct snd_pcm_hw_params *params,
																struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct sun6i_dma_params *dma_data;

	/* play or record */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		dma_data = &sun6i_hdmiaudio_pcm_stereo_out;
	else
		dma_data = &sun6i_hdmiaudio_pcm_stereo_in;

	snd_soc_dai_set_dma_data(rtd->cpu_dai, substream, dma_data);

	return 0;
}

static int sun6i_hdmiaudio_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;
	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_hdmiaudio(substream, 1);
			} else {
				sun6i_snd_txctrl_hdmiaudio(substream, 1);
			}
		break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
				sun6i_snd_rxctrl_hdmiaudio(substream, 0);
			} else {
			  sun6i_snd_txctrl_hdmiaudio(substream, 0);
			}
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

//freq:   1: 22.5792MHz   0: 24.576MHz
static int sun6i_hdmiaudio_set_sysclk(struct snd_soc_dai *cpu_dai, int clk_id,
                                 unsigned int freq, int dir)
{
	return 0;
}

static int sun6i_hdmiaudio_set_clkdiv(struct snd_soc_dai *cpu_dai, int div_id, int div)
{
	return 0;
}

u32 sun6i_hdmiaudio_get_clockrate(void)
{
	return 0;
}
EXPORT_SYMBOL_GPL(sun6i_hdmiaudio_get_clockrate);

static int sun6i_hdmiaudio_dai_probe(struct snd_soc_dai *dai)
{
	return 0;
}
static int sun6i_hdmiaudio_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

static int sun6i_hdmiaudio_suspend(struct snd_soc_dai *cpu_dai)
{
	return 0;
}

static int sun6i_hdmiaudio_resume(struct snd_soc_dai *cpu_dai)
{
	return 0;
}

#define SUN6I_I2S_RATES (SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT)
static struct snd_soc_dai_ops sun6i_hdmiaudio_dai_ops = {
	.trigger 		= sun6i_hdmiaudio_trigger,
	.hw_params 	= sun6i_hdmiaudio_hw_params,
	.set_fmt 		= sun6i_hdmiaudio_set_fmt,
	.set_clkdiv = sun6i_hdmiaudio_set_clkdiv,
	.set_sysclk = sun6i_hdmiaudio_set_sysclk,
};
static struct snd_soc_dai_driver sun6i_hdmiaudio_dai = {
	.probe 		= sun6i_hdmiaudio_dai_probe,
	.suspend 	= sun6i_hdmiaudio_suspend,
	.resume 	= sun6i_hdmiaudio_resume,
	.remove 	= sun6i_hdmiaudio_dai_remove,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = SUN6I_I2S_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,},
	.ops = &sun6i_hdmiaudio_dai_ops,
};

static int __devinit sun6i_hdmiaudio_dev_probe(struct platform_device *pdev)
{
	int ret = 0;

	ret = snd_soc_register_dai(&pdev->dev, &sun6i_hdmiaudio_dai);

	return 0;
}

static int __devexit sun6i_hdmiaudio_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_dai(&pdev->dev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static struct platform_device sun6i_hdmiaudio_device = {
	.name = "sun6i-hdmiaudio",
};

static struct platform_driver sun6i_hdmiaudio_driver = {
	.probe = sun6i_hdmiaudio_dev_probe,
	.remove = __devexit_p(sun6i_hdmiaudio_dev_remove),
	.driver = {
		.name = "sun6i-hdmiaudio",
		.owner = THIS_MODULE,
	},
};

static int __init sun6i_hdmiaudio_init(void)
{
	int err = 0;

	if((err = platform_device_register(&sun6i_hdmiaudio_device))<0)
		return err;

	if ((err = platform_driver_register(&sun6i_hdmiaudio_driver)) < 0)
		return err;

	return 0;
}
module_init(sun6i_hdmiaudio_init);

static void __exit sun6i_hdmiaudio_exit(void)
{
	platform_driver_unregister(&sun6i_hdmiaudio_driver);
}
module_exit(sun6i_hdmiaudio_exit);

module_platform_driver(sun6i_hdmiaudio_driver);
/* Module information */
MODULE_AUTHOR("ALLWINNER");
MODULE_DESCRIPTION("sun6i hdmiaudio SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: sun6i-hdmiaudio");

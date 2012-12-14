/*
 * sound\soc\sun6i\pcm\sun6i_sndpcm.c
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

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>
#include <mach/sys_config.h>

#include "sun6i-pcmdma.h"

static int pcm_used = 0;

static int sun6i_sndpcm_startup(struct snd_pcm_substream *substream)
{
	return 0;
}

static void sun6i_sndpcm_shutdown(struct snd_pcm_substream *substream)
{
}

static int sun6i_sndpcm_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret = 0;
	u32 mclk = 22579200;
	unsigned long sample_rate = params_rate(params);
	
	switch(sample_rate)
	{
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			mclk = 24576000;
			break;
	}
	
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , mclk, 0);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0)
		return ret;		
	return 0;
}

static struct snd_soc_ops sun6i_sndpcm_ops = {
	.startup 		= sun6i_sndpcm_startup,
	.shutdown 		= sun6i_sndpcm_shutdown,
	.hw_params 		= sun6i_sndpcm_hw_params,
};

static struct snd_soc_dai_link sun6i_sndpcm_dai_link = {
	.name 			= "PCM",
	.stream_name 	= "SUN6I-PCM",
	.cpu_dai_name 	= "sun6i-pcm.0",
	.codec_dai_name = "sndpcm",
	.platform_name 	= "sun6i-pcm-pcm-audio.0",	
	.codec_name 	= "sun6i-pcm-codec.0",
	.ops 			= &sun6i_sndpcm_ops,
};

static struct snd_soc_card snd_soc_sun6i_sndpcm = {
	.name = "sndpcm",
	.owner 		= THIS_MODULE,
	.dai_link = &sun6i_sndpcm_dai_link,
	.num_links = 1,
};

static struct platform_device *sun6i_sndpcm_device;

static int __init sun6i_sndpcm_init(void)
{
	int ret = 0;
	script_item_u val;
	script_item_value_type_e  type;

	type = script_get_item("pcm_para", "pcm_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] type err!\n");
    }

	pcm_used = val.val;
    if (pcm_used) {
		sun6i_sndpcm_device = platform_device_alloc("soc-audio", 3);
		if(!sun6i_sndpcm_device)
			return -ENOMEM;
		platform_set_drvdata(sun6i_sndpcm_device, &snd_soc_sun6i_sndpcm);
		ret = platform_device_add(sun6i_sndpcm_device);		
		if (ret) {			
			platform_device_put(sun6i_sndpcm_device);
		}
	}else{
		printk("[PCM]sun6i_sndpcm cannot find any using configuration for controllers, return directly!\n");
        return 0;
	}

	return ret;
}

static void __exit sun6i_sndpcm_exit(void)
{
	if(pcm_used) {
		pcm_used = 0;
		platform_device_unregister(sun6i_sndpcm_device);
	}
}

module_init(sun6i_sndpcm_init);
module_exit(sun6i_sndpcm_exit);

MODULE_AUTHOR("chenpailin");
MODULE_DESCRIPTION("SUN6I_sndpcm ALSA SoC audio driver");
MODULE_LICENSE("GPL");

/*
 * sound\soc\sun6i\hdmiaudio\sun6i-hdmiaudio.h
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
#ifndef SUN6I_HDMIAUIDO_H_
#define SUN6I_HDMIAUIDO_H_

/*------------------------------------------------------------*/
/* Clock dividers */
#define SUN6I_DIV_MCLK	0
#define SUN6I_DIV_BCLK	1

#define SUN6I_HDMIAUDIOCLKD_MCLK_MASK   0x0f
#define SUN6I_HDMIAUDIOCLKD_MCLK_OFFS   0
#define SUN6I_HDMIAUDIOCLKD_BCLK_MASK   0x070
#define SUN6I_HDMIAUDIOCLKD_BCLK_OFFS   4
#define SUN6I_HDMIAUDIOCLKD_MCLKEN_OFFS 7

unsigned int sun6i_hdmiaudio_get_clockrate(void);
extern struct sun6i_hdmiaudio_info sun6i_hdmiaudio;

extern void sun6i_snd_txctrl_hdmiaudio(struct snd_pcm_substream *substream, int on);

struct sun6i_hdmiaudio_info {
	void __iomem   *regs;    /* IIS BASE */
	void __iomem   *ccmregs;  //CCM BASE
	void __iomem   *ioregs;   //IO BASE
};

extern struct sun6i_hdmiaudio_info sun6i_hdmiaudio;
#endif

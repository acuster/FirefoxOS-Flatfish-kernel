/*
 * sound\soc\sun7i\i2s\sun7i-i2sdma.h
 * (C) Copyright 2007-2011
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


#ifndef SUN7I_PCM_H_
#define SUN7I_PCM_H_

#define ST_RUNNING    (1<<0)
#define ST_OPENED     (1<<1)

struct sun7i_dma_params {
	struct sw_dma_client *client;
	int channel;
	dma_addr_t dma_addr;
	int dma_size;
};

#define SUN7I_DAI_I2S			1

enum sun7i_dma_buffresult {
	SUN7I_RES_OK,
	SUN7I_RES_ERR,
	SUN7I_RES_ABORT
};

/* platform data */
extern struct snd_soc_platform sun7i_soc_platform_i2s;
extern int sw_dma_enqueue(unsigned int channel, void *id,
			dma_addr_t data, int size);
extern struct sun7i_i2s_info sun7i_iis;

#endif //SUN7I_PCM_H_

/*
 * sound\soc\sun7i\hdmiaudio\sun7i-hdmipcm.h
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

#ifndef SUN7I_HDMIPCM_H_
#define SUN7I_HDMIPCM_H_

#undef HX_NEW_DMA
#define HX_NEW_DMA
#ifdef HX_NEW_DMA
	struct sun7i_dma_params {
		char *name;
		dma_addr_t dma_addr;
	};

#else
struct sun7i_dma_params {
	struct sw_dma_client *client;
	int channel;
	dma_addr_t dma_addr;
	int dma_size;
};

enum sun7i_dma_buffresult {
	SUN7I_RES_OK,
	SUN7I_RES_ERR,
	SUN7I_RES_ABORT
};

/* platform data */
extern int sw_dma_enqueue(unsigned int channel, void *id,
			dma_addr_t data, int size);
#endif
#endif //SUN7I_HDMIPCM_H_

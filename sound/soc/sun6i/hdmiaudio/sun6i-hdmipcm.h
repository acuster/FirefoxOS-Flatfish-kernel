/*
 * sound\soc\sun6i\hdmiaudio\sun6i-hdmipcm.h
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

#ifndef SUN6I_HDMIPCM_H_
#define SUN6I_HDMIPCM_H_

struct sun6i_dma_params {
	struct sw_dma_client *client;
	int channel;
	dma_addr_t dma_addr;
	int dma_size;
	char *name;
};

enum sun6i_dma_buffresult {
	SUN6I_RES_OK,
	SUN6I_RES_ERR,
	SUN6I_RES_ABORT
};

#endif //SUN6I_HDMIPCM_H_

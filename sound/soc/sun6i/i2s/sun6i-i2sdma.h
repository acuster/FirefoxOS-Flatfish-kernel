/*
 * sound\soc\sun6i\i2s\sun6i-i2sdma.h
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


#ifndef SUN6I_I2SDMA_H_
#define SUN6I_I2SDMA_H_

#define ST_RUNNING    (1<<0)
#define ST_OPENED     (1<<1)

struct sun6i_dma_params {
	char *name;
	dma_addr_t dma_addr;
};
#define SUN6I_DAI_I2S			1

extern struct sun6i_i2s_info sun6i_iis;

#endif //SUN6I_I2SDMA_H_

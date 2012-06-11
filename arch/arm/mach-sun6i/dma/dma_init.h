/*
 * arch/arm/mach-sun6i/dma/dma_init.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sun6i dma header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DMA_INIT_H
#define __DMA_INIT_H

int dma_init(struct platform_device *device);
int dma_deinit(void);


#endif  /* __DMA_INIT_H */

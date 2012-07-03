/*
 * arch/arm/XXX/gpio_common.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * XXX
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __GPIO_COMMON_H
#define __GPIO_COMMON_H

#define DBG_GPIO /* debug gpio driver */

/*
 * gpio print macro
 */
#define PIO_DBG_LEVEL	1

#if (PIO_DBG_LEVEL == 1)
	#define PIO_DBG(format,args...)   printk("[gpio-dbg] "format,##args)
	#define PIO_INF(format,args...)   printk("[gpio-inf] "format,##args)
	#define PIO_ERR(format,args...)   printk("[gpio-err] "format,##args)
#elif (PIO_DBG_LEVEL == 2)
	#define PIO_DBG(format,args...)
	#define PIO_INF(format,args...)   printk("[gpio-inf] "format,##args)
	#define PIO_ERR(format,args...)   printk("[gpio-err] "format,##args)
#elif (PIO_DBG_LEVEL == 3)
	#define PIO_DBG(format,args...)
	#define PIO_INF(format,args...)
	#define PIO_ERR(format,args...)   printk("[gpio-err] "format,##args)
#endif

#define PIO_DBG_FUN_LINE   		PIO_DBG("%s(%d)\n", __FUNCTION__, __LINE__)
#define PIO_ERR_FUN_LINE   		PIO_ERR("%s(%d)\n", __FUNCTION__, __LINE__)
#define PIO_DBG_FUN_LINE_TODO  		PIO_DBG("%s(%d) - todo########\n", __FUNCTION__, __LINE__)
#define PIO_DBG_FUN_LINE_TOCHECK 	PIO_DBG("%s(%d) - tocheck########\n", __FUNCTION__, __LINE__)
#define PIO_ASSERT(x)			if(!(x)) {PIO_INF("%s err, line %d\n", __FUNCTION__, __LINE__);}

/*
 * pull state
 */
enum pull_sta_e {
	PULL_DISABLE 	= (0b00	),  	/* XXX */
	PULL_UP 	= (0b01	),  	/* XXX */
	PULL_DOWN 	= (0b10	),  	/* XXX */
	PULL_RESERVE 	= (0b11	),  	/* XXX */
};

/*
 * driver level state
 */
enum driver_level_e {
	DRV_LVL_0 	= (0b00	),  	/* XXX */
	DRV_LVL_1 	= (0b01	),  	/* XXX */
	DRV_LVL_2 	= (0b10	),  	/* XXX */
	DRV_LVL_3 	= (0b11	),  	/* XXX */
};

/*
 * default pull and driver lever
 */
#define PIO_DEFAULT_PULL   		PULL_UP
#define PIO_DEFAULT_DRVLVL   		DRV_LVL_1

/*
 * config value for input/output, common for every pio
 */
#define PIO_CFG_INPUT   		(0b000	)
#define PIO_CFG_OUTPUT   		(0b001	)

/*
 * bits width, common for every pio
 */
#define PIO_BITS_WIDTH_CFG   		(3	)
#define PIO_BITS_WIDTH_PULL   		(2	)
#define PIO_BITS_WIDTH_DRVLVL   	(2	)

/*
 * reg offset(from cfg0 reg addr)
 */
#define PIO_OFF_REG_DATA   		(0x10	) /* data reg offset */
#define PIO_OFF_REG_PULL   		(0x1C	) /* pull0 reg offset */
#define PIO_OFF_REG_DRVLVL   		(0x14	) /* drvlevel0 reg offset */

/*
 * lock operation for gpio chip
 */
#define PIO_CHIP_LOCK_INIT(lock)	spin_lock_init((lock))
#define PIO_CHIP_LOCK_DEINIT(lock)	do{}while(0)
#define PIO_CHIP_LOCK(lock, flags)	spin_lock_irqsave((lock), (flags))
#define PIO_CHIP_UNLOCK(lock, flags)	spin_unlock_irqrestore((lock), (flags))

/*
 * reg read write operation
 */
#define PIO_READ_REG		readl
#define PIO_WRITE_REG		writel

/*
 * read/write bits value from pos of reg
 */
#define PIO_READ_REG_BITS(reg, pos, width)		((readl(reg) >> (pos)) & ((1 << (width)) - 1))
#define PIO_WRITE_REG_BITS(reg, pos, width, val)	writel(reg, (readl(reg) & (u32)(~(((1 << (width)) - 1) << (pos)))) \
									| (u32)(((val) & ((1 << (width)) - 1)) << (pos)))

#endif  /* __GPIO_COMMON_H */

/*
 * arch/arm/mach-sun6i/gpio/gpio_common.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sun6i gpio header file
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
#define PIO_DBG_LEVEL	3

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
#define PIO_ASSERT(x)			if(!(x)) {PIO_ERR("%s err, line %d\n", __FUNCTION__, __LINE__);}

/*
 * pull state
 */
enum pull_sta_e {
	PULL_DISABLE 	= (0b00	),  	/* pull up/down disable */
	PULL_UP 	= (0b01	),  	/* pull up */
	PULL_DOWN 	= (0b10	),  	/* pull down */
	PULL_RESERVE 	= (0b11	),  	/* reserve */
};

/*
 * driver level state
 */
enum driver_level_e {
	DRV_LVL_0 	= (0b00	),  	/* driver level 0 */
	DRV_LVL_1 	= (0b01	),  	/* driver level 1 */
	DRV_LVL_2 	= (0b10	),  	/* driver level 2 */
	DRV_LVL_3 	= (0b11	),  	/* driver level 3 */
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
#define PIO_READ_REG(reg)		readl(reg)
#define PIO_WRITE_REG(reg, val)		writel((val), (reg))

/*
 * read/write bits value from pos of reg
 */
#define PIO_READ_REG_BITS(reg, pos, width)		((PIO_READ_REG(reg) >> (pos)) & ((1 << (width)) - 1))
#define PIO_WRITE_REG_BITS(reg, pos, width, val)	PIO_WRITE_REG(reg, (readl(reg) & (u32)(~(((1 << (width)) - 1) << (pos)))) \
									| (u32)(((val) & ((1 << (width)) - 1)) << (pos)))

/*
 * gpio struct define below
 */
struct gpio_cfg_t;
struct gpio_pm_t;
struct aw_gpio_chip;

typedef u32 (*pset_cfg)(struct aw_gpio_chip *pchip, u32 offset, u32 val);
typedef u32 (*pget_cfg)(struct aw_gpio_chip *pchip, u32 offset);
typedef u32 (*pset_pull)(struct aw_gpio_chip *pchip, u32 offset, u32 val);
typedef u32 (*pget_pull)(struct aw_gpio_chip *pchip, u32 offset);
typedef u32 (*pset_drvlevel)(struct aw_gpio_chip *pchip, u32 offset, u32 val);
typedef u32 (*pget_drvlevel)(struct aw_gpio_chip *pchip, u32 offset);

struct gpio_cfg_t {
	pset_cfg 	set_cfg;
	pget_cfg 	get_cfg;
	pset_pull 	set_pull;
	pget_pull 	get_pull;
	pset_drvlevel 	set_drvlevel;
	pget_drvlevel 	get_drvlevel;
};

typedef u32 (*psave)(struct aw_gpio_chip *pchip);
typedef u32 (*presume)(struct aw_gpio_chip *pchip);

struct gpio_pm_t {
	psave 		save;
	presume 	resume;
};

/*
 * struct for aw gpio chip
 */
struct aw_gpio_chip {
	struct gpio_chip 	chip;
	struct gpio_cfg_t	*cfg;
	struct gpio_pm_t 	*pm;
	void __iomem		*vbase;
	spinlock_t 		lock;
};

#endif  /* __GPIO_COMMON_H */

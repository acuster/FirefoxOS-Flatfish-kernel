/*
 * arch/arm/XXX/gpio_init.h
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

#ifndef __GPIO_INIT_H
#define __GPIO_INIT_H

extern u32 g_pio_vbase, g_rpio_vbase;

/*
 * pio base for PA ~ PF
 */
#define PIO_VBASE(n) 		(g_pio_vbase + ((n) << 5) + ((n) << 2)) /* 0xf1c20800 + n * 0x24 */

/*
 * r-pio base for PL ~ PM
 */
#define RPIO_VBASE(n) 		(g_rpio_vbase + ((n) << 5) + ((n) << 2)) /* 0xf1f02c00 + n * 0x24 */

/*
 * port number for each pio
 */
#define PA_NR			28
#define PB_NR			8
#define PC_NR			27
#define PD_NR			28
#define PE_NR			17
#define PF_NR			6
#define PG_NR			19
#define PH_NR			31
/* for R-PORT PIO */
#define PL_NR			9
#define PM_NR			9 /* spec: "Pin MultiPlex" is 8 while "PM Configure Register 1" is 9 */

#define AW_GPIO_NEXT(gpio)	gpio##_NR_BASE + gpio##_NR + 1

/*
 * base index for each pio
 */
enum sun6i_gpio_number {
	PA_NR_BASE = 0,
	PB_NR_BASE = AW_GPIO_NEXT(PA),
	PC_NR_BASE = AW_GPIO_NEXT(PB),
	PD_NR_BASE = AW_GPIO_NEXT(PC),
	PE_NR_BASE = AW_GPIO_NEXT(PD),
	PF_NR_BASE = AW_GPIO_NEXT(PE),
	PG_NR_BASE = AW_GPIO_NEXT(PF),
	PH_NR_BASE = AW_GPIO_NEXT(PG),
	/* for R-PORT PIO */
	PL_NR_BASE = AW_GPIO_NEXT(PH), /* NOTE: last is PH */
	PM_NR_BASE = AW_GPIO_NEXT(PL),
};

/*
 * pio index definition
 */
#define GPIOA(n)		(PA_NR_BASE + (n))
#define GPIOB(n)		(PB_NR_BASE + (n))
#define GPIOC(n)		(PC_NR_BASE + (n))
#define GPIOD(n)		(PD_NR_BASE + (n))
#define GPIOE(n)		(PE_NR_BASE + (n))
#define GPIOF(n)		(PF_NR_BASE + (n))
#define GPIOG(n)		(PG_NR_BASE + (n))
#define GPIOH(n)		(PH_NR_BASE + (n))
#define GPIOL(n)		(PL_NR_BASE + (n))
#define GPIOM(n)		(PM_NR_BASE + (n))

/*
 * the end of pio index
 */
#define GPIO_INDEX_END		(GPIOM(PM_NR) + 1)
#define GPIO_INDEX_INVALID	(0xFFFFFFFF      )
#define GPIO_CFG_INVALID	(0xFFFFFFFF      )
#define GPIO_PULL_INVALID	(0xFFFFFFFF      )
#define GPIO_DRVLVL_INVALID	(0xFFFFFFFF      )

/*
 * port number for gpiolib
 */
#ifdef ARCH_NR_GPIOS
#undef ARCH_NR_GPIOS
#endif
#define ARCH_NR_GPIOS		(GPIO_INDEX_END)

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

#endif /* __GPIO_INIT_H */

/*
 * arch/arm/XXX/gpio_base.h
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

#ifndef __GPIO_BASE_H
#define __GPIO_BASE_H

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static inline int gpiochip_match(struct gpio_chip * chip, void * data)
{
	u32 	num = 0;

#ifdef DBG_GPIO
	if(NULL == data) {
		PIO_ERR_FUN_LINE;
		return 0;
	}
#endif /* DBG_GPIO */

	num = *(u32 *)data;
	if(num >= chip->base && num < chip->base + chip->ngpio) {
		return 1;
	}

	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static inline struct gpio_chip *to_gpiochip(u32 gpio)
{
	u32 	num = gpio;
	struct gpio_chip *pchip = NULL;

	pchip = gpiochip_find((void *)&num, gpiochip_match);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return NULL;
	}

	return pchip;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static inline struct aw_gpio_chip *to_aw_gpiochip(struct gpio_chip *gpc)
{
	return container_of(gpc, struct aw_gpio_chip, chip);
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static inline struct aw_gpio_chip *gpio_to_aw_gpiochip(u32 gpio)
{
	struct gpio_chip *pchip = NULL;

	pchip = to_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return NULL;
	}

	return to_aw_gpiochip(pchip);
}

int aw_gpiochip_add(struct gpio_chip *chip);

#endif /* __GPIO_BASE_H */

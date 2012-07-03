/*
 * arch/arm/XXX/gpio_base.c
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

#include "gpio_include.h"

#if 0
/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static int __gpio_request(struct gpio_chip *chip, unsigned offset)
{
	/*
	 * not need realise
	 */
	PIO_DBG_FUN_LINE_TOCHECK;
	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static void __gpio_free(struct gpio_chip *chip, unsigned offset)
{
	/*
	 * not need realise
	 */
	PIO_DBG_FUN_LINE_TOCHECK;
	return;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static int __gpio_set_debounce(struct gpio_chip *chip, unsigned offset, unsigned debounce)
{
	/*
	 * todo: "External Int Debounce Register"
	 * (1) Debounce clock Pre-scale n
	 * (2) PIO Interrupt clock select: LOSC or HOSC
	 */
	PIO_DBG_FUN_LINE_TODO;
	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static int __gpio_index_to_irq(struct gpio_chip *chip, unsigned offset)
{
	/*
	 * todo: External Int
	 */
	PIO_DBG_FUN_LINE_TODO;
	return 0;
}
#endif

/**
 * __gpio_direction_input - set pio to input
 * NOTE: not need set pull and driver level, user care
 * @chip:	gpio_chip
 * @offset:	gpio offset from gpio_chip->base
 *
 * Returns 0 if success, the err line number otherwise.
 */
static int __gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	u32 	ureg_off = 0, ubits_off = 0;
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	PIO_CHIP_LOCK(&pchip->lock, flags);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_INPUT);

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static int __gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	u32 	ureg_off = 0, ubits_off = 0;
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	PIO_CHIP_LOCK(&pchip->lock, flags);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_OUTPUT);
	PIO_WRITE_REG(PIO_OFF_REG_DATA + pchip->vbase, BIT(offset));

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return 0;
}

/**
 * XXX - XXX NOTE: pio is already input, so just get the value
 * XXX:	XXX
 *
 * XXX
 */
static int __gpio_get(struct gpio_chip *chip, unsigned offset)
{
	int 	iret = 0;
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

	PIO_CHIP_LOCK(&pchip->lock, flags);

	iret = (PIO_READ_REG(PIO_OFF_REG_DATA + pchip->vbase)) & BIT(offset);

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return iret;
}

/**
 * XXX - XXX NOTE: pio is already output, so just set the value
 * XXX:	XXX
 *
 * XXX
 */
static void __gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

	PIO_CHIP_LOCK(&pchip->lock, flags);

	PIO_WRITE_REG(PIO_OFF_REG_DATA + pchip->vbase, (0 == value ? 0 : BIT(offset)));

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
int aw_gpiochip_add(struct gpio_chip *chip)
{
	/*
	 * if func_api NULL, install them
	 */
	if(NULL == chip->direction_input) {
		chip->direction_input = __gpio_direction_input;
	}
	if(NULL == chip->direction_output) {
		chip->direction_output = __gpio_direction_output;
	}
	if(NULL == chip->set) {
		chip->set = __gpio_set;
	}
	if(NULL == chip->get) {
		chip->get = __gpio_get;
	}

	/*
	 * external irq: to do
	 */
	PIO_DBG_FUN_LINE_TODO;
#if 0
	if(NULL == chip->to_irq) {
		chip->to_irq = __gpio_index_to_irq;
	}
#endif

	/*
	 * register chip to gpiolib
	 */
	if(0 != gpiochip_add(chip)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	return 0;
}

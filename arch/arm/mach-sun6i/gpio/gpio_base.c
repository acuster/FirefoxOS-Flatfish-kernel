/*
 * arch/arm/mach-sun6i/gpio/gpio_base.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sun6i gpio driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "gpio_include.h"

#if 0 /* in order to remove warning: defined but not used */
/**
 * __gpio_request - platform gpio request, do nothing
 * @chip:	gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns 0 if sucess, the err line number if failed.
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
 * __gpio_free - platform gpio release, do nothing
 * @chip:	gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
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
 * __gpio_set_debounce - set debounce, for gpio int?
 * @chip:	gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @debounce:	debounce val to set
 *
 * Returns 0 if sucess, the err line number if failed.
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
 * __gpio_index_to_irq - get irq num for the gpio index
 * @chip:	gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns the irq num if the gpio can be configured as external irq
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

#if 0
//#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, aw chip 0x%08x\n", __FUNCTION__,
		(u32)chip, offset, (u32)pchip);
#endif /* DBG_GPIO */

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	PIO_CHIP_LOCK(&pchip->lock, flags);

	PIO_DBG("%s: write cfg reg 0x%08x, bits_off %d, width %d, cfg_val %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_INPUT);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_INPUT);

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return 0;
}

/**
 * __gpio_direction_output - set the gpio as ouput, and set the val
 * @chip:	gpio_chip
 * @offset:	offset from gpio_chip->base
 * @value:	the val to set
 *
 * Returns 0 if success, the err line number otherwise.
 */
static int __gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	u32 	ureg_off = 0, ubits_off = 0;
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

#if 0
//#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, val %d, aw chip 0x%08x\n", __FUNCTION__,
		(u32)chip, offset, value, (u32)pchip);
#endif /* DBG_GPIO */

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	PIO_CHIP_LOCK(&pchip->lock, flags);

	PIO_DBG("%s: write cfg reg 0x%08x, bits_off %d, width %d, cfg_val %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_OUTPUT);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_OUTPUT);

	PIO_DBG("%s: write data reg 0x%08x, offset %d, val %d\n", __FUNCTION__, \
		(u32)(PIO_OFF_REG_DATA + pchip->vbase), offset, (0 != value ? 1 : 0));

	PIO_WRITE_REG_BITS(PIO_OFF_REG_DATA + pchip->vbase, offset, 1, (0 != value ? 1 : 0));

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return 0;
}

/**
 * __gpio_get - get the gpio value. NOTE: the gpio is already input.
 * @chip:	gpio_chip
 * @offset:	offset from gpio_chip->base
 *
 * Return the gpio value(data).
 */
static int __gpio_get(struct gpio_chip *chip, unsigned offset)
{
	int 	iret = 0;
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

#if 0
//#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, aw chip 0x%08x\n", __FUNCTION__,
		(u32)chip, offset, (u32)pchip);
#endif /* DBG_GPIO */

	PIO_CHIP_LOCK(&pchip->lock, flags);

	iret = PIO_READ_REG_BITS(PIO_OFF_REG_DATA + pchip->vbase, offset, 1);

	PIO_DBG("%s: read data reg 0x%08x - 0x%08x, offset %d, ret %d\n", __FUNCTION__,
		(u32)(PIO_OFF_REG_DATA + pchip->vbase),	PIO_READ_REG(PIO_OFF_REG_DATA + pchip->vbase), offset, iret);

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return iret;
}

/**
 * __gpio_set - set the gpio value. NOTE: the gpio is already output.
 * @chip:	gpio_chip
 * @offset:	offset from gpio_chip->base
 * @value:	the val to set
 */
static void __gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	unsigned long flags;
	struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

#if 0
//#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, value %d, aw chip 0x%08x\n", __FUNCTION__,
		(u32)chip, offset, value, (u32)pchip);
#endif /* DBG_GPIO */

	PIO_CHIP_LOCK(&pchip->lock, flags);

	PIO_DBG("%s: write data reg 0x%08x, offset %d, val %d\n", __FUNCTION__,
		(u32)(PIO_OFF_REG_DATA + pchip->vbase), offset, (0 != value ? 1 : 0));

	PIO_WRITE_REG_BITS(PIO_OFF_REG_DATA + pchip->vbase, offset, 1, (0 != value ? 1 : 0));

	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return;
}

/**
 * aw_gpiochip_add - init gpio_chip struct, and register the chip to gpiolib
 * @chip:	gpio_chip
 *
 * Returns 0 if success, the err line number otherwise.
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

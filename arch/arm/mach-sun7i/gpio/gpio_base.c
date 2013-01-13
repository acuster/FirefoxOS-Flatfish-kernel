/*
 * arch/arm/mach-sun7i/gpio/gpio_base.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 * James Deng <csjamesdeng@reuuimllatech.com>
 *
 * sun7i gpio driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "gpio_include.h"

/**
 * __gpio_direction_input - set pio to input
 * NOTE: not need set pull and driver level, user care
 * @chip:   gpio_chip
 * @offset: gpio offset from gpio_chip->base
 *
 * Returns 0 if success, the err line number otherwise.
 */
static int __gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
    u32 ureg_off = 0, ubits_off = 0;
    unsigned long flags;
    struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

    ureg_off = ((offset << 2) >> 5) << 2;   /* ureg_off = ((offset * 4) / 32) * 4 */
    ubits_off = (offset << 2) % 32;     /* ubits_off = (offset * 4) % 32 */

    PIO_CHIP_LOCK(&pchip->lock, flags);

    PIO_DBG("%s: write cfg reg 0x%08x, bits_off %d, width %d, cfg_val %d\n", __FUNCTION__,
            (u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_INPUT);

    PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, PIO_CFG_INPUT);

    PIO_CHIP_UNLOCK(&pchip->lock, flags);
    return 0;
}

/**
 * __gpio_direction_output - set the gpio as ouput, and set the val
 * @chip:   gpio_chip
 * @offset: offset from gpio_chip->base
 * @value:  the val to set
 *
 * Returns 0 if success, the err line number otherwise.
 */
static int __gpio_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
    u32 ureg_off = 0, ubits_off = 0;
    unsigned long flags;
    struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

    ureg_off = ((offset << 2) >> 5) << 2;   /* ureg_off = ((offset * 4) / 32) * 4 */
    ubits_off = (offset << 2) % 32;     /* ubits_off = (offset * 4) % 32 */

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
 * @chip:   gpio_chip
 * @offset: offset from gpio_chip->base
 *
 * Return the gpio value(data).
 */
static int __gpio_get(struct gpio_chip *chip, unsigned offset)
{
    int iret = 0;
    unsigned long flags;
    struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

    PIO_CHIP_LOCK(&pchip->lock, flags);

    iret = PIO_READ_REG_BITS(PIO_OFF_REG_DATA + pchip->vbase, offset, 1);

    PIO_DBG("%s: read data reg 0x%08x - 0x%08x, offset %d, ret %d\n", __FUNCTION__,
            (u32)(PIO_OFF_REG_DATA + pchip->vbase), PIO_READ_REG(PIO_OFF_REG_DATA + pchip->vbase), offset, iret);

    PIO_CHIP_UNLOCK(&pchip->lock, flags);
    return iret;
}

/**
 * __gpio_set - set the gpio value. NOTE: the gpio is already output.
 * @chip:   gpio_chip
 * @offset: offset from gpio_chip->base
 * @value:  the val to set
 */
static void __gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
    unsigned long flags;
    struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

    PIO_CHIP_LOCK(&pchip->lock, flags);

    PIO_DBG("%s: write data reg 0x%08x, offset %d, val %d\n", __FUNCTION__,
            (u32)(PIO_OFF_REG_DATA + pchip->vbase), offset, (0 != value ? 1 : 0));

    PIO_WRITE_REG_BITS(PIO_OFF_REG_DATA + pchip->vbase, offset, 1, (0 != value ? 1 : 0));

    PIO_CHIP_UNLOCK(&pchip->lock, flags);
}

/**
 * aw_gpiochip_add - init gpio_chip struct, and register the chip to gpiolib
 * @chip:   gpio_chip
 *
 * Returns 0 if success, the err line number otherwise.
 */
int aw_gpiochip_add(struct gpio_chip *chip)
{
    /*
     * if func_api NULL, install them
     */
    if (NULL == chip->direction_input) {
        chip->direction_input = __gpio_direction_input;
    }
    if (NULL == chip->direction_output) {
        chip->direction_output = __gpio_direction_output;
    }
    if (NULL == chip->set) {
        chip->set = __gpio_set;
    }
    if (NULL == chip->get) {
        chip->get = __gpio_get;
    }

    /*
     * register chip to gpiolib
     */
    if (0 != gpiochip_add(chip)) {
        PIO_ERR_FUN_LINE;
        return __LINE__;
    }

    return 0;
}

/**
 * __pio_to_irq - find the interrupt num for the gpio
 * @chip:   gpio_chip
 * @offset: gpio offset from gpio_chip->base
 *
 * Returns the irq number if sucess, IRQ_NUM_INVALID if failed.
 */
int __pio_to_irq(struct gpio_chip *chip, unsigned offset)
{
    struct aw_gpio_chip *pchip = to_aw_gpiochip(chip);

    if (true == gpio_canbe_eint(chip->base + offset)) {
        PIO_DBG_FUN_LINE;
        return pchip->irq_num;
    }

    PIO_ERR_FUN_LINE;
    return IRQ_NUM_INVALID;
}

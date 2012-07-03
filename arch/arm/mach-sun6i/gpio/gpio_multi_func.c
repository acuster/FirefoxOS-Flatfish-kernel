/*
 * arch/arm/XXX/gpio_multi_func.c
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

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 gpio_set_cfg(struct aw_gpio_chip *pchip, u32 offset, u32 val)
{
	u32 	ureg_off = 0, ubits_off = 0;

#ifdef DBG_GPIO
	if(val & ~((1 << PIO_BITS_WIDTH_CFG) - 1)) {
		PIO_ERR_FUN_LINE;
	}
#endif /* DBG_GPIO */

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, val);
	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 gpio_get_cfg(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	ureg_off = 0, ubits_off = 0;

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	return PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG);
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 gpio_set_pull(struct aw_gpio_chip *pchip, u32 offset, u32 val)
{
	u32 	ureg_off = 0, ubits_off = 0;

#ifdef DBG_GPIO
	if(val & ~((1 << PIO_BITS_WIDTH_PULL) - 1)) {
		PIO_ERR_FUN_LINE;
	}
#endif /* DBG_GPIO */

	ureg_off = PIO_OFF_REG_PULL + (((offset << 1) >> 5) << 2); 	/* ureg_off = ((offset * 2) / 32) * 4 */
	ubits_off = (offset << 1) % 32;					/* ubits_off = (offset * 2) % 32 */

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL, val);
	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 gpio_get_pull(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	ureg_off = 0, ubits_off = 0;

	ureg_off = PIO_OFF_REG_PULL + (((offset << 1) >> 5) << 2); 	/* ureg_off = ((offset * 2) / 32) * 4 */
	ubits_off = (offset << 1) % 32;					/* ubits_off = (offset * 2) % 32 */

	return PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL);
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 gpio_set_drvlevel(struct aw_gpio_chip *pchip, u32 offset, u32 val)
{
	u32 	ureg_off = 0, ubits_off = 0;

#ifdef DBG_GPIO
	if(val & ~((1 << PIO_BITS_WIDTH_DRVLVL) - 1)) {
		PIO_ERR_FUN_LINE;
	}
#endif /* DBG_GPIO */

	ureg_off = PIO_OFF_REG_DRVLVL + (((offset << 1) >> 5) << 2); 	/* ureg_off = ((offset * 2) / 32) * 4 */
	ubits_off = (offset << 1) % 32;					/* ubits_off = (offset * 2) % 32 */

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL, val);
	return 0;
}

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 gpio_get_drvlevel(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	ureg_off = 0, ubits_off = 0;

	ureg_off = PIO_OFF_REG_DRVLVL + (((offset << 1) >> 5) << 2); 	/* ureg_off = ((offset * 2) / 32) * 4 */
	ubits_off = (offset << 1) % 32;					/* ubits_off = (offset * 2) % 32 */

	return PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL);
}

#ifdef DBG_GPIO
/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
static bool is_gpio_requested(u32 gpio)
{
	struct gpio_chip *pchip = NULL;

	pchip = to_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return NULL;
	}

	if(NULL == gpiochip_is_requested(pchip, gpio - pchip->base))
		return false;
	else
		return true;
}
#endif /* DBG_GPIO */

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_setcfg(u32 gpio, u32 val)
{
	u32 	uret = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio)) {
		PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, gpio, uret);
	}
#endif /* DBG_GPIO */

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		uret = __LINE__;
		goto End;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	if(NULL == pchip->cfg || NULL == pchip->cfg->set_cfg) {
		uret = __LINE__;
		goto End;
	}

	offset = gpio - pchip->chip.base;
	if(0 != pchip->cfg->set_cfg(pchip, offset, val)) {
		uret = __LINE__;
		goto End;
	}

End:
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	if(0 != uret) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_setcfg);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_getcfg(u32 gpio)
{
	u32 	usign = 0;
	u32 	uret = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio)) {
		PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, gpio, uret);
	}
#endif /* DBG_GPIO */

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		usign = __LINE__;
		goto End;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	if(NULL == pchip->cfg || NULL == pchip->cfg->get_cfg) {
		usign = __LINE__;
		goto End;
	}

	offset = gpio - pchip->chip.base;
	uret = pchip->cfg->get_cfg(pchip, offset);

End:
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	if(0 != usign) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, usign);
		return GPIO_CFG_INVALID;
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_getcfg);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_setpull(u32 gpio, u32 val)
{
	u32 	uret = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio)) {
		PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, gpio, uret);
	}
#endif /* DBG_GPIO */

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		uret = __LINE__;
		goto End;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	if(NULL == pchip->cfg || NULL == pchip->cfg->set_pull) {
		uret = __LINE__;
		goto End;
	}

	offset = gpio - pchip->chip.base;
	if(0 != pchip->cfg->set_pull(pchip, offset, val)) {
		uret = __LINE__;
		goto End;
	}

End:
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	if(0 != uret) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_setpull);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_getpull(u32 gpio)
{
	u32 	usign = 0;
	u32 	uret = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio)) {
		PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, gpio, uret);
	}
#endif /* DBG_GPIO */

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		usign = __LINE__;
		goto End;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	if(NULL == pchip->cfg || NULL == pchip->cfg->get_pull) {
		usign = __LINE__;
		goto End;
	}

	offset = gpio - pchip->chip.base;
	uret = pchip->cfg->get_pull(pchip, offset);

End:
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	if(0 != usign) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, usign);
		return GPIO_PULL_INVALID;
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_getpull);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_setdrvlevel(u32 gpio, u32 val)
{
	u32 	uret = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio)) {
		PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, gpio, uret);
	}
#endif /* DBG_GPIO */

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		uret = __LINE__;
		goto End;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	if(NULL == pchip->cfg || NULL == pchip->cfg->set_drvlevel) {
		uret = __LINE__;
		goto End;
	}

	offset = gpio - pchip->chip.base;
	if(0 != pchip->cfg->set_drvlevel(pchip, offset, val)) {
		uret = __LINE__;
		goto End;
	}

End:
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	if(0 != uret) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_setdrvlevel);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_getdrvlevel(u32 gpio)
{
	u32 	usign = 0;
	u32 	uret = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio)) {
		PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, gpio, uret);
	}
#endif /* DBG_GPIO */

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		usign = __LINE__;
		goto End;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	if(NULL == pchip->cfg || NULL == pchip->cfg->get_drvlevel) {
		usign = __LINE__;
		goto End;
	}

	offset = gpio - pchip->chip.base;
	uret = pchip->cfg->get_drvlevel(pchip, offset);

End:
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	if(0 != usign) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, usign);
		return GPIO_DRVLVL_INVALID;
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_getdrvlevel);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_suspend(void)
{
	u32	uret = 0;

	PIO_DBG_FUN_LINE_TOCHECK;
	//PIO_CHIP_LOCK(&pchip->lock, flags);
#if 0
	for(i = 0; i < ARRAY_SIZE(gpio_chips); i++) {
		if(NULL != gpio_chips[i].pm->save) {
			if(0 != gpio_chips[i].pm->save(&gpio_chips[i])) {
				uret = __LINE__;
				goto End;
			}
		}
	}

End:
	if(0 != uret) {
		PIO_ERR("%s err, line %d, i %d\n", __FUNCTION__, uret, i);
	}

#endif
	return uret;
}
EXPORT_SYMBOL(sw_gpio_suspend);

/**
 * XXX - XXX
 * XXX:	XXX
 *
 * XXX
 */
u32 sw_gpio_resume(void)
{
	u32	uret = 0;

	PIO_DBG_FUN_LINE_TOCHECK;
#if 0
	//PIO_CHIP_LOCK(&pchip->lock, flags);

	for(i = 0; i < ARRAY_SIZE(gpio_chips); i++) {
		if(NULL != gpio_chips[i].pm->resume) {
			if(0 != gpio_chips[i].pm->resume(&gpio_chips[i])) {
				uret = __LINE__;
				goto End;
			}
		}
	}

End:
	if(0 != uret) {
		PIO_ERR("%s err, line %d, i %d\n", __FUNCTION__, uret, i);
	}

#endif
	return uret;
}
EXPORT_SYMBOL(sw_gpio_resume);

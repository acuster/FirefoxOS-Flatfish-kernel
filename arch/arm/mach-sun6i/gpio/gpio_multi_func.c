/*
 * arch/arm/mach-sun6i/gpio/gpio_multi_func.c
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

/**
 * gpio_set_cfg - set multi sel for the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @val:	multi sel to set
 *
 * Returns 0 if sucess, the err line number if failed.
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

	PIO_DBG("%s: write cfg reg 0x%08x, bits_off %d, width %d, cfg_val %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_CFG, val);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG, val);
	return 0;
}

/**
 * gpio_get_cfg - get multi sel for the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns the multi sel value for the gpio
 */
u32 gpio_get_cfg(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	ureg_off = 0, ubits_off = 0;

	ureg_off = ((offset << 2) >> 5) << 2; 	/* ureg_off = ((offset * 4) / 32) * 4 */
	ubits_off = (offset << 2) % 32;		/* ubits_off = (offset * 4) % 32 */

	PIO_DBG("%s: read cfg reg 0x%08x, bits_off %d, width %d, ret %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_CFG,
		PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG));

	return PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_CFG);
}

/**
 * gpio_set_pull - set pull state for the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @val:	pull value to set
 *
 * Returns 0 if sucess, the err line number if failed.
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

	PIO_DBG("%s: write pull reg 0x%08x, bits_off %d, width %d, pul_val %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_PULL, val);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL, val);
	return 0;
}

/**
 * gpio_get_pull - get pull state for the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns the pull value for the gpio
 */
u32 gpio_get_pull(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	ureg_off = 0, ubits_off = 0;

	ureg_off = PIO_OFF_REG_PULL + (((offset << 1) >> 5) << 2); 	/* ureg_off = ((offset * 2) / 32) * 4 */
	ubits_off = (offset << 1) % 32;					/* ubits_off = (offset * 2) % 32 */

	PIO_DBG("%s: read pul reg 0x%08x, bits_off %d, width %d, ret %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_PULL,
		PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL));

	return PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_PULL);
}

/**
 * gpio_set_drvlevel - set driver level for the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @val:	driver level value to set
 *
 * Returns 0 if sucess, the err line number if failed.
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

	PIO_DBG("%s: write drvlevel reg 0x%08x, bits_off %d, width %d, drvlvl_val %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_DRVLVL, val);

	PIO_WRITE_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL, val);
	return 0;
}

/**
 * gpio_get_drvlevel - get driver level for the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns the driver level value for the gpio
 */
u32 gpio_get_drvlevel(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	ureg_off = 0, ubits_off = 0;

	ureg_off = PIO_OFF_REG_DRVLVL + (((offset << 1) >> 5) << 2); 	/* ureg_off = ((offset * 2) / 32) * 4 */
	ubits_off = (offset << 1) % 32;					/* ubits_off = (offset * 2) % 32 */

	PIO_DBG("%s: read drvlevel reg 0x%08x, bits_off %d, width %d, ret %d\n", __FUNCTION__,
		(u32)(ureg_off + pchip->vbase), ubits_off, PIO_BITS_WIDTH_DRVLVL,
		PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL));

	return PIO_READ_REG_BITS(ureg_off + pchip->vbase, ubits_off, PIO_BITS_WIDTH_DRVLVL);
}

#ifdef DBG_GPIO
/**
 * is_gpio_requested - check if gpio has been requested by gpio_request
 * @gpio:	the global gpio index
 *
 * Returns true if already requested, false otherwise.
 */
static bool is_gpio_requested(u32 gpio)
{
	struct gpio_chip *pchip = NULL;

	pchip = to_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return false;
	}

	if(NULL == gpiochip_is_requested(pchip, gpio - pchip->base))
		return false;
	else
		return true;
}
#endif /* DBG_GPIO */

/**
 * sw_gpio_setcfg - set multi sel for the gpio
 * @gpio:	the global gpio index
 * @val:	multi sel to set
 *
 * Returns 0 if sucess, the err line number if failed.
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
 * sw_gpio_getcfg - get multi sel for the gpio
 * @gpio:	the global gpio index
 *
 * Returns the multi sel value for the gpio if success, GPIO_CFG_INVALID otherwise.
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
 * sw_gpio_setpull - set pull state for the gpio
 * @gpio:	the global gpio index
 * @val:	pull value to set
 *
 * Returns 0 if sucess, the err line number if failed.
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
 * sw_gpio_getpull - get pull state for the gpio
 * @gpio:	the global gpio index
 *
 * Returns the pull value for the gpio if success, GPIO_PULL_INVALID otherwise.
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
 * sw_gpio_setdrvlevel - set driver level for the gpio
 * @gpio:	the global gpio index
 * @val:	driver level to set
 *
 * Returns 0 if sucess, the err line number if failed.
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
 * sw_gpio_getpull - get driver level for the gpio
 * @gpio:	the global gpio index
 *
 * Returns the driver level for the gpio if success, GPIO_DRVLVL_INVALID otherwise.
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
 * sw_gpio_set_config - config a group of pin, include multi sel, pull, driverlevel
 * @pcfg:	the config value group
 * @cfg_num:	gpio number to config, also pcfg's member number
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_set_config(struct gpio_config *pcfg, u32 cfg_num)
{
	u32 	i = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	for(i = 0; i < cfg_num; i++, pcfg++) {
#ifdef DBG_GPIO
		if(false == is_gpio_requested(pcfg->gpio)) {
			PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, pcfg->gpio, __LINE__);
		}
#endif /* DBG_GPIO */
		pchip = gpio_to_aw_gpiochip(pcfg->gpio);
		if(NULL == pchip) {
			PIO_ERR_FUN_LINE;
			return __LINE__;
		}

		PIO_CHIP_LOCK(&pchip->lock, flags);

		offset = pcfg->gpio - pchip->chip.base;
		PIO_ASSERT(0 == pchip->cfg->set_cfg(pchip, offset, pcfg->mul_sel));
		PIO_ASSERT(0 == pchip->cfg->set_pull(pchip, offset, pcfg->pull));
		PIO_ASSERT(0 == pchip->cfg->set_drvlevel(pchip, offset, pcfg->drv_level));

		PIO_CHIP_UNLOCK(&pchip->lock, flags);
	}

	return 0;
}
EXPORT_SYMBOL(sw_gpio_set_config);

/**
 * sw_gpio_get_config - get the config state for a group of pin,
 *			include multi sel, pull, driverlevel
 * @pcfg:	store the config information for pins
 * @cfg_num:	number of the pins
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_get_config(struct gpio_config *pcfg, u32 cfg_num)
{
	u32 	i = 0;
	u32 	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	for(i = 0; i < cfg_num; i++, pcfg++) {
#ifdef DBG_GPIO
		if(false == is_gpio_requested(pcfg->gpio)) {
			PIO_ERR("%s to check: gpio %d not requested, line %d\n", __FUNCTION__, pcfg->gpio, __LINE__);
		}
#endif /* DBG_GPIO */
		pchip = gpio_to_aw_gpiochip(pcfg->gpio);
		if(NULL == pchip) {
			PIO_ERR_FUN_LINE;
			return __LINE__;
		}

		PIO_CHIP_LOCK(&pchip->lock, flags);

		offset = pcfg->gpio - pchip->chip.base;
		pcfg->mul_sel = pchip->cfg->get_cfg(pchip, offset);
		pcfg->pull = pchip->cfg->get_pull(pchip, offset);
		pcfg->drv_level = pchip->cfg->get_drvlevel(pchip, offset);

		PIO_CHIP_UNLOCK(&pchip->lock, flags);
	}

	return 0;
}
EXPORT_SYMBOL(sw_gpio_get_config);

/**
 * sw_gpio_dump_config - dump config info for a group of pins
 * @pcfg:	store the config information for pins
 * @cfg_num:	number of the pins
 */
void sw_gpio_dump_config(struct gpio_config *pcfg, u32 cfg_num)
{
	u32 	i = 0;

	PIO_DBG("+++++++++++%s+++++++++++\n", __FUNCTION__);
	PIO_DBG("  port    mul_sel    pull    drvlevl\n");
	for(i = 0; i < cfg_num; i++, pcfg++) {
		PIO_DBG("  %4d    %7d    %4d    %7d\n", pcfg->gpio, pcfg->mul_sel, pcfg->pull, pcfg->drv_level);
	}
	PIO_DBG("-----------%s-----------\n", __FUNCTION__);
}
EXPORT_SYMBOL(sw_gpio_dump_config);

/**
 * sw_gpio_suspend - save somethig for gpio before enter sleep
 *
 * Returns 0 if sucess, the err line number if failed.
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
 * sw_gpio_suspend - restore somethig for gpio after wake up
 *
 * Returns 0 if sucess, the err line number if failed.
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

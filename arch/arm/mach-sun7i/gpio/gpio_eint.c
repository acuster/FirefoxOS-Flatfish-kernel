/*
 * arch/arm/mach-sun7i/gpio/gpio_eint.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
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
 * gpio_canbe_eint - check if gpio canbe configured as eint
 * @gpio:	the global gpio index
 *
 * return true if the gpio canbe configured as eint, false otherwise.
 */
bool gpio_canbe_eint(u32 gpio)
{
	int 	i = 0;
	u32 	gpio_eint_group[][2] = {
		{GPIOH(0), 	GPIOH(21)},
		{GPIOI(10), 	GPIOI(19)}
	};

	for(i = 0; i < ARRAY_SIZE(gpio_eint_group); i++)
		if(gpio >= gpio_eint_group[i][0]
			&& gpio <= gpio_eint_group[i][1])
			return true;

	return false;
}

/**
 * __para_check - check if gpio requested or canbe configured as eint
 * @gpio:	the global gpio index
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __para_check(u32 gpio)
{
#ifdef DBG_GPIO
	if(false == is_gpio_requested(gpio))
		return __LINE__;
#endif /* DBG_GPIO */
	if(false == gpio_canbe_eint(gpio))
		return __LINE__;
	return 0;
}

/**
 * __is_gpio_i - check if gpio is in GPIO I.
 * @gpio:	the global gpio index
 *
 * return true if the gpio is in GPIO I, false otherwise.
 */
u32 inline __is_gpio_i(u32 gpio)
{
	return (gpio >= GPIOI(0) && gpio <= GPIOI(21));
}

/**
 * gpio_eint_set_trig - set trig type of the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @trig_val:	the trig type to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_set_trig(struct aw_gpio_chip *pchip, u32 offset, enum gpio_eint_trigtype trig_val)
{
	u32 	reg_off, bits_off;

	reg_off = ((offset << 2) >> 5) << 2; /* (offset * 4) / 32 * 4 */
	bits_off = (offset << 2) & ((1 << 5) - 1); /* (offset * 4) % 32 */

#ifdef DBG_GPIO
	PIO_ASSERT(trig_val < TRIG_INALID);
	PIO_DBG("%s: chip 0x%08x, offset %d, write reg 0x%08x, bits off %d, val %d\n", __FUNCTION__,
		(u32)pchip, offset, (u32)pchip->vbase_eint + reg_off, bits_off, (u32)trig_val);
#endif /* DBG_GPIO */
	PIO_WRITE_REG_BITS(pchip->vbase_eint + reg_off, bits_off, 4, (u32)trig_val);
	return 0;
}

/**
 * gpio_eint_get_trig - get trig type of the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @pval:	the trig type got
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_get_trig(struct aw_gpio_chip *pchip, u32 offset, enum gpio_eint_trigtype *pval)
{
	u32 	reg_off, bits_off;

	reg_off = ((offset << 2) >> 5) << 2; /* (offset * 4) / 32 * 4 */
	bits_off = (offset << 2) & ((1 << 5) - 1); /* (offset * 4) % 32 */

	*pval = (enum gpio_eint_trigtype)PIO_READ_REG_BITS(pchip->vbase_eint + reg_off, bits_off, 4);
#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x, bits off %d, ret val %d\n", __FUNCTION__,
		(u32)pchip, offset, (u32)pchip->vbase_eint + reg_off,
		PIO_READ_REG((u32)pchip->vbase_eint + reg_off), bits_off, (u32)*pval);
	PIO_ASSERT(*pval < TRIG_INALID);
#endif /* DBG_GPIO */
	return 0;
}

/**
 * gpio_eint_set_enable - enable/disable the gpio eint
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @enable:	1 - enable the eint, 0 - disable the eint.
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_set_enable(struct aw_gpio_chip *pchip, u32 offset, u32 enable)
{
#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, enable %d, write reg 0x%08x\n", __FUNCTION__,
		(u32)pchip, offset, enable, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL);
#endif /* DBG_GPIO */

	if(0 != enable)
		PIO_SET_BIT(pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL, offset);
	else
		PIO_CLR_BIT(pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL, offset);
	return 0;
}

/**
 * gpio_eint_get_enable - get the gpio eint's enable/disable satus
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 * @penable:	status got, 1 - the eint is enabled, 0 - disabled
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_get_enable(struct aw_gpio_chip *pchip, u32 offset, u32 *penable)
{
	PIO_ASSERT(NULL != penable);
	*penable = PIO_READ_REG_BITS(pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL, offset, 1);
#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x, penable 0x%08x, *penable %d\n", __FUNCTION__,
		(u32)pchip, offset, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL,
		PIO_READ_REG((u32)pchip->vbase_eint + PIO_EINT_OFF_REG_CTRL), (u32)penable, *penable);
#endif /* DBG_GPIO */
	return 0;
}

/**
 * gpio_eint_get_irqpd_sta - get the irqpend status of the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns the irqpend status of the gpio. 1 - irq pend, 0 - no irq pend.
 */
u32 gpio_eint_get_irqpd_sta(struct aw_gpio_chip *pchip, u32 offset)
{
	u32	uret = 0;

	uret = PIO_READ_REG_BITS(pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS, offset, 1);
#ifdef DBG_GPIO
	PIO_DBG("%s: chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x, ret %d\n", __FUNCTION__,
		(u32)pchip, offset, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS,
		PIO_READ_REG(pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS), uret);
#endif /* DBG_GPIO */
	return uret;
}

/**
 * gpio_eint_clr_irqpd_sta - clr the irqpend status of the gpio
 * @chip:	aw_gpio_chip struct for the gpio
 * @offset:	offset from gpio_chip->base
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_clr_irqpd_sta(struct aw_gpio_chip *pchip, u32 offset)
{
	u32 	utemp = 0;
	u32 	ureg_addr = (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_STATUS;

	utemp = PIO_READ_REG_BITS(ureg_addr, offset, 1);

#ifdef DBG_GPIO
	PIO_DBG("%s: irq_pend %d, chip 0x%08x, offset %d, read reg 0x%08x - 0x%08x\n", __FUNCTION__,
		utemp, (u32)pchip, offset, ureg_addr, PIO_READ_REG(ureg_addr));
#endif /* DBG_GPIO */

	if(1 == utemp) {
		/* bug: clear all pending bits, but only need clear the offset bit here */
		//PIO_WRITE_REG_BITS(ureg_addr, offset, 1, 1);

		PIO_WRITE_REG(ureg_addr, 1 << offset);
	}

#ifdef DBG_GPIO
	if(1 == utemp) { /* to see if corrently cleared */
		utemp = PIO_READ_REG_BITS(ureg_addr, offset, 1);
		PIO_DBG("%s: after cleared, irq_pend %d, read reg 0x%08x - 0x%08x\n", __FUNCTION__,
			utemp, ureg_addr, PIO_READ_REG(ureg_addr));
	}
#endif /* DBG_GPIO */

	return 0;
}

/**
 * gpio_eint_set_debounce - set the debounce of the gpio group
 * @chip:	aw_gpio_chip struct for the gpio
 * @val:	debounce to set.
 *
 * for eint group, not for single port
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_set_debounce(struct aw_gpio_chip *pchip, struct gpio_eint_debounce val)
{
	u32 	utemp = 0;

	utemp = (val.clk_sel & 1) | ((val.clk_pre_scl & 0b111) << 4);

#ifdef DBG_GPIO
	PIO_DBG("%s: clk_sel %d, clk_pre_scl %d, write 0x%08x to reg 0x%08x\n", __FUNCTION__, val.clk_sel,
		val.clk_pre_scl, utemp, (u32)pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE);
#endif /* DBG_GPIO */

	PIO_WRITE_REG(pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE, utemp);
	return 0;
}

/**
 * gpio_eint_get_debounce - get the debounce of the gpio group
 * @chip:	aw_gpio_chip struct for the gpio
 * @val:	debounce got.
 *
 * for eint group, not for single port
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_eint_get_debounce(struct aw_gpio_chip *pchip, struct gpio_eint_debounce *pval)
{
	u32 	utemp = 0;

	PIO_ASSERT(NULL != pval);
	utemp = PIO_READ_REG(pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE);
	pval->clk_sel = utemp & 1;
	pval->clk_pre_scl = (utemp >> 4) & 0b111;
#ifdef DBG_GPIO
	PIO_DBG("%s: read from reg 0x%08x - 0x%08x, clk_sel %d, clk_pre_scl %d\n", __FUNCTION__,
		(u32)pchip->vbase_eint + PIO_EINT_OFF_REG_DEBOUNCE,
		utemp, pval->clk_sel, pval->clk_pre_scl);
#endif /* DBG_GPIO */
	return 0;
}

/**
 * sw_gpio_eint_set_trigtype - set trig type of the gpio
 * @gpio:	the global gpio index
 * @trig_type:	the trig type to set
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_set_trigtype(u32 gpio, enum gpio_eint_trigtype trig_type)
{
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	PIO_DBG("%s: gpio 0x%08x, trig_type %d\n", __FUNCTION__, gpio, (u32)trig_type);

	if(0 != __para_check(gpio)
		|| trig_type >= TRIG_INALID) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	if(__is_gpio_i(gpio))
		offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
	else
		offset = gpio - pchip->chip.base;

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_set_trig(pchip, offset, trig_type);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_set_trigtype);

/**
 * sw_gpio_eint_get_trigtype - get trig type of the gpio
 * @gpio:	the global gpio index
 * @pval:	the trig type got
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_get_trigtype(u32 gpio, enum gpio_eint_trigtype *pval)
{
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	if(__is_gpio_i(gpio))
		offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
	else
		offset = gpio - pchip->chip.base;

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_get_trig(pchip, offset, pval);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);

	PIO_DBG("%s: gpio 0x%08x, trig_type ret %d\n", __FUNCTION__, gpio, (u32)*pval);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_trigtype);

/**
 * sw_gpio_eint_set_enable - enable/disable the gpio eint
 * @gpio:	the global gpio index
 * @enable:	1 - enable the eint, 0 - disable the eint.
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_set_enable(u32 gpio, u32 enable)
{
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	PIO_DBG("%s: gpio 0x%08x, enable %d\n", __FUNCTION__, gpio, enable);
	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	if(__is_gpio_i(gpio))
		offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
	else
		offset = gpio - pchip->chip.base;

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_set_enable(pchip, offset, enable);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_set_enable);

/**
 * sw_gpio_eint_get_enable - get the gpio eint's enable/disable satus
 * @gpio:	the global gpio index
 * @penable:	status got, 1 - the eint is enabled, 0 - disabled
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_get_enable(u32 gpio, u32 *penable)
{
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	if(__is_gpio_i(gpio))
		offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
	else
		offset = gpio - pchip->chip.base;

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_get_enable(pchip, offset, penable);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);

	PIO_DBG("%s: gpio 0x%08x, penable ret %d\n", __FUNCTION__, gpio, *penable);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_enable);

/**
 * sw_gpio_eint_get_irqpd_sta - get the irqpend status of the gpio
 * @gpio:	the global gpio index
 *
 * Returns the irqpend status of the gpio. 1 - irq pend, 0 - no irq pend.
 */
u32 sw_gpio_eint_get_irqpd_sta(u32 gpio)
{
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return 0; /* note here */
	}

	if(__is_gpio_i(gpio))
		offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
	else
		offset = gpio - pchip->chip.base;

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_get_irqpd_sta(pchip, offset);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);

	PIO_DBG("%s: gpio 0x%08x, ret %d\n", __FUNCTION__, gpio, uret);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_irqpd_sta);

/**
 * sw_gpio_eint_clr_irqpd_sta - clr the irqpend status of the gpio
 * @gpio:	the global gpio index
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_clr_irqpd_sta(u32 gpio)
{
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	if(__is_gpio_i(gpio))
		offset = gpio - pchip->chip.base - PI_EINT_OFFSET + PI_EINT_START_INDEX; /* PI10 offset is 22 */
	else
		offset = gpio - pchip->chip.base;

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_clr_irqpd_sta(pchip, offset);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_clr_irqpd_sta);

/**
 * sw_gpio_eint_get_debounce - get the debounce of the gpio group
 * @gpio:	the global gpio index
 * @pdbc:	debounce got.
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_get_debounce(u32 gpio, struct gpio_eint_debounce *pdbc)
{
	u32	uret = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_get_debounce(pchip, pdbc);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_get_debounce);

/**
 * sw_gpio_eint_set_debounce - set the debounce of the gpio group
 * @gpio:	the global gpio index
 * @dbc:	debounce to set.
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_set_debounce(u32 gpio, struct gpio_eint_debounce dbc)
{
	u32	uret = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	if(0 != __para_check(gpio)) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	pchip = gpio_to_aw_gpiochip(gpio);
	if(NULL == pchip) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	PIO_CHIP_LOCK(&pchip->lock, flags);
	uret = pchip->cfg_eint->eint_set_debounce(pchip, dbc);
	PIO_CHIP_UNLOCK(&pchip->lock, flags);
	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_set_debounce);

/**
 * sw_gpio_eint_setall_range - config a range of gpio, config mul sel to eint,
 * 	set driver level and pull, set the trig mode, and enable eint.
 * @pcfg:	config info to set.
 * @cfg_num:	member cnt of pcfg
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_setall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num)
{
	u32 	i = 0;
	u32	uret = 0;
	u32	offset = 0;
	u32	mulsel_eint = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	for(i = 0; i < cfg_num; i++, pcfg++) {
		if(0 != __para_check(pcfg->gpio)
			|| pcfg->trig_type >= TRIG_INALID) {
			PIO_ERR("%s err: line %d, gpio %d\n", __FUNCTION__, __LINE__, pcfg->gpio);
			return __LINE__;
		}

		pchip = gpio_to_aw_gpiochip(pcfg->gpio);
		if(NULL == pchip) {
			PIO_ERR("%s err: line %d, gpio_to_aw_gpiochip(%d) return NULL\n",
				__FUNCTION__, __LINE__, pcfg->gpio);
			return __LINE__;
		}

		offset = pcfg->gpio - pchip->chip.base;
		PIO_DBG("%s: gpio %d, base %d, offset %d\n", __FUNCTION__, pcfg->gpio,
			pchip->chip.base, offset);

		PIO_CHIP_LOCK(&pchip->lock, flags);

		/* set mul sel to eint, and set pull and drvlvl */
		mulsel_eint = GPIO_CFG_EINT;
		PIO_ASSERT(0 == pchip->cfg->set_cfg(pchip, offset, mulsel_eint));
		PIO_ASSERT(0 == pchip->cfg->set_pull(pchip, offset, pcfg->pull));
		PIO_ASSERT(0 == pchip->cfg->set_drvlevel(pchip, offset, pcfg->drvlvl));

		/* PI10 offset is 22 */
		if(__is_gpio_i(pcfg->gpio)) {
			PIO_DBG_FUN_LINE_TOCHECK;
			offset = offset - PI_EINT_OFFSET + PI_EINT_START_INDEX;
		}

		/* set trig type */
		pchip->cfg_eint->eint_set_trig(pchip, offset, pcfg->trig_type);

		/* enable/disable eint */
		pchip->cfg_eint->eint_set_enable(pchip, offset, pcfg->enabled);

		/* clr the irqpd status */
		if(0 != pcfg->irq_pd)
			pchip->cfg_eint->eint_clr_irqpd_sta(pchip, offset);

		PIO_CHIP_UNLOCK(&pchip->lock, flags);
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_setall_range);

/**
 * sw_gpio_eint_getall_range - get a range of gpio's eint info
 * @pcfg:	config info got.
 * @cfg_num:	member cnt of pcfg
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_eint_getall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num)
{
	u32 	i = 0;
	u32	uret = 0;
	u32	offset = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	for(i = 0; i < cfg_num; i++, pcfg++) {
		if(0 != __para_check(pcfg->gpio)) {
			PIO_ERR("%s err: line %d, gpio %d\n", __FUNCTION__, __LINE__, pcfg->gpio);
			return __LINE__;
		}

		pchip = gpio_to_aw_gpiochip(pcfg->gpio);
		if(NULL == pchip) {
			PIO_ERR("%s err: line %d, gpio_to_aw_gpiochip(%d) return NULL\n",
				__FUNCTION__, __LINE__, pcfg->gpio);
			return __LINE__;
		}

		offset = pcfg->gpio - pchip->chip.base;

		PIO_CHIP_LOCK(&pchip->lock, flags);

		/* verify mul sel to eint, and get pull and drvlvl */
		PIO_ASSERT(GPIO_CFG_EINT == pchip->cfg->get_cfg(pchip, offset));
		pcfg->pull = pchip->cfg->get_pull(pchip, offset);
		pcfg->drvlvl = pchip->cfg->get_drvlevel(pchip, offset);

		/* redirect offset for eint op, for the case that r_pl_5 is PL_EINT0 */
		if(__is_gpio_i(pcfg->gpio)) {
			PIO_DBG_FUN_LINE_TOCHECK;
			offset = offset - PI_EINT_OFFSET + PI_EINT_START_INDEX;
		}

		/* set trig type */
		pchip->cfg_eint->eint_get_trig(pchip, offset, &pcfg->trig_type);
		PIO_ASSERT(pcfg->trig_type < TRIG_INALID);

		/* get enable/disable status */
		pchip->cfg_eint->eint_get_enable(pchip, offset, &pcfg->enabled);

		/* get the irqpd status */
		pcfg->irq_pd = pchip->cfg_eint->eint_get_irqpd_sta(pchip, offset);

		PIO_CHIP_UNLOCK(&pchip->lock, flags);
	}

	return uret;
}
EXPORT_SYMBOL(sw_gpio_eint_getall_range);

/**
 * sw_gpio_eint_dumpall_range - dump a range of gpio's eint config info.
 * @pcfg:	config info to dump.
 * @cfg_num:	member cnt of pcfg
 *
 */
void sw_gpio_eint_dumpall_range(struct gpio_config_eint_all *pcfg, u32 cfg_num)
{
	u32 	i = 0;
	unsigned long flags = 0;
	struct aw_gpio_chip *pchip = NULL;

	PIO_DBG("+++++++++++%s+++++++++++\n", __FUNCTION__);
	PIO_DBG("  gpio    pull    drvlevl   enabled  irq_pd   trig_type\n");
	for(i = 0; i < cfg_num; i++, pcfg++) {
		pchip = gpio_to_aw_gpiochip(pcfg->gpio);
		if(NULL == pchip) {
			PIO_ERR("%s err: line %d, gpio_to_aw_gpiochip(%d) return NULL\n",
				__FUNCTION__, __LINE__, pcfg->gpio);
			continue;
		}

		PIO_CHIP_LOCK(&pchip->lock, flags);
		PIO_DBG("  %4d    %4d    %7d   %7d  %6d   %9d\n", pcfg->gpio, pcfg->pull, pcfg->drvlvl,
			pcfg->enabled, pcfg->irq_pd, pcfg->trig_type);
		PIO_CHIP_UNLOCK(&pchip->lock, flags);
	}
	PIO_DBG("-----------%s-----------\n", __FUNCTION__);
}
EXPORT_SYMBOL(sw_gpio_eint_dumpall_range);

irqreturn_t gpio_irq_hdl(int irq, void *dev)
{
	u32 	gpio = 0;
	u32 	ret = 0;
	struct gpio_irq_handle *pdev_id = (struct gpio_irq_handle *)dev;

	/* to do: sync pdev_id for gpio_irq_hdl/sw_gpio_irq_free with mutex */
	PIO_DBG_FUN_LINE_TODO;

	gpio = pdev_id->gpio;
	if(0 == sw_gpio_eint_get_irqpd_sta(gpio))
		return IRQ_NONE;

	sw_gpio_eint_clr_irqpd_sta(gpio);

	ret = pdev_id->handler(pdev_id->parg);

	PIO_ASSERT(0 == ret);
	return IRQ_HANDLED;
}

/**
 * sw_gpio_irq_request - request gpio irq.
 * @gpio:	the global gpio index
 * @trig_type:	trig type of gpio eint
 * @handle:	irq callback
 * @para:	para of the handle function.
 *
 * Returns the handle if sucess, 0 if failed.
 */
u32 sw_gpio_irq_request(u32 gpio, enum gpio_eint_trigtype trig_type,
			peint_handle handle, void *para)
{
	int 	irq_no = 0;
	int 	irq_ret = 0;
	u32 	usign = 0;
	struct gpio_config_eint_all cfg = {0};
	struct gpio_irq_handle *pdev_id = NULL;

	PIO_DBG("%s: gpio %d, trig %d, handle 0x%08x, para 0x%08x\n", __FUNCTION__,
		gpio, trig_type, (u32)handle, (u32)para);

	if(0 != gpio_request(gpio, NULL)) {
		PIO_ERR("%s err: request gpio %d failed, line %d\n", __FUNCTION__, gpio, __LINE__);
		return 0;
	}

	if(false == gpio_canbe_eint(gpio)) {
		usign = __LINE__;
		goto End;
	}

	/* config to eint, enable the eint, and set pull, drivel level, trig type */
	cfg.gpio 	= gpio;
	PIO_DBG_FUN_LINE_TOCHECK;
	cfg.pull 	= GPIO_PULL_DEFAULT;
	cfg.drvlvl 	= GPIO_DRVLVL_DEFAULT;
	cfg.enabled	= 1; /* to modify: disable -> config -> enable */
	cfg.trig_type	= trig_type;
	if(0 != sw_gpio_eint_setall_range(&cfg, 1)) {
		usign = __LINE__;
		goto End;
	}

	pdev_id = (struct gpio_irq_handle *)kmalloc(sizeof(struct gpio_irq_handle), GFP_KERNEL);
	if(NULL == pdev_id) {
		usign = __LINE__;
		goto End;
	}
	pdev_id->gpio = gpio;
	pdev_id->handler = handle;
	pdev_id->parg = para;

	irq_no = __gpio_to_irq(gpio);
	PIO_DBG("%s: __gpio_to_irq return %d\n", __FUNCTION__, irq_no);

	irq_ret = request_irq(irq_no, gpio_irq_hdl, IRQF_DISABLED | IRQF_SHARED, "gpio_irq", (void *)pdev_id);
	if(irq_ret) {
		usign = __LINE__;
		goto End;
	}
End:
	if(0 != usign) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, usign);

		gpio_free(gpio);

		if(NULL == pdev_id)
			kfree(pdev_id);
		pdev_id = NULL;
	}
	return (u32)pdev_id;
}
EXPORT_SYMBOL(sw_gpio_irq_request);

/**
 * sw_gpio_irq_free - free gpio irq.
 * @handle:	handle return by sw_gpio_irq_request
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 sw_gpio_irq_free(u32 handle)
{
	u32 	gpio = 0;
	int 	irq_no = 0;
	struct gpio_irq_handle *pdev_id = (struct gpio_irq_handle *)handle;

	PIO_DBG("%s: handle 0x%08x\n", __FUNCTION__, (u32)handle);

	/* to do: sync pdev_id for gpio_irq_hdl/sw_gpio_irq_free with mutex */
	PIO_DBG_FUN_LINE_TODO;

	if(NULL == pdev_id) {
		PIO_ERR_FUN_LINE;
		return __LINE__;
	}

	gpio = pdev_id->gpio;
	sw_gpio_eint_set_enable(gpio, 0);
	sw_gpio_eint_clr_irqpd_sta(gpio);

	irq_no = __gpio_to_irq(gpio);
	PIO_DBG("%s: __gpio_to_irq(%d) ret %d\n", __FUNCTION__, gpio, irq_no);
	free_irq(irq_no, (void *)pdev_id);

	kfree((void *)pdev_id);

	gpio_free(gpio);
	return 0;
}
EXPORT_SYMBOL(sw_gpio_irq_free);

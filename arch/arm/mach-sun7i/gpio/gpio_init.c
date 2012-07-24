/*
 * arch/arm/mach-sun7i/gpio/gpio_init.c
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
 * gpio_save - save somethig for the chip before enter sleep
 * @chip:	aw_gpio_chip which will be saved
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_save(struct aw_gpio_chip *pchip)
{
	/* save something before suspend */
	PIO_DBG_FUN_LINE_TODO;

	return 0;
}

/**
 * gpio_resume - restore somethig for the chip after wake up
 * @chip:	aw_gpio_chip which will be saved
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 gpio_resume(struct aw_gpio_chip *pchip)
{
	/* restore something after wakeup */
	PIO_DBG_FUN_LINE_TODO;

	return 0;
}

/*
 * gpio power api struct
 */
struct gpio_pm_t g_pm = {
	gpio_save,
	gpio_resume
};

/*
 * gpio config api struct
 */
struct gpio_cfg_t g_cfg = {
	gpio_set_cfg,
	gpio_get_cfg,
	gpio_set_pull,
	gpio_get_pull,
	gpio_set_drvlevel,
	gpio_get_drvlevel,
};

/*
 * gpio eint config api struct
 */
struct gpio_eint_cfg_t g_eint_cfg = {
	gpio_eint_set_trig,
	gpio_eint_get_trig,
	gpio_eint_set_enable,
	gpio_eint_get_enable,
	gpio_eint_get_irqpd_sta,
	gpio_eint_clr_irqpd_sta,
	gpio_eint_set_debounce,
	gpio_eint_get_debounce,
};

/*
 * gpio chips for the platform
 */
struct aw_gpio_chip gpio_chips[] = {
	{
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PA_NR_BASE,
			.ngpio	= PA_NR,
			.label	= "GPA",
		},
		.vbase  = (void __iomem *)PIO_VBASE(0),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PB_NR_BASE,
			.ngpio	= PB_NR,
			.label	= "GPB",
		},
		.vbase  = (void __iomem *)PIO_VBASE(1),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PC_NR_BASE,
			.ngpio	= PC_NR,
			.label	= "GPC",
		},
		.vbase  = (void __iomem *)PIO_VBASE(2),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PD_NR_BASE,
			.ngpio	= PD_NR,
			.label	= "GPD",
		},
		.vbase  = (void __iomem *)PIO_VBASE(3),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PE_NR_BASE,
			.ngpio	= PE_NR,
			.label	= "GPE",
		},
		.vbase  = (void __iomem *)PIO_VBASE(4),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PF_NR_BASE,
			.ngpio	= PF_NR,
			.label	= "GPF",
		},
		.vbase  = (void __iomem *)PIO_VBASE(5),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PG_NR_BASE,
			.ngpio	= PG_NR,
			.label	= "GPG",
		},
		.vbase  = (void __iomem *)PIO_VBASE(6),
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PH_NR_BASE,
			.ngpio	= PH_NR,
			.label	= "GPH",
			.to_irq = __pio_to_irq,
		},
		.vbase  = (void __iomem *)PIO_VBASE(7),
		/* cfg for eint */
		.irq_num = AW_IRQ_GPIO,
		.vbase_eint = (void __iomem *)PIO_VBASE_EINT,
		.cfg_eint = &g_eint_cfg,
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PI_NR_BASE,
			.ngpio	= PI_NR,
			.label	= "GPI",
			.to_irq = __pio_to_irq,
		},
		.vbase  = (void __iomem *)PIO_VBASE(8),
		/* cfg for eint */
		.irq_num = AW_IRQ_GPIO,
		.vbase_eint = (void __iomem *)PIO_VBASE_EINT,
		.cfg_eint = &g_eint_cfg,
	}
};

#ifdef PIO_FROM_SD_TESTCODE
static void __iomem *g_ccm_reg_vbase = 0;
static void __iomem *g_r_prcm_reg_vbase = 0;
#include "pio_clkinit_from_sdtest.c"
#endif /* PIO_FROM_SD_TESTCODE */

/**
 * aw_gpio_init - gpio driver init function
 *
 * Returns 0 if sucess, the err line number if failed.
 */
static __init int aw_gpio_init(void)
{
	u32	uret = 0;
	u32 	i = 0;

#ifdef PIO_FROM_SD_TESTCODE
	g_ccm_reg_vbase = ioremap_nocache(AW_CCM_BASE, 0x1000);
	g_r_prcm_reg_vbase = ioremap_nocache(AW_R_PRCM_BASE, 0x1000);
	PIO_DBG("%s: g_ccm_reg_vbase 0x%08x, g_r_prcm_reg_vbase 0x%08x\n", __FUNCTION__,
		(u32)g_ccm_reg_vbase, (u32)g_r_prcm_reg_vbase);
	PIO_DBG("%s: NOTE - r_gpio currently NOT pass tested############################\n", __FUNCTION__);
	gpio_clk_init();
	r_gpio_clk_init();
#endif /* PIO_FROM_SD_TESTCODE */

	for(i = 0; i < ARRAY_SIZE(gpio_chips); i++) {
		/* lock init */
		PIO_CHIP_LOCK_INIT(&gpio_chips[i].lock);

		/* register gpio_chip */
		if(0 != aw_gpiochip_add(&gpio_chips[i].chip)) {
			uret = __LINE__;
			goto End;
		}
	}

End:
	if(0 != uret) {
		PIO_ERR("%s err, line %d\n", __FUNCTION__, uret);
	}

	return uret;
}

core_initcall(aw_gpio_init);

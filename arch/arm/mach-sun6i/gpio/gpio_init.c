/*
 * arch/arm/mach-sun6i/gpio/gpio_init.c
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

u32 	g_pio_vbase = 0;
u32 	g_rpio_vbase = 0;

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
	gpio_get_drvlevel
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
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PB_NR_BASE,
			.ngpio	= PB_NR,
			.label	= "GPB",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PC_NR_BASE,
			.ngpio	= PC_NR,
			.label	= "GPC",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PD_NR_BASE,
			.ngpio	= PD_NR,
			.label	= "GPD",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PE_NR_BASE,
			.ngpio	= PE_NR,
			.label	= "GPE",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PF_NR_BASE,
			.ngpio	= PF_NR,
			.label	= "GPF",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PG_NR_BASE,
			.ngpio	= PG_NR,
			.label	= "GPG",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PH_NR_BASE,
			.ngpio	= PH_NR,
			.label	= "GPH",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PL_NR_BASE,
			.ngpio	= PL_NR,
			.label	= "GPL",
		},
	}, {
		.cfg	= &g_cfg,
		.pm	= &g_pm,
		.chip	= {
			.base	= PM_NR_BASE,
			.ngpio	= PM_NR,
			.label	= "GPM",
		},
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

	g_pio_vbase = (u32)ioremap_nocache(AW_PIO_BASE, 0x400);
	g_rpio_vbase = (u32)ioremap_nocache(AW_RPIO_BASE, 0x400);
	PIO_ASSERT(0 != g_pio_vbase && 0 != g_rpio_vbase);

	for(i = 0; i < ARRAY_SIZE(gpio_chips); i++) {
		/* do some extra intils */
		//PIO_DBG_FUN_LINE_TODO;

		PIO_CHIP_LOCK_INIT(&gpio_chips[i].lock);

		if(i < 8) /* PA ~ PH */
			gpio_chips[i].vbase = (void __iomem *)PIO_VBASE(i);
		else if(i < 10)
			gpio_chips[i].vbase = (void __iomem *)RPIO_VBASE(i - 8);
		else {
			uret = __LINE__;
			goto End;
		}

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

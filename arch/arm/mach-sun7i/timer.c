/*
 * arch\arm\mach-aw163x\timer.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin<huangxin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <asm/sched_clock.h>
#include <mach/includes.h>

#undef TIMER_DBG
#undef TIMER_ERR
#if (0)
    #define TIMER_DBG(format,args...)  printk("[CLKSRC] "format,##args)
    #define TIMER_ERR(format,args...)  printk("[CLKSRC] "format,##args)
#else
    #define TIMER_DBG(...)
    #define TIMER_ERR(...)
#endif

static cycle_t aw_clksrc_read(struct clocksource *cs);

static struct clocksource aw_clocksrc =
{
    .name = "aw 64bits couter",
    .list = {NULL, NULL},
    .rating = 300,                  /* perfect clock source             */
    .read = aw_clksrc_read,         /* read clock counter               */
    .enable = 0,                    /* not define                       */
    .disable = 0,                   /* not define                       */
    .mask = CLOCKSOURCE_MASK(64),   /* 64bits mask                      */
    .mult = 0,                      /* it will be calculated by shift   */
    .shift = 32,                    /* 32bit shift for                  */
    .max_idle_ns = 1000000000000ULL,
    .flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

static cycle_t aw_clksrc_read(struct clocksource *cs)
{
    unsigned long   flags;
    __u32           lower, upper;

    /* disable interrupt response */
    raw_local_irq_save(flags);

    /* latch 64bit counter and wait ready for read */
    TMR_REG_CNT64_CTL |= (1<<1);
    while(TMR_REG_CNT64_CTL & (1<<1));

    /* read the 64bits counter */
    lower = TMR_REG_CNT64_LO;
    upper = TMR_REG_CNT64_HI;

    /* restore interrupt response */
    raw_local_irq_restore(flags);

    return (((__u64)upper)<<32) | lower;
}

void __init aw_clksrc_init(void)
{
	TIMER_DBG("all-winners clock source init!\n");
    /* we use 64bits counter as HPET(High Precision Event Timer) */
    TMR_REG_CNT64_CTL  = 0;
    /* config clock source for 64bits counter */
//    #if(AW_HPET_CLK_SRC == TMR_CLK_SRC_24MHOSC)
//        TMR_REG_CNT64_CTL |= (0<<2);
//    #else
        TMR_REG_CNT64_CTL |= (1<<2);
//    #endif
    /* clear 64bits counter */
    TMR_REG_CNT64_CTL |= (1<<0);
    TIMER_DBG("register all-winners clock source!\n");
    /* calculate the mult by shift  */
    aw_clocksrc.mult = clocksource_hz2mult(AW_HPET_CLOCK_SOURCE_HZ, aw_clocksrc.shift);
    /* register clock source */
    clocksource_register(&aw_clocksrc);
}

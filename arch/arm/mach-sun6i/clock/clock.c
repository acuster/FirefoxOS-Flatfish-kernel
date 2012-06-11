/*
 *  arch/arm/mach-sun6i/clock/clock.c
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * kevin.z.m (kevin@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>
#include <mach/clock.h>
#include "ccm_i.h"

// alloc memory for store clock informatioin
static __ccu_clk_t          aw_clock[AW_CCU_CLK_CNT];
static struct clk_lookup    lookups[AW_CCU_CLK_CNT];



/*
*********************************************************************************************************
*                           clk_init
*
*Description: clock management initialise.
*
*Arguments  : none
*
*Return     : result
*               0,  initialise successed;
*              -1,  initialise failed;
*
*Notes      :
*
*********************************************************************************************************
*/
int clk_init(void)
{
    int     i;

    CCU_DBG("aw clock manager init!\n");
    //initialise clock controller unit
    aw_ccu_init();
    //clear the data structure
    memset((void *)aw_clock, 0, sizeof(aw_clock));
    memset((void *)lookups, 0, sizeof(lookups));
    for(i=0; i<AW_CCU_CLK_CNT; i++) {
        /* initiate clk */
        if(aw_ccu_get_clk(i, &aw_clock[i]) != 0) {
            CCU_ERR("try toc get clock(id:%d) informaiton failed!\n", i);
        }
        /* register clk device */
        lookups[i].con_id = aw_clock[i].aw_clk->name;
        lookups[i].clk    = &aw_clock[i];
        clkdev_add(&lookups[i]);
    }
    /* initiate some clocks */
    lookups[AW_MOD_CLK_SMPTWD].dev_id = "smp_twd";

    return 0;
}
arch_initcall(clk_init);


int __clk_get(struct clk *hclk)
{
    /* just noitify, do nothing now, if you want record if the clock used count, you can add code here */
    return 1;
}


void __clk_put(struct clk *clk)
{
    /* just noitify, do nothing now, if you want record if the clock used count, you can add code here */
    return;
}


int clk_enable(struct clk *clk)
{
    if((clk == NULL) || IS_ERR(clk))
        return -EINVAL;

    if(!clk->enable) {
        clk->ops->set_status(clk->aw_clk->id, AW_CCU_CLK_ON);
    }
    clk->enable++;

    return 0;
}
EXPORT_SYMBOL(clk_enable);


void clk_disable(struct clk *clk)
{
    if(clk == NULL || IS_ERR(clk) || !clk->enable)
        return;

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);

    clk->enable--;
    if(clk->enable){
        return;
    }
    clk->ops->set_status(clk->aw_clk->id, AW_CCU_CLK_OFF);

    return;
}
EXPORT_SYMBOL(clk_disable);


unsigned long clk_get_rate(struct clk *clk)
{
    if((clk == NULL) || IS_ERR(clk)) {
        return 0;
    }

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);
    clk->aw_clk->rate = clk->ops->get_rate(clk->aw_clk->id);
    return (unsigned long)clk->aw_clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);


int clk_set_rate(struct clk *clk, unsigned long rate)
{
    if(clk == NULL || IS_ERR(clk))
        return -1;

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);
    if(clk->ops->set_rate(clk->aw_clk->id, rate) == 0) {
        clk->aw_clk->rate = clk->ops->get_rate(clk->aw_clk->id);
        return 0;
    }
    return -1;
}
EXPORT_SYMBOL(clk_set_rate);


struct clk *clk_get_parent(struct clk *clk)
{
    if((clk == NULL) || IS_ERR(clk)) {
        return NULL;
    }

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);
    return &aw_clock[clk->aw_clk->parent];
}
EXPORT_SYMBOL(clk_get_parent);


int clk_set_parent(struct clk *clk, struct clk *parent)
{
    if((clk == NULL) || IS_ERR(clk) || (parent == NULL) || IS_ERR(parent)) {
        return -1;
    }

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);
    if(clk->ops->set_parent(clk->aw_clk->id, parent->aw_clk->id) == 0) {
        clk->aw_clk->parent = clk->ops->get_parent(clk->aw_clk->id);
        clk->aw_clk->rate   = clk->ops->get_rate(clk->aw_clk->id);
        return 0;
    }
    return -1;
}
EXPORT_SYMBOL(clk_set_parent);


int clk_reset(struct clk *clk, __aw_ccu_clk_reset_e reset)
{
    if((clk == NULL) || IS_ERR(clk)) {
        return -EINVAL;
    }

    CCU_DBG("%s:%d:%s: %s !\n", __FILE__, __LINE__, __FUNCTION__, clk->aw_clk->name);
    clk->ops->set_reset(clk->aw_clk->id, reset);
    clk->aw_clk->reset = reset;

    return 0;
}
EXPORT_SYMBOL(clk_reset);

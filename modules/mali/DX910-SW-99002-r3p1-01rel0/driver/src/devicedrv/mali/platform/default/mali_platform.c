/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"

#include <linux/module.h>
#include <linux/clk.h>
#include <mach/irqs.h>
#include <mach/clock.h>
#include <mach/sys_config.h>


int mali_clk_div = 3;
module_param(mali_clk_div, int, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(mali_clk_div, "Clock divisor for mali");

struct clk *h_ahb_mali, *h_mali_clk, *h_ve_pll;
int mali_clk_flag=0;
_mali_osk_errcode_t mali_platform_init(void)
{
	unsigned long rate;

    script_item_u   mali_use, clk_drv;
    script_item_value_type_e  type;

	//get mali ahb clock
    h_ahb_mali = clk_get(NULL, CLK_AHB_MALI);
	if(!h_ahb_mali){
		MALI_PRINT(("try to get ahb mali clock failed!\n"));
	}
	//get mali clk
	h_mali_clk = clk_get(NULL, CLK_MOD_MALI);
	if(!h_mali_clk){
		MALI_PRINT(("try to get mali clock failed!\n"));
	}

	h_ve_pll = clk_get(NULL, CLK_SYS_PLL4);
	if(!h_ve_pll){
		MALI_PRINT(("try to get ve pll clock failed!\n"));
	}

	//set mali parent clock
	if(clk_set_parent(h_mali_clk, h_ve_pll)){
		MALI_PRINT(("try to set mali clock source failed!\n"));
	}

	//set mali clock
	rate = clk_get_rate(h_ve_pll);

    if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_used", &mali_use)){
        if(mali_use.val == 1){
            if(SCIRPT_ITEM_VALUE_TYPE_INT == script_get_item("mali_para", "mali_clkdiv", &clk_drv)){
                if(clk_drv.val > 0){
                    mali_clk_div = clk_drv.val;
                }
            }
        }
    }

	//pr_info("mali: clk_div %d\n", mali_clk_div);
	rate /= mali_clk_div;

	if(clk_set_rate(h_mali_clk, rate)){
		MALI_PRINT(("try to set mali clock failed!\n"));
	}

	if(clk_reset(h_mali_clk,AW_CCU_CLK_NRESET)){
		MALI_PRINT(("try to reset release failed!\n"));
	}

	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
    /*close mali axi/apb clock*/
    if(mali_clk_flag == 1)
    {
	//MALI_PRINT(("disable mali clock\n"));
	mali_clk_flag = 0;
       clk_disable(h_mali_clk);
       clk_disable(h_ahb_mali);
    }
    MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	if(power_mode == MALI_POWER_MODE_ON)
    {
	if(mali_clk_flag == 0)
	{
		//printk(KERN_WARNING "enable mali clock\n");
		//MALI_PRINT(("enable mali clock\n"));
		mali_clk_flag = 1;
	       if(clk_enable(h_ahb_mali))
	       {
		     MALI_PRINT(("try to enable mali ahb failed!\n"));
	       }
	       if(clk_enable(h_mali_clk))
	       {
		       MALI_PRINT(("try to enable mali clock failed!\n"));
	        }
	}
    }
    else if(power_mode == MALI_POWER_MODE_LIGHT_SLEEP)
    {
	/*close mali axi/apb clock*/
	if(mali_clk_flag == 1)
	{
		//MALI_PRINT(("disable mali clock\n"));
		mali_clk_flag = 0;
	       clk_disable(h_mali_clk);
	       clk_disable(h_ahb_mali);
	}
    }
    else if(power_mode == MALI_POWER_MODE_DEEP_SLEEP)
    {
	/*close mali axi/apb clock*/
	if(mali_clk_flag == 1)
	{
		//MALI_PRINT(("disable mali clock\n"));
		mali_clk_flag = 0;
	       clk_disable(h_mali_clk);
	       clk_disable(h_ahb_mali);
	}
    }
    MALI_SUCCESS;
}

void mali_gpu_utilization_handler(u32 utilization)
{
}

void set_mali_parent_power_domain(void* dev)
{
}

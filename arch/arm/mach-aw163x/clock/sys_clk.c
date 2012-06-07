/*
 *  arch/arm/mach-aw163x/clock/ccmu/ccm_sys_clk.c
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include <asm/delay.h>
#include "ccm_i.h"



/*
*********************************************************************************************************
*                           sys_clk_get_parent
*
*Description: get parent clock for system clock;
*
*Arguments  : id    system clock id;
*
*Return     : parent id;
*
*Notes      :
*
*********************************************************************************************************
*/
static __aw_ccu_clk_id_e sys_clk_get_parent(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_SYS_CLK_PLL2X8:
            return AW_SYS_CLK_PLL2;
        case AW_SYS_CLK_PLL6x2:
            return AW_SYS_CLK_PLL6;
        case AW_SYS_CLK_PLL3X2:
            return AW_SYS_CLK_PLL3;
        case AW_SYS_CLK_PLL7X2:
            return AW_SYS_CLK_PLL7;
        case AW_SYS_CLK_MIPIPLL:
            return aw_ccu_reg->MipiPllCtl.PllSrc? AW_SYS_CLK_PLL7:AW_SYS_CLK_PLL3;
        case AW_SYS_CLK_AC327:
            switch(aw_ccu_reg->SysClkDiv.CpuClkSrc)
            {
                case AC327_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case AC327_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                default:
                    return AW_SYS_CLK_PLL1;
            }
        case AW_SYS_CLK_AR100:
            switch(aw_cpu0_reg->Cpu0Cfg.ClkSrc)
            {
                case AR100_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case AR100_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                default:
                    return AW_SYS_CLK_PLL6;
            }
        case AW_SYS_CLK_AXI:
            return AW_SYS_CLK_AC327;
        case AW_SYS_CLK_AHB0:
            return AW_SYS_CLK_AR100;
        case AW_SYS_CLK_AHB1:
            switch(aw_ccu_reg->Ahb1Div.Ahb1ClkSrc)
            {
                case AHB1_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case AHB1_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                case AHB1_CLKSRC_AXI:
                    return AW_SYS_CLK_AXI;
                case AHB1_CLKSRC_PLL6:
                    return AW_SYS_CLK_PLL6;
            }
        case AW_SYS_CLK_APB0:
            return AW_SYS_CLK_AHB0;
        case AW_SYS_CLK_APB1:
            return AW_SYS_CLK_AHB1;
        case AW_SYS_CLK_APB2:
            switch(aw_ccu_reg->Apb2Div.ClkSrc)
            {
                case APB2_CLKSRC_LOSC:
                    return AW_SYS_CLK_LOSC;
                case APB2_CLKSRC_HOSC:
                    return AW_SYS_CLK_HOSC;
                default:
                    return AW_SYS_CLK_PLL6;
            }
        default:
            return AW_SYS_CLK_NONE;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_get_status
*
*Description: get system clock on/off status.
*
*Arguments  : id    system clock id;
*
*Return     : system clock status;
*               0, clock is off;
*              !0, clock is on;
*
*Notes      :
*
*********************************************************************************************************
*/
static __aw_ccu_clk_onff_e sys_clk_get_status(__aw_ccu_clk_id_e id)
{
    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return AW_CCU_CLK_ON;
        case AW_SYS_CLK_HOSC:
            return AW_CCU_CLK_ON;
        case AW_SYS_CLK_PLL1:
            return PLL1_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL2:
        case AW_SYS_CLK_PLL2X8:
            return PLL2_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL3X2:
            return PLL3_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL4:
            return PLL4_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL5:
            return PLL5_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL6:
        case AW_SYS_CLK_PLL6x2:
            return PLL6_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL7X2:
            return PLL7_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL8:
            return PLL8_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL9:
            return PLL9_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_PLL10:
            return PLL10_ENBLE? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_MIPIPLL:
            return aw_ccu_reg->MipiPllCtl.PLLEn? AW_CCU_CLK_ON : AW_CCU_CLK_OFF;
        case AW_SYS_CLK_AC327:
        case AW_SYS_CLK_AR100:
        case AW_SYS_CLK_AXI:
        case AW_SYS_CLK_AHB0:
        case AW_SYS_CLK_AHB1:
        case AW_SYS_CLK_APB0:
        case AW_SYS_CLK_APB1:
        case AW_SYS_CLK_APB2:
        default:
            return AW_CCU_CLK_ON;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_get_rate
*
*Description: get clock rate for system clock;
*
*Arguments  : id    system clock id;
*
*Return     : clock rate;
*
*Notes      :
*
*********************************************************************************************************
*/
static __u64 sys_clk_get_rate(__aw_ccu_clk_id_e id)
{
    __u64   tmp_rate;

    switch(id)
    {
        case AW_SYS_CLK_NONE:
            return 1;
        case AW_SYS_CLK_LOSC:
            return 32768;
        case AW_SYS_CLK_HOSC:
            return 24000000;
        case AW_SYS_CLK_PLL1:
            tmp_rate = (__u64)24000000*PLL1_FACTOR_N*PLL1_FACTOR_K;
            do_div(tmp_rate, PLL1_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL2:
            if((aw_ccu_reg->Pll2Ctl.FactorM == 20)
               && (aw_ccu_reg->Pll2Ctl.FactorN == 85)
               && (aw_ccu_reg->Pll2Ctl.FactorP == 3))
            {
                /* 24000000 * 86 / (21 * 4) = 24571000 */
                return 24576000;
            }
            else if((aw_ccu_reg->Pll2Ctl.FactorM == 20)
               && (aw_ccu_reg->Pll2Ctl.FactorN == 78)
               && (aw_ccu_reg->Pll2Ctl.FactorP == 3))
            {
                /* 24000000 * 79 / (21 * 4) = 22571000 */
                return 22579200;
            }
            else
            {
                aw_ccu_reg->Pll2Ctl.FactorM = 20;
                aw_ccu_reg->Pll2Ctl.FactorN = 78;
                aw_ccu_reg->Pll2Ctl.FactorP = 3;
                return 24576000;
            }
        case AW_SYS_CLK_PLL3:
        {
            if(!aw_ccu_reg->Pll3Ctl.ModeSel)
            {
                if(aw_ccu_reg->Pll3Ctl.FracMod)
                    return 297000000;
                else
                    return 270000000;
            }
            else
            {
                tmp_rate = (__u64)24000000*PLL3_FACTOR_N;
                do_div(tmp_rate, PLL3_FACTOR_M);
                return tmp_rate;
            }
        }
        case AW_SYS_CLK_PLL4:
            tmp_rate = (__u64)24000000*PLL4_FACTOR_N;
            do_div(tmp_rate, PLL4_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL5:
            tmp_rate = (__u64)24000000*PLL5_FACTOR_N*PLL5_FACTOR_K;
            do_div(tmp_rate, (PLL5_FACTOR_M+1));
            return tmp_rate;
        case AW_SYS_CLK_PLL6:
            tmp_rate = (__u64)24000000*PLL6_FACTOR_N*PLL6_FACTOR_K;
            return tmp_rate/2;
        case AW_SYS_CLK_PLL7:
            tmp_rate = (__u64)24000000*PLL7_FACTOR_N;
            do_div(tmp_rate, PLL7_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL8:
            tmp_rate = (__u64)24000000*PLL8_FACTOR_N;
            do_div(tmp_rate, PLL8_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL9:
            tmp_rate = (__u64)24000000*PLL9_FACTOR_N;
            do_div(tmp_rate, PLL9_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL10:
            tmp_rate = (__u64)24000000*PLL10_FACTOR_N;
            do_div(tmp_rate, PLL10_FACTOR_M);
            return tmp_rate;
        case AW_SYS_CLK_PLL2X8:
            return sys_clk_get_rate(AW_SYS_CLK_PLL2) * 8;
        case AW_SYS_CLK_PLL3X2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL3) * 2;
        case AW_SYS_CLK_PLL6x2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL6) * 2;
        case AW_SYS_CLK_PLL7X2:
            return sys_clk_get_rate(AW_SYS_CLK_PLL7) * 2;
        case AW_SYS_CLK_MIPIPLL:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            if(aw_ccu_reg->MipiPllCtl.VfbSel == 0) {
                tmp_rate *= aw_ccu_reg->MipiPllCtl.FactorN * aw_ccu_reg->MipiPllCtl.FactorK;
                do_div(tmp_rate, aw_ccu_reg->MipiPllCtl.FactorM);
            } else {
                tmp_rate *= aw_ccu_reg->MipiPllCtl.SDiv2+1;
                if(aw_ccu_reg->MipiPllCtl.FracMode == 0) {
                    tmp_rate *= aw_ccu_reg->MipiPllCtl.FeedBackDiv? 7:5;
                    do_div(tmp_rate, aw_ccu_reg->MipiPllCtl.FactorM);
                } else {
                    tmp_rate *= aw_ccu_reg->MipiPllCtl.Sel625Or750? 750:625;
                    do_div(tmp_rate, (aw_ccu_reg->MipiPllCtl.FactorM*100));
                }
            }
            return tmp_rate;

        case AW_SYS_CLK_AC327:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AC327_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AR100:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AR100_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AXI:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AXI_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AHB0:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AHB0_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_AHB1:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, AHB1_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_APB0:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, APB0_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_APB1:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, APB1_CLK_DIV);
            return tmp_rate;
        case AW_SYS_CLK_APB2:
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, APB2_CLK_DIV);
            return tmp_rate;
        default:
            CCU_DBG("system clock id is:%d\n", id);
            return 0;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_set_parent
*
*Description: set parent clock id for system clock;
*
*Arguments  : id        system clock id whose parent need be set;
*             parent    parent id to be set;
*
*Return     : result,
*               0,  set parent successed;
*              !0,  set parent failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 sys_clk_set_parent(__aw_ccu_clk_id_e id, __aw_ccu_clk_id_e parent)
{
    switch(id)
    {
        case AW_SYS_CLK_PLL2X8:
            return (parent == AW_SYS_CLK_PLL2)? 0:-1;
        case AW_SYS_CLK_PLL3X2:
            return (parent == AW_SYS_CLK_PLL3)? 0:-1;
        case AW_SYS_CLK_PLL6x2:
            return (parent == AW_SYS_CLK_PLL6)? 0:-1;
        case AW_SYS_CLK_PLL7X2:
            return (parent == AW_SYS_CLK_PLL7)? 0:-1;
        case AW_SYS_CLK_MIPIPLL:
            if(parent == AW_SYS_CLK_PLL3)
                aw_ccu_reg->MipiPllCtl.PllSrc = 0;
            else if(parent == AW_SYS_CLK_PLL7)
                aw_ccu_reg->MipiPllCtl.PllSrc = 1;
            else
                return -1;
            return 0;
        case AW_SYS_CLK_AC327:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_ccu_reg->SysClkDiv.CpuClkSrc = AC327_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_ccu_reg->SysClkDiv.CpuClkSrc = AC327_CLKSRC_HOSC;
                    return 0;
                case AW_SYS_CLK_PLL1:
                    aw_ccu_reg->SysClkDiv.CpuClkSrc = AC327_CLKSRC_PLL1;
                    return 0;
                default:
                    CCU_ERR("ac327 clock source is ivalid!\n");
                    return -1;
            }
        case AW_SYS_CLK_AR100:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_cpu0_reg->Cpu0Cfg.ClkSrc = AR100_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_cpu0_reg->Cpu0Cfg.ClkSrc = AR100_CLKSRC_HOSC;
                    return 0;
                case AW_SYS_CLK_PLL1:
                    aw_cpu0_reg->Cpu0Cfg.ClkSrc = AR100_CLKSRC_PLL6;
                    return 0;
                default:
                    CCU_ERR("ar100 clock source is ivalid!\n");
                    return -1;
            }
        case AW_SYS_CLK_AXI:
            return (parent == AW_SYS_CLK_AC327)? 0:-1;
        case AW_SYS_CLK_AHB0:
            return (parent == AW_SYS_CLK_AR100)? 0:-1;
        case AW_SYS_CLK_AHB1:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AHB1_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AW_SYS_CLK_HOSC;
                    return 0;
                case AW_SYS_CLK_AXI:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AHB1_CLKSRC_AXI;
                    return 0;
                case AW_SYS_CLK_PLL6:
                    aw_ccu_reg->Ahb1Div.Ahb1ClkSrc = AHB1_CLKSRC_PLL6;
                    return 0;
                default:
                    CCU_ERR("axi clock source is ivalid!\n");
                    return -1;
            }
        case AW_SYS_CLK_APB0:
            return (parent == AW_SYS_CLK_AHB0)? 0:-1;
        case AW_SYS_CLK_APB1:
            return (parent == AW_SYS_CLK_AHB1)? 0:-1;
        case AW_SYS_CLK_APB2:
            switch(parent)
            {
                case AW_SYS_CLK_LOSC:
                    aw_ccu_reg->Apb2Div.ClkSrc = APB2_CLKSRC_LOSC;
                    return 0;
                case AW_SYS_CLK_HOSC:
                    aw_ccu_reg->Apb2Div.ClkSrc = APB2_CLKSRC_HOSC;
                    return 0;
                case AW_SYS_CLK_PLL6:
                    aw_ccu_reg->Apb2Div.ClkSrc = APB2_CLKSRC_PLL6;
                    return 0;
                default:
                    CCU_ERR("apb2 clock source is ivalid!\n");
                    return -1;
            }

        case AW_SYS_CLK_LOSC:
        case AW_SYS_CLK_HOSC:
        case AW_SYS_CLK_PLL1:
        case AW_SYS_CLK_PLL2:
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL4:
        case AW_SYS_CLK_PLL5:
        case AW_SYS_CLK_PLL6:
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL8:
        case AW_SYS_CLK_PLL9:
        case AW_SYS_CLK_PLL10:
        default:
            return (parent == AW_SYS_CLK_NONE)? 0:-1;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_set_status
*
*Description: set on/off status for system clock;
*
*Arguments  : id        system clock id;
*             status    on/off status;
*                           AW_CCU_CLK_OFF - off
*                           AW_CCU_CLK_ON - on
*
*Return     : result;
*               0,  set status successed;
*              !0,  set status failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 sys_clk_set_status(__aw_ccu_clk_id_e id, __aw_ccu_clk_onff_e status)
{
    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return 0;
        case AW_SYS_CLK_HOSC:
            return 0;
        case AW_SYS_CLK_PLL1:
            PLL1_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL2:
            PLL2_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL3:
            PLL3_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL4:
            PLL4_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL5:
            PLL5_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL6:
            PLL6_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL7:
            PLL7_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL8:
            PLL8_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL9:
            PLL9_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_PLL10:
            PLL10_ENBLE = (status == AW_CCU_CLK_ON)? 1 : 0;
            return 0;
        case AW_SYS_CLK_MIPIPLL:
            aw_ccu_reg->MipiPllCtl.PLLEn = (status == AW_CCU_CLK_ON)? 1:0;
            return 0;
        case AW_SYS_CLK_PLL2X8:
            return (sys_clk_get_status(AW_SYS_CLK_PLL2) == status)? 0 : -1;
        case AW_SYS_CLK_PLL3X2:
            return (sys_clk_get_status(AW_SYS_CLK_PLL3) == status)? 0 : -1;
        case AW_SYS_CLK_PLL6x2:
            return (sys_clk_get_status(AW_SYS_CLK_PLL6) == status)? 0 : -1;
        case AW_SYS_CLK_PLL7X2:
            return (sys_clk_get_status(AW_SYS_CLK_PLL7) == status)? 0 : -1;

        default:
            return 0;
    }
}


/*
*********************************************************************************************************
*                           sys_clk_set_rate
*
*Description: set clock rate for system clock;
*
*Arguments  : id    system clock id;
*             rate  clock rate for system clock;
*
*Return     : result,
*               0,  set system clock rate successed;
*              !0,  set system clock rate failed;
*
*Notes      :
*
*********************************************************************************************************
*/
static __s32 sys_clk_set_rate(__aw_ccu_clk_id_e id, __u64 rate)
{
    switch(id)
    {
        case AW_SYS_CLK_LOSC:
            return (rate == 32768)? 0 : -1;
        case AW_SYS_CLK_HOSC:
            return (rate == 24000000)? 0 : -1;
        case AW_SYS_CLK_PLL1:
        {
            __ccmu_pll1_reg0000_t       tmp_pll;

            tmp_pll = aw_ccu_reg->Pll1Ctl;
            ccm_get_pll1_para(&tmp_pll, rate);
            aw_ccu_reg->Pll1Ctl = tmp_pll;
            while(aw_ccu_reg->Pll1Ctl.Lock);
            return 0;
        }
        case AW_SYS_CLK_PLL2:
        {
            if(rate == 22579200)
            {
                aw_ccu_reg->Pll2Ctl.FactorM = 20;
                aw_ccu_reg->Pll2Ctl.FactorN = 85;
                aw_ccu_reg->Pll2Ctl.FactorP = 3;
            }
            else if(rate == 24576000)
            {
                aw_ccu_reg->Pll2Ctl.FactorM = 20;
                aw_ccu_reg->Pll2Ctl.FactorN = 78;
                aw_ccu_reg->Pll2Ctl.FactorP = 3;
            }
            else
            {
                return -1;
            }
            return 0;
        }
        case AW_SYS_CLK_PLL3:
        case AW_SYS_CLK_PLL4:
        case AW_SYS_CLK_PLL7:
        case AW_SYS_CLK_PLL8:
        case AW_SYS_CLK_PLL9:
        case AW_SYS_CLK_PLL10:
        {
            volatile __ccmu_media_pll_t  *tmp_reg;

            if(id == AW_SYS_CLK_PLL3)
                tmp_reg = &aw_ccu_reg->Pll3Ctl;
            else if(id == AW_SYS_CLK_PLL4)
                tmp_reg = &aw_ccu_reg->Pll4Ctl;
            else if(id == AW_SYS_CLK_PLL7)
                tmp_reg = &aw_ccu_reg->Pll7Ctl;
            else if(id == AW_SYS_CLK_PLL8)
                tmp_reg = &aw_ccu_reg->Pll8Ctl;
            else if(id == AW_SYS_CLK_PLL9)
                tmp_reg = &aw_ccu_reg->Pll9Ctl;
            else
                tmp_reg = &aw_ccu_reg->Pll10Ctl;

            if(rate == 1000000000)
            {
                /* special frquency, control by de */
                tmp_reg->CtlMode = 1;
                return 0;
            }

            tmp_reg->CtlMode = 0;
            if((rate == 270000000) || (rate == 297000000))
            {
                tmp_reg->ModeSel = 0;
                tmp_reg->FracMod = (rate == 270000000)?0:1;
            }
            else
            {
                int     factor_n, factor_m;

                if(rate <= 128000000) {
                    factor_m = 23;
                    do_div(rate, 1000000);
                    factor_n = rate;
                }
                else if(rate <= 256000000) {
                    factor_m = 11;
                    do_div(rate, 2000000);
                    factor_n = rate;
                }
                else if(rate <= 384000000) {
                    factor_m = 7;
                    do_div(rate, 3000000);
                    factor_n = rate;
                }
                else if(rate <= 512000000) {
                    factor_m = 5;
                    do_div(rate, 4000000);
                    factor_n = rate;
                }
                else if(rate <= 768000000) {
                    factor_m = 3;
                    do_div(rate, 6000000);
                    factor_n = rate;
                }
                else {
                    CCU_ERR("Pll frequency is invalid!\n");
                    return -1;
                }
                if(tmp_reg->FactorM < factor_m) {
                    tmp_reg->FactorM = factor_m;
                    tmp_reg->FactorN = factor_n;
                } else {
                    tmp_reg->FactorN = factor_n;
                    tmp_reg->FactorM = factor_m;
                }
                while(tmp_reg->Lock);
                return 0;
            }
        }
        case AW_SYS_CLK_PLL5:
        {
            __ccmu_pll5_reg0020_t       tmp_pll;

            tmp_pll = aw_ccu_reg->Pll5Ctl;
            tmp_pll.FactorN = 16;
            tmp_pll.FactorK = 0;
            tmp_pll.FactorM = 0;
            aw_ccu_reg->Pll5Ctl = tmp_pll;
            while(aw_ccu_reg->Pll5Ctl.Lock);
            return 0;
        }
        case AW_SYS_CLK_PLL6:
        {
            do_div(rate, 12000000);
            if(rate > 32*4)
            {
                CCU_ERR("Rate(%lld) is invalid when set pll6 rate!\n", rate);
                return -1;
            }
            else if(rate > 32*3)
            {
                aw_ccu_reg->Pll6Ctl.FactorM = 3;
                do_div(rate, 4);
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;

            }
            else if(rate > 32*2)
            {
                aw_ccu_reg->Pll6Ctl.FactorM = 2;
                do_div(rate, 3);
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;
            }
            else if(rate > 32)
            {
                aw_ccu_reg->Pll6Ctl.FactorM = 1;
                do_div(rate, 2);
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;
            }
            else
            {
                aw_ccu_reg->Pll6Ctl.FactorM = 0;
                aw_ccu_reg->Pll6Ctl.FactorN = rate-1;
            }
            while(aw_ccu_reg->Pll6Ctl.Lock);
            return 0;
        }
        case AW_SYS_CLK_PLL2X8:
        case AW_SYS_CLK_PLL3X2:
        case AW_SYS_CLK_PLL6x2:
        case AW_SYS_CLK_PLL7X2:
            return 0;
        case AW_SYS_CLK_MIPIPLL:
            //#error 'how to set mipi pll?'
            return -1;
        case AW_SYS_CLK_AC327:
            return rate == sys_clk_get_rate(sys_clk_get_parent(id))? 0 : -1;
        case AW_SYS_CLK_AR100:
            return -1;
        case AW_SYS_CLK_AXI:
        {
            __u64   tmp_rate;
            tmp_rate = sys_clk_get_rate(sys_clk_get_parent(id));
            do_div(tmp_rate, rate);
            aw_ccu_reg->SysClkDiv.AXIClkDiv = tmp_rate;
            return 0;
        }
        case AW_SYS_CLK_AHB0:
            return -1;
        case AW_SYS_CLK_AHB1:
        case AW_SYS_CLK_APB0:
        case AW_SYS_CLK_APB1:
        case AW_SYS_CLK_APB2:

        default:
        {
            CCU_ERR("clock id(%d) is invaid when set rate!\n", (__s32)id);
            return -1;
        }
    }
}


__clk_ops_t sys_clk_ops = {
    .set_status = sys_clk_set_status,
    .get_status = sys_clk_get_status,
    .set_parent = sys_clk_set_parent,
    .get_parent = sys_clk_get_parent,
    .get_rate = sys_clk_get_rate,
    .set_rate = sys_clk_set_rate,
    .round_rate = 0,
    .set_reset  = 0,
    .get_reset  = 0,
};

/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : ccm_i.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-13 18:50
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __AW_CCMU_I_H__
#define __AW_CCMU_I_H__

#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <mach/clock.h>
#include <mach/ccmu.h>


extern __ccmu_reg_list_t   *aw_ccu_reg;
extern __ccmu_reg_cpu0_list_t  *aw_cpu0_reg;
extern __clk_ops_t sys_clk_ops;
extern __clk_ops_t mod_clk_ops;


#if (1)
    #define CCU_DBG(format,args...)   printk("[ccmu] "format,##args)
    #define CCU_ERR(format,args...)   printk("[ccmu] "format,##args)
#else
    #define CCU_DBG(...)
    #define CCU_ERR(...)
#endif


#define PLL1_ENBLE      (aw_ccu_reg->Pll1Ctl.PLLEn)
#define PLL2_ENBLE      (aw_ccu_reg->Pll2Ctl.PLLEn)
#define PLL3_ENBLE      (aw_ccu_reg->Pll3Ctl.PLLEn)
#define PLL4_ENBLE      (aw_ccu_reg->Pll4Ctl.PLLEn)
#define PLL5_ENBLE      (aw_ccu_reg->Pll5Ctl.PLLEn)
#define PLL6_ENBLE      (aw_ccu_reg->Pll6Ctl.PLLEn)
#define PLL7_ENBLE      (aw_ccu_reg->Pll7Ctl.PLLEn)
#define PLL8_ENBLE      (aw_ccu_reg->Pll8Ctl.PLLEn)
#define PLL9_ENBLE      (aw_ccu_reg->Pll9Ctl.PLLEn)
#define PLL10_ENBLE     (aw_ccu_reg->Pll10Ctl.PLLEn)

#define PLL1_FACTOR_N   (aw_ccu_reg->Pll1Ctl.FactorN)
#define PLL1_FACTOR_K   (aw_ccu_reg->Pll1Ctl.FactorK+1)
#define PLL1_FACTOR_M   (aw_ccu_reg->Pll1Ctl.FactorM+1)

#define PLL2_FACTOR_N   (aw_ccu_reg->Pll2Ctl.FactorN)
#define PLL2_FACTOR_K   (aw_ccu_reg->Pll2Ctl.FactorK+1)
#define PLL2_FACTOR_M   (aw_ccu_reg->Pll2Ctl.FactorM+1)

#define PLL3_FACTOR_N   (aw_ccu_reg->Pll3Ctl.FactorN)
#define PLL3_FACTOR_K   (aw_ccu_reg->Pll3Ctl.FactorK+1)
#define PLL3_FACTOR_M   (aw_ccu_reg->Pll3Ctl.FactorM+1)

#define PLL4_FACTOR_N   (aw_ccu_reg->Pll4Ctl.FactorN)
#define PLL4_FACTOR_K   (aw_ccu_reg->Pll4Ctl.FactorK+1)
#define PLL4_FACTOR_M   (aw_ccu_reg->Pll4Ctl.FactorM+1)

#define PLL5_FACTOR_N   (aw_ccu_reg->Pll5Ctl.FactorN)
#define PLL5_FACTOR_K   (aw_ccu_reg->Pll5Ctl.FactorK+1)
#define PLL5_FACTOR_M   (aw_ccu_reg->Pll5Ctl.FactorM+1)

#define PLL6_FACTOR_N   (aw_ccu_reg->Pll6Ctl.FactorN)
#define PLL6_FACTOR_K   (aw_ccu_reg->Pll6Ctl.FactorK+1)
#define PLL6_FACTOR_M   (aw_ccu_reg->Pll6Ctl.FactorM+1)

#define PLL7_FACTOR_N   (aw_ccu_reg->Pll7Ctl.FactorN)
#define PLL7_FACTOR_K   (aw_ccu_reg->Pll7Ctl.FactorK+1)
#define PLL7_FACTOR_M   (aw_ccu_reg->Pll7Ctl.FactorM+1)

#define PLL8_FACTOR_N   (aw_ccu_reg->Pll8Ctl.FactorN)
#define PLL8_FACTOR_K   (aw_ccu_reg->Pll8Ctl.FactorK+1)
#define PLL8_FACTOR_M   (aw_ccu_reg->Pll8Ctl.FactorM+1)

#define PLL9_FACTOR_N   (aw_ccu_reg->Pll9Ctl.FactorN)
#define PLL9_FACTOR_K   (aw_ccu_reg->Pll9Ctl.FactorK+1)
#define PLL9_FACTOR_M   (aw_ccu_reg->Pll9Ctl.FactorM+1)

#define PLL10_FACTOR_N  (aw_ccu_reg->Pll10Ctl.FactorN)
#define PLL10_FACTOR_K  (aw_ccu_reg->Pll10Ctl.FactorK+1)
#define PLL10_FACTOR_M  (aw_ccu_reg->Pll10Ctl.FactorM+1)

#define AC327_CLK_DIV   (1)
#define AR100_CLK_DIV   (1)
#define AXI_CLK_DIV     (aw_ccu_reg->SysClkDiv.AXIClkDiv+1)
#define AHB0_CLK_DIV    (1)
#define AHB1_CLK_DIV    ((aw_ccu_reg->Ahb1Div.Ahb1PreDiv + 1)*(1<<aw_ccu_reg->Ahb1Div.Ahb1Div))
#define APB0_CLK_DIV    (aw_cpu0_reg->Apb0Div.Div? (1<<aw_cpu0_reg->Apb0Div.Div) : 2)
#define APB1_CLK_DIV    (aw_ccu_reg->Ahb1Div.Apb1Div? (1<<aw_ccu_reg->Ahb1Div.Apb1Div) : 2)
#define APB2_CLK_DIV    ((aw_ccu_reg->Apb2Div.DivM+1)*(1<<aw_ccu_reg->Apb2Div.DivN))

int aw_ccu_init(void);
int aw_ccu_exit(void);
int aw_ccu_get_clk(__aw_ccu_clk_id_e id, __ccu_clk_t *clk);

int ccm_get_pll1_para(__ccmu_pll1_reg0000_t *factor, __u64 rate);

#endif /* #ifndef __AW_CCMU_I_H__ */

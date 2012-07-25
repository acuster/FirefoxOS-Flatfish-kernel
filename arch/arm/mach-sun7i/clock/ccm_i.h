/*
 * arch/arm/mach-sun7i/clock/ccm_i.h
 *
 * Copyright (c) 2012 Softwinner.
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
#ifndef __AW_CCMU_I_H__
#define __AW_CCMU_I_H__

#include <linux/kernel.h>
#include <asm/io.h>
#include <asm/div64.h>
#include <mach/clock.h>
#include <mach/ccmu.h>


extern __ccmu_reg_list_t   *aw_ccu_reg;
extern __clk_ops_t sys_clk_ops;
extern __clk_ops_t mod_clk_ops;


#if (1)
    #define CCU_DBG(format,args...)   printk("[ccmu] "format,##args)
    #define CCU_ERR(format,args...)   printk("[ccmu] "format,##args)
#else
    #define CCU_DBG(...)
    #define CCU_ERR(...)
#endif


int aw_ccu_init(void);
int aw_ccu_exit(void);
int aw_ccu_get_clk(__aw_ccu_clk_id_e id, __ccu_clk_t *clk);

struct core_pll_factor_t {
    __u8    FactorN;
    __u8    FactorK;
    __u8    FactorM;
    __u8    FactorP;
};

extern int ccm_clk_get_pll_para(struct core_pll_factor_t *factor, __u64 rate);

#endif /* #ifndef __AW_CCMU_I_H__ */

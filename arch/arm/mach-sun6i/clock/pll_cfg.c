/*
 *  arch/arm/mach-sun6i/clock/ccmu/pll_cfg_tbl.c
 *
 * Copyright 2012 (c) Allwinner.
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
#include "ccm_i.h"

struct pll1_freq_cfg_tbl{
    __u8    FactorN;
    __u8    FactorK;
    __u8    FactorM;
    __u8    reserved;
    __u32   Pll;
};

/* core pll parameter table */
static struct pll1_freq_cfg_tbl    Pll1Tbl[] = {

    { 17,    0,    0,    0,    408000000 },   /* freq = (6M * 68 ), index = 68  */
    { 25,    0,    0,    0,    600000000 },   /* freq = (6M * 100), index = 100 */
    { 20,    1,    0,    0,    960000000 },   /* freq = (6M * 160), index = 160 */
    { 25,    1,    0,    0,    1200000000},   /* freq = (6M * 200), index = 200 */
};


int ccm_get_pll1_para(__ccmu_pll1_reg0000_t *factor, __u64 rate)
{
    if(!factor)
    {
        return -1;
    }

    factor->FactorN = Pll1Tbl[1].FactorN;
    factor->FactorK = Pll1Tbl[1].FactorK;
    factor->FactorM = Pll1Tbl[1].FactorM;

    return 0;
}

//*****************************************************************************
//	Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)
//
//	File: 				system.c
//
//	Description:  The system file for AW1623 system
//
//	History:
//          2010/10/29      Berg Xing       0.10    Initial version for AW1623
//
//*****************************************************************************
#include "include.h"
#include "mctl_par.h"
#include "mctl_reg.h"

#ifdef CONFIG_USE_DDR

#define DRAM_PLL	(MCTL_CLK/1000000)


void gate_dram_clock(uint32 on)
{
	uint32 reg_val;
/*
	//enable dram controller clock
	reg_val = get_wvalue(CCM_REG_DRAMCLK_CTRL);
	if(on)
		reg_val |= 0x1<<15;
	else
		reg_val &= ~(0x1<<15);
	put_wvalue(CCM_REG_DRAMCLK_CTRL, reg_val);
*/
	reg_val = get_wvalue(SDR_CR);
	if(on)
		reg_val |= 0x1<<16;
	else
		reg_val &= ~(0x1<<16);
	put_wvalue(SDR_CR, reg_val);
}

void mctl_dram_pll_onoff(uint32 on)
{
	uint32 reg_val;

	reg_val = get_wvalue(CCM_PLL5_DDR_CTRL);
	if(on)
		reg_val |= 0x1U<<31;
	else
		reg_val &= ~(0x1U<<31);
	put_wvalue(CCM_PLL5_DDR_CTRL, reg_val);
}

void mctl_dram_clk_onoff(uint32 on)
{
	uint32 reg_val;

	reg_val = get_wvalue(CCM_PLL5_DDR_CTRL);
	if(on)
		reg_val |= 0x1U<<29;
	else
		reg_val &= ~(0x1U<<29);
	put_wvalue(CCM_PLL5_DDR_CTRL, reg_val);
}


void mctl_dram_pll_ldo(uint32 on)
{
	uint32 reg_val;

	reg_val = get_wvalue(CCM_PLL5_DDR_CTRL);
	if(on)
		reg_val |= 0x1U<<7;
	else
		reg_val &= ~(0x1U<<7);
	put_wvalue(CCM_PLL5_DDR_CTRL, reg_val);
}

#endif

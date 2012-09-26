//*****************************************************************************
//	Allwinner Technology, All Right Reserved. 2006-2010 Copyright (c)
//
//	File: 				mctl_hal.c
//
//	Description:  This file implements basic functions for DRAM controller
//
//	History:
//              2012/07/27      Berg Xing       0.10    Initial version
//*****************************************************************************
#include "include.h"
#include "mctl_par.h"
#include "mctl_reg.h"
#include "mctl_hal.h"

#ifdef CONFIG_USE_DDR

//*****************************************************************************
//	uint32 mctl_init()
//  Description:	DRAM Controller Initialize Procession
//
//	Arguments:		None
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************

//static __inline void wait(u32 n)
//{
//#ifdef SYSTEM_SIMULATION
//	n = 0x100;
//#endif
//	while (n--);
//
//}

void mctl_ddr3_reset(void)
{
	uint32 reg_val;

	reg_val = mctl_read_w(SDR_CR);
	reg_val &= ~(0x1<<12);
	mctl_write_w(SDR_CR, reg_val);
	wait(0x100);
	reg_val = mctl_read_w(SDR_CR);
	reg_val |= (0x1<<12);
	mctl_write_w(SDR_CR, reg_val);
}

void mctl_set_drive(void)
{
	uint32 reg_val;

	reg_val = mctl_read_w(SDR_CR);
	reg_val |= (0x6<<12);
	reg_val |= 0xF00;
	reg_val &= ~0x3;
	mctl_write_w(SDR_CR, reg_val);
}

void mctl_enable_dll0(void)
{
	put_wvalue(SDR_DLLCR0, (get_wvalue(SDR_DLLCR0) & ~0x40000000) | 0x80000000);

	wait(0x100);

	put_wvalue(SDR_DLLCR0, get_wvalue(SDR_DLLCR0) & ~0xC0000000);

	wait(0x1000);

	put_wvalue(SDR_DLLCR0, (get_wvalue(SDR_DLLCR0) & ~0x80000000) | 0x40000000);
}

void mctl_enable_dllx(void)
{
	uint32 i;
	uint32 dll_num;

	if(MCTL_BUS_WIDTH == 32)
		dll_num = 5;
	else
		dll_num = 3;

	for(i=1; i<dll_num; i++)
	{
		put_wvalue(SDR_DLLCR0+(i<<2), (get_wvalue(SDR_DLLCR0+(i<<2)) & ~0x40000000) | 0x80000000);
	}

	wait(0x100);

	for(i=1; i<dll_num; i++)
	{
		put_wvalue(SDR_DLLCR0+(i<<2), get_wvalue(SDR_DLLCR0+(i<<2)) & ~0xC0000000);
	}

	wait(0x1000);

	for(i=1; i<dll_num; i++)
	{
		put_wvalue(SDR_DLLCR0+(i<<2), (get_wvalue(SDR_DLLCR0+(i<<2)) & ~0x80000000) | 0x40000000);
	}
}


//*****************************************************************************
//	uint32 mctl_scan_readpipe(uint32 clk)
//  Description:	Search read pipe value for SDRAM
//
//	Arguments:		uint32 clk  DRAM x1 clock value
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************
#ifdef FPGA_PLATFORM
uint32 mctl_scan_readpipe(uint32 clk)
{
	uint32 reg_val;
	reg_val = 0x9;//0x8;
	if(MCTL_CS_NUM==2)
		reg_val |= 0x1<<9;
	mctl_write_w(SDR_APR, reg_val);						//clock 120Mhz (0xb)
	return (1);
}
#else


uint32 mctl_scan_readpipe(uint32 clk)
{
	uint32 ret_val;
	uint32 reg_val;

	//data training trigger
	reg_val = mctl_read_w(SDR_CCR);
	reg_val |= 0x1<<30;
	mctl_write_w(SDR_CCR, reg_val);

	//check whether data training process is end
	while(mctl_read_w(SDR_CCR) & (0x1<<30)) {};

	//check data training result
	reg_val = mctl_read_w(SDR_CSR);
	if(reg_val & (0x1<<20))
	{
		while(1){};
	}

	ret_val = 1;
	return (ret_val);
}
#endif
//*****************************************************************************
//	void mctl_mode_exit()
//  Description:	Exit from self refresh state or power down state
//
//	Arguments:		None
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************
void mctl_mode_exit(void)
{
	uint32 reg_val;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x17U<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	wait(0x100);
}
//*****************************************************************************
//	void mctl_self_refresh_entry()
//  Description:	Enter into self refresh state
//
//	Arguments:		None
//
//	Return Value:	None
//*****************************************************************************
void mctl_self_refresh_entry(void)
{
	uint32 reg_val;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x12U<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	wait(0x100);

	mctl_write_w(SDR_DPCR, 0x16510001);

}

void mctl_self_refresh_exit(void)
{
	mctl_mode_exit();
}

//*****************************************************************************
//	void mctl_power_down_entry()
//  Description:	Enter into power down state
//
//	Arguments:		None
//
//	Return Value:	None
//*****************************************************************************
void mctl_power_down_entry(void)
{
	uint32 reg_val;

	reg_val = mctl_read_w(SDR_DCR);
	reg_val &= ~(0x1fU<<27);
	reg_val |= 0x1eU<<27;
	mctl_write_w(SDR_DCR, reg_val);

	//check whether command has been executed
	while( mctl_read_w(SDR_DCR)& (0x1U<<31) );
	wait(0x100);
}

void mctl_power_down_exit(void)
{
	mctl_mode_exit();
}

//*****************************************************************************
//	void mctl_setup_sr_interval(uint32 clk)
//  Description:	Setup auto refresh interval value
//
//	Arguments:		uint32 clk DRAM x1 clock value
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************
void mctl_setup_ar_interval(uint32 clk)
{
	uint32 reg_val;
	uint32 tmp_val;

	if(MCTL_CHIP_SIZE<=1024)
		tmp_val = (131*clk)>>10;
	else
		tmp_val = (336*clk)>>10;
	reg_val = tmp_val;
	tmp_val = (7987*clk)>>10;
	tmp_val = tmp_val*9 - 200;
	reg_val |= tmp_val<<8;
	reg_val |= 0x8<<24;
	mctl_write_w(SDR_DRR, reg_val);
}

void mctl_host_port_cfg(uint32 port_no, uint32 cfg)
{
	uint32 addr = SDR_HPCR + (port_no<<2);

	mctl_write_w(addr, cfg);
}
//*****************************************************************************
//	void mctl_power_save_entry()
//  Description:	Enter into super power save state
//
//	Arguments:		None
//
//	Return Value:	None
//*****************************************************************************
void mctl_power_save_entry(void)
{
	uint32 reg_val;

	//put dram in self refresh state
	mctl_self_refresh_entry();

	//save memc ZQ value into RTC GP register
	reg_val = mctl_read_w(SDR_ZQSR)&0xfffff;
	reg_val |= 0x1<<20;				//super standby flag
	mctl_write_w(SDR_GP_REG0, reg_val);

	//memc pad hold on
	mctl_write_w(SDR_DPCR, 0x16510001);
	printk("set pad hold on. \n");
	busy_waiting();

}
void mctl_power_save_exit(void)
{
	mctl_init();
}

uint32 mctl_ahb_reset(void)
{
	uint32 reg_val;

	reg_val = mctl_read_w(0x01c20060);
	reg_val &=~(0x3<<14);
	mctl_write_w(0x01c20060,reg_val);
	wait(0x10);
	reg_val = mctl_read_w(0x01c20060);
	reg_val |=(0x3<<14);
	mctl_write_w(0x01c20060,reg_val);
	return 0;
}
//*****************************************************************************
//	uint32 mctl_init(void)
//  Description:	memc init process
//
//	Arguments:		None
//
//	Return Value:	1: Success		0: Fail
//*****************************************************************************
uint32 mctl_init(void)
{
	uint32 reg_val;
	uint32 ret_val;
	uint32 hold_flag;

	//memc reset
	mctl_ahb_reset();

	//get super standby flag
	hold_flag = mctl_read_w(SDR_DPCR)&0x1;

	//init dll
	mctl_enable_dll0();
	mctl_enable_dllx();
	wait(0x10);

	//memc clock on
	reg_val = mctl_read_w(SDR_CR);
	reg_val |= 0x1<<16;
	mctl_write_w(SDR_CR, reg_val);
	wait(0x10);

	//set DQS gate method
	reg_val = mctl_read_w(SDR_CCR);
	reg_val &= ~(0x1<<28);
	reg_val &= ~(0x1<<17);
	reg_val |= 0x1<<14;
	reg_val |= 0x1<<1;
	mctl_write_w(SDR_CCR, reg_val);

	//configure external DRAM
	reg_val = 0;
	if(MCTL_DDR_TYPE == 3)
		reg_val |= 0x1;
	reg_val |= (MCTL_IO_WIDTH>>3) <<1;
	if(MCTL_CHIP_SIZE == 256)
		reg_val |= 0x0<<3;
	else if(MCTL_CHIP_SIZE == 512)
		reg_val |= 0x1<<3;
	else if(MCTL_CHIP_SIZE == 1024)
		reg_val |= 0x2<<3;
	else if(MCTL_CHIP_SIZE == 2048)
		reg_val |= 0x3<<3;
	else if(MCTL_CHIP_SIZE == 4096)
		reg_val |= 0x4<<3;
	else if(MCTL_CHIP_SIZE == 8192)
		reg_val |= 0x5<<3;
	else
		reg_val |= 0x0<<3;
	reg_val |= ((MCTL_BUS_WIDTH>>3) - 1)<<6;
	reg_val |= (MCTL_CS_NUM -1)<<10;
	reg_val |= 0x1<<12;
	reg_val |= ((MCTL_ACCESS_MODE)&0x3)<<13;
	mctl_write_w(SDR_DCR, reg_val);

	//set refresh period
	mctl_setup_ar_interval(MCTL_CLK);

	//set timing parameters
	reg_val = 0x36d47793;
	mctl_write_w(SDR_TPR0, reg_val);
	reg_val = 0xa090;
	mctl_write_w(SDR_TPR1, reg_val);
	reg_val = 0x1aa00;
	mctl_write_w(SDR_TPR2, reg_val);

	//set mode register
	if(MCTL_DDR_TYPE==3)				//ddr3
	{
		reg_val = 0x0;
		reg_val |= (MCTL_CAS - 4)<<4;
		reg_val |= 0x5<<9;
	}
	else if(MCTL_DDR_TYPE==2)			//ddr2
	{
		reg_val = 0x2;
		reg_val |= MCTL_CAS<<4;
		reg_val |= 0x5<<9;
	}
	mctl_write_w(SDR_MR, reg_val);
	reg_val = 0x0;
	mctl_write_w(SDR_EMR, reg_val);
	reg_val = 0x8;
	mctl_write_w(SDR_EMR2, reg_val);
	reg_val = 0x0;
	mctl_write_w(SDR_EMR3, reg_val);

	//start memc ZQ process
	if(hold_flag)
	{
		reg_val = mctl_read_w(SDR_GP_REG0)&0xfffff;
		reg_val |= 0x7b<<20;
		reg_val |= 0x1<<28;
		mctl_write_w(SDR_ZQCR0, reg_val);
	}
	else
	{
		reg_val = 0x87b00000;
		mctl_write_w(SDR_ZQCR0, reg_val);
		while( !(mctl_read_w(SDR_ZQSR) & (0x1U<<31))) {};
	}

	//release reset
	mctl_ddr3_reset();

	//setup pad mode
	reg_val = (0x7<<24)|(0x7<<20)|(0x1<<16)|(0x1<<12)|(0x7<<8)|(0xf<<2);
	mctl_write_w(SDR_CR, reg_val);

	//start init external dram
	reg_val = mctl_read_w(SDR_CCR);
	reg_val |= 0x1U<<31;
	mctl_write_w(SDR_CCR, reg_val);
	while(mctl_read_w(SDR_CCR) & (0x1U<<31)) {};

	//scan read pipe value
	if(hold_flag)
	{
		mctl_self_refresh_entry();
		mctl_write_w(SDR_DPCR, 0x16510000);
		mctl_self_refresh_exit();
	}
	ret_val = mctl_scan_readpipe(MCTL_CLK);

	return (ret_val);
}
#endif

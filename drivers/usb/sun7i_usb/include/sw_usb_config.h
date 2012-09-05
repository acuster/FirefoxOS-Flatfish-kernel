/*
*************************************************************************************
*                         			      Linux
*					           USB Device Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_usb_config.h
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
*
*************************************************************************************
*/
#ifndef  __SW_USB_CONFIG_H__
#define  __SW_USB_CONFIG_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/errno.h>

#include  "sw_usb_typedef.h"
#include  "sw_usb_debug.h"
#include  "sun7i_usb_bsp.h"
#include  "sun7i_sys_reg.h"

#include <mach/sys_config.h>

#include  "sw_usb_board.h"
#include  "sw_udc.h"
#include  "sw_hcd.h"

#define   SW_USB_FPGA

#ifdef  SW_USB_FPGA

#define  SW_VA_USB0_IO_BASE     0xf1c13000
#define  SW_VA_USB1_IO_BASE     0xf1c14000
#define  SW_VA_USB2_IO_BASE     0xf1c1c000

#define  SW_VA_SRAM_IO_BASE     0xf1c00000
#define  SW_INTC_IRQNO_USB0     (12 + 32)

#endif


#endif   //__SW_USB_CONFIG_H__

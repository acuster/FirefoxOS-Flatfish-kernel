/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_int.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 20:13
* Descript: interrupt for platform standby
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"

static void *GicDDisc;
static void *GicCDisc;




/*
*********************************************************************************************************
*                                       STANDBY INTERRUPT INITIALISE
*
* Description: standby interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 standby_int_init(void)
{
	__u32 i = 0;
	GicDDisc = (void *)IO_ADDRESS(AW_GIC_DIST_BASE);
	GicCDisc = (void *)IO_ADDRESS(AW_GIC_CPU_BASE);

	//printk("gic iar == 0x%x. \n", *(volatile __u32	 *)(IO_ADDRESS(AW_GIC_CPU_BASE)+0x0c));

	/* initialise interrupt enable and mask for standby */

	/*
	 * Disable all interrupts.  Leave the PPI and SGIs alone
	 * as these enables are banked registers.
	 */
	for (i = 4; i < (0x40); i += 4)
		*(volatile __u32 *)(GicDDisc + GIC_DIST_ENABLE_CLEAR + i) = 0xffffffff;

	/*config cpu interface*/
#if 0
	*(volatile __u32 *)(GicCDisc + GIC_CPU_PRIMASK) = 0xf0;
	*(volatile __u32 *)(GicCDisc + GIC_CPU_CTRL) = 0x1;
#endif

#if 1
	/* clear external irq pending: needed */
	for (i = 4; i < (0x40); i += 4)
		*(volatile __u32 *)(GicDDisc + GIC_DIST_PENDING_CLEAR + i) = 0xffffffff;
#endif
	//the print info just to check the pending state, actually, after u read iar, u need to access end of interrupt reg;
	i = *(volatile __u32   *)(GicCDisc + 0x0c);

	if(i != 0x3ff){
		//u need to
		*(volatile __u32 *)(GicCDisc + 0x10) = i;
		printk("notice: gic iar == 0x%x. \n", i);
	}


	return 0;
}


/*
*********************************************************************************************************
*                                       STANDBY INTERRUPT INITIALISE
*
* Description: standby interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 standby_int_exit(void)
{
	int i = 0;
	volatile __u32 enable_bit = 0;

	//all the disable-int-src pending, need to be clear
	for(i = 0; i < 0x40; i += 4){
		enable_bit = *(volatile __u32 *)(GicDDisc + GIC_DIST_ENABLE_SET + i);
		*(volatile __u32 *)(GicDDisc + GIC_DIST_PENDING_CLEAR + i) &= (~enable_bit);
	}

	return 0;
}


/*
*********************************************************************************************************
*                                       QUERY INTERRUPT
*
* Description: query interrupt.
*
* Arguments  : src  interrupt source number.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 standby_enable_int(enum interrupt_source_e src)
{
    __u32   tmpGrp = (__u32)src >> 5;
    __u32   tmpSrc = (__u32)src & 0x1f;

    //enable interrupt source
    *(volatile __u32 *)(GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4) |= (1<<tmpSrc);
    //printk("GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4 = 0x%x. tmpGrp = 0x%x.\n", GicDDisc + GIC_DIST_ENABLE_SET + tmpGrp*4, tmpGrp);
    //printk("tmpSrc = 0x%x. \n", tmpSrc);

    //need to care mask or priority?

    return 0;
}


/*
*********************************************************************************************************
*                                       QUERY INTERRUPT
*
* Description: query interrupt.
*
* Arguments  : src  interrupt source number.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 standby_query_int(enum interrupt_source_e src)
{
    __s32   result = 0;
    __u32   tmpGrp = (__u32)src >> 5;
    __u32   tmpSrc = (__u32)src & 0x1f;

    result = *(volatile __u32 *)(GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4) & (1<<tmpSrc);

    //printk("GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4 = 0x%x. tmpGrp = 0x%x.\n", GicDDisc + GIC_DIST_PENDING_SET + tmpGrp*4, tmpGrp);
    //printk("tmpSrc = 0x%x. result = 0x%x. \n", tmpSrc, result);

    return result? 0:-1;
}

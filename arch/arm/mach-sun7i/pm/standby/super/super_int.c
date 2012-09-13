/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : super_int.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 20:13
* Descript: interrupt for platform mem
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "super_i.h"


/*
*********************************************************************************************************
*                                       STANDBY INTERRUPT INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_int_init(void)
{

    return 0;
}


/*
*********************************************************************************************************
*                                       STANDBY INTERRUPT INITIALISE
*
* Description: mem interrupt initialise.
*
* Arguments  : none.
*
* Returns    : 0/-1;
*********************************************************************************************************
*/
__s32 mem_int_exit(void)
{

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
__s32 mem_enable_int(enum interrupt_source_e src)
{
    __u32   tmpGrp = (__u32)src >> 5;
    __u32   tmpSrc = (__u32)src & 0x1f;

    //enable interrupt source

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
__s32 mem_query_int(enum interrupt_source_e src)
{
    __s32   result = 0;
    __u32   tmpGrp = (__u32)src >> 5;
    __u32   tmpSrc = (__u32)src & 0x1f;

    return result? 0:-1;
}

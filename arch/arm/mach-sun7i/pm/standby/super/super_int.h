/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : super_int.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 19:50
* Descript: intterupt bsp for platform mem.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __SUPER_INT_H__
#define __SUPER_INT_H__

#include "super_cfg.h"

extern __s32 mem_int_init(void);
extern __s32 mem_int_exit(void);
extern __s32 mem_enable_int(enum interrupt_source_e src);
extern __s32 mem_query_int(enum interrupt_source_e src);


#endif  //__SUPER_INT_H__

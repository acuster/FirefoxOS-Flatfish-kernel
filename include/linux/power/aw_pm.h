/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __AW_PM_H__
#define __AW_PM_H__


/**max device number of pmu*/
#define PMU_MAX_DEVS        2

/**
*@name PMU command
*@{
*/
#define AW_PMU_SET          0x10
#define AW_PMU_VALID        0x20
/**
*@}
*/

/*
* the wakeup source of main cpu: cpu0
*/
#define CPU0_WAKEUP_MSGBOX		(1<<0)  /* external interrupt, pmu event for ex.    */
#define CPU0_WAKEUP_KEY			(1<<1)  /* key event    */

//the wakeup source of assistant cpu: cpus
#define CPUS_WAKEUP_LOWBATT    	(1<<2 )
#define CPUS_WAKEUP_USB        	(1<<3 )
#define CPUS_WAKEUP_AC         	(1<<4 )
#define CPUS_WAKEUP_ASCEND     	(1<<5 )
#define CPUS_WAKEUP_DESCEND    	(1<<6 )
#define CPUS_WAKEUP_SHORT_KEY  	(1<<7 )
#define CPUS_WAKEUP_LONG_KEY   	(1<<8 )
#define CPUS_WAKEUP_IR     		(1<<9 )
#define CPUS_WAKEUP_ALM0  		(1<<10)
#define CPUS_WAKEUP_ALM1  		(1<<11)
#define CPUS_WAKEUP_TIMEOUT		(1<<12)
#define CPUS_WAKEUP_GPIO		(1<<13)
#define CPUS_WAKEUP_KEY        	(CPUS_WAKEUP_SHORT_KEY | CPUS_WAKEUP_LONG_KEY)

#define WAKEUP_GPIO_PL(num)     (1 << (num))
#define WAKEUP_GPIO_PM(num)     (1 << (num + 12))
#define WAKEUP_GPIO_AXP(num)    (1 << (num + 24))

typedef	struct super_standby_para
{
	unsigned long event;			//cpus wakeup event types
	unsigned long resume_code_src; 		//cpux resume code src
	unsigned long resume_code_length; 	//cpux resume code length
	unsigned long resume_entry; 		//cpux resume entry
	unsigned long timeout;			//wakeup after timeout seconds
	unsigned long gpio_enable_bitmap;
} super_standby_para_t;

typedef	struct normal_standby_para
{
	unsigned long event;		//cpus wakeup event types
	unsigned long timeout;		//wakeup after timeout seconds
	unsigned long gpio_enable_bitmap;
} normal_standby_para_t;


//define cpus wakeup src
#define CPUS_MEM_WAKEUP              (CPUS_WAKEUP_LOWBATT | CPUS_WAKEUP_USB | CPUS_WAKEUP_AC | \
						CPUS_WAKEUP_DESCEND | CPUS_WAKEUP_ASCEND | CPUS_WAKEUP_ALM0 | CPUS_WAKEUP_GPIO)
#define CPUS_BOOTFAST_WAKEUP         (CPUS_WAKEUP_LOWBATT | CPUS_WAKEUP_LONG_KEY |CPUS_WAKEUP_ALM0)

/*used in normal standby*/
#define CPU0_MEM_WAKEUP              (CPU0_WAKEUP_MSGBOX)
#define CPU0_BOOTFAST_WAKEUP         (CPU0_WAKEUP_MSGBOX)


/**
*@brief struct of pmu device arg
*/
struct aw_pmu_arg{
    unsigned int  twi_port;		/**<twi port for pmu chip   */
    unsigned char dev_addr;		/**<address of pmu device   */
};


/**
*@brief struct of standby
*/
struct aw_standby_para{
	unsigned int event;		/**<event type for system wakeup    */
	unsigned int axp_event;		/**<axp event type for system wakeup    */
	unsigned int debug_mask;	/* debug mask */
	signed int   timeout;		/**<time to power off system from now, based on second */
	unsigned long gpio_enable_bitmap;
};


/**
*@brief struct of power management info
*/
struct aw_pm_info{
    struct aw_standby_para		standby_para;   /* standby parameter            */
    struct aw_pmu_arg			pmu_arg;        /**<args used by main function  */
};


#endif /* __AW_PM_H__ */


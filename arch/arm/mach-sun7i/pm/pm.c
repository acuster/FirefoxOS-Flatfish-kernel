/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager for newbies chips platform.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <linux/power/aw_pm.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>
#include "pm_i.h"

#include <mach/sys_config.h>
#include <mach/system.h>

//#define CROSS_MAPPING_STANDBY
//#define CHECK_INT_SRC

#define AW_PM_DBG   1
#undef PM_DBG
#if(AW_PM_DBG)
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef ENTER_SUPER_STANDBY
#undef PRE_DISABLE_MMU
#endif

#ifdef ENTER_SUPER_STANDBY_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#undef PRE_DISABLE_MMU
#endif

#ifdef WATCH_DOG_RESET
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

//#define VERIFY_RESTORE_STATUS

/* define major number for power manager */
#define AW_PMU_MAJOR    267

static int debug_mask = PM_STANDBY_PRINT_STANDBY | PM_STANDBY_PRINT_RESUME;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static int standby_axp_enable = 1;
module_param_named(standby_axp_enable, standby_axp_enable, int, S_IRUGO | S_IWUSR | S_IWGRP);

extern char *standby_bin_start;
extern char *standby_bin_end;
extern char *suspend_bin_start;
extern char *suspend_bin_end;
extern char *resume0_bin_start;
extern char *resume0_bin_end;

extern int mem_arch_suspend(void);
extern int mem_arch_resume(void);
extern asmlinkage int mem_clear_runtime_context(void);
extern asmlinkage int mem_restore_runtime_context(void);
extern asmlinkage int mem_save_runtime_context(void);

extern void disable_cache(void);
extern void disable_program_flow_prediction(void);
extern void invalidate_branch_predictor(void);
extern void enable_cache(void);
extern void enable_program_flow_prediction(void);
extern void disable_dcache(void);
extern void disable_l2cache(void);
extern void set_ttbr0(void);
typedef  int (*suspend_func)(void);
void jump_to_suspend(__u32 ttbr1, suspend_func p);
extern void clear_reg_context(void);
extern void invalidate_dcache(void);

int (*mem)(void) = 0;

#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
extern void cpufreq_user_event_notify(void);
#endif

static struct map_desc mem_sram_md = {
	.virtual = MEM_SW_VA_SRAM_BASE,
	.pfn = __phys_to_pfn(MEM_SW_PA_SRAM_BASE),
	.length = SZ_1M,
	.type = MT_MEMORY_ITCM,
	};

static struct aw_pm_info standby_info = {
    .standby_para = {
		.event_enable  = SUSPEND_WAKEUP_SRC_EXINT,
		.axp_src = AXP_MEM_WAKEUP,
    },
    .pmu_arg = {
        .twi_port = 0,
        .dev_addr = 10,
    },
};

static struct tmr_state saved_tmr_state;
static struct twi_state saved_twi_state;
static struct gpio_state saved_gpio_state;
static struct sram_state saved_sram_state;

#ifdef GET_CYCLE_CNT
static int start = 0;
static int resume0_period = 0;
static int resume1_period = 0;

static int pm_start = 0;
static int invalidate_data_time = 0;
static int invalidate_instruct_time = 0;
static int before_restore_processor = 0;
static int after_restore_process = 0;
//static int restore_runtime_peroid = 0;

//late_resume timing
static int late_resume_start = 0;
static int backup_area_start = 0;
static int backup_area1_start = 0;
static int backup_area2_start = 0;
static int clk_restore_start = 0;
static int gpio_restore_start = 0;
static int twi_restore_start = 0;
static int int_restore_start = 0;
static int tmr_restore_start = 0;
static int sram_restore_start = 0;
static int late_resume_end = 0;
#endif

struct aw_mem_para mem_para_info;
standby_type_e standby_type = NON_STANDBY;
EXPORT_SYMBOL(standby_type);
standby_level_e standby_level = STANDBY_INITIAL;
EXPORT_SYMBOL(standby_level);

//static volatile int enter_flag = 0;
static bool mem_allocated_flag = false;
static int standby_mode = 1;
static int suspend_status_flag = 0;
module_param_named(standby_mode, standby_mode, int, S_IRUGO | S_IWUSR | S_IWGRP);

extern void create_mapping(struct map_desc *md);
extern void save_mapping(unsigned long vaddr);
extern void restore_mapping(unsigned long vaddr);
extern void save_mmu_state(struct mmu_state *saved_mmu_state);
extern void clear_mem_flag(void);
extern void save_runtime_context(__u32 *addr);
void restore_processor_ttbr0(void);
extern void flush_icache(void);
extern void flush_dcache(void);

#ifdef CHECK_INT_SRC
static void check_int_src(void)
{
#define INT_REG_0 (0x10)
#define INT_REG_1 (0x14)
#define INT_REG_2 (0x18)


	u32 data_0 = 0;
	u32 data_1 = 0;
	u32 data_2 = 0;

	data_0 = *(volatile unsigned int *)(SW_VA_INT_IO_BASE + INT_REG_0);
	data_1 = *(volatile unsigned int *)(SW_VA_INT_IO_BASE + INT_REG_1);
	data_2 = *(volatile unsigned int *)(SW_VA_INT_IO_BASE + INT_REG_2);

	pr_info("INT_REG_0 = %d \n", data_0);
	pr_info("INT_REG_1 = %d \n", data_1);
	pr_info("INT_REG_2 = %d \n", data_2);

	return;

}
#endif


/*
*********************************************************************************************************
*                           aw_pm_valid
*
*Description: determine if given system sleep state is supported by the platform;
*
*Arguments  : state     suspend state;
*
*Return     : if the state is valid, return 1, else return 0;
*
*Notes      : this is a call-back function, registered into PM core;
*
*********************************************************************************************************
*/
static int aw_pm_valid(suspend_state_t state)
{
#ifdef CHECK_IC_VERSION
	enum sw_ic_ver version = MAGIC_VER_NULL;
#endif

    PM_DBG("valid\n");

    if(!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))){
        PM_DBG("state (%d) invalid!\n", state);
        return 0;
    }

#ifdef CHECK_IC_VERSION
	if(1 == standby_mode){
			version = sw_get_ic_ver();
			if(!(MAGIC_VER_A13B == version || MAGIC_VER_A12B == version || MAGIC_VER_A10SB == version)){
				pr_info("ic version: %d not support super standby. \n", version);
				standby_mode = 0;
			}
	}
#endif

	//if 1 == standby_mode, actually, mean mem corresponding with super standby
	if(PM_SUSPEND_STANDBY == state){
		if(1 == standby_mode){
			standby_type = NORMAL_STANDBY;
		}else{
			standby_type = SUPER_STANDBY;
		}
	}else if(PM_SUSPEND_MEM == state || PM_SUSPEND_BOOTFAST == state){
		if(1 == standby_mode){
			standby_type = SUPER_STANDBY;
		}else{
			standby_type = NORMAL_STANDBY;
		}
	}

	//allocat space for backup dram data
	if((false == mem_allocated_flag) && (SUPER_STANDBY == standby_type)){
		if((DRAM_BACKUP_SIZE) < ((int)&resume0_bin_end - (int)&resume0_bin_start) ){
			//judge the reserved space for resume0 is enough or not.
			pr_info("Notice: reserved space for resume is not enough. \n");
			return 0;
		}

		memcpy((void *)DRAM_BACKUP_BASE_ADDR, (void *)&resume0_bin_start, (int)&resume0_bin_end - (int)&resume0_bin_start);
		dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR, (void *)(DRAM_BACKUP_BASE_ADDR + DRAM_BACKUP_SIZE -1) );

		mem_allocated_flag = true;
#ifdef GET_CYCLE_CNT
	// init counters:
	init_perfcounters (1, 0);
#endif
	}

	//print_flag = 0;
	return 1;

}


/*
*********************************************************************************************************
*                           aw_pm_begin
*
*Description: Initialise a transition to given system sleep state;
*
*Arguments  : state     suspend state;
*
*Return     : return 0 for process successed;
*
*Notes      : this is a call-back function, registered into PM core, and this function
*             will be called before devices suspened;
*********************************************************************************************************
*/
int aw_pm_begin(suspend_state_t state)
{
    PM_DBG("%d state begin\n", state);

	//set freq max
#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
	cpufreq_user_event_notify();
#endif

/*must init perfcounter, because delay_us and delay_ms is depandant perf counter*/
#ifndef GET_CYCLE_CNT
		backup_perfcounter();
		init_perfcounters (1, 0);
#endif

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare
*
*Description: Prepare the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core, this function
*             will be called after devices suspended, and before device late suspend
*             call-back functions;
*********************************************************************************************************
*/
int aw_pm_prepare(void)
{
    PM_DBG("prepare\n");

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare_late
*
*Description: Finish preparing the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core.
*             prepare_late is called before disabling nonboot CPUs and after
*              device drivers' late suspend callbacks have been executed;
*********************************************************************************************************
*/
int aw_pm_prepare_late(void)
{
    PM_DBG("prepare_late\n");

    return 0;
}

/*
*********************************************************************************************************
*                           aw_early_suspend
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      : -1: data is ok;
*			-2: data has been destory.
*********************************************************************************************************
*/
static int aw_early_suspend(void)
{
#define MAX_RETRY_TIMES (5)

	__s32 retry = MAX_RETRY_TIMES;

	//backup device state
	mem_ccu_save((__ccmu_reg_list_t *)(SW_VA_CCM_IO_BASE));
	mem_gpio_save(&(saved_gpio_state));
	mem_tmr_save(&(saved_tmr_state));
	mem_twi_save(&(saved_twi_state));
	mem_sram_save(&(saved_sram_state));

#if 1
    if (likely(mem_para_info.axp_enable))
    {
        //backup volt and freq state, after backup device state
        mem_twi_init(AXP_IICBUS);
        /* backup voltages */
        while(-1 == (mem_para_info.suspend_dcdc2 = mem_get_voltage(POWER_VOL_DCDC2)) && --retry){
            ;
        }
        if(0 == retry){
            print_call_info();
            return -1;
        }else{
            retry = MAX_RETRY_TIMES;
        }

        while(-1 == (mem_para_info.suspend_dcdc3 = mem_get_voltage(POWER_VOL_DCDC3)) && --retry){
            ;
        }
        if(0 == retry){
            print_call_info();
            return -1;
        }else{
            retry = MAX_RETRY_TIMES;
        }
    }
    else
    {
        mem_para_info.suspend_dcdc2 = -1;
        mem_para_info.suspend_dcdc3 = -1;
    }
#endif

	/*backup bus ratio*/
	mem_clk_getdiv(&mem_para_info.clk_div);
	/*backup pll ratio*/
	mem_clk_get_pll_factor(&mem_para_info.pll_factor);

	//backup mmu
	save_mmu_state(&(mem_para_info.saved_mmu_state));
	//backup cpu state
	__save_processor_state(&(mem_para_info.saved_cpu_context));
	//backup 0x0000,0000 page entry, size?
	save_mapping(MEM_SW_VA_SRAM_BASE);

	//prepare resume0 code for resume

	if((DRAM_BACKUP_SIZE1) < sizeof(mem_para_info)){
		//judge the reserved space for mem para is enough or not.
        print_call_info();
		return -1;
	}

	//clean all the data into dram
	memcpy((void *)DRAM_BACKUP_BASE_ADDR1, (void *)&mem_para_info, sizeof(mem_para_info));
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR1, (void *)(DRAM_BACKUP_BASE_ADDR1 + DRAM_BACKUP_SIZE1 - 1));

	//prepare dram training area data
	memcpy((void *)DRAM_BACKUP_BASE_ADDR2, (void *)DRAM_BASE_ADDR, DRAM_TRANING_SIZE);
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR2, (void *)(DRAM_BACKUP_BASE_ADDR2 + DRAM_BACKUP_SIZE2 - 1));

	mem_arch_suspend();
	save_processor_state();

	//before creating mapping, build the coherent between cache and memory
	//clean and flush
	__cpuc_flush_kern_all();
	__cpuc_coherent_kern_range(0xc0000000, 0xffffffff-1);

	//create 0x0000,0000 mapping table: 0x0000,0000 -> 0x0000,0000
	create_mapping(&mem_sram_md);
	mem_flush_tlb();

#ifdef PRE_DISABLE_MMU
	//jump to sram: dram enter selfresh, and power off.
	mem = (int (*)(void))SRAM_FUNC_START_PA;
#else
	//jump to sram: dram enter selfresh, and power off.
	mem = (int (*)(void))SRAM_FUNC_START;
#endif
    //move standby code to sram
    memcpy((void *)SRAM_FUNC_START, (void *)&suspend_bin_start, (int)&suspend_bin_end - (int)&suspend_bin_start);


#ifdef PRE_DISABLE_MMU
	//enable the mapping and jump
	//invalidate tlb? maybe, but now, at this situation,  0x0000 <--> 0x0000 mapping never stay in tlb before this.
	//busy_waiting();
	jump_to_suspend(mem_para_info.saved_cpu_context.ttb_1r, mem);
#else
	mem();
#endif

	return -2;

}

/*
*********************************************************************************************************
*                           verify_restore
*
*Description: verify src and dest region is the same;
*
*Return     : 0: same;
*                -1: different;
*
*Notes      :
*********************************************************************************************************
*/
#ifdef VERIFY_RESTORE_STATUS
static int verify_restore(void *src, void *dest, int count)
{
	volatile char *s = (volatile char *)src;
	volatile char *d = (volatile char *)dest;

	while(count--){
		if(*(s+(count)) != *(d+(count))){
			//busy_waiting();
			return -1;
		}
	}

	return 0;
}
#endif

/*
*********************************************************************************************************
*                           aw_late_resume
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      :
*********************************************************************************************************
*/
static void aw_late_resume(void)
{
	/*may be the para have not been changed by others, but, it is a good habit to get the latest data.*/
	memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1), sizeof(mem_para_info));
	mem_para_info.mem_flag = 0;

	//restore device state
	mem_ccu_restore((__ccmu_reg_list_t *)(SW_VA_CCM_IO_BASE));
	mem_gpio_restore(&(saved_gpio_state));
	mem_twi_restore(&(saved_twi_state));
	mem_tmr_restore(&(saved_tmr_state));
	mem_sram_restore(&(saved_sram_state));

	return;
}

/*
*********************************************************************************************************
*                           aw_early_suspend
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      :
*********************************************************************************************************
*/
static int aw_super_standby(suspend_state_t state)
{
	int result = 0;
	suspend_status_flag = 0;
    mem_para_info.axp_enable = standby_axp_enable;

mem_enter:
	if( 1 == mem_para_info.mem_flag){
		invalidate_branch_predictor();
		//must be called to invalidate I-cache inner shareable?
		// I+BTB cache invalidate
		__cpuc_flush_icache_all();
		//disable 0x0000 <---> 0x0000 mapping
		restore_processor_state();
		//destroy 0x0000 <---> 0x0000 mapping
		restore_mapping(MEM_SW_VA_SRAM_BASE);
		mem_arch_resume();
		goto resume;
	}

	save_runtime_context(mem_para_info.saved_runtime_context_svc);
	mem_para_info.mem_flag = 1;
	standby_level = STANDBY_WITH_POWER_OFF;
	mem_para_info.resume_pointer = (void *)&&mem_enter;
	mem_para_info.debug_mask = debug_mask;
	//busy_waiting();
	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		pr_info("resume_pointer = 0x%x. \n", (unsigned int)(mem_para_info.resume_pointer));
	}


#if 1
	/* config system wakeup evetn type */
	if(PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state){
		mem_para_info.axp_event = AXP_MEM_WAKEUP;
	}else if(PM_SUSPEND_BOOTFAST == state){
		mem_para_info.axp_event = AXP_BOOTFAST_WAKEUP;
	}
#endif

	result = aw_early_suspend();
	if(-2 == result){
		//mem_para_info.mem_flag = 1;
		//busy_waiting();
		suspend_status_flag = 2;
		goto mem_enter;
	}else if(-1 == result){
		suspend_status_flag = 1;
		mem_para_info.mem_flag = 0;
		goto suspend_err;
	}

resume:
	aw_late_resume();
	//have been disable dcache in resume1
	//enable_cache();

suspend_err:
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME)){
		pr_info("suspend_status_flag = %d. \n", suspend_status_flag);
	}

	return 0;

}

/*
*********************************************************************************************************
*                           aw_pm_enter
*
*Description: Enter the system sleep state;
*
*Arguments  : state     system sleep state;
*
*Return     : return 0 is process successed;
*
*Notes      : this function is the core function for platform sleep.
*********************************************************************************************************
*/
static int aw_pm_enter(suspend_state_t state)
{
	asm volatile ("stmfd sp!, {r1-r12, lr}" );
	int (*standby)(struct aw_pm_info *arg) = 0;
	int i = 0;

	PM_DBG("enter state %d\n", state);

	if(unlikely(debug_mask&PM_STANDBY_PRINT_IO_STATUS)){
		printk(KERN_INFO "IO status as follow:");
		for(i=0; i<(GPIO_REG_LENGTH); i++){
			printk(KERN_INFO "ADDR = %x, value = %x .\n", \
				IO_ADDRESS(SW_PA_PORTC_IO_BASE) + i*0x04, *(volatile __u32 *)(IO_ADDRESS(SW_PA_PORTC_IO_BASE) + i*0x04));
		}
	}
    standby_info.standby_para.axp_enable = standby_axp_enable;

	if(NORMAL_STANDBY== standby_type){
		print_call_info();
		standby = (int (*)(struct aw_pm_info *arg))SRAM_FUNC_START;
		//move standby code to sram
		memcpy((void *)SRAM_FUNC_START, (void *)&standby_bin_start, (int)&standby_bin_end - (int)&standby_bin_start);
		/* config system wakeup evetn type */
		if(PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state){
			standby_info.standby_para.axp_src = AXP_MEM_WAKEUP;
		}else if(PM_SUSPEND_BOOTFAST == state){
			standby_info.standby_para.axp_src = AXP_BOOTFAST_WAKEUP;
		}
		standby_info.standby_para.event_enable = (SUSPEND_WAKEUP_SRC_EXINT | SUSPEND_WAKEUP_SRC_ALARM);

		/* goto sram and run */
		standby(&standby_info);
#ifdef CHECK_INT_SRC
		check_int_src();
#endif
	}else if(SUPER_STANDBY == standby_type){
			print_call_info();
			aw_super_standby(state);
	}

	asm volatile ("ldmfd sp!, {r1-r12, lr}" );
	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_wake
*
*Description: platform wakeup;
*
*Arguments  : none;
*
*Return     : none;
*
*Notes      : This function called when the system has just left a sleep state, right after
*             the nonboot CPUs have been enabled and before device drivers' early resume
*             callbacks are executed. This function is opposited to the aw_pm_prepare_late;
*********************************************************************************************************
*/
static void aw_pm_wake(void)
{
    PM_DBG("platform wakeup, wakesource is:0x%x\n", standby_info.standby_para.event);
}

/*
*********************************************************************************************************
*                           aw_pm_finish
*
*Description: Finish wake-up of the platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called right prior to calling device drivers' regular suspend
*              callbacks. This function is opposited to the aw_pm_prepare function.
*********************************************************************************************************
*/
void aw_pm_finish(void)
{
    PM_DBG("platform wakeup finish\n");
}


/*
*********************************************************************************************************
*                           aw_pm_end
*
*Description: Notify the platform that system is in work mode now.
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called by the PM core right after resuming devices, to indicate to
*             the platform that the system has returned to the working state or
*             the transition to the sleep state has been aborted. This function is opposited to
*             aw_pm_begin function.
*********************************************************************************************************
*/
void aw_pm_end(void)
{

	//standby_type = NON_STANDBY;
	//uart_init(2, 115200);
	//save_mem_status(LATE_RESUME_START |0x10);
	//print_flag = 0;
#ifndef GET_CYCLE_CNT
	#ifndef IO_MEASURE
			restore_perfcounter();
	#endif
#endif

#ifdef IO_MEASURE
	io_init();
	io_high(1);
#endif

	PM_DBG("aw_pm_end!\n");
}


/*
*********************************************************************************************************
*                           aw_pm_recover
*
*Description: Recover platform from a suspend failure;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function alled by the PM core if the suspending of devices fails.
*             This callback is optional and should only be implemented by platforms
*             which require special recovery actions in that situation.
*********************************************************************************************************
*/
void aw_pm_recover(void)
{
    PM_DBG("aw_pm_recover\n");
}


/*
    define platform_suspend_ops which is registered into PM core.
*/
static struct platform_suspend_ops aw_pm_ops = {
    .valid = aw_pm_valid,
    .begin = aw_pm_begin,
    .prepare = aw_pm_prepare,
    .prepare_late = aw_pm_prepare_late,
    .enter = aw_pm_enter,
    .wake = aw_pm_wake,
    .finish = aw_pm_finish,
    .end = aw_pm_end,
    .recover = aw_pm_recover,
};


/*
*********************************************************************************************************
*                           aw_pm_init
*
*Description: initial pm sub-system for platform;
*
*Arguments  : none;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_pm_init(void)
{
    script_item_u setting_mod;
    script_item_value_type_e type;

    PM_DBG("aw_pm_init!\n");
    type = script_get_item("pm_para", "standby_mode", &setting_mod);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: script_parser_fetch err. \n", __func__);
		standby_mode = 0;
    }else{
        standby_mode = setting_mod.val;
		pr_info("standby_mode = %d. \n", standby_mode);
    }

    suspend_set_ops(&aw_pm_ops);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_exit
*
*Description: exit pm sub-system on platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void __exit aw_pm_exit(void)
{
	PM_DBG("aw_pm_exit!\n");
	if(true == mem_allocated_flag){
		mem_allocated_flag = false;
	}
	suspend_set_ops(NULL);
}

module_init(aw_pm_init);
module_exit(aw_pm_exit);

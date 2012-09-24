#ifndef _PM_H
#define _PM_H

/*
 * Copyright (c) 2011-2015 yanggq.young@newbietech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm_config.h"
#include "pm_errcode.h"
#include "pm_debug.h"
#include "mem_cpu.h"
#include "mem_serial.h"
#include "mem_printk.h"
#include "mach/platform.h"
#include "mem_misc.h"
#include "mach/ccmu.h"

#define AXP_IICBUS      (1)
#define TWI_CHECK_TIMEOUT       (0xf2ff)
#define PM_STANDBY_PRINT_STANDBY (1U << 0)
#define PM_STANDBY_PRINT_RESUME (1U << 1)
#define PM_STANDBY_PRINT_IO_STATUS (1U << 2)


typedef struct __MEM_TMR_REG
{
	// offset:0x00
	volatile __u32	 IntCtl;
	volatile __u32	 IntSta;
	volatile __u32	 reserved0[2];
	// offset:0x10
	volatile __u32	 Tmr0Ctl;
	volatile __u32	 Tmr0IntVal;
	volatile __u32	 Tmr0CntVal;
	volatile __u32	 reserved1;
	// offset:0x20
	volatile __u32	 Tmr1Ctl;
	volatile __u32	 Tmr1IntVal;
	volatile __u32	 Tmr1CntVal;
	volatile __u32	 reserved2;
	// offset:0x30
	volatile __u32	 Tmr2Ctl;
	volatile __u32	 Tmr2IntVal;
	volatile __u32	 Tmr2CntVal;
	volatile __u32	 reserved3;
	// offset:0x40
	volatile __u32	 Tmr3Ctl;
	volatile __u32	 Tmr3IntVal;
	volatile __u32	 reserved4[2];
	// offset:0x50
	volatile __u32	 Tmr4Ctl;
	volatile __u32	 Tmr4IntVal;
	volatile __u32	 Tmr4CntVal;
	volatile __u32	 reserved5;
	// offset:0x60
	volatile __u32	 Tmr5Ctl;
	volatile __u32	 Tmr5IntVal;
	volatile __u32	 Tmr5CntVal;
	volatile __u32	 reserved6[5];
	// offset:0x80
	volatile __u32	 AvsCtl;
	volatile __u32	 Avs0Cnt;
	volatile __u32	 Avs1Cnt;
	volatile __u32	 AvsDiv;
	// offset:0x90
	volatile __u32	 DogCtl;
	volatile __u32	 DogMode;
	volatile __u32	 reserved7[2];

} __mem_tmr_reg_t;

enum interrupt_source_e{
#ifdef SUN7I_FPGA_SIM
	    INT_SOURCE_EXTNMI   = 0,
	    INT_SOURCE_IR0      = 5,
	    INT_SOURCE_IR1      = 6,
	    INT_SOURCE_KEYPAD   = 21,
	    INT_SOURCE_TIMER0   = 22,
	    INT_SOURCE_TIMER1   = 23,
	    INT_SOURCE_ALARM    = 24,
	    INT_SOURCE_TOUCHPNL = 29,
	    INT_SOURCE_LRADC    = 31,
	    INT_SOURCE_USB0     = 38,
	    INT_SOURCE_USB1     = 39,
	    INT_SOURCE_USB2     = 40,
	    INT_SOURCE_USB3     = 64,
	    INT_SOURCE_USB4     = 65,
#else
	    INT_SOURCE_EXTNMI   = 0,
	    INT_SOURCE_IR0      = 5,
	    INT_SOURCE_IR1      = 6,
	    INT_SOURCE_KEYPAD   = 21,
	    INT_SOURCE_TIMER0   = 22,
	    INT_SOURCE_TIMER1   = 23,
	    INT_SOURCE_ALARM    = 24,
	    INT_SOURCE_TOUCHPNL = 29,
	    INT_SOURCE_LRADC    = 31,
	    INT_SOURCE_USB0     = 38,
	    INT_SOURCE_USB1     = 39,
	    INT_SOURCE_USB2     = 40,
	    INT_SOURCE_USB3     = 64,
	    INT_SOURCE_USB4     = 65,
#endif
};

struct clk_div_t {
    __u32   cpu_div:4;      /* division of cpu clock, divide core_pll */
    __u32   axi_div:4;      /* division of axi clock, divide cpu clock*/
    __u32   ahb_div:4;      /* division of ahb clock, divide axi clock*/
    __u32   apb_div:4;      /* division of apb clock, divide ahb clock*/
    __u32   reserved:16;
};
struct pll_factor_t {
    __u8    FactorN;
    __u8    FactorK;
    __u8    FactorM;
    __u8    FactorP;
    __u32   Pll;
};

struct mmu_state {
	/* CR0 */
	__u32 cssr;	/* Cache Size Selection */
	/* CR1 */
	__u32 cr;		/* Control */
	__u32 cacr;	/* Coprocessor Access Control */
	/* CR2 */
	__u32  ttb_0r;	/* Translation Table Base 0 */
	__u32  ttb_1r;	/* Translation Table Base 1 */
	__u32  ttbcr;	/* Translation Talbe Base Control */

	/* CR3 */
	__u32 dacr;	/* Domain Access Control */

	/*cr10*/
	__u32 prrr;	/* Primary Region Remap Register */
	__u32 nrrr;	/* Normal Memory Remap Register */
};

/**
*@brief struct of super mem
*/
struct aw_mem_para{
	void **resume_pointer;
	__u32 mem_flag;
	//__s32 suspend_vdd;
	__s32 suspend_dcdc2;
	__s32 suspend_dcdc3;
	__u32 suspend_freq;
	__u32 axp_event;
	__u32 sys_event;
	__u32 debug_mask;
	struct clk_div_t clk_div;
	struct pll_factor_t pll_factor;
	__u32 saved_runtime_context_svc[RUNTIME_CONTEXT_SIZE];
	__u8 saved_dram_training_area[DRAM_TRANING_SIZE];
	struct mmu_state saved_mmu_state;
	struct saved_context saved_cpu_context;
};


static inline __u32 raw_lib_udiv(__u32 dividend, __u32 divisior)
{
    __u32   tmpDiv = (__u32)divisior;
    __u32   tmpQuot = 0;
    __s32   shift = 0;

    if(!divisior)
    {
        /* divide 0 error abort */
        return 0;
    }

    while(!(tmpDiv & ((__u32)1<<31)))
    {
        tmpDiv <<= 1;
        shift ++;
    }

    do
    {
        if(dividend >= tmpDiv)
        {
            dividend -= tmpDiv;
            tmpQuot = (tmpQuot << 1) | 1;
        }
        else
        {
            tmpQuot = (tmpQuot << 1) | 0;
        }
        tmpDiv >>= 1;
        shift --;
    } while(shift >= 0);

    return tmpQuot;
}

extern void __aeabi_idiv(void);
extern void __aeabi_idivmod(void);
extern void __aeabi_uidiv(void);
extern void __aeabi_uidivmod(void);

#if 0
extern unsigned int save_sp_nommu(void);

extern void mem_flush_tlb(void);
extern void mem_preload_tlb_nommu(void);

extern void flush_icache(void);
extern void flush_dcache(void);
extern void enable_icache(void);
extern void invalidate_dcache(void);

#endif

#if 0
extern unsigned int save_sp(void);
extern void restore_sp(unsigned int sp);

extern void mem_flush_tlb(void);
extern void mem_preload_tlb(void);
void disable_mmu(void);
void enable_mmu(void);
void set_ttbr0(void);

extern void flush_icache(void);
extern void flush_dcache(void);
extern void invalidate_dcache(void);
void disable_cache_invalidate(void);

extern int jump_to_resume0(void* pointer);
static __s32 suspend_with_nommu(void);
static __s32 suspend_with_mmu(void);
#endif

#if 0
extern unsigned int save_sp(void);
extern void clear_reg_context(void);


extern void restore_mmu_state(struct mmu_state *saved_mmu_state);
extern void mem_flush_tlb(void);
extern void mem_preload_tlb_nommu(void);



extern void disable_program_flow_prediction(void);
extern void invalidate_branch_predictor(void);
extern void enable_program_flow_prediction(void);

extern void enable_cache(void);
extern void disable_cache(void);
extern void disable_dcache(void);
extern void disable_l2cache(void);
extern void flush_icache(void);
extern void flush_dcache(void);
extern void invalidate_dcache(void);

extern int jump_to_resume(void* pointer, __u32 *addr);

#endif

extern unsigned int save_sp_nommu(void);
extern unsigned int save_sp(void);
extern void clear_reg_context(void);
extern void restore_sp(unsigned int sp);

extern void enable_cache(void);
extern void enable_icache(void);
extern void disable_cache(void);
extern void disable_dcache(void);
extern void disable_l2cache(void);
extern void flush_icache(void);
extern void flush_dcache(void);
extern void invalidate_dcache(void);

extern void restore_mmu_state(struct mmu_state *saved_mmu_state);
extern void mem_flush_tlb(void);
extern void mem_preload_tlb_nommu(void);
void disable_mmu(void);
void enable_mmu(void);
void set_ttbr0(void);

extern void disable_program_flow_prediction(void);
extern void invalidate_branch_predictor(void);
extern void enable_program_flow_prediction(void);

extern int jump_to_resume(void* pointer, __u32 *addr);
extern int jump_to_resume0(void* pointer);


#endif /*_PM_H*/

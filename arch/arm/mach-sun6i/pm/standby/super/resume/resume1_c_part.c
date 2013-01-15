/* these code will be removed to sram.
 * function: open the mmu, and jump to dram, for continuing resume*/
#include "./../super_i.h"


static struct aw_mem_para mem_para_info;

extern char *__bss_start;
extern char *__bss_end;
static __s32 dcdc2, dcdc3;
static __u32 sp_backup;
static char    *tmpPtr = (char *)&__bss_start;
static __u32 status = 0; 

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#define MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#undef MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#if defined(ENTER_SUPER_STANDBY) || defined(ENTER_SUPER_STANDBY_WITH_NOMMU) || defined(WATCH_DOG_RESET)
#undef MMU_OPENED
#define POWER_OFF
#define FLUSH_TLB
#define SET_COPRO_REG
//#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

int resume1_c_part(void)
{
	//
	//busy_waiting();
	/* clear bss segment */
	do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);
	
#ifdef SET_COPRO_REG
	set_copro_default();
#endif

#ifdef MMU_OPENED
	save_mem_status(RESUME1_START |0x02);

	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	mem_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1), sizeof(mem_para_info));
#else
	//config jtag gpio
	*(volatile __u32 * )(0x01c20800 + 0x100) = 0x00033330;

	save_mem_status_nommu(RESUME1_START |0x02);

	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	mem_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1_PA), sizeof(mem_para_info));
	
	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		//config uart
		serial_init_nommu();
		serial_puts_nommu("resume1: 0. \n");
	}
#if 1
	/*restore freq from 408M to orignal freq.*/
	//busy_waiting();
	mem_clk_setdiv(&mem_para_info.clk_div);
	mem_clk_set_pll_factor(&mem_para_info.pll_factor);
	change_runtime_env(0);
	delay_ms(mem_para_info.suspend_delay_ms);
	
	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		serial_puts_nommu("resume1: 1. before restore mmu. \n");
	}
#endif	
	/*restore mmu configuration*/
	save_mem_status_nommu(RESUME1_START |0x03);
	//save_mem_status(RESUME1_START |0x03);

	restore_mmu_state(&(mem_para_info.saved_mmu_state));
	save_mem_status(RESUME1_START |0x13);

#endif

//before jump to late_resume	
#ifdef FLUSH_TLB
	save_mem_status(RESUME1_START |0x9);
	mem_flush_tlb();
#endif

#ifdef FLUSH_ICACHE
	save_mem_status(RESUME1_START |0xa);
	flush_icache();
#endif

	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		serial_puts("resume1: 3. after restore mmu, before jump.\n");
	}

	//busy_waiting();
	jump_to_resume((void *)mem_para_info.resume_pointer, mem_para_info.saved_runtime_context_svc);

	return;
}


/*******************************************************************************
* interface : set_pll
*	prototype		£ºvoid set_pll( void )
*	function		: adjust CPU frequence, from 24M hosc to pll1 384M
*	input para	: void
*	return value	: void
*	note:
*******************************************************************************/
void set_pll( void )
{
	/*when enter this func, state is as follow:
	 *	1. mmu is disable.
	 *	2. clk is 24M hosc (?)
	 *
	 */
	__ccmu_reg_list_t   *CmuReg;

	CmuReg = (__ccmu_reg_list_t   *)mem_clk_init(0);

	save_mem_status_nommu(RESUME1_START |0x26);
	//switch to 24M
	*(volatile __u32 *)(&CmuReg->SysClkDiv) = 0x00010000;
	//enable pll1 and setting PLL1 to 408M
	*(volatile __u32 *)(&CmuReg->Pll1Ctl) = (0x00001000) | (0x80000000); //N = 16, K=M=1 -> pll1 = 17*24 = 408M
	//setting pll6 to 600M
	//enable pll6
	*(volatile __u32 *)(&CmuReg->Pll6Ctl) = 0x80041811;
	//delay 
	//need reconstruction!!
	save_mem_status_nommu(RESUME1_START |0x27);
	init_perfcounters(1, 0); //need double check..
	change_runtime_env(0);
	delay_ms(10);

	save_mem_status_nommu(RESUME1_START |0x28);
	//switch to PLL1
	*(volatile __u32 *)(&CmuReg->SysClkDiv) = 0x00020000;
	
	return ;
}



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

#define IS_WFI_MODE(cpu)	(*(volatile unsigned int *)((((0x01f01c00)) + (0x48 + (cpu)*0x40))) & (1<<2))


int resume1_c_part(void)
{

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

	if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
		//config uart
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
	__ccmu_reg_list_t   *CmuReg;
	__u32 cpu_id = 0;
	__u32 cpu1_reset = 0;
	__u32 cpu2_reset = 0;
	__u32 cpu3_reset = 0;
	__u32 pwr_reg = 0;

	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r"(cpu_id)); //Read CPU ID register
	cpu_id &= 0x3;
	if(0 == cpu_id){
		/* clear bss segment */
		*(volatile __u32 *)(STATUS_REG_PA) |= (0xf<<0);
		do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

		/*when enter this func, state is as follow:
		*	1. mmu is disable.
		*	2. clk is 24M hosc (?)
		*
		*/
		CmuReg = (__ccmu_reg_list_t   *)mem_clk_init(0);

		//config jtag gpio
		*(volatile __u32 * )(0x01c20800 + 0x100) = 0x00033330;

		save_mem_status_nommu(RESUME1_START |0x26 | (cpu_id<<8));

		//switch to 24M
		*(volatile __u32 *)(&CmuReg->SysClkDiv) = 0x00010000;

		//get mem para info
		//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
		mem_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1_PA), sizeof(mem_para_info));
		//config pll para
		mem_clk_set_misc(&mem_para_info.clk_misc);
		//enable pll1 and setting PLL1 to 408M
		*(volatile __u32 *)(&CmuReg->Pll1Ctl) = (0x00001011) | (0x80000000); //N = 16, K=M=2 -> pll1 = 17*24 = 408M
		//setting pll6 to 600M
		//enable pll6
		*(volatile __u32 *)(&CmuReg->Pll6Ctl) = 0x80041811;

		init_perfcounters(1, 0); //need double check..
		change_runtime_env(0);
		delay_ms(10);		
	}else if(1 == cpu_id){
		*(volatile __u32 *)(STATUS_REG_PA) |= (0xf<<4);
		//standby_delay_cycle(340000);
	}else if(2 == cpu_id){
		*(volatile __u32 *)(STATUS_REG_PA) |= (0xf<<8);
		//standby_delay_cycle(440000);
	}else if(3 == cpu_id){
		*(volatile __u32 *)(STATUS_REG_PA) |= (0xf<<0xc);
		//standby_delay_cycle(540000);
	}
	
#if 1
	if(0 == cpu_id){
		save_mem_status_nommu(RESUME1_START |0x27 | (cpu_id<<8));
		if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){
			//config uart
			serial_init_nommu();
		}
		save_mem_status_nommu(RESUME1_START |0x28 | (cpu_id<<8));
		//printk_nommu("cpu_id = %d. \n", cpu_id);
		
		while(1){
			cpu1_reset = (0x3 == (*(volatile __u32 *)(0x01f01c00 + 0x80)) & 0x3)?1:0;
			cpu2_reset = (0x3 == (*(volatile __u32 *)(0x01f01c00 + 0xc0)) & 0x3)?1:0;
			cpu3_reset = (0x3 == (*(volatile __u32 *)(0x01f01c00 + 0x100)) & 0x3)?1:0;
			//printk_nommu("cpu1_reset = %d. \n", cpu1_reset);
			//printk_nommu("cpu2_reset = %d. \n", cpu2_reset);
			//printk_nommu("cpu3_reset = %d. \n", cpu3_reset);
			save_mem_status_nommu(RESUME1_START |0x29 | (cpu_id<<8));
			if(cpu1_reset&&cpu2_reset&&cpu3_reset){
				save_mem_status_nommu(RESUME1_START |0x30 | (cpu_id<<8));
				if((IS_WFI_MODE(1) && IS_WFI_MODE(2) && IS_WFI_MODE(3))){
					save_mem_status_nommu(RESUME1_START |0x32 | (cpu_id<<8));
					/* step9: set up cpu1+ power-off signal */
					//printk_nommu("set up cpu1+ power-off signal.\n");
					pwr_reg = (*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG));
					pwr_reg |= (0xe); //0b1110
					(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPU_PWROFF_REG)) = pwr_reg;
					delay_ms(1);

					save_mem_status_nommu(RESUME1_START |0x33 | (cpu_id<<8));
					/* step10: active the power output clamp */
					//printk_nommu("active the power output clamp.\n");
					(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(1))) = 0xff;
					(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(2))) = 0xff;
					(*(volatile __u32 *)((AW_R_PRCM_BASE) + AW_CPUX_PWR_CLAMP(3))) = 0xff;
										
					break;
				}
				if(unlikely(mem_para_info.debug_mask&PM_STANDBY_PRINT_RESUME)){			
					printk_nommu("cpu1+ wfi state as follow: \n");
					printk_nommu("cpu1 wfi = %d. \n", IS_WFI_MODE(1));
					printk_nommu("cpu2 wfi = %d. \n", IS_WFI_MODE(2));
					printk_nommu("cpu3 wfi = %d. \n", IS_WFI_MODE(3));
				}
			}
			//printk_nommu("cpu1+ reset state as follow: \n");

		}
		//printk_nommu("cpu1 go on wakeup the system...\n");

	}else{
		//just waiting.
		while(1){		
			//let the cpu1+ enter wfi state;
			/* step3: execute a CLREX instruction */
			asm("clrex" : : : "memory", "cc");

			/* step5: execute an ISB instruction */
			asm volatile ("isb");
			/* step6: execute a DSB instruction  */
			asm volatile ("dsb");

			/* step7: execute a WFI instruction */
			while(1) {
				asm("wfi" : : : "memory", "cc");
			}
		}
	}
#endif
	//switch to PLL1
	*(volatile __u32 *)(&CmuReg->SysClkDiv) = 0x00020000;
	
	return ;
}



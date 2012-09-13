//#include "pm_types.h"
#include <asm/delay.h>
#include <linux/delay.h>
#include "pm_i.h"

//#define CHECK_RESTORE_STATUS

/*
*********************************************************************************************************
*                           mem_clk_init
*
*Description: ccu init for platform mem
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 mem_clk_save(struct clk_state *pclk_state)
{
	return 0;
}


/*
*********************************************************************************************************
*                           mem_clk_exit
*
*Description: ccu exit for platform mem
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 mem_clk_restore(struct clk_state *pclk_state)
{
	return 0;
}

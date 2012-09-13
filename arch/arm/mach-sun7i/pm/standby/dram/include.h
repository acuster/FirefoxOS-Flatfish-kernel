/*
 * config.h
 *
 *  Created on: 2012-4-26
 *      Author: Administrator
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "./include/all.h"

typedef unsigned int uint32;
typedef unsigned int size_t;

#define FPGA_PLATFORM
#define CONFIG_USE_DDR

#define PLL1_CPUX_CLK	(24000000)
#define PLL5_DDR_CLK	(24000000)
#define PLL6_DEV_CLK	(24000000)

#ifdef FPGA_PLATFORM
	//#define DDR2_32B
	//#define DDR3_32B
	//#define DDR2_FPGA_BJ530 //DDR2_32B
	#define DDR2_FPGA_S2C
	//#define LPDDR2_FPGA_S2C_2CS_2CH
	#define MCTL_CLK			PLL5_DDR_CLK
	#define MCTL_ACCESS_MODE		1				//0: sequence mode, 1: interleave mode
	#define MCTL_CHANNEL_NUM		1
	#ifdef DDR2_FPGA_S2C
		#define MCTL_CS_NUM		2
	#else
		#define MCTL_CS_NUM		2
	#endif
#else
//#define DDR2_32B
#define DDR3_32B
//#define LPDDR2_32B
#define MCTL_CLK			(PLL5_DDR_CLK/1000000)
#define MCTL_ACCESS_MODE		1				//0: sequence mode, 1: interleave mode
#define MCTL_CHANNEL_NUM		1
#define MCTL_CS_NUM			1
#endif

#define MCTL_CCU_BASE	(0x01c20000)
#define R_PRCM_BASE		(0x01f01400)

#define CCM_PLL5_DDR_CTRL			(MCTL_CCU_BASE + 0x20)
#define CCM_AXI_GATE_CTRL			(MCTL_CCU_BASE + 0x5c)
#define CCM_AHB1_GATE0_CTRL			(MCTL_CCU_BASE + 0x60)
#define CCU_AHBGATE1				(MCTL_CCU_BASE + 0x64)
#define CCM_MDFS_CLK_CTRL			(MCTL_CCU_BASE + 0xf0)
//#define CCM_DRAMCLK_CFG_CTRL		(MCTL_CCU_BASE + 0xf4)
//#define CCM_AHB1_RST_REG0			(MCTL_CCU_BASE + 0x2c0)

#define R_VDD_SYS_PWROFF_GATE		(R_PRCM_BASE + 0x110)

#define aw_delay(x)	wait(x)
#define get_wvalue(a)	readl(a)
#define put_wvalue(a, v)	writel((v), (a))

#endif /* CONFIG_H_ */

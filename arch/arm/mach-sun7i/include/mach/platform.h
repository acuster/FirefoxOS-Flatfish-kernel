/*
 * arch\arm\mach-sun7i\include\mach\platform.h
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * Benn Huang <benn@reuuimllatech.com>
 *
 * core header file for Lichee Linux BSP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SW_PLATFORM_H
#define __SW_PLATFORM_H

/*
 * old macros from sun4i
 */

/* Physical Address */
#define SW_PA_BROM_START                  0xffff0000
#define SW_PA_BROM_END                    0xffff7fff   /* 32KB */

#define SW_PA_SRAM_BASE                   0x00000000
#define SW_PA_SDRAM_START                 0x40000000
#define SW_PA_IO_BASE                     0x01c00000
#define SW_PA_SRAM_IO_BASE                0x01c00000   /* 4KB */
#define SW_PA_DRAM_IO_BASE                0x01c01000
#define SW_PA_DMAC_IO_BASE                0x01c02000
#define SW_PA_NANDFLASHC_IO_BASE          0x01c03000
#define SW_PA_TSI_IO_BASE                 0x01c04000
#define SW_PA_SPI0_IO_BASE                0x01c05000
#define SW_PA_SPI1_IO_BASE                0x01c06000
#define SW_PA_MSCC_IO_BASE                0x01c07000
#define SW_PA_TVD_IO_BASE                 0x01c08000
#define SW_PA_CSI0_IO_BASE                0x01c09000
#define SW_PA_TVE_IO_BASE                 0x01c0a000
#define SW_PA_EMAC_IO_BASE                0x01c0b000
#define SW_PA_TCON0_IO_BASE               0x01c0c000
#define SW_PA_TCON1_IO_BASE               0x01c0d000
#define SW_PA_VE_IO_BASE                  0x01c0e000
#define SW_PA_SDC0_IO_BASE                0x01c0f000
#define SW_PA_SDC1_IO_BASE                0x01c10000
#define SW_PA_SDC2_IO_BASE                0x01c11000
#define SW_PA_SDC3_IO_BASE                0x01c12000
#define SW_PA_USB0_IO_BASE                0x01c13000
#define SW_PA_USB1_IO_BASE                0x01c14000
#define SW_PA_SSE_IO_BASE                 0x01c15000
#define SW_PA_HDMI_IO_BASE                0x01c16000
#define SW_PA_SPI2_IO_BASE                0x01c17000
#define SW_PA_SATA_IO_BASE                0x01c18000
#define SW_PA_PATA_IO_BASE                0x01c19000
#define SW_PA_ACE_IO_BASE                 0x01c1a000
#define SW_PA_TVE1_IO_BASE                0x01c1b000
#define SW_PA_USB2_IO_BASE                0x01c1c000
#define SW_PA_CSI1_IO_BASE                0x01c1d000
#define SW_PA_TZASC_IO_BASE               0x01c1e000
#define SW_PA_SPI3_IO_BASE                0x01c1f000
#define SW_PA_CCM_IO_BASE                 0x01c20000
#define SW_PA_INT_IO_BASE                 0x01c20400
#define SW_PA_PORTC_IO_BASE               0x01c20800
#define SW_PA_TIMERC_IO_BASE              0x01c20c00
#define SW_PA_SPDIF_IO_BASE               0x01c21000
#define SW_PA_AC97_IO_BASE                0x01c21400
#define SW_PA_IR0_IO_BASE                 0x01c21800
#define SW_PA_IR1_IO_BASE                 0x01c21c00
#define SW_PA_IIS_IO_BASE                 0x01c22400
#define SW_PA_LRADC_IO_BASE               0x01c22800
#define SW_PA_ADDA_IO_BASE                0x01c22c00
#define SW_PA_KEYPAD_IO_BASE              0x01c23000
#define SW_PA_TZPC_IO_BASE                0x01c23400
#define SW_PA_SID_IO_BASE                 0x01c23800
#define SW_PA_SJTAG_IO_BASE               0x01c23c00
#define SW_PA_TP_IO_BASE                  0x01c25000
#define SW_PA_PMU_IO_BASE                 0x01c25400
#define SW_PA_UART0_IO_BASE               0x01c28000
#define SW_PA_UART1_IO_BASE               0x01c28400
#define SW_PA_UART2_IO_BASE               0x01c28800
#define SW_PA_UART3_IO_BASE               0x01c28c00
#define SW_PA_UART4_IO_BASE               0x01c29000
#define SW_PA_UART5_IO_BASE               0x01c29400
#define SW_PA_UART6_IO_BASE               0x01c29800
#define SW_PA_UART7_IO_BASE               0x01c29c00
#define SW_PA_PS20_IO_BASE                0x01c2a000
#define SW_PA_PS21_IO_BASE                0x01c2a400
#define SW_PA_TWI0_IO_BASE                0x01c2ac00
#define SW_PA_TWI1_IO_BASE                0x01c2b000
#define SW_PA_TWI2_IO_BASE                0x01c2b400
#define SW_PA_CAN0_IO_BASE                0x01c2bc00
#define SW_PA_CAN1_IO_BASE                0x01c2c000
#define SW_PA_SCR_IO_BASE                 0x01c2c400
#define SW_PA_GPS_IO_BASE                 0x01c30000
#define SW_PA_MALI_IO_BASE                0x01c40000
#define SW_PA_DEFE0_IO_BASE               0x01e00000
#define SW_PA_DEFE1_IO_BASE               0x01e20000
#define SW_PA_DEBE0_IO_BASE               0x01e60000
#define SW_PA_DEBE1_IO_BASE               0x01e40000
#define SW_PA_MP_IO_BASE                  0x01e80000
#define SW_PA_AVG_IO_BASE                 0x01ea0000
#define SW_PA_BROM_BASE                   0xffff0000



/* Virtual Address */
#define SW_VA_SRAM_BASE                   0xf0000000	/*16KB*/
#define SW_VA_BROM_BASE                   0xf0100000	/*64KB*/

#define SW_VA_IO_BASE                     0xf1c00000
#define SW_VA_SRAM_IO_BASE                0xf1c00000   /* 4KB */
#define SW_VA_DRAM_IO_BASE                0xf1c01000
#define SW_VA_DMAC_IO_BASE                0xf1c02000
#define SW_VA_NANDFLASHC_IO_BASE          0xf1c03000
#define SW_VA_TSI_IO_BASE                 0xf1c04000
#define SW_VA_SPI0_IO_BASE                0xf1c05000
#define SW_VA_SPI1_IO_BASE                0xf1c06000
#define SW_VA_MSCC_IO_BASE                0xf1c07000
#define SW_VA_TVD_IO_BASE                 0xf1c08000
#define SW_VA_CSI0_IO_BASE                0xf1c09000
#define SW_VA_TVE_IO_BASE                 0xf1c0a000
#define SW_VA_EMAC_IO_BASE                0xf1c0b000
#define SW_VA_TCON0_IO_BASE               0xf1c0c000
#define SW_VA_TCON1_IO_BASE               0xf1c0d000
#define SW_VA_VE_IO_BASE                  0xf1c0e000
#define SW_VA_SDC0_IO_BASE                0xf1c0f000
#define SW_VA_SDC1_IO_BASE                0xf1c10000
#define SW_VA_SDC2_IO_BASE                0xf1c11000
#define SW_VA_SDC3_IO_BASE                0xf1c12000
#define SW_VA_USB0_IO_BASE                0xf1c13000
#define SW_VA_USB1_IO_BASE                0xf1c14000
#define SW_VA_SSE_IO_BASE                 0xf1c15000
#define SW_VA_HDMI_IO_BASE                0xf1c16000
#define SW_VA_SPI2_IO_BASE                0xf1c17000
#define SW_VA_SATA_IO_BASE                0xf1c18000
#define SW_VA_PATA_IO_BASE                0xf1c19000
#define SW_VA_ACE_IO_BASE                 0xf1c1a000
#define SW_VA_TVE1_IO_BASE                0xf1c1b000
#define SW_VA_USB2_IO_BASE                0xf1c1c000
#define SW_VA_CSI1_IO_BASE                0xf1c1d000
#define SW_VA_TZASC_IO_BASE               0xf1c1e000
#define SW_VA_SPI3_IO_BASE                0xf1c1f000
#define SW_VA_CCM_IO_BASE                 0xf1c20000
#define SW_VA_INT_IO_BASE                 0xf1c20400
#define SW_VA_PORTC_IO_BASE               0xf1c20800
#define SW_VA_TIMERC_IO_BASE              0xf1c20c00
#define SW_VA_SPDIF_IO_BASE               0xf1c21000
#define SW_VA_AC97_IO_BASE                0xf1c21400
#define SW_VA_IR0_IO_BASE                 0xf1c21800
#define SW_VA_IR1_IO_BASE                 0xf1c21c00
#define SW_VA_IIS_IO_BASE                 0xf1c22400
#define SW_VA_LRADC_IO_BASE               0xf1c22800
#define SW_VA_ADDA_IO_BASE                0xf1c22c00
#define SW_VA_KEYPAD_IO_BASE              0xf1c23000
#define SW_VA_TZPC_IO_BASE                0xf1c23400
#define SW_VA_SID_IO_BASE                 0xf1c23800
#define SW_VA_SJTAG_IO_BASE               0xf1c23c00
#define SW_VA_TP_IO_BASE                  0xf1c25000
#define SW_VA_PMU_IO_BASE                 0xf1c25400
#define SW_VA_UART0_IO_BASE               0xf1c28000
#define SW_VA_UART1_IO_BASE               0xf1c28400
#define SW_VA_UART2_IO_BASE               0xf1c28800
#define SW_VA_UART3_IO_BASE               0xf1c28c00
#define SW_VA_UART4_IO_BASE               0xf1c29000
#define SW_VA_UART5_IO_BASE               0xf1c29400
#define SW_VA_UART6_IO_BASE               0xf1c29800
#define SW_VA_UART7_IO_BASE               0xf1c29c00
#define SW_VA_PS20_IO_BASE                0xf1c2a000
#define SW_VA_PS21_IO_BASE                0xf1c2a400
#define SW_VA_TWI0_IO_BASE                0xf1c2ac00
#define SW_VA_TWI1_IO_BASE                0xf1c2b000
#define SW_VA_TWI2_IO_BASE                0xf1c2b400
#define SW_VA_CAN0_IO_BASE                0xf1c2bc00
#define SW_VA_CAN1_IO_BASE                0xf1c2c000
#define SW_VA_SCR_IO_BASE                 0xf1c2c400
#define SW_VA_GPS_IO_BASE                 0xf1c30000
#define SW_VA_MALI_IO_BASE                0xf1c40000
#define SW_VA_DEFE0_IO_BASE               0xf1e00000
#define SW_VA_DEFE1_IO_BASE               0xf1e20000
#define SW_VA_DEBE0_IO_BASE               0xf1e60000
#define SW_VA_DEBE1_IO_BASE               0xf1e40000
#define SW_VA_MP_IO_BASE                  0xf1e80000
#define SW_VA_AVG_IO_BASE                 0xf1ea0000



/**
 * Timer registers addr
 *
 */

#define SW_TIMER_INT_CTL_REG              (SW_VA_TIMERC_IO_BASE + 0x00)
#define SW_TIMER_INT_STA_REG              (SW_VA_TIMERC_IO_BASE + 0x04)
#define SW_TIMER0_CTL_REG                 (SW_VA_TIMERC_IO_BASE + 0x10)
#define SW_TIMER0_INTVAL_REG              (SW_VA_TIMERC_IO_BASE + 0x14)
#define SW_TIMER0_CNTVAL_REG              (SW_VA_TIMERC_IO_BASE + 0x18)


/**
 * Interrupt controller registers
 *
 */
#define SW_INT_VECTOR_REG                 (SW_VA_INT_IO_BASE + 0x00)
#define SW_INT_BASE_ADR_REG               (SW_VA_INT_IO_BASE + 0x04)
#define SW_INT_PROTECTION_REG             (SW_VA_INT_IO_BASE + 0x08)
#define SW_INT_NMI_CTRL_REG               (SW_VA_INT_IO_BASE + 0x0c)
#define SW_INT_IRQ_PENDING_REG0           (SW_VA_INT_IO_BASE + 0x10)
#define SW_INT_IRQ_PENDING_REG1           (SW_VA_INT_IO_BASE + 0x14)
#define SW_INT_IRQ_PENDING_REG2           (SW_VA_INT_IO_BASE + 0x18)

#define SW_INT_FIQ_PENDING_REG0           (SW_VA_INT_IO_BASE + 0x20)
#define SW_INT_FIQ_PENDING_REG1           (SW_VA_INT_IO_BASE + 0x24)
#define SW_INT_FIQ_PENDING_REG2           (SW_VA_INT_IO_BASE + 0x28)

#define SW_INT_SELECT_REG0                (SW_VA_INT_IO_BASE + 0x30)
#define SW_INT_SELECT_REG1                (SW_VA_INT_IO_BASE + 0x34)
#define SW_INT_SELECT_REG2                (SW_VA_INT_IO_BASE + 0x38)

#define SW_INT_ENABLE_REG0                (SW_VA_INT_IO_BASE + 0x40)
#define SW_INT_ENABLE_REG1                (SW_VA_INT_IO_BASE + 0x44)
#define SW_INT_ENABLE_REG2                (SW_VA_INT_IO_BASE + 0x48)

#define SW_INT_MASK_REG0                  (SW_VA_INT_IO_BASE + 0x50)
#define SW_INT_MASK_REG1                  (SW_VA_INT_IO_BASE + 0x54)
#define SW_INT_MASK_REG2                  (SW_VA_INT_IO_BASE + 0x58)

#define SW_INT_RESP_REG0                  (SW_VA_INT_IO_BASE + 0x60)
#define SW_INT_RESP_REG1                  (SW_VA_INT_IO_BASE + 0x64)
#define SW_INT_RESP_REG2                  (SW_VA_INT_IO_BASE + 0x68)

#define SW_INT_FASTFORCE_REG0             (SW_VA_INT_IO_BASE + 0x70)
#define SW_INT_FASTFORCE_REG1             (SW_VA_INT_IO_BASE + 0x74)
#define SW_INT_FASTFORCE_REG2             (SW_VA_INT_IO_BASE + 0x78)

#define SW_INT_SRCPRIO_REG0               (SW_VA_INT_IO_BASE + 0x80)
#define SW_INT_SRCPRIO_REG1               (SW_VA_INT_IO_BASE + 0x84)
#define SW_INT_SRCPRIO_REG2               (SW_VA_INT_IO_BASE + 0x88)
#define SW_INT_SRCPRIO_REG3               (SW_VA_INT_IO_BASE + 0x8c)
#define SW_INT_SRCPRIO_REG4               (SW_VA_INT_IO_BASE + 0x90)


#define SW_UART0_THR                      (*(volatile unsigned int *)(SW_VA_UART0_IO_BASE + 0x00))
#define SW_UART0_LSR                      (*(volatile unsigned int *)(SW_VA_UART0_IO_BASE + 0x14))
#define SW_UART0_USR                      (*(volatile unsigned int *)(SW_VA_UART0_IO_BASE + 0x7c))

#define SW_UART1_THR                      (*(volatile unsigned int *)(SW_VA_UART1_IO_BASE + 0x00))
#define SW_UART1_LSR                      (*(volatile unsigned int *)(SW_VA_UART1_IO_BASE + 0x14))
#define SW_UART1_USR                      (*(volatile unsigned int *)(SW_VA_UART1_IO_BASE + 0x7c))

#define PA_VIC_BASE                       0x01c20400
#define VA_VIC_BASE                       IO_ADDRESS(PA_VIC_BASE)
#define PIO_BASE                          SW_PA_PORTC_IO_BASE

#define SW_G2D_MEM_BASE                   0x58000000
#define SW_G2D_MEM_MAX                    0x1000000

/**
*@name DRAM controller register address
*@{
*/
#define SW_DRAM_SDR_CTL_REG               (SW_VA_DRAM_IO_BASE + 0x0C)
#define SW_DRAM_SDR_DCR                   (SW_VA_DRAM_IO_BASE + 0x04)

/*
 * add from sun7i, liugang, 2012-7-23
 */

/*
 * Memory definitions
 */
#define AW_IO_PHYS_BASE             0x01c00000
#define AW_IO_SIZE                  0x00400000  /* 4MB(Max) */
#define AW_SRAM_A1_BASE             0x00000000
#define AW_SRAM_A1_SIZE             0x00008000
#define AW_SRAM_A2_BASE             0x00040000
#define AW_SRAM_A2_SIZE             0x00008000
#define AW_SRAM_D_BASE              0x00010000
#define AW_SRAM_D_SIZE              0x00001000
#define AW_SRAM_B_BASE              0x00020000 /* Secure, 64KB */
#define AW_SRAM_B_SIZE              0x00010000
#define AW_SDRAM_BASE               0x40000000
#define AW_BROM_BASE                0xffff0000
#define AW_BROM_SIZE                0x00008000 /* 32KB*/

/*
 * Core device addresses
 */
#define AW_MSGBOX_BASE              0x01c17000
#define AW_SPINLOCK_BASE            0x01c18000
#define AW_CCM_BASE                 0x01c20000
#define AW_PIO_BASE                 0x01c20800
#define AW_RPIO_BASE                0x01f02c00 /* for r-pio */
#define AW_GIC_DIST_BASE            0x01c81000
#define AW_GIC_CPU_BASE             0x01c82000
#define AW_TIMER_BASE               0x01c20c00
#define AW_SCU_BASE                 0x01c80000
#define AW_TIMER_G_BASE             0x01c80200 /* CPU global timer, not used */
#define AW_TIMER_P_BASE             0x01c80600 /* CPU private timer, not used */
#define AW_R_CPUCFG_BASE            0x01f01c00
#define AW_DMA_BASE                 0x01c02000

/*
 * Peripheral addresses
 */
#define AW_R_UART_BASE              0x01f02800 /* R_UART */
#define AW_UART0_BASE               0x01c28000 /* UART 0 */
#define AW_UART1_BASE               0x01c28400 /* UART 1 */
#define AW_UART2_BASE               0x01c28800 /* UART 2 */
#define AW_R_PRCM_BASE              0x01f01400 /* for r-prcm */


/*
 * Timer registers
 */
#define AW_TMR_IRQ_EN_REG           0x0000
#define AW_TMR_IRQ_STA_REG          0x0004
#define AW_TMR0_CTRL_REG            0x0010
#define AW_TMR0_INTV_VALUE_REG      0x0014
#define AW_TMR0_CUR_VALUE_REG       0x0018

#define AW_AVS_CNT_CTL_REG          0x0080
#define AW_AVS_CNT0_REG             0x0084
#define AW_AVS_CNT1_REG             0x0088
#define AW_AVS_CNT_DIV_REG          0x008c

/*
 * CPUCFG
 */
#define AW_CPUCFG_P_REG0            0x01a4
#define AW_CPUCFG_P_REG1            0x01a8

/*
 * UART
 */
#define AW_UART_RBR 0x00 /* Receive Buffer Register */
#define AW_UART_THR 0x00 /* Transmit Holding Register */
#define AW_UART_DLL 0x00 /* Divisor Latch Low Register */
#define AW_UART_DLH 0x04 /* Diviso Latch High Register */
#define AW_UART_IER 0x04 /* Interrupt Enable Register */
#define AW_UART_IIR 0x08 /* Interrrupt Identity Register */
#define AW_UART_FCR 0x08 /* FIFO Control Register */
#define AW_UART_LCR 0x0c /* Line Control Register */
#define AW_UART_MCR 0x10 /* Modem Control Register */
#define AW_UART_LSR 0x14 /* Line Status Register */
#define AW_UART_MSR 0x18 /* Modem Status Register */
#define AW_UART_SCH 0x1c /* Scratch Register */
#define AW_UART_USR 0x7c /* Status Register */
#define AW_UART_TFL 0x80 /* Transmit FIFO Level */
#define AW_UART_RFL 0x84 /* RFL */
#define AW_UART_HALT 0xa4 /* Halt TX Register */

#define AW_UART_LOG(fmt, args...) do {} while(0)
#if 0
#define AW_UART_LOG(fmt, args...)                                       \
do {                                                            \
aw_printk((u32)AW_UART0_BASE, "[%s]"fmt"\n", __FUNCTION__, ##args);   \
} while (0)
#endif

#define AW_R_UART_LOG(fmt, args...)                                       \
do {                                                              \
aw_printk((u32)AW_R_UART_BASE, "[%s]"fmt"\n", __FUNCTION__, ##args);   \
} while (0)

#endif


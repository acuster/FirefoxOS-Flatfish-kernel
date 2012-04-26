/*
 * arch/arm/mach-aw163x/include/mach/platform.h
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * Benn Huang (benn@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ASM_ARCH_PLATFORM_H
#define __ASM_ARCH_PLATFORM_H

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
#define AW_GIC_DIST_BASE            0x01c81000
#define AW_GIC_CPU_BASE             0x01c80100
#define AW_TIMER_BASE               0x01c20c00
#define AW_SCU_BASE                 0x01c80000
#define AW_TIMER_G_BASE             0x01c80200 /* CPU global timer, not used */
#define AW_TIMER_P_BASE             0x01c80600 /* CPU private timer, not used */

/*
 * Peripheral addresses
 */
#define AW_R_UART_BASE              0x01f02800 /* R_UART */
#define AW_UART0_BASE               0x01c28000 /* UART 0 */
#define AW_UART1_BASE               0x01c28400 /* UART 1 */
#define AW_UART2_BASE               0x01c28800 /* UART 2 */


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


#endif	/* __ASM_ARCH_PLATFORM_H */

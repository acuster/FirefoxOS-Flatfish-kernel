/*
 * arch/arm/mach-sun6i/include/mach/irqs-sun6i.h
 *
 *  Copyright (C) 2012-2016 Allwinner Limited
 *  Benn Huang (benn@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __MACH_IRQS_AW_H
#define __MACH_IRQS_AW_H

#define AW_IRQ_GIC_START        32

/*
 * AW on-board gic irq sources
 */
#define AW_IRQ_UART0     (AW_IRQ_GIC_START + 0)    /* UART0  */
#define AW_IRQ_UART1     (AW_IRQ_GIC_START + 1)    /* UART1  */
#define AW_IRQ_UART2     (AW_IRQ_GIC_START + 2)    /* UART2  */
#define AW_IRQ_UART3     (AW_IRQ_GIC_START + 3)    /* UART3  */

#define AW_IRQ_TIMER0    (AW_IRQ_GIC_START + 18)  /* Timer0  */
#define AW_IRQ_TIMER1    (AW_IRQ_GIC_START + 19)  /* Timer1  */
#define AW_IRQ_TIMER2    (AW_IRQ_GIC_START + 20)  /* Timer2  */

#define AW_IRQ_ENMI      (AW_IRQ_GIC_START + 32)  /* External NMI  */

#define AW_IRQ_DMA	 38 			  /* dma irq phys num in fpga */

#define AW_IRQ_EINT_PA	 (AW_IRQ_GIC_START + 11)   /* PA_EINIT in spec 43 */
#define AW_IRQ_EINT_PB	 (AW_IRQ_GIC_START + 15)   /* PB_EINIT in spec 47 */
#define AW_IRQ_EINT_PE	 (AW_IRQ_GIC_START + 16)   /* PE_EINIT in spec 48 */
#define AW_IRQ_EINT_PG	 (AW_IRQ_GIC_START + 17)   /* PG_EINIT in spec 49 */

#define AW_IRQ_EINT_R_PL (AW_IRQ_GIC_START + 45)   /* R_PL_EINIT in spec 77 */
#define AW_IRQ_EINT_R_PM (AW_IRQ_GIC_START + 46)   /* R_PM_EINIT in spec 78 */

/*
 * GIC
 */
#define NR_IRQS           (AW_IRQ_GIC_START + 128)
#define MAX_GIC_NR        1


#endif    /* __MACH_IRQS_AW_H */

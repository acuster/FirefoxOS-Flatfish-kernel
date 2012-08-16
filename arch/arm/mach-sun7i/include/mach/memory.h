/*
 *  arch/arm/mach-sun7i/include/mach/memory.h
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
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

#define SUN7I_MEM_ON_FPGA

#define PLAT_PHYS_OFFSET		UL(0x40000000)

/*
 * add by liugang for mali driver, need modify.
 */
#ifdef SUN7I_MEM_ON_FPGA
#define SW_SCRIPT_MEM_BASE		SYS_CONFIG_MEMBASE	/* 0x43000000 		*/
#define SW_SCRIPT_MEM_SIZE		SYS_CONFIG_MEMSIZE	/* 0x00010000(SZ_64K) 	*/

#define SW_FB_MEM_BASE     		0x44000000		/* 0x44000000 		*/
#define SW_FB_MEM_SIZE     		0x02000000		/* SZ_32M 		*/

#define SW_G2D_MEM_BASE     		0x46000000		/* 0x46000000		*/
#define SW_G2D_MEM_SIZE     		0x01000000		/* SZ_16M 		*/

#define SW_CSI_MEM_BASE     		0x47000000		/* 0x47000000		*/
#define SW_CSI_MEM_SIZE     		0x02000000		/* SZ_32M 		*/

#define SW_GPU_MEM_BASE    		0x49000000		/* 0x49000000		*/
#define SW_GPU_MEM_SIZE    		0x04000000		/* SZ_64M		*/

#define SW_VE_MEM_BASE     		0x4d000000		/* 0x4d000000		*/
#define SW_VE_MEM_SIZE     		0x05000000		/* SZ_64M + SZ_16M	*/
#else
#define SW_SCRIPT_MEM_BASE		XXX			/* XXX 			*/
#define SW_SCRIPT_MEM_SIZE		XXX			/* XXX 			*/

#define SW_FB_MEM_BASE     		XXX			/* XXX 			*/
#define SW_FB_MEM_SIZE     		XXX			/* XXX 			*/

#define SW_G2D_MEM_BASE     		XXX			/* XXX 			*/
#define SW_G2D_MEM_SIZE     		XXX			/* XXX 			*/

#define SW_CSI_MEM_BASE     		XXX			/* XXX 			*/
#define SW_CSI_MEM_SIZE     		XXX			/* XXX 			*/

#define SW_GPU_MEM_BASE    		XXX			/* XXX 			*/
#define SW_GPU_MEM_SIZE    		XXX			/* XXX 			*/

#define SW_VE_MEM_BASE     		XXX			/* XXX 			*/
#define SW_VE_MEM_SIZE     		XXX			/* XXX 			*/
#endif /* SUN7I_MEM_ON_FPGA */

#endif

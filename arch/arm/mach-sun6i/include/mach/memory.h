/*
 *  arch/arm/mach-sun6i/include/mach/memory.h
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

#define PLAT_PHYS_OFFSET        UL(0x40000000)
#define PLAT_MEM_SIZE           SZ_2G

#define SYS_CONFIG_MEMBASE      (PLAT_PHYS_OFFSET + SZ_32M + SZ_16M)    /* +48M */
#define SYS_CONFIG_MEMSIZE      (SZ_64K)                                /* 64K */

#define SUPER_STANDBY_MEM_BASE  (PLAT_PHYS_OFFSET + SZ_64M + SZ_32M)    /* +96M */
#define SUPER_STANDBY_MEM_SIZE  (SZ_1K)                                 /* 1K */

#define HW_RESERVED_MEM_BASE    (PLAT_PHYS_OFFSET + SZ_64M + SZ_32M + SZ_4M)    /* +100M */
#define HW_RESERVED_MEM_SIZE    (SZ_128M + SZ_64M + + SZ_32M + SZ_8M)   /* 232M(DE+VE(CSI)+MP) */

#endif

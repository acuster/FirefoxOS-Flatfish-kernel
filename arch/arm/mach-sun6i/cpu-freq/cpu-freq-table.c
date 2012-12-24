/*
 *  arch/arm/mach-sun6i/cpu-freq/cpu-freq-table.c
 *
 * Copyright (c) 2012 Softwinner.
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

#include <linux/types.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include "cpu-freq.h"

struct cpufreq_frequency_table sunxi_freq_tbl[] = {
    { .frequency = 96000,   .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 108000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 120000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 132000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 144000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 156000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 168000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 180000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 192000,  .index = SUNXI_CLK_DIV(1, 1, 1, 2), },
    { .frequency = 240000,  .index = SUNXI_CLK_DIV(1, 1, 2, 2), },
    { .frequency = 288000,  .index = SUNXI_CLK_DIV(1, 1, 2, 2), },
    { .frequency = 312000,  .index = SUNXI_CLK_DIV(1, 1, 2, 2), },
    //{ .frequency = 336000,  .index = SUNXI_CLK_DIV(1, 1, 2, 2), }, frequency is not stable, 2012-12-18 9:26, kevin
    { .frequency = 360000,  .index = SUNXI_CLK_DIV(1, 1, 2, 2), },
    { .frequency = 384000,  .index = SUNXI_CLK_DIV(1, 1, 2, 2), },
    { .frequency = 432000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 480000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 528000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 576000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 624000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 672000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 720000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 768000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 816000,  .index = SUNXI_CLK_DIV(1, 2, 2, 2), },
    { .frequency = 864000,  .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 912000,  .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 960000,  .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 1008000, .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 1056000, .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 1104000, .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 1152000, .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 1200000, .index = SUNXI_CLK_DIV(1, 3, 2, 2), },
    { .frequency = 1248000, .index = SUNXI_CLK_DIV(1, 4, 2, 2), },
    { .frequency = 1296000, .index = SUNXI_CLK_DIV(1, 4, 2, 2), },
    { .frequency = 1344000, .index = SUNXI_CLK_DIV(1, 4, 2, 2), },
    { .frequency = 1392000, .index = SUNXI_CLK_DIV(1, 4, 2, 2), },
    { .frequency = 1440000, .index = SUNXI_CLK_DIV(1, 4, 2, 2), },
    { .frequency = 1488000, .index = SUNXI_CLK_DIV(1, 4, 2, 2), },

    /* table end */
    { .frequency = CPUFREQ_TABLE_END,  .index = 0,              },
};


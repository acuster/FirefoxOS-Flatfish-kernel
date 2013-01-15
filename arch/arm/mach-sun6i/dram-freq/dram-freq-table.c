/*
 * arch/arm/mach-sun6i/dram-freq/dram-freq-table.c
 *
 * Copyright (C) 2012 - 2016 Reuuimlla Limited
 * Pan Nan <pannan@reuuimllatech.com>
 *
 * SUN6I dram frequency table
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include "dram-freq.h"

struct dramfreq_frequency_table sun6i_dramfreq_tbl[] = {
    { .frequency = 312000,   .dram_div =  2 },
    { .frequency = 208000,   .dram_div =  3 },
    { .frequency = 156000,   .dram_div =  4 },
    { .frequency = 104000,   .dram_div =  6 },
    { .frequency =  78000,   .dram_div =  8 },
    { .frequency =  52000,   .dram_div = 12 },
    { .frequency =  48000,   .dram_div = 13 },
    { .frequency =  39000,   .dram_div = 16 },

	/* table end */
	{.frequency =       0,   .dram_div =  0 },
};

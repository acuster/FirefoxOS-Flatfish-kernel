/*
 * arch/arm/XXX/gpio_includes.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * XXX
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __GPIO_INCLUDES_H
#define __GPIO_INCLUDES_H

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <asm-generic/gpio.h>

#include <mach/memory.h>
#include <mach/platform.h>
#include <mach/gpio.h>

#include "gpio_common.h"
#include "gpio_script.h"
#include "gpio_init.h"
#include "gpio_base.h"
#include "gpio_multi_func.h"

#endif  /* __GPIO_INCLUDES_H */

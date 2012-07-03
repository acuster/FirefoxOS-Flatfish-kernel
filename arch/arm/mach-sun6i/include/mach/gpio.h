/*
 * arch/arm/mach-sun6i/include/mach/gpio.h
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * sun6i gpio driver header file
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __SW_GPIO_H
#define __SW_GPIO_H

#include <linux/types.h>
#include <mach/sys_config.h>

/* new api */
u32 sw_gpio_setcfg(u32 gpio, u32 val);
u32 sw_gpio_getcfg(u32 gpio);
u32 sw_gpio_setpull(u32 gpio, u32 val);
u32 sw_gpio_getpull(u32 gpio);
u32 sw_gpio_setdrvlevel(u32 gpio, u32 val);
u32 sw_gpio_getdrvlevel(u32 gpio);
u32 sw_gpio_suspend(void);
u32 sw_gpio_resume(void);

/* old api realize in new api, we recommend use these api instead of a10-old-api(eg: gpio_request_ex) */
u32 sw_gpio_request(user_gpio_set_t *gpio_list, u32 group_count_max);
u32 sw_gpio_request_ex(char *main_name, const char *sub_name);
s32 sw_gpio_release(u32 p_handler, s32 if_release_to_default_status);
s32  sw_gpio_get_all_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, u32 gpio_count_max, u32 if_get_from_hardware);
s32  sw_gpio_get_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, u32 if_get_from_hardware);
s32  sw_gpio_set_one_pin_status(u32 p_handler, user_gpio_set_t *gpio_status, const char *gpio_name, u32 if_set_to_current_input_status);
s32  sw_gpio_set_one_pin_io_status(u32 p_handler, u32 if_set_to_output_status, const char *gpio_name);
s32  sw_gpio_set_one_pin_pull(u32 p_handler, u32 set_pull_status, const char *gpio_name);
s32  sw_gpio_set_one_pin_driver_level(u32 p_handler, u32 set_driver_level, const char *gpio_name);
s32  sw_gpio_read_one_pin_value(u32 p_handler, const char *gpio_name);
s32  sw_gpio_write_one_pin_value(u32 p_handler, u32 value_to_gpio, const char *gpio_name);

#endif /* __SW_GPIO_H */

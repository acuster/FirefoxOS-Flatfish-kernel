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

extern u32 g_pio_vbase, g_rpio_vbase;

/* pio/rpio base */
//#define PIO_VBASE(n) 		(g_pio_vbase + ((n) << 5) + ((n) << 2)) /* pio(PA ~ PF), 0xf1c20800 + n * 0x24 */
//#define RPIO_VBASE(n) 		(g_rpio_vbase + ((n) << 5) + ((n) << 2)) /* r-pio(PL ~ PM), 0xf1f02c00 + n * 0x24 */
#define PIO_VBASE(n) 		(0xf1c20800 + ((n) << 5) + ((n) << 2))
#define RPIO_VBASE(n) 		(0xf1f02c00 + ((n) << 5) + ((n) << 2))

/* port number for each pio */
#define PA_NR			28
#define PB_NR			8
#define PC_NR			27
#define PD_NR			28
#define PE_NR			17
#define PF_NR			6
#define PG_NR			19
#define PH_NR			31
/* for R-PORT PIO */
#define PL_NR			9
#define PM_NR			9 /* spec: "Pin MultiPlex" is 8 while "PM Configure Register 1" is 9 */

/*
 * base index for each pio
 */
#define SUN6I_GPIO_SPACE	5 /* for debugging purposes so that failed if request extra gpio_nr */
#define AW_GPIO_NEXT(gpio)	gpio##_NR_BASE + gpio##_NR + SUN6I_GPIO_SPACE + 1
enum sun6i_gpio_number {
	PA_NR_BASE = 0,
	PB_NR_BASE = AW_GPIO_NEXT(PA),
	PC_NR_BASE = AW_GPIO_NEXT(PB),
	PD_NR_BASE = AW_GPIO_NEXT(PC),
	PE_NR_BASE = AW_GPIO_NEXT(PD),
	PF_NR_BASE = AW_GPIO_NEXT(PE),
	PG_NR_BASE = AW_GPIO_NEXT(PF),
	PH_NR_BASE = AW_GPIO_NEXT(PG),
	/* for R-PORT PIO */
	PL_NR_BASE = AW_GPIO_NEXT(PH), /* NOTE: last is PH */
	PM_NR_BASE = AW_GPIO_NEXT(PL),
};

/* pio index definition */
#define GPIOA(n)		(PA_NR_BASE + (n))
#define GPIOB(n)		(PB_NR_BASE + (n))
#define GPIOC(n)		(PC_NR_BASE + (n))
#define GPIOD(n)		(PD_NR_BASE + (n))
#define GPIOE(n)		(PE_NR_BASE + (n))
#define GPIOF(n)		(PF_NR_BASE + (n))
#define GPIOG(n)		(PG_NR_BASE + (n))
#define GPIOH(n)		(PH_NR_BASE + (n))
#define GPIOL(n)		(PL_NR_BASE + (n))
#define GPIOM(n)		(PM_NR_BASE + (n))

/* pio end, invalid macro */
#define GPIO_INDEX_END		(GPIOM(PM_NR) + 1)
#define GPIO_INDEX_INVALID	(0xFFFFFFFF      )
#define GPIO_CFG_INVALID	(0xFFFFFFFF      )
#define GPIO_PULL_INVALID	(0xFFFFFFFF      )
#define GPIO_DRVLVL_INVALID	(0xFFFFFFFF      )

/* port number for gpiolib */
#ifdef ARCH_NR_GPIOS
#undef ARCH_NR_GPIOS
#endif
#define ARCH_NR_GPIOS		(GPIO_INDEX_END)

/* gpio config info */
struct gpio_config {
	u32	gpio;		/* gpio global index, must be unique */
	u32 	mul_sel;	/* multi sel val: 0 - input, 1 - output... */
	u32 	pull;		/* pull val: 0 - pull up/down disable, 1 - pull up... */
	u32 	drv_level;	/* driver level val: 0 - level 0, 1 - level 1... */
};

/*
 * exported api below
 */

/* new api */
u32 sw_gpio_setcfg(u32 gpio, u32 val);
u32 sw_gpio_getcfg(u32 gpio);
u32 sw_gpio_setpull(u32 gpio, u32 val);
u32 sw_gpio_getpull(u32 gpio);
u32 sw_gpio_setdrvlevel(u32 gpio, u32 val);
u32 sw_gpio_getdrvlevel(u32 gpio);
u32 sw_gpio_set_config(struct gpio_config *pcfg, u32 cfg_num);
u32 sw_gpio_get_config(struct gpio_config *pcfg, u32 cfg_num);
void sw_gpio_dump_config(struct gpio_config *pcfg, u32 cfg_num);
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

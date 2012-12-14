/*
 * rtl8192cu usb wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include "wifi_pm.h"

#define rtl8192cu_msg(...)    do {printk("[rtl8192cu]: "__VA_ARGS__);} while(0)
#define PLATFORM_33_EVB

static int rtl8192cu_powerup = 0;
static int rtl8192cu_power_pin = 0;
static int rtk8192cu_suspend = 0;

// power control by axp
static int rtl8192cu_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;

	rtl8192cu_msg("rtl8192cu module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, "axp22_aldo1");
	if (!wifi_ldo)
		rtl8192cu_msg("get power regulator failed.\n");
	if (first) {
		rtl8192cu_msg("first time\n");
		regulator_force_disable(wifi_ldo);
		first = 0;
	}
	if (onoff) {
		rtl8192cu_msg("regulator on.\n");
		regulator_set_voltage(wifi_ldo, 3300000, 3300000);
		regulator_enable(wifi_ldo);
	} else {
		rtl8192cu_msg("regulator off.\n");
		regulator_disable(wifi_ldo);
	}
	return 0;
}

// power control by gpio
static int rtl8192cu_gpio_ctrl(char* name, int level)
{
	int i = 0, ret = 0, gpio = 0;
	unsigned long flags = 0;

	gpio = rtl8192cu_power_pin;
	
	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		rtl8192cu_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(gpio);
		rtl8192cu_msg("succeed to set gpio %s to %d !\n", name, level);
	}

    rtl8192cu_powerup = level;
	return 0;
}

void rtl8192cu_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
#ifdef PLATFORM_33_EVB
			rtl8192cu_gpio_ctrl("rtl8192cu_power_pin", 1);
#else
			rtl8192cu_module_power(1);
#endif
			udelay(50);
        } else {
#ifdef PLATFORM_33_EVB
			rtl8192cu_gpio_ctrl("rtl8192cu_power_pin", 0);
#else
			rtl8192cu_module_power(0);
#endif
        }
        rtl8192cu_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (rtl8192cu_powerup)
            *updown = 1;
        else
            *updown = 0;
		rtl8192cu_msg("sdio wifi power state: %s\n", rtl8192cu_powerup ? "on" : "off");
    }
    return;	
}

static void rtl8192cu_standby(int instadby)
{
	if (instadby) {
		if (rtl8192cu_powerup) {
#ifdef PLATFORM_33_EVB
			rtl8192cu_gpio_ctrl("rtl8192cu_power_pin", 0);
#else
			rtl8192cu_module_power(0);
#endif
			rtk8192cu_suspend = 1;
		}
	} else {
		if (rtk8192cu_suspend) {
#ifdef PLATFORM_33_EVB
			rtl8192cu_gpio_ctrl("rtl8192cu_power_pin", 1);
#else
			rtl8192cu_module_power(1);
#endif
			rtk8192cu_suspend = 0;
		}
	}
	rtl8192cu_msg("sdio wifi : %s\n", instadby ? "suspend" : "resume");
}

void rtl8192cu_gpio_init(void)
{
	script_item_u val ;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	rtl8192cu_msg("exec rtl8192cu_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "rtl8192cu_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		rtl8192cu_msg("get rtl8192cu rtl8192cu_power gpio failed\n");
	else
		rtl8192cu_power_pin = val.gpio.gpio;

	rtl8192cu_powerup = 0;
	rtk8192cu_suspend = 0;
	ops->gpio_ctrl = rtl8192cu_gpio_init;
	ops->power     = rtl8192cu_power;
	ops->standby   = rtl8192cu_standby;
}

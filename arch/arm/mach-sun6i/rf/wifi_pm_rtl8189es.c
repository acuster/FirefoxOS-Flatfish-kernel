/*
 * rtl8189es sdio wifi power management API
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>

#include "wifi_pm.h"

#define rtl8189es_msg(...)    do {printk("[rtl8189es]: "__VA_ARGS__);} while(0)

static int rtl8189es_powerup = 0;
static int rtl8189es_suspend = 0;
static int rtl8189es_vdd_en = 0;
static int rtl8189es_vcc_en = 0;
static int rtl8189es_shdn = 0;

static int rtl8189es_gpio_ctrl(char* name, int level)
{
	int i = 0, ret = 0, gpio = 0;
	unsigned long flags = 0;
	char* gpio_name[3] = {"rtl8189es_vdd_en", "rtl8189es_vcc_en", "rtl8189es_shdn"};

	for (i=0; i<3; i++) {
		if (strcmp(name, gpio_name[i])==0) {
			    switch (i)
			    {
			        case 0: /* rtl8189es_vdd_en */
						gpio = rtl8189es_vdd_en;
			            break;
			        case 1: /* rtl8189es_vcc_en */
						gpio = rtl8189es_vcc_en;
			            break;
					case 2: /* rtl8189es_shdn */
						gpio = rtl8189es_shdn;
						break;
					default:
            			rtl8189es_msg("no matched gpio!\n");
			    }
			break;
		}
	}

	if (3==i) {
		rtl8189es_msg("No gpio %s for rtl8189es-wifi module\n", name);
		return -1;
	}

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		rtl8189es_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(gpio);
		rtl8189es_msg("succeed to set gpio %s to %d !\n", name, level);
	}

	if (strcmp(name, "rtl8189es_vdd_en") == 0)
		rtl8189es_powerup = level;

	return 0;
}

static void rtl8189es_standby(int instadby)
{
	if (instadby) {
		if (rtl8189es_powerup) {
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 0);
			rtl8189es_gpio_ctrl("rtl8189es_vcc_en", 0);
			rtl8189es_gpio_ctrl("rtl8189es_vdd_en", 0);
			rtl8189es_suspend = 1;
		}
	} else {
		if (rtl8189es_suspend) {
			rtl8189es_gpio_ctrl("rtl8189es_vdd_en", 1);
			udelay(100);
			rtl8189es_gpio_ctrl("rtl8189es_vcc_en", 1);
			udelay(500);
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 1);
			sw_mci_rescan_card(3, 1);
			rtl8189es_suspend = 0;
		}
	}
	rtl8189es_msg("sdio wifi : %s\n", instadby ? "suspend" : "resume");
}

static void rtl8189es_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			rtl8189es_gpio_ctrl("rtl8189es_vdd_en", 1);
			udelay(100);
			rtl8189es_gpio_ctrl("rtl8189es_vcc_en", 1);
			udelay(500);
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 1);
        } else {
			rtl8189es_gpio_ctrl("rtl8189es_shdn", 0);
			rtl8189es_gpio_ctrl("rtl8189es_vcc_en", 0);
			rtl8189es_gpio_ctrl("rtl8189es_vdd_en", 0);
        }
        rtl8189es_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    } else {
        if (rtl8189es_powerup)
            *updown = 1;
        else
            *updown = 0;
		rtl8189es_msg("sdio wifi power state: %s\n", rtl8189es_powerup ? "on" : "off");
    }
    return;
}

void rtl8189es_gpio_init(void)
{
	script_item_u val ;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	rtl8189es_msg("exec rtl8189es_wifi_gpio_init\n");

	type = script_get_item(wifi_para, "rtl8189es_vdd_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		rtl8189es_msg("get rtl8189es rtl8189es_vdd_en gpio failed\n");
	else
		rtl8189es_vdd_en = val.gpio.gpio;

	type = script_get_item(wifi_para, "rtl8189es_vcc_en", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		rtl8189es_msg("get rtl8189es rtl8189es_vcc_en gpio failed\n");
	else
		rtl8189es_vcc_en = val.gpio.gpio;	

	type = script_get_item(wifi_para, "rtl8189es_shdn", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		rtl8189es_msg("get rtl8189es rtl8189es_shdn gpio failed\n");
	else
		rtl8189es_shdn = val.gpio.gpio;

	rtl8189es_powerup = 0;
	rtl8189es_suspend = 0;
	ops->gpio_ctrl 	  = rtl8189es_gpio_ctrl;
	ops->standby 	  = rtl8189es_standby;
	ops->power 		  = rtl8189es_power;
}

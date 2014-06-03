/*
 * ap6xxx sdio wifi power management API
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/regulator/consumer.h>
#include <mach/system.h>
#include "wifi_pm.h"

#define ap6xxx_msg(...)    do {printk("[ap6xxx]: "__VA_ARGS__);} while(0)

//static void tiwl18xx_config_pm_volt(void);
static int tiwl18xx_config_pm_volt(void);//modify to fix the compile warning: ‘return’ with a value, in function returning void
static void ap6xxx_config_32k_clk(void);

//static int ap6xxx_powerup = 0;
//static int ap6xxx_suspend = 0;
static int ap6xxx_wl_regon = 0;
static int ap6xxx_bt_regon = 0;
static char * axp_name = NULL;

// power control by axp
static int ap6xxx_module_power(int onoff)
{
	struct regulator* wifi_ldo = NULL;
	static int first = 1;
	int ret = 0;

	ap6xxx_msg("ap6xxx module power set by axp.\n");
	wifi_ldo = regulator_get(NULL, axp_name);
	if (!wifi_ldo) {
		ap6xxx_msg("get power regulator failed.\n");
		return -ret;
	}

	if (first) {
		ap6xxx_msg("first time\n");
		ret = regulator_force_disable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_force_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
		first = 0; 
	}

	if (onoff) {
		ap6xxx_msg("regulator on.\n");
		ret = regulator_set_voltage(wifi_ldo, 1800000, 1800000);
		if (ret < 0) {
			ap6xxx_msg("regulator_set_voltage fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}

		ret = regulator_enable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_enable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	} else {
		ap6xxx_msg("regulator off.\n");
		ret = regulator_disable(wifi_ldo);
		if (ret < 0) {
			ap6xxx_msg("regulator_disable fail, return %d.\n", ret);
			regulator_put(wifi_ldo);
			return ret;
		}
	}
	regulator_put(wifi_ldo);
	return ret;
}

static int ap6xxx_gpio_ctrl(char* name, int level)
{
	int i = 0;	
	int ret = 0;
	int gpio = 0;
	unsigned long flags = 0;
	char * gpio_name[2] = {"ap6xxx_wl_regon", "ap6xxx_bt_regon"};

	for (i = 0; i < 2; i++) {
		if (strcmp(name, gpio_name[i]) == 0) {
			switch (i)
			{
				case 0: /*ap6xxx_wl_regon*/
					gpio = ap6xxx_wl_regon;
					break;
				case 1: /*ap6xxx_bt_regon*/
					gpio = ap6xxx_bt_regon;
					break;
				default:
					ap6xxx_msg("no matched gpio.\n");
			}
			break;
		}
	}

	if (1==level)
		flags = GPIOF_OUT_INIT_HIGH;
	else
		flags = GPIOF_OUT_INIT_LOW;

	ret = gpio_request_one(gpio, flags, NULL);
	if (ret) {
		ap6xxx_msg("failed to set gpio %s to %d !\n", name, level);
		return -1;
	} else {
		gpio_free(gpio);
		ap6xxx_msg("succeed to set gpio %s to %d !\n", name, level);
	}
    
	return 0;
}

void ap6xxx_power(int mode, int *updown)
{
    if (mode) {
        if (*updown) {
			ap6xxx_gpio_ctrl("ap6xxx_wl_regon", 1);
			mdelay(100);
        } else {
			ap6xxx_gpio_ctrl("ap6xxx_wl_regon", 0);
			mdelay(100);
        }
        ap6xxx_msg("sdio wifi power state: %s\n", *updown ? "on" : "off");
    }
    return;	
}

void ap6xxx_gpio_init(void)
{
	script_item_u val;
	script_item_value_type_e type;
	struct wifi_pm_ops *ops = &wifi_select_pm_ops;

	type = script_get_item(wifi_para, "wifi_power", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		ap6xxx_msg("failed to fetch wifi_power\n");
		return;
	}

	axp_name = val.str;
	ap6xxx_msg("module power name %s\n", axp_name);

	type = script_get_item(wifi_para, "ap6xxx_wl_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		ap6xxx_msg("get ap6xxx ap6xxx_wl_regon gpio failed\n");
	else
		ap6xxx_wl_regon = val.gpio.gpio;

	type = script_get_item(wifi_para, "ap6xxx_bt_regon", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO!=type) 
		ap6xxx_msg("get ap6xxx ap6xxx_bt_regon gpio failed\n");
	else
		ap6xxx_bt_regon = val.gpio.gpio;

	ops->gpio_ctrl	= ap6xxx_gpio_ctrl;
	ops->power = ap6xxx_power;

	ap6xxx_module_power(1);
	
	tiwl18xx_config_pm_volt();
	ap6xxx_config_32k_clk();
}


static char pm_axp_name[] = "axp22_ldoio1";
//static void tiwl18xx_config_pm_volt(void)
static int tiwl18xx_config_pm_volt(void)//modify to fix the compile warning: ‘return’ with a value, in function returning void
{
   struct regulator* pm_ldo = NULL;
   int ret = 0;

   pm_ldo = regulator_get(NULL, pm_axp_name);
   if (!pm_ldo) {
       printk("get power regulator failed.\n");
       return -ret;
   } 

   ret = regulator_set_voltage(pm_ldo, 1800000, 1800000);
   if (ret < 0) {
      printk("regulator_set_voltage fail, return %d.\n", ret);
      return ret;
   }

   ret = regulator_enable(pm_ldo);
   if (ret < 0) {
   printk("regulator_enable fail, return %d.\n", ret);
      return ret;
   }

   printk("ret=%d\n", ret);
   return ret;
} 

static void ap6xxx_config_32k_clk(void)
{
   unsigned int reg_addr, reg_val;
   unsigned int tiwl18xx_32k_gpio;

   tiwl18xx_32k_gpio = GPIOM(7);
   gpio_request(tiwl18xx_32k_gpio, NULL);
   sw_gpio_setpull(tiwl18xx_32k_gpio, 1);
   sw_gpio_setdrvlevel(tiwl18xx_32k_gpio, 3);
   sw_gpio_setcfg(tiwl18xx_32k_gpio, 0x03);

//enable clk
   reg_addr = 0xf1f01400 + 0xf0;
   reg_val = readl(reg_addr);
   writel( reg_val | (1<<31), reg_addr); 
}

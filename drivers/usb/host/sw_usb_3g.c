/*
*************************************************************************************
*                         			      Linux
*					                 USB Host Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_usb_3g.c
*
* Author 		: javen
*
* Description 	: 3G驱动模板是针对MU509设计的
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2012-4-10            1.0          create this file
*
*************************************************************************************
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/signal.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/pm.h>
#include <linux/earlysuspend.h>
#endif

#include <linux/time.h>
#include <linux/timer.h>

#include <mach/sys_config.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>

#include  <mach/clock.h>
#include "sw_hci_sun6i.h"
#include "sw_usb_3g.h"

#define  SW_VA_PORTC_IO_BASE    0x01c20800
#define  SW_INT_IRQNO_PIO       28



//-----------------------------------------------------------------------------
//   debug
//-----------------------------------------------------------------------------
#define  USB_3G_DEBUG

#ifdef  USB_3G_DEBUG
#define  usb_3g_dbg(stuff...)		printk(stuff)
#define  usb_3g_err(...) (usb_3g_dbg("err:L%d(%s):", __LINE__, __FILE__), usb_3g_dbg(__VA_ARGS__))
#else
#define  usb_3g_dbg(...)
#define  usb_3g_err(...)
#endif


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

static struct sw_usb_3g g_usb_3g;


/*
*******************************************************************************
*                     usb_3g_get_config
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static s32 usb_3g_get_config(struct sw_usb_3g *usb_3g)
{
    script_item_value_type_e type = 0;
    script_item_u item_temp;

    /* 3g_used */
	type = script_get_item("3g_para", "3g_used", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g->used = item_temp.val;
	}else{
		usb_3g_err("ERR: get 3g_used failed\n");
        usb_3g->used = 0;
	}

    /* 3g_usbc_num */
	type = script_get_item("3g_para", "3g_usbc_num", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g->usbc_no = item_temp.val;
	}else{
		usb_3g_err("ERR: get 3g_usbc_num failed\n");
        usb_3g->usbc_no = 0;
	}

    /* 3g_usbc_type */
	type = script_get_item("3g_para", "3g_usbc_type", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g->usbc_type = item_temp.val;
	}else{
		usb_3g_err("ERR: get 3g_usbc_type failed\n");
        usb_3g->usbc_type = 0;
	}

    /* 3g_uart_num */
	type = script_get_item("3g_para", "3g_uart_num", &item_temp);
	if(type == SCIRPT_ITEM_VALUE_TYPE_INT){
        usb_3g->uart_no = item_temp.val;
	}else{
		usb_3g_err("ERR: get 3g_uart_num failed\n");
		usb_3g->uart_no = 0;
	}

    /* bb_vbat */
	type = script_get_item("3g_para", "bb_vbat", &usb_3g->bb_vbat);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_vbat_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_vbat failed\n");
	    usb_3g->bb_vbat_valid = 0;
	}

    /* bb_pwr_on */
	type = script_get_item("3g_para", "bb_pwr_on", &usb_3g->bb_pwr_on);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_pwr_on_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_pwr_on failed\n");
	    usb_3g->bb_pwr_on_valid  = 0;
	}

    /* bb_rst */
	type = script_get_item("3g_para", "bb_rst", &usb_3g->bb_rst);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_rst_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_rst failed\n");
	    usb_3g->bb_rst_valid  = 0;
	}

    /* bb_rf_dis */
	type = script_get_item("3g_para", "bb_rf_dis", &usb_3g->bb_rf_dis);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_rf_dis_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_rf_dis failed\n");
	    usb_3g->bb_rf_dis_valid  = 0;
	}

    /* bb_host_wake */
	type = script_get_item("3g_para", "bb_host_wake", &usb_3g->bb_host_wake);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_host_wake_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_host_wake failed\n");
	    usb_3g->bb_host_wake_valid  = 0;
	}

    /* bb_wake */
	type = script_get_item("3g_para", "bb_wake", &usb_3g->bb_wake);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_wake_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_wake failed\n");
	    usb_3g->bb_wake_valid  = 0;
	}

    /* bb_on */
	type = script_get_item("3g_para", "bb_on", &usb_3g->bb_on);
	if(type == SCIRPT_ITEM_VALUE_TYPE_PIO){
	    usb_3g->bb_on_valid = 1;
	}else{
		usb_3g_err("ERR: get bb_on failed\n");
	    usb_3g->bb_on_valid  = 0;
	}

    return 0;
}

/*
*******************************************************************************
*                     usb_3g_pin_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static s32 usb_3g_pin_init(struct sw_usb_3g *usb_3g)
{
    int ret = 0;
    u32 pull = 0;

    //---------------------------------
    //  bb_vbat
    //---------------------------------
    if(usb_3g->bb_vbat_valid){
        ret = gpio_request(usb_3g->bb_vbat.gpio.gpio, "bb_vbat");
        if(ret != 0){
            usb_3g_err("gpio_request bb_vbat failed\n");
            usb_3g->bb_vbat_valid = 0;
        }else{
            /* set config, ouput */
            sw_gpio_setcfg(usb_3g->bb_vbat.gpio.gpio, GPIO_CFG_OUTPUT);

            /* reserved is pull down */
            if(usb_3g->bb_vbat.gpio.data){
                pull = MODULE_GPIO_PULL_UP;
            }else{
                pull = MODULE_GPIO_PULL_DOWN;
            }
            sw_gpio_setpull(usb_3g->bb_vbat.gpio.gpio, pull);
        }
    }

    //---------------------------------
    //  bb_pwr_on
    //---------------------------------
    if(usb_3g->bb_pwr_on_valid){
        ret = gpio_request(usb_3g->bb_pwr_on.gpio.gpio, "bb_pwr_on");
        if(ret != 0){
            usb_3g_err("gpio_request bb_pwr_on failed\n");
            usb_3g->bb_pwr_on_valid = 0;
        }else{
            /* set config, ouput */
            sw_gpio_setcfg(usb_3g->bb_pwr_on.gpio.gpio, GPIO_CFG_OUTPUT);

            /* reserved is pull down */
            if(usb_3g->bb_pwr_on.gpio.data){
                pull = MODULE_GPIO_PULL_UP;
            }else{
                pull = MODULE_GPIO_PULL_DOWN;
            }
            sw_gpio_setpull(usb_3g->bb_pwr_on.gpio.gpio, pull);
        }
    }

    //---------------------------------
    //  bb_rst
    //---------------------------------
    if(usb_3g->bb_rst_valid){
        ret = gpio_request(usb_3g->bb_rst.gpio.gpio, "bb_rst");
        if(ret != 0){
            usb_3g_err("gpio_request bb_rst failed\n");
            usb_3g->bb_rst_valid = 0;
        }else{
            /* set config, ouput */
            sw_gpio_setcfg(usb_3g->bb_rst.gpio.gpio, GPIO_CFG_OUTPUT);

            /* reserved is pull down */
            if(usb_3g->bb_rst.gpio.data){
                pull = MODULE_GPIO_PULL_UP;
            }else{
                pull = MODULE_GPIO_PULL_DOWN;
            }
            sw_gpio_setpull(usb_3g->bb_rst.gpio.gpio, pull);
        }
    }

    //---------------------------------
    //  bb_wake
    //---------------------------------
    if(usb_3g->bb_wake_valid){
        ret = gpio_request(usb_3g->bb_wake.gpio.gpio, "bb_wake");
        if(ret != 0){
            usb_3g_err("gpio_request bb_wake failed\n");
            usb_3g->bb_wake_valid = 0;
        }else{
            /* set config, ouput */
            sw_gpio_setcfg(usb_3g->bb_wake.gpio.gpio, GPIO_CFG_OUTPUT);

            /* reserved is pull down */
            if(usb_3g->bb_wake.gpio.data){
                pull = MODULE_GPIO_PULL_UP;
            }else{
                pull = MODULE_GPIO_PULL_DOWN;
            }
            sw_gpio_setpull(usb_3g->bb_wake.gpio.gpio, pull);
        }
    }

    return 0;
}

/*
*******************************************************************************
*                     usb_3g_pin_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static s32 usb_3g_pin_exit(struct sw_usb_3g *usb_3g)
{
    //---------------------------------
    //  bb_vbat
    //---------------------------------
    if(usb_3g->bb_vbat_valid){
        gpio_free(usb_3g->bb_vbat.gpio.gpio);
        usb_3g->bb_vbat_valid = 0;
    }

    //---------------------------------
    //  bb_pwr_on
    //---------------------------------
    if(usb_3g->bb_pwr_on_valid){
        gpio_free(usb_3g->bb_pwr_on.gpio.gpio);
        usb_3g->bb_pwr_on_valid = 0;
    }

    //---------------------------------
    //  bb_rst
    //---------------------------------
    if(usb_3g->bb_rst_valid){
        gpio_free(usb_3g->bb_rst.gpio.gpio);
        usb_3g->bb_rst_valid = 0;
    }

    //---------------------------------
    //  bb_wake
    //---------------------------------
    if(usb_3g->bb_wake_valid){
        gpio_free(usb_3g->bb_wake.gpio.gpio);
        usb_3g->bb_wake_valid = 0;
    }

    return 0;
}

#ifndef CONFIG_USB_3G_SLEEP_BY_USB_WAKEUP_BY_USB

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_enable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void usb_3g_wakeup_irq_enable(struct sw_usb_3g *usb_3g)
{
    sw_gpio_eint_set_enable(usb_3g->bb_host_wake.gpio.gpio, 1);

    return;
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_disable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void usb_3g_wakeup_irq_disable(struct sw_usb_3g *usb_3g)
{
    sw_gpio_eint_set_enable(usb_3g->bb_host_wake.gpio.gpio, 0);

    return;
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_config
*
* Description:
*    Only used to provide driver mode change events
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int usb_3g_wakeup_irq_config(struct sw_usb_3g *usb_3g)
{
    struct gpio_config_eint_all pcfg;
    u32 cfg_num = 0;

    memset(&pcfg, 0, sizeof(struct gpio_config_eint_all));
    pcfg.gpio = usb_3g->bb_host_wake.gpio.gpio;
    pcfg.pull = 1;
    pcfg.enabled = 0;
    pcfg.irq_pd = 1;
    pcfg.trig_type = TRIG_EDGE_NEGATIVE;

    cfg_num = 1;
    sw_gpio_eint_setall_range(&pcfg, cfg_num);

    return 0;
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_config_clear
*
* Description:
*    Only used to provide driver mode change events
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static int usb_3g_wakeup_irq_config_clear(struct sw_usb_3g *usb_3g)
{
    struct gpio_config_eint_all pcfg;
    u32 cfg_num = 0;

    memset(&pcfg, 0, sizeof(struct gpio_config_eint_all));
    pcfg.gpio = usb_3g->bb_host_wake.gpio.gpio;
    pcfg.pull = 1;
    pcfg.enabled = 0;
    pcfg.irq_pd = 1;
    pcfg.trig_type = TRIG_EDGE_NEGATIVE;

    cfg_num = 1;
    sw_gpio_eint_setall_range(&pcfg, cfg_num);

    return 0;
}

extern void axp_pressshort_ex(void);

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_work
*
* Description:
*    Only used to provide driver mode change events
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void usb_3g_wakeup_irq_work(struct work_struct *data)
{
    usb_3g_dbg("---------usb_3g_wakeup_irq_work----------\n");

    /* 通知android，唤醒系统 */
	//axp_pressshort_ex();

	return;
}

/*
*******************************************************************************
*                     is_usb_3g_wakeup_irq_pending
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static u32 is_usb_3g_wakeup_irq_pending(struct sw_usb_3g *usb_3g)
{
	return sw_gpio_eint_get_irqpd_sta(usb_3g->bb_host_wake.gpio.gpio);
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_clear_pending
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void usb_3g_wakeup_irq_clear_pending(struct sw_usb_3g *usb_3g)
{
    sw_gpio_eint_clr_irqpd_sta(usb_3g->bb_host_wake.gpio.gpio);

    return ;
}

/*
*******************************************************************************
*                     is_usb_3g_wakeup_irq_enable
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static u32 is_usb_3g_wakeup_irq_enable(struct sw_usb_3g *usb_3g)
{
    u32 ret = 0;
    __u32 result = 0;

    ret = sw_gpio_eint_get_enable(usb_3g->bb_host_wake.gpio.gpio, &result);
    if(ret != 0){
        result = 0;
    }

    return result;
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_interrupt
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static u32 usb_3g_wakeup_irq_interrupt(void *para)
{
    __u32 result = 0;
    struct sw_usb_3g *usb_3g = &g_usb_3g;

    if(!is_usb_3g_wakeup_irq_pending(usb_3g)){
        return -1;
    }

	if(is_usb_3g_wakeup_irq_enable(usb_3g)){
	    result = 1;
	}

    usb_3g_wakeup_irq_disable(usb_3g);
    usb_3g_wakeup_irq_config_clear(usb_3g);
    usb_3g_wakeup_irq_clear_pending(usb_3g);

    if(result){
        schedule_work(&usb_3g->irq_work);
    }

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void usb_3g_early_suspend(struct early_suspend *h)
{
    struct sw_usb_3g *usb_3g = &g_usb_3g;

    usb_3g_wakeup_irq_config(usb_3g);
    usb_3g_wakeup_irq_clear_pending(usb_3g);
    usb_3g_wakeup_irq_enable(usb_3g);

    return;
}

static void usb_3g_early_resume(struct early_suspend *h)
{
    struct sw_usb_3g *usb_3g = &g_usb_3g;

    usb_3g_wakeup_irq_disable(usb_3g);
    usb_3g_wakeup_irq_config_clear(usb_3g);
    usb_3g_wakeup_irq_clear_pending(usb_3g);

    return;
}
#endif

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_3g_wakeup_irq_init(void)
{
	struct sw_usb_3g *usb_3g = &g_usb_3g;
//    int ret = 0;

    usb_3g->irq_handle = sw_gpio_irq_request(usb_3g->bb_host_wake.gpio.gpio,
                                            TRIG_EDGE_NEGATIVE,
                                            usb_3g_wakeup_irq_interrupt,
                                            usb_3g);
    if(usb_3g->irq_handle == 0){
        usb_3g_err("request_irq failed\n");
        goto failed;
    }

	/* Init IRQ workqueue before request_irq */
	INIT_WORK(&usb_3g->irq_work, usb_3g_wakeup_irq_work);

    usb_3g_wakeup_irq_config(usb_3g);

#ifdef CONFIG_HAS_EARLYSUSPEND
    usb_3g->early_suspend.suspend = usb_3g_early_suspend;
    usb_3g->early_suspend.resume = usb_3g_early_resume;
	register_early_suspend(&usb_3g->early_suspend);
#endif

    return 0;

failed:
    return -1;
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_3g_wakeup_irq_exit(void)
{
	struct sw_usb_3g *usb_3g = &g_usb_3g;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&usb_3g->early_suspend);
#endif

	usb_3g_wakeup_irq_disable(usb_3g);
    usb_3g_wakeup_irq_config_clear(usb_3g);
    usb_3g_wakeup_irq_clear_pending(usb_3g);

    //sw_gpio_irq_free(usb_3g->irq_handle);

    return 0;
}
#else
int usb_3g_wakeup_irq_init(void)
{
    return 0;
}

/*
*******************************************************************************
*                     usb_3g_wakeup_irq_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_3g_wakeup_irq_exit(void)
{
    return 0;
}

#endif

EXPORT_SYMBOL(usb_3g_wakeup_irq_init);
EXPORT_SYMBOL(usb_3g_wakeup_irq_exit);

/*
*******************************************************************************
*                     usb_3g_vbat
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*
*******************************************************************************
*/
void usb_3g_vbat(struct sw_usb_3g *usb_3g, u32 on)
{
    if(!usb_3g->bb_vbat_valid){
        return;
    }

    if(on){
        usb_3g_dbg("Set BaseBand vBat on\n");
    }else{
        usb_3g_dbg("Set BaseBand vBat off\n");
    }

    __gpio_set_value(usb_3g->bb_vbat.gpio.gpio, on);

    return;
}
EXPORT_SYMBOL(usb_3g_vbat);

/*
*******************************************************************************
*                     usb_3g_reset
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void usb_3g_reset(u32 usbc_no, u32 time)
{
    struct sw_usb_3g *usb_3g = &g_usb_3g;

    if(!usb_3g->bb_rst_valid){
        return;
    }

    __gpio_set_value(usb_3g->bb_rst.gpio.gpio, 1);
    mdelay(time);
    __gpio_set_value(usb_3g->bb_rst.gpio.gpio, 0);

    return;
}
EXPORT_SYMBOL(usb_3g_reset);

/*
*******************************************************************************
*                     usb_3g_wakeup_sleep
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
#ifdef CONFIG_USB_3G_SLEEP_BY_GPIO_WAKEUP_BY_GPIO
void usb_3g_wakeup_sleep(u32 usbc_no, u32 sleep)
{
    struct sw_usb_3g *usb_3g = &g_usb_3g;

    if(!usb_3g->bb_wake_valid){
        return;
    }

    if(sleep){
        usb_3g_dbg("sleep usb_3g\n");
    }else{
        usb_3g_dbg("wakeup usb_3g\n");
    }

    __gpio_set_value(usb_3g->bb_wake.gpio.gpio, sleep);

    return;
}
#else
void usb_3g_wakeup_sleep(u32 usbc_no, u32 sleep)
{
    return;
}
#endif
EXPORT_SYMBOL(usb_3g_wakeup_sleep);

/*
*******************************************************************************
*                     usb_3g_power_on_off
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void usb_3g_power_on_off(struct sw_usb_3g *usb_3g, u32 is_on)
{
    u32 on_off = 0;

    if(!usb_3g->bb_pwr_on_valid){
        return;
    }

    usb_3g_dbg("Set usb_3g Power %s\n", (is_on ? "ON" : "OFF"));

    /* set power */
    if(usb_3g->bb_pwr_on.gpio.data == 0){
        on_off = is_on ? 1 : 0;
    }else{
        on_off = is_on ? 0 : 1;
    }

    __gpio_set_value(usb_3g->bb_pwr_on.gpio.gpio, on_off);

    return;
}

/*
*******************************************************************************
*                     usb_3g_power
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*
* 1、vbat通过GPIO操作时，vbat关掉后，整个3G模组都掉电了，因此 power_on_off 可以省去
*    power on:
*		1、vbat pin pull up
*		2、delay 4000ms
*		3、power_on_off pin pull down
*		4、delay 700ms
*		5、power_on_off pin pull up
*
*	  power off:
*		1、vbat pin pull down
*
*
*******************************************************************************
*/
void usb_3g_power(u32 usbc_no, u32 on)
{
    struct sw_usb_3g *usb_3g = &g_usb_3g;

    usb_3g_dbg("usb_3g power %s with gpio\n", (on ? "ON" : "OFF"));

    if(on){
		usb_3g_vbat(usb_3g, 1);
		mdelay(4000);
        usb_3g_power_on_off(usb_3g, 0);
        mdelay(700);
        usb_3g_power_on_off(usb_3g, 1);
    }else{
		usb_3g_vbat(usb_3g, 0);
    }

    return;
}
EXPORT_SYMBOL(usb_3g_power);

/*
*******************************************************************************
*                     is_suspport_usb_3g
*
* Description:
*    void
*
* Parameters:
*    usbc_no   : input. 控制器编号, usb0,usb1,usb2
*    usbc_type : input. 控制器类型, 0: unkown, 1: ehci, 2: ohci
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
u32 is_suspport_usb_3g(u32 usbc_no, u32 usbc_type)
{
    if(!g_usb_3g.used){
        return 0;
    }

    if(g_usb_3g.usbc_no != usbc_no){
        return 0;
    }

    if(g_usb_3g.usbc_type != usbc_type){
        return 0;
    }

    return 1;
}
EXPORT_SYMBOL(is_suspport_usb_3g);

/*
*******************************************************************************
*                     usb_3g_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_3g_init(void)
{
    int ret = 0;

    memset(&g_usb_3g, 0, sizeof(struct sw_usb_3g));
	spin_lock_init(&g_usb_3g.lock);

    ret = usb_3g_get_config(&g_usb_3g);
    if(ret != 0){
        usb_3g_err("err: usb_3g_get_config failed\n");
        goto failed0;
    }

    ret =  usb_3g_pin_init(&g_usb_3g);
    if(ret != 0){
       usb_3g_err("err: usb_3g_pin_init failed\n");
       goto failed1;
    }

    return 0;

failed1:
failed0:

    return -1;
}
EXPORT_SYMBOL(usb_3g_init);

/*
*******************************************************************************
*                     usb_3g_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int usb_3g_exit(void)
{
    usb_3g_pin_exit(&g_usb_3g);

    return 0;
}
EXPORT_SYMBOL(usb_3g_exit);





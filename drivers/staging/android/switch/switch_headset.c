
/*
 * drivers\staging\android\switch\switch_headset.c
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * huangxin <huangxin@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "switch.h"
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/irqs-sun6i.h>

#undef SWITCH_DBG
#if (1)
    #define SWITCH_DBG(format,args...)  printk("[SWITCH] "format,##args)    
#else
    #define SWITCH_DBG(...)    
#endif

#define VIR_CODEC_BASSADDRESS      (0xf1c22c00)
#define SUN6I_PA_CTRL			   (0x24)		//new func
#define SUN6I_MIC_CTRL			   (0x28)
#define SUN6I_HMIC_CTL 	           (0x50)		//new func
#define SUN6I_HMIC_DATA	           (0x54)		//new func

/*0x24*/
#define HPCOM_CTL				  (29)
#define LTRNMUTE				  (25)
#define RTLNMUTE				  (24)

/*0x28 */
#define HBIASEN					  (31)
#define HBIASADCEN				  (29)
#define MIC2AMPEN				  (24)
#define LINEOUTL_EN			  	  (19)
#define LINEOUTR_EN			  	  (18)

/*HMIC Control Register
*codecbase+0x50
*/
#define HMIC_M					  (28)
#define HMIC_N					  (24)
#define HMIC_DIRQ				  (23)
#define HMIC_TH1_HYS			  (21)
#define	HMIC_EARPHONE_OUT_IRQ_EN  (20)
#define HMIC_EARPHONE_IN_IRQ_EN	  (19)
#define HMIC_KEY_UP_IRQ_EN		  (18)
#define HMIC_KEY_DOWN_IRQ_EN	  (17)
#define HMIC_DATA_IRQ_EN		  (16)
#define HMIC_DS_SAMP			  (14)
#define HMIC_TH2_HYS			  (13)
#define HMIC_TH2_KEY		      (8)
#define HMIC_SF_SMOOTH_FIL		  (6)
#define KEY_UP_IRQ_PEND			  (5)
#define HMIC_TH1_EARPHONE		  (0)

/*HMIC Data Register
* codecbase+0x54
*/
#define HMIC_EARPHONE_OUT_IRQ_PEND  (20)
#define HMIC_EARPHONE_IN_IRQ_PEND   (19)
#define HMIC_KEY_UP_IRQ_PEND 	    (18)
#define HMIC_KEY_DOWN_IRQ_PEND 		(17)
#define HMIC_DATA_IRQ_PEND			(16)
#define HMIC_ADC_DATA				(0)

#define FUNCTION_NAME "h2w"
#define TIMER_CIRCLE 50

#define hmic_rdreg(reg)	    readl((hmic_base+(reg)))
#define hmic_wrreg(reg,val)  writel((val),(hmic_base+(reg)))

static int gpio_earphone_switch = 0;
static void __iomem *hmic_base;

struct gpio_switch_data {
	struct switch_dev sdev;
	int pio_hdle;	
	int state;
	int pre_state;

	struct work_struct work;
	struct timer_list timer;
};

/**
* codec_wrreg_bits - update codec register bits
* @reg: codec register
* @mask: register mask
* @value: new value
*
* Writes new register value.
* Return 1 for change else 0.
*/
int hmic_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	unsigned int old, new;
		
	old	=	hmic_rdreg(reg);
	new	=	(old & ~mask) | value;

	hmic_wrreg(reg,new);

	return 0;
}

int hmic_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	hmic_wrreg_bits(reg, mask, reg_val);
	return 0;
}

static void earphone_switch_work(struct work_struct *work)
{
	struct gpio_switch_data	*data =
		container_of(work, struct gpio_switch_data, work);

	SWITCH_DBG("%s,line:%d, data->state:%d\n", __func__, __LINE__, data->state);
	switch_set_state(&data->sdev, data->state);
}

static irqreturn_t audio_hmic_irq(int irq, void *dev_id)
{
	int tmp = 0;
	struct gpio_switch_data *switch_data = (struct gpio_switch_data *)dev_id;
	if (switch_data == NULL) {
		return IRQ_NONE;
	}
	SWITCH_DBG("%s,line:%d\n", __func__, __LINE__);
	tmp = hmic_rdreg(SUN6I_HMIC_DATA);
	/*bit19 is 1 means the earphone plug in*/
	tmp &= (0x1<<19);
	if (tmp) {
		/* if the 17 bit assert 1, it means the three sections earphone has plun in
		 * if the 17 bit assert 0, it means the four sections earphone has plun in
		 */
		tmp = hmic_rdreg(SUN6I_HMIC_DATA);
		tmp &=(0x1f<<0);
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, tmp);
		if (tmp >= 0xb) {/*0xc is from hardware debug, means the three section earphone*/
			SWITCH_DBG("headphone three HP,HMIC_DAT= %d\n",(tmp&0x1f));
			switch_data->state = 2;
			/*clean the pending bit*/
			hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_KEY_DOWN_IRQ_PEND, 0x1);
			hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_EARPHONE_IN_IRQ_PEND, 0x1);
			hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_KEY_UP_IRQ_PEND, 0x1);
			hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_KEY_DOWN_IRQ_PEND, 0x1);
			hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_DATA_IRQ_PEND, 0x1);
		} else {/*the earphone is four section earphone*/
			SWITCH_DBG("headphone four HP,HMIC_DAT= %d\n",(tmp&0x1f));
			switch_data->state = 1;
			hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_EARPHONE_IN_IRQ_PEND, 0x1);

			tmp = hmic_rdreg(SUN6I_HMIC_DATA);
			tmp &= (0x1f<<0);
			if (tmp <= 1) {/*debug from hardware(hookkey press)*/
				SWITCH_DBG("headphone four HP,hookkey press\n");
				switch_data->state = 3;
			}
		}
	}

	tmp = hmic_rdreg(SUN6I_HMIC_DATA);
	if (tmp & (0x1<<20)) {
		SWITCH_DBG("%s,line:%d,tmp:%x\n", __func__, __LINE__, (tmp&0x1f));
		/*if the irq is hmic earphone pull out, when the irq coming, clean the pending bit*/
		hmic_wr_control(SUN6I_HMIC_DATA, 0x1, HMIC_EARPHONE_OUT_IRQ_PEND, 0x1);
		switch_data->state = 0;
	}

	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	
	return sprintf(buf, "%d\n", switch_data->state);
}

static ssize_t print_headset_name(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);

	return sprintf(buf, "%s\n", switch_data->sdev.name);
}

static int gpio_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0;
	
	if (!pdata) {
		return -EBUSY;
	}

	hmic_base = (void __iomem *)VIR_CODEC_BASSADDRESS;
	hmic_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASEN, 0x1);
	hmic_wr_control(SUN6I_MIC_CTRL, 0x1, HBIASADCEN, 0x1);
	hmic_wr_control(SUN6I_HMIC_CTL, 0xf, HMIC_M, 0xf);/*0xf should be get from hw_debug 28*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0xf, HMIC_N, 0xf);/*0xf should be get from hw_debug 24*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_EARPHONE_OUT_IRQ_EN, 0x1); /*20*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_EARPHONE_IN_IRQ_EN, 0x1); /*19*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x3, HMIC_DS_SAMP, 0x3); /*14*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1f, HMIC_TH2_KEY, 0x8);/*0xf should be get from hw_debug 8*/
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1f, HMIC_TH1_EARPHONE, 0x1);/*0x1 should be get from hw_debug 0*/

/*
	for key debug	
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_DATA_IRQ_EN, 0x1);
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_KEY_DOWN_IRQ_EN, 0x1);
	hmic_wr_control(SUN6I_HMIC_CTL, 0x1, HMIC_KEY_UP_IRQ_EN, 0x1);
*/

	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data) {
		printk("%s,line:%d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	switch_data->sdev.state = 0;
	switch_data->pre_state = -1;
	switch_data->sdev.name = pdata->name;
	switch_data->pio_hdle = gpio_earphone_switch;
	switch_data->sdev.print_name = print_headset_name;
	switch_data->sdev.print_state = switch_gpio_print_state;
	INIT_WORK(&switch_data->work, earphone_switch_work);

    ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0) {
		goto err_switch_dev_register;
	}

	ret = request_irq(AW_IRQ_CODEC, audio_hmic_irq, 0, "audio_hmic_irq", switch_data);
    if (ret < 0) {
        printk("request irq err\n");
        return -EINVAL;
    }
	return 0;

err_switch_dev_register:
		kfree(switch_data);

	return ret;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);

    switch_dev_unregister(&switch_data->sdev);    
	kfree(switch_data);	
	return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "switch-gpio",
		.owner	= THIS_MODULE,
	},
};

static struct gpio_switch_platform_data headset_switch_data = { 
    .name = "h2w",
};

static struct platform_device gpio_switch_device = { 
    .name = "switch-gpio",
    .dev = { 
    	.platform_data = &headset_switch_data,
    }   
};

static int __init gpio_switch_init(void)
{
	int ret = 0;
    
	ret = platform_device_register(&gpio_switch_device);
	if (ret == 0) {
		ret = platform_driver_register(&gpio_switch_driver);
	}

	return ret;
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
	platform_device_unregister(&gpio_switch_device);

}
module_init(gpio_switch_init);
module_exit(gpio_switch_exit);

MODULE_AUTHOR("huanxin<huanxin@allwinnertech.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");

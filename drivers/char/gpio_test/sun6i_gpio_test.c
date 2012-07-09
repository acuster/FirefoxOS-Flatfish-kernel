/*
 * drivers/char/gpio_test/sun6i_gpio_test.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sun6i gpio test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "sun6i_gpio_test.h"

#define TEST_REQUEST_FREE	/* test for gpio_request/gpio_free */
#define TEST_RE_REQUEST_FREE	/* test for re-gpio_request/re-gpio_free, so get warning */
#define TEST_GPIOLIB_API	/* test the standard linux gpio api */
#define TEST_CONFIG_API		/* test gpio multi-function */
#define TEST_GPIO_EINT_API	/* test gpio external interrupt */
//#define TEST_GPIO_SCRIPT_API	/* test gpio script api */

/*
 * cur test case
 */
static enum gpio_test_case_e g_cur_test_case = GTC_API;

/**
 * gpio_chip_match - match function to check if gpio is in the gpio_chip
 * @chip:	gpio_chip to match
 * @data:	data to match
 *
 * Returns 1 if match, 0 if not match
 */
static inline int gpio_chip_match(struct gpio_chip * chip, void * data)
{
	u32 	num = 0;

	num = *(u32 *)data;
	if(num >= chip->base && num < chip->base + chip->ngpio) {
		return 1;
	}

	return 0;
}

/**
 * __gtc_api - gpio test case, for api
 *
 * Returns 0 if sucess, the err line number if failed.
 */
u32 __gtc_api(void)
{
	u32 	uret = 0;
	u32	utemp = 0;
	u32	upio_index = 0;
	u32	offset = 0;
	struct gpio_chip *pchip = NULL;
	struct gpio gpio_arry[] = {
		{GPIOA(0), GPIOF_OUT_INIT_HIGH, "pa0"},
		{GPIOB(3), GPIOF_IN, "pb3"},
		{GPIOC(5), GPIOF_OUT_INIT_LOW, "pc5"},
		{GPIOH(2), GPIOF_IN, "ph2"},
	};
	struct gpio_config gpio_cfg_temp[4] = {
		{GPIOA(0)},
		{GPIOB(3)},
		{GPIOC(5)},
		{GPIOH(2)},
	};
	struct gpio_config gpio_cfg[] = {
		/* use default if you donot care the pull or driver level status */
		{GPIOE(10), 3, GPIO_PULL_DEFAULT, GPIO_DRVLVL_DEFAULT},
		{GPIOA(13), 2, 1, 2},
		{GPIOD(2),  1, 2, 1},
		{GPIOG(8),  0, 1, 1},
	};

	/*
	 * test for request and free
	 */
#ifdef TEST_REQUEST_FREE
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == gpio_request(GPIOA(0), "pa0"), uret, End);
	PIO_CHECK_RST(0 == gpio_request(GPIOA(PA_NR / 2), "pa_mid"), uret, End);
	PIO_CHECK_RST(0 == gpio_request(GPIOA(PA_NR - 1), "pa_end_sub_1"), uret, End);
#if 0	/* request un-exist gpio, err */
	PIO_CHECK_RST(0 == gpio_request(GPIOA(PA_NR), "pa_end"), uret, End); /* err */
	PIO_CHECK_RST(0 == gpio_request(GPIOA(PA_NR + 1), "pa_end_plus_1"), uret, End); /* err */
#endif
	gpio_free(GPIOA(0));
	gpio_free(GPIOA(PA_NR / 2));
	gpio_free(GPIOA(PA_NR - 1));

	PIO_CHECK_RST(0 == gpio_request(GPIOB(0), "pb0"), uret, End);
	PIO_CHECK_RST(0 == gpio_request(GPIOB(PB_NR / 2), "pb_mid"), uret, End);
	PIO_CHECK_RST(0 == gpio_request(GPIOB(PB_NR - 1), "pb_end_sub_1"), uret, End);

	gpio_free(GPIOB(0));
	gpio_free(GPIOB(PB_NR / 2));
	gpio_free(GPIOB(PB_NR - 1));
#endif /* TEST_REQUEST_FREE */

	/*
	 * test for re-request and re-free the same pio
	 */
#ifdef TEST_RE_REQUEST_FREE
	PIO_DBG_FUN_LINE;
#if 0	/* free an un-requested pio, err */
	gpio_free(GPIOA(PA_NR / 2));
	PIO_DBG_FUN_LINE;
#endif

	PIO_CHECK_RST(0 == gpio_request(GPIOA(PA_NR / 2), "pa_mid"), uret, End);
	PIO_DBG_FUN_LINE;
	gpio_free(GPIOA(PA_NR / 2));
	PIO_DBG_FUN_LINE;
#if 0	/* re-free, err */
	gpio_free(GPIOA(PA_NR / 2));
	PIO_DBG_FUN_LINE;
#endif

	PIO_CHECK_RST(0 == gpio_request(GPIOB(PB_NR / 2), "pb_mid"), uret, End);
	PIO_DBG_FUN_LINE;
#if 0	/* re-request, err */
	PIO_CHECK_RST(0 == gpio_request(GPIOB(PB_NR / 2), "pb_mid"), uret, End);
	PIO_DBG_FUN_LINE;
#endif
	gpio_free(GPIOB(PB_NR / 2));
	PIO_DBG_FUN_LINE;
#endif /* TEST_RE_REQUEST_FREE */

	/*
	 * test for gpiolib api
	 */
#ifdef TEST_GPIOLIB_API
	PIO_DBG_FUN_LINE;
	/* test gpio_request_one */
	PIO_CHECK_RST(0 == gpio_request_one(GPIOC(1), GPIOF_OUT_INIT_HIGH, "pc_1"), uret, End);
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(1 == __gpio_get_value(GPIOC(1)), uret, End); /* check if data ok */
	gpio_free(GPIOC(1));

	PIO_CHECK_RST(0 == gpio_request_one(GPIOC(1), GPIOF_OUT_INIT_LOW, "pc_1"), uret, End);
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == __gpio_get_value(GPIOC(1)), uret, End); /* check if data ok */
	gpio_free(GPIOC(1));

	/* test gpio_request_array */
	PIO_CHECK_RST(0 == gpio_request_array(gpio_arry, ARRAY_SIZE(gpio_arry)), uret, End);
	PIO_DBG_FUN_LINE;
#if 1	/* check if request array success */
	PIO_CHECK_RST(0 == sw_gpio_getall_range(gpio_cfg_temp, ARRAY_SIZE(gpio_cfg_temp)), uret, End);
	PIO_DBG_FUN_LINE;
	sw_gpio_dump_config(gpio_cfg_temp, ARRAY_SIZE(gpio_cfg_temp));
	PIO_DBG_FUN_LINE;
#endif

	/* test gpio_free_array */
	gpio_free_array(gpio_arry, ARRAY_SIZE(gpio_arry));
	PIO_DBG_FUN_LINE;

	/* test gpiochip_find */
	offset = 5;
	utemp = GPIOB(offset);
	PIO_CHECK_RST(0 == gpio_request(utemp, "pb5"), uret, End);
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(NULL != (pchip = gpiochip_find(&utemp, gpio_chip_match)), uret, End);
	PIO_DBG_FUN_LINE;
	printk("%s: gpiochip_find success, 0x%08x\n", __FUNCTION__, (u32)pchip);

	/* test gpiochip_is_requested */
	PIO_CHECK_RST(NULL != gpiochip_is_requested(pchip, offset), uret, End);
	PIO_DBG_FUN_LINE;
	gpio_free(utemp);
	PIO_DBG_FUN_LINE;

	/* test gpio_direction_input/__gpio_get_value/gpio_get_value_cansleep */
	upio_index = GPIOE(16);
	PIO_CHECK_RST(0 == gpio_direction_input(upio_index), uret, End); /* warn autorequest */
	PIO_DBG_FUN_LINE;
	utemp = __gpio_get_value(upio_index); /* __gpio_get_value */
	printk("%s: __gpio_get_value pe16 value %d\n", __FUNCTION__, utemp);
	utemp = (u32)gpio_get_value_cansleep(upio_index); /* gpio_get_value_cansleep, success even can_sleep flag not set */
	printk("%s: gpio_get_value_cansleep pe16 value %d\n", __FUNCTION__, utemp);
	gpio_free(upio_index);

	/* test gpio_direction_output/__gpio_get_value/__gpio_set_value/gpio_set_value_cansleep */
	upio_index = GPIOB(5);
	PIO_CHECK_RST(0 == gpio_request(upio_index, "pb5"), uret, End);

	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == gpio_direction_output(upio_index, 1), uret, End); /* gpio_direction_output */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(1 == __gpio_get_value(upio_index), uret, End); /* __gpio_get_value */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == gpio_direction_output(upio_index, 0), uret, End);
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == __gpio_get_value(upio_index), uret, End);
	PIO_DBG_FUN_LINE;
	__gpio_set_value(upio_index, 1); /* __gpio_set_value */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(1 == __gpio_get_value(upio_index), uret, End);
	PIO_DBG_FUN_LINE;
	gpio_set_value_cansleep(upio_index, 0); /* gpio_set_value_cansleep */
	PIO_DBG_FUN_LINE;
	/* test gpio_set_debounce, gpio_chip->set_debounce not impletment, so err here */
	utemp = gpio_set_debounce(upio_index, 10);
	printk("%s: gpio_set_debounce %d return %d\n", __FUNCTION__, upio_index, utemp);
	PIO_DBG_FUN_LINE;
	/* test __gpio_cansleep */
	utemp = (u32)__gpio_cansleep(upio_index);
	printk("%s: __gpio_cansleep %d return %d\n", __FUNCTION__, upio_index, utemp);
	/* test __gpio_to_irq */
	utemp = (u32)__gpio_to_irq(upio_index);
	printk("%s: __gpio_to_irq %d return %d\n", __FUNCTION__, upio_index, utemp);

	gpio_free(upio_index);
	PIO_DBG_FUN_LINE;
#endif /* TEST_GPIOLIB_API */

	/*
	 * test for cfg api
	 */
#ifdef TEST_CONFIG_API
	upio_index = GPIOA(1);
	PIO_CHECK_RST(0 == gpio_request(upio_index, "pa1"), uret, End);
	PIO_DBG_FUN_LINE;

	PIO_CHECK_RST(0 == gpio_direction_output(upio_index, 0), uret, End);
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(1 == sw_gpio_getcfg(upio_index), uret, End); /* sw_gpio_getcfg output */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == gpio_direction_input(upio_index), uret, End);
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(0 == sw_gpio_getcfg(upio_index), uret, End); /* sw_gpio_getcfg input */
	PIO_DBG_FUN_LINE;

	PIO_CHECK_RST(0 == sw_gpio_setcfg(upio_index, 3), uret, End); /* sw_gpio_setcfg(PA1->LCD1_D1) */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(3 == sw_gpio_getcfg(upio_index), uret, End); /* sw_gpio_getcfg */
	PIO_DBG_FUN_LINE;

	PIO_CHECK_RST(0 == sw_gpio_setpull(upio_index, 2), uret, End); /* sw_gpio_setpull down */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(2 == sw_gpio_getpull(upio_index), uret, End); /* sw_gpio_getpull */
	PIO_DBG_FUN_LINE;

	PIO_CHECK_RST(0 == sw_gpio_setdrvlevel(upio_index, 2), uret, End); /* sw_gpio_setdrvlevel level2 */
	PIO_DBG_FUN_LINE;
	PIO_CHECK_RST(2 == sw_gpio_getdrvlevel(upio_index), uret, End); /* sw_gpio_getdrvlevel */
	PIO_DBG_FUN_LINE;

	gpio_free(upio_index);
	PIO_DBG_FUN_LINE;

	PIO_CHECK_RST(0 == sw_gpio_setall_range(gpio_cfg, ARRAY_SIZE(gpio_cfg)), uret, End); /* sw_gpio_setall_range */
	PIO_DBG_FUN_LINE;

	PIO_CHECK_RST(0 == sw_gpio_getall_range(gpio_cfg, ARRAY_SIZE(gpio_cfg)), uret, End); /* sw_gpio_getall_range */
	PIO_DBG_FUN_LINE;

	sw_gpio_dump_config(gpio_cfg, ARRAY_SIZE(gpio_cfg)); /* sw_gpio_dump_config */
#endif /* TEST_CONFIG_API */

	/*
	 * test for gpio external interrupt api
	 */
#ifdef TEST_GPIO_EINT_API
	{
		struct gpio_config_eint_all cfg_eint[] = {
			{GPIOA(0) , GPIO_PULL_DEFAULT, GPIO_DRVLVL_DEFAULT, true , 0, TRIG_EDGE_POSITIVE},
			{GPIOA(26), 1                , 2                  , true , 0, TRIG_EDGE_DOUBLE  },
			{GPIOB(3) , GPIO_PULL_DEFAULT, 1                  , false, 0, TRIG_EDGE_NEGATIVE},
			{GPIOE(14), 3                , 0                  , true , 0, TRIG_LEVL_LOW     },

			/* __para_check err in sw_gpio_eint_setall_range, because rpl3 cannot be eint */
			//{GPIOL(3) , 1                , GPIO_DRVLVL_DEFAULT, false, 0, TRIG_LEVL_HIGH  },

			{GPIOL(8) , 2                , GPIO_DRVLVL_DEFAULT, false, 0, TRIG_LEVL_HIGH    },
			{GPIOM(4) , 1                , 2                  , true,  0, TRIG_LEVL_LOW     },
		};
		struct gpio_config_eint_all cfg_eint_temp[] = {
			{GPIOA(0) },
			{GPIOA(26)},
			{GPIOB(3) },
			{GPIOE(14)},
			{GPIOL(8) },
			{GPIOM(4) },
		};
		for(utemp = 0; utemp < ARRAY_SIZE(cfg_eint); utemp++)
			PIO_CHECK_RST(0 == gpio_request(cfg_eint[utemp].gpio, NULL), uret, End);
		PIO_DBG_FUN_LINE;
		sw_gpio_eint_dumpall_range(cfg_eint, ARRAY_SIZE(cfg_eint)); /* dump the orginal struct */
		PIO_DBG_FUN_LINE;
		PIO_CHECK_RST(0 == sw_gpio_eint_setall_range(cfg_eint, ARRAY_SIZE(cfg_eint)), uret, End); /* sw_gpio_eint_setall_range */
		PIO_DBG_FUN_LINE;
		PIO_CHECK_RST(0 == sw_gpio_eint_getall_range(cfg_eint_temp, ARRAY_SIZE(cfg_eint_temp)), uret, End); /* sw_gpio_eint_getall_range */
		PIO_DBG_FUN_LINE;
		sw_gpio_eint_dumpall_range(cfg_eint_temp, ARRAY_SIZE(cfg_eint_temp)); /* dump the struct get from hw */
		PIO_DBG_FUN_LINE;
		for(utemp = 0; utemp < ARRAY_SIZE(cfg_eint); utemp++)
			gpio_free(cfg_eint[utemp].gpio);
		PIO_DBG_FUN_LINE;
	}
#endif /* TEST_GPIO_EINT_API */

	/*
	 * test for script gpio api
	 */
#ifdef TEST_GPIO_SCRIPT_API
	{
		u32 	pin_hdl = 0;
		struct gpio_config cfg_card_boot0[] = {
			{GPIOF(0)},
			{GPIOF(1)},
			{GPIOF(2)},
			{GPIOF(3)},
			{GPIOF(4)},
			{GPIOF(5)},
		};
		struct gpio_config cfg_card_boot2[] = {
			{GPIOC(6)},
			{GPIOC(7)},
			{GPIOC(8)},
			{GPIOC(9)},
			{GPIOC(10)},
			{GPIOC(11)},
		};

		/*
		[card_boot0_para]
		card_ctrl 		= 0
		card_high_speed 	= 1
		card_line       	= 4
		sdc_d1      		= port:PF0<2><1><default><default>
		sdc_d0      		= port:PF1<2><1><default><default>
		sdc_clk     		= port:PF2<2><1><default><default>
		sdc_cmd     		= port:PF3<2><1><default><default>
		sdc_d3      		= port:PF4<2><1><default><default>
		sdc_d2      		= port:PF5<2><1><default><default>

		[card_boot2_para]
		card_ctrl 		= 2
		card_high_speed 	= 1
		card_line       	= 4
		sdc_cmd		 	= port:PC6<3><1>
		sdc_clk	    		= port:PC7<3><1>
		sdc_d0		 	= port:PC8<3><1>
		sdc_d1		 	= port:PC9<3><1>
		sdc_d2		 	= port:PC10<3><1>
		sdc_d3		 	= port:PC11<3><1>
		*/

		/* test for card_boot0_para */
		PIO_CHECK_RST(0 != (pin_hdl = sw_gpio_request_ex("card_boot0_para", NULL)), uret, End);
		PIO_DBG_FUN_LINE;
		PIO_CHECK_RST(0 == sw_gpio_getall_range(cfg_card_boot0, ARRAY_SIZE(cfg_card_boot0)), uret, End);
		PIO_DBG_FUN_LINE;
		sw_gpio_dump_config(cfg_card_boot0, ARRAY_SIZE(cfg_card_boot0));
		PIO_DBG_FUN_LINE;
		sw_gpio_release(pin_hdl, 1);
		PIO_DBG_FUN_LINE;

		/* test for card_boot2_para */
		PIO_CHECK_RST(0 != (pin_hdl = sw_gpio_request_ex("card_boot2_para", NULL)), uret, End);
		PIO_DBG_FUN_LINE;
		PIO_CHECK_RST(0 == sw_gpio_getall_range(cfg_card_boot2, ARRAY_SIZE(cfg_card_boot2)), uret, End);
		PIO_DBG_FUN_LINE;
		sw_gpio_dump_config(cfg_card_boot2, ARRAY_SIZE(cfg_card_boot2));
		PIO_DBG_FUN_LINE;
		sw_gpio_release(pin_hdl, 1);
		PIO_DBG_FUN_LINE;

		/*sw_gpio_request
		sw_gpio_get_all_pin_status
		sw_gpio_get_one_pin_status
		sw_gpio_set_one_pin_status
		sw_gpio_set_one_pin_io_status
		sw_gpio_set_one_pin_pull
		sw_gpio_set_one_pin_driver_level
		sw_gpio_read_one_pin_value
		sw_gpio_write_one_pin_value*/
	}
#endif /* TEST_GPIO_SCRIPT_API */

End:
	if(0 == uret)
		printk("%s: test success!\n", __FUNCTION__);
	else
		printk("%s: test failed! line %d\n", __FUNCTION__, uret);
	return uret;
}

/**
 * __gpio_test_thread - gpio test main thread
 * @arg:	thread arg, not used
 *
 * Returns 0 if success, the err line number if failed.
 */
static int __gpio_test_thread(void * arg)
{
	u32 	uResult = 0;

	switch(g_cur_test_case) {
	case GTC_API:
		uResult = __gtc_api();
		break;
	default:
		uResult = __LINE__;
		break;
	}

	if(0 == uResult)
		printk("%s: test success!\n", __FUNCTION__);
	else
		printk("%s: test failed!\n", __FUNCTION__);

	return uResult;
}

/**
 * sw_gpio_test_init - enter the gpio test module
 */
static int __init sw_gpio_test_init(void)
{
	pr_info("%s enter\n", __FUNCTION__);

	/*
	 * create the test thread
	 */
	kernel_thread(__gpio_test_thread, NULL, CLONE_FS | CLONE_SIGHAND);
	return 0;
}

/**
 * sw_gpio_test_exit - exit the gpio test module
 */
static void __exit sw_gpio_test_exit(void)
{
	pr_info("sw_gpio_test_exit: enter\n");
}

#ifdef MODULE
module_init(sw_gpio_test_init);
module_exit(sw_gpio_test_exit);
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("liugang");
MODULE_DESCRIPTION ("sun6i gpio Test driver code");
#else
__initcall(sw_gpio_test_init);
#endif /* MODULE */

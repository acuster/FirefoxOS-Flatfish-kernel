#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <mach/sys_config.h>

#ifdef CONFIG_AW_AXP20

#define CONFIG_BY_SCRIPT
#undef  CONFIG_BY_SCRIPT

enum {
	DEBUG_INIT       = 1U << 0,
	DEBUG_DATA_INFO  = 1U << 1,
	DEBUG_SUSPEND    = 1U << 2,
};

static u32 debug_mask = 1;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)


static struct gpio_hdle {
	script_item_u	val;
	script_item_value_type_e  type;
}axp_gpio_hdle[AXP_NR];

struct delayed_work  axp_io_work;
static void axp_io_work_func(struct work_struct *work)
{
	int i;
	static int onoff;
	onoff=~onoff;
	for(i=0;i<AXP_NR;i++)
	{
			__gpio_set_value(axp_gpio_hdle[i].val.gpio.gpio,onoff);

  }

  schedule_delayed_work(&axp_io_work,msecs_to_jiffies(300));

}
static int __init pmu_gpio_test_init(void)
{
    #ifdef CONFIG_BY_SCRIPT
    script_item_u val;
    script_item_value_type_e type;
    char axp_sub_key_str[]="pmu_test_pin0";
    #endif
    int i;

    pr_info("pmu gpio test example init\n");
    for(i=0;i<AXP_NR;i++){

        #ifdef CONFIG_BY_SCRIPT
        axp_sub_key_str[12] = '0'+i;
        axp_gpio_hdle[i].type = script_get_item("pmu_para",axp_sub_key_str,&(axp_gpio_hdle[i].val));
        if(SCIRPT_ITEM_VALUE_TYPE_PIO != axp_gpio_hdle[i].type){
            printk(KERN_ERR "%s scprit_get_item \" pmu_para\" \"%s\n",__FUNCTION__,axp_sub_key_str);
            goto exit;
        }
		dprintk(DEBUG_INIT, "value is: gpio %d, mul_sel %d, pull %d, drv_level %d, data %d\n",
		 axp_gpio_hdle[i].val.gpio.gpio, axp_gpio_hdle[i].val.gpio.mul_sel, axp_gpio_hdle[i].val.gpio.pull,
		 axp_gpio_hdle[i].val.gpio.drv_level, axp_gpio_hdle[i].val.gpio.data);
	    #else
		 axp_gpio_hdle[i].val.gpio.gpio=AXP_NR_BASE+i;
		 axp_gpio_hdle[i].val.gpio.mul_sel =1;
        #endif
        if(0 != sw_gpio_setall_range(&(axp_gpio_hdle[i].val.gpio),1)){
            printk(KERN_ERR "pmu gpio set err!");
		    goto exit;
        }
	}

	INIT_DELAYED_WORK(&axp_io_work,axp_io_work_func);

	schedule_delayed_work(&axp_io_work,msecs_to_jiffies(300));

  return 0;

exit:
	return -1;

}
#else

static int __init pmu_gpio_test_init(void)
{

	  pr_info("pmu gpio test example init,but pmu not used!\n");

}
#endif

#ifdef CONFIG_AW_AXP20
static void __exit pmu_gpio_test_exit(void)
{
	cancel_delayed_work_sync(&axp_io_work);
    pr_info("pmu gpio test  exit\n");
}
#else
static void __exit pmu_gpio_test_exit(void)
{
    pr_info("pmu gpio test exit\n");
}
#endif

module_init(pmu_gpio_test_init);
module_exit(pmu_gpio_test_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin_zheng, allwinner");

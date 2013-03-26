/*
 * sunxi operation system resource
 * Author:raymonxiu
 */
#ifndef __VFE__OS__H__
#define __VFE__OS__H__

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
//#include <linux/gpio.h>

#include <mach/clock.h>
#include <mach/sys_config.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

typedef unsigned int __hdle;

extern struct clk *os_clk_get(struct device *dev, const char *id);
extern void  os_clk_put(struct clk *clk);
extern int os_clk_set_parent(struct clk *clk, struct clk *parent);
extern int os_clk_set_rate(struct clk *clk, unsigned long rate);
extern int os_clk_enable(struct clk *clk);
extern void os_clk_disable(struct clk *clk);
extern int os_clk_reset_assert(struct clk *clk);
extern int os_clk_reset_deassert(struct clk *clk); 
extern int os_request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,const char *name, void *dev);
extern __hdle os_gpio_request(unsigned gpio, const char *label);
//extern __hdle os_gpio_request_ex(char *main_name, const char *sub_name);
extern int os_gpio_release(unsigned gpio);                
extern int os_gpio_write(struct gpio_config *gpio, unsigned int status);
extern int os_gpio_set_status(struct gpio_config *gpio, unsigned int status);

#endif //__VFE__OS__H__
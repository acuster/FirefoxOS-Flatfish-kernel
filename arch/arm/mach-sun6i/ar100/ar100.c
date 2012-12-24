/*
 *  arch/arm/mach-sun6i/ar100/ar100.c
 *
 * Copyright (c) 2012 Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ar100_i.h"

/* local functions */
static int     ar100_wait_ready(unsigned int timeout);
static ssize_t ar100_debug_store(struct kobject *kobject,struct attribute *attr, const char *buf, size_t count);
static ssize_t ar100_debug_show(struct kobject *kobject,struct attribute *attr, char *buf);
static void    ar100_obj_release(struct kobject *kobject);

/* external vars */
extern char *ar100_binary_start;
extern char *ar100_binary_end;

unsigned long ar100_sram_a2_vbase = (unsigned long)IO_ADDRESS(AW_SRAM_A2_BASE);

struct attribute ar100_debug_mask_attr = {
	.name = "debug_mask",
	.mode = S_IRWXUGO
};
struct attribute ar100_debug_baudrate_attr = {
	.name = "debug_baudrate",
	.mode = S_IRWXUGO
};

static struct attribute *ar100_def_attrs[] = {
	&ar100_debug_mask_attr,
	&ar100_debug_baudrate_attr,
	NULL
};

struct sysfs_ops ar100_obj_sysops = {
	.show =  ar100_debug_show,
	.store = ar100_debug_store
};

struct kobj_type ar100_ktype = {
	.release       = ar100_obj_release,
	.sysfs_ops     = &ar100_obj_sysops,
	.default_attrs = ar100_def_attrs
};

static struct kobject ar100_kobj;
unsigned int g_ar100_debug_baudrate = 57600;

int ar100_init(void)
{
	int binary_len;
	int ret;
	
	AR100_INF("ar100 initialize\n");
	
	/* 
	 * request ar100 resources:
	 * p2wi/uart gpio...
	 */
	sw_gpio_setcfg(GPIOL(0), 3);	/* p2wi sck */
	sw_gpio_setcfg(GPIOL(1), 3);	/* p2wi sda */
	sw_gpio_setcfg(GPIOL(2), 2);	/* uart tx */
	sw_gpio_setcfg(GPIOL(3), 2);	/* uart rx */
	
	AR100_INF("sram_a2 vaddr(%x)\n", (unsigned int)ar100_sram_a2_vbase);
	
	/* clear sram_a2 area */
	memset((void *)ar100_sram_a2_vbase, 0, AW_SRAM_A2_SIZE);
	
	/* load ar100 system binary data to sram_a2 */
	binary_len = (int)(&ar100_binary_end) - (int)(&ar100_binary_start);
	memcpy((void *)ar100_sram_a2_vbase, (void *)(&ar100_binary_start), binary_len);
	AR100_INF("move ar100 binary data [addr = %x, len = %x] to sram_a2 finished\n", 
	         (unsigned int)(&ar100_binary_start), (unsigned int)binary_len);
	
	/* initialize hwspinlock */
	AR100_INF("hwspinlock initialize\n");
	ar100_hwspinlock_init();
	
	/* initialize hwmsgbox */
	AR100_INF("hwmsgbox initialize\n");
	ar100_hwmsgbox_init();
	
	/* initialize message manager */
	AR100_INF("message manager initialize\n");
	ar100_message_manager_init();
	
	/* set ar100 cpu reset to de-assert state */
	AR100_INF("set ar100 reset to de-assert state\n");
	{
		volatile unsigned long value;
		value = readl((IO_ADDRESS(AW_R_CPUCFG_BASE) + 0x0));
		value |= 1;
		writel(value, (IO_ADDRESS(AW_R_CPUCFG_BASE) + 0x0));
	}
	
	/* wait ar100 ready */
	AR100_INF("wait ar100 ready....\n");
	if (ar100_wait_ready(500000)) {
		AR100_LOG("ar100 startup failed\n");
	}
	
	/* enable ar100 asyn tx interrupt */
	ar100_hwmsgbox_enable_receiver_int(AR100_HWMSGBOX_AR100_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);
	
	/* enable ar100 syn tx interrupt */
	ar100_hwmsgbox_enable_receiver_int(AR100_HWMSGBOX_AR100_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);
	
	/* config dvfs v-f table */
	if (ar100_dvfs_cfg_vf_table()) {
		AR100_WRN("config dvfs v-f table failed\n");
	}
	
	/* config dram config paras */
	if (ar100_config_dram_paras()) {
		AR100_WRN("config dram paras failed\n");
	}
	
	/* register ar100 debug device node */
	ret = kobject_init_and_add(&ar100_kobj, &ar100_ktype, NULL, "ar100");
	if (ret) {
		AR100_WRN("add ar100 kobject failed\n");
	}
	
	/* ar100 initialize succeeded */
	AR100_INF("ar100 startup succeeded, driver version : %d\n", AR100_VERSIONS);
	
	return 0;
}
subsys_initcall(ar100_init);

static int ar100_wait_ready(unsigned int timeout)
{
	unsigned long          expire;
	
	expire = msecs_to_jiffies(timeout) + jiffies;
	
	/* wait ar100 startup ready */
	while (1) {
		/*
		 * linux cpu interrupt is disable now, 
		 * we should query message by hand.
		 */
		struct ar100_message *pmessage = ar100_hwmsgbox_query_message();
		if (pmessage == NULL) {
			if (time_is_before_eq_jiffies(expire)) {
				return -ETIMEDOUT;
			}
			/* try to query again */
			continue;
		}
		/* query valid message */
		if (pmessage->type == AR100_STARTUP_NOTIFY) {
			/* check ar100 software and driver version match or not */
			if (pmessage->paras[0] != AR100_VERSIONS) {
				AR100_ERR("ar100 firmware and driver version not matched\n");
				return -EINVAL;
			}
			/* received ar100 startup ready message */
			AR100_INF("ar100 startup ready\n");
			if ((pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) ||
				(pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN)) {
				/* synchronous message, just feedback it */
				AR100_INF("ar100 startup notify message feedback\n");
				ar100_hwmsgbox_feedback_message(pmessage, AR100_SEND_MSG_TIMEOUT);
			} else {
				/* asyn message, free message directly */
				AR100_INF("ar100 startup notify message free directly\n");
				ar100_message_free(pmessage);
			}
			break;
		}
		/* 
		 * invalid message detected, ignore it.
		 * by sunny at 2012-7-6 18:34:38.
		 */
		AR100_WRN("ar100 startup waiting ignore message\n");
		if ((pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) ||
			(pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN)) {
			/* synchronous message, just feedback it */
			ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);
		} else {
			/* asyn message, free message directly */
			ar100_message_free(pmessage);
		}
		/* we need waiting continue */
	}
	return 0;
}

static ssize_t ar100_debug_store(struct kobject *kobject,struct attribute *attr, const char *buf, size_t count)
{
	u32 value = 0;
	if (strcmp(attr->name, "debug_mask") == 0) {
		sscanf(buf, "%i", &value);
		g_ar100_debug_level = value;
		ar100_set_debug_level(g_ar100_debug_level);
		AR100_LOG("debug_mask change to %d\n", g_ar100_debug_level);
	} else if (strcmp(attr->name, "debug_baudrate") == 0) {
		sscanf(buf, "%i", &value);
		if ((g_ar100_debug_baudrate != 57600) && (g_ar100_debug_baudrate != 9600)) {
			AR100_WRN("invalid ar100 uart baudrate [%d] to set\n", g_ar100_debug_baudrate);
			return 0;
		}
		g_ar100_debug_baudrate = value;
		ar100_set_uart_baudrate(g_ar100_debug_baudrate);
		AR100_LOG("debug_baudrate change to %d\n", g_ar100_debug_baudrate);
	}
	
	return count;
}

static ssize_t ar100_debug_show(struct kobject *kobject,struct attribute *attr, char *buf)
{
	ssize_t count = 0;

	if (strcmp(attr->name, "debug_mask") == 0) {
		count = sprintf(buf, "%i\n", g_ar100_debug_level);
	} else if (strcmp(attr->name, "debug_baudrate") == 0) {
		count = sprintf(buf, "%d\n", g_ar100_debug_baudrate);
	}
	
	return count;
}

static void ar100_obj_release(struct kobject *kobject)
{
	printk("ar100 obj release\n");
}


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

/* external vars */
extern char *ar100_binary_start;
extern char *ar100_binary_end;

unsigned long ar100_sram_a2_vbase = (unsigned long)IO_ADDRESS(AW_SRAM_A2_BASE);
unsigned int ar100_debug_baudrate = 57600;
unsigned int ar100_debug_dram_crc_en = 0;
unsigned int ar100_debug_dram_crc_srcaddr = 0x40000000;
unsigned int ar100_debug_dram_crc_len = (1024 * 1024);
unsigned int ar100_debug_dram_crc_error = 0;
unsigned int ar100_debug_dram_crc_total_count = 0;
unsigned int ar100_debug_dram_crc_error_count = 0;
unsigned int ar100_debug_level = 2;


ssize_t ar100_debug_mask_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t size = 0;
	
	size = sprintf(buf, "%u\n", ar100_debug_level);
	
	return size;
}

ssize_t ar100_debug_mask_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 value = 0;
	
	sscanf(buf, "%u", &value);
	if ((value < 0) || (value > 3)) {
		AR100_WRN("invalid ar100 debug mask [%d] to set\n", value);
		return size;
	}
	
	ar100_debug_level = value;
	ar100_set_debug_level(ar100_debug_level);
	AR100_LOG("debug_mask change to %d\n", ar100_debug_level);
	
	return size;
}

ssize_t ar100_debug_baudrate_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t size = 0;
	
	size = sprintf(buf, "%u\n", ar100_debug_baudrate);
	
	return size;
}

ssize_t ar100_debug_baudrate_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 value = 0;
	
	sscanf(buf, "%u", &value);
	if ((value != 57600) && (value != 9600)) {
		AR100_WRN("invalid ar100 uart baudrate [%d] to set\n", value);
		return size;
	}
	
	ar100_debug_baudrate = value;
	ar100_set_uart_baudrate(ar100_debug_baudrate);
	AR100_LOG("debug_baudrate change to %d\n", ar100_debug_baudrate);
	
	return size;
}

ssize_t ar100_dram_crc_paras_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t size = 0;
	
	size = sprintf(buf, "enable:0x%x srcaddr:0x%x lenght:0x%x\n", ar100_debug_dram_crc_en,
			                 ar100_debug_dram_crc_srcaddr, ar100_debug_dram_crc_len);
	
	return size;
}

ssize_t ar100_dram_crc_paras_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 dram_crc_en      = 0;
	u32 dram_crc_srcaddr = 0;
	u32 dram_crc_len     = 0;
	
	sscanf(buf, "%x %x %x\n", &dram_crc_en, &dram_crc_srcaddr, &dram_crc_len);
	
	if (((dram_crc_en != 0) && (dram_crc_en != 1)) ||
	    ((dram_crc_srcaddr < 0x40000000) || (dram_crc_srcaddr > 0xc0000000)) ||
	    ((dram_crc_len < 0) || (dram_crc_len > (0x80000000)))) {
		AR100_WRN("invalid ar100 debug dram crc paras [%x] [%x] [%x] to set\n",
		                          dram_crc_en, dram_crc_srcaddr, dram_crc_len);

		return size;
	}
	
	ar100_debug_dram_crc_en = dram_crc_en;
	ar100_debug_dram_crc_srcaddr = dram_crc_srcaddr;
	ar100_debug_dram_crc_len = dram_crc_len;
	ar100_set_dram_crc_paras(ar100_debug_dram_crc_en, 
	                         ar100_debug_dram_crc_srcaddr,
	                         ar100_debug_dram_crc_len);
	AR100_LOG("dram_crc_en=0x%x, dram_crc_srcaddr=0x%x, dram_crc_len=0x%x\n",
	          ar100_debug_dram_crc_en, ar100_debug_dram_crc_srcaddr, ar100_debug_dram_crc_len);
	
	return size;
}

ssize_t ar100_dram_crc_result_show(struct class *class, struct class_attribute *attr, char *buf)
{
	ssize_t size = 0;
	
	ar100_query_dram_crc_result((unsigned long *)&ar100_debug_dram_crc_error,
							    (unsigned long *)&ar100_debug_dram_crc_total_count,
							    (unsigned long *)&ar100_debug_dram_crc_error_count);
	size = sprintf(buf, "error:%u total count:%u error count:%u\n", ar100_debug_dram_crc_error,
							ar100_debug_dram_crc_total_count, ar100_debug_dram_crc_error_count);
	
	return size;
}

ssize_t ar100_dram_crc_result_store(struct class *class, struct class_attribute *attr,
			const char *buf, size_t size)
{
	u32 error = 0;
	u32 total_count = 0;
	u32 error_count = 0;
	
	sscanf(buf, "%u %u %u", &error, &total_count, &error_count);
	if (((error != 0) && (error != 1)) || (total_count < 0) || (error_count < 0)) {
		AR100_WRN("invalid ar100 dram crc result [%d] [%d] [%d] to set\n", error, total_count, error_count);
		return size;
	}
	
	ar100_debug_dram_crc_error = error;
	ar100_debug_dram_crc_total_count = total_count;
	ar100_debug_dram_crc_error_count = error_count;
	ar100_set_dram_crc_result((unsigned long)ar100_debug_dram_crc_error,
							  (unsigned long)ar100_debug_dram_crc_total_count,
							  (unsigned long)ar100_debug_dram_crc_error_count);
	AR100_LOG("debug_dram_crc_result change to error:%u total count:%u error count:%u\n",
			ar100_debug_dram_crc_error, ar100_debug_dram_crc_total_count, ar100_debug_dram_crc_error_count);
	
	return size;
}


static struct class_attribute ar100_class_attrs[] = {
	__ATTR(debug_mask, 	    0644, ar100_debug_mask_show,      ar100_debug_mask_store),
	__ATTR(debug_baudrate,	0644, ar100_debug_baudrate_show,  ar100_debug_baudrate_store),
	__ATTR(dram_crc_paras,	0644, ar100_dram_crc_paras_show,  ar100_dram_crc_paras_store),
	__ATTR(dram_crc_result,	0644, ar100_dram_crc_result_show, ar100_dram_crc_result_store),
	__ATTR_NULL,
};

static struct class ar100_class = {
	.name		 = "ar100",
	.owner		 = THIS_MODULE,
	.class_attrs = ar100_class_attrs,
};

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
	if (ar100_wait_ready(10000)) {
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
	ret = class_register(&ar100_class);
	if (ret) {
		AR100_WRN("register ar100 class failed\n");
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

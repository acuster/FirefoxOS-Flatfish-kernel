/*
 *  arch/arm/mach-sun6i/ar100/ar100_standby.c
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
 
#include "..//ar100_i.h"
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

/* record super-standby wakeup event */
static unsigned long wakeup_event;

/**
 * enter super standby.
 * @para:  parameter for enter normal standby.
 *
 * return: result, 0 - super standby successed,
 *                !0 - super standby failed;
 */
int ar100_standby_super(struct super_standby_para *para)
{
	struct ar100_message *pmessage;
	
	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_ERR("allocate message for super-standby request failed\n");
		return -ENOMEM;
	}
	
	/* check super_standby_para size valid or not */
	if (sizeof(struct super_standby_para) > sizeof(pmessage->paras)) {
		AR100_ERR("super-standby parameters number too long\n");
		return -EINVAL;
	}
	
	/* initialize message */
	pmessage->type     = AR100_SSTANDBY_ENTER_REQ;
	pmessage->attr     = AR100_MESSAGE_ATTR_HARDSYN;
	memcpy(pmessage->paras, para, sizeof(struct super_standby_para));
	pmessage->state    = AR100_MESSAGE_INITIALIZED;
	
	/* notify hwspinlock and hwmsgbox will enter super-standby */
	ar100_hwspinlock_standby_suspend();
	ar100_hwmsgbox_standby_suspend();
	
	/* before creating mapping, build the coherent between cache and memory */
	/* clean and flush */
	__cpuc_flush_kern_all();
	__cpuc_coherent_kern_range(0xc0000000, 0xffffffff-1);
	
	/* send enter super-standby request to ar100 */
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);
	
	/* enter super-standby fail, notify hwspinlock and hwmsgbox resume */
	ar100_hwmsgbox_standby_resume();
	ar100_hwspinlock_standby_resume();
	
	return 0;
}
EXPORT_SYMBOL(ar100_standby_super);


/**
 * query super-standby wakeup source.
 * @para:  point of buffer to store wakeup event informations.
 *
 * return: result, 0 - query successed,
 *                !0 - query failed;
 */
int ar100_query_wakeup_source(unsigned long *event)
{
	*event = wakeup_event;
	
	return 0;
}
EXPORT_SYMBOL(ar100_query_wakeup_source);


/**
 * notify ar100 cpux restored.
 * @para:  none.
 *
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int ar100_cpux_ready_notify(void)
{
	struct ar100_message *pmessage;
	
	/* notify hwspinlock and hwmsgbox resume first */
	ar100_hwmsgbox_standby_resume();
	ar100_hwspinlock_standby_resume();
	
	/* allocate a message frame */
	pmessage = ar100_message_allocate(AR100_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		AR100_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	
	/* initialize message */
	pmessage->type     = AR100_SSTANDBY_RESTORE_NOTIFY;
	pmessage->attr     = AR100_MESSAGE_ATTR_HARDSYN;
	pmessage->state    = AR100_MESSAGE_INITIALIZED;
	
	ar100_hwmsgbox_send_message(pmessage, AR100_SEND_MSG_TIMEOUT);
	
	/* record wakeup event */
	wakeup_event = pmessage->paras[0];
	
	/* free message */
	ar100_message_free(pmessage);
	
	return 0;
}
EXPORT_SYMBOL(ar100_cpux_ready_notify);

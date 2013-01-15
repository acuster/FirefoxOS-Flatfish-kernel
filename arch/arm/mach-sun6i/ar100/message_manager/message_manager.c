/*
 *  arch/arm/mach-sun6i/ar100/message_manager/message_manager.c
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


#include "message_manager_i.h"

/* the start and end of message pool */
static struct ar100_message *message_start;
static struct ar100_message *message_end;

/* spinlock for this module */
static spinlock_t    msg_mgr_lock;
static unsigned long msg_mgr_flag;

/* message cache manager */
static struct ar100_message_cache message_cache;

/* semaphore cache manager */
static struct ar100_semaphore_cache sem_cache;

/**
 * initialize message manager.
 * @para:  none.
 *
 * returns:  0 if initialize succeeded, others if failed.
 */
int ar100_message_manager_init(void)
{
	int i;
	
	/* initialize message pool start and end */
	message_start = (struct ar100_message *)(ar100_sram_a2_vbase + AR100_MESSAGE_POOL_START);
	message_end   = (struct ar100_message *)(ar100_sram_a2_vbase + AR100_MESSAGE_POOL_END);
	
	/* initialize message_cache */
	for (i = 0; i < AR100_MESSAGE_CACHED_MAX; i++) {
		message_cache.cache[i] = NULL;
	}
	message_cache.number = 0;
	
	/* initialzie semaphore allocator */
	for (i = 0; i < AR100_SEM_CACHE_MAX; i++) {
		sem_cache.cache[i] = NULL;
	}
	sem_cache.number = 0;
	
	/* initialize message manager spinlock */
	spin_lock_init(&(msg_mgr_lock));
	msg_mgr_flag = 0;
	
	return 0;
}

/**
 * exit message manager.
 * @para:  none.
 *
 * returns:  0 if exit succeeded, others if failed.
 */
int ar100_message_manager_exit(void)
{
	return 0;
}

static struct semaphore *ar100_semaphore_allocate(void)
{
	struct semaphore *sem = NULL;
	
	if (sem_cache.number) {
		/* allocate from cache first */
		spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
		sem_cache.number--;
		sem = sem_cache.cache[sem_cache.number];
		sem_cache.cache[sem_cache.number] = NULL;
		spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);
	}
	if (sem == NULL) {
		/* cache allocate fail, allocate from kmem */
		sem = kmalloc(sizeof(struct semaphore), GFP_KERNEL);
	}
	if (sem) {
		/* initialize allocated semaphore */
		sema_init(sem, 0);
	}
	return sem;
}

static int ar100_semaphore_free(struct semaphore *sem)
{
	if (sem == NULL) {
		AR100_WRN("free null semaphore\n");
		return -EINVAL;
	}
	if (sem_cache.number < AR100_SEM_CACHE_MAX) {
		/* free to cache */
		spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
		sem_cache.cache[sem_cache.number] = sem;
		sem_cache.number++;
		spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);
	} else {
		/* free to kmem */
		kfree(sem);
	}
	return 0;
}

static int ar100_message_invalid(struct ar100_message *pmessage)
{
	if ((pmessage >= message_start) &&
		(pmessage < message_end))
	{
		/* valid ar100 message */
		return 0;
	}
	/* invalid ar100 message */
	return 1;
}

/**
 * allocate one message frame. mainly use for send message by message-box,
 * the message frame allocate form messages pool shared memory area.
 * @para:  none.
 *
 * returns:  the pointer of allocated message frame, NULL if failed;
 */
struct ar100_message *ar100_message_allocate(unsigned int msg_attr)
{
	struct ar100_message *pmessage = NULL;
	struct ar100_message *palloc   = NULL;
	
	/* first find in message_cache */
	if (message_cache.number) {
		AR100_INF("ar100 message_cache.number = 0x%x.\n", message_cache.number);
		spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
		message_cache.number--;
		palloc = message_cache.cache[message_cache.number];
		spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);
		AR100_INF("message [%x] allocate from message_cache\n", (u32)palloc);
	}
	if (ar100_message_invalid(palloc)) {
		/*
		 * cached message_cache finded fail, 
		 * use spinlock 0 to exclusive with ar100.
		 */
		ar100_hwspin_lock_timeout(0, AR100_SPINLOCK_TIMEOUT);
		
		/* seach from the start of message pool every time. */
		pmessage = message_start;
		while (pmessage < message_end) {
			if (pmessage->state == AR100_MESSAGE_FREED) {
				/* find free message in message pool, allocate it */
				palloc = pmessage;
				palloc->state = AR100_MESSAGE_ALLOCATED;
				AR100_INF("message [%x] allocate from message pool\n", (u32)palloc);
				break;
			}
			/* next message frame */
			pmessage++;
		}
		/* unlock hwspinlock 0 */
		ar100_hwspin_unlock(0);
	}
	if (ar100_message_invalid(palloc)) {
		AR100_ERR("allocate message frame is invalid\n");
		return NULL;
	}
	/* initialize messgae frame */
	palloc->next = 0;
	palloc->attr = msg_attr;
	if (msg_attr & AR100_MESSAGE_ATTR_SOFTSYN) {
		/* syn message,allocate one semaphore for private */
		palloc->private = ar100_semaphore_allocate();
	} else {
		palloc->private = NULL;
	}
	return palloc;
}

/**
 * free one message frame. mainly use for process message finished, 
 * free it to messages pool or add to free message queue.
 * @pmessage:  the pointer of free message frame.
 *
 * returns:  none.
 */
void ar100_message_free(struct ar100_message *pmessage)
{
	/* check this message valid or not */
	if (ar100_message_invalid(pmessage)) {
		AR100_WRN("free invalid ar100 message\n");
		return;
	}
	if (pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) {
		/* free message semaphore first */
		ar100_semaphore_free((struct semaphore *)(pmessage->private));
		pmessage->private = NULL;
	}
	/* try to add free_list first */
	if (message_cache.number < AR100_MESSAGE_CACHED_MAX) {
		AR100_INF("insert message [%x] to message_cache\n", (unsigned int)pmessage);
		AR100_INF("message_cache number : %d\n", message_cache.number);
		/* cached this message, message state: ALLOCATED */
		spin_lock_irqsave(&(msg_mgr_lock), msg_mgr_flag);
		message_cache.cache[message_cache.number] = pmessage;
		message_cache.number++;
		pmessage->next = NULL;
		pmessage->state = AR100_MESSAGE_ALLOCATED;
		spin_unlock_irqrestore(&(msg_mgr_lock), msg_mgr_flag);
	} else {
		/*
		 * free to message pool,
		 * set message state as FREED.
		 */
		ar100_hwspin_lock_timeout(0, AR100_SPINLOCK_TIMEOUT);
		pmessage->state = AR100_MESSAGE_FREED;
		pmessage->next  = NULL;
		ar100_hwspin_unlock(0);
	}
}

/**
 * notify system that one message coming.
 * @pmessage:  the pointer of coming message frame.
 *
 * returns:  0 if notify succeeded, other if failed.
 */
int ar100_message_coming_notify(struct ar100_message *pmessage)
{
	int   ret;
	
	/* ac327 receive message to ar100 */
	AR100_INF("-------------------------------------------------------------\n");
	AR100_INF("                MESSAGE FROM AR100                           \n");
	AR100_INF("message addr : %x\n", (u32)pmessage);
	AR100_INF("message type : %x\n", pmessage->type);
	AR100_INF("message attr : %x\n", pmessage->attr);
	AR100_INF("-------------------------------------------------------------\n");
	
	/* message per-process */
	pmessage->state = AR100_MESSAGE_PROCESSING;
	
	/* process message */
	switch (pmessage->type) {
		case AR100_AXP_INT_COMING_NOTIFY: {
			AR100_INF("pmu interrupt coming notify\n");
			ret = ar100_axp_int_notify(pmessage);
			pmessage->result = ret;
			break;
		}
		default : {
			AR100_ERR("invalid message type for ac327 process\n");
			ret = -EINVAL;
			break;
		}
	}
	/* message post process */
	pmessage->state = AR100_MESSAGE_PROCESSED;
	if ((pmessage->attr & AR100_MESSAGE_ATTR_SOFTSYN) || 
		(pmessage->attr & AR100_MESSAGE_ATTR_HARDSYN)) {
		/* synchronous message, should feedback process result */
		ar100_hwmsgbox_feedback_message(pmessage, AR100_SEND_MSG_TIMEOUT);
	} else {
		/*
		 * asyn message, no need feedback message result,
		 * free message directly.
		 */
		ar100_message_free(pmessage);	
	}
	
	return ret;
}

/******************************************************************
 *
 * Copyright (C) 2011 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT OMCI protocol stack
 * Module  : notifychain
 * File    : notify_chain.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "list.h"
#include "notify_chain.h"
#include "fwk_rwlock.h"
#include "util.h"

/* Need to do lock*/

static struct list_head notify_chain_list[NOTIFY_CHAIN_EVENT_MAX];
struct fwk_rwlock_t notify_chain_lock;

struct notify_chain_node_t {
	//unsigned char event;
	int (*cb_func_ptr) (unsigned char event, unsigned short classid,
			    unsigned short instance_num, unsigned short meid, void *data_ptr);
	void *data_ptr;
	struct list_head node;
};

int
notify_chain_preinit()
{
	int i;
	//init function
	for (i = 0; i < NOTIFY_CHAIN_EVENT_MAX; i++)
		INIT_LIST_HEAD(&notify_chain_list[i]);

	fwk_rwlock_create(&notify_chain_lock);
	return 0;
}

int
notify_chain_register(unsigned char event,
		      int (*cb_func_ptr) (unsigned char event,
					  unsigned short classid,
					  unsigned short instance_num,
					  unsigned short meid, void *data_ptr), void *data_ptr)
{

	struct notify_chain_node_t *chain_node;

	if (event >= NOTIFY_CHAIN_EVENT_MAX) {
		dbprintf(LOG_DEBUG, "Event out of NOTIFY_CHAIN_EVENT_MAX\n");
		return -1;
	}

	fwk_rwlock_wrlock(&notify_chain_lock);	//atomic operator
	chain_node = malloc_safe(sizeof (struct notify_chain_node_t));
	//chain_node->event=event;
	chain_node->cb_func_ptr = cb_func_ptr;
	chain_node->data_ptr = data_ptr;

	dbprintf(LOG_DEBUG, "node add:%p\n", cb_func_ptr);
	list_add_tail(&chain_node->node, &notify_chain_list[event]);
	fwk_rwlock_unlock(&notify_chain_lock);	//atomic operator
	return 0;
}

int
notify_chain_unregister(unsigned char event,
			int (*cb_func_ptr) (unsigned char event,
					    unsigned short classid,
					    unsigned short instance_num,
					    unsigned short meid, void *data_ptr), void *data_ptr)
{
	struct list_head *list, *n;

	if (event >= NOTIFY_CHAIN_EVENT_MAX) {
		dbprintf(LOG_DEBUG, "Event out of NOTIFY_CHAIN_EVENT_MAX\n");
		return -1;
	}

	fwk_rwlock_wrlock(&notify_chain_lock);	//atomic operator
	list_for_each_safe(list, n, &notify_chain_list[event]) {
		struct notify_chain_node_t *chain_node = list_entry(list, struct notify_chain_node_t, node);

		if (chain_node->cb_func_ptr == cb_func_ptr && chain_node->data_ptr == data_ptr) {

			dbprintf(LOG_DEBUG, "node delete:%p\n", cb_func_ptr);
			list_del(&chain_node->node);
			free_safe(chain_node);
		}
	}

	fwk_rwlock_unlock(&notify_chain_lock);	//atomic operator
	return 0;
}

int
notify_chain_notify(unsigned char event, unsigned short classid, unsigned short instance_num, unsigned short meid)
{
	struct list_head *list, *n;
	int ret = 0;

	if (event >= NOTIFY_CHAIN_EVENT_MAX) {
		dbprintf(LOG_DEBUG, "Event out of NOTIFY_CHAIN_EVENT_MAX\n");
		return -1;
	}

	fwk_rwlock_rdlock(&notify_chain_lock);	//atomic operator
	list_for_each_safe(list, n, &notify_chain_list[event]) {
		struct notify_chain_node_t *chain_node = list_entry(list, struct notify_chain_node_t, node);

		dbprintf(LOG_DEBUG, "notify_chain_notify, classid=%u\n", classid);
		if (chain_node->cb_func_ptr) {
			if ((ret =
			     chain_node->cb_func_ptr(event, classid, instance_num, meid, chain_node->data_ptr)) != 0) {
				fwk_rwlock_unlock(&notify_chain_lock);	//atomic operator
				return ret;
			}
		}
	}
	fwk_rwlock_unlock(&notify_chain_lock);	//atomic operator
	return 0;
}

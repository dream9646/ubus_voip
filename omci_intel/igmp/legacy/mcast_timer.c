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
 * Module  : igmp
 * File    : igmp_timer.c
 *
 ******************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"
#include "mcast.h"
#include "mcast_timer.h"

static LIST_HEAD(custom_timer_g);

struct igmp_custom_timer_t {
	int event;
	int *timer_id;
	int (*cb)(void *cb_data);
	struct list_head node;
};

// unit: ms
long
igmp_timer_get_cur_time(void)
{
	struct timeval tv;	
	util_get_uptime(&tv);
	return (long)(tv.tv_sec *1000 + tv.tv_usec/1000);
}

int igmp_custom_timer_handler(struct fwk_timer_msg_t *msg)
{
	struct igmp_custom_timer_t *pos, *n;
	
	if (msg == NULL) return -1;
	
	list_for_each_entry_safe(pos, n, &custom_timer_g, node) {
		if (pos->event != msg->event) continue;
		if (*pos->timer_id != msg->timer_id) continue;
		
		pos->cb((void *)msg->user_data);	// execute callback

		list_del(&pos->node);
		free_safe(pos);
		break;		
	}
	
	return 0;
}

int igmp_register_custom_timer(int event, int *id, int interval, void *custom_data, int (*cb)(void *data))
{   
	if (!(event & TIMER_EVENT_CUSTOM))	return -1;	// custom bit not set
	if (id == NULL || cb == NULL)	return -1;	// cant be null
		
	struct igmp_custom_timer_t *t = malloc_safe(sizeof(struct igmp_custom_timer_t));
        memset(t, 0, sizeof(struct igmp_custom_timer_t));
        t->timer_id = id;
        t->event = event;
        t->cb = cb;
	list_add_tail(&t->node, &custom_timer_g);
	
	*id = fwk_timer_create(igmp_timer_qid_g, event, interval, custom_data);
	
	return 0; 
}

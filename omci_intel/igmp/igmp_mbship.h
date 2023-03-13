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
 * File    : igmp_mbship.h
 *
 ******************************************************************/

#ifndef __IGMP_MBSHIP_H__
#define __IGMP_MBSHIP_H__

#include <netinet/in.h>

struct igmp_mbship_db_t {
	struct list_head 	mbship_list;
	struct list_head 	mcast_snoop_list;
	struct list_head 	mcast_proxy_list;
	int (*mcast_mbship_db_event)(short, void *) ;
};

/* igmp_mbship.c */
#define IGMP_MEMBER_EVENT_ADD		1
#define IGMP_MEMBER_EVENT_DEL		2
#define IGMP_MEMBER_EVENT_RELOAD	3
int mcast_mbship_register_event(int (*cb)(short, void *));

extern struct igmp_mbship_db_t igmp_mbship_db_g;
int igmp_mbship_port_clear(struct igmp_mbship_db_t *igmp_mbship_db, unsigned short src_port_id);

struct mcast_mbship_t;
void mcast_mbship_db_remove_member(struct mcast_mbship_t *m);
#endif

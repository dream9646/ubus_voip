/******************************************************************
 *
 * Copyright (C) 2016 5V Technologies Ltd.
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
 * Module  : igmp counter module - calix
 * File    : igmp_counter_calix_eventlog.h
 *
 ******************************************************************/


#ifndef __IGMP_COUNTER_CALIX_EVENTLOG_H__
#define __IGMP_COUNTER_CALIX_EVENTLOG_H__

struct igmp_pkt_calix_eventlog_t {
	char time[32];
	char dir[8];
	char event[32];
	char device[128];
	char groupip[128];
	char ver[8];
	unsigned char mode;
	int num_src;
	char sourceip[128];
	struct list_head node;
};

extern struct igmp_counter_module_t calix_eventlog; 
#endif
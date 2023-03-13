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
 * File    : mcast_main.h
 *
 ******************************************************************/

#ifndef __IGMP_MAIN_H__
#define __IGMP_MAIN_H__

extern int igmp_pkt_recv_qid_g;
extern int timer_id_query_interval_g; 

/* igmp_main.c */
void *igmp_main(void *ptr);

#endif

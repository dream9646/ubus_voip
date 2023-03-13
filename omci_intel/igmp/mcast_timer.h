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
 * File    :mcast_timer.h
 *
 ******************************************************************/

#ifndef __IGMP_TIMER_H__
#define __IGMP_TIMER_H__

#include "fwk_timer.h"

/* igmp_timer.c */
long igmp_timer_get_cur_time(void);
int igmp_register_custom_timer(int event, int *id, int interval, void *custom_data, int (*cb)(void *data));
int igmp_custom_timer_handler(struct fwk_timer_msg_t *msg);

#endif

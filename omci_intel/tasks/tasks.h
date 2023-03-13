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
 * Module  : tasks
 * File    : tasks.h
 *
 ******************************************************************/

#ifndef __TASKS_H__
#define __TASKS_H__

/* tasks.c */
int tasks_switch_rt_to_normal();
int tasks_switch_normal_to_rt();
int tasks_init(void);
int tasks_shutdown(void);
int tasks_start(void);
int tasks_stop(void);
int tasks_util_is_avc_alarm_tca_allowed(void);

#endif

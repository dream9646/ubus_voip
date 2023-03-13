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
 * Module  : util
 * File    : util_run.h
 *
 ******************************************************************/

#ifndef __UTIL_RUN_H__
#define __UTIL_RUN_H__

/* util_run.c */
int util_run_by_thread(char *cmd, int is_fg);
int util_run_by_system(char *cmd);
int util_thread_sched(int policy, int priority);

#endif

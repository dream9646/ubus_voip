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
 * Module  : omciutil
 * File    : omciutil_usage.h
 *
 ******************************************************************/

#ifndef __OMCIUTIL_USAGE_H__
#define __OMCIUTIL_USAGE_H__

/* omciutil_usage.c */
char *omciutil_usage_cpu_counter_to_str(unsigned long total_counter, unsigned long cpu_counter1[]);
int omciutil_usage_cpu_used_percentage(void);
int omciutil_usage_mem_used_percentage(void);

#endif

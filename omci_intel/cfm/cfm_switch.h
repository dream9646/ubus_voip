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
 * Module  : cfm
 * File    : cfm_switch.h
 *
 ******************************************************************/

#ifndef __CFM_SWITCH_H__
#define __CFM_SWITCH_H__

/* cfm_switch.c */
int cfm_switch_extract_set(int enable);
int cfm_switch_extract_get(int *enable);

#endif

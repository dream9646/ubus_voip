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
 * Module  : meinfo_cb
 * File    : meinfo_cb.h
 *
 ******************************************************************/

#ifndef __MEINFO_CB_H__
#define __MEINFO_CB_H__

#include <stdio.h>
#include "meinfo.h"

#define MEINFO_CB_DELETE_NONE		0
#define MEINFO_CB_DELETE_ENTRY		1
#define MEINFO_CB_DELETE_ALL_ENTRY	2

// similar return value as strcmp
#define MEINFO_CB_ENTRY_GREATER		1
#define MEINFO_CB_ENTRY_LESS		-1
#define MEINFO_CB_ENTRY_MATCH		0

int meinfo_cb_init(void);
int meinfo_cb_register(unsigned short classid, struct meinfo_callback_t *cb);

#endif //__MEINFO_CB_H__

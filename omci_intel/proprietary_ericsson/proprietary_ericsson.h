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
 * Module  : proprietary_ericsson
 * File    : proprietary_ericsson.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_ERICSSON_H__
#define __PROPRIETARY_ERICSSON_H__

#include "er_group.h"

/* batchtab_cb_ericsson_pvlan.c */
int batchtab_cb_ericsson_pvlan_register(void);
/* proprietary_ericsson.c */
int proprietary_ericsson(void);
#endif

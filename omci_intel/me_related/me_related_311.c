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
 * Module  : me_related
 * File    : me_related_311.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.29 [311] Multicast subscriber monitor

// relation to 9.3.4 [47] MAC bridge port configuration data
int 
me_related_311_047(struct me_t *me1, struct me_t *me2)	
{
	return me_related_047_311(me2, me1);
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int 
me_related_311_130(struct me_t *me1, struct me_t *me2)	
{
	return me_related_130_311(me2, me1);
}


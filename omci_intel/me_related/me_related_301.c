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
 * File    : me_related_301.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.21 [301] Dot1ag default MD level

// relation to 9.3.1 [045] MAC bridge service profile
int 
me_related_301_045(struct me_t *me1, struct me_t *me2)	
{
	return me_related_045_301(me2, me1);
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int 
me_related_301_130(struct me_t *me1, struct me_t *me2)	
{
	return me_related_130_301(me2, me1);
}

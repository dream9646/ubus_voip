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
 * File    : me_related_046.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"

// relation check for 9.3.2 [046] MAC bridge configuration data

// relation to 9.3.1 [045] MAC bridge service profile
int
me_related_046_045(struct me_t *me1, struct me_t *me2)
{
	return me_related_045_046(me2, me1);
}

// relation to 9.3.18 [298] Dot1_rate_limiter(before G.984.4 amd2)
int 
me_related_046_298(struct me_t *me1, struct me_t *me2)
{
	return me_related_298_046(me2, me1);
}

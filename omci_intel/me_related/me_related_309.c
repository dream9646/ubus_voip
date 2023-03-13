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
 * File    : me_related_309.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"

// relation check for 9.3.27 [309] Multicast operation profile

// relation to 9.3.28 [310] Multicast subscriber config info
int 
me_related_309_310(struct me_t *me1, struct me_t *me2)	
{
	return me_related_310_309(me2, me1);
}
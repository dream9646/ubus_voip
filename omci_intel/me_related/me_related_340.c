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
 * File    : me_related_340.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.12.16 [340] BBF TR-069 management server

// relation to 9.5.5 [329] Virtual Ethernet interface point
int 
me_related_340_329(struct me_t *me1, struct me_t *me2)	
{
	return me_related_329_340(me2, me1);
}

// relation to 9.12.3 [137] Network_address
int 
me_related_340_137(struct me_t *me1, struct me_t *me2)	
{
	return me_related_137_340(me2, me1);
}

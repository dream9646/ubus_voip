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
 * Module  : proprietary_zte
 * File    : me_related_245.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_zte.h"

// relation check for 9.99.8 [245] Traffic limit uni

// relation to 9.5.1 [011] Pptp Ethernet UNI
int
me_related_245_011(struct me_t *me1, struct me_t *me2)
{
	return me_related_011_245(me2, me1);
}
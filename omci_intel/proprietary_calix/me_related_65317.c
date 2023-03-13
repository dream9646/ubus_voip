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
 * Module  : proprietary_calix
 * File    : me_related_65317.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_calix.h"

// relation check for CalixRgConfig

// relation to 9.5.5 [329] Virtual Ethernet interface point
int
me_related_65317_329(struct me_t *me1, struct me_t *me2)
{
	return me_related_329_65317(me2, me1);
}

// relation to 9.12.4 [148] Authentication_security_method
int
me_related_65317_148(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
		return 0;

	meinfo_me_attr_get(me1, 6, &attr); // pointer to Authentication_security_method
	
	return ((unsigned short)(attr.data) == me2->meid);
}

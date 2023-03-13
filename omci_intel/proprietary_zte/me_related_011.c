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
 * File    : me_related_011.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.5.1 [011] Pptp Ethernet UNI

// relation to 9.99.8 [245] Traffic limit uni
int
me_related_011_245(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me2, 1, &attr);
	if ((unsigned short)(attr.data)==me1->meid)
		return 1;
	return 0;
}

// relation to 9.99.10 [253] Extended Physical path termination point Ethernet UNI
int
me_related_011_253(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid==me2->meid)
		return 1;
	return 0;
}

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
 * File    : me_related_136.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.4.14 [136] TCP/UDP_config_data

// relation to 9.4.12 [134] IP_host_config_data
int
me_related_136_134(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr); //pointer to ip host config data
	
	if (attr.data == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.5.5 [329] Virtual Ethernet interface point
int
me_related_136_329(struct me_t *me1, struct me_t *me2)
{
	return me_related_329_136(me2, me1);
}

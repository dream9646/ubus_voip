/******************************************************************
 *
 * Copyright (C) 2134 5V Technologies Ltd.
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
 * File    : me_related_134.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.4.12 [134] IP host config data

// relation to 9.99.9 [250] Extend ip host config
int
me_related_134_250(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid) ? 1 : 0;
}

// relation to 9.99.10 [65321] WAN
int
me_related_134_65321(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me2, 2, &attr);
	if ((unsigned short)(attr.data)==me1->meid)
		return 1;
	return 0;
}

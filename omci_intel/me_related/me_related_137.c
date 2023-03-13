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
 * File    : me_related_137.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.12.3 [137] Network_address

// relation to 9.4.16 [340] BBF_TR-069_management_server
int
me_related_137_340(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me2, 2, &attr);

	if (attr.data == me1->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.12.4 [148] Authentication_security_method
int
me_related_137_148(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr);
	
	if (attr.data == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.12.5 [157] Large_string
int
me_related_137_157(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr);
	
	if (attr.data == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

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
 * Module  : proprietary_fiberhome
 * File    : me_related_65285.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "env.h"

// relation check for 9.99.7 [65285] Switch port packet control

// relation to 9.3.4 [047] MAC bridge port configuration data
int
me_related_65285_047(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}


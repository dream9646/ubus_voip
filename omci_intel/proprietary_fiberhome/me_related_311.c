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
 * File    : me_related_311.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "env.h"


int
me_related_311_65311(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}


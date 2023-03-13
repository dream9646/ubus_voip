/******************************************************************
 *
 * Copyright (C) 2280 5V Technologies Ltd.
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
 * Module  : proprietary_alu
 * File    : me_related_280.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.11.1 [280] Traffic_descriptor

// relation to 9.99.134 [65323] Priority_Queue-G_Supplemental_1
int
me_related_280_65323(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
		return 0;

	meinfo_me_attr_get(me2, 1, &attr); //upstream td pointer
	
	return ((unsigned short)(attr.data) == me1->meid);
}


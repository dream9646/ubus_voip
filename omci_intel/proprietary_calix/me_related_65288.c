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
 * File    : me_related_65288.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_calix.h"

// relation check for 9.9.26 [65288] OamMeg

// relation to 9.99.27 [65289] OamMep
int 
me_related_65288_65289(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me2, 1, &attr1);

        if ((unsigned short)(attr1.data)==me1->meid)
                return 1;
	else
		return 0;
}

// relation to 9.99.28 [65290] OamMip
int 
me_related_65288_65290(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me2, 1, &attr1);

        if ((unsigned short)(attr1.data)==me1->meid)
                return 1;
	else
		return 0;
}

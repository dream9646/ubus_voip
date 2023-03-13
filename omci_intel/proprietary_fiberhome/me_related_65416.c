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
 * File    : me_related_65416.c
 *
 ******************************************************************/

#include "me_related.h"
#include <stdio.h>

// relation check for 9.99.16 [65416] WRI_MG_port_register_control

// relation to 9.9.1 [53] Physical_path_termination_point_POTS_UNI
int
me_related_65416_053(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //pptp pointer

	if ((unsigned short)(attr1.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}


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
 * File    : me_related_264.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.12.1 [264] UNI-G

// relation to 9.5.1 [11] Physical path termination point Ethernet UNI
int 
me_related_264_011(struct me_t *me1, struct me_t *me2)	
{
	return me_related_011_264(me2, me1);
}

// relation to 9.5.5 [329] Virtual Ethernet interface point
int 
me_related_264_329(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); // Management capability
	meinfo_me_attr_get(me1, 4, &attr2); // Non-OMCI management identifier
	
	if (attr1.data == 1 || attr1.data == 2) {
		return ((unsigned short)(attr2.data) == me2->meid);
	} else {
		return 0;
	}
}

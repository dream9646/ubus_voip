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
 * File    : me_related_302.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.22 [302] Dot1ag MEP

// relation to 9.3.4 [047] MAC bridge port configuration data
int 
me_related_302_047(struct me_t *me1, struct me_t *me2)	
{
	return me_related_047_302(me2, me1);
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int 
me_related_302_130(struct me_t *me1, struct me_t *me2)	
{
	return me_related_130_302(me2, me1);
}

// relation to 9.3.20 [300] Dot1ag maintenance association
int 
me_related_302_300(struct me_t *me1, struct me_t *me2)	
{
	return me_related_300_302(me2, me1);
}

// relation to 9.3.23 [303] Dot1ag MEP status
int 
me_related_302_303(struct me_t *me1, struct me_t *me2)	
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.24 [304] Dot1ag MEP CCM database
int 
me_related_302_304(struct me_t *me1, struct me_t *me2)	
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

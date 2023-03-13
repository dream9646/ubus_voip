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
 * File    : me_related_65301.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_calix.h"

// relation check for 9.9.37 [65301] OamMipStats

// relation to 9.99.28 [65290] OamMip
int 
me_related_65301_65290(struct me_t *me1, struct me_t *me2)	
{
	return me_related_65290_65301(me2, me1);
}

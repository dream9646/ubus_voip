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
 * File    : me_related_65299.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_calix.h"

// relation check for 9.9.35 [65299] OamMepStats

// relation to 9.99.27 [65289] OamMep
int 
me_related_65299_65289(struct me_t *me1, struct me_t *me2)	
{
	return me_related_65289_65299(me2, me1);
}

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
 * File    : me_related_278.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.11.2 [278] Traffic scheduler-G 

// relation to 9.1.6 [6] Circuit_pack
int 
me_related_278_006(struct me_t *me1, struct me_t *me2)	
{
	return me_related_006_278(me2, me1);
}

// relation to 9.2.2 [262] T-CONT
int 
me_related_278_262(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;

	// get attr1 (T-CONT pointer)
	meinfo_me_attr_get(me1, 1, &attr);	
	
	return ((unsigned short)attr.data==me2->meid);
}

// relation to 9.11.2 [278] Traffic scheduler-G pointer
int 
me_related_278_278(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;

	// get attr2 (Traffic scheduler-G pointer)
	meinfo_me_attr_get(me1, 2, &attr);	
	
	return ((unsigned short)attr.data==me2->meid);
}

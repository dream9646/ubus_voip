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
 * File    : me_related_65290.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "util.h"
#include "proprietary_fiberhome.h"

// relation to [65288] omci_me_sw_port_acl_rule
int
me_related_65290_65288(struct me_t *me1, struct me_t *me2)
{
	return me_related_65288_65290(me2, me1);
}


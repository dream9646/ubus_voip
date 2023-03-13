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
 * File    : me_related_049.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"

// relation check for 9.3.6 [049] MAC bridge port filter table data

// relation to 9.3.4 [047] MAC bridge port configuration data
int
me_related_049_047(struct me_t *me1, struct me_t *me2)
{
	return me_related_047_049(me2, me1);
}


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
 * File    : me_related_141.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.9.11 [141] VoIP_line_status

// relation check to 9.9.1 [053] Physical_path_termination_point_POTS_UNI
int
me_related_141_053(struct me_t *me1, struct me_t *me2)
{
	return me_related_053_141(me2, me1);
}

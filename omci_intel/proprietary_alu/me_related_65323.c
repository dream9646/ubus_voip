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
 * Module  : proprietary_alu
 * File    : me_related_65323.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_alu.h"

// relation check for 9.99.134 [65323] Priority_Queue-G_Supplemental_1

// relation to 9.11.1 [277] Priority_queue_G
int
me_related_65323_277(struct me_t *me1, struct me_t *me2)
{
	return me_related_277_65323(me2, me1);
}

//relation to 9.11.3 280 GEM_traffic_descriptor
int me_related_65323_280(struct me_t *me1, struct me_t *me2)
{
	return me_related_280_65323(me2, me1);
}

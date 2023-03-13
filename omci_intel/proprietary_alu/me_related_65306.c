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
 * File    : me_related_65306.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_alu.h"

// relation check for 9.99.125 [65306] Upstream_VLAN_Tag_Policer

// relation to 9.5.1 [011] Physical_path_termination_point_Ethernet_UNI
int
me_related_65306_011(struct me_t *me1, struct me_t *me2)
{
	return me_related_011_65306(me2, me1);
}

// relation to 9.5.5 [329] Virtual_Ethernet_interface_point
int
me_related_65306_329(struct me_t *me1, struct me_t *me2)
{
	return me_related_329_65306(me2, me1);
}

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
 * File    : me_related_011.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.5.1 [11] Physical path termination point Ethernet UNI

// relation to 9.99.116 [65297] UNI_Supplemental_1_V2
int
me_related_011_65297(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

// relation to 9.99.120 [65301] IP/MAC/VLAN_Address_Anti-Spoof_List_V2
int
me_related_011_65301(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

// relation to 9.99.124 [65305] Downstream_VLAN_Tag_Egress_Behavior_Supplemental_1
int
me_related_011_65305(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

// relation to 9.99.122 [65306] Upstream_VLAN_Tag_Policer
int
me_related_011_65306(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}



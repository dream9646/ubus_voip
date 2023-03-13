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
 * File    : me_related_011.c
 *
 ******************************************************************/

#include "me_related.h"

// relation check for 9.5.1 [11] Physical path termination point Ethernet UNI

// relation to 9.99.2 [242] EthernetUniExt
int
me_related_011_242(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

// relation to 9.99.9 [251] Ethernet_UNI_Extended_Configuration_Table
int
me_related_011_251(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}

// relation to 9.99.27 [65289] OamMep
int 
me_related_011_65289(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}
	// get attr1 (Layer 2 type)
	// 1:pptp eth uni, 3: 802.1p mapper service, 4: ip host, 5: gem iwtp, 6: gem mcast iwtp, 11: veip
	meinfo_me_attr_get(me2, 4, &attr1);
	meinfo_me_attr_get(me2, 5, &attr2);
	if ((unsigned char)(attr1.data)==1 && (unsigned short)(attr2.data)==me1->meid)
		return 1;
	else
		return 0;
}

// relation to 9.99.28 [65290] OamMip
int 
me_related_011_65290(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}
	// get attr1 (Layer 2 type)
	// 1:pptp eth uni, 3: 802.1p mapper service, 4: ip host, 5: gem iwtp, 6: gem mcast iwtp, 11: veip
	meinfo_me_attr_get(me2, 3, &attr1);
	meinfo_me_attr_get(me2, 4, &attr2);
	if ((unsigned char)(attr1.data)==1 && (unsigned short)(attr2.data)==me1->meid)
		return 1;
	else
		return 0;
}

int
me_related_11_65311(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	return (me1->meid==me2->meid);
}


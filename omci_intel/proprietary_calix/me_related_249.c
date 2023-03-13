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
 * File    : me_related_249.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"

// relation check for 9.99.6 [249] ExtL2VlanTagConfigData

// relation to 9.3.4 [047] MAC bridge port configuration data
int
me_related_249_047(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //determine me type
	/* 0. MAC bridge port configuration data 
	   1. 802.1p mapper service profile
	   2. Physical path termination point Ethernet UNI 
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point */
	
	if (attr1.data == 0 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int
me_related_249_130(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //determine me type
	/* 0. MAC bridge port configuration data 
	   1. 802.1p mapper service profile
	   2. Physical path termination point Ethernet UNI 
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point */
	
	if (attr1.data == 1 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.5.1 [011] Physical path termination point Ethernet UNI
int
me_related_249_011(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //determine me type
	/* 0. MAC bridge port configuration data 
	   1. 802.1p mapper service profile
	   2. Physical path termination point Ethernet UNI 
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point */
	
	if (attr1.data == 2 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.2.4 [266] GEM interworking termination point
int
me_related_249_266(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //determine me type
	/* 0. MAC bridge port configuration data 
	   1. 802.1p mapper service profile
	   2. Physical path termination point Ethernet UNI 
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point */
	
	if (attr1.data == 5 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.2.5 [281] Multicast GEM interworking termination point
int
me_related_249_281(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //determine me type
	/* 0. MAC bridge port configuration data 
	   1. 802.1p mapper service profile
	   2. Physical path termination point Ethernet UNI 
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point */
	
	if (attr1.data == 6 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.5.5 [329] Virtual Ethernet interface point
int
me_related_249_329(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr1); //determine me type
	/* 0. MAC bridge port configuration data 
	   1. 802.1p mapper service profile
	   2. Physical path termination point Ethernet UNI 
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Virtual Ethernet interface point */
	
	if (attr1.data == 10 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}


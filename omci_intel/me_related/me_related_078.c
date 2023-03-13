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
 * File    : me_related_078.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"
#include "env.h"

// relation check for 9.3.12 [078] VLAN tagging operation configuration data

// relation to 9.5.1 [011] Physical path termination point Ethernet UNI
int
me_related_078_011(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 0 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else if (attr1.data == 10 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int
me_related_078_130(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 2 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.4.12 [134] IP Host Config Data
int
me_related_078_134(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   1. Ip host config data
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 0 && 
		me1->meid == me2->meid)
	{
		return 1;
	} else if (attr1.data == 1 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.4 [047] MAC bridge port configuration data
int
me_related_078_047(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 3 && (unsigned short)(attr2.data) == me2->meid) {
		return 1;
	}
	/*if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU) {
		if (attr1.data == 3 && me1->meid == me2->meid) {
			return 1;
		}
	}*/

	return 0;
}

// relation to 9.2.4 [266] GEM interworking termination point
int
me_related_078_266(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 5 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.2.5 [281] Multicast GEM interworking termination point
int
me_related_078_281(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 6 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.5.5 [329] Virtual Ethernet interface point
int
me_related_078_329(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 4, &attr1); //determine me type
	/* 0. Physical path termination point Ethernet UNI (default, by meid)
	   2. 802.1p mapper service profile
	   3. MAC bridge port configuration data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   10. Physical path termination point Ethernet UNI
	   11. Virtual Ethernet interface point */
	meinfo_me_attr_get(me1, 5, &attr2); //associated me pointer
	
	if (attr1.data == 11 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

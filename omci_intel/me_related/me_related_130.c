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
 * File    : me_related_130.c
 *
 ******************************************************************/

#include <stdio.h>
#include "me_related.h"

// relation check for 9.3.10 [130] 802.1p mapper service profile

// relation to 9.5.1 [011] Physical path termination point Ethernet UNI
int
me_related_130_011(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 13, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI*/
	meinfo_me_attr_get(me1, 1, &attr2); //tp pointer
	
	if (attr1.data == 1 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.1 [45] MAC bridge service profile
int
me_related_130_045(struct me_t *me1, struct me_t *me2)
{
	struct me_t *me = NULL;
	struct meinfo_t *miptr = meinfo_util_miptr(47);

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if(me_related_047_130(me, me1)) {
			if(me_related_047_045(me, me2)) {
				return 1;
			} else {
				return 0;
			}
		}
	}
	return 0;
}

// relation to 9.2.4 [266] GEM interworking termination point
int
me_related_130_266(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;
	struct attr_value_t attr3;
	struct attr_value_t attr4;
	struct attr_value_t attr5;
	struct attr_value_t attr6;
	struct attr_value_t attr7;
	struct attr_value_t attr8;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 2, &attr1); //Interwork TP pointer for P-bit priority 0
	meinfo_me_attr_get(me1, 3, &attr2); //Interwork TP pointer for P-bit priority 1
	meinfo_me_attr_get(me1, 4, &attr3); //Interwork TP pointer for P-bit priority 2
	meinfo_me_attr_get(me1, 5, &attr4); //Interwork TP pointer for P-bit priority 3
	meinfo_me_attr_get(me1, 6, &attr5); //Interwork TP pointer for P-bit priority 4
	meinfo_me_attr_get(me1, 7, &attr6); //Interwork TP pointer for P-bit priority 5
	meinfo_me_attr_get(me1, 8, &attr7); //Interwork TP pointer for P-bit priority 6
	meinfo_me_attr_get(me1, 9, &attr8); //Interwork TP pointer for P-bit priority 7
	
	if ((unsigned short)(attr1.data) == me2->meid ||
		(unsigned short)(attr2.data) == me2->meid ||
		(unsigned short)(attr3.data) == me2->meid ||
		(unsigned short)(attr4.data) == me2->meid ||
		(unsigned short)(attr5.data) == me2->meid ||
		(unsigned short)(attr6.data) == me2->meid ||
		(unsigned short)(attr7.data) == me2->meid ||
		(unsigned short)(attr8.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.21 [301] Dot1ag default MD level
int
me_related_130_301(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.22 [302] Dot1ag MEP
int 
me_related_130_302(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me2, 1, &attr1);
	// get attr2 (Layer 2 type)
	// 0:mac bridge config data, 1:802.1p mapper service
	meinfo_me_attr_get(me2, 2, &attr2);
	if ((unsigned char)(attr2.data)==1 && (unsigned short)(attr1.data)==me1->meid)
		return 1;
	return 0;	
}

// relation to 9.3.25 [305] Dot1ag CFM stack
int
me_related_130_305(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	if (me1->meid == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.28 [310] Multicast subscriber config info
int 
me_related_130_310(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	
	// get attr1 (ME type) 
	// 0:mac bridge config data, 1:802.1p mapper service
	meinfo_me_attr_get(me1, 1, &attr);	
	if ((unsigned char)(attr.data)==1 && me1->meid==me2->meid)
		return 1;
	return 0;	
}

// relation to 9.3.28 [311] Multicast_subscriber_monitor
int 
me_related_130_311(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	
	// get attr1 (ME type) 
	// 0:mac bridge config data, 1:802.1p mapper service
	meinfo_me_attr_get(me1, 1, &attr);	
	if ((unsigned char)(attr.data)==1 && me1->meid==me2->meid)
		return 1;
	return 0;	
}

// relation to 9.5.5 [329] Virtual Ethernet interface point
int
me_related_130_329(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 13, &attr1); //determine me type
	/* 7. Virtual Ethernet interface point*/
	meinfo_me_attr_get(me1, 1, &attr2); //tp pointer
	
	if (attr1.data == 7 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.18 [298] Dot1_rate_limiter
int 
me_related_130_298(struct me_t *me1, struct me_t *me2)	
{
	return me_related_298_130(me2, me1);
}

// relation to 9.11.1 [277] Priority queue-G
int 
me_related_130_277(struct me_t *me1, struct me_t *me2)	
{
	return me_related_277_130(me2, me1);
}

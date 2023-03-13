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
 * File    : me_related_047.c
 *
 ******************************************************************/

#include <stdio.h>
#include "meinfo.h"
#include "me_related.h"
#include "env.h"

// relation check for 9.3.4 [047] MAC bridge port configuration data

// relation to 9.1.6 [006] Circuit Pack
int
me_related_047_006(struct me_t *me1, struct me_t *me2)
{
	return me_related_006_047(me2, me1);
}

// relation to 9.3.1 [045] MAC bridge service profile
int
me_related_047_045(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 1, &attr);	//bridge id pointer
	
	if ((unsigned short)(attr.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.5.1 [045] Physical path termination point Ethernet UNI
int
me_related_047_011(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI
	   3. 802.1p mapper service profile
	   4. IP host config data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   11. virtual_ethernet interface point*/
	meinfo_me_attr_get(me1, 4, &attr2); //tp pointer
	
	if (attr1.data == 1 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int
me_related_047_130(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI
	   3. 802.1p mapper service profile
	   4. IP host config data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   11. virtual_ethernet interface point*/
	meinfo_me_attr_get(me1, 4, &attr2); //tp pointer
	
	if (attr1.data == 3 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.4.12 [134] IP_host_config_data
int
me_related_047_134(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI
	   3. 802.1p mapper service profile
	   4. IP host config data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   11. virtual_ethernet interface point*/
	meinfo_me_attr_get(me1, 4, &attr2); //tp pointer
	
	if (attr1.data == 4 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}


// relation to 9.2.4 [266] GEM interworking termination point
int
me_related_047_266(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI
	   3. 802.1p mapper service profile
	   4. IP host config data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   11. virtual_ethernet interface point*/
	meinfo_me_attr_get(me1, 4, &attr2); //tp pointer
	
	if (attr1.data == 5 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.11.3 [280] GEM traffic descriptor
int
me_related_047_280(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 11, &attr1); //outbound td pointer
	meinfo_me_attr_get(me1, 12, &attr2); //inbound td pointer
	
	if ((unsigned short)(attr1.data) == me2->meid ||
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.2.5 [281] Multicast GEM interworking termination point
int
me_related_047_281(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI
	   3. 802.1p mapper service profile
	   4. IP host config data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   11. virtual_ethernet interface point*/
	meinfo_me_attr_get(me1, 4, &attr2); //tp pointer
	
	if (attr1.data == 6 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

// relation to 9.3.5 [048] MAC bridge port designation data
int
me_related_047_048(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.6 [049] MAC bridge port filter table data
int
me_related_047_049(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.7 [079] MAC bridge port filter preassign table
int
me_related_047_079(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.8 [050] MAC bridge port bridge table data
int
me_related_047_050(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.9 [052] MAC bridge port performance monitoring history data
int
me_related_047_052(struct me_t *me1, struct me_t *me2)
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

// relation to 9.3.11 [084] VLAN tagging filter data
int
me_related_047_084(struct me_t *me1, struct me_t *me2)
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
me_related_047_302(struct me_t *me1, struct me_t *me2)	
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
	if ((unsigned char)(attr2.data)==0 && (unsigned short)(attr1.data)==me1->meid)
		return 1;
	return 0;	
}

// relation to 9.3.28 [310] Multicast subscriber config info
int 
me_related_047_310(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	
	// get attr1 (ME type) 
	// 0:mac bridge config data, 1:802.1p mapper service
	meinfo_me_attr_get(me2, 1, &attr);	
	if ((unsigned char)(attr.data)==0 && me1->meid==me2->meid)
		return 1;
	return 0;	
}

// relation to 9.3.28 [311] Multicast_subscriber_monitor
int 
me_related_047_311(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	
	// get attr1 (ME type) 
	// 0:mac bridge config data, 1:802.1p mapper service
	meinfo_me_attr_get(me2, 1, &attr);	
	if ((unsigned char)(attr.data)==0 && me1->meid==me2->meid)
		return 1;
	return 0;	
}

// relation to 9.3.31 [321] Ethernet frame performance monitoring history data downstream
int 
me_related_047_321(struct me_t *me1, struct me_t *me2)	
{
	if (me1->meid == me2->meid)
		return 1;
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
		struct meinfo_t *miptr47 = meinfo_util_miptr(47);
		unsigned short tp_classid = miptr47->callback.get_pointer_classid_cb(me1, 4);
		unsigned short tp_meid = (unsigned short)meinfo_util_me_attr_data(me1, 4);
		if (tp_classid == 11 && tp_meid == me2->meid)
			return 1;
	}
	return 0;
}

// relation to 9.3.30 [322] Ethernet frame performance monitoring history data upstream
int 
me_related_047_322(struct me_t *me1, struct me_t *me2)	
{
	if (me1->meid == me2->meid)
		return 1;
	if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_CALIX) {
		struct meinfo_t *miptr47 = meinfo_util_miptr(47);
		unsigned short tp_classid = miptr47->callback.get_pointer_classid_cb(me1, 4);
		unsigned short tp_meid = (unsigned short)meinfo_util_me_attr_data(me1, 4);
		if (tp_classid == 11 && tp_meid == me2->meid)
			return 1;
	}
	return 0;
}

// relation to 9.5.5 [329] Virtual_Ethernet_interface_point
int 
me_related_047_329(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;
	
	if (me1 == NULL || me2 == NULL)
	{
		return 0;
	}

	meinfo_me_attr_get(me1, 3, &attr1); //determine me type
	/* 1. Physical path termination point Ethernet UNI
	   3. 802.1p mapper service profile
	   4. IP host config data
	   5. GEM interworking termination point
	   6. Multicast GEM interworking termination point
	   11. virtual_ethernet interface point*/
	meinfo_me_attr_get(me1, 4, &attr2); //tp pointer
	
	if (attr1.data == 11 && 
		(unsigned short)(attr2.data) == me2->meid)
	{
		return 1;
	} else {
		return 0;
	}
}

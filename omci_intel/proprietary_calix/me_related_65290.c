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
 * File    : me_related_65290.c
 *
 ******************************************************************/

#include "me_related.h"
#include "proprietary_calix.h"

// relation check for 9.9.28 [65290] OamMip

// relation to 9.5.1 [11] Physical path termination point Ethernet UNI
int 
me_related_65290_011(struct me_t *me1, struct me_t *me2)
{
	return me_related_011_65290(me2, me1);
}

// relation to 9.3.10 [130] 802.1p_mapper_service_profile
int 
me_related_65290_130(struct me_t *me1, struct me_t *me2)
{
	return me_related_130_65290(me2, me1);
}

// relation to 9.4.12 [134] IP_host_config_data
int 
me_related_65290_134(struct me_t *me1, struct me_t *me2)
{
	return me_related_134_65290(me2, me1);
}

// relation to 9.2.4 [266] GEM_IWTP
int 
me_related_65290_266(struct me_t *me1, struct me_t *me2)
{
	return me_related_266_65290(me2, me1);
}

// relation to 9.2.5 [281] Multicast_GEM_IWTP
int 
me_related_65290_281(struct me_t *me1, struct me_t *me2)
{
	return me_related_281_65290(me2, me1);
}

// relation to 9.5.5 [329] Virtual_Ethernet_interface_point
int 
me_related_65290_329(struct me_t *me1, struct me_t *me2)
{
	return me_related_329_65290(me2, me1);
}

// relation to 9.99.26 [65288] OamMeg
int 
me_related_65290_65288(struct me_t *me1, struct me_t *me2)	
{
	return me_related_65288_65290(me2, me1);
}

// relation to 9.99.37 [65301] OamMipStats
int 
me_related_65290_65301(struct me_t *me1, struct me_t *me2)	
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

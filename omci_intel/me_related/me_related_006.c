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
 * File    : me_related_006.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "util.h"
#include "me_related.h"

//relation check section: 9.1.6 6 Circuit_pack

// relation to 9.1.5 5 Cardholder
int me_related_006_005(struct me_t *me1, struct me_t *me2)
{
	return me_related_005_006(me2, me1);
}

// relation to 9.5.1 11 Physical path termination point Ethernet UNI
int 
me_related_006_011(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t uni_attr1;
	struct attr_value_t circuitpack_attr1;

	// get attr1 (type)
	meinfo_me_attr_get(me1, 1, &circuitpack_attr1);	

	// get attr1 (expected type)
	meinfo_me_attr_get(me2, 1, &uni_attr1);	

	if ((unsigned char)(uni_attr1.data)==0 ||
		(unsigned char)(uni_attr1.data)==(unsigned char)(circuitpack_attr1.data))
		return 1;

	return 0;	
}

// relation to 9.3.1 45 MAC bridge service profile
int
me_related_006_045(struct me_t *me1, struct me_t *me2)
{
	if (me1 == NULL || me2 == NULL)
		return 0;

	dbprintf(LOG_WARNING, "Unsure relation, classid1=6, meid1=%d, classid2=45, meid2=%d\n", me1->meid, me2->meid);
	return 1;
}

//relation to 9.3.4 47 MAC bridge port configuration data
static int
is_pon_side_circuitpack(unsigned char type)
{
	if (type>=243 && type<=254)
		return 1;
	return 0;
}
int me_related_006_047(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr_tp_type;
	struct attr_value_t attr_circuitpack_type;

	if (me1 == NULL || me2 == NULL)
		return 0;

	meinfo_me_attr_get(me1, 1, &attr_circuitpack_type);
	meinfo_me_attr_get(me2, 3, &attr_tp_type);

	switch (attr_tp_type.data) {
	case 1:	// Physical path termination point Ethernet UNI
	case 2:	// Interworking VCC termination point
	case 7:	// Physical path termination point xDSL UNI part 1
	case 8:	// Physical path termination point VDSL UNI
	case 9:	// Ethernet flow termination point
	case 10:// Physical path termination point 802.11 UNI
		if (!is_pon_side_circuitpack((unsigned char)attr_circuitpack_type.data))	// uni
			return 1;
		break;
	case 3:	// 802.1p mapper service profile
	case 5:	// GEM interworking termination point
	case 6:	// Multicast GEM interworking termination point
		if (is_pon_side_circuitpack((unsigned char)attr_circuitpack_type.data))	// wan
			return 1;
		break;
	case 4:	// IP host config data
		return 0;
		break;
	}
	return 0;
}

// relation to 9.1.1 256 ONT-G
int me_related_006_256(struct me_t *me1, struct me_t *me2)
{
	return 1;	//related
}

// relation to 9.11.2 [278] Traffic scheduler-G
int 
me_related_006_278(struct me_t *me1, struct me_t *me2)	
{
	unsigned char circuitpack_slot=(me1->meid)&0xff;
	unsigned char scheduler_slot=(me2->meid)>>8;

	return (scheduler_slot==circuitpack_slot);
}


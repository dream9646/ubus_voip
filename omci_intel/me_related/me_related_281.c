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
 * File    : me_related_281.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "me_related.h"

//relation check section: 9.2.5 281 Multicast_GEM_interworking_termination_point

//relation to 9.2.3 268 GEM_port_network_CTP
int me_related_281_268(struct me_t *me1, struct me_t *me2)
{

	//281's attr1 (GEM_port_network_CTP_connectivity_pointer) == 268's meid
	if( (unsigned short)me1->attr[1].value.data == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.3.1 045 MAC_bridge_service_profile
int me_related_281_045(struct me_t *me1, struct me_t *me2)
{
	//281's attr2(Interworking_option) == 1 && (attr3 (Service_profile_pointer) == 045's meid)
	if( (me1->attr[2].value.data == 1) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.4.1 068 IP_router_service_profile
int me_related_281_068(struct me_t *me1, struct me_t *me2)
{
	//281's attr2(Interworking_option) == 3 && (attr3 (Service_profile_pointer) == 068's meid)
	if( (me1->attr[2].value.data == 3) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.3.10 130 802.1p_mapper_service_profile
int me_related_281_130(struct me_t *me1, struct me_t *me2)
{
	//281's attr2(Interworking_option) == 5 && (attr3 (Service_profile_pointer) == 130's meid)
	if( (me1->attr[2].value.data == 5) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.2.7 272 GAL_Ethernet_profile
int me_related_281_272(struct me_t *me1, struct me_t *me2)
{
	//281's attr7 (GAL_profile_pointer) == 272's meid
	if((unsigned short)me1->attr[7].value.data == me2->meid)
		return 1;	//related
	else
		return 0;	//not related
}


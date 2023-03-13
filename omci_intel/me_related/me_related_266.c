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
 * File    : me_related_266.c
 *
 ******************************************************************/

#include <stdlib.h>
#include "me_related.h"

//relation check section: 9.2.4 266 GEM_interworking_termination_point

//relation to 9.2.3 268 GEM_port_network_CTP
int me_related_266_268(struct me_t *me1, struct me_t *me2)
{

	//266's attr1 (GEM_port_network_CTP_connectivity_pointer) == 268's meid
	if( (unsigned short)me1->attr[1].value.data == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}


//relation to 9.8.3 021 CES_service_profile_G
int me_related_266_021(struct me_t *me1, struct me_t *me2)
{
	//266's attr2 (Interworking_option) == 0 && (attr3 (Service_profile_pointer) == 021's meid)
	if( (me1->attr[2].value.data == 0) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.3.1 045 MAC_bridge_service_profile
int me_related_266_045(struct me_t *me1, struct me_t *me2)
{
	//266's attr2 (Interworking_option) == 1 && (attr3 (Service_profile_pointer) == 045's meid)
	if( (me1->attr[2].value.data == 1) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.4.1 068 IP_router_service_profile
int me_related_266_068(struct me_t *me1, struct me_t *me2)
{
	//266's attr2 (Interworking_option) == 3 && (attr3 (Service_profile_pointer) == 045's meid)
	if( (me1->attr[2].value.data == 3) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.13.7 128 Video_return_path_service_profile
int me_related_266_128(struct me_t *me1, struct me_t *me2)
{
	//266's attr2 (Interworking_option) == 4 && (attr3 (Service_profile_pointer) == 128's meid)
	if( (me1->attr[2].value.data == 4) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.3.10 130 802.1p_mapper_service_profile
int me_related_266_130(struct me_t *me1, struct me_t *me2)
{
	//266's attr2 (Interworking_option) == 5 && (attr3 (Service_profile_pointer) == 130's meid)
	if( (me1->attr[2].value.data == 5) && ((unsigned short)me1->attr[3].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.5.1 011 Physical_path_termination_point_Ethernet_UNI
int me_related_266_011(struct me_t *me1, struct me_t *me2)
{
	//266's attr4 (Interworking_termination_point_pointer) == 011's meid
	if( (unsigned short)me1->attr[4].value.data == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.8.1 012 Physical_path_termination_point_CES_UNI
int me_related_266_012(struct me_t *me1, struct me_t *me2)
{
	//266's attr4 (Interworking_termination_point_pointer) == 012's meid
	if( (unsigned short)me1->attr[4].value.data == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.8.2 013 Logical_N_x_64kbit/s_sub_port_connection_termination_point
int me_related_266_013(struct me_t *me1, struct me_t *me2)
{
	//266's attr4 (Interworking_termination_point_pointer) == 013's meid
	if( (unsigned short)me1->attr[4].value.data == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.8.10 294 TU_CTP
int me_related_266_294(struct me_t *me1, struct me_t *me2)
{
	//266's attr4 (Interworking_termination_point_pointer) == 294's meid
	if( (unsigned short)me1->attr[4].value.data == me2->meid )
		return 1;	//related
	else
		return 0;	//not related
}


//relation to 9.2.9 271 GAL_TDM_profile
int me_related_266_271(struct me_t *me1, struct me_t *me2)
{
	//266's attr2 (Interworking_option)==0 && (266's attr7 (GAL_profile_pointer) == 271's meid )
	if( (me1->attr[2].value.data == 0) && ((unsigned short)me1->attr[7].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}

//relation to 9.2.7 272 GAL_Ethernet_profile
int me_related_266_272(struct me_t *me1, struct me_t *me2)
{
	unsigned char option;

	option=me1->attr[2].value.data;
	//266's attr2 (Interworking_option)==1-5 && ( 266's attr7 (GAL_profile_pointer) == 272's meid )
	if( (option >=1 && option <=5) && ((unsigned short)me1->attr[7].value.data == me2->meid) )
		return 1;	//related
	else
		return 0;	//not related
}


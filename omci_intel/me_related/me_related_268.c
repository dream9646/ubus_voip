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
 * File    : me_related_268.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "me_related.h"
#include "hwresource.h"
#include "omciutil_misc.h"
#include "util.h"

//relation check section: 9.2.3 268 GEM_port_network_CTP

// relation to direction downstream
static int 
me_related_268_downstream(struct me_t *me1)	
{
	struct attr_value_t attr;
	
	meinfo_me_attr_get(me1, 3, &attr);
	return (((unsigned char)(attr.data)) == 2); // msb==2 means downstream
}

// relation to direction upstream
static int 
me_related_268_upstream(struct me_t *me1)	
{
	struct attr_value_t attr;
	
	meinfo_me_attr_get(me1, 3, &attr);
	return (((unsigned char)(attr.data)) == 1); // msb==1 means upstream
}

//relation to 9.2.2 262 T_CONT
int me_related_268_262(struct me_t *me1, struct me_t *me2)
{
	//268's attr2 (T_CONT_pointer) == 262's meid
	if( (unsigned short)me1->attr[2].value.data == me2->meid )
		return 1;	//related
	//256's attr4 == 1(Rate controlled) && (268's attr4 (Traffic_management_pointer_for_upstream) == 262's meid)
	if (omciutil_misc_traffic_management_option_is_rate_control()) 
		return ((unsigned short)me1->attr[4].value.data == me2->meid); //pointer to T-CONT
	return 0;	//not related
}


//relation to 9.2.6 267 GEM_port_performance_monitoring_history_data
int me_related_268_267(struct me_t *me1, struct me_t *me2)
{
	return me_related_267_268(me2, me1);
}

//relation to 9.11.1 277 Priority_queue_G
int me_related_268_277(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr;

	if (me_related_277_upstream(me2)) {
		//upstream pq
		//256's attr4 == 1 (Rate controlled) 
		if (omciutil_misc_traffic_management_option_is_rate_control()) { 
			struct me_t *me_tcont = meinfo_me_get_by_meid(262, (unsigned short)me1->attr[4].value.data);
			struct me_t *private_me_gem=hwresource_public2private(me1);
			if ((me_tcont == NULL) || (private_me_gem == NULL))
				return 0;			
			// pq pointer of igem me
			meinfo_me_attr_get(private_me_gem, 5, &attr);	
			// igem pq pointer points to pq and && gem/pq belong to same me_tcont
			return (attr.data == me2->meid && me_related_277_262(me2, me_tcont));
		} else { 
		//256's attr4 == 0 or 2 (Priority controlled or Priority and rate controlled)
			// (268's attr4 (Traffic_management_pointer_for_upstream) == 277's meid)
			return ( (unsigned short)me1->attr[4].value.data == me2->meid );
		}
	} else {
		//downstream pq
		//268's attr7 (Priority_queue_pointer_for_downstream) == 277's meid
		return ( (unsigned short)me1->attr[7].value.data == me2->meid );
	}
}

//relation to 9.11.3 280 GEM_traffic_descriptor
int me_related_268_280(struct me_t *me1, struct me_t *me2)
{
	struct attr_value_t attr1;
	struct attr_value_t attr2;

	if (me1 == NULL || me2 == NULL)
		return 0;

	meinfo_me_attr_get(me1, 5, &attr1); //upstream td pointer
	meinfo_me_attr_get(me1, 9, &attr2); //downstream td pointer
	
	return ((unsigned short)(attr1.data) == me2->meid ||
		(unsigned short)(attr2.data) == me2->meid);
}

//relation to 9.5.5 329 Virtual_Ethernet_interface_point
int me_related_268_329(struct me_t *me1, struct me_t *me2)
{
	return me_related_329_268(me2, me1);
}

// relation to 9.90.1 [401] Downstream_pseudo_obj
int 
me_related_268_401(struct me_t *me1, struct me_t *me2)	
{
	return (me_related_268_downstream(me1));
}

// relation to 9.90.2 [402] Upstream_pseudo_obj
int 
me_related_268_402(struct me_t *me1, struct me_t *me2)	
{
	return (me_related_268_upstream(me1));
}

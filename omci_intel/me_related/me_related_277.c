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
 * File    : me_related_277.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <arpa/inet.h>
#include "meinfo.h"
#include "me_related.h"
#include "util.h"

// relation check for 9.11.1 [277] Priority queue-G

static unsigned short
get_tcont_meid_by_ts_pointer(struct me_t *pq_me)
{
	struct attr_value_t attr;
	unsigned short ts_meid, tcont_meid;
	int i;

	// get attr7 (ts pointer)
	meinfo_me_attr_get(pq_me, 7, &attr);	
	ts_meid=(unsigned short)attr.data;	

	// follow the ts pointer to find the tcont that owns this pq
	for (i=0; i<10; i++) {
		struct me_t *ts_me=meinfo_me_get_by_meid(278, ts_meid);			
		if (ts_me==NULL)
			return 0;
		// tcont pointer in traffic scheduler
		tcont_meid=meinfo_util_me_attr_data(ts_me, 1);
		if (tcont_meid!=0)
			return tcont_meid;		// tcont found
		// ts pointer in traffic scheduler
		ts_meid=meinfo_util_me_attr_data(ts_me, 2);
		if (ts_meid==0)
			return 0;	// no further ts available
	}
	return 0;	// too many level of ts
}

static unsigned short
get_tcont_meid_by_related_port(struct me_t *pq_me)
{
	struct attr_value_t attr;
	unsigned int *related_port;
	unsigned short tcont_meid;

	memset(&attr, 0x00, sizeof(struct attr_value_t));
	// get attr6 (related port)
	meinfo_me_attr_get(pq_me, 6, &attr);	
	related_port=(unsigned int *)attr.ptr;
	*related_port=htonl(*related_port);
	tcont_meid = (*related_port >>16) & 0xffff; // first 2 byte == tcont meid
	meinfo_me_attr_release(pq_me->classid, 6, &attr);
	return tcont_meid;
}
	
// relation to 9.2.2 [262] T-CONT
int 
me_related_277_262(struct me_t *me1, struct me_t *me2)	
{
	unsigned short tcont_meid;

	if (me_related_277_upstream(me1)) {	// check if upstream, as pq in downstream won't relate to any tcont
		if (omci_env_g.pq_tcont_relation== ENV_PQ_TCONT_RELATION_BY_TS_POINTER) {
			tcont_meid=get_tcont_meid_by_ts_pointer(me1);
			if (tcont_meid==0)
				tcont_meid=get_tcont_meid_by_related_port(me1);
		} else {
			tcont_meid=get_tcont_meid_by_related_port(me1);
			if (tcont_meid==0)
				tcont_meid=get_tcont_meid_by_ts_pointer(me1);
		}
		if (tcont_meid == me2->meid)
			return 1;
	}
	return 0;
}

// relation to 9.5.1 [11] Physical path termination point Ethernet UNI
int 
me_related_277_011(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	unsigned int *related_port;
	unsigned char uni_slot_id;
	unsigned char uni_port_id;

	memset(&attr, 0x00, sizeof(struct attr_value_t));

	// get attr6 (related port)
	meinfo_me_attr_get(me1, 6, &attr);	
	related_port=(unsigned int *)attr.ptr;
	*related_port=htonl(*related_port);
	uni_slot_id = (*related_port>>24)&0xff;
	uni_port_id = (*related_port>>16)&0xff;

	meinfo_me_attr_release(me1->classid, 6, &attr);

	if (!me_related_277_upstream(me1) &&
		(uni_slot_id == me2->meid>>8) &&
		(uni_port_id == (me2->meid&0xff))) {	// dowstream
		return 1;
	} else {	// pq in upstream won't relate to any ethernet pptp uni
		return 0;
	}
}

// relation to 9.11.2 [278] Traffic scheduler-G
int 
me_related_277_278(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;

	// get attr7 (Traffic scheduler-G pointer)
	meinfo_me_attr_get(me1, 7, &attr);
	
	if(attr.data==0) {
#if 0
		// null TS pointer, refer to related_port's TS pointer
		unsigned int *related_port;
		unsigned short tcont_me1, tcont_me2, priority_me1, priority_me2;
		
		memset(&attr, 0x00, sizeof(struct attr_value_t));
		meinfo_me_attr_get(me1, 6, &attr);
		related_port=(unsigned int *)attr.ptr;
		*related_port=htonl(*related_port);
		tcont_me1 = (*related_port >> 16) & 0xffff; // first 2 byte == tcont meid
		priority_me1 = (*related_port >> 0) & 0xffff; // last 2 byte == priority
		
		memset(&attr, 0x00, sizeof(struct attr_value_t));
		meinfo_me_attr_get(me2, 1, &attr);
		tcont_me2 = (unsigned short)attr.data;
		memset(&attr, 0x00, sizeof(struct attr_value_t));
		meinfo_me_attr_get(me2, 4, &attr);
		priority_me2 = (unsigned char)attr.data;
		
		return ((tcont_me1==tcont_me2) && (priority_me1==priority_me2));
#else
		// null TS pointer, refer to initial config value
		struct meinfo_t *miptr=meinfo_util_miptr(277);
		struct meinfo_instance_def_t *instance_def_ptr;
		list_for_each_entry(instance_def_ptr, &miptr->config.meinfo_instance_def_list, instance_node) {
			if (instance_def_ptr->default_meid == me1->meid) {
				attr.data = instance_def_ptr->custom_default_value[7].data;
				break;
			}
		}
#endif
	}
	
	return ((unsigned short)attr.data==me2->meid);
}

// relation to 9.5.5 [329] Virtual Ethernet Interface Point
int 
me_related_277_329(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	unsigned int *related_port;
	unsigned char veip_slot_id;
	unsigned char veip_port_id;

	memset(&attr, 0x00, sizeof(struct attr_value_t));

	// get attr6 (related port)
	meinfo_me_attr_get(me1, 6, &attr);	
	related_port=(unsigned int *)attr.ptr;
	*related_port=htonl(*related_port);
	veip_slot_id = (*related_port>>24)&0xff;
	veip_port_id = (*related_port>>16)&0xff;
	meinfo_me_attr_release(me1->classid, 6, &attr);

	// veip meid starts from 1, 
	// so slot will be 0 and happen to be different than real uni
	if (!me_related_277_upstream(me1) &&
		(veip_slot_id == me2->meid>>8) &&	
		(veip_port_id == (me2->meid&0xff))) {	// dowstream
		return 1;		
	} else {	// pq in upstream won't relate to any uni/veip
		return 0;
	}
}

// relation to 9.3.10 [130] 802.1p mapper service profile
int 
me_related_277_130(struct me_t *me1, struct me_t *me2)	
{
	struct meinfo_t *miptr = meinfo_util_miptr(268);
	struct me_t *meptr_268 = NULL;
	
	list_for_each_entry(meptr_268, &miptr->me_instance_list, instance_node) {
		if(me_related(meptr_268, me1)) { // 277->268
			struct me_t *meptr_266 = NULL;
			miptr = meinfo_util_miptr(266);
			list_for_each_entry(meptr_266, &miptr->me_instance_list, instance_node) {
				if(me_related(meptr_266, meptr_268)) { // 268->266
					if(me_related(me2, meptr_266)) // 266->130
						return 1;
					else
						return 0;
				}
			}
		}
	}

	return 0;
}

// relation to 9.90.1 [401] Downstream_pseudo_obj
int 
me_related_277_401(struct me_t *me1, struct me_t *me2)	
{
	return (me_related_277_upstream(me1)==0);
}

// relation to 9.90.2 [402] Upstream_pseudo_obj
int 
me_related_277_402(struct me_t *me1, struct me_t *me2)	
{
	return (me_related_277_upstream(me1));
}

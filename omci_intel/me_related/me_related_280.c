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
 * File    : me_related_280.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.11.3 [280] Traffic_descriptor

// relation to 9.90.1 [401] Downstream pseudo obj
int 
me_related_280_401(struct me_t *me1, struct me_t *me2)	
{
	struct meinfo_t *miptr=meinfo_util_miptr(268);
	struct attr_value_t attr;

	//find class 268 GEM_port_network_CTP, attr 9 pointer to me1
	struct me_t *me;
	
	if (miptr==NULL)
		return 0;
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		meinfo_me_attr_get(me, 9, &attr);
		if (attr.data == me1->meid)
		{
			return 1;
		}
	}

	return 0;
}

// relation to 9.90.2 [402] Upstream pseudo obj
int 
me_related_280_402(struct me_t *me1, struct me_t *me2)	
{
	struct meinfo_t *miptr=meinfo_util_miptr(268);
	struct attr_value_t attr;

	//find class 268 GEM_port_network_CTP, attr 5 pointer to me1
	struct me_t *me;
	
	if (miptr==NULL)
		return 0;
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		meinfo_me_attr_get(me, 5, &attr);
		if (attr.data == me1->meid)
		{
			return 1;
		}
	}

	return 0;
}

// relation to 9.90.3 [403] Inbound pseudo obj
int 
me_related_280_403(struct me_t *me1, struct me_t *me2)	
{
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct attr_value_t attr;

	//find class 47 MAC_bridge_port_configuration_data, attr 12 pointer to me1
	struct me_t *me;
	
	if (miptr==NULL)
		return 0;
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		meinfo_me_attr_get(me, 12, &attr);
		if (attr.data == me1->meid)
		{
			return 1;
		}
	}

	return 0;
}

// relation to 9.90.4 [404] Outbound pseudo obj
int 
me_related_280_404(struct me_t *me1, struct me_t *me2)	
{
	struct meinfo_t *miptr=meinfo_util_miptr(47);
	struct attr_value_t attr;

	//find class 47 MAC_bridge_port_configuration_data, attr 11 pointer to me1
	struct me_t *me;
	
	if (miptr==NULL)
		return 0;
	
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		meinfo_me_attr_get(me, 11, &attr);
		if (attr.data == me1->meid)
		{
			return 1;
		}
	}

	return 0;
}

// relation to 9.3.18 [298] Dot1_rate_limiter
int me_related_280_298(struct me_t *me1, struct me_t *me2)
{
	return me_related_298_280(me2, me1);	
}



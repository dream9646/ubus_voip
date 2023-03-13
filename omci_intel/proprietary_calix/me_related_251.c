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
 * File    : me_related_251.c
 *
 ******************************************************************/

#include "me_related.h"
#include "omciutil_misc.h"
#include "proprietary_calix.h"

// relation check for 9.9.9 [251] Ethernet_UNI_Extended_Configuration_Table

// relation to 9.5.1 [11] Physical path termination point Ethernet UNI
int 
me_related_251_011(struct me_t *me1, struct me_t *me2)
{
	return me_related_011_251(me2, me1);
}

// relation to 9.3.1 [45] MAC_bridge_service_profile
int 
me_related_251_045(struct me_t *me1, struct me_t *me2)
{
	int found = 0;
	struct me_t *meptr = NULL;
	struct meinfo_t *miptr = meinfo_util_miptr(11);
	if(miptr) {
		list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
			if(me_related_251_011(me1, meptr)) {
				found = 1;
				break;
			}
		}
	}
	if(!found) {
		miptr=meinfo_util_miptr(329);
		if(miptr) {
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
				if(me_related_251_329(me1, meptr)) {
					found = 1;
					break;
				}
			}
		}
	}
	if(found && meptr) {
		struct me_t *bport_me = omciutil_misc_find_me_related_bport(meptr);
		return (bport_me && me_related_047_045(bport_me, me2)) ? 1 : 0;
	}
	return 0;
}

// relation to 9.5.5 [329] Virtual Ethernet interface point
int 
me_related_251_329(struct me_t *me1, struct me_t *me2)
{
	return me_related_329_251(me2, me1);
}


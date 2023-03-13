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
 * File    : me_related_300.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.20 [300] Dot1ag maintenance association

// relation to 9.3.11 [84] VLAN tagging filter data
int 
me_related_300_084(struct me_t *me1, struct me_t *me2)	
{
	int i, is_empty = 1;

	if(me1->attr[6].value.ptr == NULL || me2->attr[1].value.ptr == NULL || me2->attr[3].value.data == 0)
		return 0;
	
	// No relation if Associated_VLANs is all zeros
	for(i=0; i<12; i++) {
		if(util_bitmap_get_value(me1->attr[6].value.ptr, 24*8, i*16+4, 12)!=0) {
			is_empty = 0;
			break;
		}
	}
	if(is_empty) return 0;
	
	// If one of VLAN in ME84 VLAN_filter_list equals to ME300 Associated_VLANs, return 1
	for(i=0; i<((unsigned char) me2->attr[3].value.data); i++) {
		int j;
		unsigned short vlan_filter = (unsigned short) util_bitmap_get_value(me2->attr[1].value.ptr, 24*8, i*16+4, 12);
		for(j=0; j<12; j++) {
			unsigned short assoc_vlan = (unsigned short) util_bitmap_get_value(me1->attr[6].value.ptr, 24*8, j*16+4, 12);
			if(vlan_filter == assoc_vlan)
				return 1;
		}
	}

	return 0;	
}

// relation to 9.3.19 [299] Dot1ag maintenance domain
int 
me_related_300_299(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;

	// get attr1 (MD_pointer) 
	meinfo_me_attr_get(me1, 1, &attr);

	if ((unsigned short)(attr.data)==me2->meid)
	{
		return 1;
	}

	return 0;	
}

// relation to 9.3.22 [302] Dot1ag MEP
int 
me_related_300_302(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;

	// get attr3 (MA_pointer) 
	meinfo_me_attr_get(me2, 3, &attr);

	if ((unsigned short)(attr.data)==me1->meid)
	{
		return 1;
	}

	return 0;	
}

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
 * File    : me_related_298.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "me_related.h"
#include "util.h"

// relation check for 9.3.18 [298] Dot1_rate_limiter

// relation to 9.3.1 [45] MAC_bridge_service_profile (after G.984.4 amd2)
int 
me_related_298_045(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	unsigned char tp_type;
	
	// get attr2 (TP_type) 
	meinfo_me_attr_get(me1, 2, &attr);
	tp_type=attr.data;

	// get attr1 (Parent_ME_pointer) 
	meinfo_me_attr_get(me1, 1, &attr);

	if ( tp_type == 1 && (unsigned short)(attr.data) == me2->meid)
		return 1;
	return 0;	
}

// relation to 9.3.2 [46] MAC_bridge_configuration_data (before G.984.4 amd2)
int 
me_related_298_046(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	unsigned char tp_type;
	
	// get attr2 (TP_type) 
	meinfo_me_attr_get(me1, 2, &attr);
	tp_type=attr.data;

	// get attr1 (Parent_ME_pointer) 
	meinfo_me_attr_get(me1, 1, &attr);

	if ( tp_type == 1 && (unsigned short)(attr.data) == me2->meid)
		return 1;
	return 0;	
}

// relation to 9.3.10 [130] 802.1p_mapper_service_profile
int 
me_related_298_130(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	unsigned char tp_type;
	
	// get attr2 (TP_type) 
	meinfo_me_attr_get(me1, 2, &attr);
	tp_type=attr.data;

	// get attr1 (Parent_ME_pointer) 
	meinfo_me_attr_get(me1, 1, &attr);

	if ( tp_type == 2 && (unsigned short)(attr.data) == me2->meid) {
		return 1;
	}

	return 0;	
}

// relation to 9.11.3 [280] Traffic_descriptor
int 
me_related_298_280(struct me_t *me1, struct me_t *me2)	
{
	struct attr_value_t attr;
	
	// get attr3,4,5, one of them should == me2->meid
	meinfo_me_attr_get(me1, 3, &attr);
	if( (unsigned short)(attr.data) == me2->meid )
		return 1;
	meinfo_me_attr_get(me1, 4, &attr);
	if( (unsigned short)(attr.data) == me2->meid )
		return 1;
	meinfo_me_attr_get(me1, 5, &attr);
	if( (unsigned short)(attr.data) == me2->meid )
		return 1;

	return 0;	
}


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
 * Module  : meinfo_cb
 * File    : meinfo_cb_306.c
 *
 ******************************************************************/

#include <string.h>

#include "meinfo_cb.h"
#include "util.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"

//classid 306 9.3.26 Dot1ag chassis-management info

static int 
meinfo_cb_306_set(struct me_t *me, unsigned char attr_mask[2])
{	
	struct attr_value_t attr;
	extern cfm_chassis_t cfm_chassis_g;
	
	if (util_attr_mask_get_bit(attr_mask, 1)) { // Chassis ID length
		meinfo_me_attr_get(me, 1, &attr);
		cfm_chassis_g.chassis_id_length = (unsigned char)attr.data;
	}
	
	if (util_attr_mask_get_bit(attr_mask, 2)) { // Chassis ID subtype
		meinfo_me_attr_get(me, 2, &attr);
		cfm_chassis_g.chassis_id_subtype = (unsigned char)attr.data;
	}
	
	if (util_attr_mask_get_bit(attr_mask, 3)) { // Chassis ID part 1
		attr.ptr = meinfo_util_me_attr_ptr(me , 3);
		strncpy((char *) cfm_chassis_g.chassis_id, (char *) attr.ptr, 25);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 4)) { // Chassis ID part 2
		attr.ptr = meinfo_util_me_attr_ptr(me , 4);
		strncpy((char *) cfm_chassis_g.chassis_id+25, (char *) attr.ptr, 25);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 5)) { // Management address domain length
		meinfo_me_attr_get(me, 5, &attr);
		cfm_chassis_g.mgmtaddr_domain_length = (unsigned char)attr.data;
	}
	
	if (util_attr_mask_get_bit(attr_mask, 6)) { // Management address domain 1
		attr.ptr = meinfo_util_me_attr_ptr(me , 6);
		strncpy((char *) cfm_chassis_g.mgmtaddr_domain, (char *) attr.ptr, 25);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 7)) { // Management address domain 2
		attr.ptr = meinfo_util_me_attr_ptr(me , 7);
		strncpy((char *) cfm_chassis_g.mgmtaddr_domain+25, (char *) attr.ptr, 25);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 8)) { // Management address length
		meinfo_me_attr_get(me, 8, &attr);
		cfm_chassis_g.mgmtaddr_length = (unsigned char)attr.data;
	}
	
	if (util_attr_mask_get_bit(attr_mask, 9)) { // Management address 1
		attr.ptr = meinfo_util_me_attr_ptr(me , 9);
		strncpy((char *) cfm_chassis_g.mgmtaddr, (char *) attr.ptr, 25);
	}
	
	if (util_attr_mask_get_bit(attr_mask, 10)) { // Management address 2
		attr.ptr = meinfo_util_me_attr_ptr(me , 10);
		strncpy((char *) cfm_chassis_g.mgmtaddr+25, (char *) attr.ptr, 25);
	}
	
	return 0;
}

struct meinfo_callback_t meinfo_cb_306 = {
	.set_cb	= meinfo_cb_306_set,
};

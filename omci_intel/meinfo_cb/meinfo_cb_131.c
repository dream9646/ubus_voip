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
 * File    : meinfo_cb_131.c
 *
 ******************************************************************/

#include <string.h>
#include "meinfo_cb.h"
#include "me_related.h"
#include "util.h"

//classid 131 9.12.2 OLT_G

static int 
meinfo_cb_131_create(struct me_t *me, unsigned char attr_mask[2])
{	
	if (me == NULL)
		return -1;
	return 0;
}


static int
meinfo_cb_131_config_vendor_id_replace(struct me_t *me)
{
	struct meinfo_t *miptr;
	struct meinfo_instance_def_t *instance_def_ptr;
	char ostr[128];
	int ret;


	miptr = meinfo_util_miptr(131); 
	if (miptr != NULL && miptr->config.is_inited && miptr->config.support != ME_SUPPORT_TYPE_NOT_SUPPORTED)
	{
		instance_def_ptr = list_first_entry(&miptr->config.meinfo_instance_def_list, struct meinfo_instance_def_t, instance_node);

		meinfo_conv_attr_to_string(131, 1, &instance_def_ptr->custom_default_value[1], ostr, 128);
		dbprintf(LOG_DEBUG, "%2d: %s: %s\n", 1, miptr->attrinfo[1].name, ostr);

		memset(ostr, 0x0, sizeof(ostr));
		memcpy(ostr, me->attr[1].value.ptr, 4);
		ret = meinfo_conv_string_to_attr(131, 1, &instance_def_ptr->custom_default_value[1], ostr);
		if (ret<0) {
			dbprintf(LOG_ERR, 
				 "classid=%u, instance=%d, attr=%d, err=%d\n",
				 131, 0, 1, ret);
			return -1;
		}
	}

	return 0;
}


static int 
meinfo_cb_131_set(struct me_t *me, unsigned char attr_mask[2])
{
	if (me == NULL)
		return -1;

	// for GWS case, 
	// Adtran OLT will trigger mib reset after mib set ==> we need to keep OLT_vendor_id as ADTR for sequence me instance creation.
	if ( omci_env_g.classf_add_default_rules_by_me_84 == 2 &&
		 util_attr_mask_get_bit(attr_mask, 1) &&
		 strncmp(me->attr[1].value.ptr, "ADTR", 4) == 0 )
	{
		return meinfo_cb_131_config_vendor_id_replace(me);
	}

	return 0;
}

struct meinfo_callback_t meinfo_cb_131 = {
	.create_cb			= meinfo_cb_131_create,
	.set_cb				= meinfo_cb_131_set,
};


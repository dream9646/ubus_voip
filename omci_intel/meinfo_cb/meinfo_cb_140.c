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
 * File    : meinfo_cb_140.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "util.h"

//classid 140 9.9.12 Call_control_performance_monitoring_history_data

static int 
meinfo_cb_140_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct meinfo_t *miptr=NULL;
	struct attr_value_t attr;
	unsigned char attr_order;
	unsigned char attr_mask2[2]={0x3e,0x00};

	if (me == NULL || omci_env_g.voip_enable == ENV_VOIP_DISABLE)
	{
		return -1;
	}
	
	if ((miptr=meinfo_util_miptr(me->classid)) == NULL) {
		dbprintf(LOG_ERR, "Fail, because of miptr NULL!\n");
		return -1;
	}

	if( miptr->hardware.get_hw )
		miptr->hardware.get_hw(me, attr_mask2);	//update attr > 2

	attr.data=0; 
	attr.ptr=NULL;

	for( attr_order=3; attr_order <= miptr->attr_total; attr_order++) {
		//set current and history to 0
		meinfo_me_attr_set_pm_current_data(me, attr_order, attr);
		meinfo_me_attr_set(me, attr_order, &attr, 0);
	}

	return 0;
}

struct meinfo_callback_t meinfo_cb_140 = {
	.create_cb			= meinfo_cb_140_create,
};


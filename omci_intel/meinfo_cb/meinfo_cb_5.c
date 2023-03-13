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
 * File    : meinfo_cb_5.c
 *
 ******************************************************************/

#include "meinfo.h"
#include "meinfo_cb.h"
#include "me_related.h"
#include "omcimsg.h"
#include "alarmtask.h"
#include "util.h"

#define CARDHOLDER_MAX	16

static int is_plugin_type_invalid[CARDHOLDER_MAX]={0};

//classid 5 9.1.5 Cardholder

static int
meinfo_cb_5_create(struct me_t *me, unsigned char attr_mask[2])
{
	struct attr_value_t attr;
	struct meinfo_t *miptr = NULL;
	struct me_t *meptr = NULL;
	int port_count = 0;
	unsigned char unit_type = (unsigned char)meinfo_util_me_attr_data(me, 2);
	
	if (me == NULL || 
		(unit_type == 32 && omci_env_g.voip_enable == ENV_VOIP_DISABLE))
	{
		return -1;
	}

	if (me->instance_num < CARDHOLDER_MAX)
		is_plugin_type_invalid[me->instance_num]=0;
	
	// Expected_port_count
	switch (unit_type) {
		case 22:	// 10BASE-T
		case 23:	// 100BASE-T
		case 24:	// 10-100BASE-T
		case 34:	// Gigabit optical Ethernet
		case 47:	// Ethernet BASE-T
			miptr= meinfo_util_miptr(11);
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
				port_count++;
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 3, &attr, 0);
			break;
		case 32:	// POTS
			if(omci_env_g.voip_enable != ENV_VOIP_DISABLE) {
				miptr= meinfo_util_miptr(53);
				list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
					port_count++;
			}
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 3, &attr, 0);
			break;
		case 38:	// VIDEO
			miptr= meinfo_util_miptr(82);
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
				port_count++;
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 3, &attr, 0);
			break;
		case 48:	// VEIP
			miptr= meinfo_util_miptr(329);
			list_for_each_entry(meptr, &miptr->me_instance_list, instance_node)
				port_count++;
			attr.ptr = NULL;
			attr.data = port_count;
			meinfo_me_attr_set(me, 3, &attr, 0);
			break;
		default:
			break;
	}
	return 0;
}

static int
meinfo_cb_5_is_attr_valid(struct me_t *me, unsigned char attr_order, struct attr_value_t *attr)
{
	int ret=1;

	if (attr_order==2) {
		// expected plugin type should be equal to actual plugin type
		if ((unsigned char)(attr->data)==(unsigned char)meinfo_util_me_attr_data(me, 1)) {
			if (me->instance_num < CARDHOLDER_MAX)
				is_plugin_type_invalid[me->instance_num]=0;
			ret=1;
		} else {
			if (me->instance_num < CARDHOLDER_MAX)
				is_plugin_type_invalid[me->instance_num]=1;
			ret=0;
		}
		alarmtask_send_msg(me->classid, me->instance_num);
	}
		
	return ret;
}

static int
meinfo_cb_5_alarm(struct me_t *me, unsigned char alarm_mask[28])
{
	struct meinfo_t *miptr=meinfo_util_miptr(me->classid);
	
	if (util_alarm_mask_get_bit(miptr->alarm_mask, 0)) {
		if (me->instance_num < CARDHOLDER_MAX) {
			if (is_plugin_type_invalid[me->instance_num]) {
				util_alarm_mask_set_bit(alarm_mask, 1);	// alarm 1: plugin type mismatch
			} else {
				util_alarm_mask_clear_bit(alarm_mask, 1);	// alarm 1: plugin type mismatch
			}
		}
	}
	return 0;
}

struct meinfo_callback_t meinfo_cb_5 = {
	.create_cb		= meinfo_cb_5_create,
	.is_attr_valid_cb	= meinfo_cb_5_is_attr_valid,
	.alarm_cb		= meinfo_cb_5_alarm,
};


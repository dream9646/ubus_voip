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
 * Module  : er_group_hw
 * File    : er_group_hw_11_47.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "util.h"
#include "meinfo.h"
#include "omciutil_misc.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"

// 11 Physical_path_termination_point_Ethernet_UNI
// 47 MAC_bridge_port_configuration_data

//11@inst, 47@10
int
er_group_hw_ethernet_macaddr(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	// private me wont be updated by olt, no need to do er_group update
	if (me && me->classid == 47 && me->is_private)
		return 0;

	// do nothing if this pptp is an equivlance of veip
	if (me && omciutil_misc_find_pptp_related_veip(me) != NULL)
		return 0;
       
	switch(action)
	{
	case ER_ATTR_GROUP_HW_OP_ADD:
		// we dont use the default value in me47 for macaddr, so we ignore ER_ATTR_GROUP_HW_OP_ADD event
		// we only use the macaddr set from olt or user cli, so we care event ER_ATTR_GROUP_HW_OP_UPDATE
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		{
			struct er_attr_group_instance_t *current_attr_inst;
			int port, port_enable_state=0;
			unsigned char mac[6], mac_old[6];
	
			//get current value 
			if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
			{
				dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
				return -1;
			}		
			// uni port
			port = current_attr_inst->er_attr_instance[0].attr_value.data;
			// mac new
			memcpy(mac, (unsigned char *)current_attr_inst->er_attr_instance[1].attr_value.ptr, 6);		
			// release current attr_int		
			er_attr_group_util_release_attrinst(current_attr_inst);
	
			// issue change only if mac old != new
			switch_hw_g.eth_address_get(port, mac_old);		


			if (memcmp(mac_old, mac, 6) != 0) {		
				dbprintf(LOG_WARNING, "port=%d, mac change from %02x:%02x:%02x:%02x:%02x:%02x to %02x:%02x:%02x:%02x:%02x:%02x\n", 
					port, 
					mac_old[0], mac_old[1], mac_old[2], mac_old[3], mac_old[4], mac_old[5],
					mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

				if (switch_hw_g.port_enable_get(port , &port_enable_state ) < 0 ) {
					dbprintf (LOG_ERR,"get eth enable err\n" );
					return -1;
				}
				if (port_enable_state == 0) { // interface is originally disabled, set mac directly
					if (switch_hw_g.eth_address_set(port, mac) < 0 ) {
						dbprintf (LOG_ERR,"set eth macaddr err\n");
						return -1;
					}
				} else { // interface is originally enabled, disable mac before setting mac
					int ret=0;	
					if (switch_hw_g.port_enable_set(port, 0 ) < 0 ) {
						dbprintf (LOG_ERR,"set eth enable err\n" );
						return -1;
					}
					if (switch_hw_g.eth_address_set(port, mac) < 0 ) {
						dbprintf (LOG_ERR,"set eth macaddr err\n");
						ret=-1;
					}
					if (switch_hw_g.port_enable_set(port, 1) < 0 ) {
						dbprintf (LOG_ERR,"set eth enable err\n");
						return -1;
					}					
					if (ret <0)
						return ret;
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		break;
	default:
       		dbprintf(LOG_ERR,"Unknow operator\n");
	}
	return 0;
}


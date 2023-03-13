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
 * Module  : cfm
 * File    : cfm_bport.c
 *
 ******************************************************************/

#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "hwresource.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_frame.h"
#include "proprietary_alu.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_bport.h"

struct me_t *
cfm_bport_find(cfm_config_t *cfm_config)
{
	// 422 -> 47 -> 45
	struct me_t *meptr_422 = NULL; 
	struct meinfo_t *miptr = meinfo_util_miptr(422);
	int me422_found = 0;
	
	list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(meptr_422, 1) == 1 &&	// occupied
		    meinfo_util_me_attr_data(meptr_422, 4) == cfm_config->port_type &&
		    meinfo_util_me_attr_data(meptr_422, 5) == cfm_config->port_type_index &&
		    meinfo_util_me_attr_data(meptr_422, 6) == cfm_config->logical_port_id) {
			me422_found = 1;
			break;
		}
	}
	if (!me422_found)
		return NULL;
	return meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(meptr_422, 2));
}

#define ME47_ATTR3_PPTP			1
#define ME47_ATTR3_1PMAPPER		3  
#define ME47_ATTR3_IPHOST		4
#define ME47_ATTR3_GEM_IWTP		5
#define ME47_ATTR3_MCAST_GEM_IWTP	6
#define ME47_ATTR3_VEIP			11
struct me_t *
cfm_bport_find_peer(cfm_config_t *cfm_config)
{
	// 422 -> 47 -> 45
	struct me_t *meptr_45, *meptr_47, *meptr_47_myself, *meptr_422;
	struct meinfo_t *miptr = meinfo_util_miptr(422);
	int me422_found = 0;
	struct me_t *me_wan_with_vfilter, *me_wan_without_vfilter, *me_wan, *me_uni, *me_veip, *me_iphost;
	
	list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(meptr_422, 1) == 1 &&	// occupied
		    meinfo_util_me_attr_data(meptr_422, 4) == cfm_config->port_type &&
		    meinfo_util_me_attr_data(meptr_422, 5) == cfm_config->port_type_index &&
		    meinfo_util_me_attr_data(meptr_422, 6) == cfm_config->logical_port_id) {
			me422_found = 1;
			break;
		}
	}
	if (!me422_found)
		return NULL;
	if ((meptr_47_myself = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(meptr_422, 2))) == NULL)
		return NULL;
	if ((meptr_45 = meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(meptr_47_myself, 1))) == NULL)
		return NULL;

	meptr_47 = me_wan_with_vfilter = me_wan_without_vfilter = me_wan = me_uni = me_veip = me_iphost = NULL;
	
	miptr = meinfo_util_miptr(47);
	list_for_each_entry(meptr_47, &miptr->me_instance_list, instance_node) {
		unsigned char me_pointer_type;
	
		if (meptr_47 == meptr_47_myself)
			continue;
		if (meinfo_util_me_attr_data(meptr_47, 1) != meptr_45->meid)
			continue;
		me_pointer_type = meinfo_util_me_attr_data(meptr_47, 3);

		// check me within the same bridge id
		switch (me_pointer_type) {
			case ME47_ATTR3_PPTP:
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN)
				if (!me_uni)
					me_uni = meptr_47;
			break;

			case ME47_ATTR3_VEIP:
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN)
				if (!me_veip)
					me_veip = meptr_47;
			break;

			case ME47_ATTR3_IPHOST:
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_WAN)
				if (!me_iphost)
					me_iphost = meptr_47;
			break;

			case ME47_ATTR3_1PMAPPER:
			case ME47_ATTR3_GEM_IWTP:
			case ME47_ATTR3_MCAST_GEM_IWTP:
			if (cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_UNI ||
			    cfm_config->port_type == ENV_BRIDGE_PORT_TYPE_CPU) {
				// 47->84
				struct me_t *meptr_84;
				 if (!me_wan)
				 	me_wan = meptr_47;
				if ((meptr_84 = meinfo_me_get_by_meid(84, meptr_47->meid))!= NULL) {
					unsigned char *bitmap = meinfo_util_me_attr_ptr(meptr_84, 1);
					// check 1st vlan entry only and assume it is postive match...to be fixed!
					if (meinfo_util_me_attr_data(meptr_84, 3) == 1 &&
					    util_bitmap_get_value(bitmap, 24*8, 4, 12) == cfm_config->pri_vlan) {
					    	if (!me_wan_with_vfilter)
							me_wan_with_vfilter = meptr_47;
					}
				} else {
					if (!me_wan_without_vfilter)
						me_wan_without_vfilter = meptr_47;
				}
			}
			break;
		}
	}
	if (me_wan_with_vfilter)
		return me_wan_with_vfilter;
	if (me_wan_without_vfilter)
		return me_wan_without_vfilter;
	if (me_wan)
		return me_wan;
	if (me_uni)
		return me_uni;
	if (me_veip)
		return me_veip;
	if (me_iphost)
		return me_iphost;
	return NULL;
}

unsigned int
cfm_bport_find_peer_index(cfm_config_t *cfm_config)
{
	struct me_t *bridge_port_me = cfm_bport_find_peer(cfm_config);
	if(bridge_port_me) {
		struct me_t *meptr_422 = hwresource_public2private(bridge_port_me);
		if(meptr_422 && (meinfo_util_me_attr_data(meptr_422, 1) == 1)) // Occupied
			return  meinfo_util_me_attr_data(meptr_422, 5);
	}
	return -1;
}

unsigned int
cfm_bport_find_peer_logical_portmask(cfm_config_t *cfm_config)
{
	// 422 -> 47 -> 45
	struct me_t *meptr_45, *meptr_47, *meptr_422, *meptr_422_myself;
	struct meinfo_t *miptr = meinfo_util_miptr(422);
	unsigned short bridge_meid;
	int me422_found = 0;
	unsigned int portmask = 0;
	
	list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(meptr_422, 1) == 1 &&	// occupied
		    meinfo_util_me_attr_data(meptr_422, 4) == cfm_config->port_type &&
		    meinfo_util_me_attr_data(meptr_422, 5) == cfm_config->port_type_index &&
		    meinfo_util_me_attr_data(meptr_422, 6) == cfm_config->logical_port_id) {
			me422_found = 1;
			meptr_422_myself = meptr_422;
			break;
		}
	}
	if (!me422_found)
		return 0;
	if ((meptr_47 = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(meptr_422, 2))) == NULL)
		return 0;
	if ((meptr_45 = meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(meptr_47, 1))) == NULL)
		return 0;
	bridge_meid = meptr_45->meid;

	list_for_each_entry(meptr_422, &miptr->me_instance_list, instance_node) {
		if (meinfo_util_me_attr_data(meptr_422, 1) == 1 &&      // occupied 
		    meptr_422 != meptr_422_myself) {
			unsigned char port_type =  meinfo_util_me_attr_data(meptr_422, 4);
			int logical_port_id =  meinfo_util_me_attr_data(meptr_422, 6);
			
			switch (cfm_config->port_type) {
				case ENV_BRIDGE_PORT_TYPE_WAN:
				case ENV_BRIDGE_PORT_TYPE_DS_BCAST:
				if (port_type == ENV_BRIDGE_PORT_TYPE_WAN ||
			    	    port_type == ENV_BRIDGE_PORT_TYPE_DS_BCAST)
			    	    	continue;
				break;

				case ENV_BRIDGE_PORT_TYPE_CPU:
				case ENV_BRIDGE_PORT_TYPE_IPHOST:
				case ENV_BRIDGE_PORT_TYPE_UNI:
				if (port_type == ENV_BRIDGE_PORT_TYPE_CPU ||
			    	    port_type == ENV_BRIDGE_PORT_TYPE_IPHOST ||
			    	    port_type == ENV_BRIDGE_PORT_TYPE_UNI)
			    	    	continue;
				break;
			}
			
			if ((meptr_47 = meinfo_me_get_by_meid(47, meinfo_util_me_attr_data(meptr_422, 2))) == NULL)
				continue;
			if ((meptr_45 = meinfo_me_get_by_meid(45, meinfo_util_me_attr_data(meptr_47, 1))) == NULL)
				continue;
			if (meptr_45->meid != bridge_meid)
				continue;
			if (cfm_config->logical_port_id == logical_port_id)
				continue;				

			portmask |= (1<< logical_port_id);
		}
	}
	return portmask;
}

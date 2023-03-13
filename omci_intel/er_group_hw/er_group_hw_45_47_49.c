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
 * File    : er_group_hw_45_47_49.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>

#include "util.h"
#include "meinfo.h"
#include "me_related.h"
#include "er_group.h"
#include "hwresource.h"
#include "switch.h"
#include "batchtab.h"

// 45 MAC_bridge_service_profile
// 47 MAC_bridge_port_configuration_data
// 49 MAC_bridge_port_filter_table_data

static char *
macaddr2str(char *macaddr) {
	static char s[20];
	snprintf(s, 20, "%02x:%02x:%02x:%02x:%02x:%02x", 
		macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	return s;
}

//45@inst,47@2,49@1
int er_group_hw_l2s_mac_table_create_del(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct switch_port_info_t port_info;
	struct me_t *bridgeport_me;
	struct switch_mac_tab_entry_t mac_table_entry;
	int i, is_static = 0; // only add/delete dynamic entry to avoid static entry is affected (such as rg predefined entries)
	struct er_attr_group_instance_t *current_attr_inst;
	struct attr_table_header_t *table_header = NULL;
	struct attr_table_entry_t *table_entry_pos = NULL;
	
	if (attr_inst == NULL) {
       		dbprintf(LOG_ERR,"attr_inst NULL\n");
		return -1;
	}

	// attr_inst[1] is bridge port config data port number
	if ((bridgeport_me=er_attr_group_util_attrinst2me(attr_inst, 1)) == NULL) {
		dbprintf(LOG_ERR,"bridge port me null?\n");
		return -1;
	}
	if (switch_get_port_info_by_me(bridgeport_me, &port_info)<0) {
		dbprintf(LOG_ERR,"get port_info by bridge_me %d err?\n", bridgeport_me->meid);
		return -1;
	}

	switch(action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[2].attr_value.ptr) == NULL) {
			dbprintf(LOG_WARNING, "mac filter table is empty\n");
			return 0;
		}
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				unsigned char dir = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 14, 1);
				unsigned char filter = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 15, 1);
				memset(&mac_table_entry, 0, sizeof(struct switch_mac_tab_entry_t));
				for (i=0; i<6; i++) {
					unsigned char mac = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 16+i*8, 8);
					mac_table_entry.mac_addr[i] = mac;
				}
				mac_table_entry.is_dynamic = !is_static;
				mac_table_entry.is_ivl = omci_env_g.ivlan_enable;
				if(filter) { // filter
					batchtab_omci_update("mac_filter");
				} else { // forward
					// da: this as egress port, sa: find peer ingress port
					if (dir==1) { // sa
						if (port_info.logical_port_id == switch_get_wan_logical_portid()) { // lan
							struct me_t *meptr47 = meinfo_me_get_by_meid(47, me->meid);
							struct meinfo_t *miptr45 = meinfo_util_miptr(45);
							struct me_t *meptr45 = NULL;
							list_for_each_entry(meptr45, &miptr45->me_instance_list, instance_node) {
								if(me_related(meptr47, meptr45)) {
									struct meinfo_t *miptr47 = meinfo_util_miptr(47);
									struct me_t *meptr = NULL;
									list_for_each_entry(meptr, &miptr47->me_instance_list, instance_node) {
										unsigned char tp_type = (unsigned char)meinfo_util_me_attr_data(meptr, 3);
										if(me_related(meptr, meptr45) && (tp_type == 1)) {
											struct me_t *ibridgeport_me = hwresource_public2private(meptr);
											if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
												unsigned int port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
												mac_table_entry.port_bitmap=(1<<port_id);
											} else {
												dbprintf_bat(LOG_ERR, "bridge port is null\n");
												return -1;
											}
											break;
										}
									}
									break;
								}
							}
						} else { // wan
							mac_table_entry.port_bitmap=1<<switch_get_wan_logical_portid();
						}
					} else { // da
						mac_table_entry.port_bitmap=1<<port_info.logical_port_id;
					}

					if (mac_table_entry.port_bitmap && switch_hw_g.mac_entry_add) {
						if (switch_hw_g.mac_entry_add(&mac_table_entry, is_static) <0) {
							dbprintf (LOG_ERR,"add macaddr %s failed\n", macaddr2str(mac_table_entry.mac_addr));	
							return -1;
						}
					}
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		if ((table_header = (struct attr_table_header_t *) attr_inst->er_attr_instance[2].attr_value.ptr) == NULL) {
			dbprintf(LOG_WARNING, "mac filter table is empty\n");
			return 0;
		}
		list_for_each_entry(table_entry_pos, &table_header->entry_list, entry_node) {
			if (table_entry_pos->entry_data_ptr != NULL) {
				unsigned char dir = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 14, 1);
				unsigned char filter = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 15, 1);
				memset(&mac_table_entry, 0, sizeof(struct switch_mac_tab_entry_t));
				for (i=0; i<6; i++) {
					unsigned char mac = util_bitmap_get_value(table_entry_pos->entry_data_ptr, 8*8, 16+i*8, 8);
					mac_table_entry.mac_addr[i] = mac;
				}
				mac_table_entry.is_dynamic = !is_static;
				mac_table_entry.is_ivl = omci_env_g.ivlan_enable;
				if(filter) { // filter
					batchtab_omci_update("mac_filter");
				} else { // forward
					// da: this as egress port, sa: find peer ingress port
					if (dir==1) { // sa
						if (port_info.logical_port_id == switch_get_wan_logical_portid()) { // lan
							struct me_t *meptr47 = meinfo_me_get_by_meid(47, me->meid);
							struct meinfo_t *miptr45 = meinfo_util_miptr(45);
							struct me_t *meptr45 = NULL;
							list_for_each_entry(meptr45, &miptr45->me_instance_list, instance_node) {
								if(me_related(meptr47, meptr45)) {
									struct meinfo_t *miptr47 = meinfo_util_miptr(47);
									struct me_t *meptr = NULL;
									list_for_each_entry(meptr, &miptr47->me_instance_list, instance_node) {
										unsigned char tp_type = (unsigned char)meinfo_util_me_attr_data(meptr, 3);
										if(me_related(meptr, meptr45) && (tp_type == 1)) {
											struct me_t *ibridgeport_me = hwresource_public2private(meptr);
											if(ibridgeport_me && (meinfo_util_me_attr_data(ibridgeport_me, 1) == 1)) { // Occupied
												unsigned int port_id = meinfo_util_me_attr_data(ibridgeport_me, 6);
												mac_table_entry.port_bitmap=(1<<port_id);
											} else {
												dbprintf_bat(LOG_ERR, "bridge port is null\n");
												return -1;
											}
											break;
										}
									}
									break;
								}
							}
						} else { // wan
							unsigned int phy_portmask, ext_portmask;
							switch_hw_g.portmask_logical_to_private(1<<switch_get_wan_logical_portid(), &phy_portmask, &ext_portmask);
							mac_table_entry.port_bitmap=phy_portmask;
						}
					} else { // da
						unsigned int phy_portmask, ext_portmask;
						switch_hw_g.portmask_logical_to_private(1<<port_info.logical_port_id, &phy_portmask, &ext_portmask);
						mac_table_entry.port_bitmap=phy_portmask;
					}

					if (mac_table_entry.port_bitmap && switch_hw_g.mac_entry_del) {
						if (switch_hw_g.mac_entry_del(&mac_table_entry) <0) {
							dbprintf (LOG_ERR,"del macaddr %s failed\n", macaddr2str(mac_table_entry.mac_addr));	
							return -1;
						}
					}
				}
			}
		}
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		if (er_group_hw_l2s_mac_table_create_del(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0)
		{
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}
		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL)
		{
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}
		//allocate
		if (er_group_hw_l2s_mac_table_create_del(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0)
		{
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	}

	return 0;
}

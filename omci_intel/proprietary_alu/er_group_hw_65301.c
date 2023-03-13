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
 * Module  : proprietary_alu
 * File    : er_group_hw_65301.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>

#include "util.h"
#include "meinfo.h"
#include "er_group.h"
#include "er_group_hw.h"
#include "hwresource.h"
#include "switch.h"
#include "acl.h"

#define ER_GROUP_HW_65301_MAC_ADDR_STR_LEN	13
// 65301 IP/MAC/VLAN_Address_Anti-Spoof_List_V2

int
er_group_hw_static_mcast_workaround(unsigned int ip_addr_1, unsigned int ip_addr_2, unsigned int ip_addr_3, unsigned int ip_addr_4, unsigned int ip_addr_5, unsigned int ip_addr_6, unsigned int ip_addr_7, unsigned int ip_addr_8)
{
	struct me_t *meptr = NULL;
	struct meinfo_t *miptr = meinfo_util_miptr(309);
	int table_size = 0;

	if(!miptr) return -1;

	list_for_each_entry(meptr, &miptr->me_instance_list, instance_node) {
		if(meptr && (table_size=meinfo_me_attr_get_table_entry_count(meptr, 8)) > 0) { // Static ACL
			int i;
			struct get_attr_table_context_t table_context;
			void *active_entry_data = malloc_safe(24);
			for(i=0; i<table_size; i++) {
				if (meinfo_me_attr_get_table_entry(meptr, 8, &table_context, active_entry_data) == MEINFO_RW_OK) {
					do {
						unsigned int dst_ip_start = (unsigned int)util_bitmap_get_value(active_entry_data, 24*8, 10*8, 32);
						unsigned int dst_ip_end = (unsigned int)util_bitmap_get_value(active_entry_data, 24*8, 14*8, 32);
						if( ((ip_addr_1!=dst_ip_start) && (ip_addr_1!=dst_ip_end)) &&
						    ((ip_addr_2!=dst_ip_start) && (ip_addr_2!=dst_ip_end)) &&
						    ((ip_addr_3!=dst_ip_start) && (ip_addr_3!=dst_ip_end)) &&
						    ((ip_addr_4!=dst_ip_start) && (ip_addr_4!=dst_ip_end)) &&
						    ((ip_addr_5!=dst_ip_start) && (ip_addr_5!=dst_ip_end)) &&
						    ((ip_addr_6!=dst_ip_start) && (ip_addr_6!=dst_ip_end)) &&
						    ((ip_addr_7!=dst_ip_start) && (ip_addr_7!=dst_ip_end)) &&
						    ((ip_addr_8!=dst_ip_start) && (ip_addr_8!=dst_ip_end)) ) {
							struct attr_table_entry_t *found_table_entry = meinfo_me_attr_locate_table_entry(meptr, 8, active_entry_data);
							if (found_table_entry) {
								unsigned char attr_mask[2] = {0x00, 0x00};
								meinfo_me_attr_delete_table_entry(meptr, 8, found_table_entry, 0);
								util_attr_mask_set_bit(attr_mask, 8); // update attr8
								meinfo_me_issue_self_er_attr_group(meptr, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask);
								break;
							}
						}
					} while (meinfo_me_attr_get_table_entry_next(meptr, 8, &table_context, active_entry_data) == MEINFO_RW_OK);
				}
			}
			free_safe(active_entry_data);
		}
	}

	return 0;
}

// 65301@meid,1,2,3,4,5,6,7,8,9,10,11,12,13
int
er_group_hw_anti_spoof_list(int action, struct er_attr_group_instance_t *attr_inst, struct me_t *me, unsigned char attr_mask[2])
{
	struct er_attr_group_instance_t *current_attr_inst;
	struct me_t *meptr = NULL, *iethuni_me = NULL;
	unsigned char mode, mac_addr_any[IFHWADDRLEN], mac_addr_ignore[IFHWADDRLEN], mac_addr_1[IFHWADDRLEN], mac_addr_2[IFHWADDRLEN], mac_addr_3[IFHWADDRLEN], mac_addr_4[IFHWADDRLEN];
	unsigned short meid, port, tci_1 = 0, tci_2 = 0, tci_3 = 0, tci_4 = 0, tci_5 = 0, tci_6 = 0, tci_7 = 0, tci_8 = 0;
	unsigned int ip_addr_1 = 0, ip_addr_2 = 0, ip_addr_3 = 0, ip_addr_4 = 0, ip_addr_5 = 0, ip_addr_6 = 0, ip_addr_7 = 0, ip_addr_8 = 0;

	if (attr_inst == NULL)
		return -1;

	meid = (unsigned short) attr_inst->er_attr_instance[0].attr_value.data;
	if ((meptr = meinfo_me_get_by_meid(11, meid)) == NULL) {
		dbprintf(LOG_ERR, "could not get bridge port me, classid=11, meid=%u\n", meid);
		return -1;
	}

	iethuni_me = hwresource_public2private(meptr);
	port = meinfo_util_me_attr_data(iethuni_me, 4);
	mode = (unsigned char) attr_inst->er_attr_instance[1].attr_value.data;
	ip_addr_1 = htonl((unsigned int) attr_inst->er_attr_instance[2].attr_value.data);
	ip_addr_2 = htonl((unsigned int) attr_inst->er_attr_instance[3].attr_value.data);
	ip_addr_3 = htonl((unsigned int) attr_inst->er_attr_instance[4].attr_value.data);
	ip_addr_4 = htonl((unsigned int) attr_inst->er_attr_instance[5].attr_value.data);
	ip_addr_5 = htonl((unsigned int) attr_inst->er_attr_instance[6].attr_value.data);
	ip_addr_6 = htonl((unsigned int) attr_inst->er_attr_instance[7].attr_value.data);
	ip_addr_7 = htonl((unsigned int) attr_inst->er_attr_instance[8].attr_value.data);
	ip_addr_8 = htonl((unsigned int) attr_inst->er_attr_instance[9].attr_value.data);

	memset(mac_addr_any, 0x00, sizeof(mac_addr_any)); // All '00'
	memset(mac_addr_ignore, 0xFF, sizeof(mac_addr_ignore)); // All 'FF'
	memcpy(mac_addr_1, (unsigned char *) attr_inst->er_attr_instance[10].attr_value.ptr, sizeof(mac_addr_1));
	memcpy(mac_addr_2, (unsigned char *) attr_inst->er_attr_instance[11].attr_value.ptr, sizeof(mac_addr_2));
	memcpy(mac_addr_3, (unsigned char *) attr_inst->er_attr_instance[12].attr_value.ptr, sizeof(mac_addr_3));
	memcpy(mac_addr_4, (unsigned char *) attr_inst->er_attr_instance[13].attr_value.ptr, sizeof(mac_addr_4));

	tci_1 = ((mac_addr_1[1] << 8) | mac_addr_1[2]);
	tci_2 = ((mac_addr_1[4] << 8) | mac_addr_1[5]);
	tci_3 = ((mac_addr_2[1] << 8) | mac_addr_2[2]);
	tci_4 = ((mac_addr_2[4] << 8) | mac_addr_2[5]);
	tci_5 = ((mac_addr_3[1] << 8) | mac_addr_3[2]);
	tci_6 = ((mac_addr_3[4] << 8) | mac_addr_3[5]);
	tci_7 = ((mac_addr_4[1] << 8) | mac_addr_4[2]);
	tci_8 = ((mac_addr_4[4] << 8) | mac_addr_4[5]);

	switch (action) {
	case ER_ATTR_GROUP_HW_OP_ADD:
		switch(mode) {
			case 0:	// No anti-spoof filtering at all
				acl_del_by_port(port);
				break;
			case 1:	// IP Addresses only - Inclusive
				// Workaround for CHT case: Remove multicast static ACL entries by these fields
				if (omci_env_g.olt_workaround_enable == ENV_OLT_WORKAROUND_ALU && omci_env_g.port2port_enable) {
					 // Only notice UNI#1 (0x101) ACL table
					 if(meid==0x101) {
					 	er_group_hw_static_mcast_workaround(htonl(ip_addr_1), htonl(ip_addr_2),
					 		htonl(ip_addr_3), htonl(ip_addr_4), htonl(ip_addr_5),
					 		htonl(ip_addr_6), htonl(ip_addr_7), htonl(ip_addr_8));
					}
				} else {
					if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_5 != 0 && ip_addr_5 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_5, ip_addr_5, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_6 != 0 && ip_addr_6 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_6, ip_addr_6, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_7 != 0 && ip_addr_7 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_7, ip_addr_7, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					if(ip_addr_8 != 0 && ip_addr_8 != 0xFFFFFFFF)
						acl_add_ip(port, 0, 0, ip_addr_8, ip_addr_8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
					acl_change_mode(port, ACL_MODE_WHITE);
				}
				break;
			case 2:	// MAC Addresses only - Inclusive
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				acl_change_mode(port, ACL_MODE_WHITE);
				break;
			case 3:	// IP/MAC Address Combo - Inclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				acl_change_mode(port, ACL_MODE_WHITE);
				break;
			case 4:	// Per Service IP only - Inclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_5 != 0 && ip_addr_5 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_5, ip_addr_5, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_6 != 0 && ip_addr_6 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_6, ip_addr_6, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_7 != 0 && ip_addr_7 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_7, ip_addr_7, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_8 != 0 && ip_addr_8 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_8, ip_addr_8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(tci_1 != 0xFFFF)
					acl_add_vid(port, (tci_1&0xFFF), (tci_1&0xFFF), ((tci_1>>13)&0x7), 0);
				if(tci_2 != 0xFFFF)
					acl_add_vid(port, (tci_2&0xFFF), (tci_2&0xFFF), ((tci_2>>13)&0x7), 0);
				if(tci_3 != 0xFFFF)
					acl_add_vid(port, (tci_3&0xFFF), (tci_3&0xFFF), ((tci_3>>13)&0x7), 0);
				if(tci_4 != 0xFFFF)
					acl_add_vid(port, (tci_4&0xFFF), (tci_4&0xFFF), ((tci_4>>13)&0x7), 0);
				if(tci_5 != 0xFFFF)
					acl_add_vid(port, (tci_5&0xFFF), (tci_5&0xFFF), ((tci_5>>13)&0x7), 0);
				if(tci_6 != 0xFFFF)
					acl_add_vid(port, (tci_6&0xFFF), (tci_6&0xFFF), ((tci_6>>13)&0x7), 0);
				if(tci_7 != 0xFFFF)
					acl_add_vid(port, (tci_7&0xFFF), (tci_7&0xFFF), ((tci_7>>13)&0x7), 0);
				if(tci_8 != 0xFFFF)
					acl_add_vid(port, (tci_8&0xFFF), (tci_8&0xFFF), ((tci_8>>13)&0x7), 0);
				acl_change_mode(port, ACL_MODE_WHITE);
				break;
			case 5:	// IP Addresses only - Exclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_5 != 0 && ip_addr_5 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_5, ip_addr_5, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_6 != 0 && ip_addr_6 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_6, ip_addr_6, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_7 != 0 && ip_addr_7 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_7, ip_addr_7, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_8 != 0 && ip_addr_8 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_8, ip_addr_8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				acl_change_mode(port, ACL_MODE_BLACK);
				break;
			case 6:	// MAC Addresses only - Exclusive
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				acl_change_mode(port, ACL_MODE_BLACK);
				break;
			case 7:	// IP/MAC Address Combo - Exclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_add_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, NULL, 0, 0);
				}
				acl_change_mode(port, ACL_MODE_BLACK);
				break;
			case 8:	// MAC Addresses OUI only - Inclusive
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_add_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0, 0);
				}
				acl_change_mode(port, ACL_MODE_WHITE);
				break;
			default:
				dbprintf(LOG_ERR,"Unknown filter mode %d\n", mode);
				break;
		}
		aclinfo_flush_to_hw();
		break;
	case ER_ATTR_GROUP_HW_OP_DEL:
		switch(mode) {
			case 0:	// No anti-spoof filtering at all
				acl_del_by_port(port);
				break;
			case 1:	// IP Addresses only - Inclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_5 != 0 && ip_addr_5 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_5, ip_addr_5, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_6 != 0 && ip_addr_6 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_6, ip_addr_6, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_7 != 0 && ip_addr_7 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_7, ip_addr_7, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_8 != 0 && ip_addr_8 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_8, ip_addr_8, 0, 0, 0, 0, 0, 0, 0, 0);
				break;
			case 2:	// MAC Addresses only - Inclusive
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				break;
			case 3:	// IP/MAC Address Combo - Inclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0);
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				break;
			case 4:	// Per Service IP only - Inclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_5 != 0 && ip_addr_5 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_5, ip_addr_5, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_6 != 0 && ip_addr_6 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_6, ip_addr_6, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_7 != 0 && ip_addr_7 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_7, ip_addr_7, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_8 != 0 && ip_addr_8 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_8, ip_addr_8, 0, 0, 0, 0, 0, 0, 0, 0);
				if(tci_1 != 0xFFFF)
					acl_del_vid(port, (tci_1&0xFFF), (tci_1&0xFFF), ((tci_1>>13)&0x7));
				if(tci_2 != 0xFFFF)
					acl_del_vid(port, (tci_2&0xFFF), (tci_2&0xFFF), ((tci_2>>13)&0x7));
				if(tci_3 != 0xFFFF)
					acl_del_vid(port, (tci_3&0xFFF), (tci_3&0xFFF), ((tci_3>>13)&0x7));
				if(tci_4 != 0xFFFF)
					acl_del_vid(port, (tci_4&0xFFF), (tci_4&0xFFF), ((tci_4>>13)&0x7));
				if(tci_5 != 0xFFFF)
					acl_del_vid(port, (tci_5&0xFFF), (tci_5&0xFFF), ((tci_5>>13)&0x7));
				if(tci_6 != 0xFFFF)
					acl_del_vid(port, (tci_6&0xFFF), (tci_6&0xFFF), ((tci_6>>13)&0x7));
				if(tci_7 != 0xFFFF)
					acl_del_vid(port, (tci_7&0xFFF), (tci_7&0xFFF), ((tci_7>>13)&0x7));
				if(tci_8 != 0xFFFF)
					acl_del_vid(port, (tci_8&0xFFF), (tci_8&0xFFF), ((tci_8>>13)&0x7));
				break;
			case 5:	// IP Addresses only - Exclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_5 != 0 && ip_addr_5 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_5, ip_addr_5, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_6 != 0 && ip_addr_6 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_6, ip_addr_6, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_7 != 0 && ip_addr_7 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_7, ip_addr_7, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_8 != 0 && ip_addr_8 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_8, ip_addr_8, 0, 0, 0, 0, 0, 0, 0, 0);
				break;
			case 6:	// MAC Addresses only - Exclusive
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				break;
			case 7:	// IP/MAC Address Combo - Exclusive
				if(ip_addr_1 != 0 && ip_addr_1 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_1, ip_addr_1, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_2 != 0 && ip_addr_2 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_2, ip_addr_2, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_3 != 0 && ip_addr_3 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_3, ip_addr_3, 0, 0, 0, 0, 0, 0, 0, 0);
				if(ip_addr_4 != 0 && ip_addr_4 != 0xFFFFFFFF)
					acl_del_ip(port, 0, 0, ip_addr_4, ip_addr_4, 0, 0, 0, 0, 0, 0, 0, 0);
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[ER_GROUP_HW_65301_MAC_ADDR_STR_LEN];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, NULL, 0);
				}
				break;
			case 8:	// MAC Addresses OUI only - Inclusive
				mac_addr_1[3] = mac_addr_1[4] = mac_addr_1[5] = 0;
				mac_addr_2[3] = mac_addr_2[4] = mac_addr_2[5] = 0;
				mac_addr_3[3] = mac_addr_3[4] = mac_addr_3[5] = 0;
				mac_addr_4[3] = mac_addr_4[4] = mac_addr_4[5] = 0;
				if(memcmp(mac_addr_1, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_1, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[13];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_1[0], mac_addr_1[1], mac_addr_1[2], mac_addr_1[3], mac_addr_1[4], mac_addr_1[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0);
				}
				if(memcmp(mac_addr_2, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_2, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[13];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_2[0], mac_addr_2[1], mac_addr_2[2], mac_addr_2[3], mac_addr_2[4], mac_addr_2[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0);
				}
				if(memcmp(mac_addr_3, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_3, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[13];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_3[0], mac_addr_3[1], mac_addr_3[2], mac_addr_3[3], mac_addr_3[4], mac_addr_3[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0);
				}
				if(memcmp(mac_addr_4, mac_addr_any, sizeof(mac_addr_any)) &&
				   memcmp(mac_addr_4, mac_addr_ignore, sizeof(mac_addr_ignore))) {
				   	char mac_addr[13];
				   	sprintf(mac_addr, "%02x%02x%02x%02x%02x%02x", mac_addr_4[0], mac_addr_4[1], mac_addr_4[2], mac_addr_4[3], mac_addr_4[4], mac_addr_4[5]);
					acl_del_mac(port, NULL, NULL, mac_addr, "FFFFFF000000", 0);
				}
				break;
			default:
				dbprintf(LOG_ERR,"Unknown filter mode %d\n", mode);
				break;
		}
		aclinfo_flush_to_hw();
		break;
	case ER_ATTR_GROUP_HW_OP_UPDATE:
		//delete
		if (er_group_hw_anti_spoof_list(ER_ATTR_GROUP_HW_OP_DEL, attr_inst, me, attr_mask) < 0) {
			dbprintf(LOG_ERR, "fail to update for deleting\n");
			return -1;
		}

		//get current value
		if ((current_attr_inst = er_attr_group_util_gen_current_value_attrinst(attr_inst)) == NULL) {
			dbprintf(LOG_ERR, "fail to gen current value attr inst\n");
			return -1;
		}

		//every thing the same: do nothing.
		if(	current_attr_inst->er_attr_instance[2].attr_value.data == attr_inst->er_attr_instance[2].attr_value.data &&
			current_attr_inst->er_attr_instance[3].attr_value.data == attr_inst->er_attr_instance[3].attr_value.data &&
			current_attr_inst->er_attr_instance[4].attr_value.data == attr_inst->er_attr_instance[4].attr_value.data &&
			current_attr_inst->er_attr_instance[5].attr_value.data == attr_inst->er_attr_instance[5].attr_value.data &&
			current_attr_inst->er_attr_instance[6].attr_value.data == attr_inst->er_attr_instance[6].attr_value.data &&
			current_attr_inst->er_attr_instance[7].attr_value.data == attr_inst->er_attr_instance[7].attr_value.data &&
			current_attr_inst->er_attr_instance[8].attr_value.data == attr_inst->er_attr_instance[8].attr_value.data &&
			current_attr_inst->er_attr_instance[9].attr_value.data == attr_inst->er_attr_instance[9].attr_value.data &&
			!memcmp(&current_attr_inst->er_attr_instance[10].attr_value.ptr, &attr_inst->er_attr_instance[10].attr_value.ptr, IFHWADDRLEN) &&
			!memcmp(&current_attr_inst->er_attr_instance[11].attr_value.ptr, &attr_inst->er_attr_instance[11].attr_value.ptr, IFHWADDRLEN) &&
			!memcmp(&current_attr_inst->er_attr_instance[12].attr_value.ptr, &attr_inst->er_attr_instance[12].attr_value.ptr, IFHWADDRLEN) &&
			!memcmp(&current_attr_inst->er_attr_instance[13].attr_value.ptr, &attr_inst->er_attr_instance[13].attr_value.ptr, IFHWADDRLEN)) {
			mode = (unsigned char) current_attr_inst->er_attr_instance[1].attr_value.data;
			if(mode != attr_inst->er_attr_instance[1].attr_value.data) {
				switch(mode) {
					case 0:	// No anti-spoof filtering at all
						acl_del_by_port(port);
						break;
					case 1:	// IP Addresses only - Inclusive
						acl_change_mode(port, ACL_MODE_WHITE);
						break;
					case 2:	// MAC Addresses only - Inclusive
						acl_change_mode(port, ACL_MODE_WHITE);
						break;
					case 3:	// IP/MAC Address Combo - Inclusive
						acl_change_mode(port, ACL_MODE_WHITE);
						break;
					case 4:	// Per Service IP only - Inclusive
						acl_change_mode(port, ACL_MODE_WHITE);
						break;
					case 5:	// IP Addresses only - Exclusive
						acl_change_mode(port, ACL_MODE_BLACK);
						break;
					case 6:	// MAC Addresses only - Exclusive
						acl_change_mode(port, ACL_MODE_BLACK);
						break;
					case 7:	// IP/MAC Address Combo - Exclusive
						acl_change_mode(port, ACL_MODE_BLACK);
						break;
					case 8:	// MAC Addresses OUI only - Inclusive
						acl_change_mode(port, ACL_MODE_WHITE);
						break;
					default:
						dbprintf(LOG_ERR,"Unknown filter mode %d\n", mode);
						break;
				}
				aclinfo_flush_to_hw();
			}
			dbprintf(LOG_ERR,"Field the same do nothing\n");
			er_attr_group_util_release_attrinst(current_attr_inst);
			return 0;
		}

		//add
		if (er_group_hw_anti_spoof_list(ER_ATTR_GROUP_HW_OP_ADD, current_attr_inst, me, attr_mask) < 0) {
			er_attr_group_util_release_attrinst(current_attr_inst);
			dbprintf(LOG_ERR, "fail to update for adding\n");
			return -1;
		}
		er_attr_group_util_release_attrinst(current_attr_inst);
		break;
	default:
		dbprintf(LOG_ERR,"Unknown operator\n");
	}

	return 0;
}

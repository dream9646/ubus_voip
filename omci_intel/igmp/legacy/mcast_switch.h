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
 * Module  : igmp
 * File    : mcast_switch.h
 *
 ******************************************************************/

#ifndef __IGMP_SWITCH_H__
#define __IGMP_SWITCH_H__


struct igmp_mac_tab_entry_t{
	unsigned short vlan;
	unsigned char mac_addr[6];
	unsigned char mac_mask[6];
	int port_bitmap;
};

struct ipv6_mac_member_t {
	unsigned char 		mac_addr[6];
	struct in6_addr		ipv6;
	struct list_head	node;
};

/*
struct switch_hw_t {

	int (*igmp_mode_set) (int mode);
	int (*igmp_mode_get) (void);
	int (*mac_table_locate_entry) (unsigned short vlan, unsigned char macaddr[] );
	int (*mac_table_locate_empty) (unsigned short vlan);
	
	int (*mac_table_add_entry) (unsigned short entryid, struct igmp_mac_tab_entry_t *mac_tab_entry);
	int (*mac_table_del_entry) (unsigned short entryid);
	int (*mac_table_update_entry) (unsigned short entryid, 
			struct igmp_mac_tab_entry_t *mac_tab_entry);
	
	int (*mac_table_get_entry) (unsigned short entryid, struct igmp_mac_tab_entry_t * );
	int (*mac_table_mcast_add) (struct mac_tab_entry_t *mac_tab_entry);
	int (*mac_table_mcast_del) (struct mac_tab_entry_t *mac_tab_entry);
	int (*mac_table_mcast_update) (struct igmp_mac_tab_entry_t *mac_tab_entry);
	int (*mac_table_mcast_get) (struct mac_tab_entry_t *mac_tab_entry);
	int (*frame_send) (struct igmp_pktinfo_t *, unsigned char pbit_val);
	int (*frame_recv_is_available) (struct timeval *timeout);
	int (*frame_recv)(struct igmp_pktinfo_t * );
	int (*vlan_filter)(struct igmp_pktinfo_t *, int );
	int (*vlan_inbound_tag_translation)(struct igmp_pktinfo_t *);
	int (*vlan_outbound_tag_translation)(struct igmp_pktinfo_t *, unsigned char *pbit_value);
	int (*get_bridge_port_assignment)(unsigned char port_type,unsigned char port_id,unsigned char *mac_table);
	int (*init)(void);
	int (*shutdown)(void);
};
*/

/* igmp_switch.c */
int igmp_switch_ratelimit_set(int port, int upstream_rate);
int igmp_switch_igmp_extract_set(int enable);
int igmp_switch_igmp_extract_get(unsigned int *enable);
int igmp_switch_mcast_entry_add(struct switch_mac_tab_entry_t *mac_tab_entry);
int igmp_switch_mcast_entry_del(struct switch_mac_tab_entry_t *mac_tab_entry);
int igmp_switch_mcast_entry_update(struct switch_mac_tab_entry_t *mac_tab_entry);
int igmp_switch_mcast_entry_get(struct switch_mac_tab_entry_t *mac_tab_entry);
int igmp_switch_mac_table_client_update(struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group);
int igmp_switch_mac_table_mbship_refresh(struct igmp_mbship_db_t *igmp_mbship_db);
int igmp_switch_frame_send(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_switch_get_l2_mcast_entry_counter(int br_id);
int igmp_switch_preassign_list_exist(union ipaddr *ip);
int igmp_switch_init(void);
int igmp_switch_shutdown(void);

#endif

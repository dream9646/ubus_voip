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
 * File    : igmp_omci.h
 *
 ******************************************************************/

#ifndef __IGMP_OMCI_H__
#define __IGMP_OMCI_H__

#define IGMP_ACL_DYNAMIC		0
#define IGMP_ACL_STATIC			1

#define IGMP_ACTIVE_GROUP_LIST_GROUP	0
#define IGMP_ACTIVE_GROUP_LIST_CLIENT	1

#define IGMP_PORT_SCOPE			0
#define IGMP_VID_SCOPE			1

struct igmp_omci_acl_node_t {
	unsigned short row_id;
	unsigned short gemport;		// stream gemport
	unsigned short vid_ani;		// stream vlanid
	unsigned int source_ip;		// stream server
	struct in6_addr   source_ipv6;
	unsigned short vid_uni;		// client vlanid
	unsigned int dst_ip_start;
	unsigned int dst_ip_end;
	struct in6_addr   dst_ipv6;
	unsigned int imputed_bw;
	unsigned short preview_length;
	unsigned short preview_repeat_time;
	unsigned short preview_repeat_count;
	unsigned short preview_reset_time;
	struct list_head entry_node;
};

struct igmp_omci_allowed_preview_node_t {
	unsigned short row_id;
	unsigned int source_ip;		// stream server
	struct in6_addr source_ipv6;
	unsigned int dst_ip;
	struct in6_addr dst_ipv6;
	unsigned short vid_ani;		// stream vlanid
	unsigned short vid_uni;		// client vlanid
	unsigned short duration;
	unsigned short timeleft;
	unsigned int imputed_bw;
	struct list_head entry_node;
};

struct igmp_omci_msgrate_stat_t {
	unsigned char bridge_id;
	unsigned char bridge_port_instance;
	unsigned int vid;
	unsigned int time;
	unsigned int msg_total;
	struct list_head entry_node;
};

struct igmp_omci_group_t {
	struct igmp_group_t igmp_group;
	unsigned int uni_vid;
	unsigned int bandwidth;
	struct list_head entry_node;
};


/* igmp_omci.c */
extern unsigned short cli_mcast_op_profile_meid;
struct me_t *igmp_omci_get_subscriber_config_me(struct me_t *bridge_port_me);
struct me_t *igmp_omci_get_subscriber_monitor_me(struct me_t *bridge_port_me);
struct me_t *igmp_omci_get_op_profile_me(struct me_t *bridge_port_me);
struct me_t *igmp_omci_get_mspt_op_profile_me(struct me_t *bridge_port_me, int client_vid );
int igmp_omci_check_is_in_static_acl(union ipaddr *match_ip);
int igmp_omci_check_join_is_allowed(struct me_t *bridge_port_me, struct igmp_clientinfo_t *cinfo);
int igmp_omci_update_join(struct me_t *bridge_port_me, struct igmp_clientinfo_t *cinfo);
int igmp_omci_update_leave(struct me_t *bridge_port_me, struct igmp_clientinfo_t *cinfo);
int igmp_omci_check_msgrate_is_allowed(struct me_t *bridge_port_me,int client_vid);
int igmp_omci_update_msgrate(struct me_t *bridge_port_me, unsigned int vid_uni);
int igmp_omci_read_igmp_profile_me(struct igmp_config_t *igmp_config, struct me_t *bridge_port, int vid_uni);
int igmp_omci_clear_all_mcast_subscriber_monitor_me(void);
int igmp_omci_bridge_port_assignment_update( void );
int igmp_omci_ipv4_active_group_remove( struct me_t *, struct igmp_mbship_t *igmp_mbship);
int igmp_omci_ipv6_active_group_remove( struct me_t *, struct igmp_mbship_t *igmp_mbship);
int igmp_omci_get_logical_port_id_by_bridge_port_meid( unsigned short port_meid );
struct me_t * igmp_omci_mcast_bridge_port_get(struct me_t *bridge_port_me);
int igmp_omci_rm_illeagal_group_in_client(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo);
void *igmp_omci_locate_ipv4_active_group_list_client(struct me_t *bridge_port_me, unsigned short client_vid, struct in_addr group_ip, struct in_addr src_ip, struct in_addr client_ip,unsigned char mode);
int igmp_omci_total_bandwidth_in_active_group_list( struct me_t *bridge_port, int vid, unsigned char mode );
int igmp_omci_load_acl_table_988(struct list_head *acl_list, struct attr_table_header_t *tab_header);
int igmp_omci_unload_acl_table(struct list_head *acl_list );
int igmp_omci_load_allowed_preview_groups_table(struct list_head *acl_list, struct attr_table_header_t *tab_header);
int igmp_omci_unload_apgt_table(struct list_head *apgt_list );
int igmp_omci_max_allowed_limit_filter(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_omci_rm_all_active_group_list_group();
int igmp_omci_update_upstream_rate(int rate);
int igmp_omci_remove_bridge_port_rate_list();
//static int igmp_omci_allowed_preview_group_table_entry_find(struct me_t *bridge_port_me, struct igmp_clientinfo_t *igmp_clientinfo, struct in_addr group_ip, struct igmp_omci_allowed_preview_node_t *apgt_node);
int igmp_omci_check_mbships(void);
#endif

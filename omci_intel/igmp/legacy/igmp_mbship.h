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
 * File    : igmp_mbship.h
 *
 ******************************************************************/

#ifndef __IGMP_MBSHIP_H__
#define __IGMP_MBSHIP_H__

#include <netinet/in.h>

#define MBSHIP_GROUP_ADD_ALREADY_EXIST 	0	
#define MBSHIP_GROUP_ADD_OK			1

#define MBSHIP_GROUP_DEL_NOT_EXIST 		0	
#define MBSHIP_GROUP_DEL_CLIENT_NOT_EMPTY	1	
#define MBSHIP_GROUP_DEL_OK			2	

#define MBSHIP_CLIENT_ADD_ALREADY_EXIST 	0	
#define MBSHIP_CLIENT_ADD_OK 			1	
#define MBSHIP_CLIENT_ADD_DENY 			2	

#define MBSHIP_CLIENT_DEL_NOT_EXIST 		0	
#define MBSHIP_CLIENT_DEL_OK 			1	

#define MBSHIP_CLIENT_SEARCH_NOT_FOUND		0	
#define MBSHIP_CLIENT_SEARCH_FOUND		1	

#define MBSHIP_CLIENT_ACL_PREVIEW_NOT_EXIST	0
#define MBSHIP_CLIENT_ACL_PREVIEW_EXIST		1
#define MBSHIP_CLIENT_ACL_PREVIEW_OVERFLOW	2

//#define PREVIEW_JOIN_GAP			1	
#define IGMP_V3_LW_SIGNATURE            0xEFDCAEFB
#define IGMPV3_QUERY_TIMEOUT_INTERVAL igmp_config_g.max_respon_time*100 + igmp_config_g.robustness*igmp_config_g.query_interval*1000

struct igmp_mbship_db_t {
	struct list_head 	mbship_list;
	struct list_head 	preview_acl_list;
	struct list_head 	apgt_list;
	struct list_head 	alu_signal_list;
	struct list_head 	mcast_snoop_list;
	struct list_head 	mcast_proxy_list;
};

struct igmp_mbship_preview_acl_t {
	union ipaddr		group_ip;
	union ipaddr		src_ip;
	union ipaddr		client_ip;
	unsigned short 		preview_len;
	unsigned char 		repeat_count;
	long	 		last_join_time;
	int			timer_id;
	struct list_head	entry_node;
};

struct igmp_mbship_apgt_t {
	union ipaddr		group_ip;
	union ipaddr		src_ip;
	int			ani_vid;
	int			uni_vid;
	unsigned short 		duration;
	long			start_time;
	struct list_head	entry_node;
};

struct igmp_alu_group_t{
    union ipaddr	group_ip;
    short int signal;
    struct list_head	entry_node;
};

struct igmp_mbship_t{
	unsigned short		src_port_meid;
	unsigned int 		uni_vid;
	unsigned int 		ani_vid;
	union ipaddr		group_ip;
	union ipaddr		src_ip;
	union ipaddr		client_ip;
	unsigned char 		client_mac[6];
	unsigned char 		allowed_type; // 0: full acl auth1: dynamic acl preview  2: allowed preview group
	unsigned char		logical_port_id;
	unsigned short		preview_repeat;
	long	 		access_time;
	unsigned short 		p2p_cfg;
	struct list_head 	entry_node;
	struct list_head 	entry_node_v3;
	unsigned int            lw_igmpv3_signature;
};

// new code compatibility: for igmp/counter/*.c which uses mcast_mbship_t instead of igmp_mbship_t
#define mcast_mbship_t igmp_mbship_t

/*
struct igmp_mbship_t {
	struct igmp_group_t 	group;
	struct list_head 	clientinfo_list[IGMP_MAX_UNI_PORTS];
	struct list_head 	clientinfo_preview_timeup_list[IGMP_MAX_UNI_PORTS];
	struct list_head 	mbship_node;
};
*/

/* igmp_mbship.c */
void igmp_mbship_V3_lw_timeout_update_routine();
void alu_igmp_update_group(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group);
short alu_igmp_get_group_signal(struct igmp_mbship_db_t *igmp_mbship_db, union ipaddr *igmp_group);
void alu_igmp_set_group_signal(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_group_t *igmp_group, short signal);
extern struct igmp_mbship_db_t igmp_mbship_db_g;
int igmp_mbship_db_init(struct igmp_mbship_db_t *igmp_mbship_db);
int igmp_mbship_db_deinit(struct igmp_mbship_db_t *igmp_mbship_db);
int igmp_mbship_init(struct igmp_mbship_t *igmp_mbship);
int igmp_mbship_db_timeout_update(long timer_interval);
int igmp_mbship_timeout_update(struct igmp_mbship_t *igmp_mbship, long timer_interval);
int igmp_mbship_free(struct igmp_mbship_t *igmp_mbship);
int igmp_mbship_db_free(struct igmp_mbship_db_t *igmp_mbship_db);
void igmp_mbship_timeout_update_routine(void *unused);
void igmp_mbship_lw_v3_timeout_update_routine(void *unused);
void igmp_mbship_group_leave_update(void *igmp_group);
int igmp_mbship_dbg_traverse(struct igmp_mbship_db_t *igmp_mbship_db);
int igmp_mbship_total_group_add_is_allow ( struct igmp_mbship_db_t * igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_mbship_total_group_add_update ( struct igmp_mbship_db_t * igmp_mbship_db,struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_mbship_total_group_del_update ( struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_mbship_db_preview_timeout_update( void ); 
int igmp_mbship_preview_timeout_update(struct igmp_mbship_t *igmp_mbship ); 
int igmp_mbship_port_clear( struct igmp_mbship_db_t *igmp_mbship_db, unsigned short src_port_id); 
int igmp_mbship_add_client ( struct igmp_mbship_db_t *igmp_mbship_db , struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group);
int igmp_mbship_del_client ( struct igmp_mbship_db_t *igmp_mbship_db , struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group);
int igmp_mbship_rm_client_groups_in_mbship_db( struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, long timeout);
int igmp_mbship_rm_mbship_db_groups_in_client(struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo);
struct igmp_mbship_preview_acl_t *igmp_mbship_search_preview_acl_list( struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group);
struct igmp_mbship_apgt_t * igmp_mbship_search_apgt_list( struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group);
int igmp_mbship_client_already_exist ( struct igmp_mbship_db_t * igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_mbship_client_access_time_update( struct igmp_mbship_db_t *igmp_mbship_db, struct igmp_clientinfo_t *igmp_clientinfo, struct igmp_group_t *igmp_group);
int igmp_mbship_list_sort(struct igmp_mbship_db_t *igmp_mbship_db);
int igmp_mbship_group_other_client_exist (struct igmp_mbship_db_t * igmp_mbship_db, struct igmp_group_t *igmp_group );
#endif

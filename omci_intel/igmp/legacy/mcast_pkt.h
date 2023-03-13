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
 * File    : mcast_pkt.h
 *
 ******************************************************************/

#ifndef __IGMP_PKT_H__
#define __IGMP_PKT_H__

#define FULL_ACL_AUTH		0
#define PREVIEW_ACL_AUTH	1
#define ALLOWED_PREVIEW_GROUP	2

#define IGMP_CLIENTINFO_SIGNATURE	0x27888118

struct igmp_group_t {
	union ipaddr		group_ip;
	union ipaddr		src_ip;
	unsigned char		msg_type;
	unsigned char 		record_type;
	unsigned char		allowed_type; // 0: full acl auth1: dynamic acl preview  2: allowed preview group
	unsigned short		preview_length;
	unsigned short 		preview_repeat_time;
	unsigned short 		preview_repeat_count;
	unsigned short 		preview_reset_time;

	struct list_head	entry_node; 
};

struct src_ip_t {
	union ipaddr		src_ip;
	struct list_head 	src_ip_list;
};

enum IGMP_PKT_STATE
{
        IGMP_PKT_PARSE_CLIENTINFO = 0,
        IGMP_PKT_BEFORE_TAGFILTER,
        IGMP_PKT_TAGFILTER,
        IGMP_PKT_AFTER_TAGFILTER,
        IGMP_PKT_BRIDGE,
        IGMP_PKT_DONE    
};   

enum IGMP_PKT_STATE_RESULT
{
        STOP = -1,
        CONTINUE = 0
};   
  
struct igmp_pkt_state {
        enum  IGMP_PKT_STATE            exec_state;
        enum  IGMP_PKT_STATE_RESULT     exec_ret;       
};

#define IGMP_PKT_STATE_SET(x, y)                x->state.exec_ret=0;x->state.exec_state=y;
#define IGMP_PKT_STATE_RESULT_SET(x, y)         x->state.exec_ret=y;

struct igmp_clientinfo_t {
	int 			signature;	// 0x27888118
	struct list_head	group_list;
	unsigned short		group_num;
	union ipaddr		client_ip;
	unsigned char 		client_mac[6];
	int			vid;
        int			ani_vid;
	struct cpuport_desc_t 	rx_desc;
	struct cpuport_info_t	*pktinfo;
	long 			join_time;
	long 			leave_time;
	long 			access_time;
	unsigned char 		igmp_version; 
	unsigned char 		igmp_msg_type; // leave/join/query
	unsigned char 		max_response_time; 
	unsigned int		ethertype;
	unsigned short 		tx_meid[8];
	struct cpuport_tci_t	tx_tci[8];
        struct igmp_pkt_state   state;
        unsigned char		*igmphdr;
};



/* igmp_pkt.c */
int igmp_pkt_tag_num_get(struct cpuport_info_t *pktinfo);
int igmp_pkt_recv_is_igmp_join_leave(struct cpuport_info_t *pktinfo);
int igmp_pkt_recv_is_igmp_query(struct cpuport_info_t *pktinfo);
int igmp_pkt_recv_is_mld_join_leave(struct cpuport_info_t *pktinfo);
int igmp_pkt_recv_is_mld_query(struct cpuport_info_t *pktinfo);
int igmp_pkt_parse_mld_type(unsigned char *frame_ptr, struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_pkt_tag_filter_iteration(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_pkt_clientinfo_str(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_pkt_parse_clientinfo(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_pkt_parse_clientinfo_router_mode(struct igmp_clientinfo_t *igmp_clientinfo);
int igmp_pkt_remove_igmp_v3_group(struct igmp_clientinfo_t *igmp_clientinfo, unsigned int uni_vid, struct in_addr group_ip, struct in_addr src_ip);
int igmp_pkt_remove_mld_v2_group(struct igmp_clientinfo_t *igmp_clientinfo, unsigned int uni_vid, struct in6_addr group_ipv6, struct in6_addr src_ipv6);
int igmp_pkt_ip_cmp(union ipaddr *src_ip, union ipaddr *dst_ip);
int igmp_pkt_ip_cpy(union ipaddr *dst_ip, union ipaddr *src_ip);
#endif

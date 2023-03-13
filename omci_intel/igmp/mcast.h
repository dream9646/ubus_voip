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
 * File    : mcast.h
 *
 ******************************************************************/

#ifndef __IGMP_PROXY_H__
#define __IGMP_PROXY_H__

#include "switch.h"
#include "mcast_pkt.h"
#include "fwk_timer.h"

#define TIMER_EVENT_QUERY_INTERVAL		1
#define TIMER_EVENT_QUERY_TIMEOUT_UPDATE	2
#define TIMER_EVENT_NONE_FAST_LEAVE		3
#define TIMER_EVENT_PREVIEW_CYCLE		4
#define TIMER_EVENT_ROUTER_NONE_FAST_LEAVE	5
#define TIMER_EVENT_ROUTER_COMPATIBITY          6
#define TIMER_EVENT_CUSTOM			0xF0

#define PREVIEW_CYCLE				10

#define IGMP_EXIT	1


enum igmp_cmd_t {
	IGMP_CMD_RUN_PKT,
	IGMP_CMD_OMCI,
	IGMP_CMD_ME_RELOAD,
	IGMP_CMD_ENV,
	IGMP_CMD_TERMINATE
};

struct env_info_t {
	char env_name[128];
	int value;
};

struct igmp_cmd_msg_t {
	enum igmp_cmd_t cmd;
	void *usr_data;
	struct list_head q_entry;
};

#define ENV_SIZE_IFNAME			16
#define ENV_SIZE_PROPRIETARY_VENDOR	16
#define ENV_SIZE_IGMP_PREASSIGN_LIST	8
 
struct igmp_env_igmp_preassign_t {
	unsigned char mac[6];		// for L2 mcast preassign 
	unsigned char mask[6];
	unsigned int ipaddr;		// for ipv4 mcast preassign
	unsigned int ipaddr_mask;
};

struct mcast_mbship_t {
	unsigned short		src_port_id;	// in omci refers to meid
	unsigned int 		uni_vid;
	unsigned int 		ani_vid;
	union ipaddr		group_ip;
	union ipaddr		src_ip;
	union ipaddr		client_ip;
	unsigned char 		client_mac[6];
	unsigned char 		allowed_type; // 0: full acl auth1: dynamic acl preview  2: allowed preview group
	unsigned char		logical_port_id;
	unsigned int		hw_port_mask;
	unsigned short		preview_repeat;
	long	 		access_time;
	unsigned short 		p2p_cfg;
	struct list_head 	entry_node;
	struct list_head 	entry_node_v3;
};

struct igmp_grprec_t
{
	unsigned int 		flag;
	union ipaddr		client_ip;
	struct igmp_group_t	*rec;	
};

struct igmp_counter_module_t {
        const char * (*module_name)(void);
	int (*init)(void);
	int (*reset)(void);
        int (*igmp_counter_handler)(struct igmp_clientinfo_t *igmp_clientinfo);
        int (*igmp_cli_handler)(int);
        int (*igmp_counter_iterator)(int (*iterator_function)(), void *iterator_data);
        struct list_head m_node;
};

#define MCAST_FILTER_CONTINUE	0
#define MCAST_FILTER_STOP	-1
#define MCAST_FILTER_INVALID_MASK	0xffff0000

#define NO_IGMP_SWITCH_FORWARD	1	 

#define FILTER_EXECUTE(func, ptr) {				\
	struct igmp_filter_module_t *f; 				\
	dbprintf_igmp(LOG_INFO,"Execute [" #func "]\n");		\
	list_for_each_entry(f, &igmp_config_g.m_filter_list, m_node) { 	\
		int ret = 0;						\
		if (f->func == NULL) continue;				\
		ret = f->func(ptr);                         \
		dbprintf_igmp(LOG_INFO,"%s -> %s\n", f->filter_name(), (ret)?"fail":"success");   \
	        if (ret) return -1;		\
	} }

#define MCAST_FILTER_NO_IPV6(obj)	if (obj && IS_IPV6_ADDR(obj->client_ip)) return MCAST_FILTER_CONTINUE; 
	
struct igmp_filter_module_t {
	short priority;
        const char * (*filter_name)(void);
	int (*init)(void);
	int (*deinit)(void);
	int (*raw_filter)(struct cpuport_info_t *pktinfo);		// called before pkt being parsed
	int (*pre_filter)(struct igmp_clientinfo_t *igmp_clientinfo);	// called after pkt being parsed
        int (*filter)(struct igmp_grprec_t *r);				// called per parsed group record
        int (*forward)(struct mcast_mbship_t *h);	// called before a member portmask is determined
        int (*cli)(int fd, int argc, char *argv[]);			// cli callback
        struct list_head m_node;
};

struct igmp_daemon_t {
        const char * (*daemon_name)(void);
        void (*igmp_daemon_init)();
        void (*igmp_mainloop_before)();
        int (*igmp_joinleave_hook)(struct cpuport_info_t *pktinfo);
        int (*igmp_query_hook)(struct cpuport_info_t *pktinfo);
        int (*igmp_timer_hook)(struct fwk_timer_msg_t *timer_msg);
        int (*igmp_cmd_hook)(struct igmp_cmd_msg_t *igmp_cmd_msg);
        int (*igmp_cli_hook)(int fd, int argc, char *argv[]);
        void (*igmp_mainloop_after)();
        void (*igmp_daemon_deinit)();
};

struct igmp_config_t {
	unsigned char igmp_function; //0: transparent IGMP snooping
				     //1: IGMP snooping with proxy reporting
				     //2: IGMP proxy	
	unsigned char igmp_version;
	unsigned char igmp_compatibity_mode;   //0: disable
	                                       //1: enable
	
	long max_respon_time;
	long query_interval;
	long last_member_query_interval;
	unsigned int group_limit; 
	unsigned char robustness;
	unsigned char immediate_leave;
	int max_group_member;

	unsigned char mac_addr[6];
	char querier_ip[16];
	unsigned int upstream_igmp_rate;
	
	unsigned short upstream_igmp_tci;
	unsigned char upstream_igmp_tag_ctl;
	unsigned short downstream_igmp_tci;
	unsigned char downstream_igmp_tag_ctl;
	int max_ssm_source;
	
	// used on startup..
	unsigned int startupqueryinterval;
	unsigned int startupquerycount;
	// Last member probe...
	unsigned int lastmemberquerycount;
        
	struct igmp_daemon_t *daemon; 
        struct list_head m_counter_list;
        struct list_head m_filter_list;
        int (*cli_hook)(int fd, int argc, char *argv[]);
        int (*tag_filter_iteration)(struct igmp_clientinfo_t *);
        int (*mcast_TX_L2)(struct igmp_clientinfo_t *);
};

/* igmp_proxy.c */
int igmp_proxy_init(void);
int igmp_proxy_shutdown(void);
int igmp_proxy_start(void);
int igmp_proxy_stop(void);
int igmp_proxy_send_cmd(enum igmp_cmd_t code, void *data);
int igmp_proxy_env_cmd(char *cmd_str, int cmd_value);
int igmp_proxy_reload(void);

extern struct igmp_config_t igmp_config_g; 
extern struct igmp_config_t igmp_old_config_g; 
extern int igmpproxytask_loop_g;
extern int igmpproxytask_dummy_fd_g[2];

extern int igmp_join_leave_qid_g;
extern int igmp_query_qid_g;
extern int igmp_timer_qid_g;
extern int igmp_cmd_qid_g;
extern int igmp_qsetid_g;

int igmp_pkt_upstream_tci_ctl(struct igmp_config_t *igmp_config, struct cpuport_info_t *pktinfo, struct cpuport_tci_t *dst_tci);
int igmp_pkt_downstream_tci_ctl(struct igmp_config_t *igmp_config, struct cpuport_info_t *pktinfo, struct cpuport_tci_t *dst_tci);
int mcast_register_tag_filter(int (*cb)(struct igmp_clientinfo_t *));
int mcast_register_TX_L2(int (*cb)(struct igmp_clientinfo_t *));

#endif

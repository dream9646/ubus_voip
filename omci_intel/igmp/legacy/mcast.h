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
	IGMP_CMD_ME_ADD_MSG,
	IGMP_CMD_ME_MODIFY_MSG,
	IGMP_CMD_ME_DEL_MSG,
	IGMP_CMD_ME_RELOAD,
	IGMP_CMD_TERMINATE
};

struct igmp_cmd_msg_t {
	enum igmp_cmd_t cmd;
	unsigned int msglen;
	unsigned char *usr_data;
	unsigned short classid;
	unsigned short meid;
	struct list_head q_entry;
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
        int (*cli_hook)(int fd, int argc, char *argv[]);
};


/* igmp_proxy.c */
int igmp_proxy_init(void);
int igmp_proxy_shutdown(void);
int igmp_proxy_start(void);
int igmp_proxy_stop(void);
int igmp_proxy_uni_ports_nums_get(void);
int igmp_proxy_env_cmd(char *cmd_str, int cmd_value);
int igmp_proxy_reload(void);

extern struct igmp_config_t igmp_config_g; 
extern struct igmp_config_t igmp_old_config_g; 
extern int igmpproxytask_loop_g ;
extern int igmpproxytask_dummy_fd_g[2];

extern int igmpproxy_uni_ports_g;
extern int igmpproxy_wan_ports_g;

extern int igmp_join_leave_qid_g ;
extern int igmp_query_qid_g ;
extern int igmp_timer_qid_g ;
extern int igmp_cmd_qid_g ;
extern int igmp_qsetid_g  ;

#endif

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
 * File    : igmp_proxy.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"

#include "mcast_const.h"
#include "mcast.h"
#include "mcast_switch.h"
#include "mcast_main.h"
#include "cpuport_extract_filter.h"
#include "mcast_snoop.h"
#include "mcast_cli.h"
#include "platform.h"


int igmpproxytask_loop_g = 0;
int igmpproxytask_dummy_fd_g[2];
static struct fwk_thread_t igmpproxytask_thread_g;

unsigned char igmpproxy_active_port_mask_g=0x0;

int igmp_join_leave_qid_g = -1;
int igmp_query_qid_g = -1;
int igmp_timer_qid_g = -1;
int igmp_cmd_qid_g = -1;
int igmp_qsetid_g = -1;

struct igmp_config_t igmp_config_g = {
	// cli_hook
	.cli_hook = igmp_cli_cmdline,
	.daemon = &igmpv3_lw,
	
	.max_respon_time = 100,
	.query_interval = 125,
	.robustness = 2,
	.immediate_leave = TRUE,
	.igmp_version = IGMP_V2_BIT,
	.igmp_compatibity_mode = 0,
	.igmp_function = IGMP_FUNCTION_TRANSPARENT,
	.mac_addr = {0x00, 0x46, 0x76, 0x08, 0x56, 0x00},
	.querier_ip = "0.0.0.0",
	.upstream_igmp_rate = 3,
	.upstream_igmp_tci = 0,
	.upstream_igmp_tag_ctl = 0,
	.downstream_igmp_tci = 0,
	.downstream_igmp_tag_ctl = 0,
	.last_member_query_interval = 10,
	.group_limit = 128,
	.max_ssm_source = 8,
	.max_group_member = 25,
	// Used on startup..
	.startupqueryinterval = 10,
	.startupquerycount = 1,
	// Last member probe...
	.lastmemberquerycount = 2
};

struct igmp_config_t igmp_old_config_g = {
	// cli_hook
	.cli_hook = igmp_cli_cmdline,
	.daemon = &igmpv3_lw,
	
	.max_respon_time = 100,
	.query_interval = 125,
	.robustness = 2,
	.immediate_leave = TRUE,
	.igmp_version = IGMP_V2_BIT,
	.igmp_function = IGMP_FUNCTION_TRANSPARENT,
	.mac_addr = {0x00, 0x46, 0x76, 0x08, 0x56, 0x00},
	.querier_ip = "0.0.0.0",
	.upstream_igmp_rate = 3,
	.upstream_igmp_tci = 0,
	.upstream_igmp_tag_ctl = 0,
	.downstream_igmp_tci = 0,
	.downstream_igmp_tag_ctl = 0,
	.last_member_query_interval = 10,
	.max_group_member = 25,
	.group_limit = 0,
	.max_ssm_source = 8,
	// Used on startup..
	.startupqueryinterval = 10,
	.startupquerycount = 1,
	// Last member probe...
	.lastmemberquerycount = 2
};

static int
igmp_queue_init(void)
{
	int ret = -1;
	if ((igmp_join_leave_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_IGMP_JOIN_LEAVE" ) ) < 0)
	{
		dbprintf_igmp(LOG_ERR, "igmp pkt queue alloc error(%d)\n",igmp_join_leave_qid_g);
		return (-1);
	}

	if ((igmp_query_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_IGMP_QUERY" )) <0)
	{
		fwk_msgq_release(igmp_join_leave_qid_g);
		dbprintf_igmp(LOG_ERR, "igmp pkt queue alloc error(%d)\n",igmp_query_qid_g);
		return (-1);
	}

	if ((igmp_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_IGMP_TIMER" )) < 0)
	{
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		dbprintf_igmp(LOG_ERR, "igmp timer queue alloc error(%d)\n",igmp_timer_qid_g);
		return (-1);
	}

	if ((igmp_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_IGMP_CMD" )) < 0)
	{
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		fwk_msgq_release(igmp_timer_qid_g);
		dbprintf_igmp(LOG_ERR, "igmp cmd queue alloc error(%d)\n",igmp_cmd_qid_g);
		return (-1);
	}

	if ((igmp_qsetid_g = fwk_msgq_set_alloc()) < 0)
	{
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		fwk_msgq_release(igmp_timer_qid_g);
		fwk_msgq_release(igmp_cmd_qid_g);
		dbprintf_igmp(LOG_ERR, "queue set alloc error(%d)\n", igmp_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( igmp_qsetid_g )) < 0)
	{
		fwk_msgq_set_release(igmp_qsetid_g);
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		fwk_msgq_release(igmp_timer_qid_g);
		fwk_msgq_release(igmp_cmd_qid_g);
		dbprintf_igmp(LOG_ERR, "queue set zero error(%d)\n", igmp_qsetid_g);
		return -1;
	}

	if ((ret = fwk_msgq_set_set( igmp_qsetid_g ,igmp_join_leave_qid_g )) < 0)
	{
		fwk_msgq_set_release(igmp_qsetid_g);
		fwk_msgq_release(igmp_join_leave_qid_g );
		fwk_msgq_release(igmp_query_qid_g );
		fwk_msgq_release(igmp_timer_qid_g );
		fwk_msgq_release(igmp_cmd_qid_g );
		dbprintf_igmp(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( igmp_qsetid_g ,igmp_query_qid_g ))<0 )
	{
		fwk_msgq_set_release(igmp_qsetid_g);
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		fwk_msgq_release(igmp_timer_qid_g);
		fwk_msgq_release(igmp_cmd_qid_g);
		dbprintf_igmp(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( igmp_qsetid_g ,igmp_timer_qid_g)) < 0)
	{
		fwk_msgq_set_release(igmp_qsetid_g);
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		fwk_msgq_release(igmp_timer_qid_g);
		fwk_msgq_release(igmp_cmd_qid_g);
		dbprintf_igmp(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set(igmp_qsetid_g ,igmp_cmd_qid_g)) < 0)
	{
		fwk_msgq_set_release(igmp_qsetid_g);
		fwk_msgq_release(igmp_join_leave_qid_g);
		fwk_msgq_release(igmp_query_qid_g);
		fwk_msgq_release(igmp_timer_qid_g);
		fwk_msgq_release(igmp_cmd_qid_g );
		dbprintf_igmp(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	return 0;
}

static int
igmp_queue_deinit(void)
{
	if (igmp_qsetid_g != -1 )
	{
		if (igmp_join_leave_qid_g != -1)
		{
			fwk_msgq_set_clear(igmp_qsetid_g, igmp_join_leave_qid_g);
			fwk_msgq_release(igmp_join_leave_qid_g);
		}

		if (igmp_query_qid_g != -1)
		{
			fwk_msgq_set_clear(igmp_qsetid_g, igmp_query_qid_g);
			fwk_msgq_release(igmp_query_qid_g);
		}

		if ( igmp_timer_qid_g != -1 )
		{
			fwk_msgq_set_clear(igmp_qsetid_g, igmp_timer_qid_g);
			fwk_msgq_release(igmp_timer_qid_g);
		}

		if (igmp_cmd_qid_g != -1)
		{
			fwk_msgq_set_clear(igmp_qsetid_g, igmp_cmd_qid_g);
			fwk_msgq_release(igmp_cmd_qid_g);
		}
		fwk_msgq_set_release(igmp_qsetid_g);
	}

	return 0;
}

int
igmp_proxy_init(void)
{
	igmp_switch_init();
        
	igmp_queue_init();
	
	igmp_config_g.daemon->igmp_daemon_init();
	
	return 0;
}

int
igmp_proxy_start(void)
{
	int ret;

	igmpproxytask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	igmpproxytask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_HI;
	igmpproxytask_thread_g.stack_size = 128*1024;
	igmpproxytask_thread_g.join = 1;
	sprintf(igmpproxytask_thread_g.name, THREAD_NAME);
	igmpproxytask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&igmpproxytask_thread_g, igmp_main, NULL)) < 0)
	{
		dbprintf_igmp(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}

	if (cpuport_extract_filter_register(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE, "igmp_join_leave", igmp_pkt_recv_is_igmp_join_leave, igmp_join_leave_qid_g))
	{
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register igmp_join_leave failed\n");
		return -1;
	}	
	if (cpuport_extract_filter_register(CPUPORT_EXTRACT_IGMP_QUERY, "igmp_query", igmp_pkt_recv_is_igmp_query, igmp_query_qid_g))
	{
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE);
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register igmp_query failed\n");
		return -1;
	}
	if (cpuport_extract_filter_register(CPUPORT_EXTRACT_MLD_JOIN_LEAVE, "mld_join_leave", igmp_pkt_recv_is_mld_join_leave, igmp_join_leave_qid_g))
	{
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE);
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_QUERY);
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register mld_join_leave failed\n");
		return -1;
	}		
	if (cpuport_extract_filter_register(CPUPORT_EXTRACT_MLD_QUERY, "mld_query", igmp_pkt_recv_is_mld_query, igmp_query_qid_g))
	{
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE);
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_QUERY);
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_MLD_JOIN_LEAVE);
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register mld_query failed\n");
		return -1;
	}
	return 0;
}

int
igmp_proxy_stop(void)
{
	int ret;

	cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE);
	cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_QUERY);
	cpuport_extract_filter_unregister(CPUPORT_EXTRACT_MLD_JOIN_LEAVE);
	cpuport_extract_filter_unregister(CPUPORT_EXTRACT_MLD_QUERY);

	igmpproxytask_loop_g = 0;	//start loop

	struct igmp_cmd_msg_t *igmp_cmd_msg;
	igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t));
	igmp_cmd_msg->cmd = IGMP_CMD_TERMINATE;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	if ((ret = fwk_thread_join(&igmpproxytask_thread_g)) < 0)
	{
		dbprintf_igmp(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}

int
igmp_proxy_shutdown(void)
{
	igmp_switch_shutdown();
	igmp_queue_deinit();
	        
        igmp_config_g.daemon->igmp_daemon_deinit();
        
	return 0;
}

int
igmp_proxy_send_cmd(enum igmp_cmd_t code, void *data)
{
	struct igmp_cmd_msg_t *igmp_cmd_msg;
	if ((igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t))) == NULL) return -1;
	igmp_cmd_msg->cmd = code;
	igmp_cmd_msg->usr_data = data;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	return 0;
}

int
igmp_proxy_env_cmd(char *cmd_str, int cmd_value)
{
	struct env_info_t *p = malloc_safe(sizeof (struct env_info_t));
	strcpy(p->env_name, cmd_str);
	p->value = cmd_value; 
	igmp_proxy_send_cmd(IGMP_CMD_ENV, (void *)p);
	
	return 0;
}

int
igmp_proxy_reload(void)
 {
	struct igmp_cmd_msg_t *igmp_cmd_msg;
	if ((igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t))) == NULL) return -1;
	igmp_cmd_msg->cmd = IGMP_CMD_ME_RELOAD;
	igmp_cmd_msg->usr_data = NULL;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	return 0;
}

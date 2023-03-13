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

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"

#include "cpuport.h"
#include "switch.h"
#include "mcast_const.h"
#include "mcast.h"
#include "mcast_pkt.h"
#include "igmp_mbship.h"
#include "switch.h"
#include "mcast_switch.h"
#include "mcast_main.h"
#include "mcast_timer.h"
#include "mcast_router.h"
#include "mcast_proxy.h"
#include "cpuport_extract_filter.h"
#include "mcast_snoop.h"
#include "igmp_omci.h"
#include "mcast_cli.h"
#include "counter/igmp_counter_simple.h"
#include "counter/igmp_counter_calix.h"
#include "counter/igmp_counter_calix_proxy.h"
#include "counter/igmp_counter_calix_eventlog.h"
#include "counter/igmp_counter_calix_proxy_eventlog.h"
#include "counter/igmp_counter_calix_15mins.h"
#include "metacfg_adapter.h"

int igmpproxytask_loop_g = 0;
int igmpproxytask_dummy_fd_g[2];
static struct fwk_thread_t igmpproxytask_thread_g;

unsigned char igmpproxy_active_port_mask_g=0x0;

int igmp_join_leave_qid_g = -1;
int igmp_query_qid_g = -1;
int igmp_timer_qid_g = -1;
int igmp_cmd_qid_g = -1;
int igmp_qsetid_g = -1;
int layer2bridged = 0;

struct igmp_config_t igmp_config_g = {
	// cli_hook
	.cli_hook = igmp_cli_cmdline,
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
	.group_limit = 128,
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

static void calix_igmp_config_from_file(char *mode)
{

	struct metacfg_t kv_config;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	int ret = metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "brwan0_name", "igmp_function");
	
	if (ret == -1)
	{
		metacfg_adapter_config_release(&kv_config);
		dbprintf_igmp(LOG_WARNING, "no igmp metakeys\n" );
		return;
	}

	char *tmp;	
	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpVer", 0);
	if (tmp != NULL)
	{
		switch (util_atoi(tmp))
		{
		case 1:
			igmp_config_g.igmp_version = IGMP_V1_BIT;
			break;
		case 2:
			igmp_config_g.igmp_version = IGMP_V2_BIT;
			break;
		case 3:
			igmp_config_g.igmp_version = IGMP_V3_BIT;
			break;
		}
	}
	
	// query interval is in second, max response/last query interval are in 0.1sec
	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpQI", 0);
	if (tmp != NULL) igmp_config_g.query_interval =  util_atoi(tmp);

	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpQRI", 0);
	if (tmp != NULL) igmp_config_g.max_respon_time =  util_atoi(tmp);
	
	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpLMQI", 0);
	if (tmp != NULL) igmp_config_g.last_member_query_interval =  util_atoi(tmp);
	
	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpRV", 0);
	if (tmp != NULL) igmp_config_g.robustness =  util_atoi(tmp);	

	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpMaxGroups", 0);
	if (tmp != NULL) igmp_config_g.group_limit =  util_atoi(tmp);	
	
	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpMaxMembers", 0);
	if (tmp != NULL) igmp_config_g.max_group_member =  util_atoi(tmp);

	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpMaxSources", 0);
	if (tmp != NULL) igmp_config_g.max_ssm_source =  util_atoi(tmp);

	tmp = metacfg_adapter_config_get_value(&kv_config, "igmpFastLeaveEnable", 0);
	if (tmp != NULL) igmp_config_g.immediate_leave =  util_atoi(tmp);		

	tmp = metacfg_adapter_config_get_value(&kv_config, "igmp_function", 1);
	strcpy(mode, tmp);

	int i;
	char tmp2[64];
	layer2bridged = 0;
	for (i=0;i < 8;i++)
	{
		sprintf(tmp2,  "brwan%d_igmp_enable", i);
		if (util_atoi(metacfg_adapter_config_get_value(&kv_config, tmp2, 1)) == 1)
		{
			layer2bridged = 1;
			break;
		}	
	}
		
	metacfg_adapter_config_release(&kv_config);
}

int
igmp_proxy_init(void)
{
	igmp_switch_init();
	igmp_mbship_db_init(&igmp_mbship_db_g);
        
        INIT_LIST_HEAD(&igmp_config_g.m_counter_list);
        
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		char mode[32];
		calix_igmp_config_from_file(mode);
		
		if (strcmp(mode, "proxy") == 0)
		{
			list_add_tail(&calix_proxy.m_node, &igmp_config_g.m_counter_list);
			list_add_tail(&calix_proxy_eventlog.m_node, &igmp_config_g.m_counter_list);
		}
                else
                {
                	list_add_tail(&calix.m_node, &igmp_config_g.m_counter_list);
                	list_add_tail(&calix_eventlog.m_node, &igmp_config_g.m_counter_list);
                	list_add_tail(&calix_15mins.m_node, &igmp_config_g.m_counter_list);
                }
                
                if (strcmp(mode, "proxy") == 0 && omci_env_g.igmp_proxy_snooping == 0)
                {
                	// daemon is started in proxy but there is a short time in snooping mode
                	// ideally it shouldn't accept any packet before router_mode_init()
                	dbprintf_igmp(LOG_ERR, "halt igmp intake %x\n",  (1<<ENV_TASK_NO_IGMP));
                	omci_env_g.task_pause_mask |= (1<<ENV_TASK_NO_IGMP);
		}
		else
			omci_env_g.task_pause_mask &= ~(1<<ENV_TASK_NO_IGMP);
        }
        else
		list_add_tail(&simple.m_node, &igmp_config_g.m_counter_list);

	igmp_queue_init();
	
	struct igmp_counter_module_t *m=NULL;
        
        list_for_each_entry(m,  &igmp_config_g.m_counter_list, m_node) {
        	m->init();            
        }
	
	igmp_omci_rm_all_active_group_list_group();
	
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
	sprintf(igmpproxytask_thread_g.name, "omci-IGMP");
	igmpproxytask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&igmpproxytask_thread_g, igmp_main, NULL)) < 0)
	{
		dbprintf_igmp(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}

	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE, "igmp_join_leave", igmp_pkt_recv_is_igmp_join_leave, igmp_join_leave_qid_g))
	{
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register igmp_join_leave failed\n");
		return -1;
	}	
	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_IGMP_QUERY, "igmp_query", igmp_pkt_recv_is_igmp_query, igmp_query_qid_g))
	{
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE);
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register igmp_query failed\n");
		return -1;
	}
	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_MLD_JOIN_LEAVE, "mld_join_leave", igmp_pkt_recv_is_mld_join_leave, igmp_join_leave_qid_g))
	{
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_JOIN_LEAVE);
		cpuport_extract_filter_unregister(CPUPORT_EXTRACT_IGMP_QUERY);
		dbprintf_igmp(LOG_ERR, "cpuport_extract_filter_register mld_join_leave failed\n");
		return -1;
	}		
	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_MLD_QUERY, "mld_query", igmp_pkt_recv_is_mld_query, igmp_query_qid_g))
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
	
	return 0;
}

int
igmp_proxy_add_cmd(int igmp_cmd_qid_g, unsigned short classid, unsigned short meid )
{
	struct igmp_cmd_msg_t *igmp_cmd_msg;
	if ((igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t))) == NULL) return -1;
	igmp_cmd_msg->cmd = IGMP_CMD_ME_ADD_MSG;
	igmp_cmd_msg->classid = classid;
	igmp_cmd_msg->meid = meid;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	return 0;
}

int
igmp_proxy_modify_cmd(int igmp_cmd_qid_g, unsigned short classid, unsigned short meid)
{
	struct igmp_cmd_msg_t *igmp_cmd_msg;
	if ((igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t))) == NULL)
		return -1;
	igmp_cmd_msg->cmd = IGMP_CMD_ME_MODIFY_MSG;
	igmp_cmd_msg->classid = classid;
	igmp_cmd_msg->meid = meid;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	return 0;
}

int
igmp_proxy_del_cmd(int igmp_cmd_qid_g, unsigned short classid, unsigned short meid)
{
	struct igmp_cmd_msg_t *igmp_cmd_msg;
	if ((igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t))) == NULL)
		return -1;
	igmp_cmd_msg->cmd = IGMP_CMD_ME_DEL_MSG;
	igmp_cmd_msg->classid = classid;
	igmp_cmd_msg->meid = meid;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	return 0;
}

int igmp_proxy_env_cmd(char *cmd_str, int cmd_value)
{
	return 0;	// bogus function
}

int
igmp_proxy_reload(void)
{
	struct igmp_cmd_msg_t *igmp_cmd_msg;
	if ((igmp_cmd_msg = malloc_safe(sizeof (struct igmp_cmd_msg_t))) == NULL) return -1;
	igmp_cmd_msg->cmd = IGMP_CMD_ME_RELOAD;
	igmp_cmd_msg->classid = -1;
	igmp_cmd_msg->meid = -1;
	INIT_LIST_NODE(&igmp_cmd_msg->q_entry);
	if (fwk_msgq_send(igmp_cmd_qid_g, &igmp_cmd_msg->q_entry) < 0)
	{
		free_safe(igmp_cmd_msg);
	}
	return 0;
}

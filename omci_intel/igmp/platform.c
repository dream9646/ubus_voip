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
 * File    : platform.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "mcast_const.h"
#include "mcast.h"
#include "metacfg_adapter.h"

#include "counter/igmp_counter_simple.h"
#include "counter/igmp_counter_calix.h"
#include "counter/igmp_counter_calix_proxy.h"
#include "counter/igmp_counter_calix_eventlog.h"
#include "counter/igmp_counter_calix_proxy_eventlog.h"
#include "counter/igmp_counter_calix_15mins.h"
#include "filter/2511b.h"
#include "filter/preassign.h"
#include "filter/calixlimit.h"
#include "filter/alu_signal.h"
#include "filter/alu_p2p.h"
#include "filter/calix_txme.h"
#include "filter/mcast_version.h"
#include "filter/port_validator.h"
#include "filter/igmp_omci.h"
#include "filter/pktdbg.h"

unsigned short alu_olt_ethertype = 0;
 
static void calix_igmp_config_from_file(char *mode)
{
#ifdef OMCI_METAFILE_ENABLE
	struct metacfg_t kv_config;

	memset(&kv_config, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config);
	int ret = metacfg_adapter_config_file_load_part(&kv_config, "/etc/wwwctrl/metafile.dat", "igmpVer", "igmp_function");
	
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
		case 4:
			igmp_config_g.igmp_version = MLD_V1_BIT;
			break;
		case 5:
			igmp_config_g.igmp_version = MLD_V2_BIT;
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
		
	metacfg_adapter_config_release(&kv_config);		
#endif
}

int module_filter_specific_add()
{
        INIT_LIST_NODE(&preassign_filter.m_node);
	list_add_tail(&preassign_filter.m_node, &igmp_config_g.m_filter_list);
        INIT_LIST_NODE(&pktdbg_filter.m_node);
	list_add_tail(&pktdbg_filter.m_node, &igmp_config_g.m_filter_list);
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0)
	{
		INIT_LIST_NODE(&alu_signal_filter.m_node);
		list_add_tail(&alu_signal_filter.m_node, &igmp_config_g.m_filter_list);
	}
	
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	{
		INIT_LIST_NODE(&calixlimit_filter.m_node);
		list_add_tail(&calixlimit_filter.m_node, &igmp_config_g.m_filter_list);    
	        INIT_LIST_NODE(&calix_txme_filter.m_node);
		list_add_tail(&calix_txme_filter.m_node, &igmp_config_g.m_filter_list);        
	}
	INIT_LIST_NODE(&omci_filter.m_node);		
	list_add_tail(&omci_filter.m_node, &igmp_config_g.m_filter_list);
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_ALU) == 0)
	{
		INIT_LIST_NODE(&alu_p2p_filter.m_node);
		list_add_tail(&alu_p2p_filter.m_node, &igmp_config_g.m_filter_list);
	}
	INIT_LIST_NODE(&x2511b_filter.m_node);
	list_add_tail(&x2511b_filter.m_node, &igmp_config_g.m_filter_list);
	INIT_LIST_NODE(&mcast_version.m_node);
	list_add_tail(&mcast_version.m_node, &igmp_config_g.m_filter_list);
	INIT_LIST_NODE(&port_validator.m_node);
	list_add_tail(&port_validator.m_node, &igmp_config_g.m_filter_list);
	
	return 0;
}

int module_counter_specific_add()
{
	if (strcmp(omci_env_g.olt_proprietary_support, ENV_OLT_PROPRIETARY_CALIX) == 0)
	//if(1)
	{
		char mode[32];
		calix_igmp_config_from_file(mode);
		
		if (strcmp(mode, "proxy") == 0)
		{
			INIT_LIST_NODE(&calix_proxy.m_node);
			list_add_tail(&calix_proxy.m_node, &igmp_config_g.m_counter_list);
			INIT_LIST_NODE(&calix_proxy_eventlog.m_node);
			list_add_tail(&calix_proxy_eventlog.m_node, &igmp_config_g.m_counter_list);
		}
                else
                {
                	INIT_LIST_NODE(&calix.m_node);
			list_add_tail(&calix.m_node, &igmp_config_g.m_counter_list);
			INIT_LIST_NODE(&calix_eventlog.m_node);
                	list_add_tail(&calix_eventlog.m_node, &igmp_config_g.m_counter_list);
                	INIT_LIST_NODE(&calix_15mins.m_node);
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
        {
		INIT_LIST_NODE(&simple.m_node);
		list_add_tail(&simple.m_node, &igmp_config_g.m_counter_list);
	}
	
	return 0;
}


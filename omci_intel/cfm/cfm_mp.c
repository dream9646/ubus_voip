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
 * Module  : cfm
 * File    : cfm_mp.c
 *
 ******************************************************************/
 
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "util.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_frame.h"
#include "cfm_core.h"
#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_switch.h"
#include "cfm_print.h"
#include "cfm_send.h"

static unsigned char cfm_mp_index[CFM_MAX_MP];
struct list_head cfm_config_list;

// a pseudo mep to generate AIS for pkt not handled by any mep
cfm_config_t server_mep_cfm_config_g;
unsigned int server_mep_logical_portmask_g;

cfm_chassis_t 
cfm_chassis_g = {
	.chassis_id_length = 0,
	.chassis_id_subtype = CHASSIS_ID_SUBTYPE_LOCAL,
	.chassis_id = "",
	.mgmtaddr_domain_length = 0,
	.mgmtaddr_domain = "",
	.mgmtaddr_length = 0,
	.mgmtaddr = ""
};

static cfm_config_t 
cfm_config_default = {
	.port_type = 0,
	.port_type_index = 0,
	.auto_discovery = 1,

	.nextLBMtransID = 0,
	.nextLTMtransID = 0,
	.CCIsentCCMs = 0,
	.tst_sequence_number = 0,
	.tlv_tst_length = 64,
	.payload_format=0,	//0:skip option tlv
	.payload_pattern = 0,
	.tlv_data_length = 64,
	.send_lb_senderid = 1,
	.send_lt_senderid = 1,
	.send_cc_senderid = 1,
	.send_lb_port_status = 0,
	.send_lt_port_status = 0,
	.send_cc_port_status = 1,
	.send_lb_interface_status = 0,
	.send_lt_interface_status = 0,
	.send_cc_interface_status = 1,
	.send_lb_data_status = 1,
	.send_lt_data_status = 0,
	.send_cc_data_status = 0,

	.cfm_send_lmm_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	.cfm_send_dmm_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	.cfm_recv_dmr_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	.cfm_send_lbm_timestamp = 0,
	.TxTimestampf = 0,
	.RxTimestampf = 0,
	.TxTimestampb = 0,
	.RxTimestampb = 0,
	.frame_delay = 0,
	
	.frame_delay_min = 0,
	.frame_delay_max = 0,
	.frame_delay_avg = 0,
	.round_trip_min = 0,
	.round_trip_max = 0,
	.round_trip_avg = 0,
	.response_time_min = 0,
	.response_time_max = 0,
	.response_time_avg = 0,

	.md_meid = 0,
	.md_level = 5,
	.md_format = MD_NAME_FORMAT_STRING,
	.md_name_1 = "",
	.md_name_2 = "",
	.md_mhf_creation = MA_MHF_CREATION_NONE,
	.md_sender_id_permission = MA_SENDER_ID_PERMISSION_NONE,

	.ma_meid = 0,
	.ma_format = MA_NAME_FORMAT_STRING,
	.ma_name_1 = "",
	.ma_name_2 = "",
	.ccm_interval = CCM_INTERVAL_DISABLED,
	.assoc_vlans = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.ma_mhf_creation = MA_MHF_CREATION_DEFER,
	.ma_sender_id_permission = MA_SENDER_ID_PERMISSION_DEFER,

	.mep_meid = 0,
	.mep_id = 0,
	.mep_control = MEP_CONTROL_CCM_ENABLE,
	.mep_control2 = MEP_CONTROL2_LBR_ENABLE | MEP_CONTROL2_LTR_ENABLE,
	.pri_vlan = 0,
	.pri_pbit = 0,
	.pbit_filter = 0,
	.admin_state = 0,
	.priority = 0,
	.egress_id = {0, 0, 0, 0, 0, 0, 0, 0},
	.peer_mep_ids = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	.eth_ais_control = 0,
	.fault_alarm_threshold = 0,
	.alarm_declaration_soak_time = 2500,
	.alarm_clear_soak_time = 10000
};

static int
cfm_config_init(cfm_config_t *cfm_config, int index, unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index)
{
	struct switch_port_info_t port_info;

	*cfm_config = cfm_config_default;
	memset(cfm_config->cfm_pkt_pm, 0, sizeof(cfm_config->cfm_pkt_pm));
	
	cfm_config->index = index;
	cfm_config->md_level = md_level;
	cfm_config->mep_id = mep_id;
	cfm_config->port_type = port_type;
	cfm_config->port_type_index = port_type_index;

	INIT_LIST_HEAD(&cfm_config->cfm_recv_lbr_list);
	INIT_LIST_HEAD(&cfm_config->cfm_recv_ltm_list);
	INIT_LIST_HEAD(&cfm_config->cfm_recv_ltr_list);
	INIT_LIST_HEAD(&cfm_config->cfm_recv_rmep_list);

	// find logical port id by port_type & port_type_index
	if (switch_get_port_info_by_port_type_index(port_type, port_type_index, &port_info) < 0) {
		dbprintf_cfm(LOG_WARNING, "port type %d, port_type_index %d not found?\n", port_type, port_type_index);
		cfm_config->logical_port_id = port_type_index;
		//return -1;
	} else 
		cfm_config->logical_port_id = port_info.logical_port_id;
	cfm_util_get_mac_addr(cfm_config, cfm_config->macaddr, omci_env_g.cfm_mac_ifname);
	
	return 0;
}

int
cfm_config_list_init(void)
{
	INIT_LIST_HEAD(&cfm_config_list);
	memset(cfm_mp_index, 0, CFM_MAX_MP);

	// init server mep
	{
		unsigned char ais_md_level = 0;	// md level where client mep is located, 0 means auto search
		unsigned char ais_priority = 7;
		unsigned char ais_interval = 0;	// 0:1sec, 1:1minute		
		cfm_config_t *cfm_config = &server_mep_cfm_config_g;

		// reserve index 0 for server mep
		cfm_mp_index[0] = 1;
		// downMEP@wan, index 0, md level 0, mepid CFM_SERVER_MEP_MPID, downMEP@wan0
		cfm_config_init(cfm_config, 0, 0, CFM_SERVER_MEP_MPID, ENV_BRIDGE_PORT_TYPE_WAN, 0);

		cfm_config->ma_mhf_creation = MA_MHF_CREATION_NONE;
		cfm_config->md_mhf_creation = MA_MHF_CREATION_NONE;
		cfm_config->eth_ais_control = (ais_md_level<<4)| (ais_priority<<1) | ais_interval;
		cfm_config->mep_control = MEP_CONTROL_SERVER_MEP_ENABLE | MEP_CONTROL_AIS_ENABLE;
		// while server mep doesnt have ccm timer, the interval is required by cfm_update_defect_status() for timeout calculation
		cfm_config->ccm_interval = 4;	

		server_mep_logical_portmask_g = 0;
	}
	return 0;
}

int
cfm_config_list_release(void)
{
	int ret, index;
	cfm_config_t *cfm_config = NULL, *cfm_config_safe = NULL;

	for(index=0; index<CFM_MAX_MP; index++)
		cfm_mp_index[index] = 0;

	list_for_each_entry_safe(cfm_config, cfm_config_safe, &cfm_config_list, node) {
		cfm_config->index = 0xFF;
		cfm_mp_flush_ltr_list(cfm_config);
		cfm_mp_flush_rmep_list(cfm_config);
		if ((ret = fwk_msgq_release(cfm_config->timer_qid)) < 0)
			dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
		if ((ret = fwk_msgq_release(cfm_config->timer_event_qid)) < 0)
			dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
		list_del(&cfm_config->node);
		free_safe(cfm_config);
	}

	server_mep_cfm_config_g = cfm_config_default;	// clear server mep
	server_mep_logical_portmask_g = 0;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

cfm_config_t*
cfm_mp_add(unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index)
{
	int index = 0;
	cfm_config_t *cfm_config = NULL;

	if (md_level >7) {
		dbprintf_cfm(LOG_ERR, "CFM MP mdl=%u is invalid, should be 0..7\n", md_level);
		return NULL;
	}
	if (mep_id <1 || mep_id >8191) {
		dbprintf_cfm(LOG_ERR, "CFM MP mpid=%u is invalid, should be 1..8191\n", mep_id);
		return NULL;
	}

	if (cfm_mp_find(md_level, mep_id, port_type, port_type_index)!= NULL) {
		dbprintf_cfm(LOG_ERR, "CFM MP mdl=%d, mpid=%u, port_type=%u, port_type_index=%u already exist\n", 			
			md_level, mep_id, port_type, port_type_index);
		return NULL;
	}

	for(index=0; index<CFM_MAX_MP; index++) {
		if (cfm_mp_index[index] == 0) {
			cfm_mp_index[index] = 1;
			break;
		}
	}
	if (index >= CFM_MAX_MP) {
		dbprintf_cfm(LOG_ERR, "CFM MP full, limit=%d\n", CFM_MAX_MP);
		return NULL;
	}

	cfm_config = malloc_safe(sizeof(cfm_config_t));
	if (cfm_config_init(cfm_config, index, md_level, mep_id, port_type, port_type_index) < 0) {
		free_safe(cfm_config);
		return NULL;
	}	

	if (list_empty(&cfm_config_list))
		cfm_switch_extract_set(1);

	list_add_tail(&cfm_config->node, &cfm_config_list);

	cfm_mp_add_peer_mep_ids_to_rmep_list(cfm_config);
	cfm_config->ccm_send_timer_id = fwk_timer_create(cfm_timer_qid_g,
		CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK, CFM_CCM_TIMER_INTERVAL, NULL);
	dbprintf_cfm(LOG_ERR, "CFM MP added (mdl=%u, mpid=%d)\n", md_level, mep_id);
	return cfm_config;
}

cfm_config_t*
cfm_mp_find(unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index)
{
	cfm_config_t *cfm_config = NULL, *cfm_config_safe = NULL;

	list_for_each_entry_safe(cfm_config, cfm_config_safe, &cfm_config_list, node) {
		int is_match = 0;
		if (port_type != 0xff && port_type_index != 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id) &&
				(cfm_config->port_type == port_type) && (cfm_config->port_type_index == port_type_index)) ? 1 : 0;
		} else if (port_type != 0xff && port_type_index == 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id) &&
				(cfm_config->port_type == port_type)) ? 1 : 0;
		} else if (port_type == 0xff && port_type_index != 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id) &&
				(cfm_config->port_type_index == port_type_index)) ? 1 : 0;
		} else if (port_type == 0xff && port_type_index == 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id)) ? 1 : 0;
		}
		if (is_match)
			return cfm_config;
	}
	return NULL;
}

int
cfm_mp_del(unsigned char md_level, unsigned short mep_id, unsigned char port_type, unsigned char port_type_index)
{
	cfm_config_t *cfm_config = NULL, *cfm_config_safe = NULL;
	int is_match = 0;

	list_for_each_entry_safe(cfm_config, cfm_config_safe, &cfm_config_list, node) {
		if (port_type != 0xff && port_type_index != 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id) &&
				(cfm_config->port_type == port_type) && (cfm_config->port_type_index == port_type_index)) ? 1 : 0;
		} else if (port_type != 0xff && port_type_index == 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id) &&
				(cfm_config->port_type == port_type)) ? 1 : 0;
		} else if (port_type == 0xff && port_type_index != 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id) &&
				(cfm_config->port_type_index == port_type_index)) ? 1 : 0;
		} else if (port_type == 0xff && port_type_index == 0xff) {
			is_match = ((cfm_config->md_level == md_level) && (cfm_config->mep_id == mep_id)) ? 1 : 0;
		}
		if (is_match) {
			cfm_mp_index[cfm_config->index] = 0;
			cfm_config->index = 0xFF;
			cfm_mp_flush_ltr_list(cfm_config);
			cfm_mp_flush_rmep_list(cfm_config);
			if (cfm_config->ccm_send_timer_id)
				fwk_timer_delete(cfm_config->ccm_send_timer_id);
			list_del(&cfm_config->node);
			free_safe(cfm_config);

			if (list_empty(&cfm_config_list))
				cfm_switch_extract_set(0);
			dbprintf_cfm(LOG_ERR, "CFM MP deleted (mdl=%u, mpid=%d)\n", md_level, mep_id);
		}
	}

	return (is_match) ? 0 : -1;
}

int
cfm_mp_del_all(void)
{
	int ret;
	cfm_config_t *cfm_config = NULL, *cfm_config_safe = NULL;

	list_for_each_entry_safe(cfm_config, cfm_config_safe, &cfm_config_list, node) {
		cfm_mp_index[cfm_config->index] = 0;
		cfm_config->index = 0xFF;
		cfm_mp_flush_ltr_list(cfm_config);
		cfm_mp_flush_rmep_list(cfm_config);
		if (cfm_config->ccm_send_timer_id)
			fwk_timer_delete(cfm_config->ccm_send_timer_id);
		if ((ret = fwk_msgq_release(cfm_config->timer_qid)) < 0) {
			dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
		}
		if ((ret = fwk_msgq_release(cfm_config->timer_event_qid)) < 0) {
			dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
		}
		list_del(&cfm_config->node);
		free_safe(cfm_config);
	}

	if (list_empty(&cfm_config_list))
		cfm_switch_extract_set(0);
	dbprintf_cfm(LOG_ERR, "CFM MP deleted all\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

int
cfm_mp_flush_lbr_list(cfm_config_t *cfm_config)
{
	cfm_pkt_lbr_entry_t *entry = NULL, *entry_safe = NULL;
	int total = 0;
	
	list_for_each_entry_safe(entry, entry_safe, &cfm_config->cfm_recv_lbr_list, node) {
		list_del(&entry->node);
		free_safe(entry);
		total++;
	}
	return total;
}

int
cfm_mp_flush_ltm_list(cfm_config_t *cfm_config)
{
	cfm_pkt_ltm_entry_t *entry = NULL, *entry_safe = NULL;
	int total = 0;
	
	list_for_each_entry_safe(entry, entry_safe, &cfm_config->cfm_recv_ltm_list, node) {
		list_del(&entry->node);
		free_safe(entry);
		total++;
	}
	return total;
}

int
cfm_mp_flush_ltr_list(cfm_config_t *cfm_config)
{
	cfm_pkt_ltr_entry_t *entry = NULL, *entry_safe = NULL;
	int total = 0;
	
	list_for_each_entry_safe(entry, entry_safe, &cfm_config->cfm_recv_ltr_list, node) {
		list_del(&entry->node);
		free_safe(entry);
		total++;
	}
	return total;
}

int
cfm_mp_flush_rmep_list(cfm_config_t *cfm_config)
{
	cfm_pkt_rmep_entry_t *entry = NULL, *entry_safe = NULL;
	int total = 0;
	
	list_for_each_entry_safe(entry, entry_safe, &cfm_config->cfm_recv_rmep_list, node) {
		list_del(&entry->node);
		free_safe(entry);
		total++;
	}

	// clear the mep statistics collected from rmep
	cfm_config->highest_priority_defect_observed = 0;
	cfm_config->last_recv_err_ccm_msg_len = 0;
	cfm_config->last_xcon_err_ccm_msg_len = 0;
	memset(cfm_config->last_recv_err_ccm_msg, 0, sizeof(cfm_config->last_recv_err_ccm_msg));
	memset(cfm_config->last_xcon_err_ccm_msg, 0, sizeof(cfm_config->last_xcon_err_ccm_msg));
	cfm_config->invalid_senderid_ccm_count = 0;
	cfm_config->invalid_port_status_ccm_count = 0;
	cfm_config->invalid_interface_status_ccm_count = 0;
	cfm_config->out_of_seqence_ccm_count = 0;
	cfm_config->unexpected_ltr_count = 0;

	return total;
}

///////////////////////////////////////////////////////////////////////////////

// add expected peers in cfm_config->peer_mep_id[] to rmep_list
int
cfm_mp_add_peer_mep_ids_to_rmep_list(cfm_config_t *cfm_config)
{
	unsigned long long now = cfm_util_get_time_in_us();
	cfm_pkt_rmep_entry_t *entry;
	int i, total_added = 0;

	if (cfm_config == NULL)
		return -1;
		
	for (i=0; i<12; i++) {
		if (cfm_config->peer_mep_ids[i]) {
			int found = 0;
			if(cfm_config->peer_mep_ids[i] == cfm_config->mep_id)
				continue;
			list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
				if (entry->rmep_id == cfm_config->peer_mep_ids[i]) {
					found = 1;
					break;
				}
			}
			if (found)
				continue;
			if ((entry = malloc_safe(sizeof(cfm_pkt_rmep_entry_t))) == NULL)
				return -1;
			memset(&entry->rmep_mac, 0, IFHWADDRLEN);
			entry->rmep_state = CCM_RMEP_STATE_EXPECT;
			entry->rmep_interval = 0;
			entry->rmep_id = cfm_config->peer_mep_ids[i];
			entry->rmep_timestamp = now;	// unit: us
			entry->rmep_clear_timestamp = 0;
			list_add_tail(&entry->node, &cfm_config->cfm_recv_rmep_list);
			total_added++;
		}
	}
	if (total_added) {
		dbprintf_cfm(LOG_WARNING, "%s add %d expected rMEP to ccm list\n", 
			cfm_print_get_cfm_config_str(cfm_config), total_added);
	}
	return total_added;
}

int
cfm_mp_is_peer_mep_id(cfm_config_t *cfm_config, unsigned short rmepid)
{
	if (cfm_config) {
		int i;
		for (i =0; i<12; i++) {
			if (cfm_config->peer_mep_ids[i] == rmepid)
				return 1;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

static inline int
cfm_mp_update_defect_status_one(cfm_config_t *cfm_config, int defect_number, int is_set, unsigned long long now)
{
	// update defect_status	
	if (is_set) {		
		if ((cfm_config->defect_status & (1<<defect_number)) == 0) {	// 0->1
			cfm_config->defect_status |= (1<<defect_number);
			cfm_config->defect_status_timestamp[defect_number] = now;
			return 1;
		}
	} else {
		if (cfm_config->defect_status & (1<<defect_number)) {		// 1->0
			cfm_config->defect_status &= (~(1<<defect_number));
			cfm_config->defect_status_timestamp[defect_number] = now;
			return 1;
		}
	}
	return 0;
}

int
cfm_mp_update_defect_status(cfm_config_t *cfm_config)
{
	unsigned long long now = cfm_util_get_time_in_us(); 
	unsigned int ccm_interval_time = cfm_util_ccm_interval_to_ms(cfm_config->ccm_interval);
	unsigned int ais_interval_time = (cfm_config->eth_ais_control&1)?10000:1000;	// unit ms
	unsigned int ccm_timeout = ccm_interval_time * omci_env_g.cfm_rmep_ccm_lost_threshold * 1000 +
				   ccm_interval_time * 500;
	int is_rmep_ccm_timeout = 0;
	cfm_pkt_rmep_entry_t *entry, *entry_next;
	int update = 0;

	/* 0 ~ alarm_declaration_soak_time: detect defect and enter defect state
	 alarm_declaration_soak_time ~ ccm_timeout: issue alarm and stay in defect state
	 ccm_timeout ~ alarm_clear_soak_time: if no defect happen again, leave defect state
	 alarm_clear_soak_time ~ : clear alarm */
	// clear defect_unexp_period_timestamp if defect is longer than alarm_time + ccm_timeout
	if(now - cfm_config->defect_unexp_period_timestamp > cfm_config->alarm_declaration_soak_time*1000 + ccm_timeout)
		cfm_config->defect_unexp_period_timestamp = 0;
	// defect DEFECT_REMOTE_CCM
	list_for_each_entry_safe(entry, entry_next, &cfm_config->cfm_recv_rmep_list, node) {
		if (entry->rmep_clear_timestamp && now - entry->rmep_clear_timestamp > cfm_config->alarm_clear_soak_time*1000) {
			if (entry->rmep_state == CCM_RMEP_STATE_LOST) {
				// remove if entry is LOST && alarm_clear_soak_timeout
				is_rmep_ccm_timeout = 1;
				entry->rmep_ccm_consecutive = 0;
				entry->rmep_port_state = 0;
				entry->rmep_iface_state = 0;
				entry->rmep_state = CCM_RMEP_STATE_REMOVE;
			}
		}
		if ((now >= entry->rmep_timestamp) && (now - entry->rmep_timestamp > ccm_timeout)) {
			dbprintf_cfm(LOG_WARNING, "%s ccm timeout (rMEPid=%d) (now=%lld prev=%lld ccm_timeout=%d)\n",
				cfm_print_get_cfm_config_str(cfm_config), entry->rmep_id, now, entry->rmep_timestamp, ccm_timeout);
			if (entry->rmep_state == CCM_RMEP_STATE_ACTIVE) {
				// mark rdi if entry is ACTIVE && ccm timeouted
				is_rmep_ccm_timeout = 1;
				entry->rmep_ccm_consecutive = 0;
				entry->rmep_port_state = 0;
				entry->rmep_iface_state = 0;
				entry->rmep_clear_timestamp = now;
				entry->rmep_state = CCM_RMEP_STATE_LOST;
				dbprintf_cfm(LOG_ERR, "%s CFM alarm LOST_PEER is set (rMEPid=%d)\n",
					cfm_print_get_cfm_config_str(cfm_config), entry->rmep_id);
			} else if (entry->rmep_state == CCM_RMEP_STATE_LOST) {
				is_rmep_ccm_timeout = 1;
				entry->rmep_port_state = 0;
				entry->rmep_iface_state = 0;
			} else if (entry->rmep_state == CCM_RMEP_STATE_DISCOVER ||
				   entry->rmep_state == CCM_RMEP_STATE_HOLD ||
				   entry->rmep_state == CCM_RMEP_STATE_REMOVE) {
				if (cfm_mp_is_peer_mep_id(cfm_config, entry->rmep_id)) {
					entry->rmep_ccm_consecutive = 0;
					entry->rmep_state = CCM_RMEP_STATE_EXPECT;
				} else {
					// remove DISCOVER/HOLD/REMOVE entry if it is ccm timeouted
					dbprintf_cfm(LOG_ERR, "%s CFM alarm REMOVE_PEER is set (rMEPid=%d)\n",
						cfm_print_get_cfm_config_str(cfm_config), entry->rmep_id);
					list_del(&entry->node);
					free_safe(entry);
				}
			} else if (entry->rmep_state == CCM_RMEP_STATE_EXPECT) {
				// issue remote_ccm defect for the non-existed peer mep (static rmep)
				if (cfm_mp_is_peer_mep_id(cfm_config, entry->rmep_id)) {
					// don't report remote_ccm if there is unexp_period alarm
					if(!cfm_config->defect_unexp_period_timestamp)
						is_rmep_ccm_timeout = 1;
				}
			}
		}
	}
	update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_REMOTE_CCM, 
		is_rmep_ccm_timeout, now);

	// defect DEFECT_RDI_CCM
	if (cfm_config->defect_rdi_ccm_timestamp)
		update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_RDI_CCM, 
			now - cfm_config->defect_rdi_ccm_timestamp < ccm_interval_time * 3500, now);
	// defect DEFECT_MAC_STATUS
	if (cfm_config->defect_mac_status_timestamp)
		update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_MAC_STATUS, 
			now - cfm_config->defect_mac_status_timestamp < ccm_interval_time * 3500, now);
	// defect DEFECT_ERROR_CCM
	if (cfm_config->defect_error_ccm_timestamp)
		update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_ERROR_CCM, 
			now - cfm_config->defect_error_ccm_timestamp < ccm_interval_time * 3500, now);
	// defect DEFECT_XCON_CCM
	if (cfm_config->defect_xcon_ccm_timestamp)
		update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_XCON_CCM, 
			now - cfm_config->defect_xcon_ccm_timestamp < ccm_interval_time * 3500, now);
	// defect DEFECT_UNEXP_PERIOD
	if (cfm_config->defect_unexp_period_timestamp)
		update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_UNEXP_PERIOD,
			now - cfm_config->defect_unexp_period_timestamp < ccm_interval_time * 3500, now);
	// defect DEFECT_AIS
	if (cfm_config->defect_ais_timestamp)
		update += cfm_mp_update_defect_status_one(cfm_config, DEFECT_AIS, 
			now - cfm_config->defect_ais_timestamp < ais_interval_time * 3500, now);
	
	if (update >0) {
		int priority = cfm_util_get_send_priority_by_defect_status(cfm_config->defect_status);
		if (priority > cfm_config->highest_priority_defect_observed)
			cfm_config->highest_priority_defect_observed = priority;

		dbprintf_cfm(LOG_DEBUG, "DEFECT changed, now=%llu\n"
			"defect_rdi_ccm_timestamp=%llu\n"
			"defect_mac_status_timestamp=%llu\n"
			"defect_error_ccm_timestamp=%llu\n"
			"defect_xcon_ccm_timestamp=%llu\n"
			"defect_unexp_period_timestamp=%llu\n"
			"defect_ais_timestamp=%llu\n",
			now,
			cfm_config->defect_rdi_ccm_timestamp,
			cfm_config->defect_mac_status_timestamp,
			cfm_config->defect_error_ccm_timestamp,
			cfm_config->defect_xcon_ccm_timestamp,
			cfm_config->defect_unexp_period_timestamp,
			cfm_config->defect_ais_timestamp);
	}	

	return update;
}

static inline int
cfm_mp_update_alarm_status_one(cfm_config_t *cfm_config, int defect_number, unsigned long long now)
{
	// update alarm_status
	if (cfm_config->defect_status & (1<<defect_number)) {	// defect on
		if ((cfm_config->alarm_status & (1<<defect_number)) == 0) {		// 0->1
			if (now - cfm_config->defect_status_timestamp[defect_number] > cfm_config->alarm_declaration_soak_time * 1000) {
				cfm_config->alarm_status |= (1<<defect_number);
				return 1;
			}
		}
	} else {						// defect off
		if (cfm_config->alarm_status & (1<<defect_number)) {		// 1->0
			if (now - cfm_config->defect_status_timestamp[defect_number] > cfm_config->alarm_clear_soak_time * 1000) {
				cfm_config->alarm_status &= (~(1<<defect_number));
				return 1;
			}
		}
	}
	return 0;
}	

int
cfm_mp_update_alarm_status(cfm_config_t *cfm_config)
{
	unsigned long long now = cfm_util_get_time_in_us(); 
	int update = 0, defect_number;

	for (defect_number = 0; defect_number < DEFECT_INVALID; defect_number++)
		update += cfm_mp_update_alarm_status_one(cfm_config, defect_number, now);
	return update;
}

int
cfm_mp_check_interval_and_send_ais_lck(cfm_config_t *cfm_config)
{
	int ret = 0;

	dbprintf_cfm(LOG_NOTICE, "ais/lck interval check for %s\n", cfm_print_get_cfm_config_str(cfm_config));
	if (omci_env_g.cfm_y1731_enable) {
		unsigned long long now = cfm_util_get_time_in_us();
		unsigned int ais_interval_us = ((cfm_config->eth_ais_control&1)?60:1) *1000 *1000;
		char flags = (cfm_config->eth_ais_control&1)?6:4;	// 6 means 1min, 4 means 1 sec
		char priority = (cfm_config->eth_ais_control>>1) & 7;

		if (now - cfm_config->cfm_send_ais_timestamp > ais_interval_us) {		
			// while recv lck doesnt mean defect, it also applies 3.5*ais_interval as timeout
			unsigned char is_recv_lck = (now - cfm_config->recv_lck_timestamp <= ais_interval_us*3.5)?1:0;			
			unsigned char defect= cfm_config->defect_status;
			if (defect) {
				if (cfm_config->defect_status & (1<<DEFECT_AIS))
					defect &= (~omci_env_g.cfm_defect_mask_supressed_by_ais);
				if (is_recv_lck)
					defect &= (~omci_env_g.cfm_defect_mask_supressed_by_lck);
			}
			if ((defect & omci_env_g.cfm_defect_mask_in_ais_gen) || is_recv_lck) {
				cfm_send_ais(cfm_config, "NULL", priority, 0, flags);	
				cfm_config->cfm_send_ais_timestamp = now;
				ret++;
			}
		}
		if (now - cfm_config->cfm_send_lck_timestamp > ais_interval_us) {
			if (cfm_config->admin_state) {
				cfm_send_lck(cfm_config, "NULL", priority, 0, flags);	
				cfm_config->cfm_send_lck_timestamp = now;
				ret++;
			}
		}
	}				
	return ret;
}

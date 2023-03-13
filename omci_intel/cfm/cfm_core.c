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
 * File    : cfm_core.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "fwk_mutex.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "fwk_timer.h"
#include "util.h"
#include "env.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_util.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "cpuport_extract_filter.h"

#include "cfm_pkt.h"
#include "cfm_mp.h"
#include "cfm_util.h"
#include "cfm_switch.h"
#include "cfm_print.h"
#include "cfm_recv.h"
#include "cfm_send.h"
#include "cfm_core.h"

int cfm_task_loop_g = 0;
static struct fwk_thread_t cfm_task_thread_g;

static int cfm_pkt_recv_qid_g = -1;
static int cfm_qsetid_g = -1;
static int cfm_alarm_timerid = 0;
int cfm_cmd_qid_g = -1;
int cfm_timer_qid_g = -1;

static void *
eth_slm_dual_stop_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_slm_dual_recv_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}
static void *
eth_lm_dual_stop_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_lm_dual_stop_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}

static void *
eth_lm_single_start_check_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_lm_single_start_check_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}
static void *
eth_lm_single_stop_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_lm_single_stop_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}
static void *
eth_lm_single_stop_check_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_lm_single_stop_check_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}

static void *
eth_slm_dual_sender_loop_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry=NULL;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_slm_dual_sender_loop_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}

static void *
eth_slm_single_sender_loop_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry=NULL;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_slm_single_sender_loop_timer == timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}
static void *
eth_slm_single_sender_stop_timer_search( struct list_head *cfm_recv_rmep_list, int timer_id )
{
	cfm_pkt_rmep_entry_t *entry=NULL;
	list_for_each_entry(entry, cfm_recv_rmep_list, node) {
		if (  entry->eth_slm_single_sender_stop_timer== timer_id ) {
			return (void *)entry;
		}
	}
	return NULL;
}

static int
cfm_rmep_entry_find ( cfm_config_t *cfm_config, unsigned short rmep_id, void *(*search)( struct list_head *, int ), int search_param, cfm_pkt_rmep_entry_t **rmep_entry )
{
	cfm_pkt_rmep_entry_t *entry=NULL;

	if ( search )  {
		entry = (cfm_pkt_rmep_entry_t *)search( &cfm_config->cfm_recv_rmep_list, search_param );
		if (entry ) {
			if (rmep_id == 0) {
				*rmep_entry = entry;
			}else{
				if (entry->rmep_id == rmep_id) {
					*rmep_entry = entry;
				}
			}
			return 0;
		}
	}else {
		list_for_each_entry(entry, &cfm_config->cfm_recv_rmep_list, node) {
			if ( entry->rmep_id == rmep_id) {
				*rmep_entry = entry;
				return 0;
			}
		}
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////////////

static int
process_cfm_pkt_recv_q(void)
{
	cfm_pktinfo_t *cfm_pktinfo = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	struct list_head *list_node = NULL;
	int ret;

	if ((ret = fwk_msgq_recv(cfm_pkt_recv_qid_g, &list_node)) < 0) {
		dbprintf_cfm(LOG_ERR, "cfm_pkt_recv_qid_g recv err:%d \n", ret);
		return -1;
	}
	if ((pktinfo = list_entry(list_node, struct cpuport_info_t, node)) == NULL)
		return -1;
	pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_CFM];
	util_get_uptime(&pktinfo->process_time);
	if (omci_env_g.cpuport_history_enable)
		cpuport_history_add(pktinfo);

	dbprintf_cfm(LOG_INFO, "incoming packet pktinfo->frame_len %d\n", pktinfo->frame_len);
	//cpuport_sniffer_dbprint_pkt(LOG_DEBUG, "cfm_rx", pktinfo->frame_ptr, pktinfo->frame_len);
	/*if (pktinfo->rx_desc.bridge_port_me && pktinfo->tx_desc.bridge_port_me) {
		struct cpuport_tci_t dst_tci;
		if (cpuport_vtagging_vfilter(pktinfo->rx_desc.bridge_port_me, pktinfo->tx_desc.bridge_port_me, &pktinfo->tci, &dst_tci) != CPUPORT_VTAGGING_VFILTER_PASS) {
			if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
			if (pktinfo) free_safe(pktinfo);
			continue;
		}
	}*/

	cfm_pktinfo =(cfm_pktinfo_t *) malloc_safe(sizeof(cfm_pktinfo_t));
	if (cfm_pktinfo == NULL) {
		if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
		if (pktinfo) free_safe(pktinfo);
		return -1;
	}
	cfm_pkt_parse_pktinfo(pktinfo, cfm_pktinfo);
	cfm_recv_pkt_process(pktinfo, cfm_pktinfo);

	util_get_uptime(&pktinfo->forward_time);
	if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
		dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
	if (omci_env_g.cpuport_history_enable)
		cpuport_history_sync(pktinfo);

	if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
	if (pktinfo) free_safe(pktinfo);
	if (cfm_pktinfo) free_safe(cfm_pktinfo);
	return 0;
}

static int
process_cfm_cmd_q(void)
{
	cfm_cmd_msg_t *cfm_cmd_msg;
	cfm_config_t *cfm_config=NULL;
	cfm_pkt_rmep_entry_t *rmep_entry=NULL;
	struct list_head *list_node = NULL;
	int ret;

	if ((ret = fwk_msgq_recv(cfm_cmd_qid_g, &list_node)) < 0) {
		dbprintf_cfm(LOG_ERR, "cfm_cmd_qid_g recv err(%d)\n", ret);
		return -1;
	}
	cfm_cmd_msg = list_entry(list_node, cfm_cmd_msg_t, q_entry);

	// cfm_config is required by Lm & LMM & CCM_INTERVAL_UPFATE
	if (cfm_cmd_msg->cmd == CFM_CMD_ETH_LM_DUAL_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_ETH_LM_SINGLE_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_ETH_SLM_DUAL_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_ETH_SLM_SINGLE_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_CCM_INTERVAL_UPDATE) {
		cfm_config = (cfm_config_t *)cfm_cmd_msg->usr_data;
		if (!cfm_config) {
			dbprintf_cfm(LOG_ERR,"MEP config not found!\n");
			return -1;
		}
	}
	// remp is required by LM & SLM
	if (cfm_cmd_msg->cmd == CFM_CMD_ETH_LM_DUAL_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_ETH_LM_SINGLE_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_ETH_SLM_DUAL_START ||
	    cfm_cmd_msg->cmd == CFM_CMD_ETH_SLM_SINGLE_START) {
		if (cfm_rmep_entry_find (  cfm_config, cfm_cmd_msg->rmepid, NULL, 0, &rmep_entry ) < 0 ) {
			dbprintf_cfm(LOG_ERR,"mep %d CCM information for rmep id %d level (%d) is not found!\n", cfm_config->mep_id,
					cfm_cmd_msg->rmepid, cfm_cmd_msg->mdlevel);
			return -1;
		}
	}

	switch (cfm_cmd_msg->cmd) {
		case CFM_CMD_ME_ADD_MSG:
		case CFM_CMD_ME_MODIFY_MSG:
		case CFM_CMD_ME_DEL_MSG:
		case CFM_CMD_TERMINATE:
			break;
		case CFM_CMD_ETH_LM_DUAL_START:
			memset(&(rmep_entry->eth_lm_dual_start_time), 0, sizeof(time_t));
			memset(&(rmep_entry->eth_lm_dual_expect_time), 0xffff, sizeof(unsigned int));
			memset(&(rmep_entry->eth_lm_dual_stop_time), 0, sizeof(time_t));
			memset(&(rmep_entry->eth_lm_dual_fratio), 0, sizeof(unsigned int));
			memset(&(rmep_entry->eth_lm_dual_nratio), 0, sizeof(unsigned int));
			rmep_entry->eth_lm_dual_repeat = cfm_cmd_msg->repeat;
			time(&rmep_entry->eth_lm_dual_start_time);
			rmep_entry->eth_lm_dual_expect_time = rmep_entry->eth_lm_dual_repeat*cfm_cmd_msg->interval/1000  ;
			rmep_entry->eth_lm_dual_going_on = 1;

			rmep_entry->TxFCf_p = rmep_entry->TxFCl_ccm;
			rmep_entry->RxFCb_p = rmep_entry->RxFCb_ccm;
			rmep_entry->TxFCb_p = rmep_entry->TxFCb_ccm;
			rmep_entry->RxFCl_p = rmep_entry->RxFCl_ccm;
			rmep_entry->eth_lm_dual_stop_timer = fwk_timer_create(cfm_timer_qid_g,
					CFM_TIMER_EVENT_ETH_LM_DUAL_STOP,
					rmep_entry->eth_lm_dual_repeat*1000,
					(void *)cfm_config);
			rmep_entry->eth_lm_dual_expect_time = cfm_cmd_msg->repeat*cfm_cmd_msg->interval/1000 ;
			break;
		case CFM_CMD_ETH_LM_SINGLE_START:
			rmep_entry->eth_lm_single_going_on = 1;
			memset(&(rmep_entry->eth_lm_single_start_time), 0, sizeof(time_t));
			memset(&(rmep_entry->eth_lm_single_expect_time), 0xffff, sizeof(unsigned int));
			memset(&(rmep_entry->eth_lm_single_stop_time), 0, sizeof(time_t));
			time(&rmep_entry->eth_lm_single_start_time);
			rmep_entry->eth_lm_single_repeat = cfm_cmd_msg->repeat;
			rmep_entry->eth_lm_single_expect_time = rmep_entry->eth_lm_single_repeat*cfm_cmd_msg->interval/1000;
			memset(&rmep_entry->eth_lm_single_fratio, 0, sizeof(unsigned int));
			memset(&rmep_entry->eth_lm_single_nratio, 0, sizeof(unsigned int));

			rmep_entry->TxFCf_p = 0;
			rmep_entry->RxFCf_p = 0;
			rmep_entry->TxFCb_p = 0;
			rmep_entry->RxFCl_p = 0;
			rmep_entry->TxFCf_c = 0;
			rmep_entry->RxFCf_c = 0;
			rmep_entry->TxFCb_c = 0;
			rmep_entry->RxFCl_c = 0;
			cfm_send_lmm(cfm_config, rmep_entry, -1, 0);
			rmep_entry->eth_lm_single_start_check_timer =
				fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_LM_SINGLE_START_CHECK,
						2000, (void *)cfm_config);
			break;
		case CFM_CMD_ETH_SLM_DUAL_START:
			if (rmep_entry) {
				memset(&(rmep_entry->eth_slm_dual_start_time), 0, sizeof(time_t));
				memset(&(rmep_entry->eth_slm_dual_expect_time), 0xffff, sizeof(unsigned int));
				memset(&(rmep_entry->eth_slm_dual_stop_time), 0, sizeof(time_t));
				memset(&(rmep_entry->eth_slm_dual_fratio), 0, sizeof(unsigned int));
				memset(&(rmep_entry->eth_slm_dual_nratio), 0, sizeof(unsigned int));
				time(&rmep_entry->eth_slm_dual_start_time);
				rmep_entry->test_id_1sl = cfm_util_get_time_in_us();
				rmep_entry->eth_slm_dual_interval = cfm_cmd_msg->interval;
				rmep_entry->eth_slm_dual_repeat = cfm_cmd_msg->repeat;
				rmep_entry->eth_slm_dual_expect_time = cfm_cmd_msg->interval*cfm_cmd_msg->repeat/1000;
				rmep_entry->eth_slm_dual_sender_loop_timer =
					fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_SLM_DUAL_SENDER_LOOP
						, rmep_entry->eth_slm_dual_interval, (void *)cfm_config );
			}
			break;
		case CFM_CMD_ETH_SLM_SINGLE_START:
			if (rmep_entry) {
				if ( rmep_entry->eth_slm_single_going_on == 0 ) {
					rmep_entry->test_id_slm = cfm_util_get_time_in_us();
					rmep_entry->eth_slm_single_going_on = 1;
				}else{
					dbprintf_cfm(LOG_ERR,"Another SLM single end test is going on.\n");
					break;
				}
				memset(&(rmep_entry->eth_slm_single_start_time), 0, sizeof(time_t));
				memset(&(rmep_entry->eth_slm_single_expect_time), 0xffff, sizeof(unsigned int));
				memset(&(rmep_entry->eth_slm_single_stop_time), 0, sizeof(time_t));
				memset(&(rmep_entry->eth_slm_single_fratio), 0, sizeof(unsigned int));
				memset(&(rmep_entry->eth_slm_single_nratio), 0, sizeof(unsigned int));
				time(&rmep_entry->eth_slm_single_start_time);
				rmep_entry->eth_slm_single_interval = cfm_cmd_msg->interval;
				rmep_entry->eth_slm_single_repeat = cfm_cmd_msg->repeat;
				rmep_entry->TxFCf_p = 0;
				rmep_entry->TxFCb_p = 0;
				rmep_entry->RxFCl_p = 0;
				//rmep_entry->TxFCl_slm++;
				//cfm_send_slm(cfm_config, rmep_entry, -1);
				rmep_entry->eth_slm_single_expect_time = cfm_cmd_msg->repeat*cfm_cmd_msg->interval/1000;
				rmep_entry->eth_slm_single_sender_loop_timer =
					fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_LOOP
							, rmep_entry->eth_slm_single_interval, (void *)cfm_config);
			} else {
				if ( !rmep_entry )
					dbprintf_cfm(LOG_ERR,"remote mepid MAC not found!\n");
			}
			break;
		case CFM_CMD_CCM_INTERVAL_UPDATE:
			if (!(cfm_config->md_level == server_mep_cfm_config_g.md_level &&
			      cfm_config->mep_id == server_mep_cfm_config_g.mep_id)) {
				unsigned int ccm_interval_time = cfm_util_ccm_interval_to_ms(cfm_config->ccm_interval);
				if (cfm_config->ccm_send_timer_id)	// erase existing ccm timer
					fwk_timer_delete(cfm_config->ccm_send_timer_id);
				cfm_config->ccm_send_timer_id = fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK, ccm_interval_time, NULL);
			}
			break;
		default:
			dbprintf_cfm(LOG_ERR, "wrong CFM msg cmd\n");
			break;
	}
	if (cfm_cmd_msg) free_safe(cfm_cmd_msg);
	return 0;
}

static int
timer_event_alarm_check(cfm_config_t *cfm_config)
{
	unsigned char alarm_status_prev = cfm_config->alarm_status;
	unsigned char alarm_status_changed;

	cfm_mp_update_alarm_status(cfm_config);
	// find out the alarm status bit that has been changed
	alarm_status_changed = alarm_status_prev ^ cfm_config->alarm_status;

	if (alarm_status_changed) {
		static unsigned char alarm_threshold_bitmap[] = {
			ALARM_THRESHOLD_0, ALARM_THRESHOLD_1, ALARM_THRESHOLD_2, ALARM_THRESHOLD_3,
			ALARM_THRESHOLD_4, ALARM_THRESHOLD_5, ALARM_THRESHOLD_6
		};
		static char *alarm_str[] = {"RDICCM", "MACstatus", "RemoteCCM", "ErrorCCM", "XconCCM", "UnexpPeriod", "AIS"};
		unsigned long long now = cfm_util_get_time_in_us();
		unsigned int ais_interval_us = ((cfm_config->eth_ais_control&1)?60:1) *1000 *1000;
		unsigned char alarm_allowed_mask = alarm_threshold_bitmap[0];
		int i;

		if (cfm_config->fault_alarm_threshold >= 1 &&
		    cfm_config->fault_alarm_threshold <= 6) {
		    	alarm_allowed_mask = alarm_threshold_bitmap[cfm_config->fault_alarm_threshold];
		}

		for (i = DEFECT_INVALID -1 ; i>=0; i--) {
			if (alarm_status_changed & (1<<i)) {
				if ((cfm_config->defect_status & (1<<DEFECT_AIS)) &&
				    (cfm_config->alarm_status & omci_env_g.cfm_defect_mask_supressed_by_ais)) {
					dbprintf_cfm(LOG_WARNING, "%s CFM alarm %s is suppressed by AIS\n",
						cfm_print_get_cfm_config_str(cfm_config), alarm_str[i]);
					continue;
				}
				if (now - cfm_config->recv_lck_timestamp <= ais_interval_us*3.5 &&
				    (cfm_config->alarm_status & omci_env_g.cfm_defect_mask_supressed_by_lck)) {
					dbprintf_cfm(LOG_WARNING, "%s CFM alarm %s is suppressed by LCK\n",
						cfm_print_get_cfm_config_str(cfm_config), alarm_str[i]);
					continue;
				}
				// generate alarm to NMS, eg: tr069
				dbprintf_cfm(LOG_ERR, "%s CFM alarm %s is %s%s\n",
					cfm_print_get_cfm_config_str(cfm_config), alarm_str[i],
					(cfm_config->alarm_status & (1<<i))?"set":"cleared",
					(alarm_allowed_mask & (1<<i))?"":"(ignored)");
			}
		}

		return alarm_status_changed & alarm_allowed_mask;
	}
	return 0;
}

static void
timer_event_eth_cc_periodic_check(cfm_config_t *cfm_config)
{
	dbprintf_cfm(LOG_NOTICE, "ccm send check for %s: admin=%d, is_mep=%d, ccm_interval=%d, ccm_enabled=%d\n",
		cfm_print_get_cfm_config_str(cfm_config),
		cfm_config->admin_state,
		(cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)?1:0,
		cfm_config->ccm_interval, (cfm_config->mep_control & MEP_CONTROL_CCM_ENABLE)?1:0);

	if (cfm_config->admin_state == 0 && 				// not locked
	    cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE && 	// mep
	    cfm_config->ccm_interval!= CCM_INTERVAL_DISABLED) {		// ccm interval enabled

		// send ccm only if enabled
		if (cfm_config->mep_control & MEP_CONTROL_CCM_ENABLE) {
			cfm_config->CCIsentCCMs++;
			cfm_send_ccm(cfm_config, -1, 0);
		}
	}
}

static void
timer_event_eth_dmm_periodic_check(cfm_config_t *cfm_config)
{
	dbprintf_cfm(LOG_NOTICE, "dmm send check for %s: admin=%d, is_mep=%d, ccm_interval=%d, dmm_enabled=%d\n",
		cfm_print_get_cfm_config_str(cfm_config),
		cfm_config->admin_state,
		(cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)?1:0,
		cfm_config->ccm_interval, (cfm_config->mep_control2 & MEP_CONTROL2_DMM_ENABLE)?1:0);

	if (cfm_config->admin_state == 0 && 				// not locked
	    cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE && 	// mep
	    cfm_config->ccm_interval!= CCM_INTERVAL_DISABLED) {		// ccm interval enabled

		// send dmm only if enabled
		if (cfm_config->mep_control2 & MEP_CONTROL2_DMM_ENABLE) {
			unsigned char null_mac[IFHWADDRLEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
			if(memcmp(cfm_config->cfm_send_dmm_mac, null_mac, IFHWADDRLEN) != 0)
				cfm_send_dmm(cfm_config, cfm_config->cfm_send_dmm_mac, -1, 0);
		}
	}
}

static void
timer_event_eth_lmm_periodic_check(cfm_config_t *cfm_config)
{
	dbprintf_cfm(LOG_NOTICE, "lmm send check for %s: admin=%d, is_mep=%d, ccm_interval=%d, lmm_enabled=%d\n",
		cfm_print_get_cfm_config_str(cfm_config),
		cfm_config->admin_state,
		(cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE)?1:0,
		cfm_config->ccm_interval, (cfm_config->mep_control2 & MEP_CONTROL2_LMM_ENABLE)?1:0);

	if (cfm_config->admin_state == 0 && 				// not locked
	    cfm_util_get_mhf_creation(cfm_config) == MA_MHF_CREATION_NONE && 	// mep
	    cfm_config->ccm_interval!= CCM_INTERVAL_DISABLED) {		// ccm interval enabled

		// send lmm only if enabled
		if (cfm_config->mep_control2 & MEP_CONTROL2_LMM_ENABLE) {
			cfm_pkt_rmep_entry_t *rmep_entry = NULL;
			list_for_each_entry(rmep_entry, &cfm_config->cfm_recv_rmep_list, node) {
				if(memcmp(cfm_config->cfm_send_lmm_mac, rmep_entry->rmep_mac, IFHWADDRLEN) == 0) {
					cfm_send_lmm(cfm_config, rmep_entry, -1, 0);
					break;
				}
			}
		}
	}
}

static int
process_cfm_timer_qid(void)
{
	struct fwk_timer_msg_t *cfm_timer_msg=NULL;
	struct list_head *list_node = NULL;
	cfm_config_t *cfm_config=NULL;
	cfm_pkt_rmep_entry_t *rmep_entry = NULL;
	int ccm_send_timer_id_matched;
	int ret;

	if ((ret = fwk_msgq_recv(cfm_timer_qid_g, &list_node)) < 0) {
		dbprintf_cfm(LOG_ERR, "cfm_timer_qid_g recv err(%d)\n", ret);
		return -1;
	}

	cfm_timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);
	switch (cfm_timer_msg->event) {
		case CFM_TIMER_EVENT_ALARM_CHECK:
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ALARM_CHECK\n");
			// mep action					when
			//-------------------------------------------	--------------------
			// update defect time				pkt recv
			// cfm_mp_update_defect_status()		alarm loop, ccm send or pkt recv
			// cfm_mp_check_interval_and_send_ais_lck	alarm loop, ccm send or pkt recv
			// timer_event_alarm_check			alarm loop

			// we do cfm_mp_update_defect_status(), cfm_mp_check_interval_and_send_ais_lck() in
			// a. alarm loop: for quick response on mep with very long interval (eg: 1min, 10min)
			// b. ccm send: for normal mep with very short interval (eg: 3.3ms)
			// c. pkt recv: for server mep which has no ccm send

			if (cfm_alarm_timerid!=0 && cfm_timer_msg->timer_id==cfm_alarm_timerid) {
				// normal mep
				list_for_each_entry(cfm_config, &cfm_config_list, node) {
					if (cfm_util_get_mhf_creation(cfm_config) != MA_MHF_CREATION_NONE)	// skip mip
						continue;
					// update defect
					cfm_mp_update_defect_status(cfm_config);
					// send ais for defect, send lck for adminlock
					if (cfm_config->mep_control & MEP_CONTROL_AIS_ENABLE)
						cfm_mp_check_interval_and_send_ais_lck(cfm_config);
					// send alarm if necessary
					timer_event_alarm_check(cfm_config);
				}
				// server mep
				if (server_mep_cfm_config_g.mep_control & MEP_CONTROL_SERVER_MEP_ENABLE) {      // server_mep enabled
					cfm_config = &server_mep_cfm_config_g;
					// update defect
					cfm_mp_update_defect_status(cfm_config);
					if (cfm_config->defect_status == 0)
						server_mep_logical_portmask_g = 0;	// clear history of defect ingress ports
					// send ais for defect, send lck for adminlock
					if (cfm_config->mep_control & MEP_CONTROL_AIS_ENABLE)
						cfm_mp_check_interval_and_send_ais_lck(cfm_config);
					// send alarm if necessary
					timer_event_alarm_check(cfm_config);
				}

				// prepear next timer
				cfm_alarm_timerid=fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ALARM_CHECK, CFM_ALARM_CHECK_INTERVAL, NULL);
			} else {
				dbprintf_cfm(LOG_CRIT, "msg->timerid=0x%x, cfm_alarm_timerid=0x%x\n", cfm_timer_msg->timer_id, cfm_alarm_timerid);
			}
			break;
		case CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK:
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK\n");
			ccm_send_timer_id_matched = 0;
			list_for_each_entry(cfm_config, &cfm_config_list, node) {
				if ( cfm_config->ccm_send_timer_id == cfm_timer_msg->timer_id ) {
					ccm_send_timer_id_matched = 1;
					unsigned int ccm_interval_time = cfm_util_ccm_interval_to_ms(cfm_config->ccm_interval);
					// cfm_mp_update_defect_status() in done alarm loop, ccm send or pkt recv
					if (cfm_mp_update_defect_status(cfm_config) > 0) {
						// send ais ASAP if any defect detected
						cfm_mp_check_interval_and_send_ais_lck(cfm_config);
					}
					// send ccm
					timer_event_eth_cc_periodic_check(cfm_config);
					// send dmm
					if (omci_env_g.cfm_y1731_enable) timer_event_eth_dmm_periodic_check(cfm_config);
					if (omci_env_g.cfm_y1731_enable) timer_event_eth_lmm_periodic_check(cfm_config);
					// prepare next timer
					cfm_config->ccm_send_timer_id = fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK, ccm_interval_time, NULL);
					break;
				}
			}
			if (!ccm_send_timer_id_matched) {
				dbprintf_cfm(LOG_NOTICE, "ccm send check timer_id %d is not matched with any mip/mep?\n");
			}
			break;

		case CFM_TIMER_EVENT_ETH_LM_DUAL_STOP:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_LM_DUAL_STOP, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config , 0, eth_lm_dual_stop_timer_search, cfm_timer_msg->timer_id ,
						 &rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_lm_dual_stop_timer information for mep id %d is not found!\n",
						cfm_config->mep_id);
				break;
			}

			if (rmep_entry) {
				rmep_entry->TxFCf_c = rmep_entry->TxFCl_ccm;
				rmep_entry->RxFCb_c = rmep_entry->RxFCb_ccm;
				rmep_entry->TxFCb_c = rmep_entry->TxFCb_ccm;
				rmep_entry->RxFCl_c = rmep_entry->RxFCl_ccm;

				rmep_entry->eth_lm_dual_tx_floss = rmep_entry->TxFCb_c - rmep_entry->TxFCb_p;
				rmep_entry->eth_lm_dual_rx_floss = rmep_entry->RxFCb_c - rmep_entry->RxFCb_p;
				rmep_entry->eth_lm_dual_tx_nloss = rmep_entry->TxFCf_c - rmep_entry->TxFCf_p;
				rmep_entry->eth_lm_dual_rx_nloss = rmep_entry->RxFCl_c - rmep_entry->RxFCl_p;

				dbprintf_cfm(LOG_WARNING," farend tx=TxFCb_c(%d)-TxFCb_p(%d), rx=RxFCb_c(%d)-RxFCb_p(%d)\n",
						rmep_entry->TxFCb_c, rmep_entry->TxFCb_p,
						rmep_entry->RxFCb_c, rmep_entry->RxFCb_p);
				dbprintf_cfm(LOG_WARNING,"nearend tx=TxFCf_c(%d)-TxFCf_p(%d), rx=RxFCl_c(%d)-RxFCl_p(%d)\n",
						rmep_entry->TxFCf_c, rmep_entry->TxFCf_p,
						rmep_entry->RxFCl_c, rmep_entry->RxFCl_p);

				if ( rmep_entry->eth_lm_dual_force_stop == 0 ) {
					int floss = rmep_entry->eth_lm_dual_tx_floss - rmep_entry->eth_lm_dual_rx_floss;
					int nloss = rmep_entry->eth_lm_dual_tx_nloss - rmep_entry->eth_lm_dual_rx_nloss;

					if (floss > 0 && rmep_entry->eth_lm_dual_tx_floss > 0)
						rmep_entry->eth_lm_dual_fratio = floss*10000/rmep_entry->eth_lm_dual_tx_floss;
					if (nloss > 0 && rmep_entry->eth_lm_dual_tx_nloss > 0)
						rmep_entry->eth_lm_dual_nratio = nloss*10000/rmep_entry->eth_lm_dual_tx_nloss;
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0,
						"\nLoss Mesaurement (dual):\n"
						"md_level %d, mep %d, rmep %d:\n"
						"loss farend=%d (tx=%u rx=%u ratio=%u.%02u%%), nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, rmep_entry->rmep_id,
						floss, rmep_entry->eth_lm_dual_tx_floss, rmep_entry->eth_lm_dual_rx_floss, rmep_entry->eth_lm_dual_fratio/100, rmep_entry->eth_lm_dual_fratio %100,
						nloss, rmep_entry->eth_lm_dual_tx_nloss, rmep_entry->eth_lm_dual_rx_nloss, rmep_entry->eth_lm_dual_nratio/100, rmep_entry->eth_lm_dual_nratio %100);
				}

				rmep_entry->eth_lm_dual_stop_timer = -1;
				time(&rmep_entry->eth_lm_dual_stop_time);
			}else{
				dbprintf_cfm(LOG_ERR,"Remote MEIP not found!\n");
			}
			break;
		case CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config, 0, eth_lm_single_stop_timer_search, cfm_timer_msg->timer_id ,
						&rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_lm_single_stop_timer information for mep id %d is not found!\n",
						cfm_config->mep_id);
				break;
			}
			if (rmep_entry) {
				cfm_send_lmm(cfm_config, rmep_entry, -1, 0);
				rmep_entry->eth_lm_single_stop_check_timer =
					fwk_timer_create(cfm_timer_qid_g,
							CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP_CHECK,
							2000, // wait LMR for 2 sec
							(void *)cfm_config);
				time(&rmep_entry->eth_lm_single_stop_time);
			}else {
				dbprintf_cfm(LOG_ERR,"Remote MEIP not found!");
			}
			break;
		case CFM_TIMER_EVENT_ETH_LM_SINGLE_START_CHECK:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_LM_SINGLE_START_CHECK, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config, 0, eth_lm_single_start_check_timer_search, cfm_timer_msg->timer_id ,
						 &rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_lm_single_start_timer information for mep id %d(%x) is not found!\n",
						*(unsigned short *)cfm_timer_msg->user_data, *(unsigned short *)cfm_timer_msg->user_data);
				break;
			}
			if (rmep_entry->TxFCf_p == 0
					&& rmep_entry->RxFCf_p == 0
					&& rmep_entry->TxFCb_p == 0) {
				dbprintf_cfm(LOG_ERR,"rmep (%d) LMR_p not found!\n",rmep_entry->rmep_id);
				fwk_timer_delete(rmep_entry->eth_lm_single_start_check_timer );
			} else {//send lmm stop frame
				rmep_entry->eth_lm_single_stop_timer =
					fwk_timer_create(cfm_timer_qid_g,
							CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP,
							rmep_entry->eth_lm_single_repeat*1000 - 2000,
							(void *)cfm_config);
			}
			break;

		case CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP_CHECK:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_LM_SINGLE_STOP_CHECK, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config,0,eth_lm_single_stop_check_timer_search,cfm_timer_msg->timer_id ,
						&rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_lm_single_stop_check_timer information for mep id %d is not found!\n",
						cfm_config->mep_id );
				break;
			}
			if (rmep_entry ) {
				rmep_entry->eth_lm_single_stop_timer = 0;
				if (rmep_entry->TxFCf_c == 0
						&& rmep_entry->RxFCf_c == 0
						&& rmep_entry->TxFCb_c == 0
						&& rmep_entry->RxFCl_c == 0 ) {
					dbprintf_cfm(LOG_ERR,"LMR_c not found!\n");
					fwk_timer_delete(rmep_entry->eth_lm_single_stop_check_timer);
				}
			}
			break;
		case CFM_TIMER_EVENT_ETH_SLM_DUAL_STOP_CHECK:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_SLM_DUAL_STOP_CHECK, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config, 0, eth_slm_dual_stop_timer_search,cfm_timer_msg->timer_id , &rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_slm_dual_stop_check_timer information for mep id %d is not found!\n",
						cfm_config->mep_id );
				break;
			}
			if (rmep_entry) {
				if (rmep_entry->TxFCf_1sl_stop_check == rmep_entry->TxFCf_1sl
						|| rmep_entry->eth_slm_dual_force_stop  == 1 ) {
					rmep_entry->TxFCf_c = rmep_entry->TxFCf_1sl;
					rmep_entry->RxFCl_c = rmep_entry->RxFCl_1sl;

					rmep_entry->eth_slm_dual_tx_nloss = rmep_entry->TxFCf_c - rmep_entry->TxFCf_p;
					rmep_entry->eth_slm_dual_rx_nloss = rmep_entry->RxFCl_c - rmep_entry->RxFCl_p;

					dbprintf_cfm(LOG_WARNING,"nearend tx=TxFCf_c(%d)-TxFCf_p(%d), rx=RxFCl_c(%d)-RxFCl_p(%d)\n",
							rmep_entry->TxFCf_c, rmep_entry->TxFCf_p,
							rmep_entry->RxFCl_c, rmep_entry->RxFCl_p);

					{
						int nloss = rmep_entry->eth_slm_dual_tx_nloss - rmep_entry->eth_slm_dual_rx_nloss;

						if (nloss > 0 && rmep_entry->eth_slm_dual_tx_nloss > 0)
							rmep_entry->eth_slm_dual_nratio = nloss*10000/rmep_entry->eth_slm_dual_tx_nloss;

						util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0,
							"\nSynthetic Loss (dual):\n"
							"md_level %d, mep %d, rmep %d: testid %u\n"
							"loss nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
							cfm_config->md_level, cfm_config->mep_id, rmep_entry->rmep_id, rmep_entry->test_id_1sl,
							nloss, rmep_entry->eth_slm_dual_tx_nloss, rmep_entry->eth_slm_dual_rx_nloss, rmep_entry->eth_slm_dual_nratio/100, rmep_entry->eth_slm_dual_nratio%100);
					}
					//rmep_entry->test_id_1sl = 0;
					rmep_entry->eth_slm_dual_going_on = 0;
					rmep_entry->eth_slm_dual_force_stop  = 0;
					rmep_entry->eth_slm_dual_recv_timer = 0;
					time(&rmep_entry->eth_slm_dual_stop_time);
				}else {
					rmep_entry->TxFCf_1sl_stop_check = rmep_entry->TxFCf_1sl;
					rmep_entry->eth_slm_dual_recv_timer =
						fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ETH_SLM_DUAL_STOP_CHECK, 5*1000 , (void *)cfm_config);
				}

			}
			break;
		case CFM_TIMER_EVENT_ETH_SLM_DUAL_SENDER_LOOP:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_SLM_DUAL_SENDER_LOOP, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config , 0,
					eth_slm_dual_sender_loop_timer_search, cfm_timer_msg->timer_id, &rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_slm_dual_sender_loop_timer_search information for mep id %d is not found!\n",cfm_config->mep_id);
				break;
			}
			if ( rmep_entry ) {
				if ( rmep_entry->eth_slm_dual_repeat > 0 )  {
					rmep_entry->TxFCl_1sl++;
					cfm_send_1sl(cfm_config, rmep_entry, -1, 0);
					rmep_entry->eth_slm_dual_repeat--;
					rmep_entry->eth_slm_dual_sender_loop_timer =
							fwk_timer_create(cfm_timer_qid_g,
									CFM_TIMER_EVENT_ETH_SLM_DUAL_SENDER_LOOP,
									rmep_entry->eth_slm_dual_interval, (void *)cfm_config);
				}else{
					dbprintf_cfm(LOG_ERR,"SLM dual test stop!\n");
				}
			}else{
				dbprintf_cfm(LOG_ERR,"remote mepid (%d) not found.\n"
						,*(unsigned short *)cfm_timer_msg->user_data);

			}
			break;
		case CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_LOOP:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_LOOP, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config, 0, eth_slm_single_sender_loop_timer_search,
						cfm_timer_msg->timer_id, &rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_lm_single_stop_check_timer information for mep id %d is not found!\n",cfm_config->mep_id );
				break;
			}
			if ( rmep_entry ) {
				if ( rmep_entry->eth_slm_single_repeat > 0 )  {
					rmep_entry->eth_slm_single_sender_loop_timer =
							fwk_timer_create(cfm_timer_qid_g,
									CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_LOOP,
									rmep_entry->eth_slm_single_interval ,
									(void *)cfm_config);
					rmep_entry->TxFCl_slm++;
					rmep_entry->eth_slm_single_repeat--;
					cfm_send_slm(cfm_config, rmep_entry, -1, 0);
				}else{	// wait 1 more second after repeat is 0

					rmep_entry->eth_slm_single_sender_loop_timer = 0;
					rmep_entry->eth_slm_single_sender_stop_timer =
					fwk_timer_create(cfm_timer_qid_g,
							CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_STOP,
							1000, // 1 sec
							(void *)cfm_config);
				}
			}else{
				if (!rmep_entry)
					dbprintf_cfm(LOG_ERR,"remote mepid (%d) not found.\n"
							,*(unsigned short *)cfm_timer_msg->user_data);

			}
			break;
		case CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_STOP:
			cfm_config = (cfm_config_t *)cfm_timer_msg->user_data;
			dbprintf_cfm(LOG_DEBUG, "CFM_TIMER_EVENT_ETH_SLM_SINGLE_SENDER_STOP, %s\n", cfm_print_get_cfm_config_str(cfm_config));
			if (cfm_rmep_entry_find ( cfm_config, 0,
						eth_slm_single_sender_stop_timer_search,
						cfm_timer_msg->timer_id, &rmep_entry ) < 0 ) {
				dbprintf_cfm(LOG_ERR, "CCM with eth_lm_single_stop_check_timer information for mep id %d is not found!\n",cfm_config->mep_id );
				break;
			}
			if ( rmep_entry ) {
				rmep_entry->TxFCf_c = rmep_entry->TxFCf_slr;
				rmep_entry->TxFCb_c = rmep_entry->TxFCb_slr;
				rmep_entry->RxFCl_c = rmep_entry->RxFCl_slr;

				rmep_entry->eth_slm_single_tx_floss = rmep_entry->TxFCf_c - rmep_entry->TxFCf_p;
				rmep_entry->eth_slm_single_rx_floss = rmep_entry->TxFCb_c - rmep_entry->TxFCb_p;
				rmep_entry->eth_slm_single_tx_nloss = rmep_entry->TxFCb_c - rmep_entry->TxFCb_p;
				rmep_entry->eth_slm_single_rx_nloss = rmep_entry->RxFCl_c - rmep_entry->RxFCl_p;

				dbprintf_cfm(LOG_WARNING," farned tx=TxFCf_c(%d)-TxFCf_p(%d), rx=TxFCb_c(%d)-TxFCb_p(%d)\n",
						rmep_entry->TxFCf_c, rmep_entry->TxFCf_p,
						rmep_entry->TxFCb_c, rmep_entry->TxFCb_p);
				dbprintf_cfm(LOG_WARNING,"nearend tx=TxFCb_c(%d)-TxFCb_p(%d), rx=RxFCl_c(%d)-RxFCl_p(%d)\n",
						rmep_entry->TxFCb_c, rmep_entry->TxFCb_p,
						rmep_entry->RxFCl_c, rmep_entry->RxFCl_p);
				rmep_entry->eth_slm_single_sender_stop_timer = 0;

				if ( rmep_entry->eth_slm_single_force_stop == 0 ) {
					int floss = rmep_entry->eth_slm_single_tx_floss - rmep_entry->eth_slm_single_rx_floss;
					int nloss = rmep_entry->eth_slm_single_tx_nloss - rmep_entry->eth_slm_single_rx_nloss;

					if (floss > 0 && rmep_entry->eth_slm_single_tx_floss > 0)
						rmep_entry->eth_slm_single_fratio = floss*10000/rmep_entry->eth_slm_single_tx_floss;
					if (nloss > 0 && rmep_entry->eth_slm_single_tx_nloss > 0)
						rmep_entry->eth_slm_single_nratio = nloss*10000/rmep_entry->eth_slm_single_tx_nloss;
					util_dbprintf(omci_env_g.debug_level_cfm, LOG_ERR, 0,
						"\nSynthetic Loss (single):\n"
						"md_level %d, mep %d, rmep %d:\n"
						"loss farend=%d (tx=%u rx=%u ratio=%u.%02u%%), nearend=%d (tx=%u rx=%u ratio=%u.%02u%%)\n",
						cfm_config->md_level, cfm_config->mep_id, rmep_entry->rmep_id,
						floss, rmep_entry->eth_slm_single_tx_floss, rmep_entry->eth_slm_single_rx_floss, rmep_entry->eth_slm_single_fratio/100, rmep_entry->eth_slm_single_fratio%100,
						nloss, rmep_entry->eth_slm_single_tx_nloss, rmep_entry->eth_slm_single_rx_nloss, rmep_entry->eth_slm_single_nratio/100, rmep_entry->eth_slm_single_nratio%100);
				}

				rmep_entry->eth_slm_single_going_on = 0;
				time(&rmep_entry->eth_slm_single_stop_time);
			}

			break;
		default:
			dbprintf_cfm(LOG_ERR, "wrong CFM timer event\n");
			break;
	}
	if (cfm_timer_msg) {
		free_safe(cfm_timer_msg);
		cfm_timer_msg = NULL;
	}
	return 0;
}

static void *
cfm_core_function(void *ptr)
{
	int num = 0, ret;
	cfm_config_t *cfm_config;

	util_set_thread_name("omci-cfm");
	omci_env_g.taskname[ENV_TASK_NO_CFM] = "CFM";
	omci_env_g.taskid[ENV_TASK_NO_CFM] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_CFM] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_CFM] = 0;
	omci_env_g.taskts[ENV_TASK_NO_CFM] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_CFM] = 0; //idle

	cfm_alarm_timerid=fwk_timer_create(cfm_timer_qid_g, CFM_TIMER_EVENT_ALARM_CHECK, CFM_ALARM_CHECK_INTERVAL, NULL);
	list_for_each_entry(cfm_config, &cfm_config_list, node) {
		cfm_config->ccm_send_timer_id
			= fwk_timer_create(cfm_timer_qid_g,
					CFM_TIMER_EVENT_ETH_CC_PERIODIC_CHECK, CFM_CCM_TIMER_INTERVAL, NULL);
	}

	while (cfm_task_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_CFM]++;
		omci_env_g.taskts[ENV_TASK_NO_CFM] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_CFM] = 0; //idle

		if ((num = fwk_msgq_select(cfm_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0) {
			omci_env_g.taskstate[ENV_TASK_NO_CFM] = 1; //running
			// receive cfm packets
			if ((ret = fwk_msgq_set_isset(cfm_qsetid_g, cfm_pkt_recv_qid_g)) < 0) {
				dbprintf_cfm(LOG_ERR, "cfm_pkt_recv_qid_g isset err:%d \n", ret);
			} else if (ret > 0) {
				process_cfm_pkt_recv_q();
			}
			if ((ret = fwk_msgq_set_isset(cfm_qsetid_g, cfm_cmd_qid_g)) < 0) {
				dbprintf_cfm(LOG_ERR, "cfm_cmd_qid_g isset err(%d)\n", ret);
			} else if (ret > 0) {
				process_cfm_cmd_q();
			}
			if ((ret = fwk_msgq_set_isset(cfm_qsetid_g, cfm_timer_qid_g)) < 0) {
				dbprintf_cfm(LOG_ERR, "cfm_timer_qid_g isset err(%d)\n", ret);
			} else if (ret > 0) {
				process_cfm_timer_qid();
			}
		}
	}
	omci_env_g.taskstate[ENV_TASK_NO_CFM] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_CFM] = 0;
	dbprintf_cfm(LOG_ERR, "task terminate\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int
cfm_queue_init(void)
{
	int ret = -1;

	// alloc msgq & msgq set
	if ((cfm_pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_CFM_RECV")) < 0)
	{
		dbprintf_cfm(LOG_ERR, "cfm pkt queue alloc error(%d)\n", cfm_pkt_recv_qid_g);
		return (-1);
	}
	if ((cfm_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_CFM_CMD")) <0)
	{
		fwk_msgq_release(cfm_pkt_recv_qid_g);
		dbprintf_cfm(LOG_ERR, "cfm cmd queue alloc error(%d)\n", cfm_cmd_qid_g);
		return -1;
	}
	if ((cfm_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_CFM_TIMER")) < 0) {
		fwk_msgq_release(cfm_cmd_qid_g);
		fwk_msgq_release(cfm_pkt_recv_qid_g);
		dbprintf_cfm(LOG_ERR, "cfm timer queue alloc error(%d)\n", cfm_timer_qid_g);
		return (-1);
	}
	if ((cfm_qsetid_g = fwk_msgq_set_alloc()) < 0)
	{
		fwk_msgq_release(cfm_timer_qid_g);
		fwk_msgq_release(cfm_cmd_qid_g);
		fwk_msgq_release(cfm_pkt_recv_qid_g);
		dbprintf_cfm(LOG_ERR, "cfm queue set alloc error(%d)\n", cfm_qsetid_g);
		return -1;
	}

	// clear msgq set
	if ((fwk_msgq_set_zero(cfm_qsetid_g)) < 0)
	{
		fwk_msgq_set_release(cfm_qsetid_g);
		fwk_msgq_release(cfm_timer_qid_g);
		fwk_msgq_release(cfm_cmd_qid_g);
		fwk_msgq_release(cfm_pkt_recv_qid_g);
		dbprintf_cfm(LOG_ERR, "cfm queue set zero error(%d)\n", cfm_qsetid_g);
		return -1;
	}
	// set msgq set
	if (fwk_msgq_set_set(cfm_qsetid_g, cfm_pkt_recv_qid_g) <0 ||
	    fwk_msgq_set_set(cfm_qsetid_g, cfm_cmd_qid_g) < 0 ||
	    fwk_msgq_set_set(cfm_qsetid_g, cfm_timer_qid_g) <0)
	{
		fwk_msgq_set_release(cfm_qsetid_g);
		fwk_msgq_release(cfm_timer_qid_g);
		fwk_msgq_release(cfm_cmd_qid_g);
		fwk_msgq_release(cfm_pkt_recv_qid_g);
		dbprintf_cfm(LOG_ERR, "cfm queue set set error(%d)\n", ret);
		return -1;
	}

	return 0;
}

int
cfm_init(void)
{
	fwk_mutex_create(&cfm_util_get_time_mutex_g);

	cfm_queue_init();
 	cfm_config_list_init();

	return 0;
}

int
cfm_start(void)
{
	int ret;

	cfm_task_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	cfm_task_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_MED;
	cfm_task_thread_g.stack_size = 128*1024;
	cfm_task_thread_g.join = 1;
	sprintf(cfm_task_thread_g.name, "omci-CFM");
	cfm_task_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&cfm_task_thread_g, cfm_core_function, NULL)) < 0) {
		dbprintf_cfm(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}

	if (cpuport_extract_filter_register(CPUPORT_EXTRACT_8021AG, "802.1ag", cfm_pkt_recv_is_cfm, cfm_pkt_recv_qid_g)) {
		dbprintf_cfm(LOG_ERR, "cpupkt extract error(%d)\n", ret);
		return -1;
	}

	return 0;
}

int
cfm_stop(void)
{
	int ret;

	if (cpuport_extract_filter_unregister(CPUPORT_EXTRACT_8021AG))
		return -1;

	cfm_task_loop_g = 0;	//stop loop

	cfm_cmd_msg_t *cfm_cmd_msg;
	cfm_cmd_msg = malloc_safe(sizeof (cfm_cmd_msg_t));
	cfm_cmd_msg->cmd = CFM_CMD_TERMINATE;
	INIT_LIST_NODE(&cfm_cmd_msg->q_entry);
	if (fwk_msgq_send(cfm_cmd_qid_g, &cfm_cmd_msg->q_entry) < 0)
	{
		free_safe(cfm_cmd_msg);
	}
	if ((ret = fwk_thread_join(&cfm_task_thread_g)) < 0) {
		dbprintf_cfm(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}

int
cfm_shutdown(void)
{
	int ret;

	cfm_config_list_release();

	if ((ret = fwk_msgq_set_release(cfm_qsetid_g)) < 0)
		dbprintf_cfm(LOG_ERR, "set release error(%d)\n", ret);
	if ((ret = fwk_msgq_release(cfm_pkt_recv_qid_g)) < 0)
		dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
	if ((ret = fwk_msgq_release(cfm_timer_qid_g)) < 0)
		dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
	if ((ret = fwk_msgq_release(cfm_cmd_qid_g)) < 0)
		dbprintf_cfm(LOG_ERR, "release error(%d)\n", ret);
	cfm_pkt_recv_qid_g = -1;
	cfm_cmd_qid_g = -1;
	cfm_timer_qid_g = -1;

	fwk_mutex_destroy(&cfm_util_get_time_mutex_g);

	return 0;
}

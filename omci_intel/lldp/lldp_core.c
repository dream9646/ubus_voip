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
 * Module  : lldp
 * File    : lldp_core.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include "env.h"
#include "switch.h"
#include "cpuport_frame.h"
#include "cpuport_history.h"
#include "cpuport_extract_filter.h"
#include "lldp_core.h"
#include "metacfg_adapter.h"
#include "iphost.h"

#define LLDP_SYSPERIOD 1 // sec
#define LLDP_MIN_SYS_TIMER_PERIOD 500 // ms

int lldp_task_loop_g = 0;
static struct fwk_thread_t lldp_task_thread_g;
static int lldp_send_timerid = 0;
static int lldp_qsetid_g = -1;
int lldp_timer_qid_g = -1;
static int lldp_pkt_recv_qid_g = -1;
lldp_parm_t lldp_parm[SWITCH_LOGICAL_PORT_TOTAL];
lldp_pkt_t neighbor[SWITCH_LOGICAL_PORT_TOTAL];

unsigned char lldp_base_mac[IFHWADDRLEN];

///////////////////////////////////////////////////////////////////////////////
static int lldp_get_base_mac(void)
{
	char *ont_mac = util_get_bootenv("ethaddr");
	int values[IFHWADDRLEN];
	int i;

	if (ont_mac == NULL) {
		dbprintf_lldp(LOG_ERR, "Get Base Mac failed\n");
		return -1;
	}

	if( IFHWADDRLEN == sscanf( ont_mac, "%x:%x:%x:%x:%x:%x%*c",
				&values[0], &values[1], &values[2],
				&values[3], &values[4], &values[5]))
	{
		/* convert to char */
		for( i = 0; i < IFHWADDRLEN; ++i )
			lldp_base_mac[i] = (unsigned char) values[i];
	}
	else
	{
		dbprintf_lldp(LOG_ERR, "Invalid mac [%s]\n", ont_mac);
		return -1;
	}
	return 0;
}

#ifdef OMCI_METAFILE_ENABLE
static int
somethingChangedLocal(int port_id, lldp_mib_t *local_mib, int med_cap)
{
	lldp_mib_t *mib = &lldp_parm[port_id].mib;
	unsigned char d_mac[IFHWADDRLEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E};

	if (local_mib->adminStatus == LLDP_DISABLE || local_mib->adminStatus == LLDP_RXONLY) {
		if (mib->adminStatus == LLDP_RXTX || mib->adminStatus == LLDP_TXONLY) {
		dbprintf_lldp(LOG_NOTICE, "port[%d] from [%d] to [%d] : shutdown\n",
				port_id, mib->adminStatus, local_mib->adminStatus);

		lldp_parm[port_id].sm.is_shutdown = TRUE;
		mib->adminStatus = local_mib->adminStatus;
		return 0;
		}
	} else {

		if (mib->adminStatus != local_mib->adminStatus)
			mib->adminStatus = local_mib->adminStatus;

		d_mac[5] = omci_env_g.lldp_dst_mac;
		if (d_mac[5] != mib->tx_dst_mac[5]) {
			lldp_parm[port_id].sm.localChange = TRUE;
			memcpy(mib->tx_dst_mac, d_mac, IFHWADDRLEN);
		}

		if (memcmp(local_mib->tx_src_mac, mib->tx_src_mac, IFHWADDRLEN ) != 0) {
			lldp_parm[port_id].sm.localChange = TRUE;
			memcpy(mib->tx_src_mac, local_mib->tx_src_mac, IFHWADDRLEN);
		}

		if (med_cap & LLDP_MED_CAP_POLICY) {
			if (local_mib->network_policy.type != mib->network_policy.type ||
					local_mib->network_policy.policy != mib->network_policy.policy ||
					local_mib->network_policy.vid != mib->network_policy.vid ||
					local_mib->network_policy.pbit != mib->network_policy.pbit ||
					local_mib->network_policy.dscp != mib->network_policy.dscp ||
					local_mib->network_policy.tagged != mib->network_policy.tagged) {
				lldp_parm[port_id].sm.localChange = TRUE;
				memcpy(&mib->network_policy, &local_mib->network_policy, sizeof(lldp_network_policy_t));
			}
		}

		if (med_cap & LLDP_MED_CAP_MDI_PSE) {
			if (local_mib->power_priority != mib->power_priority) {
				lldp_parm[port_id].sm.localChange = TRUE;
				mib->power_priority = local_mib->power_priority;
			}
			if (local_mib->power_value != mib->power_value) {
				lldp_parm[port_id].sm.localChange = TRUE;
				mib->power_value = local_mib->power_value;
			}
		}
	}
	if (lldp_parm[port_id].sm.localChange == TRUE) {
		dbprintf_lldp(LOG_ERR, "port[%d] localChange\n", port_id);
	}
	return 0;
}
#endif

static int lldp_update_local_mib(void)
{
#ifdef OMCI_METAFILE_ENABLE
	int meta_value, med_cap;
	int MAYBE_UNUSED lldp_mgt;
	int port_id, poe_max_power;
	lldp_mib_t local_mib;

	struct metacfg_t kv_config_metafile;
	char* meta_state_str[] = {"lldp_port0_admin_state", "lldp_port1_admin_state", "lldp_port2_admin_state", "lldp_port3_admin_state"};
	char* meta_policy_str[] = {"lldp_port0_network_policy", "lldp_port1_network_policy", "lldp_port2_network_policy", "lldp_port3_network_policy"};
	char* meta_tagged_str[] = {"lldp_port0_tagged", "lldp_port1_tagged", "lldp_port2_tagged", "lldp_port3_tagged"};
	char* meta_vid_str[] = {"lldp_port0_vlan_vid", "lldp_port1_vlan_vid", "lldp_port2_vlan_vid", "lldp_port3_vlan_vid"};
	char* meta_pbit_str[] = {"lldp_port0_pbit", "lldp_port1_pbit", "lldp_port2_pbit", "lldp_port3_pbit"};
	char* meta_dscp_str[] = {"lldp_port0_dscp", "lldp_port1_dscp", "lldp_port2_dscp", "lldp_port3_dscp"};
	char* meta_poe_priority_str[] = {"poe_port0_priority", "poe_port1_priority", "poe_port2_priority", "poe_port3_priority"};
	char* meta_poe_high_power_str[] = {"poe_port0_high_power_mode", "poe_port1_high_power_mode", "poe_port2_high_power_mode", "poe_port3_high_power_mode"};

	local_mib.adminStatus = LLDP_DISABLE;
	iphost_get_hwaddr(omci_env_g.lldp_ifname, local_mib.tx_src_mac);

	memset(&kv_config_metafile, 0x0, sizeof(struct metacfg_t));
	metacfg_adapter_config_init(&kv_config_metafile);
	metacfg_adapter_config_file_load_part(&kv_config_metafile, "/etc/wwwctrl/metafile.dat", "poe_max_power_budget", "lldp_port3_dscp");

	lldp_mgt = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, "lldp_management_capability", 1));

	med_cap = lldp_get_med_cap();

	if (med_cap & LLDP_MED_CAP_MDI_PSE) {
		poe_max_power = (util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, "poe_max_power_budget", 1)) * 10);
	}

	for (port_id=0;port_id < SWITCH_LOGICAL_PORT_TOTAL;port_id++) {
		if ((1<<port_id) & switch_get_uni_logical_portmask()) {
			if (port_id < 4) {
				local_mib.adminStatus = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_state_str[port_id], 1));

				if (med_cap & LLDP_MED_CAP_POLICY) {
					/* Refer to TIA-1057 */
					local_mib.network_policy.type = 1; /* 10.2.3.1	Application Type : 1: voice */
					/* 10.2.3.2 Unknown Policy Flag : 0: defined 1: unknown */
					/* In "unknown" case, the VLAN ID, Layer 2 priority and DSCP value fields are ignored */
					meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_policy_str[port_id], 1));
					local_mib.network_policy.policy = meta_value;
					/* 10.2.3.3 Tagged Flag : 0:untagged 1: tagged */
					/* untagged : both the VLAN ID and the priority fields are ignored and only the DSCP value has relevance */
					/* tagged : both the VLAN ID and the Layer 2 priority values are being used */
					meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_tagged_str[port_id], 1));
					local_mib.network_policy.tagged = meta_value;

					meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_vid_str[port_id], 1));
					local_mib.network_policy.vid = meta_value;
					meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_pbit_str[port_id], 1));
					local_mib.network_policy.pbit = meta_value;
					meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_dscp_str[port_id], 1));
					local_mib.network_policy.dscp = meta_value;
				}
			}

			if (med_cap & LLDP_MED_CAP_MDI_PSE) {
				meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_poe_priority_str[port_id], 1));
				local_mib.power_priority = meta_value;
				meta_value = util_atoi(metacfg_adapter_config_get_value(&kv_config_metafile, meta_poe_high_power_str[port_id], 1));
				if (meta_value == 0) {
					local_mib.power_value = 150; // 0.1 W
				} else {
					local_mib.power_value = 300; // 0.1 W
				}

				if (poe_max_power < local_mib.power_value)
					local_mib.power_value = poe_max_power;
			}

			somethingChangedLocal(port_id, &local_mib, med_cap);
		}
	}

	metacfg_adapter_config_release(&kv_config_metafile);
#endif
	return 0;
}

static int
process_lldp_timer_qid(void)
{
	struct fwk_timer_msg_t *lldp_timer_msg=NULL;
	struct list_head *list_node = NULL;
	int ret;
	int port_id;
	struct timeval tv_prev;
	struct timeval tv;
	unsigned long msec;
	unsigned long sys_period_msec;

	if ((ret = fwk_msgq_recv(lldp_timer_qid_g, &list_node)) < 0) {
		dbprintf_lldp(LOG_ERR, "lldp_timer_qid_g recv err(%d)\n", ret);
		return -1;
	}

	lldp_timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);
	switch (lldp_timer_msg->event) {
		case LLDP_TIMER_SYSPERIOD:
			util_get_uptime(&tv);
			lldp_update_local_mib();

			for (port_id=0;port_id < SWITCH_LOGICAL_PORT_TOTAL;port_id++) {
				if ((1<<port_id) & switch_get_uni_logical_portmask()) {
					lldp_tx_timer_sm(port_id);
					if (lldp_parm[port_id].sm.neighbor_couter > 0) {
						if (lldp_parm[port_id].mib.rxTTL > 0) lldp_parm[port_id].mib.rxTTL--;
						if (lldp_parm[port_id].mib.rxTTL == 0) lldp_parm[port_id].sm.rxInfoAge = TRUE;
					}
				}
			}
			/* Correct timer because process time
			 * Cause lldp_update_local_mib() need 17ms to load metafile
			 * System period will become 1.017 sec
			 * If msgTxInterval is 30 will have error 0.017 * 30 = 0.51 sec become to 30.5 sec
			 * After correction tx interval is 30.05 sec
			 */
			util_get_uptime(&tv_prev);
			msec = (unsigned long)util_timeval_diff_usec(&tv_prev, &tv)/1000;
			sys_period_msec = (LLDP_SYSPERIOD*1000) - (msec > LLDP_MIN_SYS_TIMER_PERIOD ? LLDP_MIN_SYS_TIMER_PERIOD : msec);
			// prepare next timer
			dbprintf_lldp(LOG_DEBUG, "lldp timer: process time %d start a new timer %u\n", msec, sys_period_msec);
			lldp_send_timerid = fwk_timer_create(lldp_timer_qid_g, LLDP_TIMER_SYSPERIOD, sys_period_msec, NULL);
			break;
		default:
			dbprintf_lldp(LOG_ERR, "wrong LLDP timer event\n");
			break;
	}
	if (lldp_timer_msg) {
		free_safe(lldp_timer_msg);
		lldp_timer_msg = NULL;
	}
	return 0;
}


static int
process_lldp_pkt_recv_q(void)
{
	lldp_pktinfo_t *lldp_pktinfo = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	struct list_head *list_node = NULL;
	int ret;

	if ((ret = fwk_msgq_recv(lldp_pkt_recv_qid_g, &list_node)) < 0) {
		dbprintf_lldp(LOG_ERR, "lldp_pkt_recv_qid_g recv err:%d \n", ret);
		return -1;
	}
	if ((pktinfo = list_entry(list_node, struct cpuport_info_t, node)) == NULL)
		return -1;
	pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_LLDP];
	util_get_uptime(&pktinfo->process_time);
	if (omci_env_g.cpuport_history_enable)
		cpuport_history_add(pktinfo);
	dbprintf_lldp(LOG_INFO, "incoming packet pktinfo->frame_len %d\n", pktinfo->frame_len);
	//cpuport_sniffer_dbprint_pkt(LOG_DEBUG, "lldp_rx", pktinfo->frame_ptr, pktinfo->frame_len);
	/*if (pktinfo->rx_desc.bridge_port_me && pktinfo->tx_desc.bridge_port_me) {
		struct cpuport_tci_t dst_tci;
		if (cpuport_vtagging_vfilter(pktinfo->rx_desc.bridge_port_me, pktinfo->tx_desc.bridge_port_me, &pktinfo->tci, &dst_tci) != CPUPORT_VTAGGING_VFILTER_PASS) {
			if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
			if (pktinfo) free_safe(pktinfo);
			continue;
		}
	}*/
	lldp_pktinfo =(lldp_pktinfo_t *) malloc_safe(sizeof(lldp_pktinfo_t));
	if (lldp_pktinfo == NULL) {
		if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
		if (pktinfo) free_safe(pktinfo);
		return -1;
	}

	if (lldp_parm[pktinfo->rx_desc.logical_port_id].mib.adminStatus == LLDP_RXTX ||
	    lldp_parm[pktinfo->rx_desc.logical_port_id].mib.adminStatus == LLDP_RXONLY) {
		lldp_pkt_parse_pktinfo(pktinfo, lldp_pktinfo);
		lldp_recv_pkt_process(lldp_pktinfo);
	}


	util_get_uptime(&pktinfo->forward_time);
	if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
		dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
	if (omci_env_g.cpuport_history_enable)
		cpuport_history_sync(pktinfo);
	if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
	if (pktinfo) free_safe(pktinfo);
	if (lldp_pktinfo) free_safe(lldp_pktinfo);
	return 0;
}

static void *
lldp_core_function(void *ptr)
{
	int num = 0, ret;
	int port_id;

	util_set_thread_name("omci-lldp");
	omci_env_g.taskname[ENV_TASK_NO_LLDP] = "LLDP";
	omci_env_g.taskid[ENV_TASK_NO_LLDP] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_LLDP] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_LLDP] = 0;
	omci_env_g.taskts[ENV_TASK_NO_LLDP] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_LLDP] = 0; //idle

	lldp_send_timerid = fwk_timer_create(lldp_timer_qid_g, LLDP_TIMER_SYSPERIOD, LLDP_SYSPERIOD*1000, NULL);
	
	while (lldp_task_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_LLDP]++;
		omci_env_g.taskts[ENV_TASK_NO_LLDP] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_LLDP] = 0; //idle

		if ((num = fwk_msgq_select(lldp_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0) {
			omci_env_g.taskstate[ENV_TASK_NO_LLDP] = 1; //running
			if ((ret = fwk_msgq_set_isset(lldp_qsetid_g, lldp_timer_qid_g)) < 0) {
				dbprintf_lldp(LOG_ERR, "lldp_timer_qid_g isset err(%d)\n", ret);
			} else if (ret > 0) {
				process_lldp_timer_qid();
			}
			// receive lldp packets
			if ((ret = fwk_msgq_set_isset(lldp_qsetid_g, lldp_pkt_recv_qid_g)) < 0) {
				dbprintf_lldp(LOG_ERR, "lldp_pkt_recv_qid_g isset err:%d \n", ret);
			} else if (ret > 0) {
				process_lldp_pkt_recv_q();
			}

			for (port_id=0;port_id < SWITCH_LOGICAL_PORT_TOTAL;port_id++) {
				if ((1<<port_id) & switch_get_uni_logical_portmask()) {
					lldp_rx_sm(port_id);
					lldp_tx_sm(port_id);
				}
			}
		}
	}
	omci_env_g.taskstate[ENV_TASK_NO_LLDP] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_LLDP] = 0;
	dbprintf_lldp(LOG_ERR, "task terminate\n");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int
lldp_queue_init(void)
{
	int ret = -1;

	// alloc msgq & msgq set
	if ((lldp_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_LLDP_TIMER")) < 0) {
		dbprintf_lldp(LOG_ERR, "lldp timer queue alloc error(%d)\n", lldp_timer_qid_g);
		return (-1);
	}
	if ((lldp_pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_LLDP_RECV")) < 0) {
		dbprintf_lldp(LOG_ERR, "lldp pkt queue alloc error(%d)\n", lldp_pkt_recv_qid_g);
		fwk_msgq_release(lldp_timer_qid_g);
		return (-1);
	}
	if ((lldp_qsetid_g = fwk_msgq_set_alloc()) < 0)
	{
		fwk_msgq_release(lldp_timer_qid_g);
		fwk_msgq_release(lldp_pkt_recv_qid_g);
		dbprintf_lldp(LOG_ERR, "lldp queue set alloc error(%d)\n", lldp_qsetid_g);
		return -1;
	}

	// clear msgq set
	if ((fwk_msgq_set_zero(lldp_qsetid_g)) < 0)
	{
		fwk_msgq_set_release(lldp_qsetid_g);
		fwk_msgq_release(lldp_timer_qid_g);
		fwk_msgq_release(lldp_pkt_recv_qid_g);
		dbprintf_lldp(LOG_ERR, "lldp queue set zero error(%d)\n", lldp_qsetid_g);
		return -1;
	}
	// set msgq set
	if (fwk_msgq_set_set(lldp_qsetid_g, lldp_timer_qid_g) <0 || fwk_msgq_set_set(lldp_qsetid_g, lldp_pkt_recv_qid_g) <0)
	{
		fwk_msgq_set_release(lldp_qsetid_g);
		fwk_msgq_release(lldp_timer_qid_g);
		fwk_msgq_release(lldp_pkt_recv_qid_g);
		dbprintf_lldp(LOG_ERR, "lldp queue set set error(%d)\n", ret);
		return -1;
	}

	return 0;
}

static void lldp_parm_init(lldp_parm_t * lldp_param_p)
{
	memset(&lldp_param_p->mib, 0,  sizeof(lldp_mib_t));
	lldp_param_p->mib.adminStatus = LLDP_DISABLE;

	lldp_tx_timer_sm_init(&(lldp_param_p->sm));
	lldp_param_p->sm.neighbor_couter = 0;
	lldp_param_p->sm.rcvFrame = FALSE;
	lldp_param_p->sm.rxInfoAge = FALSE;

	memset(&lldp_param_p->pkt_c, 0,  sizeof(lldp_counter_t));
}


int
lldp_init(void)
{
	int i;
	lldp_queue_init();
	for (i=0;i<SWITCH_LOGICAL_PORT_TOTAL;i++)
		lldp_parm_init(&lldp_parm[i]);

	lldp_get_base_mac();

	return 0;
}

int
lldp_start(void)
{
	int ret;

	lldp_task_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	lldp_task_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_MED;
	lldp_task_thread_g.stack_size = 128*1024;
	lldp_task_thread_g.join = 1;
	sprintf(lldp_task_thread_g.name, "omci-LLDP");
	lldp_task_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&lldp_task_thread_g, lldp_core_function, NULL)) < 0) {
		dbprintf_lldp(LOG_ERR, "thread create error(%d)\n", ret);
		return -1;
	}

	if (cpuport_extract_filter_register(CPUPORT_EXTRACT_LLDP, "lldp", lldp_pkt_recv_is_lldp, lldp_pkt_recv_qid_g)) {
		dbprintf_lldp(LOG_ERR, "cpupkt extract error(%d)\n", ret);
		return -1;
	}

	return 0;
}

int
lldp_stop(void)
{
	int ret;

	if (cpuport_extract_filter_unregister(CPUPORT_EXTRACT_LLDP))
		return -1;

	lldp_task_loop_g = 0;	//stop loop

	if ((ret = fwk_thread_join(&lldp_task_thread_g)) < 0) {
		dbprintf_lldp(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}

int
lldp_shutdown(void)
{
	int ret;

	if ((ret = fwk_msgq_set_release(lldp_qsetid_g)) < 0)
		dbprintf_lldp(LOG_ERR, "set release error(%d)\n", ret);
	if ((ret = fwk_msgq_release(lldp_timer_qid_g)) < 0)
		dbprintf_lldp(LOG_ERR, "release error(%d)\n", ret);
	if ((ret = fwk_msgq_release(lldp_pkt_recv_qid_g)) < 0)
		dbprintf_lldp(LOG_ERR, "release error(%d)\n", ret);
	lldp_timer_qid_g = -1;
	lldp_pkt_recv_qid_g = -1;

	return 0;
}

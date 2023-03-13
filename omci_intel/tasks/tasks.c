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
 * Module  : tasks
 * File    : tasks.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netinet/ip.h>

#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "tranxmod.h"
#include "meinfo.h"

#include "clitask.h"
#include "coretask.h"
#include "recvtask.h"
#include "alarmtask.h"
#include "pmtask.h"
#include "avctask.h"
#include "testtask.h"
#include "cpuport.h"
#include "cpuport_recv.h"
#include "cpuport_pkt.h"
#include "mcast.h"
#include "cfm_core.h"
#include "lldp_core.h"
#include "cpuport_arp.h"
#include "cpuport_dhcp.h"
#include "cpuport_pppoe.h"
#include "extoam_entry.h"
#include "rstp.h"
#if 0
#include "voipcmdtask.h"
#endif

#ifndef X86
#include "meinfo_hw_util.h"
#endif

struct task_table_t {
	char *name;
	int (*init)(void);
	int (*start)(void);
	int (*stop)(void);
	int (*shutdown)(void);
	int (*is_enabled)(void);
};

static int is_enabled_always(void) 		{ return 1; }
static int is_enabled_igmp(void) 		{ return omci_env_g.igmp_enable; }

#if 0	// edwin, temporary remove for ifsw
static int is_enabled_cfm(void) 		{ return omci_env_g.cfm_enable; }
static int is_enabled_lldp(void) 		{ return omci_env_g.lldp_enable; }
static int is_enabled_cpuport_arp(void) 	{ return omci_env_g.cpuport_arp_enable; }
static int is_enabled_cpuport_dhcp(void)	{ return omci_env_g.cpuport_dhcp_enable; }
static int is_enabled_cpuport_pppoe(void) 	{ return omci_env_g.cpuport_pppoe_enable; }
static int is_enabled_extoam(void) 		{ return omci_env_g.extoam_enable; }
static int is_enabled_igmp(void) 		{ return omci_env_g.igmp_enable; }
static int is_enabled_rstp(void) 		{ return omci_env_g.stp_enable; }
#endif

static struct task_table_t task_table[]={
#if 1	// kevin, TBD
	{"clitask", clitask_init, clitask_start, clitask_stop, clitask_shutdown, is_enabled_always},
	{"coretask", coretask_init, coretask_start, coretask_stop, coretask_shutdown, is_enabled_always},
#ifdef INTEL_OMCC_LAYER
	{"recvtask", recvtask_init_intel_omcc, NULL, NULL, recvtask_shutdown_intel_omcc, is_enabled_always},
#else
	{"recvtask", recvtask_init, recvtask_start, recvtask_stop, recvtask_shutdown, is_enabled_always},
#endif
	{"alarmtask", alarmtask_init, alarmtask_start, alarmtask_stop, alarmtask_shutdown, is_enabled_always},
	{"pmtask", pmtask_init, pmtask_start, pmtask_stop, pmtask_shutdown, is_enabled_always},
	{"avctask", avctask_init, avctask_start, avctask_stop, avctask_shutdown, is_enabled_always},
	{"cpuporttask", cpuport_recv_init, cpuport_recv_start, cpuport_recv_stop, cpuport_recv_shutdown, is_enabled_always},
	{"testtask", testtask_init, testtask_start, testtask_stop, testtask_shutdown, is_enabled_always},
	{"igmp_proxy", igmp_proxy_init, igmp_proxy_start, igmp_proxy_stop, igmp_proxy_shutdown, is_enabled_igmp},
	{NULL, NULL, NULL, NULL, NULL}
#else
	{"clitask", clitask_init, clitask_start, clitask_stop, clitask_shutdown, is_enabled_always},
	{"coretask", coretask_init, coretask_start, coretask_stop, coretask_shutdown, is_enabled_always},
	{"recvtask", recvtask_init, recvtask_start, recvtask_stop, recvtask_shutdown, is_enabled_always},
	{"alarmtask", alarmtask_init, alarmtask_start, alarmtask_stop, alarmtask_shutdown, is_enabled_always},
	{"pmtask", pmtask_init, pmtask_start, pmtask_stop, pmtask_shutdown, is_enabled_always},
	{"avctask", avctask_init, avctask_start, avctask_stop, avctask_shutdown, is_enabled_always},
	{"cpuporttask", cpuport_recv_init, cpuport_recv_start, cpuport_recv_stop, cpuport_recv_shutdown, is_enabled_always},
	{"cpuport_pkt", cpuport_pkt_init, cpuport_pkt_start, cpuport_pkt_stop, cpuport_pkt_shutdown, is_enabled_always},
	{"testtask", testtask_init, testtask_start, testtask_stop, testtask_shutdown, is_enabled_always},

	{"cfm", cfm_init, cfm_start, cfm_stop, cfm_shutdown, is_enabled_cfm},
	{"lldp", lldp_init, lldp_start, lldp_stop, lldp_shutdown, is_enabled_lldp},
	{"cpuport_arp", cpuport_arp_init, cpuport_arp_start, cpuport_arp_stop, cpuport_arp_shutdown, is_enabled_cpuport_arp},
	{"cpuport_dhcp", cpuport_dhcp_init, cpuport_dhcp_start, cpuport_dhcp_stop, cpuport_dhcp_shutdown, is_enabled_cpuport_dhcp},
	{"cpuport_pppoe", cpuport_pppoe_init, cpuport_pppoe_start, cpuport_pppoe_stop, cpuport_pppoe_shutdown, is_enabled_cpuport_pppoe},
	{"extoam", extoamtask_init, extoamtask_start, extoamtask_stop, extoamtask_shutdown, is_enabled_extoam},
	{"igmp_proxy", igmp_proxy_init, igmp_proxy_start, igmp_proxy_stop, igmp_proxy_shutdown, is_enabled_igmp},
	{"rstp", rstp_init, rstp_start, rstp_stop, rstp_shutdown, is_enabled_rstp},
	{NULL, NULL, NULL, NULL, NULL}
#endif

};

int 
tasks_init(void)
{
	int i, j;
	
	for (i=0; task_table[i].name!=NULL; i++) {
		if (task_table[i].is_enabled() && task_table[i].init() <0) {
			dbprintf(LOG_ERR, "omci %s init error\n", task_table[i].name);
			for (j=i-1; j>=0; j--) {
				if (task_table[j].is_enabled())
					task_table[j].shutdown();
			}
			return -1;
		}
	}
	return 0;
}

int
tasks_shutdown(void)
{
	int i, j;

	// locate task table end
	for (i=0; task_table[i].name!=NULL; i++);
	// shutdown in reverse order
	for (j=i-1; j>=0; j--) {
		if (task_table[j].is_enabled() && task_table[j].shutdown() <0)
			dbprintf(LOG_ERR, "omci %s shutdown error\n", task_table[j].name);
	}
	return 0;
}

int
tasks_start(void)
{
	int i, j;
	
	for (i=0; task_table[i].name!=NULL; i++) {
		if (task_table[i].is_enabled() && task_table[i].start && task_table[i].start() <0) {
			dbprintf(LOG_ERR, "omci %s start error\n", task_table[i].name);
			for (j=i-1; j>=0; j--) {
				if (task_table[j].is_enabled())
					task_table[j].stop();
			}
			return -1;
		}
	}
	return 0;
}

int
tasks_stop(void)
{
	int i, j;

	// locate task table end
	for (i=0; task_table[i].name!=NULL; i++);
	// stop in reverse order
	for (j=i-1; j>=0; j--) {
		if (task_table[j].is_enabled() && task_table[j].stop && task_table[j].stop() <0)
			dbprintf(LOG_ERR, "omci %s stop error\n", task_table[j].name);
	}
	return 0;
}

int
tasks_util_is_avc_alarm_tca_allowed()
{
	int ontstate = 0;
#ifdef X86
	ontstate = 5;
#else
	ontstate = meinfo_hw_util_get_ontstate();	
#endif
	// no notification to olt if 
	// a. not O5
	// b. doing mibreset
	// c. doing mibupload
	// d. doing get_all_alarms
	// e. mib upload not happened after mib reset and mib reset finished till now is less than 60sec
	if (ontstate!=5 ||
	    omcimsg_mib_reset_clear_is_in_progress() || 
	    omcimsg_mib_upload_is_in_progress() || 
	    omcimsg_get_all_alarms_is_in_progress() ||
	    (omcimsg_mib_upload_after_reset_get()==0 && omcimsg_mib_reset_finish_time_till_now()<60))
	    	return 0;
	return 1;
}

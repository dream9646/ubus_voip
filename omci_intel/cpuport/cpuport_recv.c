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
 * Module  : cpuport
 * File    : cpuport_recv.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/if_ether.h>
#include <linux/ppp_defs.h>

#include "list.h"
#include "meinfo.h"
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "list.h"
#include "util.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_frame.h"
#include "cpuport_util.h"
#include "cpuport_recv.h"
#include "cpuport_pkt.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "cpuport_extract_filter.h"
#include "cpuport_util_send.h"

static struct fwk_thread_t pkt_recvtask_thread_g;
int pkt_recvtask_loop_g=0; 


// cpuport_info will be consumed by matched/default pkt handler or this routine itself (if error)
// so there is no need to free cpuport_info after calling this routine
// ret: CPUPORT_MATCH - dispatch to specific handler
//	CPUPORT_DROP - dispatch to specific handler, and dropped
//	CPUPORT_UNMATCH - dispatch to default pkt handler
int
cpuport_recv_dispatch(struct cpuport_info_t *cpuport_info)
{
	struct list_head *pos = NULL,*n = NULL;
	struct cpuport_extract_filter_t *cpuport_extract_filter = NULL;
	int pkt_state= CPUPORT_UNMATCH;

	util_get_uptime(&cpuport_info->recv_time);

	if (omci_env_g.debug_level_cpuport >=LOG_WARNING) {
		char *src_name = switch_get_logical_portname(cpuport_info->rx_desc.logical_port_id);
		dbprintf_cpuport(LOG_WARNING,"rx logical port=%d (%s)\n",cpuport_info->rx_desc.logical_port_id, src_name); 
		cpuport_sniffer_dbprint_pkt(LOG_WARNING, "cpu_rx", cpuport_info->frame_ptr, cpuport_info->frame_len, cpuport_info->rx_desc.router_mode);
	}
		
	// if mac learning limit is enabled and sa is not found in mactab, then drop the incoming pkt
	// over_maclimit_action_get will get drop state by l2 or rg
	// todo: if you want to be more rigorous, we can also check depth

	if (switch_hw_g.over_maclimit_action_get && cpuport_info->rx_desc.router_mode == 0) {
		unsigned int rx_portmask = 1<<cpuport_info->rx_desc.logical_port_id;		
		if ((rx_portmask & switch_get_uni_logical_portmask()) ||
		    (rx_portmask & switch_get_wan_logical_portmask()) ) {
			unsigned int is_drop_enabled = 0;
			unsigned int action;
			// mac limiting	by port
		    	if (switch_hw_g.over_maclimit_action_get(cpuport_info->rx_desc.logical_port_id, &action) == 0) {
		    		if (action == FORWARD_ACTION_DROP)
		    			is_drop_enabled = 1;
			}

			if (is_drop_enabled) {	// bypass maclimit for IGMP
				if (cpuport_info->tci.ethertype == ETH_P_IP ||
				    (cpuport_info->tci.ethertype == ETH_P_PPP_SES && cpuport_info->pppoe_proto == PPP_IP)) {
					struct iphdr *ip = cpuport_info->nh;
					if (ip && ip->protocol == IPPROTO_IGMP)
						is_drop_enabled = 0;	// cancel maclimit if this is an IGMP pkt
				}
			}

			if (is_drop_enabled) {			
				unsigned int portmask=switch_get_dmac_logical_portmask(cpuport_info->src_mac);
				if (portmask == 0) {	// macaddr portmask==0 means the macaddr is not learned in mactab
					dbprintf_cpuport(LOG_WARNING, "unknown sa %s dropped because over maclimit is enabled\n", util_str_macaddr(cpuport_info->src_mac)); 
					free_safe(cpuport_info->buf_ptr);
					free_safe(cpuport_info);
					return CPUPORT_DROP;
				}
			}
		}
	}

	// locate proper pkt handler for ingress packt 					
	list_for_each_safe( pos, n, &cpuport_extract_filter_g ) {					
		cpuport_extract_filter = list_entry(pos,struct cpuport_extract_filter_t,node);
		pkt_state = cpuport_extract_filter->filter_cb(cpuport_info);						
		if ( pkt_state == CPUPORT_MATCH ) {
			dbprintf_cpuport(LOG_NOTICE,"rx pkt type=%d (%s)\n\n", cpuport_extract_filter->pkt_type, cpuport_extract_filter->pkt_type_desc); 
			util_get_uptime(&cpuport_info->dispatch_time);
			if (fwk_msgq_send(cpuport_extract_filter->pkt_qid ,&cpuport_info->node) < 0){
				free_safe(cpuport_info->buf_ptr);
				free_safe(cpuport_info);
			}
			break;
		}
		if ( pkt_state == CPUPORT_DROP) {
			free_safe(cpuport_info->buf_ptr);
			free_safe(cpuport_info);
			break;
		}
	}
	
	// no pkt handler found, use generic for L2 forwarding or drop L3
	if ( pkt_state == CPUPORT_UNMATCH ) {
		if (cpuport_info->rx_desc.router_mode) {
			dbprintf_cpuport(LOG_NOTICE, "rx pkt type: no match, drop router_mode pkt\n\n"); 
			if (omci_env_g.cpuport_history_enable) {
				cpuport_info->thread_name = "L3drop";
				util_get_uptime(&cpuport_info->process_time);
				cpuport_info->forward_time = cpuport_info->process_time;
				cpuport_history_add(cpuport_info);
			}
			free_safe(cpuport_info->buf_ptr);
			free_safe(cpuport_info);
		} else {
			dbprintf_cpuport(LOG_NOTICE, "rx pkt type: no match, send to pkt task\n"); 
			util_get_uptime(&cpuport_info->dispatch_time);
			if (fwk_msgq_send(pkt_recv_qid_g ,&cpuport_info->node) < 0){
				free_safe(cpuport_info->buf_ptr);
				free_safe(cpuport_info);
			}
		}
	}

	return pkt_state;	
}

static void *
cpuport_recv_func(void *ptr)
{
	struct cpuport_info_t *cpuport_info;
	int recv_avail = 0;
	int ret = 0;

	util_set_thread_name("omci-cpuport");
	omci_env_g.taskname[ENV_TASK_NO_CPUPORT] = "CPUPORT";
	omci_env_g.taskid[ENV_TASK_NO_CPUPORT] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_CPUPORT] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_CPUPORT] = 0;
	omci_env_g.taskts[ENV_TASK_NO_CPUPORT] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_CPUPORT] = 0; //idle
	
	if (cpuport_hw_g.frame_recv_is_available == NULL)
		dbprintf_cpuport(LOG_ERR,"cpuport_hw_g.frame_recv_is_available == NULL\n");

	if (cpuport_hw_g.frame_recv == NULL)
		dbprintf_cpuport(LOG_ERR,"cpuport_hw_g.frame_recv == NULL\n");
		
	while(pkt_recvtask_loop_g)
	{
		omci_env_g.taskloopcount[ENV_TASK_NO_CPUPORT]++;
		omci_env_g.taskts[ENV_TASK_NO_CPUPORT] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_CPUPORT] = 0; //idle

		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_CPUPORT)) {
			msleep(100);
			continue;
		}

		recv_avail = cpuport_hw_g.frame_recv_is_available (NULL); 

		if ( recv_avail > 0  && cpuport_hw_g.frame_recv ) 
		{
			int i;

			omci_env_g.taskstate[ENV_TASK_NO_CPUPORT] = 1; //running
			
			for( i = 0; i < recv_avail ;i++)
			{
				// receive packets
				cpuport_info = (struct cpuport_info_t *)malloc_safe(sizeof(struct cpuport_info_t));
				if (cpuport_info == NULL) {
					dbprintf_cpuport(LOG_NOTICE,"fail to malloc cpuport_info\n"); 
					continue;
				}
				cpuport_info->buf_ptr = (unsigned char *)malloc_safe(2*CPUPORT_VTAG_LEN + CPUPORT_BUF_SIZE);
				if (cpuport_info->buf_ptr == NULL) {
					dbprintf_cpuport(LOG_NOTICE,"fail to malloc cpuport_info>buf_ptr\n"); 
					free_safe(cpuport_info);
					continue;
				}
				cpuport_info->frame_ptr = cpuport_info->buf_ptr + 2*CPUPORT_VTAG_LEN;
				cpuport_info->buf_len = CPUPORT_BUF_SIZE; 
				ret = cpuport_hw_g.frame_recv(cpuport_info, CPUPORT_BUF_SIZE); 

				if ( ret == 0 ) { // no pkt found in all socket, quit recv_avail loop
					free_safe(cpuport_info->buf_ptr);
					free_safe(cpuport_info);
					dbprintf_cpuport(LOG_NOTICE,"no pkt found in all socket, quit recv_avail loop\n"); 
					break;
					
				} else if ( ret < 0 ) { // error happend in specific socket
					free_safe(cpuport_info->buf_ptr);
					free_safe(cpuport_info);
					dbprintf_cpuport(LOG_NOTICE,"error happend in specific socket\n"); 

				} else if (ret  == 2 ) {	// dummy socket pkt to stop cpuport task
					pkt_recvtask_loop_g = 0;
					free_safe(cpuport_info->buf_ptr);
					free_safe(cpuport_info);

				} else if (ret  == 3) { // dummy socket pkt to leave select syscall
					free_safe(cpuport_info->buf_ptr);
					free_safe(cpuport_info);

				} else if (ret == 1) {	// pkt found in specific socket
					cpuport_recv_dispatch(cpuport_info);
				}
			}	// end of recv_avail loop
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_CPUPORT] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_CPUPORT] = 0;

	dbprintf_cpuport(LOG_ERR, "task terminate\n");
	return 0;
}

int
cpuport_recv_init(void)
{
	cpuport_extract_filter_init ();
	cpuport_history_init();
	if (cpuport_hw_g.init)
	{
		return cpuport_hw_g.init();
	}else{
		dbprintf_cpuport(LOG_ERR, "frame_recv_init err\n" );
		return -1;
	}
	return 0;
}
	
int
cpuport_recv_shutdown(void)
{
	cpuport_extract_filter_shutdown();
	if (cpuport_hw_g.shutdown == NULL) {
		dbprintf_cpuport(LOG_ERR, "shutdown == NULL?\n" );
	} else {
		cpuport_hw_g.shutdown();
	}
	return 0;
}

int
cpuport_recv_start(void )
{
	int ret = -1;
	pkt_recvtask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	pkt_recvtask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_RECV;
	pkt_recvtask_thread_g.stack_size = 128*1024;
	pkt_recvtask_thread_g.join = 1;
	sprintf(pkt_recvtask_thread_g.name, "omci-CPUPORT");
	pkt_recvtask_loop_g = 1;	//start loop

	if ((ret = fwk_thread_create(&pkt_recvtask_thread_g, cpuport_recv_func, NULL)) < 0) {
		dbprintf_cpuport(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}

	return 0;
}

int
cpuport_recv_stop(void )
{
	int ret = 0;

	pkt_recvtask_loop_g = 0;	//stop loop
	cpuport_frame_recv_abort_loop();
	if ((ret = fwk_thread_join(&pkt_recvtask_thread_g)) < 0) {
		dbprintf_cpuport(LOG_ERR, "join error(%d)\n", ret);
	}
	
	return 0;
}

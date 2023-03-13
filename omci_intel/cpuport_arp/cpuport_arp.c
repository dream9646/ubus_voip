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
 * Module  : cpuport_arp
 * File    : cpuport_arp.c
 *
 ******************************************************************/

#include <time.h>
#include <string.h>

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "util.h"
#include "env.h"
#include "cpuport.h"
#include "cpuport_extract_filter.h"
#include "cpuport_util.h"
#include "cpuport_frame.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "cpuport_util_send.h"
#include "hwresource.h"

#include "cpuport_arp.h"
#include "batchtab_cb.h"
#include "batchtab.h"
#include "meinfo.h"
#include "switch.h"

struct cpuport_arper_t cpuport_arper_g;
int arp_pkt_recv_qid_g = -1;
int arp_cmd_qid_g = -1;
int arp_qsetid_g = -1;
int arptask_loop_g = 0;
static struct fwk_thread_t arptask_thread_g;
int rtn;

int
cpuport_arp_pkt_filter( struct cpuport_info_t *pktinfo )
{
	//arp requet, arp respond, rarp request or rarp respond
	unsigned short arp_type = 0;
	int num_of_tags;
	
	if (pktinfo->rx_desc.router_mode)
		return CPUPORT_UNMATCH;
	
	num_of_tags = cpuport_util_get_tagnum_from_tci(&pktinfo->tci);
	if ( num_of_tags == 1 ) {
		if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+ 12 +4 )) != 0x0806 )
			return CPUPORT_UNMATCH;
		arp_type = ntohs( *(unsigned short *)( pktinfo->frame_ptr+ 24 ));
	}else if ( num_of_tags == 2 ) {
		if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+ 12 + 8 )) != 0x0806 )
			return CPUPORT_UNMATCH;
		arp_type = ntohs( *(unsigned short *)( pktinfo->frame_ptr+ 28 ));
	}else if (num_of_tags == 0 ) {
		if ( ntohs( *(unsigned short *)( pktinfo->frame_ptr+ 12 )) != 0x0806 )
			return CPUPORT_UNMATCH;
		arp_type = ntohs( *(unsigned short *)( pktinfo->frame_ptr+ 20 ));
	} else {
		return CPUPORT_UNMATCH;
	}

	if (arp_type == 1 || arp_type == 2 || arp_type == 3 || arp_type ==4)
		return CPUPORT_MATCH;
	return CPUPORT_UNMATCH;
}

static int
is_arp_over_limit( struct cpuport_info_t *pktinfo, char *prompt)
{
	int now = time(0);
	
	if (cpuport_arper_g.time != now)
		cpuport_arper_g.arp_total = 0;

	if (cpuport_arper_g.arp_limit == 0  ||
	    cpuport_arper_g.arp_total < cpuport_arper_g.arp_limit) {
		cpuport_arper_g.arp_total++;
		cpuport_arper_g.time = now;
		return 0;
	} else
		return 1;
}

/* packet from lan side send to wan */
static int
arp_send_wan( struct cpuport_info_t *pktinfo)
{
	cpuport_util_send_multi_wan(pktinfo, "arp-to-wan", is_arp_over_limit, NULL);
	return 0;
}

/* packet from wan side send to all uni port and cpu */
static int
arp_send_lan( struct cpuport_info_t *pktinfo)
{
	cpuport_util_send_multi_lan(pktinfo, "arp-to-lan", cpuport_util_is_ctc_internet_bridge_enable, NULL);
	return 0;
}

/* packet from lan side send to other uni ports and cpu */
static int
arp_send_other_lan( struct cpuport_info_t *pktinfo)
{
	cpuport_util_send_multi_lan_from_lan(pktinfo, "arp-to-other-lan", cpuport_util_is_ctc_internet_bridge_lan2lan);
	return 0;
}

static void *
cpuport_arp_function(void *ptr)
{
	struct list_head *list_node = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	int num;

	util_set_thread_name("omci-arp");
	omci_env_g.taskname[ENV_TASK_NO_ARP] = "ARP";
	omci_env_g.taskid[ENV_TASK_NO_ARP] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_ARP] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_ARP] = 0;
	omci_env_g.taskts[ENV_TASK_NO_ARP] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_ARP] = 0; //idle

	while ( arptask_loop_g )
	{
		omci_env_g.taskloopcount[ENV_TASK_NO_ARP]++;
		omci_env_g.taskts[ENV_TASK_NO_ARP] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_ARP] = 0; //idle
		
		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_ARP)) {
			msleep(100);
			continue;
		}
		if ((num = fwk_msgq_select(arp_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0)
		{
			int i = 0;

			omci_env_g.taskstate[ENV_TASK_NO_ARP] = 1; //running
			
			for( i = 0 ; i < num ; i ++ )
			{
				if ( fwk_msgq_set_isset(arp_qsetid_g, arp_pkt_recv_qid_g) )
				{
					if (fwk_msgq_recv(arp_pkt_recv_qid_g, &list_node) < 0) {
						dbprintf_cpuport(LOG_ERR, "arp_cmd_qid_g recv err\n");
					} else {
						pktinfo = list_entry(list_node, struct cpuport_info_t, node);
						pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_ARP];
						util_get_uptime(&pktinfo->process_time);
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_add(pktinfo);	
						
						//cpuport_sniffer_dbprint_pkt(LOG_DEBUG, "arp_rx", pktinfo->frame_ptr, pktinfo->frame_len);
						if (!pktinfo->rx_desc.bridge_port_me ) {
							if (!cpuport_util_rx_port_type_is_wan(pktinfo) && omci_env_g.localbridging_default) {
								dbprintf_cpuport(LOG_DEBUG, "packet arp come from lan side port=%d and send to other lan\n", pktinfo->rx_desc.logical_port_id);
								arp_send_other_lan(pktinfo);
							}
						} else {
							if( cpuport_util_rx_port_type_is_wan(pktinfo)) {
								dbprintf_cpuport(LOG_DEBUG, "packet arp come from wan side meid=%d and send to lan\n", pktinfo->rx_desc.bridge_port_me->meid);
								arp_send_lan(pktinfo);
							} else {
								dbprintf_cpuport(LOG_DEBUG, "packet arp come from lan side port=%d and send to wan\n", pktinfo->rx_desc.logical_port_id);
								if (omci_env_g.localbridging_default) {
									dbprintf_cpuport(LOG_DEBUG, "packet arp come from lan side port=%d and send to other lan\n", pktinfo->rx_desc.logical_port_id);
									arp_send_other_lan(pktinfo);
								}
								arp_send_wan(pktinfo);
							}
						}
						
						util_get_uptime(&pktinfo->forward_time);
						if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
							dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_sync(pktinfo);	
						
						free_safe(pktinfo->buf_ptr);
						free_safe(pktinfo);
					}
				}

				if ( fwk_msgq_set_isset(arp_qsetid_g, arp_cmd_qid_g) )
				{
					if (fwk_msgq_recv(arp_cmd_qid_g, &list_node) < 0)
					{
						dbprintf_cpuport(LOG_ERR, "arp_cmd_qid_g recv err\n");
					}else{
						struct arp_cmd_msg_t *arp_cmd_msg;
						arp_cmd_msg = list_entry(list_node, struct arp_cmd_msg_t, q_entry);

						if (arp_cmd_msg->cmd == ARP_CMD_LIMIT_CHANGE)
						{
							memset( &cpuport_arper_g, 0, sizeof(struct cpuport_arper_t ) );
							cpuport_arper_g.arp_limit = arp_cmd_msg->arp_limit;
						}

						if (arp_cmd_msg->cmd == ARP_CMD_TERMINATE)
						{
							arptask_loop_g = 0;
						}

						free_safe(arp_cmd_msg);
					}
				}
			}
		}
	}
	omci_env_g.taskstate[ENV_TASK_NO_ARP] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_ARP] = 0;

	dbprintf_cpuport(LOG_ERR, "task terminate\n");
	return 0;
}
int
cpuport_arp_init(void)
{
	int ret = 0;
	if ((arp_pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_ARP_RECV" ) ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "arp recv queue alloc error(%d)\n",arp_pkt_recv_qid_g );
		return (-1);
	}
	if ((arp_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_ARP_CMD") ) <0 )
	{
		dbprintf_cpuport(LOG_ERR, "arp cmd queue alloc error(%d)\n",arp_cmd_qid_g );
		return (-1);
	}
	if ((arp_qsetid_g = fwk_msgq_set_alloc())< 0)
	{
		fwk_msgq_release( arp_pkt_recv_qid_g );
		fwk_msgq_release( arp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set alloc error(%d)\n", arp_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( arp_qsetid_g ))< 0)
	{
		fwk_msgq_release( arp_pkt_recv_qid_g );
		fwk_msgq_release( arp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set zero error(%d)\n", arp_qsetid_g );
		return -1;
	}

	if ((ret = fwk_msgq_set_set( arp_qsetid_g ,arp_pkt_recv_qid_g ))<0 )
	{
		fwk_msgq_set_release( arp_qsetid_g );
		fwk_msgq_release( arp_pkt_recv_qid_g );
		fwk_msgq_release( arp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( arp_qsetid_g ,arp_cmd_qid_g))<0 )
	{
		fwk_msgq_set_release( arp_qsetid_g );
		fwk_msgq_release( arp_pkt_recv_qid_g );
		fwk_msgq_release( arp_cmd_qid_g );
		dbprintf_cpuport(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
cpuport_arp_shutdown(void)
{
	fwk_msgq_set_release( arp_qsetid_g );
	fwk_msgq_release( arp_pkt_recv_qid_g );
	fwk_msgq_release( arp_cmd_qid_g );
	return 0;
}

int
cpuport_arp_start(void)
{
	int ret;

	memset( &cpuport_arper_g, 0, sizeof(struct cpuport_arper_t ) );
	cpuport_arper_g.time = time(0);

	if ( cpuport_extract_filter_register(CPUPORT_EXTRACT_ARP, "arp", cpuport_arp_pkt_filter , arp_pkt_recv_qid_g) )
		return -1;
	arptask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	arptask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_MED;
	arptask_thread_g.stack_size = 128*1024;
	arptask_thread_g.join = 1;
	sprintf(arptask_thread_g.name, "omci-ARP");
	arptask_loop_g = 1;	//start loop
	if ((ret = fwk_thread_create(&arptask_thread_g, cpuport_arp_function, NULL)) < 0) {
		dbprintf_cpuport(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}


	return 0;
}

int
cpuport_arp_stop(void)
{
	int ret;

	if (cpuport_extract_filter_unregister(CPUPORT_EXTRACT_ARP))
		return -1;

	arptask_loop_g = 0;	//start loop

	struct arp_cmd_msg_t *arp_cmd_msg;
	arp_cmd_msg = malloc_safe(sizeof (struct arp_cmd_msg_t));
	arp_cmd_msg->cmd = ARP_CMD_TERMINATE;
	INIT_LIST_NODE(&arp_cmd_msg->q_entry);
	if (fwk_msgq_send(arp_cmd_qid_g, &arp_cmd_msg->q_entry) < 0)
	{
		free_safe(arp_cmd_msg);
	}
	if ((ret = fwk_thread_join(&arptask_thread_g)) < 0) {
		dbprintf_cpuport(LOG_ERR, "join error(%d)\n", ret);
	}
	return 0;
}

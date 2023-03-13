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
 * File    : igmp_main.c
 *
 ******************************************************************/


#include <stdlib.h>
#include <string.h>

#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "util.h"
#include "cpuport.h"
#include "cpuport_history.h"

#include "mcast.h"
#include "platform.h"

int timer_id_query_interval_g = 0;

static void
igmp_main_pktinfo_free(struct cpuport_info_t *pktinfo)
{
	if (!pktinfo) return;
		
	util_get_uptime(&pktinfo->forward_time);
	if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
		dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");

	if (pktinfo->buf_len > CPUPORT_BUF_SIZE || 
	    pktinfo->buf_len < 64 ||
	    pktinfo->frame_len > CPUPORT_BUF_SIZE || 
	    pktinfo->frame_len > pktinfo->buf_len || 
	    pktinfo->frame_ptr < pktinfo->buf_ptr)
	{
	    	// it seems when task is terminated, an invalid pkt returned from fwk_msgq_recv()?
		dbprintf_cpuport(LOG_ERR, "packt pktinfo 0x%p is invalid! (buf_ptr %p, buf_len %d, frame_ptr %p, frame_len %d\n",
			pktinfo, pktinfo->buf_ptr, pktinfo->buf_len, pktinfo->frame_ptr, pktinfo->frame_len);
		return;
	}
		
	if (omci_env_g.cpuport_history_enable) cpuport_history_sync(pktinfo);
	if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
	free_safe(pktinfo);
}

static int
handle_igmp_cmd_qid()
{
	int ret;
	struct list_head *list_node = NULL;

	ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_cmd_qid_g);
	if (ret < 0) {
		dbprintf_igmp(LOG_ERR, "igmp_cmd_qid_g isset err(%d)\n", ret);
		return ret;
	} else if (ret == 0) {
		return 0;
	}

	while ((ret = fwk_msgq_recv(igmp_cmd_qid_g, &list_node)) == 0) {
		struct igmp_cmd_msg_t *igmp_cmd_msg;
		igmp_cmd_msg = list_entry(list_node, struct igmp_cmd_msg_t, q_entry);

		if (!(THREAD_ENV.task_pause_mask & (1 << ENV_TASK_NO_IGMP))) {
			THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 1;
			igmp_config_g.daemon->igmp_cmd_hook(igmp_cmd_msg);
		}

		free_safe(igmp_cmd_msg);
	}

	if (ret != FWK_MSGQ_ERROR_EMPTY)
		dbprintf_igmp(LOG_ERR, "igmp_cmd_qid_g recv err(%d)\n", ret);

	return ret;
}

static int
handle_igmp_query_qid()
{
	int ret;
	struct cpuport_info_t *pktinfo = NULL;
	struct list_head *list_node = NULL;

	ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_query_qid_g);
	if (ret < 0) {
		dbprintf_igmp(LOG_ERR, "igmp_query_qid_g isset err: %d\n", ret);
		return ret;
	} else if (ret == 0) {
		return 0;
	}

	while ((ret = fwk_msgq_recv(igmp_query_qid_g, &list_node)) == 0) {
		pktinfo = list_entry(list_node, struct cpuport_info_t, node);
		if ((pktinfo == NULL) || !M_HAS_SRC_PORTID(pktinfo)) {
			dbprintf_igmp(LOG_WARNING, "pktinfo is NULL or Bridge port not found.\n");
		} else if (!(THREAD_ENV.task_pause_mask & (1 << ENV_TASK_NO_IGMP))) {
			THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 1;
			pktinfo->thread_name = THREAD_ENV.taskname[ENV_TASK_NO_IGMP];
			util_get_uptime(&pktinfo->process_time);

			if (omci_env_g.cpuport_history_enable)
				cpuport_history_add(pktinfo);

			igmp_config_g.daemon->igmp_query_hook(pktinfo);
		}
		igmp_main_pktinfo_free(pktinfo);
	}

	if (ret != FWK_MSGQ_ERROR_EMPTY)
		dbprintf_igmp(LOG_ERR, "igmp_query_qid_g recv err(%d)\n", ret);

        return ret;
}

static int
handle_igmp_timer_qid()
{
	int ret;
	struct fwk_timer_msg_t *timer_msg = NULL;
	struct list_head *list_node = NULL;

	ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_timer_qid_g);
	if (ret < 0) {
		dbprintf_igmp(LOG_ERR, "igmp_timer_qid_g isset err(%d) \n", ret);
		return ret;
	} else if (ret == 0) {
		return 0;
	}

	while ((ret = fwk_msgq_recv(igmp_timer_qid_g, &list_node)) == 0) {
		timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);
		if (!(THREAD_ENV.task_pause_mask & (1 << ENV_TASK_NO_IGMP))) {
			THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 1;
			igmp_config_g.daemon->igmp_timer_hook(timer_msg);
		}
		free_safe(timer_msg);
	}

	if (ret != FWK_MSGQ_ERROR_EMPTY)
		dbprintf_igmp(LOG_ERR, "igmp_timer_qid_g recv err: %d\n", ret);

	return ret;
}

static int
handle_igmp_join_leave_qid()
{
	int ret;
	struct cpuport_info_t *pktinfo = NULL;
	struct list_head *list_node = NULL;

	ret = fwk_msgq_set_isset(igmp_qsetid_g, igmp_join_leave_qid_g);
	if (ret < 0) {
		dbprintf_igmp(LOG_ERR, "igmp_join_leave_qid_g isset err: %d\n", ret);
		return ret;
	} else if (ret == 0) {
		return 0;
	}

	while ((ret = fwk_msgq_recv(igmp_join_leave_qid_g, &list_node)) == 0) {
		pktinfo = list_entry(list_node, struct cpuport_info_t, node);

		if (!(THREAD_ENV.task_pause_mask & (1 << ENV_TASK_NO_IGMP))) {
			THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 1;
			pktinfo->thread_name = THREAD_ENV.taskname[ENV_TASK_NO_IGMP];
			util_get_uptime(&pktinfo->process_time);

			if (omci_env_g.cpuport_history_enable)
				cpuport_history_add(pktinfo);

			igmp_config_g.daemon->igmp_joinleave_hook(pktinfo);
		}

		igmp_main_pktinfo_free(pktinfo);
	}

	if (ret != FWK_MSGQ_ERROR_EMPTY)
		dbprintf_igmp(LOG_ERR, "igmp_join_leave_qid_g recv err: %d\n", ret);

	return ret;
}

void *
igmp_main(void *ptr)
{
	int num;
	
	util_set_thread_name(THREAD_NAME);
	THREAD_ENV.taskname[ENV_TASK_NO_IGMP] = "IGMP";
	THREAD_ENV.taskid[ENV_TASK_NO_IGMP] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_IGMP] = pthread_self();
	THREAD_ENV.taskloopcount[ENV_TASK_NO_IGMP] = 0;
	THREAD_ENV.taskts[ENV_TASK_NO_IGMP] = 0;
	THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 0; //idle

	igmp_config_g.daemon->igmp_mainloop_before();
	
	while(igmpproxytask_loop_g) {
		THREAD_ENV.taskloopcount[ENV_TASK_NO_IGMP]++;
		THREAD_ENV.taskts[ENV_TASK_NO_IGMP] = util_get_uptime_sec();
		THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 0; //idle   

		if ((num = fwk_msgq_select(igmp_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) <= 0)
			continue;

		// receive igmp join/leave packets
		handle_igmp_join_leave_qid();

		// receive igmp query packets
		handle_igmp_query_qid();

		// receive timer timeout message
		handle_igmp_timer_qid();

		handle_igmp_cmd_qid();
	}
	
	igmp_config_g.daemon->igmp_mainloop_after();

	THREAD_ENV.taskstate[ENV_TASK_NO_IGMP] = 0; //idle
	THREAD_ENV.taskid[ENV_TASK_NO_IGMP] = 0;
	
	dbprintf_igmp(LOG_ERR, "task terminate\n");
	return 0;
}

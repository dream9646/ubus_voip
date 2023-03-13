/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT TRAF EXTENDED OAM
 * Module  : tasks
 * File    : extoamtask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "fwk_timer.h"
#include "list.h"
#include "env.h"
#include "util.h"
#include "switch.h"
#include "cpuport.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "cpuport_extract_filter.h"
#include "extoam.h"

#include "extoam_handle.h"
#include "extoam_cmd.h"
#include "extoam_queue.h"
#include "extoam_link.h"
#include "extoam_swdl.h"
#include "extoam_msg.h"
#include "extoam_loopback.h"
//
#define OAMTASK_BUFFER_SIZE 2048

static struct fwk_thread_t extoamtask_thread_g;
int extoam_qid_g = -1;
int extoam_timer_qid_g = -1;
int extoam_qsetid_g = -1; 
int extoamtask_loop_g = 0; 
int extoam_cmd_qid_g = -1;
struct extoam_cmd_msg_t *extoam_cmd_msg;
extern struct loopback_t loopback_g;

extern struct extoam_queue_t extoam_queue_g; 
extern struct extoam_link_status_t cpe_link_status_g;
extern struct sw_dl_proc_t cpe_rx_g;
extern struct sw_dl_proc_t cpe_tx_g;

static void *
extoamtask_function(void *ptr)
{
	unsigned char tx_buff[BUFF_LEN]; 
	unsigned char *tx_oam_buf; 
	int num;
	struct oam_pdu_t *oam_pdu= NULL;
	struct list_head *list_node;
	struct cpuport_info_t *pktinfo;

	int ret;

	util_set_thread_name("omci-extoam");
	omci_env_g.taskname[ENV_TASK_NO_EXTOAM] = "EXTOAM";
	omci_env_g.taskid[ENV_TASK_NO_EXTOAM] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_EXTOAM] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_EXTOAM] = 0;
	omci_env_g.taskts[ENV_TASK_NO_EXTOAM] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_EXTOAM] = 0; //idle

	fwk_thread_set_cancellable();

	extoam_link_discover(&cpe_link_status_g, &tx_oam_buf );
	if ( tx_oam_buf ){ 
		extoam_msg_send( tx_oam_buf , 60 );
		free_safe(tx_oam_buf);
	}

	while (extoamtask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_EXTOAM]++;
		omci_env_g.taskts[ENV_TASK_NO_EXTOAM] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_EXTOAM] = 0; //idle
	
		if ((num = fwk_msgq_select(extoam_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0){
			omci_env_g.taskstate[ENV_TASK_NO_EXTOAM] = 1; //running
			
			// receive extoam packets
			if ( (ret = fwk_msgq_set_isset(extoam_qsetid_g, extoam_qid_g )) < 0 ){
				dbprintf(LOG_ERR, "extoam_qid_g isset err:%d\n",ret);
			}else if (ret > 0 ){
				while ((ret = fwk_msgq_recv(extoam_qid_g , &list_node)) == 0) {
					pktinfo = list_entry(list_node, struct cpuport_info_t, node);
					pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_EXTOAM];
					util_get_uptime(&pktinfo->process_time);
					if (omci_env_g.cpuport_history_enable)
						cpuport_history_add(pktinfo);
					
					oam_pdu = (struct oam_pdu_t *)pktinfo->frame_ptr;
					extoam_cmd_dispatch ( oam_pdu , pktinfo->frame_len, &oam_handler_list_g );

					util_get_uptime(&pktinfo->forward_time);
					if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
						dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
					if (omci_env_g.cpuport_history_enable)
						cpuport_history_sync(pktinfo);

					free_safe(pktinfo->buf_ptr);
					free_safe(pktinfo);
				}
			}		

			if ((ret = fwk_msgq_set_isset( extoam_qsetid_g , extoam_timer_qid_g )) < 0 ) {
				dbprintf(LOG_ERR, "extoam_timer_qid_g isset err(%d)\n",ret );
			}else if ( ret > 0 ) {
				if ((ret = fwk_msgq_recv( extoam_timer_qid_g, &list_node ) ) < 0 )
					dbprintf(LOG_ERR,"extoam_timer_qid_g recv err: %d\n",ret);
				else{
					struct fwk_timer_msg_t *timer_msg; 
					timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);
					if ( timer_msg->event == TIMER_EVENT_CHECK_LINK_STAT) {
						/*
						 *current link state is not ACK(discover phase not finish), 
						 *and is the same with last state when the timer is triggered.
						 */

						cpe_link_status_g.last_link_state_timer_id = 0;
						if ( cpe_link_status_g.link_state != LINK_ACK ){

						//if ( cpe_link_status_g.link_state == LINK_UNLINK 
						//	|| cpe_link_status_g.link_state == LINK_DISCOVER)
							extoam_link_discover(&cpe_link_status_g, &tx_oam_buf );
							if ( tx_oam_buf && cpe_link_status_g.last_reset_timer_id == 0 ){ 
								extoam_msg_send( tx_oam_buf , 60 );
								free_safe(tx_oam_buf);
							}
						}
						
					}else if ( timer_msg->event == TIMER_EVENT_RM_QUEUE_CMD ) {
						struct extoam_cmd_t *extoam_cmd;
						unsigned short tr_id = *(unsigned short *)timer_msg->user_data;

						free_safe(timer_msg->user_data);

						extoam_cmd = extoam_queue_transaction_search( &extoam_queue_g , tr_id ); 
						if ( extoam_queue_g.count != 0 && extoam_cmd ) {
							if (extoam_cmd->finish == 0 ) {
								//todo: send cmd not finish event after 30 seconds
							}
							extoam_queue_del (&extoam_queue_g , tr_id );
						}
					} else if ( timer_msg->event == TIMER_EVENT_FILE_RX_TIMEOUT ) {

						unsigned short *block_num;
						if (timer_msg->user_data )
							block_num = (unsigned short *)(timer_msg->user_data);
						if ( cpe_rx_g.on == 0 || cpe_rx_g.block != *block_num ) {
							free_safe(block_num);
							continue;
						}
						if ( cpe_rx_g.retry >= 3 ) {

							if ( cpe_rx_g.fd )
								fclose(cpe_rx_g.fd);
							cpe_rx_g.on = 0;
							cpe_rx_g.transaction_id = 0;
							cpe_rx_g.block = 0;

							//todo: download fail and send error message
							extoam_link_reset( &cpe_link_status_g, &extoam_queue_g, &tx_oam_buf );
							if ( tx_oam_buf ){ 
								extoam_msg_send( tx_oam_buf , 60 );
								free_safe(tx_oam_buf);
							}
							free_safe(block_num);
							continue;
						}
						cpe_rx_g.retry++;
						//resend ACK
						extoam_msg_swdl_ack( cpe_link_status_g.link_id, cpe_rx_g.transaction_id,*block_num,SW_DL_AR_ACK,tx_buff);
						extoam_msg_send( tx_buff, 60);
						fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_RX_TIMEOUT, 500, (void *)block_num );
					} else if ( timer_msg->event == TIMER_EVENT_FILE_TX_TIMEOUT ) {

						if ( cpe_tx_g.fd == NULL || cpe_tx_g.size != 0 )  //file TX finished.
							continue;

						if ( cpe_tx_g.on == 0 )
							continue;

						if ( cpe_tx_g.block > cpe_tx_g.previous_block ) {
							//dbprintf(LOG_ERR, "cpe_tx_g.block %d cpe_tx_g.previous_block %d\n", cpe_tx_g.block, cpe_tx_g.previous_block );
							fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_TX_TIMEOUT, 500, NULL);
							cpe_tx_g.previous_block = cpe_tx_g.block; 
							continue;
						}

						if ( cpe_tx_g.retry >= 3 ) {
							//todo: download fail and send error message
							extoam_link_reset( &cpe_link_status_g, &extoam_queue_g, &tx_oam_buf );
							if ( tx_oam_buf ) {
								extoam_msg_send( tx_oam_buf , 60 );
							}
							cpe_tx_g.on = 0;
							cpe_tx_g.transaction_id = 0;
							cpe_tx_g.block = 0;
							continue;
						}
						cpe_tx_g.retry++;
						//resend data 
						extoam_swdl_file_block_tx( &cpe_tx_g );
						fwk_timer_create( extoam_timer_qid_g, TIMER_EVENT_FILE_TX_TIMEOUT, 2000, NULL);
					} else if ( timer_msg->event == TIMER_EVENT_FILE_TX_END_TIMEOUT ) {
						if ( cpe_tx_g.fd == NULL)
							continue;
					} else if ( timer_msg->event == TIMER_EVENT_CHECK_LINK_RESET ) {
						cpe_link_status_g.last_reset_timer_id = 0;
					} else if ( timer_msg->event == TIMER_EVENT_LOOPBACK_CHECK) {
						if ( loopback_g.timer_id != 0 )
							dbprintf(LOG_ERR,"Loopback test finished.......Fail!\n");
					}
					
				}
			}
		}
		if ( fwk_msgq_set_isset(extoam_qsetid_g, extoam_cmd_qid_g) )
		{
			omci_env_g.taskstate[ENV_TASK_NO_EXTOAM] = 1; //running
			
			if (fwk_msgq_recv(extoam_cmd_qid_g, &list_node) < 0)
			{
				dbprintf(LOG_ERR, "extoam_cmd_qid_g recv err\n");
			}else{
				struct extoam_cmd_msg_t *extoam_cmd_msg;
				extoam_cmd_msg = list_entry(list_node, struct extoam_cmd_msg_t, q_entry);


				if (extoam_cmd_msg->cmd == EXTOAM_CMD_TERMINATE)
				{
					extoamtask_loop_g = 0;
				}

				free_safe(extoam_cmd_msg);
			}
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_EXTOAM] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_EXTOAM] = 0;
	return 0;
}

static int
extoam_pkt_filter ( struct cpuport_info_t *pktinfo )
{
	struct oam_pdu_t *oam_pdu;

	if (pktinfo == NULL || pktinfo->frame_ptr == NULL ) {
		dbprintf(LOG_WARNING,"pktinfo || pktinfo->frame_ptr == NULL  \n");
		return CPUPORT_UNMATCH;
	}
	if (pktinfo->rx_desc.router_mode)
		return CPUPORT_UNMATCH;

	oam_pdu = (struct oam_pdu_t *)pktinfo->frame_ptr;
	if ( oam_pdu->ether_type == htons(0x8809) && oam_pdu->code == 0xFE ) {
		if ( oam_pdu->ext_oam.ext_code == htons(EXT_OP_LINK_DISCOVER) 
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_SW_DL)
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_GET_REQUEST)
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_GET_RESPONSE)
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_EVENT)
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_LOOPBACK)
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_SET_REQUEST)
				|| oam_pdu->ext_oam.ext_code == htons(EXT_OP_SET_RESPONSE) )
			return CPUPORT_MATCH;
	}
	return CPUPORT_UNMATCH;
}

static int
extoam_queue_set_init(void)
{
	int ret = -1;
	if ((extoam_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_GFAST_OAM" ) ) <0 )
	{
		dbprintf(LOG_ERR, "extoam pkt queue alloc error(%d)\n",extoam_qid_g );
		return (-1);
	}

	if ((extoam_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_EXTOAM_CMD") ) <0 )
	{
		dbprintf(LOG_ERR, "extoam cmd queue alloc error(%d)\n", extoam_cmd_qid_g );
		return (-1);
	}

	if ((extoam_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_GFAST_TIMER" ) ) <0 )
	{
		fwk_msgq_release( extoam_qid_g );
		dbprintf(LOG_ERR, "extoam timer queue alloc error(%d)\n",extoam_timer_qid_g );
		return (-1);
	}


	if ((extoam_qsetid_g = fwk_msgq_set_alloc())< 0)
	{
		fwk_msgq_release( extoam_qid_g );
		fwk_msgq_release( extoam_timer_qid_g );
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", extoam_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( extoam_qsetid_g ))< 0)
	{
		fwk_msgq_set_release( extoam_qsetid_g );
		fwk_msgq_release( extoam_qid_g );
		fwk_msgq_release( extoam_timer_qid_g );
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", extoam_qsetid_g );
		return -1;
	}

	if ((ret = fwk_msgq_set_set( extoam_qsetid_g , extoam_qid_g ))<0 )
	{
		fwk_msgq_set_release( extoam_qsetid_g );
		fwk_msgq_release( extoam_qid_g );
		fwk_msgq_release( extoam_timer_qid_g );
		dbprintf(LOG_ERR, "extoam queue set init error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( extoam_qsetid_g ,extoam_timer_qid_g))<0 )
	{
		fwk_msgq_set_release( extoam_qsetid_g );
		fwk_msgq_release( extoam_qid_g );
		fwk_msgq_release( extoam_timer_qid_g );
		dbprintf(LOG_ERR, "extoam timer queue set set error(%d)\n", ret);
		return -1;
	}
	if ((ret = fwk_msgq_set_set( extoam_qsetid_g , extoam_cmd_qid_g ))<0 )
	{
		fwk_msgq_set_release( extoam_qsetid_g );
		fwk_msgq_release( extoam_qid_g );
		fwk_msgq_release( extoam_timer_qid_g );
		fwk_msgq_release( extoam_cmd_qid_g );
		dbprintf(LOG_ERR, "extoam timer queue set set error(%d)\n", ret);
		return -1;
	}
	return 0;
}

static int
extoam_queue_deinit(void)
{
	if (extoam_qsetid_g != -1 ) {
		if ( extoam_qid_g != -1 ) {
			fwk_msgq_set_clear( extoam_qsetid_g, extoam_qid_g );
			fwk_msgq_release( extoam_qid_g );
		}

		if ( extoam_timer_qid_g != -1 ) {
			fwk_msgq_set_clear( extoam_qsetid_g, extoam_timer_qid_g );
			fwk_msgq_release( extoam_timer_qid_g );
		}

		fwk_msgq_set_release(extoam_qsetid_g);
	}

	return 0;
}
int
extoamtask_init(void)
{
	extoam_queue_set_init();
	extoam_handler_init();
	extoam_queue_init ( &extoam_queue_g );
	extoam_link_init(&cpe_link_status_g);
	extoam_loopback_init( &loopback_g);
	cpe_link_status_g.trap_state = 1;

	return 0;
}

int
extoamtask_start(void)
{
	int ret;

	if ( cpuport_extract_filter_register( CPUPORT_EXTRACT_EXT_OAM, "extoam", extoam_pkt_filter , extoam_qid_g ) )
		return -1;

	/*create extoam task */
	extoamtask_thread_g.policy = FWK_THREAD_POLICY_OMCI_CORE;
	extoamtask_thread_g.priority = FWK_THREAD_PRIO_OMCI_CORE;
	extoamtask_thread_g.stack_size = 128*1024;
	extoamtask_thread_g.join = 1;
	sprintf(extoamtask_thread_g.name, "extoam-gfast");

	extoamtask_loop_g = 1;	//start loop

	if ((ret = fwk_thread_create(&extoamtask_thread_g, extoamtask_function, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}
	if (switch_hw_g.extoam_extract_enable_set == NULL) {
		dbprintf(LOG_ERR, "switch_hw_g.extoam_extract_enable_set = NULL\n");
		return -1;
	}
	return switch_hw_g.extoam_extract_enable_set(1);
}

int
extoamtask_shutdown(void)
{


	extoam_handler_deinit();
	extoam_queue_deinit();
	extoam_loopback_deinit( &loopback_g);

	fwk_msgq_release( extoam_cmd_qid_g );

	return 0;
}

int
extoamtask_stop(void)
{
	int ret;
	struct extoam_cmd_msg_t *extoam_cmd_msg;

	if (switch_hw_g.extoam_extract_enable_set != NULL)
		switch_hw_g.extoam_extract_enable_set(0);

	if ( cpuport_extract_filter_unregister(CPUPORT_EXTRACT_EXT_OAM) )
		return -1;

	extoam_cmd_msg = malloc_safe(sizeof (struct extoam_cmd_msg_t));
	extoam_cmd_msg->cmd = EXTOAM_CMD_TERMINATE;
	INIT_LIST_NODE(&extoam_cmd_msg->q_entry);
	if (fwk_msgq_send(extoam_cmd_qid_g, &extoam_cmd_msg->q_entry) < 0)
	{
		free_safe(extoam_cmd_msg);
	}
	if ((ret = fwk_thread_join(&extoamtask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}

	return 0;
}

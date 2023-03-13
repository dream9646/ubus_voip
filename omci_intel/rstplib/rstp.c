#include <time.h>
#include <string.h>

#include "list.h"
#include "fwk_msgq.h"
#include "fwk_thread.h"
#include "fwk_timer.h"
#include "util.h"
#include "env.h"
#include "cpuport.h"
#include "cpuport_extract_filter.h"
#include "cpuport_util.h"
#include "cpuport_sniffer.h"
#include "cpuport_history.h"
#include "switch.h"
#include "hwresource.h"

#include "uid.h"
#include "bridge.h"
#include "base.h"
#include "stp_to.h"
#include "stp_in.h"

#include "rstp.h"
#include "rstp_omci.h"
#include "rstp_port.h"
#include "rstp_cmd_handler.h"

int rstp_pkt_recv_qid_g = -1;
int rstp_cmd_qid_g = -1;
int rstp_qsetid_g = -1;
int rstp_timer_qid_g = -1;

int rstptask_loop_g = 0;
static unsigned char link_status[MAX_STP_PORTS];

static struct fwk_thread_t rstptask_thread_g;

int 
rstp_port_link_status_change(int port_index) {

	unsigned char prev_link_status;

	prev_link_status = link_status[port_index-1];
	link_status[port_index-1] = STP_OUT_get_port_link_status (port_index);

	if ( prev_link_status==0 && link_status[port_index-1]==1 )
		return 1;
	if ( prev_link_status==1 && link_status[port_index-1]==0 )
		return 2;

	return 0;
}

int 
rstp_pkt_filter( struct cpuport_info_t *pktinfo ) {

	if (pktinfo->rx_desc.router_mode)
		return CPUPORT_UNMATCH;

	if( ntohl(*(unsigned long *)pktinfo->frame_ptr) != 0x0180c200 )
		return CPUPORT_UNMATCH;

	if( *(unsigned short *)(pktinfo->frame_ptr+0x4) != 0 )
		return CPUPORT_UNMATCH;

	if(*(unsigned char *)(pktinfo->frame_ptr+14) == 0x42 
				&& *(unsigned char *)(pktinfo->frame_ptr+15) == 0x42
				&& *(unsigned short *)(pktinfo->frame_ptr+17) == 0)
		return CPUPORT_MATCH; 

	if(*(unsigned char *)(pktinfo->frame_ptr+18) == 0x42 
				&& *(unsigned char *)(pktinfo->frame_ptr+19) == 0x42
				&& *(unsigned short *)(pktinfo->frame_ptr+21) == 0 )
		return CPUPORT_MATCH; 

	if(*(unsigned char *)(pktinfo->frame_ptr+22) == 0x42 
				&& *(unsigned char *)(pktinfo->frame_ptr+23) == 0x42
				&& *(unsigned short *)(pktinfo->frame_ptr+25) == 0  )
		return CPUPORT_MATCH;

	return CPUPORT_UNMATCH;
}

static int
rstp_pktinfo_to_rstplib_msg(struct cpuport_info_t *pktinfo, UID_MSG_T* msg, struct cpuport_desc_t stp_port[]) {

	unsigned char *buf_ptr;

	if ( pktinfo == NULL || msg == NULL)
	{
		dbprintf(LOG_ERR, "pktinfo || msg == NULL\n" );
		return -1;
	}


	msg->header.cmd_type = UID_BPDU;
	msg->header.body_len = pktinfo->frame_len;

	buf_ptr = (unsigned char *)msg;

	memcpy(buf_ptr+ sizeof( UID_MSG_HEADER_T ), pktinfo->frame_ptr, pktinfo->frame_len);
	msg->header.destination_port = pktinfo->rx_desc.logical_port_id + 1; 
	return 0;
}


static void *
rstp_function(void *ptr) {

	struct list_head *list_node = NULL;
	struct cpuport_info_t *pktinfo = NULL;
	int num = 0;
	UID_MSG_T *msg = NULL;
//	unsigned char *buf_ptr=NULL; 
	struct fwk_timer_msg_t *timer_msg;
	int ret;

	util_set_thread_name("omci-rstp");
	omci_env_g.taskname[ENV_TASK_NO_RSTP] = "RSTP";
	omci_env_g.taskid[ENV_TASK_NO_RSTP] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_RSTP] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_RSTP] = 0;
	omci_env_g.taskts[ENV_TASK_NO_RSTP] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_RSTP] = 0; //idle

	fwk_timer_create(rstp_timer_qid_g, RSTP_EVENT_STP_IN_ONE_SEC, 1000, NULL);
	fwk_timer_create(rstp_timer_qid_g, RSTP_EVENT_PORT_CONNECTION_CHECK, 4000, NULL);

	while ( rstptask_loop_g ) {
		omci_env_g.taskloopcount[ENV_TASK_NO_RSTP]++;
		omci_env_g.taskts[ENV_TASK_NO_RSTP] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_RSTP] = 0; //idle

		if ((num = fwk_msgq_select(rstp_qsetid_g, FWK_MSGQ_WAIT_INFINITE)) > 0) {

			int i = 0;
			omci_env_g.taskstate[ENV_TASK_NO_RSTP] = 1; //running
			while ( i< num) {
				if ( fwk_msgq_set_isset(rstp_qsetid_g, rstp_pkt_recv_qid_g) >0 ) {

					if ((ret = fwk_msgq_recv(rstp_pkt_recv_qid_g, &list_node)) < 0) {
						dbprintf(LOG_ERR, "rstp_pkt_qid_g recv err 0x%x\n",ret);
					}else{
						pktinfo = list_entry(list_node, struct cpuport_info_t, node);
						pktinfo->thread_name = omci_env_g.taskname[ENV_TASK_NO_RSTP];
						util_get_uptime(&pktinfo->process_time);
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_add(pktinfo);

						cpuport_sniffer_dbprint_pkt(LOG_WARNING, "rstp_rx", pktinfo->frame_ptr, pktinfo->frame_len, pktinfo->rx_desc.router_mode);

						msg = (UID_MSG_T*) malloc_safe(sizeof(UID_MSG_T));

						if (rstp_pktinfo_to_rstplib_msg(pktinfo,msg,stp_port) == 0){
							bridge_rx_bpdu (msg, sizeof(UID_MSG_T));
						}

						util_get_uptime(&pktinfo->forward_time);
						if (util_timeval_diff_usec(&pktinfo->forward_time, &pktinfo->recv_time) > 5*1E6)
							dbprintf_cpuport(LOG_ERR, "packet is processed for more than 5 sec, system busy?\n");
						if (omci_env_g.cpuport_history_enable)
							cpuport_history_sync(pktinfo);

						if (pktinfo->buf_ptr) free_safe(pktinfo->buf_ptr);
						free_safe(pktinfo);
						free_safe(msg);
					}
					i++;
				}

				if ( fwk_msgq_set_isset(rstp_qsetid_g, rstp_timer_qid_g) >0 ) {
					if ((ret = fwk_msgq_recv(rstp_timer_qid_g, &list_node)) < 0) {
						dbprintf(LOG_ERR, "rstp_timer_qid_g recv err 0x%x\n",ret);
					}else{

						timer_msg = list_entry(list_node, struct fwk_timer_msg_t, node);

						if ( timer_msg->event == RSTP_EVENT_STP_IN_ONE_SEC ) {

							STP_IN_one_second ();
							fwk_timer_create(rstp_timer_qid_g, RSTP_EVENT_STP_IN_ONE_SEC, 1000, NULL);
						}

						if ( timer_msg->event == RSTP_EVENT_PORT_CONNECTION_CHECK ) {

							int i = 0;
							for ( i = 0; i <MAX_STP_PORTS; i++) {
								int rtn;

								rtn = rstp_port_link_status_change(i+1);
								// 1: from disconnect to connect 
								// 2: from connect to disconnect
								if( rtn == 1 ) 
									STP_IN_enable_port (i+1, True);
								else if( rtn == 2 ) 
									STP_IN_enable_port (i+1, False);
							}
							fwk_timer_create(rstp_timer_qid_g, RSTP_EVENT_PORT_CONNECTION_CHECK , 4000, NULL);
						}
						fwk_timer_delete(timer_msg->timer_id);
						free_safe(timer_msg);
					}
					i++;
				}

				if ( fwk_msgq_set_isset(rstp_qsetid_g, rstp_cmd_qid_g) >0 ) {
					if ((ret = fwk_msgq_recv(rstp_cmd_qid_g, &list_node)) < 0) {
						dbprintf(LOG_ERR, "rstp_cmd_qid_g recv err 0x%x\n",ret);
					}else{
						struct rstp_cmd_msg_t *rstp_cmd_msg;
						rstp_cmd_msg = list_entry(list_node, struct rstp_cmd_msg_t, q_entry);

						switch(rstp_cmd_msg->cmd) {

							case RSTP_CMD_ME_ADD_MSG:
								rstp_cmd_me_add_handler( rstp_cmd_msg);
								break;
							case RSTP_CMD_ME_UPDATE_MSG:
								rstp_cmd_me_update_handler( rstp_cmd_msg);
								break;
							case RSTP_CMD_ME_DEL_MSG:
								rstp_cmd_me_del_handler( rstp_cmd_msg);
								break;
							case RSTP_CMD_TERMINATE:
								break;
							default:
								dbprintf(LOG_ERR, "wrong RSTP msg cmd\n");
						}
						 
						free_safe(rstp_cmd_msg);
					}
					i++;
				}
			}
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_RSTP] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_RSTP] = 0;

	dbprintf(LOG_ERR, "task terminate\n");
	return 0;
}

int
rstp_init(void) {

	int ret = 0;
	if ((rstp_pkt_recv_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, FWK_MSGQ_MAX_QUEUE_SIZE, "Q_RTSP_RECV" ) ) <0 ) {
		dbprintf(LOG_ERR, "rstp recv queue alloc error(%d)\n",rstp_pkt_recv_qid_g );
		return (-1);
	}

	if ((rstp_cmd_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 5, "Q_RTSP_CMD") ) <0 ) {
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		dbprintf(LOG_ERR, "rstp cmd queue alloc error(%d)\n",rstp_cmd_qid_g );
		return (-1);
	}

	if ((rstp_timer_qid_g = fwk_msgq_alloc(FWK_MSGQ_ALLOC_ID_UNASSIGN, 10, "Q_RTSP_TIMER") ) <0 ) {
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		fwk_msgq_release( rstp_cmd_qid_g );
		dbprintf(LOG_ERR, "rstp timer queue alloc error(%d)\n",rstp_timer_qid_g );
		return (-1);
	}

	if ((rstp_qsetid_g = fwk_msgq_set_alloc())< 0) {
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		fwk_msgq_release( rstp_cmd_qid_g );
		fwk_msgq_release( rstp_timer_qid_g );
		dbprintf(LOG_ERR, "queue set alloc error(%d)\n", rstp_qsetid_g);
		return -1;
	}

	if ((fwk_msgq_set_zero( rstp_qsetid_g ))< 0){

		fwk_msgq_set_release( rstp_qsetid_g );
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		fwk_msgq_release( rstp_cmd_qid_g );
		fwk_msgq_release( rstp_timer_qid_g );
		dbprintf(LOG_ERR, "queue set zero error(%d)\n", rstp_qsetid_g );
		return -1;
	}

	if ((ret = fwk_msgq_set_set( rstp_qsetid_g ,rstp_timer_qid_g ))<0 ) {

		fwk_msgq_set_release( rstp_qsetid_g );
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		fwk_msgq_release( rstp_cmd_qid_g );
		fwk_msgq_release( rstp_timer_qid_g );
	}

	if ((ret = fwk_msgq_set_set( rstp_qsetid_g ,rstp_pkt_recv_qid_g ))<0 ) {

		fwk_msgq_set_release( rstp_qsetid_g );
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		fwk_msgq_release( rstp_cmd_qid_g );
		fwk_msgq_release( rstp_timer_qid_g );
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}

	if ((ret = fwk_msgq_set_set( rstp_qsetid_g ,rstp_cmd_qid_g))<0 ) {

		fwk_msgq_set_release( rstp_qsetid_g );
		fwk_msgq_release( rstp_pkt_recv_qid_g );
		fwk_msgq_release( rstp_cmd_qid_g );
		fwk_msgq_release( rstp_timer_qid_g );
		dbprintf(LOG_ERR, "queue set set error(%d)\n", ret);
		return -1;
	}
	return 0;
}

int
rstp_shutdown(void) {

	fwk_msgq_set_release( rstp_qsetid_g );
	fwk_msgq_release( rstp_pkt_recv_qid_g );
	fwk_msgq_release( rstp_cmd_qid_g );
	fwk_msgq_release( rstp_timer_qid_g );
	return 0;
}

int
rstp_start(void) {

	int ret;

	if ( cpuport_extract_filter_register( CPUPORT_EXTRACT_BPDU, "rstp", rstp_pkt_filter, rstp_pkt_recv_qid_g) )
		return -1;

	if (rstp_omci_stp_bridge_instance_check() == 0) {
		if ( bridge_start()<0 )
			return -1;
	}

	rstp_omci_notify_chain_init(); 

	rstptask_thread_g.policy = FWK_THREAD_POLICY_CPUPORT_HANDLER;
	rstptask_thread_g.priority = FWK_THREAD_PRIO_CPUPORT_MED;
	rstptask_thread_g.stack_size = 128*1024;
	rstptask_thread_g.join = 1;
	sprintf(rstptask_thread_g.name, "omci-RSTP");
	rstptask_loop_g = 1;	//start loop

	if ((ret = fwk_thread_create(&rstptask_thread_g, rstp_function, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}
	return 0;
}

int
rstp_stop(void)
{
	int ret;

	bridge_shutdown(); 

	if (cpuport_extract_filter_unregister(CPUPORT_EXTRACT_BPDU))
		return -1;

	rstptask_loop_g = 0;	//start loop

	struct rstp_cmd_msg_t *rstp_cmd_msg;
	rstp_cmd_msg = malloc_safe(sizeof (struct rstp_cmd_msg_t));
	rstp_cmd_msg->cmd = RSTP_CMD_TERMINATE;
	INIT_LIST_NODE(&rstp_cmd_msg->q_entry);
	if (fwk_msgq_send(rstp_cmd_qid_g, &rstp_cmd_msg->q_entry) < 0)
	{
		free_safe(rstp_cmd_msg);
	}
	if ((ret = fwk_thread_join(&rstptask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}
	rstp_omci_notify_chain_finish();
	return 0;
}

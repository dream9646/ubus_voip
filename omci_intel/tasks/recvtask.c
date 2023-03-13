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
 * File    : recvtask.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "recvtask.h"
#include "coretask.h"
#include "omcidump.h"
#include "util.h"
#include "util_socket.h"
#include "crc.h"
#include "list.h"
#include "env.h"
#include "cli.h"
#include "omcimain.h"
#include "proprietary_alu_sw_download.h"
#include "proprietary_calix_sw_download.h"

#ifndef X86
#include "meinfo_hw_util.h"
#endif

#ifndef INTEL_OMCC_LAYER
int recvtask_omcc_socket_fd_g;
static int recvtask_dummy_fd_g[2];
static int recvtask_loop_g = 0;

static struct sockaddr_ll recvtask_omcc_raw_sa_g;
static struct sockaddr_in recvtask_omcc_udp_sa_g;
static struct fwk_thread_t recvtask_thread_g;


static void *recvtask_func(void *ptr);

static int
recvtask_calculate_remote_info()
{
	unsigned int ponid, onuid;
	unsigned short onuudpport;

	ponid = omci_env_g.sim_pon_id % 4;	// pon=0..3
	onuid = omci_env_g.sim_onu_id % 32;	// onu=0..31
	onuudpport = omci_env_g.sim_olt_udp_port + 1 + ponid * 32 + onuid;	// implicit rule implied by OCS olt.out

	fprintf(stderr,
		"Arguments: OLTip=%s, OLTudpport=%d, PON%d, ONU%d, ONUudpport=%d\n",
		omci_env_g.sim_olt_ip, omci_env_g.sim_olt_udp_port, ponid, onuid, onuudpport);

	return onuudpport;
}
#endif

#ifdef INTEL_OMCC_LAYER
#include "intel_px126_adapter.h"
static void intel_omcc_recvtask_func(void *bufmsg,unsigned short len,unsigned int crc);

int
recvtask_init_intel_omcc(void)
{
	intel_adapter_register_omcc_rcv_cb(intel_omcc_recvtask_func);

	return 0;
}
int
recvtask_shutdown_intel_omcc(void)
{
	return 0; 
}

#else
int
recvtask_init(void)
{
	/*create omcc socket */
	if (omci_env_g.transport_type == 0) 	//for raw socket
	{
		if ((recvtask_omcc_socket_fd_g=util_create_raw_socket(omci_env_g.omcc_if, &recvtask_omcc_raw_sa_g))<0)
			return -1;
	} else {				//for udp socket
		//char ipstr[20];
		//unsigned int local_ip;
		unsigned short onuudpport;
		
		//get_host_address(omci_env_g.sim_onu_if, ipstr);
		//local_ip=ntohl(inet_addr(ipstr));
		onuudpport=recvtask_calculate_remote_info();
		if ((recvtask_omcc_socket_fd_g=util_create_udp_socket(INADDR_ANY, onuudpport, &recvtask_omcc_udp_sa_g))<0)
			return -1;
	}

	/*create dummy socket for control */
	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, recvtask_dummy_fd_g) < 0) {
		dbprintf(LOG_ERR, "dummy socket error(%d)\n", recvtask_dummy_fd_g[0]);
		close(recvtask_omcc_socket_fd_g);
		return (-1);
	}

	return 0;
}

int
recvtask_start(void)
{
	int ret;

	/*create recv task */
	recvtask_thread_g.policy = FWK_THREAD_POLICY_OMCI_CORE;
	recvtask_thread_g.priority = FWK_THREAD_PRIO_OMCI_CORE;
	recvtask_thread_g.stack_size = 128*1024;
	recvtask_thread_g.join = 1;
	sprintf(recvtask_thread_g.name, "omci-RECV");

	recvtask_loop_g = 1;	//start loop

	if ((ret = fwk_thread_create(&recvtask_thread_g, recvtask_func, NULL)) < 0) {
		dbprintf(LOG_ERR, "thread create error(%d)\n", ret);
		return (-1);
	}

	return 0;
}

int
recvtask_shutdown(void)
{
	int ret;

	if ((ret = close(recvtask_omcc_socket_fd_g)) < 0) {
		dbprintf(LOG_ERR, "omcc close error(%d)\n", ret);
	}

	if ((ret = close(recvtask_dummy_fd_g[0])) < 0 || (ret = close(recvtask_dummy_fd_g[1])) < 0) {
		dbprintf(LOG_ERR, "dummy close error(%d)\n", ret);
	}

	recvtask_omcc_socket_fd_g = 0;
	recvtask_dummy_fd_g[0] = 0;
	recvtask_dummy_fd_g[1] = 0;

	return 0;
}

int
recvtask_stop(void)
{
	int dummy;
	int ret;

	recvtask_loop_g = 0;	//stop loop

	if ((ret =
	     write(recvtask_dummy_fd_g[1], &dummy, sizeof (dummy))) < 0) {
		dbprintf(LOG_ERR, "dummy write error(%d)\n", ret);
	}

	if ((ret = fwk_thread_join(&recvtask_thread_g)) < 0) {
		dbprintf(LOG_ERR, "join error(%d)\n", ret);
	}

	return 0;
}
#endif

static int
recvtask_handle_omcimsg(struct omcimsg_layout_t *req_msg)
{
	unsigned int msgtype=omcimsg_header_get_type(req_msg);
	struct coretask_msg_t *coretask_msg;
	int ret;
	
	if (msgtype == OMCIMSG_DOWNLOAD_SECTION && !omcimsg_header_get_flag_ar(req_msg)) {
		// handle sw download in recvtask directly to speedup sw download
		struct omcimsg_layout_t resp_msg;	// not used
		
		//add to omcclog, history, similar to coretask_process_omci_msg()
		if (omci_env_g.omccrx_log_enable) {
			if (omci_env_g.omccrx_log_msgmask & (1<<msgtype))
				omcidump_print_raw_to_file(req_msg, omci_env_g.omccrx_log_file, OMCIDUMP_DIR_RX);
		}
		cli_history_add_msg(req_msg);

		omcimsg_download_section_handler(req_msg, &resp_msg);

	} else {
		// not handle omcimsg in recvtask, pass the msg to coretask with msgq ipc

		//get timestamp
		util_get_uptime(&req_msg->timestamp);

		/*alloc omci msg memory, omci data */
		coretask_msg = malloc_safe(sizeof (struct coretask_msg_t));
		INIT_LIST_NODE(&coretask_msg->q_entry);

		coretask_msg->omci_data = malloc_safe(sizeof(struct omcimsg_layout_t));
		memcpy(coretask_msg->omci_data, req_msg, sizeof(struct omcimsg_layout_t));

		coretask_msg->msglen = sizeof(struct omcimsg_layout_t);
		coretask_msg->cmd = CORETASK_CMD_PROCESS_MSG;
		coretask_msg->from = 0; //from socket

		if (omcimsg_util_get_msg_priority(req_msg)) {
			ret = fwk_msgq_send(coretask_hi_qid_g, &coretask_msg->q_entry);
		} else {
			ret = fwk_msgq_send(coretask_lo_qid_g, &coretask_msg->q_entry);
		}
		if (ret < 0)
		{
			dbprintf(LOG_ERR, "fwd omcimsg 0x%x to coretask error\n", omcimsg_header_get_transaction_id(req_msg), ret);
			free_safe(coretask_msg->omci_data);
			free_safe(coretask_msg);
			return -1;
		}
	}
	return 0;
}
#ifdef INTEL_OMCC_LAYER
static void intel_omcc_recvtask_func(void *bufmsg,unsigned short len,unsigned int crc_mic)
{
	char recvbuf[RECVTASK_BUFFER_SIZE];

	struct omcimsg_layout_t *msg;
	unsigned int msg_crc, crc_accum, crc_le, crc_be,crc;
	char crc_type;

	if(len >= (RECVTASK_BUFFER_SIZE-OMCIMSG_CRC_LEN))
	{
		dbprintf(LOG_ERR, "OMCI len error(%d)\n", len);
		return;
	}

	
	crc_type = omci_env_g.omcc_rx_crc_type;
	
	
	memcpy(recvbuf,bufmsg,len);
	memcpy((recvbuf+len),&crc_mic,OMCIMSG_CRC_LEN);

	msg = (struct omcimsg_layout_t *) recvbuf;

	// drop pkt if omci init is not done yet
	if (!omci_is_ready()) {
		dbprintf(LOG_ERR, "OMCI is initializing, msg tid=0x%x dropped\n", omcimsg_header_get_transaction_id(msg));
		return;
	}
	
	if((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ALU) &&
	   (omcimsg_header_get_type(msg) == OMCIMSG_DOWNLOAD_SECTION)) {
		proprietary_alu_sw_download(recvbuf);
		return;
	} else if((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX) &&
	   (omcimsg_header_get_type(msg) == OMCIMSG_DOWNLOAD_SECTION) &&
	   (omci_get_omcc_version() == 0x96 || omci_get_omcc_version() >= 0xB0)) {
		proprietary_calix_sw_download(recvbuf);
		return;
	}

	// check omcimsg crc			
	msg_crc = omcimsg_trailer_get_crc(msg); //only for crc enabled and udp socket

	/*process recv data, check crc first */
	switch(crc_type)
	{
		case ENV_CRC_TYPE_AUTO:
			crc_accum = 0xFFFFFFFF;					
			crc_be = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
			crc_accum = 0xFFFFFFFF;					
			crc_le = ~(crc_le_update(crc_accum, (void *) msg, OMCIMSG_BEFORE_CRC_LEN));
			crc_le = htonl(crc_le);

			if (crc_be == msg_crc)
			{
				crc = crc_be;
				crc_type = ENV_CRC_TYPE_BE; //be
			} else {
				crc = crc_le;
				crc_type = ENV_CRC_TYPE_LE; //le
			}
			break;
		case ENV_CRC_TYPE_BE:
			crc_accum = 0xFFFFFFFF;					
			crc = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
			break;
		case ENV_CRC_TYPE_LE:
			crc_accum = 0xFFFFFFFF;					
			crc = ~(crc_le_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
			crc = htonl(crc);
			break;
		case ENV_CRC_TYPE_DISABLE:
			//do nothing
			break;
		default:
			dbprintf(LOG_ERR, "crc type error(%d)\n", crc_type);
			crc_accum = 0xFFFFFFFF;					
			crc_be = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
			crc_accum = 0xFFFFFFFF;					
			crc_le = ~(crc_le_update(crc_accum, (void *) msg, OMCIMSG_BEFORE_CRC_LEN));
			crc_le = htonl(crc_le);
			if (crc_be == msg_crc)
			{
				crc = crc_be;
				crc_type = ENV_CRC_TYPE_BE; //be
			} else {
				crc = crc_le;
				crc_type = ENV_CRC_TYPE_LE; //le
			}
	}

	if (crc_type != ENV_CRC_TYPE_DISABLE && crc != msg_crc) {
		dbprintf(LOG_ERR, "recv crc error, crc_type=%d, orig=%.8X, crc=%.8X\n", crc_type, msg_crc, crc);
		return;
	}

	// process omcimsg directly or bypass omcimsg to coretask
	recvtask_handle_omcimsg(msg);

}
int
recvtask_util_send_omci_msg(struct omcimsg_layout_t *msg, int ms_diff, unsigned char from)
{
	unsigned char msgtype=omcimsg_header_get_type(msg);
	//int msglen = (omci_env_g.omcc_tx_crc_type == ENV_CRC_TYPE_DISABLE)?OMCIMSG_BEFORE_CRC_LEN:OMCIMSG_LEN;
	unsigned short msglen = OMCIMSG_BEFORE_CRC_LEN;
	unsigned int crc_accum, crc;
	
	int console_fd = util_dbprintf_get_console_fd();
	int ontstate = 0;

#ifdef X86
	ontstate = 5;
#else
	ontstate=meinfo_hw_util_get_ontstate();		// for debug
#endif

	if (msg == NULL ) 
		return (RECVTASK_SEND_ERROR_SEND_DATA);

	if (omci_env_g.omcidump_msgmask & (1<<msgtype)) {
		switch (omci_env_g.omcidump_enable) {
		case 1:	omcidump_print_msg(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_exec_diff(console_fd, ms_diff);
			break;
		case 2:	omcidump_print_raw(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_exec_diff(console_fd, ms_diff);
			break;
		case 3:	omcidump_print_msg(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_raw(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_exec_diff(console_fd, ms_diff);
			break;
		case 4:	omcidump_print_line(console_fd, msg, OMCIDUMP_DIR_TX, ms_diff);
			break;
		case 8:	omcidump_print_char(console_fd, msg, OMCIDUMP_DIR_TX);
			break;
		}
	}

	//show a emergence msg that execution time is over the limitation
	if (ms_diff >= omci_env_g.omci_exec_time_limitation)
	{
		dbprintf(LOG_ERR, "omci exec time %ums > %ums!\n", ms_diff, omci_env_g.omci_exec_time_limitation);
	}

	//fill trailer
	msg->cpcsuu_cpi = htons(OMCIMSG_CPCSUU_CPI);
	msg->cpcssdu_len = htons(OMCIMSG_CPCSSDU_LEN);
	//crc
	if (omci_env_g.omcc_tx_crc_type == ENV_CRC_TYPE_BE) {
		crc_accum = 0xFFFFFFFF;
		crc = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
	} else if (omci_env_g.omcc_tx_crc_type == ENV_CRC_TYPE_LE) {
		crc_accum = 0xFFFFFFFF;
		crc = ~(crc_le_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
		crc = htonl(crc);
	} else {
		crc = 0;
	}
	omcimsg_trailer_set_crc(msg, crc);

	intel_adapter_omcc_send((char*)msg,msglen,crc);

	return 0;
}

#else
static void *
recvtask_func(void *ptr)
{
	char recvbuf[RECVTASK_BUFFER_SIZE];
	struct omcimsg_layout_t *msg= (struct omcimsg_layout_t *) recvbuf;

	unsigned int msg_crc, crc_accum, crc_le, crc_be, crc = 0;
	char crc_type;

	socklen_t socklen;
	fd_set recv_fd_set;
	int maxfd = -1;
	int ret;

	util_set_thread_name("omci-recv");
	omci_env_g.taskname[ENV_TASK_NO_RECV] = "RECV";
	omci_env_g.taskid[ENV_TASK_NO_RECV] = fwk_thread_gettid();
	omci_env_g.pthreadid[ENV_TASK_NO_RECV] = pthread_self();
	omci_env_g.taskloopcount[ENV_TASK_NO_RECV] = 0;
	omci_env_g.taskts[ENV_TASK_NO_RECV] = 0;
	omci_env_g.taskstate[ENV_TASK_NO_RECV] = 0; //idle

	if (recvtask_omcc_socket_fd_g > maxfd) {
		maxfd = recvtask_omcc_socket_fd_g;
	}
	if (recvtask_dummy_fd_g[0] > maxfd) {
		maxfd = recvtask_dummy_fd_g[0];
	}

	crc_type = omci_env_g.omcc_rx_crc_type;

	while (recvtask_loop_g) {
		omci_env_g.taskloopcount[ENV_TASK_NO_RECV]++;
		omci_env_g.taskts[ENV_TASK_NO_RECV] = util_get_uptime_sec();
		omci_env_g.taskstate[ENV_TASK_NO_RECV] = 0; //idle
		
		if (omci_env_g.task_pause_mask & (1<<ENV_TASK_NO_RECV)) {
			msleep(100);
			continue;
		}

		FD_ZERO(&recv_fd_set);

		FD_SET(recvtask_omcc_socket_fd_g, &recv_fd_set);
		FD_SET(recvtask_dummy_fd_g[0], &recv_fd_set);

		if ((ret = select(maxfd + 1, &recv_fd_set, NULL, NULL, NULL)) < 0) {
			dbprintf(LOG_ERR, "select error(%d)\n", ret);
			continue;
		}

		omci_env_g.taskstate[ENV_TASK_NO_RECV] = 1; //running

		// for omci over raw or udp socket
		if (FD_ISSET(recvtask_omcc_socket_fd_g, &recv_fd_set)) {
			if (omci_env_g.transport_type == 0) //for raw socket
			{
				struct sockaddr_ll recvaddr_raw;
				
				memset(&recvaddr_raw, 0, sizeof (struct sockaddr_ll));
				socklen = sizeof (recvaddr_raw);
				if ((ret =
				     recvfrom(recvtask_omcc_socket_fd_g, recvbuf,
					      RECVTASK_BUFFER_SIZE, 0, (struct sockaddr *) &recvaddr_raw, &socklen)) <= 0) {
					dbprintf(LOG_ERR, "recv error(%d)\n", ret);
					continue;
				}
			} else { //for udp socket
				struct sockaddr_in recvaddr_udp;

				memset(&recvaddr_udp, 0, sizeof (struct sockaddr_in));	
				socklen = sizeof (recvaddr_udp);
				if ((ret =
				     recvfrom(recvtask_omcc_socket_fd_g, recvbuf,
					      RECVTASK_BUFFER_SIZE, 0, (struct sockaddr *) &recvaddr_udp, &socklen)) <= 0) {
					dbprintf(LOG_ERR, "recv error(%d)\n", ret);
					continue;
				}
			}

			// drop pkt if omci init is not done yet
			if (!omci_is_ready()) {
				dbprintf(LOG_ERR, "OMCI is initializing, msg tid=0x%x dropped\n", omcimsg_header_get_transaction_id(msg));
				continue;
			}

			if((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_ALU) &&
			   (omcimsg_header_get_type(msg) == OMCIMSG_DOWNLOAD_SECTION)) {
				proprietary_alu_sw_download(recvbuf);
				continue;
			} else if((omci_get_olt_vendor() == ENV_OLT_WORKAROUND_CALIX) &&
			   (omcimsg_header_get_type(msg) == OMCIMSG_DOWNLOAD_SECTION) &&
			   (omci_get_omcc_version() == 0x96 || omci_get_omcc_version() >= 0xB0)) {
				proprietary_calix_sw_download(recvbuf);
				continue;
			}

			// check omcimsg crc			
			msg_crc = omcimsg_trailer_get_crc(msg); //only for crc enabled and udp socket

			/*process recv data, check crc first */
			switch(crc_type)
			{
				case ENV_CRC_TYPE_AUTO:
					crc_accum = 0xFFFFFFFF;					
					crc_be = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
					crc_accum = 0xFFFFFFFF;					
					crc_le = ~(crc_le_update(crc_accum, (void *) msg, OMCIMSG_BEFORE_CRC_LEN));
					crc_le = htonl(crc_le);

					if (crc_be == msg_crc)
					{
						crc = crc_be;
						crc_type = ENV_CRC_TYPE_BE; //be
					} else {
						crc = crc_le;
						crc_type = ENV_CRC_TYPE_LE; //le
					}
					break;
				case ENV_CRC_TYPE_BE:
					crc_accum = 0xFFFFFFFF;					
					crc = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
					break;
				case ENV_CRC_TYPE_LE:
					crc_accum = 0xFFFFFFFF;					
					crc = ~(crc_le_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
					crc = htonl(crc);
					break;
				case ENV_CRC_TYPE_DISABLE:
					//do nothing
					break;
				default:
					dbprintf(LOG_ERR, "crc type error(%d)\n", crc_type);
					crc_accum = 0xFFFFFFFF;					
					crc_be = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
					crc_accum = 0xFFFFFFFF;					
					crc_le = ~(crc_le_update(crc_accum, (void *) msg, OMCIMSG_BEFORE_CRC_LEN));
					crc_le = htonl(crc_le);
					if (crc_be == msg_crc)
					{
						crc = crc_be;
						crc_type = ENV_CRC_TYPE_BE; //be
					} else {
						crc = crc_le;
						crc_type = ENV_CRC_TYPE_LE; //le
					}
			}

			if (crc_type != ENV_CRC_TYPE_DISABLE && crc != msg_crc) {
				dbprintf(LOG_ERR, "recv crc error, crc_type=%d, orig=%.8X, crc=%.8X\n", crc_type, msg_crc, crc);
				continue;
			}

			// process omcimsg directly or bypass omcimsg to coretask
			recvtask_handle_omcimsg(msg);
		}

		// for dummy over unix domain socket
		if (FD_ISSET(recvtask_dummy_fd_g[0], &recv_fd_set)) {
			struct sockaddr_in recvaddr_udp;
			
			memset(&recvaddr_udp, 0, sizeof (struct sockaddr_in));	
			socklen = sizeof (recvaddr_udp);
			if ((ret =
			     read(recvtask_dummy_fd_g[0], recvbuf, RECVTASK_BUFFER_SIZE)) < 0) {
				dbprintf(LOG_ERR, "dummy recv error(%d)\n", ret);
				continue;
			}

			dbprintf(LOG_ERR, "task terminate\n");

			continue;
		}
	}

	omci_env_g.taskstate[ENV_TASK_NO_RECV] = 0; //idle
	omci_env_g.taskid[ENV_TASK_NO_RECV] = 0;

	return 0;
}

// util function used by tranmod.c ///////////////////////////////////////////////////////////////////////////////////////////////

int
recvtask_util_send_omci_msg(struct omcimsg_layout_t *msg, int ms_diff, unsigned char from)
{
	unsigned char msgtype=omcimsg_header_get_type(msg);
	int msglen = (omci_env_g.omcc_tx_crc_type == ENV_CRC_TYPE_DISABLE)?OMCIMSG_BEFORE_CRC_LEN:OMCIMSG_LEN;
	unsigned int crc_accum, crc;
	struct sockaddr_in remote_addr_l;
	socklen_t socklen;
	int console_fd = util_dbprintf_get_console_fd();
	int ret;
	int ontstate = 0;

#ifdef X86
	ontstate = 5;
#else
	ontstate=meinfo_hw_util_get_ontstate();		// for debug
#endif

	if (msg == NULL || recvtask_omcc_socket_fd_g == 0) 
		return (RECVTASK_SEND_ERROR_SEND_DATA);

	if (omci_env_g.omcidump_msgmask & (1<<msgtype)) {
		switch (omci_env_g.omcidump_enable) {
		case 1:	omcidump_print_msg(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_exec_diff(console_fd, ms_diff);
			break;
		case 2:	omcidump_print_raw(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_exec_diff(console_fd, ms_diff);
			break;
		case 3:	omcidump_print_msg(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_raw(console_fd, msg, OMCIDUMP_DIR_TX);
			omcidump_print_exec_diff(console_fd, ms_diff);
			break;
		case 4:	omcidump_print_line(console_fd, msg, OMCIDUMP_DIR_TX, ms_diff);
			break;
		case 8:	omcidump_print_char(console_fd, msg, OMCIDUMP_DIR_TX);
			break;
		}
	}

	//show a emergence msg that execution time is over the limitation
	if (ms_diff >= omci_env_g.omci_exec_time_limitation)
	{
		dbprintf(LOG_ERR, "omci exec time %ums > %ums!\n", ms_diff, omci_env_g.omci_exec_time_limitation);
	}

	//fill trailer
	msg->cpcsuu_cpi = htons(OMCIMSG_CPCSUU_CPI);
	msg->cpcssdu_len = htons(OMCIMSG_CPCSSDU_LEN);
	//crc
	if (omci_env_g.omcc_tx_crc_type == ENV_CRC_TYPE_BE) {
		crc_accum = 0xFFFFFFFF;
		crc = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
	} else if (omci_env_g.omcc_tx_crc_type == ENV_CRC_TYPE_LE) {
		crc_accum = 0xFFFFFFFF;
		crc = ~(crc_le_update(crc_accum, (char *) msg, OMCIMSG_BEFORE_CRC_LEN));
		crc = htonl(crc);
	} else {
		crc = 0;
	}
	omcimsg_trailer_set_crc(msg, crc);

	if (omci_env_g.transport_type == 0) //for raw socket
	{
		socklen = sizeof (recvtask_omcc_raw_sa_g);

		switch (omci_env_g.omci_tx_flag)
		{

		case 1: 
			//always send
			if ((ret = sendto(recvtask_omcc_socket_fd_g, (char *) msg, msglen, 0,
			    (struct sockaddr *) &recvtask_omcc_raw_sa_g, socklen)) < 0) {
				dbprintf(LOG_ERR, "sendto error(%d, %s, O%d)\n", errno, strerror(errno), ontstate);
				return (RECVTASK_SEND_ERROR_SEND_ERROR);
			}
			break;
		case 2:
			//always not send
			//do notthing
			break;
		default:
			//auto, depend on "from" parameter
			if (from == 0) //from socket
			{
				if ((ret = sendto(recvtask_omcc_socket_fd_g, (char *) msg, msglen, 0,
				    (struct sockaddr *) &recvtask_omcc_raw_sa_g, socklen)) < 0) {
					dbprintf(LOG_ERR, "sendto error(%d, %s, O%d)\n", errno, strerror(errno), ontstate);
					return (RECVTASK_SEND_ERROR_SEND_ERROR);
				}
			} //else do not send 
		}
	} else { //for udp socket
		remote_addr_l.sin_addr.s_addr = inet_addr(omci_env_g.sim_olt_ip);
		remote_addr_l.sin_family = AF_INET;
		remote_addr_l.sin_port = htons(omci_env_g.sim_olt_udp_port);
		memset(remote_addr_l.sin_zero, 0, sizeof (remote_addr_l.sin_zero));

		socklen = sizeof (remote_addr_l);

		if ((ret = sendto(recvtask_omcc_socket_fd_g, (char *) msg, OMCIMSG_LEN, 0,
			    (struct sockaddr *) &remote_addr_l, socklen)) < 0) {
			dbprintf(LOG_ERR, "sendto error(%d, %s)\n", errno, strerror(errno));
			return (RECVTASK_SEND_ERROR_SEND_ERROR);
		}
	}

	return 0;
}
#endif


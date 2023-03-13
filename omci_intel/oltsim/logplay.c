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
 * Module  : oltsim
 * File    : logplay.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <pthread.h>
#include <sched.h>

#include <assert.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <linux/sockios.h>

#include "util.h"
#include "omcistr.h"
#include "omcilog.h"
#include "logplay.h"

static int logplay_send_next(struct logplay_state_t *state);
static struct logplay_state_t logplay_state;
static pthread_t logplay_thread;

static int
logplay_send_next(struct logplay_state_t *state)
{	
	if (!state->is_running)
		return -1;

	while (omcilog_ptr[state->run_next]) { 
		unsigned char msgtype=omcimsg_header_get_type(&omcilog_ptr[state->run_next]->msg);
				
		if (msgtype==OMCIMSG_MIB_RESET && state->skip_mibreset)
			state->run_next++;
		else if ((msgtype==OMCIMSG_MIB_UPLOAD||msgtype==OMCIMSG_MIB_UPLOAD_NEXT) && state->skip_mibupload)
			state->run_next++;
		else
			break;
	}

	if (state->run_next<= state->run_end && state->run_next< omcilog_total) {
		int i=state->run_next;
		state->run_next++;
		if (omcilog_ptr[i]) {
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			
			struct sockaddr_in remote_addr;
			unsigned short onuport=logplay_calc_onuport(state->oltport, state->ponid, state->onuid);
			int sent;

			memset(&remote_addr, 0, sizeof(remote_addr));
			util_ipstr_to_sockaddr(&remote_addr, state->onuipstr, onuport);

			{	// print info of sending pkt
				unsigned char msgtype=omcimsg_header_get_type(msg);
				unsigned short tid=omcimsg_header_get_transaction_id(msg);
				unsigned short classid=omcimsg_header_get_entity_class_id(msg);
				unsigned short meid=omcimsg_header_get_entity_instance_id(msg);		
				util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "send pkt%d to %s:%u, len=%d\n", 
					i,
					inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port), 
					sizeof(struct omcimsg_layout_t));
				util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "tid=0x%04x, %s: %d %s, me 0x%x(%d)\n\n",
					tid,
					omcistr_msgtype2name(msgtype),
					classid, omcistr_classid2name(classid),
					meid, meid);
				state->last_sent_tid=tid;
			}

			sent=sendto(state->oltfd, (void*)msg, sizeof(struct omcimsg_layout_t), 0, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
			
			if (sent!= sizeof(struct omcimsg_layout_t)) {
				dbprintf(LOG_ERR, "send error?\n");
				return -1;
			}
			return 0;
		} else {
			dbprintf(LOG_ERR, "omcilog_ptr[%d] is null?\n", i);
		}
			
	} else {
		logplay_send_stop();
		return 0;
	}
	return -1;
}

static void *
logplay_func(void *dataptr)
{
	static char buff[2048];
	struct omcimsg_layout_t *msg=(struct omcimsg_layout_t *)buff;
	struct logplay_state_t *state=&logplay_state;
	int msg_timeout=0;

	state->pid=getpid();
	dbprintf(LOG_DEBUG, "enter thread pid=%u\n", state->pid);

	// init oltfd for server port
	state->oltfd=util_udp_server(state->oltport);
	if (state->oltfd<0) {
		dbprintf(LOG_ERR, "error bind olt port %u\n", state->oltport);
		return 0;
	}

	while (state->loop) {
		fd_set readMask;
		struct timeval t;
		int n, recv;

		t.tv_sec=state->timeout/2;
		t.tv_usec=0;
		FD_ZERO (&readMask);
		FD_SET (state->oltfd, &readMask);
		n=select(state->oltfd+1, &readMask, NULL, NULL, &t);
		if (n>0) {
			struct sockaddr_in cliaddr;
			unsigned cliaddr_len=sizeof(cliaddr);
	
			memset(&cliaddr, 0, sizeof(cliaddr));		
			recv = recvfrom(state->oltfd, (void*)buff, 2048, 0, 
					(struct sockaddr*)&cliaddr, &cliaddr_len);

			if (recv==48) {
				unsigned char msgtype=omcimsg_header_get_type(msg);
				unsigned short tid=omcimsg_header_get_transaction_id(msg);
				unsigned short classid=omcimsg_header_get_entity_class_id(msg);
				unsigned short meid=omcimsg_header_get_entity_instance_id(msg);	
				unsigned char result_reason=omcilog_save_result_reason(msg, state->run_next);
	
				util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "recv pkt from %s:%u, len=%d\n", 
					inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), recv);
				util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "tid=0x%04x, %s: %d %s, me 0x%x(%d) %s\n\n",
					tid,
					omcistr_msgtype2name(msgtype),
					classid, omcistr_classid2name(classid),
					meid, meid, (result_reason==0)?"":"ERR!");

				if (state->is_running) {
					if (state->last_sent_tid!=LOGPLAY_TID_NONE && tid!=state->last_sent_tid)
						dbprintf(LOG_ERR, "response doesn't match last sent tid 0x%x\n", state->last_sent_tid);
					logplay_send_next(state);
					msg_timeout=time(0)+state->timeout;
					continue;
				}
			} else if (recv==0) {
				dbprintf(LOG_ERR, "remote peer shutdown?\n");
			} else if (recv>0) {
				dbprintf(LOG_ERR, "recv pkt from %s:%u, len=%d, not OMCI pkt!\n", 
					inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), recv);
			} else {
				dbprintf(LOG_ERR, "recvfrom err: %s\n", strerror(errno));
			}
		} else if (n==0) {	// select timeout for 1 sec
			dbprintf(LOG_DEBUG, "select timeout\n"); 
		} else {
			if (errno!=EINTR) {
				dbprintf(LOG_ERR, "select err: %s\n", strerror(errno));
			}
		}
		
		// house keeping, check if next pkt has to be sent
		if (state->is_running) {
			if (state->last_sent_tid==LOGPLAY_TID_NONE) {	// this is 1st msg
				logplay_send_next(state);
				msg_timeout=time(0)+state->timeout;
			} else {
				if (time(0) > msg_timeout) {	// for other msg, we send them after reponse msg timeout
					dbprintf(LOG_ERR, "recv pkt timeout\n");
					logplay_send_next(state);
					msg_timeout=time(0)+state->timeout;
				}
			}
		}
	}

	close(state->oltfd);
	
	dbprintf(LOG_DEBUG, "leave thread pid=%u\n", state->pid);
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////

void
logplay_show_state(void)
{
	struct logplay_state_t *state=&logplay_state;
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "timeout = %d\n", state->timeout);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "oltport = %u\n", state->oltport);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "ponid = %d\n", state->ponid);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "onuid = %d\n", state->onuid);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "onuip = %s\n", state->onuipstr);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "onuport = %u\n", logplay_calc_onuport(state->oltport, state->ponid, state->onuid));
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "is_running = %d\n", state->is_running);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "run_start = %d\n", state->run_start);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "run_end = %d\n", state->run_end);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "run_next = %d\n", state->run_next);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "skip_mibreset = %d\n", state->skip_mibreset);
	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "skip_mibupload = %d\n", state->skip_mibupload);
}

void
logplay_set_onuinfo(char *onuipstr, unsigned char ponid, unsigned char onuid)
{
	struct logplay_state_t *state=&logplay_state;
	
	strncpy(state->onuipstr, onuipstr, 64);
	state->ponid=ponid;
	state->onuid=onuid;

	util_dbprintf(omci_env_g.debug_level, LOG_EMERG, "OLTudpport=%u, PON%d, ONU%d, ONUip=%s, ONUudpport=%u\n",
		state->oltport, state->ponid, state->onuid, state->onuipstr, 
		logplay_calc_onuport(state->oltport, state->ponid, state->onuid));
}
	
void
logplay_set_timeout(int timeout)
{
	struct logplay_state_t *state=&logplay_state;
	if (timeout>=1)
		state->timeout=timeout;
}

void
logplay_set_skip_mibreset(int skip)
{
	struct logplay_state_t *state=&logplay_state;
	state->skip_mibreset=skip;;
}

int
logplay_get_i(void)
{
	struct logplay_state_t *state=&logplay_state;

	return state->run_next;
}

int
logplay_get_i_before_next_write(unsigned short current)
{
	int i;
	
	for (i=current+1; i< omcilog_total; i++) {
		if (omcilog_ptr[i]) {		
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			unsigned char msgtype=omcimsg_header_get_type(msg);
			if (omcistr_msgtype_is_write(msgtype))
				return i-1;
		}
	}
	return OMCILOG_TOTAL_MAX-1;
}

int
logplay_get_i_before_next_mibreset(unsigned short current)
{
	int i;
	
	for (i=current+1; i< omcilog_total; i++) {
		if (omcilog_ptr[i]) {		
			struct omcimsg_layout_t *msg=&omcilog_ptr[i]->msg;
			unsigned char msgtype=omcimsg_header_get_type(msg);
			if (msgtype==OMCIMSG_MIB_RESET)
				return i-1;
		}
	}
	return OMCILOG_TOTAL_MAX-1;
}

void
logplay_set_i(unsigned short i)
{
	struct logplay_state_t *state=&logplay_state;
	state->run_next=i;
}

void
logplay_send_start(unsigned short start, unsigned short end)
{
	struct logplay_state_t *state=&logplay_state;
	if (!state->is_running) {
		state->run_start=start;
		state->run_end=end;
		state->run_next=start;
		state->is_running=1;
		state->last_sent_tid=LOGPLAY_TID_NONE;
		logplay_send_next(state);
		dbprintf(LOG_INFO, "started from %d\n", state->run_next); 
	}
}
	
void
logplay_send_stop(void)
{
	struct logplay_state_t *state=&logplay_state;
	if (state->is_running) {
		state->is_running=0;
		dbprintf(LOG_EMERG, "stopped at %d\n", state->run_next); 
	}
}

void
logplay_init(unsigned short oltport)
{
	struct logplay_state_t *state=&logplay_state;
	
	dbprintf(LOG_DEBUG, "init\n"); 
	
	memset(state, 0, sizeof(struct logplay_state_t));

	state->loop=0;
	state->timeout=LOGPLAY_TIMEOUT;
	state->oltport=oltport;
	state->oltfd=-1;
	
	logplay_set_onuinfo(LOGPLAY_ONUIP, LOGPLAY_PONID, LOGPLAY_ONUID);
	
	state->is_running=0;
	state->last_sent_tid=LOGPLAY_TID_NONE;
	state->run_start=0;
	state->run_end=OMCILOG_TOTAL_MAX-1;
	state->run_next=state->run_start;
	state->skip_mibreset=0;
	state->skip_mibupload=1;
}

void
logplay_start(void)
{
	struct logplay_state_t *state=&logplay_state;
	dbprintf(LOG_DEBUG, "start\n"); 
	
	state->loop=1;
	util_thread_create(&logplay_thread, SCHED_OTHER, 110, logplay_func, NULL);
}

void 
logplay_stop(void)
{
	struct logplay_state_t *state=&logplay_state;
	dbprintf(LOG_DEBUG, "stop\n"); 
	
	logplay_send_stop();
	state->loop=0;	
}

void
logplay_shutdown(void)
{
//	struct logplay_state_t *state=&logplay_state;
	dbprintf(LOG_DEBUG, "shutdown\n"); 	
	pthread_join(logplay_thread, NULL);
}

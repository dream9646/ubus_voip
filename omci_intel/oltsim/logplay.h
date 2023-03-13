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
 * File    : logplay.h
 *
 ******************************************************************/

#ifndef __LOGPLAY_H__
#define __LOGPLAY_H__

#define LOGPLAY_TIMEOUT		5
#define LOGPLAY_ONUIP		"127.0.0.1"
#define LOGPLAY_PONID		1
#define LOGPLAY_ONUID		1
#define LOGPLAY_TID_NONE	-1

struct logplay_state_t {	
	int pid;

	// for olt, local
	int loop;
	int timeout;
	unsigned short oltport;
	int oltfd;
	
	// for onu, remote
	unsigned char ponid;
	unsigned char onuid;
	char onuipstr[64];

	// for running state
	int is_running;
	int last_sent_tid;
	unsigned short run_start;
	unsigned short run_end;
	unsigned short run_next;
	int skip_mibreset;
	int skip_mibupload;
};


// rule implied by 5vt omci
static inline unsigned short 
logplay_calc_onuport(unsigned short oltport, unsigned char ponid, unsigned char onuid)
{
	return oltport + 1 + ponid * 32 + onuid;
}

/* logplay.c */
void logplay_show_state(void);
void logplay_set_onuinfo(char *onuipstr, unsigned char ponid, unsigned char onuid);
void logplay_set_timeout(int timeout);
void logplay_set_skip_mibreset(int skip);
int logplay_get_i(void);
int logplay_get_i_before_next_write(unsigned short current);
int logplay_get_i_before_next_mibreset(unsigned short current);
void logplay_set_i(unsigned short i);
void logplay_send_start(unsigned short start, unsigned short end);
void logplay_send_stop(void);
void logplay_init(unsigned short oltport);
void logplay_start(void);
void logplay_stop(void);
void logplay_shutdown(void);

#endif

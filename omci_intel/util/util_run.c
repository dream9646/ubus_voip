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
 * Module  : util
 * File    : util_run.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "util.h"
#include "conv.h"  
#include "regexp.h"
#include "fwk_thread.h"

static void * 
cmd_thread_function(void *data)
{
	util_set_thread_name("cmd_thread");
	nice(19);	
	system((char*)data);
	return 0;
}

static int
cmd_thread_warpper(char *cmd, int is_fg, enum fwk_thread_sp_t policy, unsigned priority)
{
	static char cmd_buff[4][256];
	static int cmd_buff_index = 0;
	char *cmd2;
	
	if (is_fg) {
		cmd2 = cmd;
	} else {
		// copy cmd to static buffer for bg thread
		// since if the cmd is a local var in caller stack frame, the cmd wont be available when the background thread refers to it
		cmd2 = cmd_buff[cmd_buff_index++%4];
		strncpy(cmd2, cmd, 255); cmd2[255] = 0;
	}
	return fwk_thread_run_function(cmd_thread_function, cmd2, 
				is_fg?FWK_THREAD_RUN_FUNC_SYNC:FWK_THREAD_RUN_FUNC_ASYNC, 
				policy, priority);
}


// util_run_by_thread() & util_run_by_system() should be used instead of system() 
// thus we can see which program is executed from omci and how much time it takes

int
util_run_by_thread(char *cmd, int is_fg)
{
	enum fwk_thread_sp_t policy;
	int priority, ret;
	
	fwk_thread_get_priority(fwk_thread_gettid(), &policy, &priority);
	if (policy == FWK_THREAD_SP_NORMAL) {
		priority = 0;	// normal 
	} else {
		priority = 1;	// lowest real time
	}
	
	if (omci_env_g.debug_level >= LOG_WARNING) {
		static char *policy_str[] = {"normal", "fifo", "rr" };
		struct timeval tv_b, tv_e;
		unsigned int msec;
		
		util_get_uptime(&tv_b);
		ret=cmd_thread_warpper(cmd, is_fg, policy, priority);	// fork a low prio thread of same policy to run cmd
		util_get_uptime(&tv_e);
		
		msec = ((tv_e.tv_sec - tv_b.tv_sec) * 1000) + (tv_e.tv_usec - tv_b.tv_usec)/1000;		
		dbprintf(LOG_WARNING, "cmd=%s, ret=%d (%s thread, policy=%s, prio=%d, %dms)\n", cmd, ret, is_fg?"fg":"bg", policy_str, priority, msec);
	} else {
		ret=cmd_thread_warpper(cmd, is_fg, policy, priority);	// fork a low prio thread of same policy to run cmd
	}
	return ret;
}

int 
util_run_by_system(char *cmd)
{
	int ret, i;
	sighandler_t old_handler;

	if (omci_env_g.debug_level >= LOG_WARNING) {
		struct timeval tv_b, tv_e;
		unsigned int msec;

		util_get_uptime(&tv_b);
		/*
		-1: fork err
		WEXITSTATUS(status)==127: cmd not found
		WEXITSTATUS(status)==126: cmd access mode is incorect
		*/
		for(i=0; i < 3; i++) {
			/* Temporarily change the SIGCHLD handler to SIG_DFL for waitxx() in system(). */
			old_handler = signal(SIGCHLD, SIG_DFL);
			ret = system(cmd);
			signal(SIGCHLD, old_handler);

			if( ret != -1)
				break;
		}
		util_get_uptime(&tv_e);
		msec = ((tv_e.tv_sec - tv_b.tv_sec) * 1000) + (tv_e.tv_usec - tv_b.tv_usec)/1000;
		
		dbprintf(LOG_NOTICE, "cmd=%s, ret=%d (system, %dms)\n", cmd, ret, msec);
	} else {
		for(i=0; i < 3; i++) {
			/* Temporarily change the SIGCHLD handler to SIG_DFL for waitxx() in system(). */
			old_handler = signal(SIGCHLD, SIG_DFL);
			ret = system(cmd);
			signal(SIGCHLD, old_handler);

			if( ret != -1)
				break;
		}
	}
	return ret;
}

int
util_thread_sched(int policy, int priority)
{
	struct sched_param param;

	
	/*
	 * Set the priority and scheduler
	 */
	param.sched_priority = priority;
	if (pthread_setschedparam(pthread_self(), policy, &param) == -1) {
		dbprintf(LOG_ERR, "error setting scheduler %d!\n", policy);
		return -1;
	}

	return 0;
}

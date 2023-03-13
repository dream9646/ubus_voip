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
 * Module  : cli
 * File    : cli_history.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>

#include "fwk_mutex.h"
#include "fwk_thread.h"
#include "fwk_msgq.h"
#include "coretask.h"
#include "omcimsg.h"
#include "omcidump.h"
#include "env.h"
#include "util.h"
#include "crc.h"
#include "conv.h"
#include "list.h"
#include "cli.h"

#define CLI_SESSION_MAX		16
#define CLI_SESSION_TIMEOUT	600

struct cli_session_list_t {
	unsigned int session_count;
	struct list_head session_list;
	struct fwk_mutex_t mutex;
};

struct cli_session_t {
	unsigned char name[64];
	unsigned int pid;
	long start_time;
	struct list_head list_node;
};

static struct cli_session_list_t cli_session_list_g;

int
cli_session_list_init(void)
{
	memset(&cli_session_list_g, 0x00, sizeof(struct cli_session_list_t));
	INIT_LIST_HEAD(&cli_session_list_g.session_list);
	fwk_mutex_create(&cli_session_list_g.mutex);

	return 0;
}

int
cli_session_list_shutdown(void)
{
	struct cli_session_t *session_ptr, *session_n;

	list_for_each_entry_safe(session_ptr, session_n, &cli_session_list_g.session_list, list_node)
	{
		list_del(&session_ptr->list_node);
		free_safe(session_ptr);
		cli_session_list_g.session_count--;
	}

	memset(&cli_session_list_g, 0x00, sizeof(struct cli_session_list_t));
	fwk_mutex_destroy(&cli_session_list_g.mutex);

	return 0;
}

// if same name session exist, return -1
int
cli_session_add(char *session_name, unsigned int session_pid)
{
	struct cli_session_t *session_ptr, *session_n;
	long now = util_get_uptime_sec();
	struct cli_session_t *session_found = NULL;

	fwk_mutex_lock(&cli_session_list_g.mutex);
	list_for_each_entry_safe(session_ptr, session_n, &cli_session_list_g.session_list, list_node) {
		if (now - session_ptr->start_time > CLI_SESSION_TIMEOUT) {	//release stale ones
			list_del(&session_ptr->list_node);
			free_safe(session_ptr);
			cli_session_list_g.session_count--;
		} else {
			if (strncmp(session_name, session_ptr->name, 63) == 0)
				session_found = session_ptr;
		}			
	}	
	if (session_found) {
		unsigned int mypid= getpid();
		fwk_mutex_unlock(&cli_session_list_g.mutex);
		if (session_found->pid == mypid)
			return 0;
		return -1;
	}		
	if (cli_session_list_g.session_count >= CLI_SESSION_MAX) {
		fwk_mutex_unlock(&cli_session_list_g.mutex);
		return -2;
	}
		
	session_ptr = malloc_safe(sizeof(struct cli_session_t));
	strncpy(session_ptr->name, session_name, 63);
	session_ptr->name[63] = 0;
	session_ptr->pid = session_pid;
	session_ptr->start_time = now;
		
	cli_session_list_g.session_count++;

	list_add_tail(&session_ptr->list_node, &cli_session_list_g.session_list);

	fwk_mutex_unlock(&cli_session_list_g.mutex);
	return 0;
}

// if sess name exist and owned by pid, then do deletion
int
cli_session_del(char *session_name, unsigned int session_pid)
{
	struct cli_session_t *session_ptr, *session_n;
	struct cli_session_t *session_found = NULL;
	int ret = 0;

	fwk_mutex_lock(&cli_session_list_g.mutex);
	list_for_each_entry_safe(session_ptr, session_n, &cli_session_list_g.session_list, list_node) {
		if (strncmp(session_name, session_ptr->name, 63) == 0) {
			session_found = session_ptr;
			break;
		}
		
	}
	if (session_found) {
		if (session_pid == session_ptr->pid || session_pid == 0) {
			list_del(&session_ptr->list_node);
			free_safe(session_ptr);
			cli_session_list_g.session_count--;
			ret = 0;
		} else {
			ret = -2;	// not owner
		}
	} else {
		ret = -1;	// not found
	}
	fwk_mutex_unlock(&cli_session_list_g.mutex);
	
	return ret;
}

int
cli_session_list_print(int fd)
{
	struct cli_session_t *session_ptr, *session_n;
	long now = util_get_uptime_sec();

	fwk_mutex_lock(&cli_session_list_g.mutex);
	list_for_each_entry_safe(session_ptr, session_n, &cli_session_list_g.session_list, list_node) {
		if (now - session_ptr->start_time > CLI_SESSION_TIMEOUT) {	//release stale ones
			list_del(&session_ptr->list_node);
			free_safe(session_ptr);
			cli_session_list_g.session_count--;
		} else {
			util_fdprintf(fd, "%s - pid=%u, age=%dsec\n", session_ptr->name, session_ptr->pid, now - session_ptr->start_time);
		}
	}
	fwk_mutex_unlock(&cli_session_list_g.mutex);
	
	return 0;
}

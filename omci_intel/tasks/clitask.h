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
 * Purpose : 5VT TRAF MANAGER
 * Module  : tasks
 * File    : clitask.h
 *
 ******************************************************************/

#ifndef __CLITASK_H__
#define __CLITASK_H__

#define CLITASK_BUFFER_SIZE 2048

#define CLITASK_ERROR_BASE 0x3000
#define CLITASK_SEND_ERROR_SEND_ERROR -(CLITASK_ERROR_BASE + 1)
#define CLITASK_SEND_ERROR_SEND_DATA -(CLITASK_ERROR_BASE + 2)

#include "vacl.h"
#include "vacl_util.h"

struct clitask_client_info_t {
	int fd;
	struct vacl_user_node_t acl_rule;
	int is_set_init_l;
};

extern int clitask_consolefd_g;	// fd to /dev/console
extern int clitask_nullfd_g;	// fd to /dev/null

void *clitask_get_thread_private(void);
int clitask_deteach_stdin_stderr_redirect_consolefd(void);
int clitask_init(void);
int clitask_start(void);
int clitask_shutdown(void);
int clitask_stop(void);

#endif

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
 * File    : tranxmod.h
 *
 ******************************************************************/

#ifndef __TRANXMOD_H__
#define __TRANXMOD_H__

#include "fwk_mutex.h"
#include "omcimsg.h"
#include "list.h"

#define TRANXMOD_OK			0
#define TRANXMOD_BYPASS			1
#define TRANXMOD_ERROR_BASE		0x7000
#define TRANXMOD_ERROR_RETRANS_MATCH	-(TRANXMOD_ERROR_BASE+1)
#define TRANXMOD_ERROR_BUSY		-(TRANXMOD_ERROR_BASE+2)

enum tranxmod_ont_state_t { 
	TRANXMOD_ONT_STATE_PROCESSING, 
	TRANXMOD_ONT_STATE_TERMINATED	//Acknowledged
};

struct tranxmod_ont_db_t {
	struct fwk_mutex_t hi_tranx_mutex;
	unsigned int hi_tranx_count;
	struct list_head hi_tranx_list;
	struct fwk_mutex_t lo_tranx_mutex;
	unsigned int lo_tranx_count;
	struct list_head lo_tranx_list;
};

struct tranxmod_ont_node_t {
	enum tranxmod_ont_state_t state;
	unsigned char *request_data;
	unsigned char *response_data;
	struct list_head node;
};

int tranxmod_init(void);
int tranxmod_shutdown(void); int tranxmod_recv_omci_msg(struct omcimsg_layout_t *msg, unsigned char from); int tranxmod_send_omci_msg(struct omcimsg_layout_t *msg, unsigned char from);

#endif

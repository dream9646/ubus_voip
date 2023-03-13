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
 * Module  : omcimsg
 * File    : omcimsg_handler.h
 *
 ******************************************************************/

#ifndef __OMCIMSG_HANDLER_H__
#define __OMCIMSG_HANDLER_H__

#include "omcimsg.h"

struct omcimsg_handler_coretask_t {
	int (*tranx_send_omci_msg_cb) (struct omcimsg_layout_t * msg, unsigned char from);
	int (*send_tca_determination_pmtask_cb) (unsigned short classid, unsigned short meid);
	int timer_qid;
};

extern struct omcimsg_handler_coretask_t omcimsg_handler_coretask_g;

#define omcimsg_handler_coretask_tranx_send_omci_msg(msg, from) \
	(omcimsg_handler_coretask_g.tranx_send_omci_msg_cb(msg, from))

#define omcimsg_handler_coretask_send_tca_determination_pmtask(classid, meid) \
	(omcimsg_handler_coretask_g.send_tca_determination_pmtask_cb(classid, meid))

#define omcimsg_handler_coretask_timer_qid \
	(omcimsg_handler_coretask_g.timer_qid)

int omcimsg_handler_init(struct omcimsg_handler_coretask_t *h_ptr);
int omcimsg_handler_shutdown();

#endif

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
 * File    : omcimsg_handler.c
 *
 ******************************************************************/

#include <string.h>

#include "omcimsg_handler.h"

struct omcimsg_handler_coretask_t omcimsg_handler_coretask_g;

int
omcimsg_handler_init(struct omcimsg_handler_coretask_t *h_ptr)
{
	memset(&omcimsg_handler_coretask_g, 0x00, sizeof (struct omcimsg_handler_coretask_t));

	omcimsg_handler_coretask_g.tranx_send_omci_msg_cb = h_ptr->tranx_send_omci_msg_cb;
	omcimsg_handler_coretask_g.send_tca_determination_pmtask_cb = h_ptr->send_tca_determination_pmtask_cb;
	omcimsg_handler_coretask_g.timer_qid = h_ptr->timer_qid;

	return 0;
}

int
omcimsg_handler_shutdown()
{
	memset(&omcimsg_handler_coretask_g, 0x00, sizeof (struct omcimsg_handler_coretask_t));

	return 0;
}

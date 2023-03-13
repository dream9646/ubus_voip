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
 * Module  : notifychain
 * File    : notify_chain.h
 *
 ******************************************************************/

#ifndef __NOTIFY_CHAIN_H__
#define __NOTIFY_CHAIN_H__

#include <pthread.h>

#define NOTIFY_CHAIN_EVENT_ME_CREATE	0
#define NOTIFY_CHAIN_EVENT_ME_DELETE	1
#define NOTIFY_CHAIN_EVENT_ME_UPDATE	2
#define NOTIFY_CHAIN_EVENT_BOOT		3
#define NOTIFY_CHAIN_EVENT_SHUTDOWN	4
#define NOTIFY_CHAIN_EVENT_O5_SEQUENCE	5
#define NOTIFY_CHAIN_EVENT_MAX		6

int
 notify_chain_register(unsigned char event,
		       int (*cb_func_ptr) (unsigned char event,
					   unsigned short classid,
					   unsigned short instance_num,
					   unsigned short meid, void *data_ptr), void *data_ptr);
int notify_chain_unregister(unsigned char event,
			    int (*cb_func_ptr) (unsigned char event,
						unsigned short classid,
						unsigned short instance_num,
						unsigned short meid, void *data_ptr), void *data_ptr);
int notify_chain_notify(unsigned char event, unsigned short classid, unsigned short instance_num, unsigned short meid);

int notify_chain_preinit(void);

#endif

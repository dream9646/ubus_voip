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
 * Module  : notifychain_cb
 * File    : notify_chain_cb.h
 *
 ******************************************************************/

#ifndef __NOTIFY_CHAIN_CB_H__
#define __NOTIFY_CHAIN_CB_H__

int me_createdel_create_handler(unsigned char event,
				unsigned short classid,
				unsigned short instance_num, unsigned short meid, void *data_ptr);
int me_createdel_delete_handler(unsigned char event,
				unsigned short classid,
				unsigned short instance_num, unsigned short meid, void *data_ptr);
int
 me_createdel_boot_handler(unsigned char event,
			   unsigned short classid, unsigned short instance_num, unsigned short meid, void *data_ptr);
int me_createdel_shutdown_handler(unsigned char event,
				  unsigned short classid,
				  unsigned short instance_num, unsigned short meid, void *data_ptr);

#endif


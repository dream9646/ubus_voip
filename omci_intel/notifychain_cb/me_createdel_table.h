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
 * File    : me_createdel_table.h
 *
 ******************************************************************/

#ifndef __ME_CREATEDEL_TABLE_H__
#define __ME_CREATEDEL_TABLE_H__

#include "list.h"

//pthread_mutex_t me_createdel_table_lock;

int me_createdel_table_load(char *filename);
int me_createdel_table_dump(int fd);
int me_createdel_table_free();
int me_createdel_traverse_table(unsigned char event,
				unsigned short classid,
				unsigned short instance_num, unsigned short meid, void *data_ptr);

struct instance_type_t {
	unsigned char  state;/*0: specific instance num, .=254:instance relativity, *=255: all instance */
	unsigned short start;
	unsigned short end;
};

struct me_createdel_src_t {
	unsigned char event;
	unsigned short classid;
	unsigned short instance_num;
	struct list_head list_frame_ptr_src;
	struct list_head list_frame_ptr_dst;
};

struct me_createdel_dst_t {
	unsigned short classid;
	struct instance_type_t instance_type;
	struct list_head list_frame_ptr_dst;
};

#define	INSTANCE_RELATIVITY	254	
#define	INSTANCE_ALL		255

#endif

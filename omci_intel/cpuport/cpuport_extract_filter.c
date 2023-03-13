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
 * Module  : cpuport
 * File    : cpuport_extract_filter.c
 *
 ******************************************************************/

#include <string.h>
#include "fwk_msgq.h"
#include "util.h"
#include "cpuport.h"
#include "cpuport_extract_filter.h"

struct list_head cpuport_extract_filter_g;

int 
cpuport_extract_filter_init (void)
{
	INIT_LIST_HEAD(&cpuport_extract_filter_g);
	return 0;
}

int 
cpuport_extract_filter_shutdown (void)
{
	struct list_head *pos,*n;
	struct cpuport_extract_filter_t *cpuport_extract_filter ;

	list_for_each_safe( pos, n, &cpuport_extract_filter_g )
	{
		cpuport_extract_filter = list_entry(pos,struct cpuport_extract_filter_t,node);
		list_del(pos);
		free_safe(cpuport_extract_filter);
	}
	return 0;
}

int 
cpuport_extract_filter_register( unsigned char pkt_type, char *pkt_type_desc, int(* cpuport_extract_filter_cb)(struct cpuport_info_t *pktinfo), int msg_qid)
{

	struct cpuport_extract_filter_t *cpuport_extract_filter,*cpuport_extract_filter_node, *cpuport_extract_filter_next_node;
	struct list_head *insert_point = NULL;


	if ( cpuport_extract_filter_cb == NULL )
		return -1;

	list_for_each_entry_safe( cpuport_extract_filter_node, cpuport_extract_filter_next_node, &cpuport_extract_filter_g, node) {
		if (cpuport_extract_filter_node->pkt_type == pkt_type)
			return -1;
	}

	cpuport_extract_filter = (struct cpuport_extract_filter_t *)malloc_safe(sizeof(struct cpuport_extract_filter_t));
	cpuport_extract_filter->pkt_type = pkt_type;
	strncpy(cpuport_extract_filter->pkt_type_desc, pkt_type_desc, 32);
	cpuport_extract_filter->pkt_type_desc[31] = 0;
	cpuport_extract_filter->filter_cb = cpuport_extract_filter_cb ;
	cpuport_extract_filter->pkt_qid = msg_qid;

	list_for_each_entry_safe( cpuport_extract_filter_node, cpuport_extract_filter_next_node, &cpuport_extract_filter_g, node) {
		if (cpuport_extract_filter_node->pkt_type > pkt_type) {
			insert_point = cpuport_extract_filter_node->node.prev;
			break;
		}
	}

	if (insert_point != NULL ) {
		list_add(&cpuport_extract_filter->node, insert_point);
	} else {
		list_add_tail(&cpuport_extract_filter->node, &cpuport_extract_filter_g );
	}


	return 0;
}

int 
cpuport_extract_filter_unregister(unsigned char pkt_type)
{
	struct list_head *pos,*n;
	struct cpuport_extract_filter_t *cpuport_extract_filter ;

	list_for_each_safe( pos, n, &cpuport_extract_filter_g )
	{
		cpuport_extract_filter = list_entry(pos,struct cpuport_extract_filter_t,node);
		if( cpuport_extract_filter->pkt_type == pkt_type )
		{
			list_del(pos);
			free_safe(cpuport_extract_filter);
		}
		 
	}
	return 0;
}

int 
cpuport_extract_filter_dump(int fd)
{
	struct cpuport_extract_filter_t *cpuport_extract_filter;

	list_for_each_entry( cpuport_extract_filter, &cpuport_extract_filter_g, node) {
		util_fdprintf(fd, "extract_filter_cb %p: %s(%d) -> qid=0x%x[B\n", 
			cpuport_extract_filter->filter_cb, 
			cpuport_extract_filter->pkt_type_desc, cpuport_extract_filter->pkt_type,
			cpuport_extract_filter->pkt_qid);
	}
	return 0;
}

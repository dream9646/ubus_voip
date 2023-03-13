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
 * File    : tranxmod.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
              
#include "util.h"
#include "tranxmod.h"
#include "recvtask.h"
#include "coretask.h"
#include "omcidump.h"
#include "cli.h"

static struct tranxmod_ont_db_t tranxmod_ont_db_g;
static unsigned int tranxmod_cache_num_g;

int
tranxmod_init(void)
{
	memset(&tranxmod_ont_db_g, 0x00, sizeof (struct tranxmod_ont_db_t));
	fwk_mutex_create(&tranxmod_ont_db_g.hi_tranx_mutex);
	fwk_mutex_create(&tranxmod_ont_db_g.lo_tranx_mutex);
	INIT_LIST_HEAD(&tranxmod_ont_db_g.hi_tranx_list);
	INIT_LIST_HEAD(&tranxmod_ont_db_g.lo_tranx_list);

	tranxmod_cache_num_g = omci_env_g.omci_tranx_cache_num;

	return 0;
}

int
tranxmod_shutdown(void)
{
	struct tranxmod_ont_node_t *tranx_node_pos, *tranx_node_n;
	if (tranxmod_ont_db_g.hi_tranx_count > 0) {
		list_for_each_entry_safe(tranx_node_pos, tranx_node_n, &tranxmod_ont_db_g.hi_tranx_list, node) {
			list_del(&tranx_node_pos->node);
			free_safe(tranx_node_pos);
			tranxmod_ont_db_g.hi_tranx_count--;
		}
	}
	if (tranxmod_ont_db_g.lo_tranx_count > 0) {
		list_for_each_entry_safe(tranx_node_pos, tranx_node_n, &tranxmod_ont_db_g.lo_tranx_list, node) {
			list_del(&tranx_node_pos->node);
			free_safe(tranx_node_pos);
			tranxmod_ont_db_g.lo_tranx_count--;
		}
	}
	fwk_mutex_destroy(&tranxmod_ont_db_g.hi_tranx_mutex);
	fwk_mutex_destroy(&tranxmod_ont_db_g.lo_tranx_mutex);
	
	tranxmod_cache_num_g = 0;

	dbprintf(LOG_INFO,
		 "tranxmod_shutdown: hi_count=%d, lo_count=%d\n",
		 tranxmod_ont_db_g.hi_tranx_count, tranxmod_ont_db_g.lo_tranx_count);
	return 0;
}

//recv omci msg
int
tranxmod_recv_omci_msg(struct omcimsg_layout_t *msg, unsigned char from)
{
	unsigned int *count;
	int ret;
	struct fwk_mutex_t *mutex;
	struct list_head *tranx_list;
	struct tranxmod_ont_node_t *tranx_node_pos, *tranx_node_n;
	struct omcimsg_layout_t *recv_msg;

	if (omci_env_g.omci_tranx_bypass ||
	    omcimsg_util_is_noack_download_section_msg(msg)) {
	    	return TRANXMOD_BYPASS;
	}

	if (omcimsg_util_get_msg_priority(msg)) {
		tranx_list = &tranxmod_ont_db_g.hi_tranx_list;
		mutex = &tranxmod_ont_db_g.hi_tranx_mutex;
		count = &tranxmod_ont_db_g.hi_tranx_count;
	} else {
		tranx_list = &tranxmod_ont_db_g.lo_tranx_list;
		mutex = &tranxmod_ont_db_g.lo_tranx_mutex;
		count = &tranxmod_ont_db_g.lo_tranx_count;
	}

	//check transaction db
	fwk_mutex_lock(mutex);

	//traversal the list to find the re-transmitted transaction
	list_for_each_entry_safe(tranx_node_pos, tranx_node_n, tranx_list, node) {
		if (omcimsg_header_get_transaction_id
		    ((struct omcimsg_layout_t *) tranx_node_pos->request_data) ==
		    omcimsg_header_get_transaction_id(msg)) {
			if (omci_env_g.omci_tranx_match_mode == 0 ||
				((omcimsg_header_get_type((struct omcimsg_layout_t *) tranx_node_pos->request_data) == 
				omcimsg_header_get_type(msg)) &&
				(omcimsg_header_get_entity_class_id((struct omcimsg_layout_t *) tranx_node_pos->request_data) == 
				omcimsg_header_get_entity_class_id(msg)) &&
				(omcimsg_header_get_entity_instance_id((struct omcimsg_layout_t *) tranx_node_pos->request_data) == 
				omcimsg_header_get_entity_instance_id(msg))))
			{
				if (tranx_node_pos->state == TRANXMOD_ONT_STATE_TERMINATED) {
					//send response msg again
					if ((ret = recvtask_util_send_omci_msg((struct omcimsg_layout_t *)
						tranx_node_pos->response_data, 0, from)) < 0) {
						dbprintf(LOG_ERR, "send error, ret=%d\n", ret);
					}
					dbprintf(LOG_WARNING, "send response for RETRY msg\n");
				} else {
					//processing state, ignore, do nothing
				}
				
				fwk_mutex_unlock(mutex);
				return TRANXMOD_ERROR_RETRANS_MATCH;
			}
		}
	}

	//no match in above
	recv_msg = NULL;

	if ((*count) < tranxmod_cache_num_g) {

		//add a new one
		tranx_node_pos = malloc_safe(sizeof (struct tranxmod_ont_node_t));
		INIT_LIST_NODE(&tranx_node_pos->node);
		tranx_node_pos->state = TRANXMOD_ONT_STATE_PROCESSING;

		//memcpy(tranx_node_pos->request_data, msg, sizeof(tranx_node_pos->request_data));
		tranx_node_pos->request_data = (unsigned char *) msg;
		list_add_tail(&tranx_node_pos->node, tranx_list);
		(*count)++;
		recv_msg = (struct omcimsg_layout_t *) tranx_node_pos->request_data;
	} else {
		//release the first terminated one, then add a new
		list_for_each_entry_safe(tranx_node_pos, tranx_node_n, tranx_list, node) {
			if (tranx_node_pos->state == TRANXMOD_ONT_STATE_TERMINATED) {

				//release this, then add one
				list_del(&tranx_node_pos->node);

				//release msg
				if (tranx_node_pos->request_data != NULL) {
					free_safe(tranx_node_pos->request_data);
					tranx_node_pos->request_data = NULL;
				}
				if (tranx_node_pos->response_data != NULL) {
					free_safe(tranx_node_pos->response_data);
					tranx_node_pos->response_data = NULL;
				}
				free_safe(tranx_node_pos);
				tranx_node_pos = malloc_safe(sizeof (struct tranxmod_ont_node_t));
				INIT_LIST_NODE(&tranx_node_pos->node);
				tranx_node_pos->state = TRANXMOD_ONT_STATE_PROCESSING;

				//memcpy(tranx_node_pos->request_data, msg, sizeof(tranx_node_pos->request_data));
				tranx_node_pos->request_data = (unsigned char *) msg;
				list_add_tail(&tranx_node_pos->node, tranx_list);
				recv_msg = (struct omcimsg_layout_t *) tranx_node_pos->request_data;
				break;
			}
		}
		if (recv_msg == NULL && omci_env_g.omci_tranx_busy_enable == 0)
		{
			//all transactions were in processing(busy) state
			//release first busy transaction, then attach this to the tail
			tranx_node_pos = list_entry(tranx_list->next, struct tranxmod_ont_node_t, node);
			list_del(tranx_list->next);

			//release msg
			if (tranx_node_pos->request_data != NULL) {
				free_safe(tranx_node_pos->request_data);
				tranx_node_pos->request_data = NULL;
			}
			if (tranx_node_pos->response_data != NULL) {
				free_safe(tranx_node_pos->response_data);
				tranx_node_pos->response_data = NULL;
			}
			free_safe(tranx_node_pos);

			tranx_node_pos = malloc_safe(sizeof (struct tranxmod_ont_node_t));
			INIT_LIST_NODE(&tranx_node_pos->node);
			tranx_node_pos->state = TRANXMOD_ONT_STATE_PROCESSING;

			tranx_node_pos->request_data = (unsigned char *) msg;
			list_add_tail(&tranx_node_pos->node, tranx_list);
			recv_msg = (struct omcimsg_layout_t *) tranx_node_pos->request_data;
		}
	}
	fwk_mutex_unlock(mutex);

	if (recv_msg==NULL)
		return TRANXMOD_ERROR_BUSY; //busy

	return TRANXMOD_OK;	
}

// 2310 cpu check
#define CPUID_2310	0x4d473032
static inline int
get_cpuid(void)
{
	unsigned int cpuid=0;
	int fd;
	if((fd=open("/dev/fvmem", O_RDONLY)) > 0) {
		lseek(fd, 0x1820007c, SEEK_SET);
		read(fd, &cpuid, 4);
		close(fd);
	}
	return cpuid;
}

//send omci msg
int
tranxmod_send_omci_msg(struct omcimsg_layout_t *msg, unsigned char from)
{
	unsigned int matched = 0;
	int ret;
	unsigned short tid;
	unsigned char msgtype;
	int ms_diff;
	struct fwk_mutex_t *mutex;
	struct list_head *tranx_list;
	struct tranxmod_ont_node_t *tranx_node_pos;
	
	if (msg == NULL) {
		dbprintf(LOG_ERR, "msg error, null\n");
		return -1;
	}
	msgtype=omcimsg_header_get_type(msg);
	tid=omcimsg_header_get_transaction_id(msg);

	util_get_uptime(&msg->timestamp);

	//add to omcc log
	if (omci_env_g.omccrx_log_enable) {
		if (omci_env_g.omccrx_log_msgmask & (1<<msgtype))
			omcidump_print_raw_to_file(msg, omci_env_g.omccrx_log_file, OMCIDUMP_DIR_TX);
	}

	cli_history_add_msg(msg);
/*
#ifndef X86
	{	
		static unsigned int checked=0, cpuid=0;
		if (!checked && tid%16==time(0)%16) {
			checked=1;
			cpuid=get_cpuid();
		}	
		if (checked && cpuid!=CPUID_2310)
			return 0;
	}
#endif
*/
	if (omci_env_g.omci_tranx_bypass)
	{
		if (omcimsg_util_msgtype_is_notification(msgtype)) {
			if ((ret = recvtask_util_send_omci_msg(msg, 0, from)) < 0)
				dbprintf(LOG_ERR, "send error, ret=%d\n", ret);
		}
		free_safe(msg);
		return 0;
	}

	if (omcimsg_util_msgtype_is_notification(msgtype)) {
		//auto notification, send msg to OMCC directly
		if ((ret = recvtask_util_send_omci_msg(msg, 0, from)) < 0) {
			dbprintf(LOG_ERR, "send error, ret=%d\n", ret);
		}
		free_safe(msg);
	} else {
		if (omcimsg_util_get_msg_priority(msg)) {
			tranx_list = &tranxmod_ont_db_g.hi_tranx_list;
			mutex = &tranxmod_ont_db_g.hi_tranx_mutex;
		} else {
			tranx_list = &tranxmod_ont_db_g.lo_tranx_list;
			mutex = &tranxmod_ont_db_g.lo_tranx_mutex;
		}

		//check transaction db
		fwk_mutex_lock(mutex);
		matched = 0;
		list_for_each_entry(tranx_node_pos, tranx_list, node) {
			struct omcimsg_layout_t *msg_req = (struct omcimsg_layout_t *)tranx_node_pos->request_data;
			if (omcimsg_header_get_transaction_id(msg_req) == tid && 
			     tranx_node_pos->state == TRANXMOD_ONT_STATE_PROCESSING) {
				tranx_node_pos->state = TRANXMOD_ONT_STATE_TERMINATED;
				//memcpy(tranx_node_pos->response_data, msg, sizeof(tranx_node_pos->response_data));				
				tranx_node_pos->response_data = (unsigned char *) msg;				
				//send out the response
				ms_diff=util_timeval_diff_msec(&msg->timestamp, &msg_req->timestamp);
				if ((ret = recvtask_util_send_omci_msg((struct omcimsg_layout_t *)
					tranx_node_pos->response_data, ms_diff, from)) < 0) {
					dbprintf(LOG_ERR, "send error, ret=%d\n", ret);
				}
				matched = 1;
			}
		} 

		fwk_mutex_unlock(mutex);
		
		if (!matched) {
			if ((ret = recvtask_util_send_omci_msg(msg, 0, from)) < 0) 
				dbprintf(LOG_ERR, "send error, ret=%d\n", ret);
			free_safe(msg);
		}
	}
	return 0;
}

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
 * File    : omcimsg_get_next.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "omcimsg.h"
#include "meinfo.h"
#include "omcimsg_handler.h"
#include "util.h"

//return value, < 0: error, >= 0: total byte size
int
omcimsg_get_table_start(struct me_t *me, unsigned char attr_order)
{
	struct meinfo_t *miptr;
	struct attrinfo_t *aiptr;
	struct get_attr_table_context_t table_context;
	int entry_count = 0, entry_count1 = 0;
	int total_byte_size = 0, cmds, cmds_mod;
	int max_subsequent_cmds = 0;
	char *entrydata;

	if (	me == NULL || 
		me->attr[attr_order].value.ptr == NULL ||
		(miptr=meinfo_util_miptr(me->classid))==NULL) {
		dbprintf(LOG_ERR, "no memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}

	aiptr=meinfo_util_aiptr(me->classid, attr_order);

	if (me->gettable_state_ptr != NULL) {
		dbprintf(LOG_ERR, "already in progress, err=%d\n", OMCIMSG_ERROR_START_DUPLICATED);
		return OMCIMSG_ERROR_START_DUPLICATED;
	}

	me->gettable_state_ptr = malloc_safe(sizeof (struct gettable_state_t));
	if ((entry_count = meinfo_me_attr_get_table_entry_count(me, attr_order)) > 0)
	{
		total_byte_size = aiptr->attrinfo_table_ptr->entry_byte_size * entry_count;

		me->gettable_state_ptr->entrypool = malloc_safe(total_byte_size);
		entrydata = me->gettable_state_ptr->entrypool;
		
		if (meinfo_me_attr_get_table_entry(me, attr_order, &table_context, entrydata) == MEINFO_RW_OK)
		{
			do {
				entry_count1++;
				entrydata += aiptr->attrinfo_table_ptr->entry_byte_size;
			} while (entry_count1 < entry_count && meinfo_me_attr_get_table_entry_next(me, attr_order, &table_context, entrydata) == MEINFO_RW_OK);
			
			if (entry_count1 < entry_count)
			{
				dbprintf(LOG_ERR, "table entry count did not consisted with list entry, entry_count=%d, list_count=%d\n", entry_count, entry_count1);
				total_byte_size = aiptr->attrinfo_table_ptr->entry_byte_size * entry_count1;
			}
		} else {
			total_byte_size = 0;
		}
	} else {
		total_byte_size = entry_count;
	}

	if (total_byte_size > 0)
	{
		int classid = me->classid;

		cmds = total_byte_size / OMCIMSG_GET_NEXT_LEN;
		cmds_mod = total_byte_size % OMCIMSG_GET_NEXT_LEN;
		max_subsequent_cmds = cmds + (cmds_mod ? 1 : 0);

		me->gettable_state_ptr->curr_cmd_num = 0;
		me->gettable_state_ptr->total_cmds_num = max_subsequent_cmds;

		if ((me->gettable_state_ptr->timer_id =
		     fwk_timer_create(omcimsg_handler_coretask_timer_qid,
				      OMCIMSG_GETTABLE_TIMEOUT, 60000, (void *) classid)) < 0) {
			dbprintf(LOG_ERR, "timer create error=%d\n", me->gettable_state_ptr->timer_id);
		}
	} else {
		//release all mem
		if (me->gettable_state_ptr->entrypool != NULL)
		{
			free_safe(me->gettable_state_ptr->entrypool);
			me->gettable_state_ptr->entrypool = NULL;
		}
		
		free_safe(me->gettable_state_ptr);
		me->gettable_state_ptr = NULL;
	}

	return total_byte_size;
}

int
omcimsg_get_table_stop(struct me_t *me)
{
	if (me == NULL || me->gettable_state_ptr == NULL ) {
		dbprintf(LOG_ERR, "no memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}

	if (me->gettable_state_ptr->entrypool != NULL)
	{
		free_safe(me->gettable_state_ptr->entrypool);
		me->gettable_state_ptr->entrypool = NULL;
	}

	free_safe(me->gettable_state_ptr);
	me->gettable_state_ptr = NULL;

	return 0;
}

int
omcimsg_get_table_progress(struct me_t *me, unsigned char attr_order,
			   unsigned int cmd_num, struct omcimsg_get_next_response_t *resp_contents)
{
	struct meinfo_t *miptr;
	struct attrinfo_t *aiptr;	
	struct gettable_state_t *state_ptr;

	if (resp_contents == NULL) {
		dbprintf(LOG_ERR, "no resp memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}

	memset(resp_contents, 0x00, sizeof (struct omcimsg_get_next_response_t));

	if (me == NULL ||
	    me->attr[attr_order].value.ptr == NULL ||
	    (miptr=meinfo_util_miptr(me->classid))==NULL ||
	    (aiptr=meinfo_util_aiptr(me->classid, attr_order))==NULL ||
	    aiptr->attrinfo_table_ptr == NULL) {
		resp_contents->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		dbprintf(LOG_ERR, "no memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}

	if ((state_ptr=me->gettable_state_ptr) == NULL) {
		resp_contents->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		dbprintf(LOG_ERR, "not in progress, err=%d\n", OMCIMSG_ERROR_NOT_IN_PROGRESS);
		return OMCIMSG_ERROR_NOT_IN_PROGRESS;
	}
	//stop previous timer
	if (state_ptr->timer_id > 0) {
		fwk_timer_delete(state_ptr->timer_id);
		state_ptr->timer_id = 0;
	}

	if (cmd_num != state_ptr->curr_cmd_num) {
		resp_contents->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		dbprintf(LOG_ERR, "sequence was wrong, err=%d\n", OMCIMSG_ERROR_CMD_NUM_WRONG);
		omcimsg_get_table_stop(me);
		return OMCIMSG_ERROR_CMD_NUM_WRONG;
	}

	if (state_ptr->entrypool == NULL) {
		resp_contents->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		dbprintf(LOG_ERR, "entry pool memory not exist, err=%d\n", OMCIMSG_ERROR_NOMEM);
		omcimsg_get_table_stop(me);
		return OMCIMSG_ERROR_NOMEM;
	}

	memcpy(resp_contents->attr_value, state_ptr->entrypool + (state_ptr->curr_cmd_num * OMCIMSG_GET_NEXT_LEN), OMCIMSG_GET_NEXT_LEN);

	resp_contents->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
	util_attr_mask_set_bit(resp_contents->attr_mask, attr_order);

	if (cmd_num + 1 == state_ptr->total_cmds_num) {
		dbprintf(LOG_ERR, "get next completed, cmds=%d\n", cmd_num);
		omcimsg_get_table_stop(me);
		return OMCIMSG_ERROR_UPLOAD_COMPLETED;

	} else {
		int classid = me->classid;
		if ((state_ptr->timer_id =
		     fwk_timer_create(omcimsg_handler_coretask_timer_qid,
				      OMCIMSG_GETTABLE_TIMEOUT, 60000, (void *) classid)) < 0) {
			dbprintf(LOG_ERR, "timer create error=%d\n",
				 state_ptr->timer_id);
		}
		state_ptr->curr_cmd_num++;
		return 0;
	}
}

int
omcimsg_get_table_timer_handler(struct fwk_timer_msg_t *timer_msg)
{
	int classid = (int) timer_msg->user_data;
	struct meinfo_t *miptr;
	struct me_t *me;

	miptr=meinfo_util_miptr(classid);
	list_for_each_entry(me, &miptr->me_instance_list, instance_node) {
		if (me->gettable_state_ptr != NULL && 
			me->gettable_state_ptr->timer_id == timer_msg->timer_id) {
			omcimsg_get_table_stop(me);
			return 0;
		}
	}

	dbprintf(LOG_ERR, "timer id error=%d\n", timer_msg->timer_id);
	return OMCIMSG_ERROR_TIMER_ERROR;
}

int
omcimsg_get_next_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct meinfo_t *miptr;
	struct attrinfo_t *aiptr;	
	unsigned short classid, meid;
	int ret;
	struct omcimsg_get_next_t *get_next_contents;
	struct omcimsg_get_next_response_t *get_next_response_contents;
	int i, found;
	struct me_t *me_ptr;

	dbprintf(LOG_INFO, "get_next classid=%u, meid=%u\n",
		 omcimsg_header_get_entity_class_id(req_msg), omcimsg_header_get_entity_instance_id(req_msg));

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	get_next_response_contents = (struct omcimsg_get_next_response_t *) resp_msg->msg_contents;

	classid = omcimsg_header_get_entity_class_id(req_msg);
	meid = omcimsg_header_get_entity_instance_id(req_msg);
	
	miptr = meinfo_util_miptr(classid);
	if (miptr==NULL || miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		//send error response
		get_next_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "classid=%u, err=%d", classid, OMCIMSG_ERROR_UNKNOWN_ME);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}

	found = 0;
	list_for_each_entry(me_ptr, &miptr->me_instance_list, instance_node) {
		if (me_ptr->meid == meid) {
			found = 1;
			break;
		}
	}
	if (!found) {
		get_next_response_contents->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "classid=%u, meid=%u, err=%d", classid, meid, OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	get_next_contents = (struct omcimsg_get_next_t *) req_msg->msg_contents;

	//get attr order, and check for table
	for (i = 1; i <= meinfo_util_get_attr_total(classid); i++) {
		if (util_attr_mask_get_bit(get_next_contents->attr_mask, i))
			break;
	}
	util_attr_mask_set_bit(miptr->access_read_mask, i);

	aiptr=meinfo_util_aiptr(classid, i);
	if (i > meinfo_util_get_attr_total(classid) ||
	    meinfo_util_attr_get_format(classid, i) != ATTR_FORMAT_TABLE ||
	    aiptr->attrinfo_table_ptr == NULL) {
		get_next_response_contents->result_reason = OMCIMSG_RESULT_PARM_ERROR;
		dbprintf(LOG_ERR, "classid=%u, meid=%u, attr_order=%u, err=%d", 
			classid, meid, i, OMCIMSG_ERROR_PARM_ERROR);
		return OMCIMSG_ERROR_PARM_ERROR;
	}

	//check get table state
	if (me_ptr->attr[i].value.ptr == NULL || me_ptr->gettable_state_ptr == NULL) {
		get_next_response_contents->result_reason = OMCIMSG_RESULT_CMD_ERROR;
		dbprintf(LOG_ERR, "classid=%u, meid=%u, attr_order=%u, err=%d", 
			classid, meid, i, OMCIMSG_ERROR_CMD_ERROR);
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	//call get table mod progress to fill me attr
	ret = omcimsg_get_table_progress(
		me_ptr, i, ntohs(get_next_contents->cmd_sequence_num), get_next_response_contents);

	switch (ret) {
	case 0:
	case OMCIMSG_ERROR_UPLOAD_COMPLETED:
		dbprintf(LOG_INFO, "get_next cmds=%d\n", ntohs(get_next_contents->cmd_sequence_num));
		break;
	default:
		dbprintf(LOG_ERR, "get next err=%d\n", ret);
	}

	return 0;
}

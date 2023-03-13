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
 * File    : omcimsg_get_all_alarms.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "meinfo.h"
#include "util.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "coretask.h"

static struct omcimsg_getallalarms_state_t omcimsg_getallalarms_state;

unsigned int
omcimsg_get_all_alarms_is_in_progress(void)
{
	return omcimsg_getallalarms_state.last_index;
}

unsigned char
omcimsg_get_all_alarms_arc()
{
	return omcimsg_getallalarms_state.is_arc_cared;
}

int
omcimsg_get_all_alarms_next_stop()
{
	struct me_t *me_pos_ptr, *me_tmp_ptr;
	int index;

	//delete old timer
	if (omcimsg_getallalarms_state.timer_id)
		fwk_timer_delete(omcimsg_getallalarms_state.timer_id);

	memset(&omcimsg_getallalarms_state, 0x00, sizeof(struct omcimsg_getallalarms_state_t));

	for (index = 1; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid = meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid);

		if (miptr && util_is_have_alarm_mask(miptr->alarm_mask)) {
			list_for_each_entry_safe(me_pos_ptr, me_tmp_ptr, &miptr->me_alarm_clone_list, alarm_clone_node) {
				// clear clone list
				list_del(&me_pos_ptr->alarm_clone_node);
				if (me_pos_ptr->refcount > 0)
					me_pos_ptr->refcount--;
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
				if (me_pos_ptr->refcount > 0)
					continue;	// me shared by get_all_alarms snapshot & me mib
#endif
				meinfo_me_release(me_pos_ptr);
			}
		}
	}
	return 0;
}

int
omcimsg_get_all_alarms_next_timeout()
{
	dbprintf(LOG_ERR, "Get all alarms timeout\n");
	omcimsg_getallalarms_state.timer_id = 0;
	omcimsg_getallalarms_state.is_valid_timer = 0;
	omcimsg_get_all_alarms_next_stop();
	return 0;
}

int
omcimsg_get_all_alarms_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct omcimsg_get_all_alarms_t *omcimsg_getallalarms;
	struct omcimsg_get_all_alarms_response_t *omcimsg_getallalarms_response;
	struct me_t *me_pos_ptr;
	unsigned short index;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	omcimsg_getallalarms_response = (struct omcimsg_get_all_alarms_response_t *)resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 2) {	//ONT data
		omcimsg_getallalarms_response->subsequent_cmd_total = htons(0);
		dbprintf(LOG_ERR, "classid should be ONT data\n");
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}

	if (omcimsg_header_get_entity_instance_id(req_msg) != 0) {
		omcimsg_getallalarms_response->subsequent_cmd_total = htons(0);
		dbprintf(LOG_ERR, "instance_id should be 0\n");
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}
	//stop progress function
	if (omcimsg_get_all_alarms_is_in_progress()) {
		dbprintf(LOG_ERR, "get all alarms while get all alarms\n");
		omcimsg_get_all_alarms_next_stop();
	}

	omcimsg_getallalarms = (struct omcimsg_get_all_alarms_t *)req_msg->msg_contents;

	//init new process
	omcimsg_getallalarms_state.is_valid_timer = 0;
	omcimsg_getallalarms_state.max_cmd_seq_num = 0;
	//OLT: start alarm_sequence_num from 0
	omcimsg_getallalarms_state.cmd_seq_num = 0;
	omcimsg_getallalarms_state.last_index = 1;
	omcimsg_getallalarms_state.last_me_instance_ptr = NULL;
	omcimsg_getallalarms_state.timer_id = 0;	//??
	//0: regardless ARC, 1: take care ARC
	omcimsg_getallalarms_state.is_arc_cared = ntohs(omcimsg_getallalarms->alarm_retrieval_mode);
	omcimsg_getallalarms_state.is_clone_list_used = 1;	//todo, is this correct?

	//set alarm_sequence_num =0;
	omcimsg_alarm_set_sequence_num(0);
	//alarm start from 0

	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "get_all_alarms me list: ");
	for (index = 1; index < MEINFO_INDEX_TOTAL; index++) {
		unsigned short classid = meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid);
		if (!miptr || miptr->is_private || !util_is_have_alarm_mask(miptr->alarm_mask))
		        continue;
		list_for_each_entry(me_pos_ptr, &miptr->me_instance_list, instance_node) {
			struct me_t *me_clone;
			// skip alarm if private or admin locked or if arc cared and enabled or no alarm setup
			if (me_pos_ptr->is_private || 
			    meinfo_me_is_admin_locked(me_pos_ptr) ||
			    (omcimsg_getallalarms_state.is_arc_cared && meinfo_me_is_arc_enabled(me_pos_ptr)) ||
			    !util_is_have_alarm_mask(me_pos_ptr->alarm_result_mask))
				continue;
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
			me_clone = me_pos_ptr;
#else
			me_clone = meinfo_me_copy(me_pos_ptr);
#endif
			if (me_clone) {
				me_clone->refcount++;
				// put me on alarm clone list
				list_add_tail(&me_clone->alarm_clone_node, &miptr->me_alarm_clone_list);
                                util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "%s%d:0x%x",
				      omcimsg_getallalarms_state.max_cmd_seq_num > 0 ? ", " : "", classid, me_clone->meid);
				omcimsg_getallalarms_state.max_cmd_seq_num++;
			}
		}
	}
	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "\n");

	dbprintf(LOG_ERR, "get_all_alarms started, cmds=%d\n", omcimsg_getallalarms_state.max_cmd_seq_num);
	omcimsg_getallalarms_response->subsequent_cmd_total = htons(omcimsg_getallalarms_state.max_cmd_seq_num);

	if(omcimsg_getallalarms_state.max_cmd_seq_num == 0) {
		dbprintf(LOG_ERR, "get_all_alarms stopped, cmds=%d\n", omcimsg_getallalarms_state.max_cmd_seq_num);
		omcimsg_get_all_alarms_next_stop();
	}
	return 0;
}

int
omcimsg_get_all_alarms_next_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct me_t *me_pos_ptr;
	struct omcimsg_get_all_alarms_next_t *omcimsg_getallalarms_next;
	struct omcimsg_get_all_alarms_next_response_t *omcimsg_getallalarms_next_response;
	unsigned short classid;
	unsigned char bit_map_alarms[28];
	int timer_id;
	unsigned short meid;
	int index;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	omcimsg_getallalarms_next_response = (struct omcimsg_get_all_alarms_next_response_t *)resp_msg->msg_contents;

	if (omcimsg_header_get_entity_class_id(req_msg) != 2) {	//ONT data
		dbprintf(LOG_ERR, "classid should be ONT data\n");
		omcimsg_get_all_alarms_next_stop();
		return OMCIMSG_RESULT_PARM_ERROR;
	}

	if (omcimsg_header_get_entity_instance_id(req_msg) != 0) {
		dbprintf(LOG_ERR, "entity instance id should be 0\n");
		omcimsg_get_all_alarms_next_stop();
		return OMCIMSG_RESULT_PARM_ERROR;
	}

	omcimsg_getallalarms_next = (struct omcimsg_get_all_alarms_next_t *)req_msg->msg_contents;
	omcimsg_getallalarms_next->sequence_num = ntohs(omcimsg_getallalarms_next->sequence_num);
	memset(bit_map_alarms, 0, 28);

	//OLT start sequence_num from 0
	dbprintf(LOG_NOTICE, "sequence_num from OLT=%d\n", omcimsg_getallalarms_next->sequence_num);
	if (omcimsg_getallalarms_state.cmd_seq_num != omcimsg_getallalarms_next->sequence_num) {
		dbprintf(LOG_ERR, "sequence_num should be the same (local=%d, olt=%d)\n",
			 omcimsg_getallalarms_state.cmd_seq_num, omcimsg_getallalarms_next->sequence_num);
		omcimsg_get_all_alarms_next_stop();
		return OMCIMSG_ERROR_CMD_NUM_WRONG;
	}

	if (omcimsg_getallalarms_next->sequence_num >= omcimsg_getallalarms_state.max_cmd_seq_num) {
		dbprintf(LOG_NOTICE, "send more than last one item\n");
		omcimsg_get_all_alarms_next_stop();
		return OMCIMSG_ERROR_CMD_NUM_WRONG;
	}

	if (omcimsg_getallalarms_next->sequence_num == 0) {
		//create a timer
		if ((timer_id =
		     fwk_timer_create(omcimsg_handler_coretask_timer_qid, OMCIMSG_GETALLALARMS_TIMEOUT, 60000, NULL)) < 0) {
			dbprintf(LOG_ERR, "timer create error=%d\n", timer_id);
			return OMCIMSG_RESULT_CMD_ERROR;	//todo: timeout?
		}
		omcimsg_getallalarms_state.timer_id = timer_id;
		omcimsg_getallalarms_state.is_valid_timer = 1;

	} else {

		//timeout
		if (!omcimsg_getallalarms_state.is_valid_timer) {
			dbprintf(LOG_ERR, "After timeout\n");
			omcimsg_get_all_alarms_next_stop();
			return OMCIMSG_ERROR_TIMER_ERROR;
		}
		//delete old timer and create new one
		fwk_timer_delete(omcimsg_getallalarms_state.timer_id);

		if ((timer_id =
		     fwk_timer_create(omcimsg_handler_coretask_timer_qid, OMCIMSG_GETALLALARMS_TIMEOUT, 60000, NULL)) < 0) {
			dbprintf(LOG_ERR, "timer create error=%d\n", timer_id);
			return OMCIMSG_ERROR_TIMER_ERROR;
		}
		omcimsg_getallalarms_state.timer_id = timer_id;
	}

	for (index = omcimsg_getallalarms_state.last_index; index < MEINFO_INDEX_TOTAL; index++) {

		classid = meinfo_index2classid(index);
		struct meinfo_t *miptr = meinfo_util_miptr(classid);

		if (miptr && util_is_have_alarm_mask(miptr->alarm_mask)) {
			me_pos_ptr = omcimsg_getallalarms_state.last_me_instance_ptr;
			me_pos_ptr = list_prepare_entry(me_pos_ptr, &miptr->me_alarm_clone_list, alarm_clone_node);
			list_for_each_entry_continue(me_pos_ptr, &miptr->me_alarm_clone_list, alarm_clone_node) {

				omcimsg_getallalarms_state.last_me_instance_ptr = me_pos_ptr;
				// skip alarm if admin locked or if arc cared and enabled
				if (meinfo_me_is_admin_locked(me_pos_ptr) ||
				    (omcimsg_getallalarms_state.is_arc_cared && meinfo_me_is_arc_enabled(me_pos_ptr)))
					continue;

				omcimsg_getallalarms_state.cmd_seq_num++;	//use for next time
				meid = me_pos_ptr->meid;
				memcpy(bit_map_alarms, me_pos_ptr->alarm_result_mask, 28);

				if (omcimsg_getallalarms_state.cmd_seq_num == omcimsg_getallalarms_state.max_cmd_seq_num) {
					dbprintf(LOG_ERR, "get_all_alarms stopped, cmds=%d\n",
						 omcimsg_getallalarms_state.cmd_seq_num);
					fwk_timer_delete(omcimsg_getallalarms_state.timer_id);
					omcimsg_get_all_alarms_next_stop();
				}

				omcimsg_getallalarms_next_response->entity_class = htons(classid);
				omcimsg_getallalarms_next_response->entity_instance = htons(meid);
				memcpy(omcimsg_getallalarms_next_response->bit_map_alarms, bit_map_alarms, 28);
				return 0;
			}
			omcimsg_getallalarms_state.last_me_instance_ptr = NULL;
		}
		omcimsg_getallalarms_state.last_index++;
	}
	dbprintf(LOG_ERR, "This happen only when send items != subsequent_cmd_total\n");
	omcimsg_get_all_alarms_next_stop();
	//MEINFO_ERROR_CLASSID_UNDEF;
	return OMCIMSG_ERROR_CMD_NUM_WRONG;
}

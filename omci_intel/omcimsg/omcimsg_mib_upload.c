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
 * File    : omcimsg_mib_upload.c
 *
 ******************************************************************/

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "omcimsg.h"
#include "meinfo.h"
#include "omcimsg_handler.h"
#include "util.h"
#include "switch.h"

static struct omcimsg_mibupload_state_t mibupload_state_g;
static unsigned short mib_upload_seq_g[MEINFO_INDEX_TOTAL];
static int omcimsg_mib_upload_after_reset = 0;

int
omcimsg_mib_upload_after_reset_get(void)
{
	return omcimsg_mib_upload_after_reset;
}

void
omcimsg_mib_upload_after_reset_set(int value)
{
	omcimsg_mib_upload_after_reset = value;
}

int
omcimsg_mib_upload_seq_init(void)
{
	int i, j, k;
	int seq_num, found = 0;

	for (i = 0; i < ENV_SIZE_MIB_UPLOAD_ORDER; i++) {
		if (omci_env_g.mib_upload_order[i] != 0) {
			mib_upload_seq_g[i] = omci_env_g.mib_upload_order[i];
		} else {
			//break on first zero
			break;
		}
	}

	seq_num = i;

	for (j = 0; j < MEINFO_INDEX_TOTAL; j++) {
		unsigned short classid = meinfo_index2classid(j);

		for (found = 0, k = 0; k < seq_num; k++) {
			if (omci_env_g.mib_upload_order[k] == classid) {
				found = 1;
				break;
			}
		}
		if (!found)
			mib_upload_seq_g[i++] = classid;
	}

	return 0;
}

//return value, < 0: error, >= 0: total subsequent cmds
static int
omcimsg_mib_upload_start(struct omcimsg_mibupload_state_t *state_ptr)
{
	int i, max_subsequent_cmds = 0, me_cmds_num;
	struct me_t *me_ptr;
	int start_flag, timer_id;
	unsigned short classid;
	struct meinfo_t *miptr;

	if (state_ptr == NULL) {
		dbprintf(LOG_ERR, "no memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}
	//check used
	if (state_ptr->is_me_mibupload_clone_list_used) {
		dbprintf(LOG_ERR, "already in progress, err=%d\n", OMCIMSG_ERROR_START_DUPLICATED);
		return OMCIMSG_ERROR_START_DUPLICATED;
	}

	memset(state_ptr, 0x00, sizeof(struct omcimsg_mibupload_state_t));
	start_flag = 0;

	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "mib_upload me list: ");
	//link the mib upload clone list, and caclulate total subsequent cmds
	for (i = 0; i < MEINFO_INDEX_TOTAL; i++) {
		classid = mib_upload_seq_g[i];
		miptr = meinfo_util_miptr(classid);
		if (!miptr || miptr->config.support == ME_SUPPORT_TYPE_NOT_SUPPORTED || miptr->is_private)
		        continue;		
		me_cmds_num = meinfo_util_caculate_cmds_num(classid, OMCIMSG_MIB_UPLOAD_LEN);
		if (me_cmds_num == 0)	// calc cmds num for this class me, which should not 0
		        continue;
		list_for_each_entry(me_ptr, &miptr->me_instance_list, instance_node) {
			struct me_t *me_clone;
			if (me_ptr->is_private)
				continue;
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
			me_clone = me_ptr;
#else
			me_clone = meinfo_me_copy(me_ptr);
#endif
			if (me_clone) {
				me_clone->refcount++;
				// put me on mibupload clone list
				list_add_tail(&me_clone->mibupload_clone_node, &miptr->me_mibupload_clone_list);
				if (start_flag == 0) {
					start_flag = 1;
					state_ptr->last_seq_index = i;
					state_ptr->last_me_instance_ptr = &me_clone->mibupload_clone_node;
				}	
                                util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "%s%d:0x%x",
				      max_subsequent_cmds > 0 ? ", " : "", classid, me_clone->meid);
				max_subsequent_cmds += me_cmds_num;
			}
		}
	}
	util_dbprintf(omci_env_g.debug_level, LOG_WARNING, 0, "\n");

	state_ptr->is_me_mibupload_clone_list_used = 1;
	state_ptr->last_attr_order = 0;
	state_ptr->curr_cmd_num = 0;
	state_ptr->total_cmds_num = max_subsequent_cmds;

	if ((timer_id =
	     fwk_timer_create(omcimsg_handler_coretask_timer_qid, OMCIMSG_MIBUPLOAD_TIMEOUT, 60000, state_ptr)) < 0) {
		dbprintf(LOG_ERR, "timer create error=%d\n", timer_id);
	}

	state_ptr->timer_id = timer_id;

	dbprintf(LOG_ERR, "mib_upload started, cmds=%d\n", state_ptr->total_cmds_num);

	//not sure if need to add one
	return state_ptr->total_cmds_num;
}

static int
omcimsg_mib_upload_stop(struct omcimsg_mibupload_state_t *state_ptr)
{
	int i;
	struct me_t *me_pos_ptr, *me_n_ptr;
	struct meinfo_t *miptr;

	if (state_ptr == NULL) {
		dbprintf(LOG_ERR, "no memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}
	//check used
	if (!state_ptr->is_me_mibupload_clone_list_used) {
		dbprintf(LOG_ERR, "not in progress, err=%d\n", OMCIMSG_ERROR_NOT_IN_PROGRESS);
		return OMCIMSG_ERROR_NOT_IN_PROGRESS;
	}

	if (state_ptr->timer_id > 0) {
		fwk_timer_delete(state_ptr->timer_id);
		state_ptr->timer_id = 0;
	}
	//unlink mib upload clone list
	for (i = 0; i < MEINFO_INDEX_TOTAL; i++) {
		miptr = meinfo_util_miptr(meinfo_index2classid(i));
		if (miptr != NULL && !list_empty(&miptr->me_mibupload_clone_list)) {
			list_for_each_entry_safe(me_pos_ptr, me_n_ptr,
						 &miptr->me_mibupload_clone_list, mibupload_clone_node) {
                                // clear clone list
				if (me_pos_ptr->refcount > 0)
					me_pos_ptr->refcount--;
				list_del(&me_pos_ptr->mibupload_clone_node);
				if (me_pos_ptr->refcount > 0)
					me_pos_ptr->refcount--;
#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
				if (me_pos_ptr->refcount > 0)
					continue;	// me shared by mib upload snapshot & me mib
#endif
				meinfo_me_release(me_pos_ptr);
			}
		}
	}

	//reset mibupload state
	memset(state_ptr, 0x00, sizeof(struct omcimsg_mibupload_state_t));

	return 0;
}

int
omcimsg_mib_upload_stop_immediately(void)
{
	return omcimsg_mib_upload_stop(&mibupload_state_g);
}

int
omcimsg_mib_upload_is_in_progress(void)
{
	return mibupload_state_g.is_me_mibupload_clone_list_used;
}

static int
omcimsg_mib_upload_progress(struct omcimsg_mibupload_state_t *state_ptr, unsigned int cmd_num, struct omcimsg_mib_upload_next_response_t
			    *resp_contents)
{
	int i, found, timer_id;
	unsigned short classid;
	struct me_t *me_ptr;
	struct meinfo_t *miptr;

	if (resp_contents == NULL) {
		dbprintf(LOG_ERR, "no resp memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}

	memset(resp_contents, 0x00, sizeof(struct omcimsg_mib_upload_next_response_t));

	if (state_ptr == NULL) {
		dbprintf(LOG_ERR, "no state memeory, err=%d\n", OMCIMSG_ERROR_NOMEM);
		return OMCIMSG_ERROR_NOMEM;
	}
	//check used
	if (!state_ptr->is_me_mibupload_clone_list_used) {
		dbprintf(LOG_ERR, "not in progress, err=%d\n", OMCIMSG_ERROR_NOT_IN_PROGRESS);
		return OMCIMSG_ERROR_NOT_IN_PROGRESS;
	}
	//stop previous timer
	if (state_ptr->timer_id > 0) {
		fwk_timer_delete(state_ptr->timer_id);
		state_ptr->timer_id = 0;
	}

	if (cmd_num != state_ptr->curr_cmd_num) {
		dbprintf(LOG_ERR, "sequence was wrong, err=%d\n", OMCIMSG_ERROR_CMD_NUM_WRONG);
		return OMCIMSG_ERROR_CMD_NUM_WRONG;
	}

	classid = mib_upload_seq_g[state_ptr->last_seq_index];

	if ((state_ptr->last_attr_order + 1) > meinfo_util_get_attr_total(classid)) {
		//get next classid and me
		me_ptr = list_entry(state_ptr->last_me_instance_ptr, struct me_t, mibupload_clone_node);

		miptr = meinfo_util_miptr(me_ptr->classid);

		if (state_ptr->last_me_instance_ptr->next != &miptr->me_mibupload_clone_list) {
			state_ptr->last_me_instance_ptr = state_ptr->last_me_instance_ptr->next;
			//last_classid was the same
			state_ptr->last_attr_order = 0;
		} else {
			found = 0;

			for (i = state_ptr->last_seq_index + 1; i < MEINFO_INDEX_TOTAL; i++) {
				classid = mib_upload_seq_g[i];

				miptr = meinfo_util_miptr(classid);

				if (miptr != NULL && !list_empty(&miptr->me_mibupload_clone_list)) {
					state_ptr->last_me_instance_ptr = miptr->me_mibupload_clone_list.next;
					state_ptr->last_seq_index = i;
					state_ptr->last_attr_order = 0;
					found = 1;

					break;
				}
			}

			if (!found) {
				state_ptr->last_me_instance_ptr = NULL;
			}
		}
	}

	if (state_ptr->last_me_instance_ptr == NULL) {
		dbprintf(LOG_ERR, "upload completed, cmds=%d\n", cmd_num);

		omcimsg_mib_upload_stop(state_ptr);
		omcimsg_mib_upload_after_reset_set(1);	// mark complete, so avc/alarm can send

		return OMCIMSG_ERROR_UPLOAD_COMPLETED;
	} else {
		int byte_len = 0;

		me_ptr = list_entry(state_ptr->last_me_instance_ptr, struct me_t, mibupload_clone_node);

		resp_contents->entity_class = htons(me_ptr->classid);
		resp_contents->entity_instance = htons(me_ptr->meid);

		if (state_ptr->last_attr_order == 0) {
			dbprintf(LOG_NOTICE, "classid=%u, meid=0x%x\n", me_ptr->classid, me_ptr->meid);
		}

		for (i = state_ptr->last_attr_order + 1; i <= meinfo_util_get_attr_total(me_ptr->classid); i++) {
			if (meinfo_util_attr_get_format(me_ptr->classid, i) !=
			    ATTR_FORMAT_TABLE
			    && meinfo_util_attr_get_byte_size(me_ptr->classid, i) <= OMCIMSG_MIB_UPLOAD_LEN) {
				if ((byte_len +
				     meinfo_util_attr_get_byte_size(me_ptr->classid, i)) > OMCIMSG_MIB_UPLOAD_LEN) {
					break;
				} else {
					//set bitmap
					util_attr_mask_set_bit(resp_contents->attr_mask, i);

					//fill data
					omcimsg_util_set_attr_by_mask(resp_contents->value_first_attr, 26,
								      me_ptr->classid, i,
								      resp_contents->attr_mask,
								      &(me_ptr->attr[i].value));

					//store value;
					byte_len += meinfo_util_attr_get_byte_size(me_ptr->classid, i);
					state_ptr->last_attr_order = i;
				}
			} else {
				state_ptr->last_attr_order = i;
			}
		}

		if (cmd_num + 1 == state_ptr->total_cmds_num) {
			dbprintf(LOG_ERR, "mib upload completed, cmds=%d\n", cmd_num + 1);

			omcimsg_mib_upload_stop(state_ptr);
			omcimsg_mib_upload_after_reset_set(1);	// mark complete, so avc/alarm can send

			return OMCIMSG_ERROR_UPLOAD_COMPLETED;
		} else {
			if ((timer_id =
			     fwk_timer_create(omcimsg_handler_coretask_timer_qid,
					      OMCIMSG_MIBUPLOAD_TIMEOUT, 60000, state_ptr)) < 0) {
				dbprintf(LOG_ERR, "timer create error=%d\n", timer_id);
			}

			state_ptr->timer_id = timer_id;

			state_ptr->curr_cmd_num++;

			return 0;
		}
	}
}

int
omcimsg_mib_upload_timer_handler(struct fwk_timer_msg_t *timer_msg)
{
	struct omcimsg_mibupload_state_t *state_ptr;

	state_ptr = (struct omcimsg_mibupload_state_t *)timer_msg->user_data;

	if (state_ptr == NULL || timer_msg->timer_id != state_ptr->timer_id) {
		dbprintf(LOG_ERR, "timer id error=%d\n", state_ptr->timer_id);
		return OMCIMSG_ERROR_TIMER_ERROR;
	}

	state_ptr->timer_id = 0;

	omcimsg_mib_upload_stop(state_ptr);

	return 0;
}

int
omcimsg_mib_upload_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid, meid;
	int ret;
	struct omcimsg_mib_upload_response_t *mib_upload_response_contents;

	dbprintf(LOG_INFO, "mib_upload classid(%d)\n", omcimsg_header_get_entity_class_id(req_msg));

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	mib_upload_response_contents = (struct omcimsg_mib_upload_response_t *)resp_msg->msg_contents;

	classid = omcimsg_header_get_entity_class_id(req_msg);
	meid = omcimsg_header_get_entity_instance_id(req_msg);

	if (classid != 2 || meid != 0) {
		//send command processing error
		memset(resp_msg->msg_contents, 0x00, OMCIMSG_CONTENTS_LEN);
		dbprintf(LOG_ERR, "mib upload handler, err=%d", OMCIMSG_ERROR_PARM_ERROR);
		return OMCIMSG_ERROR_PARM_ERROR;
	}
	//call mib upload mod start to caculate total subsequent cmds
	if ((ret = omcimsg_mib_upload_start(&mibupload_state_g)) < 0) {
		//send error response
		memset(resp_msg->msg_contents, 0x00, OMCIMSG_CONTENTS_LEN);
	} else {
		//send success response
		mib_upload_response_contents->subsequent_cmd_total = htons(ret);
	}

	return 0;
}

int
omcimsg_mib_upload_next_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid, meid;
	int ret;
	struct omcimsg_mib_upload_next_t *mib_upload_next_contents;
	struct omcimsg_mib_upload_next_response_t
	*mib_upload_next_response_contents;

	dbprintf(LOG_INFO, "mib_upload_next classid(%d)\n", omcimsg_header_get_entity_class_id(req_msg));

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);
	mib_upload_next_response_contents = (struct omcimsg_mib_upload_next_response_t *)resp_msg->msg_contents;

	classid = omcimsg_header_get_entity_class_id(req_msg);
	meid = omcimsg_header_get_entity_instance_id(req_msg);

	if (classid != 2 || meid != 0) {
		//send command processing error
		memset(resp_msg->msg_contents, 0x00, OMCIMSG_CONTENTS_LEN);
		dbprintf(LOG_ERR, "mib upload next handler, err=%d", OMCIMSG_ERROR_CMD_ERROR);
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	mib_upload_next_contents = (struct omcimsg_mib_upload_next_t *)req_msg->msg_contents;

	//call mib upload mod progress to fill me attr
	ret =
	    omcimsg_mib_upload_progress(&mibupload_state_g,
					ntohs(mib_upload_next_contents->sequence_num),
					mib_upload_next_response_contents);

	switch (ret) {
	case 0:
	case OMCIMSG_ERROR_UPLOAD_COMPLETED:
		dbprintf(LOG_INFO, "mib_upload_next cmds=%d\n", ntohs(mib_upload_next_contents->sequence_num));
		break;
	default:
		dbprintf(LOG_ERR, "mib upload next err=%d\n", ret);
	}

	return 0;
}

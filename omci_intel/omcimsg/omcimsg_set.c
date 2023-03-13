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
 * File    : omcimsg_set.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "meinfo.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "util.h"
#include "env.h"
#include "er_group.h"

#define SET_ATTRS_SIZE	30

int
omcimsg_set_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(req_msg);
	unsigned short meid = omcimsg_header_get_entity_instance_id(req_msg);
	struct meinfo_t *miptr = meinfo_util_miptr(classid);

	struct me_t *me = meinfo_me_get_by_meid(classid, meid);
	struct omcimsg_set_t *req;
	struct omcimsg_set_response_t *response;
	int reason = OMCIMSG_RESULT_CMD_SUCCESS;	// default reason: success

	int next_start_byte;
	int attr_order;
	struct attr_value_t attr;
	unsigned char attr_mask[2];
	int hw_ret = 0;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	req = (struct omcimsg_set_t *) req_msg->msg_contents;
	response = (struct omcimsg_set_response_t *) resp_msg->msg_contents;

	if (miptr == NULL) {
		response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}
	miptr->access_write_mask[0] |= req->attr_mask[0];
	miptr->access_write_mask[1] |= req->attr_mask[1];
	
	if (me == NULL) {
		response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "set handler, err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

#if defined(USE_MEINFO_ME_COPY_ON_WRITE)
	if (me->refcount > 1) {
		struct me_t *me_clone;
		if (meinfo_me_copy_on_write(me, &me_clone) < 0) {
			response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			dbprintf(LOG_ERR, "set handler, err=%d (copy on write err)\n", OMCIMSG_ERROR_CMD_ERROR);
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		me = me_clone;	// change me to the clone one
	}
#endif

	attr.ptr = malloc_safe(SET_ATTRS_SIZE);	// max value possible in set

	for (next_start_byte = 0, attr_order = 1; attr_order <= miptr->attr_total; attr_order++) {
		int ret, ret2;

		if (!util_attr_mask_get_bit(req->attr_mask, attr_order))
			continue;

		if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE )
			next_start_byte += meinfo_util_attr_get_table_entry_byte_size(classid, attr_order);
		else
			next_start_byte += meinfo_util_attr_get_byte_size(classid, attr_order);		

		if (next_start_byte > SET_ATTRS_SIZE)
			break;

		ret = omcimsg_util_get_attr_by_mask(req->attr_value, SET_ATTRS_SIZE,
						  classid, attr_order, req->attr_mask, &attr);
		switch (ret) {
		case OMCIMSG_ERROR_PARM_ERROR:
			response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
			dbprintf(LOG_ERR, "set handler, err=%d\n", OMCIMSG_ERROR_PARM_ERROR);
			return OMCIMSG_ERROR_PARM_ERROR;

		case OMCIMSG_RW_OK:	// ok, validate && put value in response msg
			if (omci_env_g.mib_readonly_check == ENV_MIB_READONLY_CHECK_ERROR &&
			    !meinfo_util_attr_is_write(me->classid, attr_order)) {
				ret2 = MEINFO_ERROR_ATTR_READONLY;
			} else if (meinfo_me_attr_is_valid(me, attr_order, &attr)) {
				if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE )
					ret2=meinfo_me_attr_set_table_entry(me, attr_order, attr.ptr, 0);
				else 
					ret2=meinfo_me_attr_set(me, attr_order, &attr, 0);
			} else {
				ret2 = MEINFO_ERROR_ATTR_VALUE_INVALID;
			}
			switch (ret2) {
			case MEINFO_ERROR_ATTR_READONLY:
				reason = OMCIMSG_ERROR_CMD_NOT_SUPPORTED;;
				util_attr_mask_set_bit(response->optional_attr_mask, attr_order);
				dbprintf(LOG_ERR, "set handler, attr %d is readonly?\n", attr_order);
				break;
			case MEINFO_ERROR_ATTR_UNDEF:
				reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
				util_attr_mask_set_bit(response->optional_attr_mask, attr_order);
				dbprintf(LOG_ERR, "set handler, attr %d not support err\n", attr_order);
				break;
			case MEINFO_RW_OK:
				if (omci_env_g.mib_readonly_check == ENV_MIB_READONLY_CHECK_WANRING &&
				    !meinfo_util_attr_is_write(me->classid, attr_order)) {
					dbprintf(LOG_ERR, "classid=%u, attr=%u, write to rdonly attr?\n", me->classid, attr_order);									
				}
				break;
			default:
				reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
				util_attr_mask_set_bit(response->attr_exec_mask, attr_order);
				dbprintf(LOG_ERR, "set handler, attr %d write err\n", attr_order);
				break;
			}
			break;

		case OMCIMSG_ERROR_CMD_ERROR:
			reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
			util_attr_mask_set_bit(response->attr_exec_mask, attr_order);
			dbprintf(LOG_ERR, "set handler, attr %d cmd err\n", attr_order);
			break;

		default:	// unknown err? 
			reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
			util_attr_mask_set_bit(response->attr_exec_mask, attr_order);
			dbprintf(LOG_ERR, "set handler, attr %d unknown err\n", attr_order);
			break;
		}
	}

	attr_mask[0] = req->attr_mask[0];
	attr_mask[1] = req->attr_mask[1];

	if (miptr->callback.set_cb && (hw_ret = miptr->callback.set_cb(me, attr_mask)) < 0)
	{
		reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
		response->attr_exec_mask[0] = response->attr_exec_mask[0] | attr_mask[0];
		response->attr_exec_mask[1] = response->attr_exec_mask[1] | attr_mask[1];
		dbprintf(LOG_ERR,
			"err=%d, classid=%u, meid=%u, ret=%d, mask[0]=0x%2x, mask[1]=0x%2x\n",
			OMCIMSG_ERROR_SW_ME_SET, classid, meid, hw_ret, attr_mask[0], attr_mask[1]);
	}

	if ((omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW || omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_SWHW ))
	{
		attr_mask[0] = req->attr_mask[0];
		attr_mask[1] = req->attr_mask[1];

		if (meinfo_me_issue_self_er_attr_group(me, ER_ATTR_GROUP_HW_OP_UPDATE, attr_mask) < 0)
		{
			dbprintf(LOG_ERR,
			 "issue self er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		}

		//check me ready state related with this class
		if (meinfo_me_issue_related_er_attr_group(me->classid) < 0)
		{
			dbprintf(LOG_ERR,
			 "issue related er_attr_group error, classid=%u, instance_num=%d, meid=%u\n",
			me->classid, me->instance_num, me->meid);
		}
	}

	free_safe(attr.ptr);
	response->result_reason = reason;

	return 0;
}

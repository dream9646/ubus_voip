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
 * File    : omcimsg_get.c
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
#define GET_ATTRS_SIZE	25

int
omcimsg_get_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid = omcimsg_header_get_entity_class_id(req_msg);
	unsigned short meid = omcimsg_header_get_entity_instance_id(req_msg);
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	
	struct me_t *me;
	struct omcimsg_get_t *req;
	struct omcimsg_get_response_t *response;
	int reason = OMCIMSG_RESULT_CMD_SUCCESS;	// default reason: success

	int next_start_byte;
	int attr_order;
	struct attr_value_t attr;
	unsigned char attr_mask[2];
	int hw_ret = 0, sw_ret=0;

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	req = (struct omcimsg_get_t *) req_msg->msg_contents;
	response = (struct omcimsg_get_response_t *) resp_msg->msg_contents;

	if (miptr == NULL) {
		response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME);
		return OMCIMSG_ERROR_UNKNOWN_ME;
	}
	miptr->access_read_mask[0] |= req->attr_mask[0];
	miptr->access_read_mask[1] |= req->attr_mask[1];

	if ((me = meinfo_me_get_by_meid(classid, meid)) == NULL) {
		response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
		dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE);
		return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
	}

	if (omci_env_g.mib_rw_mode == ENV_MIB_RW_MODE_HWHW && miptr->hardware.get_hw)
	{
		attr_mask[0] = req->attr_mask[0];
		attr_mask[1] = req->attr_mask[1];
		hw_ret = miptr->hardware.get_hw(me, attr_mask);		
		
		if (hw_ret < 0) {
			dbprintf(LOG_ERR, "err=%d, classid=%u, meid=%u, ret=%d, mask[0]=0x%2x, mask[1]=0x%2x\n",
				 OMCIMSG_ERROR_HW_ME_GET, classid, meid, hw_ret, attr_mask[0], attr_mask[1]);
		}
		if (hw_ret == MEINFO_ERROR_ME_NOT_FOUND) {
			response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
			return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
		}
	}

	if (miptr->callback.get_cb)
	{
		attr_mask[0] = req->attr_mask[0];
		attr_mask[1] = req->attr_mask[1];
		sw_ret = miptr->callback.get_cb(me, attr_mask);

		if (sw_ret < 0) {
			dbprintf(LOG_ERR, "err=%d, classid=%u, meid=%u, ret=%d, mask[0]=0x%2x, mask[1]=0x%2x\n",
				 OMCIMSG_ERROR_SW_ME_GET, classid, meid, sw_ret, attr_mask[0], attr_mask[1]);
		}
		// handle specific error returned by get_cb
		if (sw_ret == MEINFO_ERROR_ME_NOT_FOUND) {
			response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
			return OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE;
		} else if (sw_ret == MEINFO_ERROR_ATTR_UNDEF) {
			// in case of error ATTR_UNDEF, the get_cb is assume to put UNDEF attr bits in attr_mask[]
			response->result_reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
			response->optional_attr_mask[0] |= attr_mask[0];
			response->optional_attr_mask[1] |= attr_mask[1];
			return OMCIMSG_ERROR_ATTR_FAILED_UNKNOWN;
		}
	}

	attr.ptr = malloc_safe(GET_ATTRS_SIZE);	// max value possible in get

	for (next_start_byte = 0, attr_order = 1; attr_order <= miptr->attr_total; attr_order++) {
		int ret;

		if (!util_attr_mask_get_bit(req->attr_mask, attr_order))
			continue;

		next_start_byte += meinfo_util_attr_get_byte_size(classid, attr_order);
		if (next_start_byte > GET_ATTRS_SIZE)
			break;

		ret = meinfo_me_attr_get(me, attr_order, &attr);

		switch (ret) {
		case MEINFO_ERROR_CLASSID_UNDEF:	// classid invalid
		case MEINFO_ERROR_CLASSID_NOT_SUPPORT:	// classid not supported
			response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME;
			dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME);
			free_safe(attr.ptr);
			return OMCIMSG_ERROR_UNKNOWN_ME;
		case MEINFO_ERROR_ME_NOT_FOUND:	// instance not found
			response->result_reason = OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE;
			dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_UNKNOWN_ME);
			free_safe(attr.ptr);
			return OMCIMSG_ERROR_UNKNOWN_ME;
		case MEINFO_ERROR_ME_PTR_NULL:	// me null
		case MEINFO_ERROR_ATTR_PTR_NULL:	// ptr null, should not happen
			response->result_reason = OMCIMSG_RESULT_CMD_ERROR;
			dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_CMD_ERROR);
			free_safe(attr.ptr);
			return OMCIMSG_ERROR_CMD_ERROR;

		case MEINFO_RW_OK:	// ok, put value in response msg
			if (hw_ret == 0 || !util_attr_mask_get_bit(attr_mask, attr_order))
			{
				if (omcimsg_util_set_attr_by_mask
			    		(response->attr_value, GET_ATTRS_SIZE, classid, attr_order, req->attr_mask, &attr) == 0) {
					util_attr_mask_set_bit(response->attr_mask, attr_order);
				} else {
					reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
					util_attr_mask_set_bit(response->attr_exec_mask, attr_order);	// mem err?
					dbprintf(LOG_ERR, "attr %d mem err \n", attr_order);
				}
			} else {
				reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
				util_attr_mask_set_bit(response->attr_exec_mask, attr_order);
				dbprintf(LOG_ERR, "attr %d get_hw error \n", attr_order);
			}
			break;

		case MEINFO_ERROR_ATTR_UNDEF:	// attr order invalid
			reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
			util_attr_mask_set_bit(response->optional_attr_mask, attr_order);
			dbprintf(LOG_ERR, "attr %d invalid \n", attr_order);
			break;
		case MEINFO_ERROR_ATTR_INVALID_ACTION_ON_TABLE:	// do get on table?
			if (omcimsg_util_check_attr_bit_exclusive(req->attr_mask, attr_order) == 1) {
				int size;
				
				if ((size = omcimsg_get_table_start(me, attr_order)) >= 0) {
					util_bitmap_set_value(response->attr_value, GET_ATTRS_SIZE * 8 , 0, 4 * 8, size);
					util_attr_mask_set_bit(response->attr_mask, attr_order);
					response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;
				} else {
					response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
					dbprintf(LOG_ERR, "err=%d\n", OMCIMSG_ERROR_PARM_ERROR);
					return OMCIMSG_RESULT_PARM_ERROR;
				}
				
				free_safe(attr.ptr);				
				return OMCIMSG_RESULT_CMD_SUCCESS;
			} 
			reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
			util_attr_mask_set_bit(response->attr_exec_mask, attr_order);
			dbprintf(LOG_ERR, "attr %d get table with other attr err\n", attr_order);
			break;

		default:	// unknown err? 
			reason = OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN;
			util_attr_mask_set_bit(response->attr_exec_mask, attr_order);
			dbprintf(LOG_ERR, "attr %d unknown error \n", attr_order);
			break;
		}
	}

	free_safe(attr.ptr);
	response->result_reason = reason;

	/*send an event to pmtask to check this me's tca*/
	if (meinfo_util_get_action(classid) & MEINFO_ACTION_MASK_GET_CURRENT_DATA) {
		omcimsg_handler_coretask_send_tca_determination_pmtask(classid, meid);
	}

	return 0;
}

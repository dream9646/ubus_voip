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
 * File    : omcimsg.c
 *
 ******************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "meinfo.h"
#include "omcimsg.h"
#include "omcimsg_handler.h"
#include "util.h"

struct omcimsg_type_attr_t omcimsg_type_attr[32] = {
	{0, "Reserved 0", 0, 0, 0, 0, 0},	//0 reserved
	{0, "Reserved 1", 0, 0, 0, 0, 0},	//1 reserved
	{0, "Reserved 2", 0, 0, 0, 0, 0},	//2 reserved
	{0, "Reserved 3", 0, 0, 0, 0, 0},	//3 reserved
	{1, "Create", 1, 1, 1, 1, 0},	//4
	{0, "Create complete connection (deprecated)", 0, 0, 0, 0, 0},	//5 Deprecate
	{1, "Delete", 1, 1, 1, 1, 0},	//6
	{0, "Delete complete connection (deprecated)", 0, 0, 0, 0, 0},	//7 Deprecate
	{1, "Set", 1, 1, 1, 1, 0},	//8
	{1, "Get", 1, 0, 0, 1, 0},	//9
	{0, "Get complete connection (deprecated)", 0, 0, 0, 0, 0},	//10 Deprecate
	{1, "Get all alarms", 1, 0, 0, 0, 0},	//11
	{1, "Get all alarms next", 1, 0, 0, 0, 0},	//12
	{1, "MIB upload", 1, 0, 0, 0, 0},	//13
	{1, "MIB upload next", 1, 0, 0, 0, 0},	//14
	{1, "MIB reset", 1, 0, 1, 1, 0},	//15
	{1, "Alarm", 0, 0, 0, 0, 1},	//16
	{1, "Attribute value change", 0, 0, 0, 0, 1},	//17
	{1, "Test", 1, 0, 0, 1, 0},	//18
	{1, "Start software download", 1, 1, 1, 1, 0},	//19
	{1, "Download section", 2, 0, 1, 1, 0},	//20
	{1, "End software download", 1, 1, 1, 1, 0},	//21
	{1, "Activate software", 1, 1, 1, 1, 0},	//22
	{1, "Commit software", 1, 1, 1, 1, 0},	//23
	{1, "Synchronize Time", 1, 0, 1, 1, 0},	//24
	{1, "Reboot", 1, 0, 1, 1, 0},	//25
	{1, "Get next", 1, 0, 0, 1, 0},	//26
	{1, "Test result", 0, 0, 0, 0, 1},	//27
	{1, "Get current data", 1, 0, 0, 1, 0},	//28
	{1, "Set table", 1, 1, 1, 1, 0},	//29
	{0, "Reserved 30", 0, 0, 0, 0, 0},	//30 reserved
	{0, "Reserved 31", 0, 0, 0, 0, 0}		//31 reserved
};

// store omcimsg counters
unsigned int omcimsg_count_g[32];

int
omcimsg_util_get_attr_from_create(void *attrs, unsigned short classid,
				  unsigned char attr_order, struct attr_value_t *attr)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	unsigned short start_byte = 0;
	unsigned short byte_size = meinfo_util_attr_get_byte_size(classid, attr_order);
	int i;

	if (miptr==NULL) {
		dbprintf(LOG_ERR,
			 "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "err=%d, classid=%u, attr_order=%u\n",
			 MEINFO_ERROR_ATTR_UNDEF, classid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	for (i = 1; i < attr_order; i++) {
		if (meinfo_util_attr_is_sbc(classid, i)) {
			start_byte += meinfo_util_attr_get_byte_size(classid, i);
		}
	}

	switch (meinfo_util_attr_get_format(classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		// the util_bitmap_get function  will convert value from network order to host order
		attr->data = util_bitmap_get_value(attrs, 32 * 8, start_byte * 8, byte_size * 8);
		break;
	case ATTR_FORMAT_BIT_FIELD:	//not sure
	case ATTR_FORMAT_STRING:
		if (attr->ptr) {
			memcpy(attr->ptr, attrs + start_byte, byte_size);
			//((char*)attr->ptr)[byte_size-1]=0;    // terminate str
		} else {
			dbprintf(LOG_ERR, "value get error because attr value ptr null\n");
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		break;
	case ATTR_FORMAT_TABLE:
		dbprintf(LOG_INFO, "value get ignored bcause format is table\n");
		return OMCIMSG_ERROR_PARM_ERROR;
	}
	return 0;
}

int
omcimsg_util_get_attr_by_mask(void *attrs, int attrs_byte_size,
			      unsigned short classid,
			      unsigned char attr_order, unsigned char mask[2], struct attr_value_t *attr)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	unsigned short start_byte = 0;
	unsigned short byte_size;
	int i;

	if (miptr==NULL) {
		dbprintf(LOG_ERR,
			 "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "err=%d, classid=%u, attr_order=%u\n",
			 MEINFO_ERROR_ATTR_UNDEF, classid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	if (meinfo_util_attr_get_format(classid, attr_order) == ATTR_FORMAT_TABLE )
		byte_size = meinfo_util_attr_get_table_entry_byte_size(classid, attr_order);
	else
		byte_size = meinfo_util_attr_get_byte_size(classid, attr_order);		

	for (i = 1; i < attr_order; i++) {
		if (util_attr_mask_get_bit(mask, i)) {
			if (meinfo_util_attr_get_format(classid, i) == ATTR_FORMAT_TABLE )
				start_byte += meinfo_util_attr_get_table_entry_byte_size(classid, i);
			else
				start_byte += meinfo_util_attr_get_byte_size(classid, i);
		}
	}

	if (start_byte + byte_size > attrs_byte_size) {
		dbprintf(LOG_ERR, "value set error because out of attrs byte size\n");
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	switch (meinfo_util_attr_get_format(classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		// the util_bitmap_get function  will convert value from network order to host order
		attr->data = util_bitmap_get_value(attrs, 32 * 8, start_byte * 8, byte_size * 8);
		break;
	case ATTR_FORMAT_BIT_FIELD:	//not sure
	case ATTR_FORMAT_STRING:
	case ATTR_FORMAT_TABLE:
		if (attr->ptr) {
			memcpy(attr->ptr, attrs + start_byte, byte_size);
			//((char*)attr->ptr)[byte_size-1]=0;    // terminate str
		} else {
			dbprintf(LOG_ERR, "value get error because attr value ptr null\n");
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		break;
	}
	return 0;
}

int
omcimsg_util_set_attr_by_mask(void *attrs, int attrs_byte_size,
			      unsigned short classid,
			      unsigned char attr_order, unsigned char mask[2], struct attr_value_t *attr)
{
	struct meinfo_t *miptr=meinfo_util_miptr(classid);
	unsigned short start_byte = 0;
	unsigned short byte_size = meinfo_util_attr_get_byte_size(classid, attr_order);
	int i;

	if (miptr==NULL) {
		dbprintf(LOG_ERR,
			 "err=%d, classid=%u\n", MEINFO_ERROR_CLASSID_UNDEF, classid);
		return MEINFO_ERROR_CLASSID_UNDEF;	//class_id out of range
	}

	if (attr_order > miptr->attr_total) {
		dbprintf(LOG_ERR, "err=%d, classid=%u, attr_order=%u\n",
			 MEINFO_ERROR_ATTR_UNDEF, classid, attr_order);
		return (MEINFO_ERROR_ATTR_UNDEF);
	}

	for (i = 1; i < attr_order; i++) {
		if (util_attr_mask_get_bit(mask, i)) {
			if (meinfo_util_attr_get_format(classid, i) == ATTR_FORMAT_TABLE )
				start_byte += meinfo_util_attr_get_table_entry_byte_size(classid, i);
			else
				start_byte += meinfo_util_attr_get_byte_size(classid, i);
		}
	}
	if (start_byte + byte_size > attrs_byte_size) {	// total bytes > attrs limit
		dbprintf(LOG_ERR, "value set error because out of attrs byte size\n");
		return OMCIMSG_ERROR_CMD_ERROR;
	}

	switch (meinfo_util_attr_get_format(classid, attr_order)) {
	case ATTR_FORMAT_POINTER:
	case ATTR_FORMAT_UNSIGNED_INT:
	case ATTR_FORMAT_ENUMERATION:
	case ATTR_FORMAT_SIGNED_INT:
		// the util_bitmap_set function  will convert value from host order to network order
		util_bitmap_set_value(attrs, 32 * 8, start_byte * 8, byte_size * 8, attr->data);
		break;
	case ATTR_FORMAT_BIT_FIELD:
	case ATTR_FORMAT_STRING:
	case ATTR_FORMAT_TABLE:
		if (attr->ptr) {
			memcpy(attrs + start_byte, attr->ptr, byte_size);
			//((char*)attrs)[start_byte+byte_size-1]=0;     // terminate str
		} else {
			dbprintf(LOG_ERR, "value set error because attr value ptr null\n");
			return OMCIMSG_ERROR_CMD_ERROR;
		}
		break;
	}
	return 0;
}

//return value, 0: low priority, 1: high priority
unsigned int
omcimsg_util_get_msg_priority(struct omcimsg_layout_t *msg)
{
	// the util_bitmap_get function  will convert value from network order to host order
	return util_bitmap_get_value((unsigned char *) msg, 16, 0, 1);
}

//0: not, 1: yes
int
omcimsg_util_is_noack_download_section_msg(struct omcimsg_layout_t *msg)
{
	if (msg == NULL) {
		return 0;
	}

	if ((omcimsg_header_get_type(msg) == OMCIMSG_DOWNLOAD_SECTION)
	    && !omcimsg_header_get_flag_ar(msg)
	    && !omcimsg_header_get_flag_ak(msg)) {
		return 1;
	}

	return 0;
}

int
omcimsg_util_fill_resp_header_by_orig(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	if (req_msg == NULL || resp_msg == NULL) {
		return OMCIMSG_ERROR_NOMEM;
	}

	memcpy(resp_msg, req_msg, OMCIMSG_HEADER_LEN);
	omcimsg_header_set_flag_ar(resp_msg, 0);
	omcimsg_header_set_flag_ak(resp_msg, 1);

	return 0;
}

int
omcimsg_util_fill_preliminary_error_resp(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg, int reason)
{
	if (req_msg == NULL || resp_msg == NULL) {
		return OMCIMSG_ERROR_NOMEM;
	}

	omcimsg_util_fill_resp_header_by_orig(req_msg, resp_msg);

	resp_msg->msg_contents[0] = reason;

	if (omcimsg_header_get_type(resp_msg) == OMCIMSG_GET_ALL_ALARMS_NEXT ||
	    omcimsg_header_get_type(resp_msg) == OMCIMSG_MIB_UPLOAD_NEXT) {
		memset(resp_msg->msg_contents, 0x00, OMCIMSG_CONTENTS_LEN);
	}

	return 0;
}

//0: no error, <0: error
int
omcimsg_util_check_err_for_header(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	unsigned short classid;
	int type;
	struct meinfo_t *miptr;

	if (req_msg == NULL || resp_msg == NULL)
		return -1;

	type = omcimsg_header_get_type(req_msg);
	classid = omcimsg_header_get_entity_class_id(req_msg);
	miptr = meinfo_util_miptr(classid);
	
	if (omcimsg_header_get_flag_db(req_msg)) {
		omcimsg_util_fill_preliminary_error_resp(req_msg, resp_msg, OMCIMSG_RESULT_PARM_ERROR);
		dbprintf(LOG_ERR, "omcimsg with db ==1?\n");
		return -1;
        }
	
	if (!omcimsg_type_attr[type].supported) {
		omcimsg_util_fill_preliminary_error_resp(req_msg, resp_msg, OMCIMSG_RESULT_CMD_NOT_SUPPORTED);
		dbprintf(LOG_ERR, "not supported msgtype(%s)?\n", omcimsg_type_attr[type].name);
		return -1;
	}

	if (omcimsg_header_get_device_id(req_msg) != OMCIMSG_DEVICE_ID && omcimsg_header_get_device_id(req_msg) != OMCIMSG_EXTENDED_DEVICE_ID ) {
		omcimsg_util_fill_preliminary_error_resp(req_msg, resp_msg, OMCIMSG_RESULT_CMD_ERROR);
		dbprintf(LOG_ERR, "wrong device id (%d)?\n", omcimsg_header_get_device_id(req_msg));
		return -1;
	}

	if (miptr==NULL || miptr->config.is_inited == ME_SUPPORT_TYPE_NOT_SUPPORTED) {
		omcimsg_util_fill_preliminary_error_resp(req_msg, resp_msg, OMCIMSG_RESULT_UNKNOWN_ME);
		dbprintf(LOG_ERR, "me class error(%d)?\n", classid);
		return -1;
	}

	return 0;
}

int omcimsg_util_check_attr_bit_exclusive(unsigned char mask[2], unsigned char attr_order)
{
	int i;
	
	for (i = 1; i < MEINFO_ATTR_ORDER_MAX; i++) {
		if (i != attr_order && util_attr_mask_get_bit(mask, i) == 1) {
			return 0;
		}
	}

	return 1;
}

char *
omcimsg_util_classid2name(unsigned short classid)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid);
	char *name=NULL;	

	if (miptr)
		name=miptr->name;
	if (name==NULL)
		return "proprietary class?";
	return name;
}
		
char *
omcimsg_util_classid2section(unsigned short classid)
{
	struct meinfo_t *miptr = meinfo_util_miptr(classid);
	char *section=NULL;

	if (miptr)
		section=miptr->section;
	if (section==NULL)
		return "proprietary?";
	return section;
}
		
char *
omcimsg_util_msgtype2name(unsigned char msgtype)
{
	char *name=omcimsg_type_attr[msgtype&0x1f].name;
	if (name==NULL)
		return "unknow action";
	return name;
}

int
omcimsg_util_msgtype_is_write(unsigned char msgtype)
{
 	return  omcimsg_type_attr[msgtype&0x1f].is_write;
}

int
omcimsg_util_msgtype_is_inc_mibdatasync(unsigned char msgtype)
{
 	return  omcimsg_type_attr[msgtype&0x1f].inc_mib_data_sync;
}

int
omcimsg_util_msgtype_is_notification(unsigned char msgtype)
{
 	return  omcimsg_type_attr[msgtype&0x1f].is_notification;
}

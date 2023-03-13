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
 * Module  : omcidump
 * File    : omcidump.c
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
#include "omcidump.h"

static void
omcidump_rawheader(int fd, struct omcimsg_layout_t *msg)
{
	unsigned char msgtype = omcimsg_header_get_type(msg);
	unsigned char db = omcimsg_header_get_flag_db(msg);
	unsigned char ar = omcimsg_header_get_flag_ar(msg);
	unsigned char ak = omcimsg_header_get_flag_ak(msg);

	util_fdprintf(fd, "tid=0x%.4x(%u), mt=0x%.2X(",
		omcimsg_header_get_transaction_id(msg), 
		omcimsg_header_get_transaction_id(msg), 
		msg->msg_type);
	if (db)
		util_fdprintf(fd, "db,");
	if (ar)
		util_fdprintf(fd, "ar,");
	if (ak)
		util_fdprintf(fd, "ak,");
	if (omcimsg_type_attr[msgtype].name) {
		util_fdprintf(fd, " %s", omcimsg_type_attr[msgtype].name);
	} else {
		util_fdprintf(fd, " 0x%x", msgtype);
	}
	util_fdprintf(fd, "), devid=0x%.2X, classid=0x%x(%u), meid=0x%x(%u)\n",
		msg->device_id_type, 
		omcimsg_header_get_entity_class_id(msg),
		omcimsg_header_get_entity_class_id(msg),
		omcimsg_header_get_entity_instance_id(msg),
		omcimsg_header_get_entity_instance_id(msg));
}

void
omcidump_header(int fd, struct omcimsg_layout_t *msg)
{
	unsigned short classid=omcimsg_header_get_entity_class_id(msg);
	struct meinfo_t *miptr = meinfo_util_miptr(classid);

	unsigned char msgtype = omcimsg_header_get_type(msg);
	unsigned char db = omcimsg_header_get_flag_db(msg);
	unsigned char ar = omcimsg_header_get_flag_ar(msg);
	unsigned char ak = omcimsg_header_get_flag_ak(msg);

	if (omcimsg_type_attr[msgtype].name) {
		util_fdprintf(fd, " %s", omcimsg_type_attr[msgtype].name);
	} else {
		util_fdprintf(fd, " 0x%x", msgtype);
	}

	if (db)
		util_fdprintf(fd, ", db");
	if (ar)
		util_fdprintf(fd, ", ar");
	if (ak)
		util_fdprintf(fd, ", ak");
	util_fdprintf(fd, "\n");

	util_fdprintf(fd, "tid: 0x%.4x(%u)\n",
		omcimsg_header_get_transaction_id(msg), 
		omcimsg_header_get_transaction_id(msg));

	util_fdprintf(fd, "classid: 0x%x(%u), %s\n",
		omcimsg_header_get_entity_class_id(msg),
		omcimsg_header_get_entity_class_id(msg),
		(miptr&&miptr->name) ? miptr->name : "?");

	util_fdprintf(fd, "meid: 0x%x(%u)\n",
		omcimsg_header_get_entity_instance_id(msg),
		omcimsg_header_get_entity_instance_id(msg));
}

char *
omcidump_reason_str(unsigned char reason)
{
	static char buff[64];

	switch (reason) {
		case 0:	return "success";
		case 1:	return "cmd error";
		case 2:	return "cmd not supported";
		case 3:	return "parameter error";
		case 4:	return "unknown me classid";
		case 5:	return "unknown me instance";
		case 6:	return "device busy";
		case 7: return "instance exists";
		case 9:	return "attr failed or unknown";
	}
	snprintf(buff, 64, "%d, unknown reason code", reason);
	return buff;
}

void
omcidump_print_raw_with_mark(int fd, struct omcimsg_layout_t *msg, unsigned char dir, unsigned int mark_offset, unsigned mark_len)
{
	unsigned char *m=(unsigned char*)msg;
	int i;

	util_fdprintf(fd, "%s[%u.%.6u]:\n", dir == OMCIDUMP_DIR_RX ? "OMCCRX" : "OMCCTX", (unsigned int) msg->timestamp.tv_sec, (unsigned int) msg->timestamp.tv_usec);

	// header
	omcidump_rawheader(fd, msg);
	// content
	for (i = 0; i < 48; i++) {
		util_fdprintf(fd, "%02X", m[i]);
		if (mark_len>0 && i+1==mark_offset) {
			util_fdprintf(fd, "[");
		} else if (mark_len>0 && i+1==mark_offset+mark_len) {
			util_fdprintf(fd, "]");
		} else if (mark_len>0 && i+1>mark_offset && i+1<mark_offset+mark_len) {
			util_fdprintf(fd, "-");
		} else {
			util_fdprintf(fd, " ");
		}
		if ((i + 1) % 16 == 0)
			util_fdprintf(fd, "\n");
	}
	// space line
	util_fdprintf(fd, "\n");
}

void
omcidump_print_raw(int fd, struct omcimsg_layout_t *msg, unsigned char dir)
{
	omcidump_print_raw_with_mark(fd, msg, dir, 0, 0);
}

// store pkt sent by olt in minimum format
int
omcidump_print_raw_to_file(struct omcimsg_layout_t *msg, char *filename, unsigned char dir)
{
	unsigned char msgtype = omcimsg_header_get_type(msg);
	unsigned char *m=(unsigned char*)msg;
	FILE *fp;
	int i;

	if ((fp = fopen(filename, "a")) == NULL)
		return -1;

	if (msgtype==OMCIMSG_MIB_RESET)
		fprintf(fp, "\n");

	fprintf(fp, "%s[%u.%.6u]:\n", dir == OMCIDUMP_DIR_RX ? "OMCCRX" : "OMCCTX", (unsigned int) msg->timestamp.tv_sec, (unsigned int) msg->timestamp.tv_usec);

	// save without omci tailer to minimize log size
	// tailer will be regenerated in by oltsim in log replay
	for (i = 0; i < OMCIMSG_LEN-OMCIMSG_TRAILER_LEN; i++) {
		fprintf(fp, "%02X ", m[i]);
		if ((i + 1) % 16 == 0)
			fprintf(fp, "\n");
	}
	fprintf(fp, "\n");

	fclose(fp);
	return 0;
}

void
omcidump_print_msg(int fd, struct omcimsg_layout_t *msg, unsigned char dir)
{
	unsigned char msgtype = omcimsg_header_get_type(msg);
	unsigned char ar = omcimsg_header_get_flag_ar(msg);

	util_fdprintf(fd, "%s[%u.%.6u]:", dir == OMCIDUMP_DIR_RX ? "MsgRX" : "MsgTX", (unsigned int) msg->timestamp.tv_sec, (unsigned int) msg->timestamp.tv_usec);

	switch (msgtype) {
	case OMCIMSG_CREATE:
		if (ar)
			omcidump_create(fd, msg);
		else
			omcidump_create_response(fd, msg);
		break;
	case OMCIMSG_DELETE:
		if (ar)
			omcidump_delete(fd, msg);
		else
			omcidump_delete_response(fd, msg);
		break;
	case OMCIMSG_SET:
	case OMCIMSG_SET_TABLE:
		if (ar)
			omcidump_set(fd, msg);
		else
			omcidump_set_response(fd, msg);
		break;
	case OMCIMSG_GET:
		if (ar)
			omcidump_get(fd, msg);
		else
			omcidump_get_response(fd, msg);
		break;
	case OMCIMSG_GET_ALL_ALARMS:
		if (ar)
			omcidump_get_all_alarms(fd, msg);
		else
			omcidump_get_all_alarms_response(fd, msg);
		break;
	case OMCIMSG_GET_ALL_ALARMS_NEXT:
		if (ar)
			omcidump_get_all_alarms_next(fd, msg);
		else
			omcidump_get_all_alarms_next_response(fd, msg);
		break;
	case OMCIMSG_MIB_UPLOAD:
		if (ar)
			omcidump_mib_upload(fd, msg);
		else
			omcidump_mib_upload_response(fd, msg);
		break;
	case OMCIMSG_MIB_UPLOAD_NEXT:
		if (ar)
			omcidump_mib_upload_next(fd, msg);
		else
			omcidump_mib_upload_next_response(fd, msg);
		break;
	case OMCIMSG_MIB_RESET:
		if (ar)
			omcidump_mib_reset(fd, msg);
		else
			omcidump_mib_reset_response(fd, msg);
		break;
	case OMCIMSG_SYNCHRONIZE_TIME:	
		if (ar)
			omcidump_synchronize_time(fd, msg);
		else
			omcidump_synchronize_time_response(fd, msg);
		break;
	case OMCIMSG_GET_NEXT:
		if (ar)
			omcidump_get_next(fd, msg);
		else
			omcidump_get_next_response(fd, msg);
		break;
	case OMCIMSG_GET_CURRENT_DATA:
		if (ar)
			omcidump_get_current_data(fd, msg);
		else
			omcidump_get_current_data_response(fd, msg);
		break;
	case OMCIMSG_ALARM:
		omcidump_alarm(fd, msg);
		break;
	case OMCIMSG_ATTRIBUTE_VALUE_CHANGE:
		omcidump_avc(fd, msg);
		break;
	case OMCIMSG_REBOOT:
		if (ar)
			omcidump_reboot(fd, msg);
		else
			omcidump_reboot_response(fd, msg);
		break;
	case OMCIMSG_TEST:
		if (ar)
			omcidump_test(fd, msg);
		else
			omcidump_test_response(fd, msg);
		break;		
	case OMCIMSG_TEST_RESULT:
		omcidump_test_result(fd, msg);
		break;	
	case OMCIMSG_START_SOFTWARE_DOWNLOAD:
		if (ar)
			omcidump_sw_download(fd, msg);
		else
			omcidump_sw_download_response(fd, msg);
		break;
	case OMCIMSG_DOWNLOAD_SECTION:	
		if (omcimsg_header_get_flag_ak(msg) == 0)
			omcidump_download_section(fd, msg);
		else
			omcidump_download_section_response(fd, msg);
		break;
	case OMCIMSG_END_SOFTWARE_DOWNLOAD:
		if (ar)
			omcidump_end_sw_download(fd, msg);
		else
			omcidump_end_sw_download_response(fd, msg);
		break;
	case OMCIMSG_ACTIVATE_SOFTWARE:
		if (ar)
			omcidump_activate_image(fd, msg);
		else
			omcidump_activate_image_response(fd, msg);
		break;
	case OMCIMSG_COMMIT_SOFTWARE:
		if (ar)
			omcidump_commit_image(fd, msg);
		else
			omcidump_commit_image_response(fd, msg);
		break;

	default:
		dbprintf(LOG_ERR, "dump msgtype 0x%x(%d) unknown\n", msgtype, msgtype);
	}
}

void
omcidump_print_line(int fd, struct omcimsg_layout_t *msg, unsigned char dir, int ms_diff)
{
	unsigned short tid=omcimsg_header_get_transaction_id(msg);
	unsigned char msgtype = omcimsg_header_get_type(msg);
	unsigned char ak = omcimsg_header_get_flag_ak(msg);
	unsigned short classid=omcimsg_header_get_entity_class_id(msg);
	unsigned short meid=omcimsg_header_get_entity_instance_id(msg);
	unsigned char result_reason = msg->msg_contents[0];

	util_fdprintf(fd, "%s%c0x%.4x", 
		dir == OMCIDUMP_DIR_RX? "R" : "T", 
		(ak && omcimsg_type_attr[msgtype].result_reason && msg->msg_contents[0]!=0)?'-':' ', tid);

	if (omcimsg_type_attr[msgtype].name) {
		util_fdprintf(fd, ", %s:", omcimsg_type_attr[msgtype].name);
	} else {
		util_fdprintf(fd, " 0x%x:", msgtype);
	}	

	util_fdprintf(fd, " %u %s, me 0x%x(%u)", 
		classid, omcimsg_util_classid2name(classid), meid, meid);

	if (ak && omcimsg_type_attr[msgtype].result_reason && result_reason!=0)
		util_fdprintf(fd, " - %s", omcidump_reason_str(result_reason));

	if (dir == OMCIDUMP_DIR_TX) {
		util_fdprintf(fd, " %lu.%03lus [%d]\n", msg->timestamp.tv_sec, msg->timestamp.tv_usec/1000, ms_diff);
	} else {
		util_fdprintf(fd, " %lu.%03lus\n", msg->timestamp.tv_sec, msg->timestamp.tv_usec/1000);	
	}
}

void
omcidump_print_char(int fd, struct omcimsg_layout_t *msg, unsigned char dir)
{
	unsigned char msgtype = omcimsg_header_get_type(msg);
	unsigned char ak = omcimsg_header_get_flag_ak(msg);

	if (ak) {
		util_fdprintf(fd, "%c",
			(msg->msg_contents[0]!=0)?'-':'.');
		return;
	}
	
	if (msgtype == OMCIMSG_REBOOT || msgtype == OMCIMSG_MIB_RESET)
		util_fdprintf(fd, "\n");
		
	switch (msgtype) {
	case OMCIMSG_CREATE:
		util_fdprintf(fd, "c");
		break;
	case OMCIMSG_DELETE:
		util_fdprintf(fd, "d");
		break;
	case OMCIMSG_SET:
		util_fdprintf(fd, "s");
		break;
	case OMCIMSG_GET:
		util_fdprintf(fd, "g");
		break;
	case OMCIMSG_GET_ALL_ALARMS:
		util_fdprintf(fd, "a");
		break;
	case OMCIMSG_GET_ALL_ALARMS_NEXT:
		util_fdprintf(fd, "a");
		break;
	case OMCIMSG_MIB_UPLOAD:
	case OMCIMSG_MIB_UPLOAD_NEXT:
		util_fdprintf(fd, "u");
		break;
	case OMCIMSG_MIB_RESET:
		util_fdprintf(fd, "z");
		break;
	case OMCIMSG_SYNCHRONIZE_TIME:	
		util_fdprintf(fd, "y");
		break;
	case OMCIMSG_GET_NEXT:
	case OMCIMSG_GET_CURRENT_DATA:
		util_fdprintf(fd, "g");
		break;
	case OMCIMSG_ALARM:
		util_fdprintf(fd, "A");
		break;
	case OMCIMSG_ATTRIBUTE_VALUE_CHANGE:
		util_fdprintf(fd, "V");
		break;
	case OMCIMSG_REBOOT:
		util_fdprintf(fd, "b");
		break;
	case OMCIMSG_TEST:
		util_fdprintf(fd, "t");
		break;	
	case OMCIMSG_TEST_RESULT:
		util_fdprintf(fd, "T");
		break;	
	case OMCIMSG_START_SOFTWARE_DOWNLOAD:
	case OMCIMSG_DOWNLOAD_SECTION:
	case OMCIMSG_END_SOFTWARE_DOWNLOAD:
		util_fdprintf(fd, "l");
		break;
	case OMCIMSG_ACTIVATE_SOFTWARE:
	case OMCIMSG_COMMIT_SOFTWARE:
		util_fdprintf(fd, "i");
		break;
	default:
		util_fdprintf(fd, "?");
	}
}

void
omcidump_print_exec_diff(int fd, int ms_diff)
{
	util_fdprintf(fd, "==Exec time: %dms\n\n", ms_diff);
}


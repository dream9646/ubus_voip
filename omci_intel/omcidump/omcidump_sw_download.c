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
 * File    : omcidump_sw_download.c
 *
 ******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "util.h"
#include "omcidump.h"
//#include "omcimsg_handler.h"

struct instance_id_t {
	unsigned char ms;
	unsigned char ls;
};

int 
omcidump_sw_download(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_start_software_download_t *start_sw_download;
	
	omcidump_header(fd, msg);
	start_sw_download = (struct omcimsg_start_software_download_t *) msg->msg_contents;

	util_fdprintf(fd, "Window size -1 (%d)\n", start_sw_download->window_size_minus1);
	util_fdprintf(fd, "Image size in bytes (%u)\n", ntohl(start_sw_download->image_size_in_bytes));
	util_fdprintf(fd, "Number of circuit packs to be updated in parallel(1..9) %d\n", start_sw_download->number_of_update);
#if 0
	//we may skip follow information in current version (value maybe 1,1, 0..1)
	start_sw_download->target[0].image_slot;
	start_sw_download->target[0].image_instance; */
#endif
	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_sw_download_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_start_software_download_response_t *start_sw_download_response;

	omcidump_header(fd, msg);
	start_sw_download_response = (struct omcimsg_start_software_download_response_t *) msg->msg_contents;

	util_fdprintf(fd, "Window size -1 (%d)\n", start_sw_download_response->window_size_minus1);
	util_fdprintf(fd, "Number of instances responding(0..9)(%d)\n", start_sw_download_response->number_of_responding);
	util_fdprintf(fd, "result: %d, %s\n", start_sw_download_response->result_reason, 
		omcidump_reason_str(start_sw_download_response->result_reason));
	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_download_section(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_download_section_t *download_section;
	unsigned char flag_need_ack;

	omcidump_header(fd, msg);
	flag_need_ack=omcimsg_header_get_flag_ar(msg);
	download_section = (struct omcimsg_download_section_t *)msg->msg_contents;

	util_fdprintf(fd, "AR=%d\n",flag_need_ack);
	util_fdprintf(fd, "Download section mumber %d\n", download_section->download_section_num);
	util_fdprintf(fd, "\n");
	return 0;

}

int 
omcidump_download_section_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_download_section_response_t *download_section_response;

	omcidump_header(fd, msg);
	download_section_response = (struct omcimsg_download_section_response_t *)msg->msg_contents;
	util_fdprintf(fd, "Download section mumber %d\n", download_section_response->download_section_num);
	util_fdprintf(fd, "result: %d, %s\n", download_section_response->result_reason, 
		omcidump_reason_str(download_section_response->result_reason));
	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_end_sw_download(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_end_software_download_t *end_software_download;

	omcidump_header(fd, msg);
	end_software_download = (struct omcimsg_end_software_download_t *)msg->msg_contents;
	util_fdprintf(fd, "CRC-32=%x\n", ntohl(end_software_download->crc_32));
	util_fdprintf(fd, "Image size in bytes=%u\n", ntohl(end_software_download->image_size_in_bytes));
	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_end_sw_download_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_end_software_download_response_t *end_software_download_response;

	omcidump_header(fd, msg);
	end_software_download_response = (struct omcimsg_end_software_download_response_t *)msg->msg_contents;
	util_fdprintf(fd, "result: %d, %s\n", end_software_download_response->result_reason,
		omcidump_reason_str(end_software_download_response->result_reason));
	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_activate_image(int fd, struct omcimsg_layout_t *msg)
{
	//struct omcimsg_activate_image_t *activate_image;
	struct instance_id_t *instance_id;

	omcidump_header(fd, msg);
	instance_id = (struct instance_id_t *) &msg->entity_instance_id;
	if (instance_id->ms == 0)
		util_fdprintf(fd, "Target: ONT-G\n");
	else
		util_fdprintf(fd, "Slot=%d\n", instance_id->ms);

	if (instance_id->ls == 0)
		util_fdprintf(fd, "first instance\n");
	else
		util_fdprintf(fd, "second instance\n");

	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_activate_image_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_activate_image_response_t *activate_image_response;
	struct instance_id_t *instance_id;

	activate_image_response = (struct omcimsg_activate_image_response_t *)msg->msg_contents;
	omcidump_header(fd, msg);
	instance_id = (struct instance_id_t *) &msg->entity_instance_id;
	if (instance_id->ms == 0)
		util_fdprintf(fd, "Target: ONT-G\n");
	else
		util_fdprintf(fd, "Slot=%d\n", instance_id->ms);

	if (instance_id->ls == 0)
		util_fdprintf(fd, "first instance\n");
	else
		util_fdprintf(fd, "second instance\n");

	util_fdprintf(fd, "result: %d, %s\n", activate_image_response->result_reason, 
		omcidump_reason_str(activate_image_response->result_reason));
	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_commit_image(int fd, struct omcimsg_layout_t *msg)
{
	//struct omcimsg_commit_image_t *commit_image;
	struct instance_id_t *instance_id;

	omcidump_header(fd, msg);
	instance_id = (struct instance_id_t *) &msg->entity_instance_id;
	if (instance_id->ms == 0)
		util_fdprintf(fd, "Target: ONT-G\n");
	else
		util_fdprintf(fd, "Slot=%d\n", instance_id->ms);

	if (instance_id->ls == 0)
		util_fdprintf(fd, "first instance\n");
	else
		util_fdprintf(fd, "second instance\n");

	util_fdprintf(fd, "\n");
	return 0;
}

int 
omcidump_commit_image_response(int fd, struct omcimsg_layout_t *msg)
{
	struct omcimsg_commit_image_response_t *commit_image_response;
	struct instance_id_t *instance_id;

	commit_image_response = (struct omcimsg_commit_image_response_t *)msg->msg_contents;

	omcidump_header(fd, msg);
	instance_id = (struct instance_id_t *) &msg->entity_instance_id;
	if (instance_id->ms == 0)
		util_fdprintf(fd, "Target: ONT-G\n");
	else
		util_fdprintf(fd, "Slot=%d\n", instance_id->ms);

	if (instance_id->ls == 0)
		util_fdprintf(fd, "first instance\n");
	else
		util_fdprintf(fd, "second instance\n");


	util_fdprintf(fd, "result: %d, %s\n", commit_image_response->result_reason,
		omcidump_reason_str(commit_image_response->result_reason));
	util_fdprintf(fd, "\n");
	return 0;
}


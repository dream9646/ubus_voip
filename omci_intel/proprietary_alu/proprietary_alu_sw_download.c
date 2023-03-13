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
 * Module  : proprietary_alu
 * File    : proprietary_alu_sw_download.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include "cli.h"
#include "util.h"
#include "coretask.h"
#include "recvtask.h"
#include "tranxmod.h"
#include "crc.h"
#include "omcidump.h"
#include "proprietary_alu_sw_download.h"

extern unsigned int global_crc_accum_be;
extern unsigned int global_crc_accum_le;

static unsigned short last_download_section_tid = 0;

struct download_info_t {
	unsigned char image_target;	//0:ONT-G, 1..254 slot number, but current version only support 0
	unsigned char instance_id; 
	unsigned char window_size_minus1;	/* window size - 1 */
	unsigned char *section_map;
	unsigned int image_size_in_bytes;
	unsigned int last_window_end;
	unsigned char update_state;	/**/
	unsigned char sw_state, active0_ok, active1_ok;
	time_t	download_start_time;
	int image_fd;
};

int
proprietary_alu_fill_resp_header_by_orig(struct proprietary_alu_sw_download_section_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	if (req_msg == NULL || resp_msg == NULL) {
		return OMCIMSG_ERROR_NOMEM;
	}

	memcpy(resp_msg, req_msg, OMCIMSG_HEADER_LEN);
	resp_msg->device_id_type = OMCIMSG_DEVICE_ID;
	omcimsg_header_set_flag_ar(resp_msg, 0);
	omcimsg_header_set_flag_ak(resp_msg, 1);

	return 0;
}

//return value, 0: low priority, 1: high priority
unsigned int
proprietary_alu_get_msg_priority(struct proprietary_alu_sw_download_section_layout_t *msg)
{
	// the util_bitmap_get function  will convert value from network order to host order
	return util_bitmap_get_value((unsigned char *) msg, 16, 0, 1);
}

int 
proprietary_alu_download_section_handler(struct proprietary_alu_sw_download_section_layout_t *req_msg, struct omcimsg_layout_t *resp_msg)
{
	struct proprietary_alu_sw_download_section_t *download_section = (struct proprietary_alu_sw_download_section_t *) req_msg->msg_contents;
	long long current_window_offset, file_offset, data_size;
	int ret, i;
	extern struct download_info_t download_info;
	
#if 0	//detail valid check
	if (proprietary_alu_sw_download_section_header_get_entity_class_id(req_msg) != 7) {	//sw image
		dbprintf(LOG_ERR, "classid should be sw image\n");
		goto exit_download_section_handler;
	}

	instance_id = (struct instance_id_t *) &req_msg->entity_instance_id;
	if(instance_id->ms != 0 || instance_id->ls == 255 ) {
		dbprintf(LOG_ERR, "Currently only support update ONT-G image, ms=%d, ls=%d\n", instance_id->ms, instance_id->ls);
		goto exit_download_section_handler;
	}

	if (instance_id->ls != download_info.instance_id) {
		dbprintf(LOG_ERR, "Instance id should match with start sw download\n");
		goto exit_download_section_handler;
	}

	// check if action is valid
	if( (download_info.instance_id == 0 && download_info.sw_state != S2P) || (download_info.instance_id == 1 && download_info.sw_state != S2 ) ) {
		dbprintf(LOG_ERR, "Action is invalid\n");
		goto exit_download_section_handler;
	}
#endif

	//calculate file offset, data size
	current_window_offset = download_section->download_section_num * OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN;
	file_offset = download_info.last_window_end + current_window_offset;
	data_size =  download_info.image_size_in_bytes - file_offset;

	if( data_size > OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN) {
		data_size=OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN;
	} else if (data_size >=0) {
		dbprintf(LOG_DEBUG, "last data_size=%lld\n", data_size);
	} else {
		dbprintf(LOG_ERR, "last data_size %lld <0?\n", data_size);
		goto exit_download_section_handler;
	}
		
	if( (download_info.image_fd=open(omci_env_g.sw_download_image_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0 ){
		dbprintf(LOG_ERR, "Error file open!\n");
		goto exit_download_section_handler;
        }

	if (file_offset == 0) {
		//init crc value
		global_crc_accum_be = 0xFFFFFFFF;
		global_crc_accum_le = 0xFFFFFFFF;
	}

	for( i=0; i < 3; i++) {
		if(omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg) == last_download_section_tid) {
			dbprintf(LOG_ERR, "download_section retransmission! tid=0x%02x prev_tid=0x%02x file_offset=%lld data_size=%lld\n", omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg), last_download_section_tid, file_offset,data_size);
			syslog(LOG_ERR, "download_section retransmission! tid=0x%02x prev_tid=0x%02x file_offset=%lld data_size=%lld\n", omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg), last_download_section_tid, file_offset,data_size);
			download_info.last_window_end = download_info.last_window_end - current_window_offset - data_size;
			file_offset = download_info.last_window_end + current_window_offset;
		}
		lseek(download_info.image_fd, file_offset, SEEK_SET);
		//write to file and check return value
		ret=write(download_info.image_fd, download_section->data, data_size);
		if ( ret == data_size ) {
			last_download_section_tid = omcimsg_header_get_transaction_id((struct omcimsg_layout_t *)req_msg);
			break;	
		}	
	}
	close(download_info.image_fd);
	download_info.image_fd=-1;

	dbprintf(LOG_WARNING, "ONT receive data size=%lld\n", data_size);
	//too noisy
	//syslog(LOG_ERR, "ONT receive data size=%lld\n", data_size);
	if ( ret != data_size ) {
		dbprintf(LOG_ERR, "Write to file error=%d\n", ret);
		goto exit_download_section_handler;
	}

	// mark this section as ok within the window		
	download_info.section_map[download_section->download_section_num]=1;

exit_download_section_handler:
	// gen response only if AR is set
	if (proprietary_alu_sw_download_section_header_get_flag_ar(req_msg)) {
		struct omcimsg_download_section_response_t *download_section_response = (struct omcimsg_download_section_response_t *) resp_msg->msg_contents;
		download_section_response->result_reason = OMCIMSG_RESULT_CMD_SUCCESS;

		for(i=0; i <= download_section->download_section_num; i++ ) {
			if( download_info.section_map[i] != 1 ) {
				dbprintf(LOG_ERR, "window %d section %d error, offset=0x%X\n", download_info.last_window_end/((download_info.window_size_minus1+1)*OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN), i, download_info.last_window_end+OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN*i);
				syslog(LOG_ERR, "window %d section %d error, offset=0x%X\n", download_info.last_window_end/((download_info.window_size_minus1+1)*OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN), i, download_info.last_window_end+OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN*i);
				download_section_response->result_reason = OMCIMSG_RESULT_PARM_ERROR;
				break;
			}
		}
		// clean section map
		memset(download_info.section_map, 0, sizeof(unsigned char) * (download_info.window_size_minus1+1));

		proprietary_alu_fill_resp_header_by_orig(req_msg, resp_msg);
		download_section_response->download_section_num=download_section->download_section_num;

		if (download_section_response->result_reason == OMCIMSG_RESULT_CMD_SUCCESS) {
			char *section_buf = malloc_safe(current_window_offset + data_size + 1);
			if(section_buf) {
				download_info.image_fd = open(omci_env_g.sw_download_image_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
				if(download_info.image_fd >= 0) {
					lseek(download_info.image_fd, download_info.last_window_end, SEEK_SET);
					read(download_info.image_fd, section_buf, current_window_offset + data_size);
					close(download_info.image_fd);
					download_info.image_fd = -1;
					switch(omci_env_g.omcc_rx_crc_type) {
						case ENV_CRC_TYPE_LE:
							global_crc_accum_le = crc_le_update(global_crc_accum_le, section_buf, current_window_offset + data_size);
							break;
						case ENV_CRC_TYPE_BE:
							global_crc_accum_be = crc_be_update(global_crc_accum_be, section_buf, current_window_offset + data_size);
							break;
						case ENV_CRC_TYPE_AUTO:
						default:
							global_crc_accum_be = crc_be_update(global_crc_accum_be, section_buf, current_window_offset + data_size);
							global_crc_accum_le = crc_le_update(global_crc_accum_le, section_buf, current_window_offset + data_size);
							break;
					}
				}
				free_safe(section_buf);
			}
			download_info.last_window_end = download_info.last_window_end + current_window_offset + data_size;
			return OMCIMSG_RW_OK;
		} else {
			return OMCIMSG_ERROR_PARM_ERROR;
		}

	} else {
		//dbprintf(LOG_DEBUG, "No error during section download, and no need response\n");
		resp_msg->transaction_id=0;
		resp_msg->device_id_type=0;
	}
	return OMCIMSG_RW_OK;
}

//0: not handled, 1: handled
int
proprietary_alu_handle_download_section(struct proprietary_alu_sw_download_section_layout_t *req_msg)
{
	struct omcimsg_layout_t *resp_msg;	
	//allocate response msg
	resp_msg = (struct omcimsg_layout_t *) malloc_safe(sizeof (struct omcimsg_layout_t));

	//add to omcclog, history, similar to coretask_process_omci_msg()
	if (omci_env_g.omccrx_log_enable) {
		if (omci_env_g.omccrx_log_msgmask & (1<<OMCIMSG_DOWNLOAD_SECTION))
			omcidump_print_raw_to_file((struct omcimsg_layout_t *) req_msg, omci_env_g.omccrx_log_file, OMCIDUMP_DIR_RX);
	}

	cli_history_add_msg((struct omcimsg_layout_t *) req_msg);

	proprietary_alu_download_section_handler(req_msg, resp_msg);

	if (proprietary_alu_sw_download_section_header_get_flag_ar(req_msg)) {
		// send back response if AR is set
		tranxmod_send_omci_msg(resp_msg, 0);
	} else {
		free_safe(resp_msg);
	}
	return 0;
}

int
proprietary_alu_sw_download(char *recvbuf)
{
	static char crc_type = -1;	// use static variable to keep the crc_type calc result
	struct proprietary_alu_sw_download_section_layout_t *msg = (struct proprietary_alu_sw_download_section_layout_t *) recvbuf;
	unsigned int msg_crc, crc_accum, crc_le, crc_be, crc = 0;

	if (crc_type == -1)
		crc_type = omci_env_g.omcc_rx_crc_type;		// init crc_type with env default, and modify it to be/le if default is auto
	msg_crc = proprietary_alu_sw_download_section_trailer_get_crc(msg); //only for crc enabled and udp socket
	
	/*process recv data, check crc first */
	switch(crc_type)
	{
		case ENV_CRC_TYPE_AUTO:
			crc_accum = 0xFFFFFFFF;
			crc_be = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_DL_SECTION_BEFORE_CRC_LEN));
			crc_accum = 0xFFFFFFFF;
			crc_le = ~(crc_le_update(crc_accum, (char *) msg, OMCIMSG_DL_SECTION_BEFORE_CRC_LEN));
			crc_le = htonl(crc_le);

			if (crc_be == msg_crc)
			{
				crc = crc_be;
				crc_type = ENV_CRC_TYPE_BE; //be
			} else {
				crc = crc_le;
				crc_type = ENV_CRC_TYPE_LE; //le
			}
			break;
		case ENV_CRC_TYPE_BE:
			crc_accum = 0xFFFFFFFF;
			crc = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_DL_SECTION_BEFORE_CRC_LEN));
			break;
		case ENV_CRC_TYPE_LE:
			crc_accum = 0xFFFFFFFF;
			crc = ~(crc_le_update(crc_accum, (char *) msg, OMCIMSG_DL_SECTION_BEFORE_CRC_LEN));
			crc = htonl(crc);
			break;
		case ENV_CRC_TYPE_DISABLE:
			//do nothing
			break;
		default:
			dbprintf(LOG_ERR, "crc type error(%d), treated as auto\n", crc_type);
			crc_accum = 0xFFFFFFFF;
			crc_be = ~(crc_be_update(crc_accum, (char *) msg, OMCIMSG_DL_SECTION_BEFORE_CRC_LEN));
			crc_accum = 0xFFFFFFFF;
			crc_le = ~(crc_le_update(crc_accum, (void *) msg, OMCIMSG_DL_SECTION_BEFORE_CRC_LEN));
			crc_le = htonl(crc_le);
			if (crc_be == msg_crc)
			{
				crc = crc_be;
				crc_type = ENV_CRC_TYPE_BE; //be
			} else {
				crc = crc_le;
				crc_type = ENV_CRC_TYPE_LE; //le
			}
	}

	if (crc_type != ENV_CRC_TYPE_DISABLE && crc != msg_crc) {
		dbprintf(LOG_ERR,
			"recv crc error, crc_type=%d, orig=%.8X, crc=%.8X\n",
			crc_type, msg_crc, crc);
		return -1;
	}
	
	proprietary_alu_handle_download_section(msg);

	return 0;
}

int
proprietary_alu_start_sw_download(struct omcimsg_start_software_download_response_t *start_sw_download_response)
{
	start_sw_download_response->number_of_responding=(OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN>>8) & 0xFF;
	start_sw_download_response->instance_result[0].image_slot=OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN & 0xFF;
	
	return 0;
}

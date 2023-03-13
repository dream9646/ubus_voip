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
 * File    : proprietary_alu_sw_download.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY_ALU_SW_DOWNLOAD_H__
#define __PROPRIETARY_ALU_SW_DOWNLOAD_H__

#define OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN	1440
#define OMCIMSG_DL_SECTION_BEFORE_CRC_LEN	(OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN+13)

/* msg layout */
struct proprietary_alu_sw_download_section_layout_t {
	unsigned short transaction_id;
	unsigned char msg_type;
	unsigned char device_id_type;
	unsigned short entity_class_id;
	unsigned short entity_instance_id;
	unsigned char msg_contents[OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN+1];
	unsigned short cpcsuu_cpi;
	unsigned short cpcssdu_len;
	unsigned int crc;
	struct timeval timestamp;
} __attribute__ ((packed));

struct proprietary_alu_sw_download_section_t {
	unsigned char download_section_num;
	unsigned char data[OMCIMSG_ALU_DL_SECTION_CONTENTS_LEN];
} __attribute__ ((packed));

static inline unsigned int
proprietary_alu_sw_download_section_trailer_get_crc(struct proprietary_alu_sw_download_section_layout_t *msg)
{
	return ntohl(msg->crc);
}

static inline unsigned char
proprietary_alu_sw_download_section_header_get_flag_ar(struct proprietary_alu_sw_download_section_layout_t *msg)
{
	return (msg->msg_type & (1 << 6)) ? 1 : 0;
}

static inline unsigned short
proprietary_alu_sw_download_section_header_get_entity_class_id(struct proprietary_alu_sw_download_section_layout_t *msg)
{
	return ntohs(msg->entity_class_id);
}

int proprietary_alu_sw_download(char *);
int proprietary_alu_start_sw_download(struct omcimsg_start_software_download_response_t *);
#endif

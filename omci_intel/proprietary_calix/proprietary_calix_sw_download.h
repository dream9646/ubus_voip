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
 * Module  : proprietary_calix
 * File    : proprietary_calix_sw_download.h
 *
 ******************************************************************/

#ifndef __PROPRIETARY__CALIX__SW_DOWNLOAD_H__
#define __PROPRIETARY__CALIX__SW_DOWNLOAD_H__

#define OMCIMSG_CALIX_DL_SECTION_CONTENTS_LEN	1500
#define OMCIMSG_CALIX_DL_SECTION_BEFORE_CRC_LEN	(OMCIMSG_CALIX_DL_SECTION_CONTENTS_LEN+11)

/* msg layout */
struct proprietary_calix_sw_download_section_layout_t {
	unsigned short transaction_id;
	unsigned char msg_type;
	unsigned char device_id_type;
	unsigned short entity_class_id;
	unsigned short entity_instance_id;
	unsigned short msg_length;
	unsigned char msg_contents[OMCIMSG_CALIX_DL_SECTION_CONTENTS_LEN+1];
	unsigned int crc;
	struct timeval timestamp;
} __attribute__ ((packed));

struct proprietary_calix_sw_download_section_t {
	unsigned char download_section_num;
	unsigned char data[OMCIMSG_CALIX_DL_SECTION_CONTENTS_LEN];
} __attribute__ ((packed));

static inline unsigned int
proprietary_calix_sw_download_section_trailer_get_crc(struct proprietary_calix_sw_download_section_layout_t *msg)
{
	return ntohl(msg->crc);
}

static inline unsigned char
proprietary_calix_sw_download_section_header_get_flag_ar(struct proprietary_calix_sw_download_section_layout_t *msg)
{
	return (msg->msg_type & (1 << 6)) ? 1 : 0;
}

static inline unsigned short
proprietary_calix_sw_download_section_header_get_entity_class_id(struct proprietary_calix_sw_download_section_layout_t *msg)
{
	return ntohs(msg->entity_class_id);
}

int proprietary_calix_sw_download(char *);
#endif

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
 * Module  : oltsim
 * File    : omcistr.h
 *
 ******************************************************************/

#ifndef __OMCISTR_H__
#define __OMCISTR_H__

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <linux/sockios.h>

/* omcistr.c */
char *omcistr_classid2name(unsigned short classid);
char *omcistr_classid2section(unsigned short classid);
char *omcistr_msgtype2name(unsigned char msgtype);
int omcistr_msgtype_is_write(unsigned char msgtype);

/* omci msg layout */
struct omcimsg_layout_t {
	unsigned short transaction_id;
	unsigned char msg_type;
	unsigned char device_id_type;
	unsigned short entity_class_id;
	unsigned short entity_instance_id;
	unsigned char msg_contents[32];
	unsigned short cpcsuu_cpi;
	unsigned short cpcssdu_len;
	unsigned int crc;
} __attribute__ ((packed));

#define OMCIMSG_LEN 48
#define OMCIMSG_HEADER_LEN 8
#define OMCIMSG_CONTENTS_LEN 32
#define OMCIMSG_TRAILER_LEN 8
#define OMCIMSG_BEFORE_CRC_LEN 44

/* msg type */
#define	OMCIMSG_CREATE			4
#define OMCIMSG_DELETE			6
#define OMCIMSG_SET			8
#define OMCIMSG_GET			9
#define OMCIMSG_GET_ALL_ALARMS		11
#define OMCIMSG_GET_ALL_ALARMS_NEXT	12
#define OMCIMSG_MIB_UPLOAD		13
#define OMCIMSG_MIB_UPLOAD_NEXT		14
#define OMCIMSG_MIB_RESET		15
#define OMCIMSG_ALARM			16
#define OMCIMSG_ATTRIBUTE_VALUE_CHANGE	17
#define OMCIMSG_TEST			18
#define OMCIMSG_START_SOFTWARE_DOWNLOAD	19
#define OMCIMSG_DOWNLOAD_SECTION	20
#define OMCIMSG_END_SOFTWARE_DOWNLOAD	21
#define OMCIMSG_ACTIVATE_SOFTWARE	22
#define OMCIMSG_COMMIT_SOFTWARE		23
#define OMCIMSG_SYNCHRONIZE_TIME	24
#define OMCIMSG_REBOOT			25
#define OMCIMSG_GET_NEXT		26
#define OMCIMSG_TEST_RESULT		27
#define OMCIMSG_GET_CURRENT_DATA	28

#define OMCIMSG_CREATE_COMPLETE_CONNECTION	5	// deprecated
#define OMCIMSG_DELETE_COMPLETE_CONNECTION	7	// deprecated
#define OMCIMSG_GET_COMPLETE_CONNECTION		10	// deprecated

/* header get function */
static inline unsigned short
omcimsg_header_get_transaction_id(struct omcimsg_layout_t *msg)
{
	return ntohs(msg->transaction_id);
}

static inline unsigned char
omcimsg_header_get_type(struct omcimsg_layout_t *msg)
{
	return msg->msg_type & 0x1f;
}

static inline unsigned char
omcimsg_header_get_flag_db(struct omcimsg_layout_t *msg)
{
	return (msg->msg_type & (1 << 7)) ? 1 : 0;
}
static inline unsigned char
omcimsg_header_get_flag_ar(struct omcimsg_layout_t *msg)
{
	return (msg->msg_type & (1 << 6)) ? 1 : 0;
}
static inline unsigned char
omcimsg_header_get_flag_ak(struct omcimsg_layout_t *msg)
{
	return (msg->msg_type & (1 << 5)) ? 1 : 0;
}

static inline unsigned char
omcimsg_header_get_device_id(struct omcimsg_layout_t *msg)
{
	return msg->device_id_type;
}

static inline unsigned short
omcimsg_header_get_entity_class_id(struct omcimsg_layout_t *msg)
{
	return ntohs(msg->entity_class_id);
}

static inline unsigned short
omcimsg_header_get_entity_instance_id(struct omcimsg_layout_t *msg)
{
	return ntohs(msg->entity_instance_id);
}

static inline unsigned short
omcimsg_trailer_get_cpcsuu_cpi(struct omcimsg_layout_t *msg)
{
	return ntohs(msg->cpcsuu_cpi);
}

static inline unsigned short
omcimsg_trailer_get_cpcssdu_len(struct omcimsg_layout_t *msg)
{
	return ntohs(msg->cpcssdu_len);
}

static inline unsigned int
omcimsg_trailer_get_crc(struct omcimsg_layout_t *msg)
{
	return ntohl(msg->crc);
}

#endif

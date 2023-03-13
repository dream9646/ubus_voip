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
 * File    : omcimsg.h
 *
 ******************************************************************/

#ifndef __OMCIMSG_H__
#define __OMCIMSG_H__

#include <arpa/inet.h>
#include "fwk_timer.h"
#include "meinfo.h"

#define OMCIMSG_LEN 48
#define OMCIMSG_HEADER_LEN 8
#define OMCIMSG_CONTENTS_LEN 32
#define OMCIMSG_TRAILER_LEN 8
#define OMCIMSG_BEFORE_CRC_LEN 44
#define OMCIMSG_CRC_LEN 4
#define OMCIMSG_MIB_UPLOAD_LEN 26
#define OMCIMSG_GET_NEXT_LEN 29
#define OMCIMSG_DL_SECTION_CONTENTS_LEN	(OMCIMSG_CONTENTS_LEN-1)

#define OMCIMSG_CPCSUU_CPI 0x0000
#define OMCIMSG_CPCSSDU_LEN 0x0028

#define OMCIMSG_DEVICE_ID 0xa
#define OMCIMSG_EXTENDED_DEVICE_ID 0xb

#define	OMCIMSG_SW_UPDATE_NORMAL		0
#define	OMCIMSG_SW_UPDATE_DOWNLOAD		1
#define	OMCIMSG_SW_UPDATE_FLASH_WRITE		2
#define	OMCIMSG_SW_UPDATE_FLASH_WRITE_FAIL	3
#define	OMCIMSG_SW_UPDATE_ACTIVE_IMAGE_FAIL	4

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
#define OMCIMSG_SET_TABLE		29

#define OMCIMSG_CREATE_COMPLETE_CONNECTION	5	// deprecated
#define OMCIMSG_DELETE_COMPLETE_CONNECTION	7	// deprecated
#define OMCIMSG_GET_COMPLETE_CONNECTION		10	// deprecated

/* response result reason, used in omci msg */
#define OMCIMSG_RESULT_CMD_SUCCESS		0
#define OMCIMSG_RESULT_CMD_ERROR		1
#define OMCIMSG_RESULT_CMD_NOT_SUPPORTED	2
#define OMCIMSG_RESULT_PARM_ERROR		3
#define OMCIMSG_RESULT_UNKNOWN_ME		4
#define OMCIMSG_RESULT_UNKNOWN_ME_INSTANCE	5
#define OMCIMSG_RESULT_DEVICE_BUSY		6
#define OMCIMSG_RESULT_INSTANCE_EXISTS		7
#define OMCIMSG_RESULT_ATTR_FAILED_UNKNOWN	9

/* OMCIMSG function return value */
#define OMCIMSG_RW_OK				0
#define OMCIMSG_ERROR_BASE			0x2000
#define OMCIMSG_ERROR_CMD_ERROR			-(OMCIMSG_ERROR_BASE + 0x1)
#define OMCIMSG_ERROR_CMD_NOT_SUPPORTED		-(OMCIMSG_ERROR_BASE + 0x2)
#define OMCIMSG_ERROR_PARM_ERROR		-(OMCIMSG_ERROR_BASE + 0x3)
#define OMCIMSG_ERROR_UNKNOWN_ME		-(OMCIMSG_ERROR_BASE + 0x4)
#define OMCIMSG_ERROR_UNKNOWN_ME_INSTANCE	-(OMCIMSG_ERROR_BASE + 0x5)
#define OMCIMSG_ERROR_DEVICE_BUSY		-(OMCIMSG_ERROR_BASE + 0x6)
#define OMCIMSG_ERROR_INSTANCE_EXISTS		-(OMCIMSG_ERROR_BASE + 0x7)
#define OMCIMSG_ERROR_ATTR_FAILED_UNKNOWN	-(OMCIMSG_ERROR_BASE + 0x9)

#define OMCIMSG_ERROR_NOMEM 			-(OMCIMSG_ERROR_BASE + 0x10)
#define OMCIMSG_ERROR_START_DUPLICATED 		-(OMCIMSG_ERROR_BASE + 0x11)
#define OMCIMSG_ERROR_CMD_NUM_WRONG 		-(OMCIMSG_ERROR_BASE + 0x12)
#define OMCIMSG_ERROR_UPLOAD_COMPLETED 		-(OMCIMSG_ERROR_BASE + 0x13)
#define OMCIMSG_ERROR_NOT_IN_PROGRESS 		-(OMCIMSG_ERROR_BASE + 0x14)
#define OMCIMSG_ERROR_TIMER_ERROR 		-(OMCIMSG_ERROR_BASE + 0x15)

#define OMCIMSG_ERROR_HW_ME_GET 		-(OMCIMSG_ERROR_BASE + 0x16)
#define OMCIMSG_ERROR_HW_ME_SET 		-(OMCIMSG_ERROR_BASE + 0x17)
#define OMCIMSG_ERROR_SW_ME_GET 		-(OMCIMSG_ERROR_BASE + 0x18)
#define OMCIMSG_ERROR_SW_ME_SET 		-(OMCIMSG_ERROR_BASE + 0x19)

#define OMCIMSG_TEST_RESULT_SELFTEST_FAILED		0
#define OMCIMSG_TEST_RESULT_SELFTEST_PASSED		1
#define OMCIMSG_TEST_RESULT_SELFTEST_NOT_COMPLETED	2

#define OMCIMSG_TEST_RESULT_IP_HOST_TIMEOUT		0
#define OMCIMSG_TEST_RESULT_IP_HOST_RESPONSE		1
#define OMCIMSG_TEST_RESULT_IP_HOST_TIME_EXCEEDED	2
#define OMCIMSG_TEST_RESULT_IP_HOST_UNEXPECTED_RESPONSE	3

#define OMCIMSG_TEST_RESULT_DOT1AG_MEP_SUCCESS		0
#define OMCIMSG_TEST_RESULT_DOT1AG_MEP_TIMEOUT		1

struct omcimsg_sw_download_param_t {
	char version0[15];	//14+1
	char version1[15];	//14+1
	unsigned char active0, active0_ok, commit0, valid0, active1, active1_ok, commit1, valid1;
};

/* msg layout */
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
	struct timeval timestamp;
} __attribute__ ((packed));

struct omcimsg_type_attr_t {
	unsigned char supported;	//0: not supported, 1: supported
	char *name;
	unsigned char ak;		//0: no, 1: yes, 2: both
	unsigned char inc_mib_data_sync;	//0: no, 1: yes
	unsigned char is_write;
	unsigned char result_reason;
	unsigned char is_notification;
};

///////////////////////////////////////////////////////////////////////////

struct omcimsg_create_t {
	unsigned char attr_value[0];	// variable length
} __attribute__ ((packed));

struct omcimsg_create_response_t {
	unsigned char result_reason;
	unsigned char attr_exec_mask[2];
} __attribute__ ((packed));

struct omcimsg_delete_t {
	unsigned char padding[0];	// zero length
} __attribute__ ((packed));

struct omcimsg_delete_response_t {
	unsigned char result_reason;
} __attribute__ ((packed));

struct omcimsg_set_t {
	unsigned char attr_mask[2];
	unsigned char attr_value[0];	// variable length
} __attribute__ ((packed));

struct omcimsg_set_response_t {
	unsigned char result_reason;
	unsigned char optional_attr_mask[2];
	unsigned char attr_exec_mask[2];
} __attribute__ ((packed));

struct omcimsg_get_t {
	unsigned char attr_mask[2];
} __attribute__ ((packed));

struct omcimsg_get_response_t {
	unsigned char result_reason;
	unsigned char attr_mask[2];
	unsigned char attr_value[25];
	unsigned char optional_attr_mask[2];
	unsigned char attr_exec_mask[2];
} __attribute__ ((packed));

struct omcimsg_get_response_extend_t {
	unsigned char result_reason;
	unsigned char attr_mask[2];
	unsigned char optional_attr_mask[2];
	unsigned char attr_exec_mask[2];
	unsigned char attr_value[0];	// variable length
} __attribute__ ((packed));

struct omcimsg_get_all_alarms_t {
	unsigned char alarm_retrieval_mode;
} __attribute__ ((packed));

struct omcimsg_get_all_alarms_response_t {
	unsigned short subsequent_cmd_total;
} __attribute__ ((packed));

struct omcimsg_get_all_alarms_next_t {
	unsigned short sequence_num;
} __attribute__ ((packed));

struct omcimsg_get_all_alarms_next_response_optional_t {
	unsigned short entity_class;
	unsigned short entity_instance;
	unsigned char bit_map_alarms[28];
} __attribute__ ((packed));
struct omcimsg_get_all_alarms_next_response_t {
	unsigned short entity_class;
	unsigned short entity_instance;
	unsigned char bit_map_alarms[28];
	struct omcimsg_get_all_alarms_next_response_optional_t optional[0]; // optional, only supported by extend msg 
} __attribute__ ((packed));

struct omcimsg_mib_upload_t {
	unsigned char padding[0];	// zero length
} __attribute__ ((packed));

struct omcimsg_mib_upload_response_t {
	unsigned short subsequent_cmd_total;
} __attribute__ ((packed));

struct omcimsg_mib_upload_next_t {
	unsigned short sequence_num;
} __attribute__ ((packed));

struct omcimsg_mib_upload_next_response_optional_t {
	unsigned short entity_class;
	unsigned short entity_instance;
	unsigned char attr_mask[2];
	unsigned char value_first_attr[0];	// variable length
} __attribute__ ((packed));
struct omcimsg_mib_upload_next_response_t {
	unsigned short entity_class;
	unsigned short entity_instance;
	unsigned char attr_mask[2];
	unsigned char value_first_attr[0];	// variable length
	//struct omcimsg_mib_upload_next_response_optional_t optional[0];	// optional, only supported by extend msg
} __attribute__ ((packed));

struct omcimsg_mib_reset_t {
	unsigned char padding[0];		// zero length
} __attribute__ ((packed));

struct omcimsg_mib_reset_response_t {
	unsigned char result_reason;
} __attribute__ ((packed));

struct omcimsg_alarm_t {
	unsigned char alarm_mask[28];
	unsigned char padding[3];
	unsigned char alarm_sequence_num;
} __attribute__ ((packed));

struct omcimsg_alarm_extend_t {
	unsigned char alarm_mask[28];
	unsigned char alarm_sequence_num;
} __attribute__ ((packed));

struct omcimsg_attr_value_change_t {
	unsigned char attr_mask[2];
	unsigned char attr_value[0];
} __attribute__ ((packed));

/* ONT-G, ANI-G and circuit pack entity classes */

struct omcimsg_test_ont_ani_circuit_t {
	unsigned char select_test;
	unsigned short pointer_to_general_purpose_buffer_me;	// only supported by extend msg
	unsigned short pointer_to_octet_string_me;		// only supported by extend msg
} __attribute__ ((packed));

/* IP host config data entity class*/
struct omcimsg_test_ip_host_t {
	unsigned char select_test;
	union {
		unsigned int target_ipaddr;
		unsigned char target_ipv6addr[16];	// only supported by extend msg
	} u;
	unsigned char number_of_ping;			// only supported by extend msg
	unsigned short pointer_to_large_string_me;	// only supported by extend msg, dns name to ping
} __attribute__ ((packed));

/* POTS UNI and PPTP ISDN UNI entity classes */
struct omcimsg_test_pots_isdn_t {
	unsigned char select_test;
	union {
		struct {
			unsigned char dbdt_timer_t1;
			unsigned char dbdt_timer_t2;
			unsigned char dbdt_timer_t3;
			unsigned char dbdt_timer_t4;
			unsigned char dbdt_control_byte;
			unsigned char digit_to_be_dialled;
			unsigned short dial_tone_frequency_1;
			unsigned short dial_tone_frequency_2;
			unsigned short dial_tone_frequency_3;
			unsigned char dial_tone_power_threshold;
			unsigned char idle_channel_power_threshold;
			unsigned char dc_hazardous_voltage_threshold;
			unsigned char ac_hazardous_voltage_threshold;
			unsigned char dc_foreign_voltage_threshold;
			unsigned char ac_foreign_voltage_threshold;
			unsigned char tip_ground_and_ring_ground_resistance_threshold;
			unsigned char tip_ring_resistance_threshold;
			unsigned short ringer_equivalence_min_threshold;
			unsigned short ringer_equivalence_max_threshold;
			unsigned short pointer_to_a_general_purpose_buffer_me;
			unsigned short pointer_to_an_octet_string_me;
		} __attribute__ ((packed)) class0;
		struct {
			unsigned char number_to_dial[16];
		} __attribute__ ((packed)) class1;	// only supported by extend msg
	} u;
} __attribute__ ((packed));

/* Dot1ag MEP entity class */
struct omcimsg_test_dot1ag_mep_t {
	unsigned char select_test;
	union {
		struct {
			unsigned char priority;
			unsigned char target_mac[6];
			unsigned short target_mep_id;
			unsigned short repeat_count;
			unsigned short octet_string_meid_1;
			unsigned short octet_string_meid_2;
			unsigned short octet_string_meid_3;
			unsigned short octet_string_meid_4;
		} __attribute__ ((packed)) lb;
		struct {
			unsigned char flags;
			unsigned char target_mac[6];
			unsigned short target_mep_id;
			unsigned char ttl;
			unsigned short pointer_to_a_general_purpose_buffer_me;
		} __attribute__ ((packed)) lt;
	} u;
} __attribute__ ((packed));

struct omcimsg_test_response_t {
	unsigned char result_reason;
} __attribute__ ((packed));

struct omcimsg_start_software_download_t {
	unsigned char window_size_minus1;	/* window size - 1 */
	unsigned int image_size_in_bytes;
	unsigned char number_of_update;		/*1-9 ?*/
	struct {
		unsigned char image_slot;
		unsigned char image_instance;
	} __attribute__ ((packed)) target[9];
} __attribute__ ((packed));

struct omcimsg_start_software_download_response_t {
	unsigned char result_reason;
	unsigned char window_size_minus1;	/* window size - 1 */
	unsigned char number_of_responding;	/*0-9*/
	struct {
		unsigned char image_slot;
		unsigned char image_instance;
		unsigned char result_reason;
	} __attribute__ ((packed)) instance_result[9];
} __attribute__ ((packed));

struct omcimsg_download_section_t {
	unsigned char download_section_num;
	unsigned char data[0];	// variable length
} __attribute__ ((packed));

struct omcimsg_download_section_response_t {
	unsigned char result_reason;
	unsigned char download_section_num;
} __attribute__ ((packed));

struct omcimsg_end_software_download_t {
	unsigned int crc_32;
	unsigned int image_size_in_bytes;
	unsigned char parallel_download_instances_total;	// 1..9
	unsigned short software_image_instance[9];
} __attribute__ ((packed));

struct omcimsg_end_software_download_response_t {
	unsigned char result_reason;
	unsigned char responding_instances_total;
	struct {
		unsigned short instance_id;
		unsigned char result_reason;
	} __attribute__ ((packed)) instance_result[9];
} __attribute__ ((packed));

struct omcimsg_activate_image_t {
	unsigned char flags_pots;	// only supported by extend msg
} __attribute__ ((packed));

struct omcimsg_activate_image_response_t {
	unsigned char result_reason;
} __attribute__ ((packed));

struct omcimsg_commit_image_t {
	unsigned char padding[0];	// zero length
} __attribute__ ((packed));

struct omcimsg_commit_image_response_t {
	unsigned char result_reason;
} __attribute__ ((packed));

struct omcimsg_synchronize_time_t {
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
} __attribute__ ((packed));

struct omcimsg_synchronize_time_response_t {
	unsigned char result_reason;
	unsigned char success_result_info;
} __attribute__ ((packed));

struct omcimsg_reboot_t {
	unsigned char flags_pots;	// only supported by extend msg
} __attribute__ ((packed));

struct omcimsg_reboot_response_t {
	unsigned char result_reason;
} __attribute__ ((packed));

struct omcimsg_get_next_t {
	unsigned char attr_mask[2];
	unsigned short cmd_sequence_num;
} __attribute__ ((packed));

struct omcimsg_get_next_response_t {
	unsigned char result_reason;
	unsigned char attr_mask[2];
	unsigned char attr_value[0];	// variable length
} __attribute__ ((packed));

/* self test action invoked against ONT-G and circuit pack entity classes */
struct omcimsg_test_result_ont_circuit_selftest_t {
	unsigned char unused;
	unsigned char self_test_result;
	unsigned short pointer_to_gereral_purpose_buffer_me;
} __attribute__ ((packed));

/* vendor-specific test actions invoked against ONT-G and circuit pack entity classes */
struct omcimsg_test_result_ont_vendortest_t {
	unsigned char type_1;
	unsigned short value_1;
	unsigned char type_2;
	unsigned short value_2;
	unsigned char type_3;
	unsigned short value_3;
	unsigned char type_4;
	unsigned short value_4;
	unsigned char type_5;
	unsigned short value_5;
	unsigned char type_6;
	unsigned short value_6;
	unsigned char type_7;
	unsigned short value_7;
	unsigned char type_8;
	unsigned short value_8;
	unsigned char type_9;
	unsigned short value_9;
	unsigned char type_10;
	unsigned short value_10;
} __attribute__ ((packed));

/* POTS UNI and PPTP ISDN UNI entity classes */
struct omcimsg_test_result_pots_isdn_t {
	unsigned char select_test_and_mlt_drop_test_result;
	unsigned char self_or_vendor_test_result;
	unsigned char dial_tone_make_break_flags;
	unsigned char dial_tone_power_flags;
	unsigned char loop_test_dc_voltage_flags;
	unsigned char loop_test_ac_voltage_flags;
	unsigned char loop_test_resistance_flags_1;
	unsigned char loop_test_resistance_flags_2;
	unsigned char time_to_draw_dial_tone;
	unsigned char time_to_break_dial_tone;
	unsigned char total_dial_tone_power_measurement;
	unsigned char quiet_channel_power_measurement;
	unsigned short tip_ground_dc_voltage;
	unsigned short ring_ground_dc_voltage;
	unsigned char tip_ground_ac_voltage;
	unsigned char ring_ground_ac_voltage;
	unsigned short tip_ground_dc_resistance;
	unsigned short ring_ground_dc_resistance;
	unsigned short tip_ring_dc_resistance;
	unsigned char ringer_equivalence;
	unsigned short pointer_to_a_general_purpose_buffer_me;
	unsigned char loop_tip_ring_test_ac_dc_voltage_flags;
	unsigned char tip_ring_ac_voltage;
	unsigned short tip_ring_dc_voltage;
} __attribute__ ((packed));

/* test action invoked against IP host config data entity class */
struct omcimsg_test_result_ip_host_t {
	unsigned char test_result;
	unsigned char meaningful_bytes_total;
	union {
		struct {
			unsigned short response_delay[0];	// variable length
		} __attribute__ ((packed)) result_001;
		struct {
			union {
				unsigned int ipaddr;
				unsigned char ipv6addr[16];
			} u;
			unsigned short response_delay[0];	// variable length
		} __attribute__ ((packed)) result_001_extended_ping;
		struct {
			unsigned int neighbour_ipaddr[0];
		} __attribute__ ((packed)) result_010;
		struct {
			unsigned char neighbour_ipv6addr[0];
		} __attribute__ ((packed)) result_010_ipv6;
		struct {
			unsigned char type;	// icmpmsg[0]
			unsigned char code;	// icmpmsg[1]
			unsigned short checksum;	// icmpmsg[2,3]
			unsigned char icmp_payload[0];	// variable length for icmpmsg[4..]
		} __attribute__ ((packed)) result_011;
	} u;
} __attribute__ ((packed));  

/* optical line supervision test action invoked against ANI-G entity class */
struct omcimsg_test_result_ani_linetest_t {
	unsigned char type1;
	short power_feed_voltage;
	unsigned char type3;
	short received_optical_power;
	unsigned char type5;
	short transmitted_optical_power;
	unsigned char type9;
	unsigned short laser_bias_current;
	unsigned char type12;
	short temperature_degrees;
	unsigned short pointer_to_general_purpose_buffer_me;
} __attribute__ ((packed));

/* test action invoked against IP host config data entity class */
struct omcimsg_test_result_dot1ag_mep_t {
	unsigned char test_result;
	union {
		struct {
			unsigned short lbr_count_valid;
			unsigned short lbr_count_out_of_order;
			unsigned short lbr_count_mismatch;
			unsigned int lbr_delay;
		} __attribute__ ((packed)) lb;
		struct {
			unsigned int ltm_xid;
			unsigned char ltm_tlv[8];
		} __attribute__ ((packed)) lt;
	} u;
} __attribute__ ((packed));

struct omcimsg_get_current_data_t {
	unsigned char attr_mask[2];
} __attribute__ ((packed));

struct omcimsg_get_current_data_response_t {
	unsigned char result_reason;
	unsigned char attr_mask[2];
	unsigned char attr_value[25];
	unsigned char optional_attr_mask[2];
	unsigned char attr_exec_mask[2];
} __attribute__ ((packed));

struct omcimsg_get_current_data_response_extend_t {
	unsigned char result_reason;
	unsigned char attr_mask[2];
	unsigned char optional_attr_mask[2];
	unsigned char attr_exec_mask[2];
	unsigned char attr_value[0];	// variable length
} __attribute__ ((packed));

struct omcimsg_set_table_t {		// only supported by extend msg
	unsigned char attr_mask[2];
	unsigned char attr_value[0];	// variable length, values of all rows of a table
} __attribute__ ((packed));

struct omcimsg_set_table_response_t {	// only supported by extend msg
	unsigned char result_reason;
} __attribute__ ((packed));


///////////////////////////////////////////////////////////////////////////

struct omcimsg_mibupload_state_t {
	unsigned char is_me_mibupload_clone_list_used;	//0: not use, 1: used
	unsigned short last_seq_index;
	struct list_head *last_me_instance_ptr;
	unsigned char last_attr_order;
	unsigned int curr_cmd_num;
	unsigned int total_cmds_num;
	unsigned int timer_id;
};

struct omcimsg_getallalarms_state_t {
	unsigned char is_valid_timer;
	unsigned char cmd_seq_num;
	unsigned char is_arc_cared;		//alarm_retrieval_mode
	unsigned char is_clone_list_used;
	unsigned short last_index;
	struct me_t *last_me_instance_ptr;
	unsigned short max_cmd_seq_num;		//unsigned short subsequent_cmd_total;
	int timer_id;
};

enum omcimsg_timer_event_t {
	OMCIMSG_MIBUPLOAD_TIMEOUT,
	OMCIMSG_GETALLALARMS_TIMEOUT,
	OMCIMSG_GETTABLE_TIMEOUT
};

struct omcimsg_count_t{
	unsigned int alarm;
	unsigned int avc;
	unsigned int create;
	unsigned int delete;
	unsigned int get;
	unsigned int get_all_alarms;
	unsigned int get_current_data;
	unsigned int get_next;
	unsigned int handler;
	unsigned int mib_reset;
	unsigned int mib_upload;
	unsigned int mib_upload_seq;
	unsigned int omcimsg;
	unsigned int set;
	unsigned int test;
	unsigned int reboot;
	unsigned int start_software_download;
	unsigned int download_section;
	unsigned int end_software_download;
	unsigned int activate_software;
	unsigned int commit_software;
	unsigned int get_all_alarms_next;
	unsigned int mib_upload_next;
	unsigned int synchronize_time;
};

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

/* header set function */
static inline void
omcimsg_header_set_transaction_id(struct omcimsg_layout_t *msg, unsigned short transaction_id)
{
	msg->transaction_id = htons(transaction_id);
}

static inline void
omcimsg_header_set_type(struct omcimsg_layout_t *msg, unsigned char msg_type)
{
	msg->msg_type &= 0xe0;
	msg->msg_type |= (msg_type & 0x1f);
}

static inline void
omcimsg_header_set_flag_db(struct omcimsg_layout_t *msg, int db)
{
	if (db)
		msg->msg_type |= (1 << 7);
	else
		msg->msg_type &= (~(1 << 7));
}
static inline void
omcimsg_header_set_flag_ar(struct omcimsg_layout_t *msg, int ar)
{
	if (ar)
		msg->msg_type |= (1 << 6);
	else
		msg->msg_type &= (~(1 << 6));
}
static inline void
omcimsg_header_set_flag_ak(struct omcimsg_layout_t *msg, int ak)
{
	if (ak)
		msg->msg_type |= (1 << 5);
	else
		msg->msg_type &= (~(1 << 5));
}

static inline void
omcimsg_header_set_device_id(struct omcimsg_layout_t *msg, unsigned char device_id)
{
	msg->device_id_type = 0xa;	// must be 0xa
}

static inline void
omcimsg_header_set_entity_class_id(struct omcimsg_layout_t *msg, unsigned short entity_class_id)
{
	msg->entity_class_id = htons(entity_class_id);
}

static inline void
omcimsg_header_set_entity_instance_id(struct omcimsg_layout_t *msg, unsigned short entity_instance_id)
{
	msg->entity_instance_id = htons(entity_instance_id);
}

static inline void
omcimsg_trailer_set_cpcsuu_cpi(struct omcimsg_layout_t *msg, unsigned short cpcsuu_cpi)
{
	msg->cpcsuu_cpi = htons(cpcsuu_cpi);
}

static inline void
omcimsg_trailer_set_cpcssdu_len(struct omcimsg_layout_t *msg, unsigned short cpcssdu_len)
{
	msg->cpcssdu_len = htons(cpcssdu_len);
}

static inline void
omcimsg_trailer_set_crc(struct omcimsg_layout_t *msg, unsigned int crc)
{
	msg->crc = htonl(crc);
}

extern struct omcimsg_type_attr_t omcimsg_type_attr[32];
extern unsigned int omcimsg_count_g[32];

/* omcimsg.c */
int omcimsg_util_get_attr_from_create(void *attrs, unsigned short classid, unsigned char attr_order, struct attr_value_t *attr);
int omcimsg_util_get_attr_by_mask(void *attrs, int attrs_byte_size, unsigned short classid, unsigned char attr_order, unsigned char mask[2], struct attr_value_t *attr);
int omcimsg_util_set_attr_by_mask(void *attrs, int attrs_byte_size, unsigned short classid, unsigned char attr_order, unsigned char mask[2], struct attr_value_t *attr);
unsigned int omcimsg_util_get_msg_priority(struct omcimsg_layout_t *msg);
int omcimsg_util_is_noack_download_section_msg(struct omcimsg_layout_t *msg);
int omcimsg_util_fill_resp_header_by_orig(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_util_fill_preliminary_error_resp(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg, int reason);
int omcimsg_util_check_err_for_header(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_util_check_attr_bit_exclusive(unsigned char mask[2], unsigned char attr_order);
char *omcimsg_util_classid2name(unsigned short classid);
char *omcimsg_util_classid2section(unsigned short classid);
char *omcimsg_util_msgtype2name(unsigned char msgtype);
int omcimsg_util_msgtype_is_write(unsigned char msgtype);
int omcimsg_util_msgtype_is_inc_mibdatasync(unsigned char msgtype);
int omcimsg_util_msgtype_is_notification(unsigned char msgtype);

/* omcimsg_alarm.c */
unsigned char omcimsg_alarm_get_sequence_num(void);
void omcimsg_alarm_set_sequence_num(unsigned char val);
int omcimsg_alarm_generator(unsigned short classid, unsigned short meid, unsigned char *alarm_mask);

/* omcimsg_avc.c */
int omcimsg_avc_generator(unsigned short classid, unsigned short meid, unsigned char *attr_mask, unsigned char *attr_value);

/* omcimsg_create.c */
int omcimsg_create_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_delete.c */
int omcimsg_delete_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_get_all_alarms.c */
unsigned int omcimsg_get_all_alarms_is_in_progress(void);
unsigned char omcimsg_get_all_alarms_arc(void);
int omcimsg_get_all_alarms_next_stop(void);
int omcimsg_get_all_alarms_next_timeout(void);
int omcimsg_get_all_alarms_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_get_all_alarms_next_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_get.c */
int omcimsg_get_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_get_current_data.c */
int omcimsg_get_current_data_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_get_next.c */
int omcimsg_get_table_start(struct me_t *me, unsigned char attr_order);
int omcimsg_get_table_stop(struct me_t *me);
int omcimsg_get_table_progress(struct me_t *me, unsigned char attr_order, unsigned int cmd_num, struct omcimsg_get_next_response_t *resp_contents);
int omcimsg_get_table_timer_handler(struct fwk_timer_msg_t *timer_msg);
int omcimsg_get_next_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_mib_reset.c */
int omcimsg_mib_reset_clear(void);
int omcimsg_mib_reset_init(void);
int omcimsg_mib_reset_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_mib_reset_clear_is_in_progress(void);
int omcimsg_mib_reset_is_in_progress(void);
int omcimsg_mib_reset_finish_time_till_now(void);
int omcimsg_mib_reset_finish_timestamp(void);

/* omcimsg_mib_upload.c */
int omcimsg_mib_upload_after_reset_get(void);
void omcimsg_mib_upload_after_reset_set(int value);        
int omcimsg_mib_upload_seq_init(void);
int omcimsg_mib_upload_stop_immediately(void);
int omcimsg_mib_upload_is_in_progress(void);
int omcimsg_mib_upload_timer_handler(struct fwk_timer_msg_t *timer_msg);
int omcimsg_mib_upload_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_mib_upload_next_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_mib_upload_seq.c */

/* omcimsg_reboot.c */
int omcimsg_reboot_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_set.c */
int omcimsg_set_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_sw_download.c */
int omcimsg_sw_download_get_diagram_state(void);
void omcimsg_sw_download_set_diagram_state(unsigned char val);
int omcimsg_sw_download_init_diagram_state(struct me_t *me);
int sw_image_state(unsigned char event);
void *start_sw_update(void *data);
int omcimsg_sw_download_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_download_section_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_end_sw_download_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_activate_image_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_commit_image_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);
int omcimsg_sw_download_dump_state(int fd);

/* omcimsg_synchronize_time.c */
int omcimsg_synchronize_time_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

/* omcimsg_test.c */
int omcimsg_test_result_generator(unsigned short classid, unsigned short meid, unsigned short transaction_id, unsigned char *result_content);
int omcimsg_test_handler(struct omcimsg_layout_t *req_msg, struct omcimsg_layout_t *resp_msg);

#endif

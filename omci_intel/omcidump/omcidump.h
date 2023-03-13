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
 * File    : omcidump.h
 *
 ******************************************************************/

#ifndef __OMCIDUMP_H__
#define __OMCIDUMP_H__

#include "omcimsg.h"

#define	OMCIDUMP_DIR_TX		'T'
#define OMCIDUMP_DIR_RX		'R'

/* omcidump_alarm.c */
int omcidump_alarm(int fd, struct omcimsg_layout_t *msg);

/* omcidump_avc.c */
int omcidump_avc(int fd, struct omcimsg_layout_t *msg);

/* omcidump.c */
void omcidump_header(int fd, struct omcimsg_layout_t *msg);
char *omcidump_reason_str(unsigned char reason);
void omcidump_print_raw_with_mark(int fd, struct omcimsg_layout_t *msg, unsigned char dir, unsigned int mark_offset, unsigned mark_len);
void omcidump_print_raw(int fd, struct omcimsg_layout_t *msg, unsigned char dir);
int omcidump_print_raw_to_file(struct omcimsg_layout_t *msg, char *filename, unsigned char dir);
void omcidump_print_msg(int fd, struct omcimsg_layout_t *msg, unsigned char dir);
void omcidump_print_line(int fd, struct omcimsg_layout_t *msg, unsigned char dir, int ms_diff);
void omcidump_print_char(int fd, struct omcimsg_layout_t *msg, unsigned char dir);
void omcidump_print_exec_diff(int fd, int ms_diff);

/* omcidump_create.c */
int omcidump_create(int fd, struct omcimsg_layout_t *msg);
int omcidump_create_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_delete.c */
int omcidump_delete(int fd, struct omcimsg_layout_t *msg);
int omcidump_delete_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_get_all_alarms.c */
int omcidump_get_all_alarms(int fd, struct omcimsg_layout_t *msg);
int omcidump_get_all_alarms_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_get_all_alarms_next(int fd, struct omcimsg_layout_t *msg);
int omcidump_get_all_alarms_next_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_get.c */
int omcidump_get(int fd, struct omcimsg_layout_t *msg);
int omcidump_get_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_get_current_data.c */
int omcidump_get_current_data(int fd, struct omcimsg_layout_t *msg);
int omcidump_get_current_data_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_get_next.c */
int omcidump_get_next(int fd, struct omcimsg_layout_t *msg);
int omcidump_get_next_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_mib_reset.c */
int omcidump_mib_reset(int fd, struct omcimsg_layout_t *msg);
int omcidump_mib_reset_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_mib_upload.c */
int omcidump_mib_upload(int fd, struct omcimsg_layout_t *msg);
int omcidump_mib_upload_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_mib_upload_next(int fd, struct omcimsg_layout_t *msg);
int omcidump_mib_upload_next_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_reboot.c */
int omcidump_reboot(int fd, struct omcimsg_layout_t *msg);
int omcidump_reboot_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_set.c */
int omcidump_set(int fd, struct omcimsg_layout_t *msg);
int omcidump_set_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_sw_download.c */
int omcidump_sw_download(int fd, struct omcimsg_layout_t *msg);
int omcidump_sw_download_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_download_section(int fd, struct omcimsg_layout_t *msg);
int omcidump_download_section_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_end_sw_download(int fd, struct omcimsg_layout_t *msg);
int omcidump_end_sw_download_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_activate_image(int fd, struct omcimsg_layout_t *msg);
int omcidump_activate_image_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_commit_image(int fd, struct omcimsg_layout_t *msg);
int omcidump_commit_image_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_synchronize_time.c */
int omcidump_synchronize_time(int fd, struct omcimsg_layout_t *msg);
int omcidump_synchronize_time_response(int fd, struct omcimsg_layout_t *msg);

/* omcidump_test.c */
int omcidump_test(int fd, struct omcimsg_layout_t *msg);
int omcidump_test_response(int fd, struct omcimsg_layout_t *msg);
int omcidump_test_result(int fd, struct omcimsg_layout_t *msg);

#endif

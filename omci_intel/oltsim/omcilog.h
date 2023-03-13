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
 * File    : omcilog.h
 *
 ******************************************************************/

#ifndef __OMCILOG_H__
#define __OMCILOG_H__

#define OMCILOG_TOTAL_MAX	65536

extern struct omcilog_t *omcilog_ptr[OMCILOG_TOTAL_MAX];	// 0..65535
extern int omcilog_total;

struct omcilog_t {
	unsigned int linenum;
	struct omcimsg_layout_t msg;
	unsigned char result_reason;
};

/* omcilog.c */
int omcilog_load(char *filename);
int omcilog_clearall(void);
int omcilog_save_result_reason(struct omcimsg_layout_t *msg, int current);
int omcilog_list(unsigned short start, unsigned short end);
int omcilog_listw(unsigned short start, unsigned short end);
int omcilog_listerr(unsigned short start, unsigned short end);
int omcilog_listz(unsigned short start, unsigned short end);

#endif

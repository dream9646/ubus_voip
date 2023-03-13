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
 * Module  : tasks
 * File    : restore.h
 *
 ******************************************************************/

#ifndef __RESTORE_H__
#define __RESTORE_H__

/* restore.c */
void restore_append_omcclog(struct omcimsg_layout_t *msg, char *filename);
int restore_load_omcclog(char *filename);

#endif

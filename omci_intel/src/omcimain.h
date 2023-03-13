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
 * Module  : src
 * File    : omcimain.h
 *
 ******************************************************************/

#ifndef __OMCIMAIN_H__
#define __OMCIMAIN_H__

/* omcimain.c */
int omci_is_ready(void);
int omci_init_start(char *env_file_path);
int omci_stop_shutdown(void);
int omci_get_olt_vendor(void);
unsigned char omci_get_omcc_version(void);

#endif

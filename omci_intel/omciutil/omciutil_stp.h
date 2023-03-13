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
 * Module  : omciutil
 * File    : omciutil_stp.h
 *
 ******************************************************************/

#ifndef __OMCIUTIL_STP_H__
#define __OMCIUTIL_STP_H__

/* omciutil_stp.c */
int omciutil_stp_state_by_porttype(int stp_conf_before_omci, int porttype);
int omciutil_stp_set_before_omci(void);
int omciutil_stp_set_after_omci(void);
int omciutil_stp_set_before_shutdown(void);

#endif

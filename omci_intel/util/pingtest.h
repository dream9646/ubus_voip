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
 * Module  : util
 * File    : pingtest.h
 *
 ******************************************************************/

#ifndef __PINGTEST_H__
#define __PINGTEST_H__

/* pingtest.c */
int pingtest(unsigned int dstip, unsigned int srcip, int count, unsigned long *triptime_log, char *errbuff, int errbuff_size);

#endif

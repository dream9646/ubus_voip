/******************************************************************
 *
 * Copyright (C) 2014 5V Technologies Ltd.
 * All Rights Reserved.
 *
 * This program is the proprietary software of 5V Technologies Ltd
 * and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from 5VT.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : 5VT TRAF MANAGER
 * Module  : util
 * File    : prbs.h
 *
 ******************************************************************/

#ifndef __PRBS_H__
#define __PRBS_H__

/* prbs.c */
unsigned char prbs7(unsigned char seed, int len, char *buff);
unsigned short prbs15(unsigned short seed, int len, char *buff);
unsigned int prbs23(unsigned int seed, int len, char *buff);
unsigned int prbs31(unsigned int seed, int len, char *buff);
int prbs7_checker(int len, char *buff);
int prbs15_checker(int len, char *buff);
int prbs23_checker(int len, char *buff);
int prbs31_checker(int len, char *buff);

#endif

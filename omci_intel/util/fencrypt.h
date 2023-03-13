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
 * File    : fencrypt.h
 *
 ******************************************************************/

#ifndef __FENCRYPT_H__
#define __FENCRYPT_H__

/* fencrypt.c */
int fencrypt_encrypt(char *src_file, char *dst_file, char *keystr);
int fencrypt_decrypt(char *src_file, char *dst_file, char *keystr);

#endif

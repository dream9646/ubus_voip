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
 * File    : crc.h
 *
 ******************************************************************/

#ifndef __CRC_H__
#define __CRC_H__

/* crc.c */
unsigned int crc_le_slow(unsigned int crc, unsigned char const *p, size_t len);
unsigned int crc_be_slow(unsigned int crc, unsigned char const *p, size_t len);
unsigned int crc_le_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size);
unsigned int crc_be_update(unsigned int crc_accum, char *data_blk_ptr, int data_blk_size);
int crc_file_check(unsigned int *crc_accum, char *filename, unsigned char is_big_endian);

#endif

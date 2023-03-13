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
 * File    : conv.h
 *
 ******************************************************************/

#ifndef __CONV_H__
#define __CONV_H__

struct conv_str2val_t {
	char *str;
	unsigned int val;
};

struct conv_hex_map_t
{
	char chr;
	int value;
};

/* conv.c */
int conv_str2array(char *s, char *delim, char *array[], int array_size);
int conv_numlist2array(char *str, void *array_base, int element_size, int element_total);
int conv_numlist2alarmmask(char *str, unsigned char *maskptr, int mask_bytes);
int conv_numlist2attrmask(char *str, unsigned char *maskptr, int mask_bytes);
int conv_mask2strnum(unsigned char *mask, unsigned int masklen, char *delim, char *buf, unsigned int len, unsigned char start_flag);
int conv_str_mac_to_mem(char *mem, char *str_mac);
int conv_str_ipv4_to_mem(char *mem, char *str_ipv4);
int conv_hexstr_to_mem(unsigned char *mem, int mem_total, char *str);
int conv_mem_to_hexstr(char *str, int str_size, unsigned char *mem, int mem_size);
int conv_str2value(char *str, struct conv_str2val_t *t);
char *conv_value2str(int value, struct conv_str2val_t *t);
int conv_strlist2mask(char *str, struct conv_str2val_t *t);
int conv_mask2strarray(unsigned int mask, struct conv_str2val_t *t, char *delim, char *buf, unsigned int len);

#endif

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
 * File    : conv.c
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"
#include "conv.h"

int
conv_str2array(char *s, char *delim, char *array[], int array_size)
{
	int dlen = strlen(delim);
	int i = 0;
	char *p;

	while (s != NULL) {
		p = strstr(s, delim);
		if (p != NULL) {
			*p = 0x0;
			array[i] = util_trim(s);
			if (strlen(array[i])>0)
				i++;
			s = p + dlen;
			if (i == array_size)
				break;
		} else {
			break;
		}
	}
	if (s != NULL && i < array_size) {
		array[i] = util_trim(s);
		if (strlen(array[i])>0)
			i++;
	}

	return i;
}

int
conv_numlist2array(char *str, void *array_base, int element_size, int element_total)
{
	char buff[1024];
	char *a[128];
	int n, i;
	
	unsigned char *b1=array_base;
	unsigned short *b2=array_base;
	unsigned int *b4=array_base;
	unsigned long long *b8=array_base;
	
	strncpy(buff, str, 1024);
	buff[1023] = 0;

	n = conv_str2array(buff, ",", a, 128);
	if (n>element_total) 
		n=element_total;
		
	for (i = 0; i < n; i++) {
		switch (element_size) {
		case sizeof(unsigned char):	b1[i]=(unsigned char)util_atoi(a[i]); break;
		case sizeof(unsigned short):	b2[i]=(unsigned short)util_atoi(a[i]); break;
		case sizeof(unsigned int):	b4[i]=(unsigned int)util_atoi(a[i]); break;
		case sizeof(unsigned long long):b8[i]=(unsigned long long)util_atoll(a[i]); break;
		}
	}
	return n;
}

// this func convert list of num str into alarm mask 
int
conv_numlist2alarmmask(char *str, unsigned char *maskptr, int mask_bytes)
{
	char buff[LINE_MAXSIZE];
	char *a[128];
	int n, i, bitnum;
	strncpy(buff, str, LINE_MAXSIZE - 1);
	buff[LINE_MAXSIZE - 1] = 0;
	n = conv_str2array(buff, ",", a, 128);
	for(i = 0; i < n; i++) {
		bitnum = util_atoi(a[i]);
		if(bitnum / 8 >= mask_bytes)
			return -1;
		util_alarm_mask_set_bit(maskptr, bitnum);
	}
	return 0;
}

// this func convert list of num str into attr mask 
int
conv_numlist2attrmask(char *str, unsigned char *maskptr, int mask_bytes)
{
	char buff[LINE_MAXSIZE];
	char *a[128];
	int n, i, bitnum;
	strncpy(buff, str, LINE_MAXSIZE - 1);
	buff[LINE_MAXSIZE - 1] = 0;
	n = conv_str2array(buff, ",", a, 128);
	for(i = 0; i < n; i++) {
		bitnum = util_atoi(a[i]);
		if(bitnum < 1 || (bitnum - 1) / 8 >= mask_bytes)
			return -1;
		util_attr_mask_set_bit(maskptr, bitnum);
	}
	return 0;
}

int
conv_mask2strnum(unsigned char *mask, unsigned int masklen, char *delim,
		 char *buf, unsigned int len, unsigned char start_flag)
{
	int i;
	int size = masklen * 8;
	int slen = 0;
	int delim_size = strlen(delim);
	int first = 1;
	char digit_buf[16];
	memset(buf, 0x00, len);
	for(i = 0; i < size; i++) {
		if(util_alarm_mask_get_bit(mask, i) == 1) {
			if(start_flag) {
				sprintf(digit_buf, "%u", i + 1);
			} else {
				sprintf(digit_buf, "%u", i);
			}
			if(first) {
				if(len < strlen(digit_buf) + 1) {
					return -1;
				} else {
					strcat(buf, digit_buf);
					slen = strlen(digit_buf);
					first = 0;
				}
			} else {
				if(len < (slen + delim_size + strlen(digit_buf) + 1)) {
					return -1;
				} else {
					strcat(buf, delim);
					strcat(buf, digit_buf);
					slen +=
					    (delim_size + strlen(digit_buf));
				}
			}
		}
	}
	return 0;
}

int
conv_str_mac_to_mem(char *mem, char *str_mac)
{
	int i;

	if (str_mac[2] != ':' || str_mac[5] != ':' || str_mac[8] != ':' || str_mac[11] != ':' || str_mac[14] != ':')
		return -1;
	for (i = 0; i < 6; i++)
		mem[i] = (util_aschex2char(str_mac[i * 3]) << 4) + util_aschex2char(str_mac[i * 3 + 1]);

	return 0;
}

// convert ipv4 str to mem in network order (big endian)
int
conv_str_ipv4_to_mem(char *mem, char *str_ipv4)
{
	char buff[16];
	char *str_num[4];
	int i, n;

	strncpy(buff, str_ipv4, 16);
	buff[15] = 0;
	n = conv_str2array(buff, ".", str_num, 4);
	if (n != 4)
		return -1;

	for (i = 0; i < 4; i++)
		mem[i] = atoi(str_num[i]);
	return 0;
}

int
conv_hexstr_to_mem(unsigned char *mem, int mem_total, char *str)
{
	char buff[1024];
	char *a[128];
	int n, i, j;
	
	strncpy(buff, str, 1024);
	buff[1023] = 0;

	n = conv_str2array(buff, " ", a, 128);
	if (n>mem_total) 
		n=mem_total;
		
	for (i=j=0; i < n; i++) {	
		if (strlen(a[i])==2) {
			char *p;
			mem[j]=(unsigned char) strtol(a[i], &p, 16);
			j++;
		}
	}
	return j;
}

int
conv_mem_to_hexstr(char *str, int str_size, unsigned char *mem, int mem_size)
{
	int i;
	char *s=str;


	for (i = 0; i < mem_size; i++) {
		if (str_size-strlen(str)<=1)
			break;
		if (i % 16 == 0)
			s+=snprintf(s, str_size-strlen(str), "%04x", i);
		s+=snprintf(s, str_size-strlen(str), " %02x", mem[i]);
		if (i % 8 == 7)
			s+=snprintf(s, str_size-strlen(str), " ");
		if (i % 16 == 15)
			s+=snprintf(s, str_size-strlen(str), "\n");
	}
	if (i % 16 != 0)
		s+=snprintf(s, str_size-strlen(str), "\n");

	str[str_size-1]=0;
	return strlen(str);
}

// routines that rely on conv_str2valu_t for mapping
int
conv_str2value(char *str, struct conv_str2val_t *t)
{
	int i;
	for(i = 0; t[i].str != NULL; i++) {
		if(strcmp(t[i].str, str) == 0)
			return t[i].val;
	}
	return 0;
}

char *
conv_value2str(int value, struct conv_str2val_t *t)
{
	int i;
	for(i = 0; t[i].str != NULL && t[i].val != 0; i++) {
		if(t[i].val == value)
			return t[i].str;
	}
	return NULL;
}

int
conv_strlist2mask(char *str, struct conv_str2val_t *t)
{
	char buff[LINE_MAXSIZE];
	char *a[128];
	unsigned int mask = 0;
	int n, i, j;
	strncpy(buff, str, LINE_MAXSIZE - 1);
	buff[LINE_MAXSIZE - 1] = 0;
	n = conv_str2array(buff, ",", a, 128);
	for(i = 0; i < n; i++) {
		for(j = 0; t[j].str != NULL; j++) {
			if(strcmp(a[i], t[j].str) == 0) {
				mask |= t[j].val;
				break;
			}
		}
	}
	return mask;
}

int
conv_mask2strarray(unsigned int mask, struct conv_str2val_t *t, char *delim,
		   char *buf, unsigned int len)
{
	int i, j;
	int size = sizeof(mask) * 8;
	int index;
	int slen = 0;
	int delim_size = strlen(delim);
	int first = 1;
	memset(buf, 0x00, len);
	if(t == NULL) {
		return -1;
	}
	for(i = 0; i < size; i++) {
		index = mask & (1 << i);
		for(j = 0; t[j].str != NULL && t[j].val != 0; j++) {
			if(t[j].val == index) {
				if(first) {
					if(len < strlen(t[j].str) + 1) {
						return -1;
					} else {
						strcat(buf, t[j].str);
						slen = strlen(t[j].str);
						first = 0;
					}
				} else {
					if(len <
					   (slen + delim_size +
					    strlen(t[j].str) + 1)) {
						return -1;
					} else {
						strcat(buf, delim);
						strcat(buf, t[j].str);
						slen += (delim_size + strlen(t[j].str));
					}
				}
			}
		}
	}
	return 0;
}
